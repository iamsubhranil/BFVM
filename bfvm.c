#include <stdbool.h>
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

bool skipAll(char **s) {
	char *source  = *s;
	bool  skipped = false;
	while(*source) {
		switch(*source) {
			case '>':
			case '<':
			case '+':
			case '-':
			case '.':
			case ',':
			case '[':
			case ']': *s = source; return skipped;
			default:
				source++;
				skipped = true;
				break;
		}
	}
	*s = source;
	return skipped;
}

void check_repeat(char c, char **source, Array *program, int code_start) {
	int   count = 0; // 0 based count, as c has already occurred
	char *s     = *source;
	while(*s == c || (skipAll(&s) && *s == c)) {
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
				array_insert(program, JMPZ);
				array_insert(program, 0);
				array_insert(&jumpstack, program->size);
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
				program->values[lastJump - 1] = program->size - lastJump;
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

#define BFVM_COMPUTED_GOTO

#define next_code() (*(code++))
#ifdef BFVM_COMPUTED_GOTO
#define LOOP() while(1)
#define SWITCH() \
	{ goto *dispatchTable[next_code()]; }
#define CASE(x) LABEL_##x
#define DISPATCH() goto *dispatchTable[next_code()]
#else
#define LOOP() while(1)
#define SWITCH() switch(next_code())
#define CASE(x) case x
#define DISPATCH() break
#endif

#define SPECIALIZED8_SINGLE_INS(name, x, op, num) \
	CASE(name##_##num) : {                        \
		x op## = num;                             \
		DISPATCH();                               \
	}
#define SPECIALIZED8_INS_X(name, x, op) \
	CASE(name##_X) : {                  \
		x op## = *code++;               \
		DISPATCH();                     \
	}
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
#ifdef BFVM_COMPUTED_GOTO
#define SPECIALIZED8_LABEL(name, x) &&LABEL_##name##_##x
#define SPECIALIZED8_GOTO(x)                                \
	SPECIALIZED8_LABEL(x, 1), SPECIALIZED8_LABEL(x, 2),     \
	    SPECIALIZED8_LABEL(x, 3), SPECIALIZED8_LABEL(x, 4), \
	    SPECIALIZED8_LABEL(x, 5), SPECIALIZED8_LABEL(x, 6), \
	    SPECIALIZED8_LABEL(x, 7), SPECIALIZED8_LABEL(x, 8), &&LABEL_##x##_X

	void *dispatchTable[] = {&&LABEL_START,
	                         SPECIALIZED8_GOTO(INCR),
	                         SPECIALIZED8_GOTO(DECR),
	                         SPECIALIZED8_GOTO(LEFT),
	                         SPECIALIZED8_GOTO(RIGHT),
	                         &&LABEL_INPUT,
	                         &&LABEL_OUTPUT,
	                         &&LABEL_JMPZ,
	                         &&LABEL_JMPNZ,
	                         &&LABEL_END};
#endif
	LOOP() {
		SWITCH() {
			SPECIALIZED8_IMPL(INCR, (*cell), +);
			SPECIALIZED8_IMPL(DECR, (*cell), -);
			SPECIALIZED8_IMPL(LEFT, cell, -);
			SPECIALIZED8_IMPL(RIGHT, cell, +);
			CASE(INPUT) : {
				*cell = getchar();
				DISPATCH();
			}
			CASE(OUTPUT) : {
				putchar(*cell);
				DISPATCH();
			}
			CASE(JMPZ) : {
				int where = next_code();
				if(*cell == 0) {
					code += where;
				}
				DISPATCH();
			}
			CASE(JMPNZ) : {
				int where = next_code();
				if(*cell) {
					code += where;
				}
				DISPATCH();
			}
			CASE(END) : { return; }
			CASE(START) : {
				DISPATCH(); // dummy
			}
		}
	}
}

#define SPECIALIZED8_SINGLE_LINE(name, x, op, num) \
	case name##_##num:                             \
		fprintf(f, #x " " #op "="                  \
		              " " #num ";\n");             \
		break;
#define SPECIALIZED8_LINE_X(name, x, op) \
	case name##_X:                       \
		fprintf(f,                       \
		        #x " " #op "="           \
		           " "                   \
		           "%d;\n",              \
		        *program++);             \
		break;
#define SPECIALIZED8_LINE(name, x, op)        \
	SPECIALIZED8_SINGLE_LINE(name, x, op, 1); \
	SPECIALIZED8_SINGLE_LINE(name, x, op, 2); \
	SPECIALIZED8_SINGLE_LINE(name, x, op, 3); \
	SPECIALIZED8_SINGLE_LINE(name, x, op, 4); \
	SPECIALIZED8_SINGLE_LINE(name, x, op, 5); \
	SPECIALIZED8_SINGLE_LINE(name, x, op, 6); \
	SPECIALIZED8_SINGLE_LINE(name, x, op, 7); \
	SPECIALIZED8_SINGLE_LINE(name, x, op, 8); \
	SPECIALIZED8_LINE_X(name, x, op);

void transpile_rec(FILE *f, int **pgm, int level) {
	int *program = *pgm;
	while(1) {
		for(int i = 0; i < level - (*program == END || *program == JMPNZ);
		    i++) {
			fprintf(f, "\t");
		}
		switch(*program++) {
			SPECIALIZED8_LINE(INCR, (*cell), +);
			SPECIALIZED8_LINE(DECR, (*cell), -);
			SPECIALIZED8_LINE(LEFT, cell, -);
			SPECIALIZED8_LINE(RIGHT, cell, +);
			case INPUT: fprintf(f, "*cell = getchar();\n"); break;
			case OUTPUT: fprintf(f, "putchar(*cell);\n"); break;
			case JMPZ:
				program++; // ignore
				fprintf(f, "while(*cell) {\n");
				transpile_rec(f, &program, level + 1);
				fprintf(f, "}\n");
				break;
			case JMPNZ:
				program++; // ignore
				*pgm = program;
				return;
			case END: return;
			case START: return;
		}
	}
}

void transpile(const char *filename, Array *program) {
	// change all the dots to underscore
	for(char *c = (char *)filename; *c; c++) {
		if(*c == '.') {
			*(c + 1) = 'c';
			*(c + 2) = 0;
			break;
		}
	}
	FILE *f = fopen(filename, "wb");
	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "#include <time.h>\n\n");
	fprintf(f, "int memory[%d];\n\n", MAX_CELLS);
	fprintf(f, "int main() {\n");
	fprintf(f, "\tint *cell = memory;\n");
	fprintf(f, "\tclock_t start = clock();\n");
	int *pgm = program->values;
	transpile_rec(f, &pgm, 1);
	fprintf(f, "\tprintf(\"\\nElapsed: %%fs\\n\",(double)(clock() - "
	           "start)/CLOCKS_PER_SEC);\n");
	fprintf(f, "\treturn 0;\n");
	fprintf(f, "}");
	fclose(f);
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
	transpile(argv[1], compiled);
	clock_t start = clock();
	execute(compiled);
	printf("Elapsed: %fs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
	array_free(compiled);
	free(compiled);
}
