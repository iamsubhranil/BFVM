#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_CELLS 65536

int memory[MAX_CELLS];

#define SPECIALIZED8(x) \
	x##_1, x##_2, x##_3, x##_4, x##_5, x##_6, x##_7, x##_8, x##_X

enum {
	START = 0, // dummy
	SPECIALIZED8(INCR),
	SPECIALIZED8(DECR),
	SPECIALIZED8(LEFT),
	SPECIALIZED8(RIGHT),
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

void check_repeat(char c, char **source, Array *program, int code_start) {
	int   count = 0; // 0 based count, as c has already occurred
	char *s     = *source;
	while(*s == c) {
		s++;
		count++;
	}
	*source = s;
	switch(count) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7: array_insert(program, code_start + count); break;
		default:
			array_insert(program, code_start + 8);
			array_insert(program, count + 1);
			break;
	}
}

void skipAll(char **s) {
	char *source = *s;
	while(*source) {
		switch(*source) {
			case '>':
			case '<':
			case '+':
			case '-':
			case '.':
			case ',':
			case '[':
			case ']': *s = source; return;
			default: source++; break;
		}
	}
	*s = source;
}

Array *compile(char *source) {
	Array *program = (Array *)malloc(sizeof(Array));
	array_init(program);
	Array jumpstack;
	array_init(&jumpstack);
	while(*source) {
		switch(*source++) {
			case '>': check_repeat('>', &source, program, RIGHT_1); break;
			case '<': check_repeat('<', &source, program, LEFT_1); break;
			case '+': check_repeat('+', &source, program, INCR_1); break;
			case '-': check_repeat('-', &source, program, DECR_1); break;
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
			default: skipAll(&source); break;
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

#define SPECIALIZED8_SINGLE_INS(name, x, op, num) \
	case name##_##num:                            \
		x op## = num;                             \
		break;
#define SPECIALIZED8_INS_X(name, x, op) \
	case name##_X:                      \
		x op## = *code++;               \
		break;
#define SPECIALIZED8_IMPL(name, x, op)       \
	SPECIALIZED8_SINGLE_INS(name, x, op, 1); \
	SPECIALIZED8_SINGLE_INS(name, x, op, 2); \
	SPECIALIZED8_SINGLE_INS(name, x, op, 3); \
	SPECIALIZED8_SINGLE_INS(name, x, op, 4); \
	SPECIALIZED8_SINGLE_INS(name, x, op, 5); \
	SPECIALIZED8_SINGLE_INS(name, x, op, 6); \
	SPECIALIZED8_SINGLE_INS(name, x, op, 7); \
	SPECIALIZED8_SINGLE_INS(name, x, op, 8); \
	SPECIALIZED8_INS_X(name, x, op);

void execute(Array *program) {
	int *code = program->values;
	int *cell = memory;
	while(1) {
		switch(*code++) {
			SPECIALIZED8_IMPL(RIGHT, cell, +);
			SPECIALIZED8_IMPL(LEFT, cell, -);
			SPECIALIZED8_IMPL(INCR, (*cell), +);
			SPECIALIZED8_IMPL(DECR, (*cell), -);
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
