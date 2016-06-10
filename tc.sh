#!/bin/sh

# Instead of recompiling tcd for every change, tcd could call this script
# sudo cp tc.sh /run/tcd/tc.sh

set -e
set -x

tc qdisc del dev eth0 root || true

tc -s -d qdisc show dev eth0

tc qdisc add dev eth0 handle 1:0 root htb r2q 2 default 1

tc class add dev eth0 parent 1:0 classid 1:1 htb rate 80000bps
tc qdisc add dev eth0 handle 2:0 parent 1:1 netem

tc class add dev eth0 parent 1:0 classid 1:2 htb rate 80000bps
tc qdisc add dev eth0 handle 3:0 parent 1:2 netem

tc class add dev eth0 parent 1:0 classid 1:3 htb rate 80000bps
tc qdisc add dev eth0 handle 4:0 parent 1:3 netem

tc filter replace dev eth0 parent 1: bpf obj bpf_shared.o sec egress verbose

# ingress
tc qdisc add dev eth0 handle ffff: ingress
tc filter replace dev eth0 parent ffff: bpf obj bpf_shared.o sec ingress verbose

