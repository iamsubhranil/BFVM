#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_CELLS 65536

int memory[MAX_CELLS];

enum {
	INCR = 0, // x
	DECR,     // x
	LEFT,
	RIGHT,
	INPUT,
	OUTPUT,
	JMPZ,  // x
	JMPNZ, // x
	END,
};

typedef struct {
	int *values, size, capacity;
} Array;

void array_init(Array *a) {
	a->capacity = a->size = 0;
	a->values             = NULL;
}

void array_insert(Array *a, int value) {
	if(a->size == a->capacity) {
		if(a->capacity == 0)
			a->capacity = 1;
		else
			a->capacity *= 2;
		a->values = (int *)realloc(a->values, sizeof(int) * a->capacity);
	}
	a->values[a->size++] = value;
}

void array_free(Array *a) {
	free(a->values);
	a->capacity = a->size = 0;
	a->values             = NULL;
}

Array *compile(const char *source) {
	Array *program = (Array *)malloc(sizeof(Array));
	array_init(program);
	Array jumpstack;
	array_init(&jumpstack);
	while(*source) {
		switch(*source++) {
			case '>': array_insert(program, RIGHT); break;
			case '<': array_insert(program, LEFT); break;
			case '+': array_insert(program, INCR); break;
			case '-': array_insert(program, DECR); break;
			case '.': array_insert(program, OUTPUT); break;
			case ',': array_insert(program, INPUT); break;
			case '[':
				array_insert(&jumpstack, program->size);
				array_insert(program, JMPZ);
				array_insert(program, 0);
				break;
			case ']': {
				if(jumpstack.size == 0) {
					printf("Unmatched ']'!\n");
					array_free(program);
					array_free(&jumpstack);
					return NULL;
				}
				int lastJump = jumpstack.values[--jumpstack.size];
				array_insert(program, JMPNZ);
				array_insert(program, lastJump - program->size - 1);
				program->values[lastJump + 1] = program->size - lastJump - 2;
				break;
			}
			default: break;
		}
	}
	if(jumpstack.size > 0) {
		printf("Unmatched '['!\n");
		array_free(program);
		array_free(&jumpstack);
		return NULL;
	}
	array_free(&jumpstack);
	array_insert(program, END);
	return program;
}

void execute(Array *program) {
	int *code = program->values;
	int *cell = memory;
	while(1) {
		switch(*code++) {
			case RIGHT: cell++; break;
			case LEFT: cell--; break;
			case INCR: (*cell)++; break;
			case DECR: (*cell)--; break;
			case INPUT: *cell = getchar(); break;
			case OUTPUT: putchar(*cell); break;
			case JMPZ: {
				int where = *code++;
				if(*cell == 0) {
					code += where;
				}
				break;
			}
			case JMPNZ: {
				int where = *code++;
				if(*cell) {
					code += where;
				}
				break;
			}
			case END: {
				return;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		return 0;
	}
	FILE *f = fopen(argv[1], "rb");
	if(f == NULL) {
		printf("Unable to open the file!\n");
		return 1;
	}
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *program = (char *)malloc(len + 1);
	fread(program, len, 1, f);
	fclose(f);
	program[len] = 0;
	// printf("%s\n", program);
	Array *compiled = compile(program);
	free(program);
	if(compiled == NULL) {
		printf("Error occurred while compilation!\n");
		return 2;
	}
	clock_t start = clock();
	execute(compiled);
	printf("Elapsed: %fs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
	array_free(compiled);
	free(compiled);
}
