#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_CELLS 65536

char memory[MAX_CELLS];

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
	JMPZ,       // x
	JMPNZ,      // x
	RESET_CELL, // reset present cell's value to 0
	END,
};

typedef struct {
	int idx;
	int value;
} Change;

#define ARRAY(type, name)                                                  \
	typedef struct {                                                       \
		type *values;                                                      \
		int   size, capacity;                                              \
	} name##Array;                                                         \
                                                                           \
	void type##_array_init(name##Array *a) {                               \
		a->capacity = a->size = 0;                                         \
		a->values             = NULL;                                      \
	}                                                                      \
                                                                           \
	void type##_array_resize(name##Array *a, int capacity) {               \
		a->values   = (type *)realloc(a->values, sizeof(type) * capacity); \
		a->capacity = capacity;                                            \
	}                                                                      \
                                                                           \
	void type##_array_insert(name##Array *a, type value) {                 \
		if(a->size == a->capacity) {                                       \
			if(a->capacity == 0)                                           \
				a->capacity = 1;                                           \
			else                                                           \
				a->capacity *= 2;                                          \
			type##_array_resize(a, a->capacity);                           \
		}                                                                  \
		a->values[a->size++] = value;                                      \
	}                                                                      \
                                                                           \
	void type##_array_free(name##Array *a) {                               \
		free(a->values);                                                   \
		a->capacity = a->size = 0;                                         \
		a->values             = NULL;                                      \
	}

ARRAY(int, Int);
ARRAY(Change, Change);

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

void check_repeat(char c, char **source, IntArray *program, int code_start) {
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
		case 7: int_array_insert(program, code_start + count); break;
		default:
			int_array_insert(program, code_start + 8);
			int_array_insert(program, count + 1);
			break;
	}
}

IntArray *compile(char *source) {
	IntArray *program = (IntArray *)malloc(sizeof(IntArray));
	int_array_init(program);
	IntArray jumpstack;
	int_array_init(&jumpstack);
	while(*source) {
		switch(*source++) {
			case '>': check_repeat('>', &source, program, RIGHT_1); break;
			case '<': check_repeat('<', &source, program, LEFT_1); break;
			case '+': check_repeat('+', &source, program, INCR_1); break;
			case '-': check_repeat('-', &source, program, DECR_1); break;
			case '.': int_array_insert(program, OUTPUT); break;
			case ',': int_array_insert(program, INPUT); break;
			case '[': {
				char *bak = source;
				skipAll(&bak);
				if(*bak == '-' || *bak == '+') {
					bak++;
					skipAll(&bak);
					if(*bak == ']') {
						bak++;
						source = bak;
						int_array_insert(program, RESET_CELL);
						break;
					}
				}
				int_array_insert(program, JMPZ);
				int_array_insert(program, 0);
				int_array_insert(&jumpstack, program->size);
				break;
			}
			case ']': {
				if(jumpstack.size == 0) {
					printf("Unmatched ']'!\n");
					int_array_free(program);
					int_array_free(&jumpstack);
					return NULL;
				}
				int lastJump = jumpstack.values[--jumpstack.size];
				int_array_insert(program, JMPNZ);
				int_array_insert(program, lastJump - program->size - 1);
				program->values[lastJump - 1] = program->size - lastJump;
				break;
			}
			default: skipAll(&source); break;
		}
	}
	if(jumpstack.size > 0) {
		printf("Unmatched '['!\n");
		int_array_free(program);
		int_array_free(&jumpstack);
		return NULL;
	}
	int_array_free(&jumpstack);
	int_array_insert(program, END);
	return program;
}

#define SPECIALIZED8_SINGLE_LINE_DEBUG(name, num) \
	case name##_##num:                            \
		printf(#name "\t\t" #num);                \
		break;
#define SPECIALIZED8_LINE_X_DEBUG(name)           \
	case name##_X:                                \
		printf(#name "_X\t\t%d", *(program + 1)); \
		return ip + 2;
#define SPECIALIZED8_DEBUG(name)             \
	SPECIALIZED8_SINGLE_LINE_DEBUG(name, 1); \
	SPECIALIZED8_SINGLE_LINE_DEBUG(name, 2); \
	SPECIALIZED8_SINGLE_LINE_DEBUG(name, 3); \
	SPECIALIZED8_SINGLE_LINE_DEBUG(name, 4); \
	SPECIALIZED8_SINGLE_LINE_DEBUG(name, 5); \
	SPECIALIZED8_SINGLE_LINE_DEBUG(name, 6); \
	SPECIALIZED8_SINGLE_LINE_DEBUG(name, 7); \
	SPECIALIZED8_SINGLE_LINE_DEBUG(name, 8); \
	SPECIALIZED8_LINE_X_DEBUG(name);

int disassemble_single(int ip, int *program) {
	printf("%6d: ", ip);
	switch(*program) {
		SPECIALIZED8_DEBUG(INCR);
		SPECIALIZED8_DEBUG(DECR);
		SPECIALIZED8_DEBUG(LEFT);
		SPECIALIZED8_DEBUG(RIGHT);
		case INPUT: printf("INPUT"); break;
		case OUTPUT: printf("OUTPUT"); break;
		case JMPZ: printf("JMPZ\t\t%d", *(program + 1)); return ip + 2;
		case JMPNZ: printf("JMPNZ\t\t%d", *(program + 1)); return ip + 2;
		case RESET_CELL: printf("RESET_CELL"); break;
		case END: printf("END"); break;
		case START: printf("START"); break;
	}
	return ip + 1;
}

void disassemble_all(IntArray *pgm) {
	int *program = pgm->values;
	for(int i = 0; i < pgm->size;) {
		i = disassemble_single(i, &program[i]);
		printf("\n");
	}
	printf("\n");
}

#ifndef DEBUG
#define BFVM_COMPUTED_GOTO
#endif

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

void execute(IntArray *program, FILE *stream) {
	int * code = program->values;
	char *cell = memory;
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
	                         &&LABEL_RESET_CELL,
	                         &&LABEL_END};
#endif
	LOOP() {
#ifdef DEBUG
		disassemble_single(code - program->values, code);
		printf("\t\tCell: %ld\t\tValue: %u\n", cell - memory, *cell);
#endif
		SWITCH() {
			SPECIALIZED8_IMPL(INCR, (*cell), +);
			SPECIALIZED8_IMPL(DECR, (*cell), -);
			SPECIALIZED8_IMPL(LEFT, cell, -);
			SPECIALIZED8_IMPL(RIGHT, cell, +);
			CASE(INPUT) : {
				*cell = fgetc(stream);
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
			CASE(RESET_CELL) : {
				*cell = 0;
				DISPATCH();
			}
			CASE(END) : { return; }
			CASE(START) : {
				DISPATCH(); // dummy
			}
		}
	}
}

void print_indent(FILE *f, int level) {
	for(int i = 0; i < level; i++) fprintf(f, "\t");
}

void print_signaware(FILE *f, const char *lhs, int rhs) {
	if(rhs < 0) {
		fprintf(f, "%s -= %d;\n", lhs, -rhs);
	} else {
		fprintf(f, "%s += %d;\n", lhs, rhs);
	}
}

void print_header(FILE *f, int level, bool *headerPrinted, bool fromJump) {
	if(fromJump && !*headerPrinted) {
		print_indent(f, level - 1);
		fprintf(f, "while(*cell) {\n");
		*headerPrinted = true;
	}
}

void dump_changes(FILE *f, ChangeArray *totalChange, int pointerShift,
                  bool isPureLoop, int level, bool *headerPrinted,
                  bool fromJump) {
	// isPureLoop = false;
	// if this is pure loop and the net pointer shift is 0,
	// we have some interesting possibilites
	if(isPureLoop && pointerShift == 0) {
		// decrease the level because we are not gonna
		// insert a loop anymore
		level--;
		// get the total change of the value at present cell
		int loopVarChange = 0;
		for(int i = 0; i < totalChange->size; i++) {
			if(totalChange->values[i].idx == 0) {
				loopVarChange = totalChange->values[i].value;
				break;
			}
		}
		print_indent(f, level);
		// for this loop to break, the value at cell[0] must reach
		// to zero. so we assume that it does with the given delta

		// if the change is negative, then the number of
		// times this loop is gonna continue is value / change
		if(loopVarChange < 0) {
			fprintf(f, "change = cell[0] / %d;\n", -loopVarChange);
		} else {
			// if the change is positive, it's gonna take (256 - value) / change
			fprintf(f, "change = (256 - cell[0]) / %d;\n", loopVarChange);
		}
		// now for the rest of the cells, the total change will be
		// (change * delta of the particular cell)
		for(int i = 0; i < totalChange->size; i++) {
			if(totalChange->values[i].idx != 0) {
				print_indent(f, level);
				fprintf(f, "cell[%d] += (change * %d);\n",
				        totalChange->values[i].idx,
				        totalChange->values[i].value);
			}
		}
		// we explicitly set the cell to 0
		print_indent(f, level);
		fprintf(f, "cell[0] = 0;\n");
		// release the array
		Change_array_free(totalChange);
		// we're done
		return;
	}
	print_header(f, level, headerPrinted, fromJump);
	// dump the changes
	for(int i = 0; i < totalChange->size; i++) {
		print_indent(f, level);
		fprintf(f, "cell[%d]", totalChange->values[i].idx);
		print_signaware(f, "", totalChange->values[i].value);
#ifdef DEBUG
		// print the value of the cell
		fprintf(f, "#ifdef DEBUG\n");
		print_indent(f, level);
		fprintf(
		    f,
		    "printf(\"\t\tCell: %%ld\\t\\tValue: %%u\\n\", cell + %d - memory, "
		    "cell[%d]);\n",
		    totalChange->values[i].idx, totalChange->values[i].idx);
		fprintf(f, "#endif\n");
#endif
	}
	if(pointerShift != 0) {
		print_indent(f, level);
		print_signaware(f, "cell", pointerShift);
	}
#ifdef DEBUG
	if(pointerShift != 0 || totalChange->size == 0) {
		// print the value of the cell
		fprintf(f, "#ifdef DEBUG\n");
		print_indent(f, level);
		fprintf(f,
		        "printf(\"\t\tCell: %%ld\\t\\tValue: %%u\\n\", cell - memory, "
		        "*cell);\n");
		fprintf(f, "#endif\n");
	}
#endif
	// release the array
	Change_array_free(totalChange);
}

void apply_total_change(ChangeArray *ca, int currentPointer, int totalChange) {
	for(int i = 0; i < ca->size; i++) {
		if(ca->values[i].idx == currentPointer) {
			ca->values[i].value += totalChange;
			return;
		}
	}
	Change_array_insert(ca, (Change){currentPointer, totalChange});
}

#define SPECIALIZED8_SINGLE_LINE2(name, x, op, num)                   \
	case name##_##num:                                                \
		apply_total_change(x, currentPointer - startPointer, op num); \
		break;
#define SPECIALIZED8_LINE_X2(name, x, op)                    \
	case name##_X:                                           \
		apply_total_change(x, currentPointer - startPointer, \
		                   op * (program++));                \
		break;
#define SPECIALIZED8_LINE2(name, x, op)        \
	SPECIALIZED8_SINGLE_LINE2(name, x, op, 1); \
	SPECIALIZED8_SINGLE_LINE2(name, x, op, 2); \
	SPECIALIZED8_SINGLE_LINE2(name, x, op, 3); \
	SPECIALIZED8_SINGLE_LINE2(name, x, op, 4); \
	SPECIALIZED8_SINGLE_LINE2(name, x, op, 5); \
	SPECIALIZED8_SINGLE_LINE2(name, x, op, 6); \
	SPECIALIZED8_SINGLE_LINE2(name, x, op, 7); \
	SPECIALIZED8_SINGLE_LINE2(name, x, op, 8); \
	SPECIALIZED8_LINE_X2(name, x, op);

#define SPECIALIZED8_SINGLE_LINE(name, x, op, num) \
	case name##_##num:                             \
		x op## = num;                              \
		break;
#define SPECIALIZED8_LINE_X(name, x, op) \
	case name##_X:                       \
		x op## = *program++;             \
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

#ifdef DEBUG
#define SPECIALIZED8_SINGLE_LINE_PRINTF(name, num)            \
	case name##_##num:                                        \
		fprintf(f, "printf(\"" #name "\\t\\t" #num "\");\n"); \
		break;
#define SPECIALIZED8_LINE_X_PRINTF(name)                                       \
	case name##_X:                                                             \
		fprintf(f, "printf(\"" #name "_X\\t\\t%%d\", %d);\n", *(program + 1)); \
		break;
#define SPECIALIZED8_PRINTF(name)             \
	SPECIALIZED8_SINGLE_LINE_PRINTF(name, 1); \
	SPECIALIZED8_SINGLE_LINE_PRINTF(name, 2); \
	SPECIALIZED8_SINGLE_LINE_PRINTF(name, 3); \
	SPECIALIZED8_SINGLE_LINE_PRINTF(name, 4); \
	SPECIALIZED8_SINGLE_LINE_PRINTF(name, 5); \
	SPECIALIZED8_SINGLE_LINE_PRINTF(name, 6); \
	SPECIALIZED8_SINGLE_LINE_PRINTF(name, 7); \
	SPECIALIZED8_SINGLE_LINE_PRINTF(name, 8); \
	SPECIALIZED8_LINE_X_PRINTF(name);

void fdisassemble(FILE *f, int ip, int *program, int level) {
	fprintf(f, "#ifdef DEBUG\n");
	print_indent(f, level);
	fprintf(f, "printf(\"%%6d: \", %d);\n", ip);
	print_indent(f, level);
	switch(*program) {
		SPECIALIZED8_PRINTF(INCR);
		SPECIALIZED8_PRINTF(DECR);
		SPECIALIZED8_PRINTF(LEFT);
		SPECIALIZED8_PRINTF(RIGHT);
		case INPUT: fprintf(f, "printf(\"INPUT\");\n"); break;
		case OUTPUT: fprintf(f, "printf(\"OUTPUT\");\n"); break;
		case JMPZ:
			fprintf(f, "printf(\"JMPZ\\t\\t%%d\", %d);\n", *(program + 1));
			break;
		case JMPNZ:
			fprintf(f, "printf(\"JMPNZ\\t\\t%%d\", %d);\n", *(program + 1));
			break;
		case RESET_CELL: fprintf(f, "printf(\"RESET_CELL\");\n"); break;
		case END: fprintf(f, "printf(\"END\");\n"); break;
		case START: fprintf(f, "printf(\"START\");\n"); break;
	}
	fprintf(f, "#endif\n");
}

#endif

// returns the current cell pointer
int transpile_rec(FILE *f, int *source, int **pgm, int startPointer, int level,
                  bool fromJump) {
	bool headerPrinted = !fromJump;
	int *program       = *pgm;
	// Pointer to the present cell, with respect to
	// the start of the loop
	int currentPointer = startPointer;
	// Each cell of the array holds the total increment
	// / decrement of the cell at pointer i with respect
	// to the present loop
	ChangeArray totalChange;
	Change_array_init(&totalChange);
	// denotes if this is a pure loop, i.e. a loop containing
	// no subloops inside
	bool isPureLoop = true;
	int  lastOpcode = END;
	while(1) {
		int bakOpcode = *program;
		// flush out the changes as required
		switch(*program) {
			case INPUT:
				print_header(f, level, &headerPrinted, fromJump);
				print_indent(f, level);
				break;
			case OUTPUT:
			case RESET_CELL:
			case START:
			case JMPZ: isPureLoop = false;
			case END:
			case JMPNZ:
				dump_changes(f, &totalChange, currentPointer - startPointer,
				             isPureLoop, level, &headerPrinted, fromJump);
				if(headerPrinted && *program != JMPZ)
					print_indent(f, level -
					                    (*program == END || *program == JMPNZ));
				startPointer = currentPointer;
				break;
		}
#ifdef DEBUG
		fdisassemble(f, program - source, program, level);
#endif
		switch(*program++) {
			SPECIALIZED8_LINE2(INCR, &totalChange, +);
			SPECIALIZED8_LINE2(DECR, &totalChange, -);
			SPECIALIZED8_LINE(LEFT, currentPointer, -);
			SPECIALIZED8_LINE(RIGHT, currentPointer, +);
			case INPUT:
				fprintf(f, "cell[%d] = getchar();\n",
				        currentPointer - startPointer);
				break;
			case OUTPUT: fprintf(f, "putchar(*cell);\n");
#ifdef DEBUG
				fprintf(f, "#ifdef DEBUG\n");
				print_indent(f, level);
				fprintf(f, "fflush(stdout);\n");
				fprintf(f, "#endif\n");
#endif
				break;
			case JMPZ:
				isPureLoop = false;
				program++; // ignore
				startPointer = currentPointer = transpile_rec(
				    f, source, &program, currentPointer, level + 1, true);
				// reinit the array
				Change_array_init(&totalChange);
				break;
			case JMPNZ:
				program++; // ignore
				*pgm = program;
				if(headerPrinted)
					fprintf(f, "}\n");
				return currentPointer;
			case RESET_CELL:
				// if we just got out of a loop, *cell will already
				// be zero
				// print_indent(f, level);
				// fprintf(f, "cell[%d] = 0;\n", currentPointer -
				// startPointer);
				if(lastOpcode != JMPNZ)
					fprintf(f, "*cell = 0;\n");
				break;
			case END: return currentPointer;
			case START: return currentPointer;
		}
		lastOpcode = bakOpcode;
	}
}

void transpile(const char *filename, IntArray *program) {
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
	fprintf(f, "#include <stdint.h>\n");
	fprintf(f, "#include <time.h>\n\n");
	fprintf(f, "char memory[%d];\n\n", MAX_CELLS);
	fprintf(f, "int main() {\n");
	fprintf(f, "\tchar *cell = memory;\n");
	fprintf(f, "\tclock_t start = clock();\n");
	fprintf(f, "\tchar change = 0;\n");
	fprintf(f, "\t(void)change;\n");
	int *pgm = program->values;
	transpile_rec(f, program->values, &pgm, 0, 1, false);
	fprintf(f, "\tprintf(\"\\nElapsed: %%fs\\n\",(double)(clock() - "
	           "start)/CLOCKS_PER_SEC);\n");
	fprintf(f, "\treturn 0;\n");
	fprintf(f, "}");
	fclose(f);
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("Usage: %s <bf source code> [<input data>]\n", argv[0]);
		return 0;
	}
	printf("Running %s..\n", argv[1]);

	FILE *readstream = stdin;
	if(argc > 2) {
		readstream = fopen(argv[2], "rb");
		if(readstream == NULL) {
			printf("Unable to open input file!\n");
			return 2;
		}
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
	IntArray *compiled = compile(program);
	free(program);
	if(compiled == NULL) {
		printf("Error occurred while compilation!\n");
		return 2;
	}
	// disassemble_all(compiled);
	transpile(argv[1], compiled);
	clock_t start = clock();
	execute(compiled, readstream);
	if(readstream != stdin)
		fclose(readstream);
	printf("Elapsed: %fs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
	int_array_free(compiled);
	free(compiled);
}
