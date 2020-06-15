bfvm: bfvm.c
	$(CC) bfvm.c -O3 -Wall -Wextra -o bfvm

profile: bfvm.c
	$(CC) bfvm.c -O2 -Wall -Wextra -g3 -o bfvm
