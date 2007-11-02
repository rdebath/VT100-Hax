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
 * 
 */
/* 8080 asmbler header */

void            OpenFiles(char *);
void		Pass1();
void		Pass2();
void            Asm();
void            CloseFiles();
void            EmitHelp();
void		RewindFiles();

int             Parse(char *);
void            PrintList(char *);
void            DumpBin();

/* Constants */

#define BINARY_TO_DUMP 1
#define PROCESSED_END 2
#define COMMENT 3
#define TEXT 4
#define LIST_ONLY 5
#define LIST_BYTES 6
#define LIST_WORDS 7

/* RPN stack for expression parser */

int             PopRPN();
void            PushRPN(int);
int             RPN(char *);
void            SwapRPN();
void            EvalRPN();

typedef struct RPNStack {
	int             word[4];
	int             level;
}               RPNSTACK;
enum Operators {
	ADD, SUB, MUL, DIV, AND, OR, LSHIFT, RSHIFT
};
/* Binary stack for lister */

typedef struct Stack {
	int             word;
	void           *next;
}               STACK;

/* Symbol table storage */

typedef struct Symbol {
	char            Symbol_Name[16];
	int             Symbol_Value;
	void           *next;
}               SYMBOL;

/* Action Prototypes */

int             EQU_proc(char *, char *);
int             ANOP_proc(char *, char *);
int             DAC_proc(char *, char *);
int             DB_proc(char *, char *);
int             DW_proc(char *, char *);
int             END_proc(char *, char *);
int             MOV_proc(char *, char *);
int             MVI_proc(char *, char *);
int             LXI_proc(char *, char *);
int             LDA_proc(char *, char *);
int             STA_proc(char *, char *);
int             LHLD_proc(char *, char *);
int             SHLD_proc(char *, char *);
int             LDAX_proc(char *, char *);
int             STAX_proc(char *, char *);
int             XCHG_proc(char *, char *);
int             ADD_proc(char *, char *);
int             ADI_proc(char *, char *);
int             ADC_proc(char *, char *);
int             ACI_proc(char *, char *);
int             SUB_proc(char *, char *);
int             SUI_proc(char *, char *);
int             SBB_proc(char *, char *);
int             SBI_proc(char *, char *);
int             INR_proc(char *, char *);
int             DCR_proc(char *, char *);
int             INX_proc(char *, char *);
int             DCX_proc(char *, char *);
int             DAD_proc(char *, char *);
int             DAA_proc(char *, char *);
int             ANA_proc(char *, char *);
int             ANI_proc(char *, char *);
int             ORA_proc(char *, char *);
int             ORI_proc(char *, char *);
int             XRA_proc(char *, char *);
int             XRI_proc(char *, char *);
int             CMP_proc(char *, char *);
int             CPI_proc(char *, char *);
int             RLC_proc(char *, char *);
int             RRC_proc(char *, char *);
int             RAL_proc(char *, char *);
int             RAR_proc(char *, char *);
int             CMA_proc(char *, char *);
int             CMC_proc(char *, char *);
int             STC_proc(char *, char *);
int             JMP_proc(char *, char *);
int             JNZ_proc(char *, char *);
int             JZ_proc(char *, char *);
int             JNC_proc(char *, char *);
int             JC_proc(char *, char *);
int             JPO_proc(char *, char *);
int             JPE_proc(char *, char *);
int             JP_proc(char *, char *);
int             JM_proc(char *, char *);
int             CALL_proc(char *, char *);
int             CNZ_proc(char *, char *);
int             CZ_proc(char *, char *);
int             CNC_proc(char *, char *);
int             CC_proc(char *, char *);
int             CPO_proc(char *, char *);
int             CPE_proc(char *, char *);
int             CP_proc(char *, char *);
int             CM_proc(char *, char *);
int             RET_proc(char *, char *);
int             RNZ_proc(char *, char *);
int             RZ_proc(char *, char *);
int             RNC_proc(char *, char *);
int             RC_proc(char *, char *);
int             RPO_proc(char *, char *);
int             RPE_proc(char *, char *);
int             RP_proc(char *, char *);
int             RM_proc(char *, char *);
int             RST_proc(char *, char *);
int             PCHL_proc(char *, char *);
int             PUSH_proc(char *, char *);
int             POP_proc(char *, char *);
int             XTHL_proc(char *, char *);
int             SPHL_proc(char *, char *);
int             IN_proc(char *, char *);
int             OUT_proc(char *, char *);
int             EI_proc(char *, char *);
int             DI_proc(char *, char *);
int             HLT_proc(char *, char *);
int             NOP_proc(char *, char *);

typedef struct Instructions {
	const char     *Name;
	int             (*fnc) (char *, char *);
}               INSTRUCTIONS;

INSTRUCTIONS    OpCodes[] =
{
	{"MOV", MOV_proc},
	{"MVI", MVI_proc},
	{"LXI", LXI_proc},
	{"LDA", LDA_proc},
	{"STA", STA_proc},
	{"LHLD", LHLD_proc},
	{"SHLD", SHLD_proc},
	{"LDAX", LDAX_proc},
	{"STAX", STAX_proc},
	{"XCHG", XCHG_proc}, {"ADD", ADD_proc}, {"ADI", ADI_proc},
	{"ADC", ADC_proc}, {"ACI", ACI_proc}, {"SUB", SUB_proc},
	{"SUI", SUI_proc}, {"SBB", SBB_proc}, {"SBI", SBI_proc},
	{"INR", INR_proc}, {"DCR", DCR_proc}, {"INX", INX_proc},
	{"DCX", DCX_proc}, {"DAD", DAD_proc}, {"DAA", DAA_proc},
	{"ANA", ANA_proc}, {"ANI", ANI_proc}, {"ORA", ORA_proc},
	{"ORI", ORI_proc}, {"XRA", XRA_proc}, {"XRI", XRI_proc},
	{"CMP", CMP_proc}, {"CPI", CPI_proc}, {"RLC", RLC_proc},
	{"RRC", RRC_proc}, {"RAL", RAL_proc}, {"RAR", RAR_proc},
	{"CMA", CMA_proc}, {"CMC", CMC_proc}, {"STC", STC_proc},
	{"JMP", JMP_proc}, {"JNZ", JNZ_proc}, {"JZ", JZ_proc},
	{"JNC", JNC_proc}, {"JC", JC_proc}, {"JPO", JPO_proc},
	{"JPE", JPE_proc}, {"JP", JP_proc}, {"JM", JM_proc},
	{"CALL", CALL_proc}, {"CNZ", CNZ_proc}, {"CZ", CZ_proc},
	{"CNC", CNC_proc}, {"CC", CC_proc}, {"CPO", CPO_proc},
	{"CPE", CPE_proc}, {"CP", CP_proc}, {"CM", CM_proc},
	{"RET", RET_proc}, {"RNZ", RNZ_proc}, {"RZ", RZ_proc},
	{"RNC", RNC_proc}, {"RC", RC_proc}, {"RPO", RPO_proc},
	{"RPE", RPE_proc}, {"RP", RP_proc}, {"RM", RM_proc},
	{"RST", RST_proc}, {"PCHL", PCHL_proc}, {"PUSH", PUSH_proc},
	{"POP", POP_proc}, {"XTHL", XTHL_proc}, {"SPHL", SPHL_proc},
	{"IN", IN_proc}, {"OUT", OUT_proc}, {"EI", EI_proc},
	{"DI", DI_proc}, {"HLT", HLT_proc}, {"NOP", NOP_proc},
	{"EQU", EQU_proc}, {"DAC", DAC_proc}, {"DB", DB_proc},
	{"DW", DW_proc}, {"END", END_proc},{"ANOP",ANOP_proc}, 
	{0, NULL}
};
