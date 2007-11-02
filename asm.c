/*
 * Copyright (c) <2007> <Jay.Cotton@Sun.COM>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * 8080 assembeler
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

/* ASM Defs */

#include "asm.h"

int             DW(char *);
int             DB(char *);
int             ProcDollar(char *);

/*
 * global defs
 */

FILE           *in;
FILE           *list;
FILE           *bin;

/* Global storage for assembler */

int             pass;		/* assembler pass */
int             addr;
int             b1;
int             b2;
int             b3;
int             b4;
int             type;
char            Equation[80];
int             opcode_bytes;

SYMBOL         *Symbols;
STACK          *ByteWordStack;

/*
 * Notes:
 * 
 * This is a single pass assembler, so, you have to define a symbol befor you
 * use it, else its not found.
 * 
 * The RPN parser is shaky at best.
 * 
 * There may be some opcodes that are in error.
 * 
 * The register pair processing needs work.
 * 
 * Overall the assembler needs a clean up.  There are a lot of places where to
 * code can be optimized.
 * 
 * Just noticed that the bin dumper is not implemented....TBD.
 * 
 * The assembler assumes that the left column is a lable the mid column is an
 * opcode and the right column is an equation.
 * 
 */
int
main(int argv, char *argc[])
{
	/* init the globals */

	addr = b1 = b2 = b3 = b4 = 0;

	Symbols = (SYMBOL *) calloc(1, sizeof(SYMBOL));
	ByteWordStack = (STACK *) calloc(1, sizeof(STACK));

	if (argv > 1) {
		OpenFiles(argc[1]);
		Pass1();
		Pass2();
		CloseFiles();
		exit(0);
	}
	EmitHelp();
	return 0;
}
void
Pass1()
{
	pass = 0;
	Asm();

}
void
Pass2()
{
	RewindFiles();
	addr = 0; 
	pass = 1;
	Asm();
}
void
OpenFiles(char *name)
{
	char            in_file[80];
	char            list_file[80];
	char            bin_file[80];

	printf("Open input file %s\n", name);

	strcpy(in_file, name);
	strcpy(list_file, name);
	strcpy(bin_file, name);
	strcat(in_file, ".asm");
	strcat(list_file, ".lst");
	strcat(bin_file, ".bin");

	/* open the files */

	in = fopen(in_file, "r");
	if (in == NULL) {
		perror("Open input failed");
		exit(1);
	}
	/* may not be needed, but */
	remove(list_file);
	remove(bin_file);

	list = fopen(list_file, "w");
	bin = fopen(bin_file, "wb");

}
void
RewindFiles()
{
	fseek(in, 0, SEEK_SET);
}
void
CloseFiles()
{
	fclose(in);
	fclose(list);
	fclose(bin);
}
void
Asm()
{
	char            text[128];
	int             EmitBin;

	/* until EOF */
	while (1) {
		EmitBin = 0;
		if (!fgets(text, 128, in)) {
			return;
		}
		if(strlen(text) == 1) goto Skip;
		/* trim text */
		text[strlen(text) - 1] = '\0';

		EmitBin = Parse(text);
Skip:
		PrintList(text);
		if (EmitBin == BINARY_TO_DUMP)
			if (pass)
				DumpBin();
			else if (EmitBin == PROCESSED_END)
				return;	/* processed END */
	}
}

/* this should print a help command list... */

void
EmitHelp()
{
}

/*
 * Label processing.
 * 
 * FindLabel: AddLabel:
 * 
 * Linked list, linear search, string compare real dumb.
 */

SYMBOL         *
FindLabel(char *text)
{
	SYMBOL         *Local = Symbols;
	char            tmp[80];
	int             i = 0;

	while (isalnum(*text))
		tmp[i++] = *text++;
	tmp[i] = '\0';

	while (Local->next) {
		if (!strcmp(Local->Symbol_Name, tmp))
			return Local;
		Local = (SYMBOL *) Local->next;
	}
	return NULL;
}
void
AddLabel(char *text)
{
	char            label[16];
	int             i = 0;
	SYMBOL         *Local = Symbols;

	if(pass) return;

	while (isalnum(*text))
		label[i++] = *text++;
	label[i] = '\0';

	if (FindLabel(label) ) {
		fprintf(list, "\nDuplicate Label: %s\n", label);
		return;
	}
	/* now add it to the list */


	while (Local->next) {	/* find end of list */
		Local = (SYMBOL *) Local->next;
	}
	strcpy(Local->Symbol_Name, label);
	Local->Symbol_Value = addr;
	Local->next = (SYMBOL *) calloc(1, sizeof(SYMBOL));
}

/* break down the text string */

int
Parse(char *text)
{
	char            opcode[6];
	int             i = 0;
	INSTRUCTIONS   *Local = OpCodes;
	char           *Equa = Equation;
	char            Label[32];
	int             status;

	if (text[0] == ';') {

		/* process comment statment in source stream */

		type = COMMENT;
		return LIST_ONLY;
	} else {		/* record any symbol and the cop */
		/*
		 * Because we filter comments above, we know that a non blank
		 * will be a symbol
		 */

		memset(opcode, 0, 6);
		memset(Label, 0, 32);
		if (isalnum(text[0])) {
			AddLabel(text);
			while (isalnum(*text))
				Label[i++] = *text++;
			Label[i] = '\0';
		}
		/* then find an op symbol */

		while (isspace(*text))
			text++;
		i = 0;
		while (isalnum(*text))
			opcode[i++] = *text++;
		opcode[i] = '\0';

		/* copy next section to equation buffer */

		memset(Equation, 0, 80);
		while (isspace(*text))
			text++;
		while (!iscntrl(*text))
			*Equa++ = *text++;

		/* lookup opcode and call proc */

		while (Local->Name) {
			if (!strcmp(Local->Name, opcode)) {
				status = Local->fnc(Label, Equation);
				if (status == PROCESSED_END)
					return status;
				break;
			} else {
				Local++;
			}
		}
		if (Local->Name) {
			type = TEXT;
		} else {
			type = COMMENT;
			fprintf(list, "\nCan't find OpCode %s\n", opcode);
		}
	}
	type = status;
	return status;
}

/*
 * output list format
 * 
 * [addr] [bn] [bn] [bn] [b4] lab^topcode^taddr^tcomment
 */

void
PrintList(char *text)
{
	SYMBOL         *Local = Symbols;
	STACK          *DLStack, *LStack = ByteWordStack;
	int             space = 0;
	switch (type) {
	case COMMENT:
		if (pass)
			fprintf(list, "                 %s\n", text);
		break;
	case TEXT:
		switch (opcode_bytes) {
		case 1:
			if (pass)
				fprintf(list, "%04x %02x          %s\n", addr, b1, text);
			addr += 1;
			break;
		case 2:
			if (pass)
				fprintf(list, "%04x %02x %02x       %s\n", addr, b1, b2, text);
			addr += 2;
			break;
		case 3:
			if (pass)
				fprintf(list, "%04x %02x %02x %02x    %s\n", addr, b1, b2, b3, text);
			addr += 3;
			break;
		case 4:
			if (pass)
				fprintf(list, "%04x %02x %02x %02x %02x %s\n", addr, b1, b2, b3, b4, text);
			addr += 4;
			break;
		default:	/* 0 */
			if (pass)
				fprintf(list, "     %02x%02x        %s\n", b2, b1, text);
			break;
		}
		break;
	case LIST_ONLY:
		break;
	case LIST_BYTES:
	case LIST_WORDS:
		if (pass)
			fprintf(list, "     %02x%02x        %s\n", b2, b1, text);
		if (pass)
			fprintf(list, "     ");
		while (LStack->next) {	/* don't count last byte/word */
			if (type == LIST_BYTES) {
				if (pass)
					fprintf(list, "%02x ", LStack->word);
				space += 3;
				addr++;
			} else {
				if (pass)
					fprintf(list, "%04x ", LStack->word);
				space += 5;
				addr += 2;
			}
			LStack = (STACK *) LStack->next;
			if (space > 16) {
				if (pass)
					fprintf(list, "\n     ");
				space = 0;
			}
		}
		if (pass)
			fprintf(list, "\n");

		/* always keep the stack root */
		LStack = ByteWordStack;
		DLStack = LStack = (STACK *) LStack->next;
		if (LStack) {
			do {
				LStack = (STACK *) LStack->next;
				free(DLStack);
				DLStack = LStack;
			}
			while (LStack);
			LStack = ByteWordStack;
			LStack->next = NULL;
		}
		break;
	case PROCESSED_END:
		if (pass)
			fprintf(list, "                 %s\n", text);
		if (pass)
			do {
				fprintf(list, "%s\t%04x\n", Local->Symbol_Name, Local->Symbol_Value);
				Local = (SYMBOL *) Local->next;
			}
			while (Local);
		break;
	}
	opcode_bytes = b1 = b2 = b3 = b4 = 0;

	/* remove after debug */
	if (pass)
		fflush(list);
}
void
DumpBin()
{
}

/*
 * Expressions can be as simple as [1], or complex like "xyzzy"+23/7(45*3)+16
 * << test3
 * 
 * Parseing forms that this parer will handle:
 * 
 * (expression) -- (stack) = (expression) [op] (expression)
 * 
 * [op] can be: +add -subtract * multiply	/ divide <<shift	left >>shift
 * right |or		&and $current  address counter.
 * 
 * 
 * expression can have strings, numbers or lables.
 * 
 * strings can be single quote, or double quote i.e.  'AB' or "AB" numbers can
 * be base 10, base 16, or base 8 i.e.  124, 0x3e or O12 (capital O)
 * 
 * at the end of the process, return (stack)
 * 
 * a typical expression that is seen in assembly is, LXI	HL,("EV" + *
 * STARTOFTABLE)/4 So, this breaks down to (stack) = (/ operator) (stack) = 4
 * (stack) = (+ operator) (stack) = STARTOFTABLE 	(the value of )
 * (stack push) = "EV"		(as a value)
 * 
 * For the curious, this is an RPN stack parser.
 * 
 */

RPNSTACK        stack;


int
ExpressionParser(char *text)
{
	stack.level = 0;
	RPN(text);
	return stack.word[0];
}
void
EvalRPN()
{
	int             a, b;
	int             op;

	if (stack.level < 3)
		return;

	b = PopRPN();
	op = PopRPN();
	a = PopRPN();
	switch (op) {
	case ADD:
		PushRPN(a + b);
		break;
	case SUB:
		PushRPN(a - b);
		break;
	case MUL:
		PushRPN(a * b);
		break;
	case DIV:
		PushRPN(a / b);
		break;
	case AND:
		PushRPN(a & b);
		break;
	case OR:
		PushRPN(a | b);
		break;
	case LSHIFT:
		PushRPN(a << b);
		break;
	case RSHIFT:
		PushRPN(a >> b);
		break;
	}
}
void
PushRPN(int value)
{
	stack.word[3] = stack.word[2];
	stack.word[2] = stack.word[1];
	stack.word[1] = stack.word[0];
	stack.word[0] = value;
	stack.level++;
}
int
PopRPN()
{
	int             value = stack.word[0];
	stack.word[0] = stack.word[1];
	stack.word[1] = stack.word[2];
	stack.word[2] = stack.word[3];
	stack.level--;
	return value;
}
void
SwapRPN()
{
	int             value = stack.word[0];
	stack.word[0] = stack.word[1];
	stack.word[1] = value;
}

/* Warning:  recursive parseing */

int
RPN(char *text)
{
	while (1) {
		switch (*text) {
			case '(':
			{
				text++;
				RPN(text);
				break;
			}
		case ')':
			{	/* evaluate the stack */
				text++;
				EvalRPN();
				break;
			}
		default:
			{
				switch (*text) {
				case '&':
					PushRPN(AND);
					text++;
					break;
				case '|':
					PushRPN(OR);
					text++;
					break;
				case '+':
					PushRPN(ADD);
					text++;
					break;
				case '-':
					PushRPN(SUB);
					text++;
					break;
				case '<':
					PushRPN(LSHIFT);
					text++;
					text++;
					break;
				case '>':
					PushRPN(RSHIFT);
					text++;
					text++;
					break;
				case '*':
					PushRPN(MUL);
					text++;
					break;
				case '/':
					PushRPN(DIV);
					text++;
					break;
				default:	/* must be a label */
					{
						SYMBOL         *Local;
						char            lab[16];
						int             i = 0;
						memset(lab, 0, 16);
						while (isalnum(*text))
							lab[i++] = *text++;
						/* could be a number */
						if (isdigit(lab[0])) {
							PushRPN(DB(lab));
						} else {
							Local = FindLabel(lab);
							if (!Local)
								return 0;
							PushRPN(Local->Symbol_Value);
						}
					}
					break;
				}
				break;
		case '$':
				PushRPN(addr);
				text++;
				break;
		case ' ':
		case '\t':
				text++;
				break;
		case '\0':
		case '\n':
				EvalRPN();
				return stack.word[0];
			}
		}
	}
}




char           *
RegPare(char *text)
{
	while (isspace(*text))
		text++;
	if (!strncmp("BC", text, 2)) {
		text++;
		text++;
		return text;
	}else if(*text == 'B')
	{
		text++;
		return text;
	}
	if (!strncmp("DE", text, 2)) {
		b1 += 0x10;
		text++;
		text++;
		return text;
	}else if (*text == 'D')
	{
		b1 += 0x10;
		text++;
		return text;
	}
	if (!strncmp("HL", text, 2)) {
		b1 += 0x20;
		text++;
		text++;
		return text;
	}else if (*text == 'H')
	{
		b1 += 0x20;
		text++;
		return text;
	}
	if (!strncmp("SP", text, 2)) {
		b1 += 0x30;
		text++;
		text++;
		return text;
	}
	if (!strncmp("PSW", text, 3)) {
		b1 += 0x30;
		text++;
		text++;
		text++;
		return text;
	}
}
char           *
DestReg(char *text)
{
	while (isspace(*text))
		text++;
	switch (*text) {
	case 'A':
		b1 += 0x7 << 3;
		break;
	case 'B':
		b1 += 0x0 << 3;
		break;
	case 'C':
		b1 += 0x1 << 3;
		break;
	case 'D':
		b1 += 0x2 << 3;
		break;
	case 'E':
		b1 += 0x3 << 3;
		break;
	case 'H':
		b1 += 0x4 << 3;
		break;
	case 'L':
		b1 += 0x5 << 3;
		break;
	case 'M':
		b1 += 0x6 << 3;
		break;
	default:
		if (pass)
			fprintf(list, "Bad destination register: %s\n", text);
		break;
	}
	text++;
	return (text);
}
char           *
SourceReg(char *text)
{
	while (isspace(*text))
		text++;

	switch (*text) {
	case 'A':
		b1 += 0x7;
		break;
	case 'B':
		b1 += 0x0;
		break;
	case 'C':
		b1 += 0x1;
		break;
	case 'D':
		b1 += 0x2;
		break;
	case 'E':
		b1 += 0x3;
		break;
	case 'H':
		b1 += 0x4;
		break;
	case 'L':
		b1 += 0x5;
		break;
	case 'M':
		b1 += 0x6;
		break;
	default:
		if (pass)
			fprintf(list, "Bad source register: %s\n", text);
		break;
	}
	text++;
	return (text);
}

int
ANOP_proc(char *label, char *equation)
{
	return EQU_proc(label, equation);
}
int
EQU_proc(char *label, char *equation)
{
	SYMBOL         *Local, *Local2;
	/* handle the EQU command. */

	Local = FindLabel(label);	/* things are very broken if not
					 * found */
	if (!Local)
		return LIST_ONLY;

	/* Evaluate the equation section */

	if (*equation == '$')
		Local->Symbol_Value = ProcDollar(equation);
	else
		/* all other possible things go here */
	{
		Local2 = FindLabel(equation);
		if (Local2)
			Local->Symbol_Value = Local2->Symbol_Value;
		else
			Local->Symbol_Value = DW(equation);
	}
	/* Copy off the value */
	b1 = Local->Symbol_Value & 0x00ff;
	b2 = (Local->Symbol_Value & 0xff00) >> 8;
	return TEXT;
}
int
DAC_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	STACK          *LStack = ByteWordStack;

	Local = FindLabel(label);
	addr = Local->Symbol_Value = ExpressionParser(equation);
	b1 = Local->Symbol_Value & 0x00ff;
	b2 = (Local->Symbol_Value & 0xff00) >> 8;
	return TEXT;
}

/*
 * for DB, and DW, there may be an unbounded list of hex, dec, and octal
 * bytes/words, and quoted strings
 */

char           *
AdvanceTo(char *text, char x)
{
	while (*text) {
		if (*text != x)
			text++;
		else
			return text;
	}
	return text--;
}
char           *
AdvancePast(char *text, char x)
{
	text = AdvanceTo(text, x);
	return (++text);
}
char           *
AdvanceToAscii(char *text)
{				/* i.e. not a space */
	while (isspace(*text))
		text++;
	return text;
}
char           *
AdvancePastSpace(char *text)
{
	return AdvanceToAscii(text);
}
int
ProcDollar(char *text)
{
	int             tmp = addr;

	SYMBOL         *Local;

	text = AdvancePast(text, '$');
	text = AdvancePastSpace(text);

	if (*text == '+') {
		text = AdvancePastSpace(++text);
		Local = FindLabel(text);
		if (Local) {
			tmp += Local->Symbol_Value;
		} else
			tmp += DW(text);
	} else if (*text == '-') {
		text = AdvancePastSpace(++text);
		Local = FindLabel(text);
		if (Local) {
			tmp += Local->Symbol_Value;
		} else
			tmp -= DW(text);
	}
	return tmp;
}
int
DW(char *text)
{
	int             tmp;
	if (*text == '\"') {
		text++;
		tmp = *text << 8;
		text++;
		tmp |= *text;
	} else
		tmp = ExpressionParser(text);
	return tmp;
}
int
DB(char *text)
{
	int             accum = 0;
	int             dec = 1;

	while (*text) {
		switch (*text) {
		case '\'':
			{
				text++;
				accum = *text;
				return accum;
			}
			break;
		case '\0':
		case '\n':
		case '\t':
		case '\"':
			return accum;
			break;
		case ' ':
			if (accum)
				return accum;
			break;
		case ',':
			return accum;
			break;
		}
		accum = accum * (dec ? 10 : 16);
		switch (toupper(*text)) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			accum += *text++ - '0';
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			accum += (toupper(*text++) - 'A') + 10;
			break;
		case 'X':
			dec = 0;
			text++;
			break;
		default:
			return accum;	/* punt */
		}
	}

	return accum;

}
char           *
Evaluate(char *text, STACK * LStack)
{				/* we get here because something looks like a
				 * number */
	int             accum = 0;
	int             dec = 1;/* assume decimal */

	while (*text) {
		switch (*text) {
		case '\0':
		case '\n':
		case ' ':
		case ',':
		case '\t':
		case '\"':
			LStack->word = accum;
			LStack->next = (STACK *) calloc(1, sizeof(STACK));
			LStack = (STACK *) LStack->next;
			return text;
			break;
		}
		accum = accum * (dec ? 10 : 16);
		if (isdigit(*text)) {
			accum += *text++ - '0';
			if (toupper(*text) == 'X') {
				dec = 0;
				text++;
			}
		} else
			text++;
	}

	LStack->word = accum;
	LStack->next = (STACK *) calloc(1, sizeof(STACK));
	LStack = (STACK *) LStack->next;
	return text;
}
int
DB_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	STACK          *LStack = ByteWordStack;
	int             value;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = addr & 0x00ff;
	b2 = (addr & 0xff00) >> 8;
	value = 0;

	/* the list could be strings, labels, or digits */

	while (*equation) {	/* go to the end of the string */
		switch (*equation) {
		case '\'':
		case '"':
			/* characters */
			equation++;
			while (*equation) {
				if (*equation == '\'')
					break;
				else if (*equation == '"')
					break;
				LStack->word = *equation++;
				LStack->next = (STACK *) calloc(1, sizeof(STACK));
				LStack = (STACK *) LStack->next;
			}
			equation++;
			break;
		case ',':
			equation++;
			break;
		case '$':
			value = addr;
			equation = AdvancePast(equation, ',');
			break;
		default:
			/* could be numbers, or a label */
			if (isdigit(*equation)) {
				value = DB(equation);
				equation = AdvancePast(equation, ',');
			} else if (*equation == '(') {
				value = DB(equation);
				equation = AdvancePast(equation, ',');
			} else {
				Local = FindLabel(equation);
				if (Local)
					value = Local->Symbol_Value;
				else
					fprintf(list, "label not found %s\n", equation);
				equation = AdvancePast(equation, ',');
			}
			/*
			 * accumulate a stack of 8 bit values to be printed
			 * by the lister
			 */
			LStack->word = value;
			LStack->next = (STACK *) calloc(1, sizeof(STACK));
			LStack = (STACK *) LStack->next;
			break;

		}
	}
	return LIST_BYTES;
}
int
DW_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	STACK          *LStack = ByteWordStack;
	int             value;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = addr & 0x00ff;
	b2 = (addr & 0xff00) >> 8;
	value = 0;

	/* the list could be strings, labels, or digits */

	while (*equation) {	/* go to the end of the string */
		switch (*equation) {
		case '\'':
		case '"':
			/* characters */
			equation++;
			while (*equation) {
				if (*equation == '\'')
					break;
				else if (*equation == '"')
					break;
				LStack->word = (*equation++) << 8;
				if (*equation == '\'') {
					LStack->next = (STACK *) calloc(1, sizeof(STACK));
					LStack = (STACK *) LStack->next;
					break;
				} else if (*equation == '"') {
					LStack->next = (STACK *) calloc(1, sizeof(STACK));
					LStack = (STACK *) LStack->next;
					break;
				}
				LStack->word += *equation++;
				LStack->next = (STACK *) calloc(1, sizeof(STACK));
				LStack = (STACK *) LStack->next;
			}
			equation++;
			break;
		case ',':
			equation++;
			break;
		case '$':
			value = ExpressionParser(equation);
			equation = AdvancePast(equation, ',');
			break;
		default:
			/* could be numbers, or a label */
			if (isdigit(*equation)) {
				value = DB(equation);
				equation = AdvancePast(equation, ',');
			} else {
				Local = FindLabel(equation);
				if (Local)
					value = Local->Symbol_Value;
				else
					fprintf(list, "label not found %s\n", equation);
				equation = AdvancePast(equation, ',');
			}
			/*
			 * accumulate a stack of 8 bit values to be printed
			 * by the lister
			 */
			LStack->word = value;
			LStack->next = (STACK *) calloc(1, sizeof(STACK));
			LStack = (STACK *) LStack->next;
			break;

		}
	}
	return LIST_WORDS;
}
int
END_proc(char *label, char *equation)
{
	type = PROCESSED_END;
	return PROCESSED_END;
}
int
MOV_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x040;
	/* proc dest reg */
	equation = DestReg(equation);
	/* proc source reg */
	equation = AdvancePastSpace(AdvancePast(equation, ','));
	equation = SourceReg(equation);

	opcode_bytes = 1;
	return TEXT;
}
int
MVI_proc(char *label, char *equation)
{
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x06;
	/* proc dest reg */
	equation = DestReg(equation);
	/* proc value */
	equation = AdvancePast(equation, ',');
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		b2 = Local2->Symbol_Value;
	else
		b2 = DB(equation);
	opcode_bytes = 2;
	return TEXT;
}
int
LXI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x01;
	/* proc dest pare */
	equation = RegPare(equation);
	/* now the value */
	equation = AdvanceToAscii(AdvancePast(equation, ','));
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
LDA_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x3a;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
STA_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x32;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
LHLD_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x2A;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
SHLD_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x22;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
LDAX_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x0a;
	/* proc dest pare */
	RegPare(equation);
	/* now the value */
	opcode_bytes = 1;
	return TEXT;
}
int
STAX_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x02;
	/* proc dest pare */
	RegPare(equation);
	/* now the value */
	opcode_bytes = 1;
	return TEXT;
}
int
XCHG_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xeb;
	opcode_bytes = 1;
	return TEXT;
}
int
ADD_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x80;
	SourceReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
ADI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xe6;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
ADC_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x88;
	SourceReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
ACI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xce;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
SUB_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x90;
	SourceReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
SUI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xd6;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
SBB_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x98;
	SourceReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
SBI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xde;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
INR_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x4;
	DestReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
DCR_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x5;
	DestReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
INX_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x3;
	/* proc dest pare */
	RegPare(equation);
	/* now the value */
	opcode_bytes = 1;
	return TEXT;
}
int
DCX_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xb;
	/* proc dest pare */
	RegPare(equation);
	/* now the value */
	opcode_bytes = 1;
	return TEXT;
}
int
DAD_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x9;
	/* proc dest pare */
	RegPare(equation);
	/* now the value */
	opcode_bytes = 1;
	return TEXT;
}
int
DAA_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0x27;
	opcode_bytes = 1;
	return TEXT;
}
int
ANA_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xa0;
	SourceReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
ANI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xe6;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
ORA_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xb0;
	SourceReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
ORI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xf6;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
XRA_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xa8;
	SourceReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
XRI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xee;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
CMP_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xB8;
	SourceReg(equation);
	opcode_bytes = 1;
	return TEXT;
}
int
CPI_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xfe;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
RLC_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 7;
	return TEXT;
}
int
RRC_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xf;
	return TEXT;
}
int
RAL_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0x17;
	return TEXT;
}
int
RAR_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0x1f;
	return TEXT;
}
int
CMA_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0x2f;
	return TEXT;
}
int
CMC_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0x3f;
	return TEXT;
}
int
STC_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0x37;
	return TEXT;
}
int
JMP_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC3;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
JNZ_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC2;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
JZ_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC2 + (1 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
JNC_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC2 + (2 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
JC_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC2 + (3 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
JPO_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC2 + (4 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
JPE_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC2 + (5 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
JP_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC2 + (6 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
JM_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC2 + (7 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CALL_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC3;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CNZ_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC4;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CZ_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC4 + (1 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CNC_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC4 + (2 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CC_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC4 + (3 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CPO_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC4 + (4 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CPE_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC4 + (5 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CP_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC4 + (6 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
CM_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xC4 + (7 << 3);
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	b3 = (tmp >> 8) & 0xff;
	opcode_bytes = 3;
	return TEXT;
}
int
RET_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC9;
	return TEXT;
}
int
RNZ_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC0;
	return TEXT;
}
int
RZ_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC0 + (1 << 3);
	return TEXT;
}
int
RNC_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC0 + (2 << 3);
	return TEXT;
}
int
RC_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC0 + (3 << 3);
	return TEXT;
}
int
RPO_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC0 + (4 << 3);
	return TEXT;
}
int
RPE_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC0 + (5 << 3);
	return TEXT;
}
int
RP_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC0 + (6 << 3);
	return TEXT;
}
int
RM_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xC0 + (7 << 3);
	return TEXT;
}
int
RST_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xd3;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0x7;
	opcode_bytes = 1;
	b1 = 0xC7 + (b2 << 3);
	return TEXT;
}
int
PCHL_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xe9;
	return TEXT;
}
int
PUSH_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xc5;
	RegPare(equation);
	return TEXT;
}
int
POP_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xc1;
	RegPare(equation);
	return TEXT;
}
int
XTHL_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xe3;
	return TEXT;
}
int
SPHL_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xf9;
	return TEXT;
}
int
IN_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xdb;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
OUT_proc(char *label, char *equation)
{
	int             tmp;
	SYMBOL         *Local, *Local2;

	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;
	b1 = 0xd3;
	/* now the value */
	equation = AdvanceToAscii(equation);
	Local2 = FindLabel(equation);
	if (Local2)
		tmp = Local2->Symbol_Value;
	else
		tmp = DW(equation);
	b2 = tmp & 0xff;
	opcode_bytes = 2;
	return TEXT;
}
int
EI_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xfb;
	return TEXT;
}
int
DI_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0xf3;
	return TEXT;
}
int
HLT_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0x76;
	return TEXT;
}
int
NOP_proc(char *label, char *equation)
{
	SYMBOL         *Local;
	Local = FindLabel(label);
	/* record the address of the label */
	if (Local)
		Local->Symbol_Value = addr;

	opcode_bytes = 1;
	b1 = 0;
	return TEXT;
}
