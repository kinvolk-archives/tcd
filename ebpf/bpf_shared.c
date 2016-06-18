/* Code borrowed from
 * https://git.kernel.org/cgit/linux/kernel/git/shemminger/iproute2.git/tree/examples/bpf/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/in.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/if_tunnel.h>
#include <linux/filter.h>
#include <linux/bpf.h>

#include "bpf_api.h"

/* Other, misc stuff. */
#define IP_MF			0x2000
#define IP_OFFSET		0x1FFF

struct bpf_elf_map __section_maps map_sh = {
	.type		= BPF_MAP_TYPE_ARRAY,
	.size_key	= sizeof(uint32_t),
	.size_value	= sizeof(uint32_t),
	.pinning	= PIN_OBJECT_NS, /* or PIN_GLOBAL_NS, or PIN_NONE */
	.max_elem	= 1,
};

/* Helper functions and definitions for the flow dissector used by the
 * example classifier. This resembles the kernel's flow dissector to
 * some extend and is just used as an example to show what's possible
 * with eBPF.
 */

struct vlan_hdr {
	__be16 h_vlan_TCI;
	__be16 h_vlan_encapsulated_proto;
};

struct flow_keys {
	__u32 src;
	__u32 dst;
	union {
		__u32 ports;
		__u16 port16[2];
	};
	__s32 th_off;
	__u8 ip_proto;
};

static __inline__ int flow_ports_offset(__u8 ip_proto)
{
	switch (ip_proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
	case IPPROTO_DCCP:
	case IPPROTO_ESP:
	case IPPROTO_SCTP:
	case IPPROTO_UDPLITE:
	default:
		return 0;
	case IPPROTO_AH:
		return 4;
	}
}

static __inline__ bool flow_is_frag(struct __sk_buff *skb, int nh_off)
{
	return !!(load_half(skb, nh_off + offsetof(struct iphdr, frag_off)) &
		  (IP_MF | IP_OFFSET));
}

static __inline__ int flow_parse_ipv4(struct __sk_buff *skb, int nh_off,
				      __u8 *ip_proto, struct flow_keys *flow)
{
	__u8 ip_ver_len;

	if (unlikely(flow_is_frag(skb, nh_off)))
		*ip_proto = 0;
	else
		*ip_proto = load_byte(skb, nh_off + offsetof(struct iphdr,
							     protocol));
	if (*ip_proto != IPPROTO_GRE) {
		flow->src = load_word(skb, nh_off + offsetof(struct iphdr, saddr));
		flow->dst = load_word(skb, nh_off + offsetof(struct iphdr, daddr));
	}

	ip_ver_len = load_byte(skb, nh_off + 0 /* offsetof(struct iphdr, ihl) */);
	if (likely(ip_ver_len == 0x45))
		nh_off += 20;
	else
		nh_off += (ip_ver_len & 0xF) << 2;

	return nh_off;
}

static __inline__ __u32 flow_addr_hash_ipv6(struct __sk_buff *skb, int off)
{
	__u32 w0 = load_word(skb, off);
	__u32 w1 = load_word(skb, off + sizeof(w0));
	__u32 w2 = load_word(skb, off + sizeof(w0) * 2);
	__u32 w3 = load_word(skb, off + sizeof(w0) * 3);

	return w0 ^ w1 ^ w2 ^ w3;
}

static __inline__ int flow_parse_ipv6(struct __sk_buff *skb, int nh_off,
				      __u8 *ip_proto, struct flow_keys *flow)
{
	*ip_proto = load_byte(skb, nh_off + offsetof(struct ipv6hdr, nexthdr));

	flow->src = flow_addr_hash_ipv6(skb, nh_off + offsetof(struct ipv6hdr, saddr));
	flow->dst = flow_addr_hash_ipv6(skb, nh_off + offsetof(struct ipv6hdr, daddr));

	return nh_off + sizeof(struct ipv6hdr);
}

static __inline__ bool flow_dissector(struct __sk_buff *skb,
				      struct flow_keys *flow)
{
	int poff, nh_off = BPF_LL_OFF + ETH_HLEN;
	__be16 proto = skb->protocol;
	__u8 ip_proto;

	/* TODO: check for skb->vlan_tci, skb->vlan_proto first */
	if (proto == htons(ETH_P_8021AD)) {
		proto = load_half(skb, nh_off +
				  offsetof(struct vlan_hdr, h_vlan_encapsulated_proto));
		nh_off += sizeof(struct vlan_hdr);
	}
	if (proto == htons(ETH_P_8021Q)) {
		proto = load_half(skb, nh_off +
				  offsetof(struct vlan_hdr, h_vlan_encapsulated_proto));
		nh_off += sizeof(struct vlan_hdr);
	}

	if (likely(proto == htons(ETH_P_IP)))
		nh_off = flow_parse_ipv4(skb, nh_off, &ip_proto, flow);
	else if (proto == htons(ETH_P_IPV6))
		nh_off = flow_parse_ipv6(skb, nh_off, &ip_proto, flow);
	else
		return false;

	switch (ip_proto) {
	case IPPROTO_GRE: {
		struct gre_hdr {
			__be16 flags;
			__be16 proto;
		};

		__u16 gre_flags = load_half(skb, nh_off +
					    offsetof(struct gre_hdr, flags));
		__u16 gre_proto = load_half(skb, nh_off +
					    offsetof(struct gre_hdr, proto));

		if (gre_flags & (GRE_VERSION | GRE_ROUTING))
			break;

		nh_off += 4;
		if (gre_flags & GRE_CSUM)
			nh_off += 4;
		if (gre_flags & GRE_KEY)
			nh_off += 4;
		if (gre_flags & GRE_SEQ)
			nh_off += 4;

		if (gre_proto == ETH_P_8021Q) {
			gre_proto = load_half(skb, nh_off +
					      offsetof(struct vlan_hdr,
						       h_vlan_encapsulated_proto));
			nh_off += sizeof(struct vlan_hdr);
		}
		if (gre_proto == ETH_P_IP)
			nh_off = flow_parse_ipv4(skb, nh_off, &ip_proto, flow);
		else if (gre_proto == ETH_P_IPV6)
			nh_off = flow_parse_ipv6(skb, nh_off, &ip_proto, flow);
		else
			return false;
		break;
	}
	case IPPROTO_IPIP:
		nh_off = flow_parse_ipv4(skb, nh_off, &ip_proto, flow);
		break;
	case IPPROTO_IPV6:
		nh_off = flow_parse_ipv6(skb, nh_off, &ip_proto, flow);
	default:
		break;
	}

	nh_off += flow_ports_offset(ip_proto);

	flow->ports = load_word(skb, nh_off);
	flow->th_off = nh_off;
	flow->ip_proto = ip_proto;

	return true;
}


__section("egress")
int emain(struct __sk_buff *skb)
{
	int key = 0, *val;
	/* unsigned division are ok in BPF. But signed divisions are unsupported. */
	unsigned int v;
	struct flow_keys flow;

	if (!flow_dissector(skb, &flow))
		return BPF_H_DEFAULT; /* Not recognised. */
	printt("flow ports: %d %d\n", flow.port16[0], flow.port16[1]),

	val = map_lookup_elem(&map_sh, &key);
	if (val) {
		printt("egress: map val: %d\n", *val);
		lock_xadd(val, 1);
	}

	if (flow.port16[0] == 80) {
		v = 2; // http slow
	} else {
		v = 3; // https fast
	}
/*
	v = 1
	v = v % 100 + 1;
	if (v > 3) {
		v = 0;
	}
*/
	printt("egress: classid: %x\n", TC_H_MAKE(1<<16, v));
	return TC_H_MAKE(1<<16, v);
}

__section("ingress")
int imain(struct __sk_buff *skb)
{
	int key = 0, *val;

	val = map_lookup_elem(&map_sh, &key);
	if (val) {
		printt("ingress: map val: %d\n", *val);
		lock_xadd(val, 1);
	}

	return BPF_H_DEFAULT;
}

BPF_LICENSE("GPL");
