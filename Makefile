bfvm: bfvm.c
	$(CC) bfvm.c -O3 -o bfvm

profile: bfvm.c
	$(CC) bfvm.c -O2 -g3 -o bfvm
