override CFLAGS += -Wall -Wextra

bfvm: bfvm.c
	$(CC) bfvm.c -O3 -Wall -Wextra -o bfvm

debug: bfvm.c
	$(CC) bfvm.c -O0 -g3 -Wall -Wextra -DDEBUG -o bfvm

mandeloptcheck: mandelbrot.c
	$(CC) mandelbrot.c -O0 -g3 $(CFLAGS) -o mandel

profile: bfvm.c
	$(CC) bfvm.c -O2 -Wall -Wextra -g3 -o bfvm

pgo: bfvm.c
	rm -f bfvm *.gcda *.gcno
	$(CC) bfvm.c -O3 -fprofile-generate -march=native -flto -o bfvm
	for filename in [^egi]*.{b,bf}; do `[ -f $$filename.in ] && echo "./bfvm $$filename $$filename.in" || echo "./bfvm $$filename"`; done
	$(CC) bfvm.c -O3 -fprofile-use -march=native -flto -o bfvm
