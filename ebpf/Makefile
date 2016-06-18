all: bpf_shared.o

bpf_shared.o: bpf_shared.c
	clang -target bpf -O2  -o bpf_shared.o -c bpf_shared.c

clean:
	rm -f *.o
