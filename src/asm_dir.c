/*	*************************************************************************
 *	Module Name:	asm_dir.c
 *	Description:	Assembler Directive.
 *	Copyright(c):	See below...
 *	Author(s):		Claude Sylvain
 *	Created:			24 December 2010
 *	Last modified:	27 March 2011
 * Notes:
 *	************************************************************************* */

/*
 * Copyright (c) <2007-2011> <jay.cotton@oracle.com>
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


/*	*************************************************************************
 *	                              INCLUDE FILES
 *	************************************************************************* */

#include <stdio.h>
#include <errno.h>

#if	defined (_TGT_OS_CYGWIN32) || defined (_TGT_OS_CYGWIN64) ||			\
		defined (_TGT_OS_LINUX32) || defined (_TGT_OS_LINUX64)
#include <unistd.h>
#endif

#include <ctype.h>
#include <stdlib.h>

#if	defined (_TGT_OS_CYGWIN32) || defined (_TGT_OS_CYGWIN64) ||			\
		defined (_TGT_OS_LINUX32) || defined (_TGT_OS_LINUX64) ||			\
		defined (_TGT_OS_WIN32) || defined (_TGT_OS_WIN64)
#include <string.h>
#elif defined (_TGT_OS_SOLARIS32) || defined (_TGT_OS_SOLARIS64)
#include <strings.h>
#else
#error !!! Unsupported operating system!
#endif

#include "project.h"
#include "err_code.h"		/*	Error Codes. */
#include "war_code.h"		/*	Warning Codes. */
#include "util.h"
#include "exp_parser.h"
#include "main.h"
#include "asm_dir.h"


/*	*************************************************************************
 *	                          FUNCTIONS DECLARATION
 *	************************************************************************* */

/*	Private functions.
 *	****************** */

static int proc_if(char *label, char *equation);
static int proc_else(char *label, char *equation);
static int proc_endif(char *label, char *equation);
static int proc_db(char *label, char *equation);
static int proc_dw(char *label, char *equation);
static int proc_ds(char *label, char *equation);
static int proc_include(char *label, char *equation);
static int proc_local(char *label, char *equation);
//static int proc_endlocal(char *label, char *equation);
//static int proc_anop(char *label, char *equation);
static int proc_equ(char *, char *);
//static int DAC_proc(char *, char *);
static int ORG_proc(char *, char *);
static int END_proc(char *, char *);


/*	*************************************************************************
 *												 CONST
 *	************************************************************************* */

/*	Public "const".
 *	*************** */

/*	Assembler Directives.
 *	--------------------- */ 	
const keyword_t	asm_dir[] =
{
//	{"EQU", proc_equ},			{"DAC", DAC_proc},	{"DB", proc_db},
	{"EQU", proc_equ},			{"DB", proc_db},
//	{"DW", proc_dw},				{"END", END_proc},	{"ANOP", proc_anop}, 
	{"DW", proc_dw},				{"END", END_proc},
  	{"INCLUDE", proc_include},	
	{"ORG", ORG_proc},			{"DS", proc_ds},		{"IF", proc_if},
	{"ELSE", proc_else},			{"ENDIF", proc_endif},
	{"SET", proc_equ},			{"LOCAL", proc_local},
//	{"ENDLOCAL", proc_endlocal},
	{0, NULL}
};


/*	*************************************************************************
 *											  VARIABLES
 *	************************************************************************* */

/*	Private variables.
 *	****************** */

//static unsigned char	end_processed	= 0;
//static unsigned char	end_processed;


/*	*************************************************************************
 *	                           FUNCTIONS DEFINITION
 *	************************************************************************* */


/*	*************************************************************************
 *	Function name:	asm_dir_start
 *	Description:	Prepare to Start a new assembler pass.
 *	Author(s):		Claude Sylvain
 *	Created:			31 December 2010
 *	Last modified:
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

void asm_dir_start(void)
{
//	end_processed	= 0;
}


#if 0
/*	*************************************************************************
 *	Function name:	asm_dir_is_end_processed
 *	Description:	Tell if "END" directive was processed.
 *	Author(s):		Claude Sylvain
 *	Created:			31 December 2010
 *	Last modified:
 *	Parameters:		void
 *
 *	Returns:			int:
 *							0	: "EQU" directive not processed.
 *							1	: "EQU" directive was processed.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

int asm_dir_is_end_processed(void)
{
	return ((int) end_processed);
}
#endif


/*	*************************************************************************
 *	Function name:	proc_if
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_if(char *label, char *equation)
{
#if FALSE
	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (if_true[if_nest] == 0)	return (LIST_ONLY);
#endif

	if (++if_nest < (sizeof (if_true) / sizeof (int)))
	{
		if_true[if_nest] = exp_parser(equation);
	}
	else
	{
		if_nest--;

		/*	Print/Display error, on assembler second pass only.
		 *	--------------------------------------------------- */	
		if (asm_pass == 1)
		{
			if (list != NULL)
				fprintf(list, "*** Error %d in \"%s\": 'if' nesting overflow!\n", EC_INO, in_fn[file_level]);
			
			fprintf(stderr, "*** Error %d in \"%s\" @%d: 'if' nesting overflow!\n", EC_INO, in_fn[file_level], codeline[file_level]);
		}
	}

	return (LIST_ONLY);
}


/*	*************************************************************************
 *	Function name:	proc_else
 *	Description:	"ELSE" assembler directive processing.
 *	Author(s):		Claude Sylvain
 *	Created:			24 December	2010
 *	Last modified:
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_else(char *label, char *equation)
{
	/*	Just toggle current "if_true[]" state.
	 *	So simple :-)
	 * */	 
	if_true[if_nest] = (if_true[if_nest] != 0) ? 0 : 1;

	return (LIST_ONLY);
}


/*	*************************************************************************
 *	Function name:	proc_endif
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	19 December 2010
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_endif(char *label, char *equation)
{
	if_true[if_nest] = 0;

	if (--if_nest < 0)
	{
		if_nest++;
		if_true[0] = 1;			/*	Restore 'if' nesting base level. */

		/*	Print/Display error, on assembler second pass only.
		 *	--------------------------------------------------- */	
		if (asm_pass == 1)
		{
			if (list != NULL)
				fprintf(list, "*** Error %d in \"%s\": 'if' nesting underflow!\n", EC_INU, in_fn[file_level]);

			fprintf(stderr, "*** Error %d in \"%s\" @%d: 'if' nesting underflow!\n", EC_INU, in_fn[file_level], codeline[file_level]);
		}
	}

	return (LIST_ONLY);
}


/*	*************************************************************************
 *	Function name:	proc_db
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_db(char *label, char *equation)
{
	SYMBOL	*Local;
	STACK		*LStack	= ByteWordStack;
	int		value;


	/*	Don't do anything, if code section is desactivated.
	 *	*/
//	if (if_true[if_nest] == 0)	return (LIST_ONLY);
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	Local = FindLabel(label);

	/* Record the address of the label.
	 * -------------------------------- */
	if (Local)
		Local->Symbol_Value = addr;

	b1		= addr & 0x00ff;
	b2		= (addr & 0xff00) >> 8;
	value	= 0;

	/* The list could be strings, labels, or digits */

	/* Go to the end of the string.
	 *	---------------------------- */
	while (*equation)
	{
		switch (*equation)
		{
			case '\'':
			case '"':
			{	
				unsigned char	in_quote		= 1;		/*	Not in Quote. */

				equation++;			/* Select next characters. */

				while (*equation)
				{
					/*	Check for empty string.
					 *	----------------------- */	
					if ((*equation == '\'') || (*equation == '"'))
					{
						in_quote	= 0;
						break;
					}

					LStack->word	= *(equation++);
					LStack->next	= (STACK *) calloc(1, sizeof(STACK));
					LStack			= (STACK *) LStack->next;
				}

				/*	- If still in quote, and we can display/print error,
				 *	  do it.
			 	 *	---------------------------------------------------- */	 
				if (in_quote)
				{
					if (asm_pass == 1)
					{
						if (list != NULL)
							fprintf(	list,
								  		"*** Warning %d in \"%s\": Missing quote!\n", WC_MQ, in_fn[file_level]);

						fprintf(	stderr,
							  		"*** Warning %d in \"%s\" @%d: Missing quote!\n",
								  	WC_MQ, in_fn[file_level], codeline[file_level]);
					}

					/*	- Notes: We must not select next character at this
					 *	  point, because "equation" have been evalued entirely,
					 *	  and we want "equation" to point to the string
					 *	  delimitor to exit from the main loop.
					 * */	
				}
				else
					equation++;		/*	Select next character. */

				break;
			}	

			/*	Bypass some characters.
			 *	----------------------- */	
			case ',':
			case ' ':
			case '\t':
				equation++;
				break;

#if 0
			case '$':
				value		= exp_parser(equation);
				equation	= AdvanceTo(equation, ',');
				break;
#endif

			default:
#if 0
				/* Could be numbers, or a label.
				 * ----------------------------- */
				if (isdigit((int) *equation))
				{
					value		= exp_parser(equation);
					equation	= AdvanceTo(equation, ',');
				}
				else if (*equation == '+')
				{
					value		+= exp_parser(equation);
					equation	= AdvanceTo(equation, ',');
				}
				else if (*equation == '-')
				{
					value		-= exp_parser(equation);
					equation	= AdvanceTo(equation, ',');
				}
				else if (*equation == '(')
				{
					value		= exp_parser(equation);
					equation	= AdvanceTo(equation, ',');
				}
				else
				{
					Local = FindLabel(equation);

					if (Local)
						value = Local->Symbol_Value;
					else if (asm_pass == 1)
					{
						fprintf(list, "*** Error %d in \"%s\": Label not found (%s)!\n", EC_LNF, in_fn[file_level], equation);
						fprintf(stderr, "*** Error %d in \"%s\" @%d: Label not found (%s)!\n", EC_LNF, in_fn[file_level], codeline[file_level], equation);
					}

					equation = AdvanceTo(equation, ',');
				}
#endif
				value		= exp_parser(equation);
				equation	= AdvanceTo(equation, ',');

				/*	Stock value and make space in stack.
				 *	------------------------------------ */	
				LStack->word	= value & 0xff;
				LStack->next	= (STACK *) calloc(1, sizeof(STACK));
				LStack			= (STACK *) LStack->next;

				value				= 0;
				break;
		}
	}

	return (LIST_BYTES);
}


/*	*************************************************************************
 *	Function name:	proc_dw
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_dw(char *label, char *equation)
{
	SYMBOL	*Local;
	STACK		*LStack	= ByteWordStack;
	int		value;
//	int		sign		= 1;


	/*	Don't do anything, if code section is desactivated.
	 *	*/
//	if (if_true[if_nest] == 0)	return (LIST_ONLY);
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	Local = FindLabel(label);

	/* Record the address of the label.
	 * -------------------------------- */
	if (Local)
		Local->Symbol_Value = addr;

	b1		= addr & 0x00ff;
	b2		= (addr & 0xff00) >> 8;
	value	= 0;

	/* The list could be strings, labels, or digits. */

	/* Go to the end of the string.
	 * ---------------------------- */
	while (*equation != '\0')
	{
		switch (*equation)
		{
			case '\'':
			case '"':
			{
				int				pos			= 0;	/*	Position in word (MSB/LSB). */
				unsigned char	in_quote		= 1;	/*	Not in Quote. */

				equation++;			/* Select next characters. */

				while (*equation)
				{
					/*	Check for empty string.
					 *	----------------------- */	
					if ((*equation == '\'') || (*equation == '"'))
					{
						in_quote	= 0;
						break;
					}

					if ((pos & 1) == 0)
						LStack->word	= ((int) *(equation++)) << 8;
					else
					{
						LStack->word	+= *(equation++);
						LStack->next	= (STACK *) calloc(1, sizeof(STACK));
						LStack			= (STACK *) LStack->next;
					}

					pos++;		/*	Select next position in word. */
				}

				/*	- If number of string characters was odd, a new stack
				 *	  space was not created.  So, created it now!
			 	 *	----------------------------------------------------- */	 
				if ((pos & 1) != 0)
				{
					LStack->next	= (STACK *) calloc(1, sizeof(STACK));
					LStack			= (STACK *) LStack->next;
				}

				/*	- If still in quote, and we can display/print error,
				 *	  do it.
			 	 *	---------------------------------------------------- */	 
				if (in_quote)
				{
					if (asm_pass == 1)
					{
						if (list != NULL)
							fprintf(	list,
								  		"*** Warning %d in \"%s\": Missing quote!\n", WC_MQ, in_fn[file_level]);

						fprintf(	stderr,
							  		"*** Warning %d in \"%s\" @%d: Missing quote!\n",
								  	WC_MQ, in_fn[file_level], codeline[file_level]);
					}

					/*	- Notes: We must not select next character at this
					 *	  point, because "equation" have been evalued entirely,
					 *	  and we want "equation" to point to the string
					 *	  delimitor to exit from the main loop.
					 * */	
				}
				else
					equation++;		/*	Select next character. */

				break;
			}	

#if 0			
			case '-':
				sign = -1;
#endif

				/*	Bypass some characters.
				 *	----------------------- */	
			case ',':
			case ' ':
			case '\t':
			case '+':
				equation++;
				break;

#if 0
			case '$':
				value		= exp_parser(equation);
				equation	= AdvanceTo(equation, ',');
				break;
#endif

			default:
#if 0
				/* Could be numbers, or a label.
				 * ----------------------------- */
				if (isdigit((int) *equation))
				{
					value		= exp_parser(equation);
					equation = AdvanceTo(equation, ',');
				}
				else
				{
					Local = FindLabel(equation);

					if (Local)
						value = Local->Symbol_Value;
					else if (asm_pass == 1)
					{
						fprintf(list, "*** Error %d in \"%s\": Label not found (%s)!\n", EC_LNF, in_fn[file_level], equation);
						fprintf(stderr, "*** Error %d in \"%s\" @%d: Label not found (%s)!\n", EC_LNF, in_fn[file_level], codeline[file_level], equation);
					}

					equation = AdvanceTo(equation, ',');
				}
#endif
				value		= exp_parser(equation);
				equation = AdvanceTo(equation, ',');

				/*	Stock value and make space in stack.
				 *	------------------------------------ */	
//				LStack->word	= value * sign;
				LStack->word	= value;
				LStack->next	= (STACK *) calloc(1, sizeof(STACK));
				LStack			= (STACK *) LStack->next;

//				sign				= 1;
				break;
		}
	}

	return (LIST_WORDS);
}


/*	*************************************************************************
 *	Function name:	proc_ds
 *	Description:	Define Space.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_ds(char *label, char *equation)
{
	SYMBOL	*Local;
//	STACK		*LStack	= ByteWordStack;
	int		value;


	/*	Don't do anything, if code section is desactivated.
	 *	*/
//	if (if_true[if_nest] == 0)	return (LIST_ONLY);
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	Local = FindLabel(label);

	/* Record the address of the label.
	 * -------------------------------- */
	if (Local)
		Local->Symbol_Value = addr;

#if 0	
	b1		= addr & 0x00ff;
	b2		= (addr & 0xff00) >> 8;
#endif

	/* Go to the end of the string.
	 * ---------------------------- */
	while (*equation != '\0')
  	{
		switch (*equation)
	  	{
		case '\'':
		case '"':
			if (asm_pass == 1)
			{
				if (list != NULL)
					fprintf(	list, "*** Error %d in \"%s\": String not accepted (%c)!\n", WC_SNA, in_fn[file_level], *equation);

				fprintf(	stderr, "*** Error %d in \"%s\" @%d: Missing quote (%c)!\n", WC_SNA, in_fn[file_level], codeline[file_level], *equation);
			}

			/*	Flush "equation".
			 *	----------------- */	
			while (*equation != '\0')
				equation++;

			break;

		/*	Bypass some characters.
		 *	----------------------- */	
		case ',':
		case ' ':
		case '\t':
			equation++;
			break;

		default:
			/* Could be numbers, or a label.
			 * ----------------------------- */
			if (isdigit((int) *equation))
			{
				value		= exp_parser(equation);
				equation = AdvanceTo(equation, ',');
			}
			else
			{
				Local = FindLabel(equation);

				if (Local)
					value = Local->Symbol_Value;
				else if (asm_pass == 1)
				{
					if (list != NULL)
						fprintf(list, "*** Error %d in \"%s\": Label not found (%s)!\n", EC_LNF, in_fn[file_level], equation);

					fprintf(stderr, "*** Error %d in \"%s\" @%d: Label not found (%s)!\n", EC_LNF, in_fn[file_level], codeline[file_level], equation);
				}

				equation = AdvanceTo(equation, ',');
			}

#if 0			
			addr	+= value;	/*	Make space! */

			if ((addr > 0xFFFF) && (asm_pass == 1))
			{
//				addr	-= value;	/*	Restore address. */

				fprintf(list, "*** Error %d: Address counter overflow (%d)!\n", EC_ACOF, addr);
				fprintf(stderr, "*** Error %d @%d: Address counter overflow (%d)!\n", EC_ACOF, codeline[file_level], addr);
			}
#endif
			data_size	= value;		/*	Space reserved! */

			if ((addr + value) > 0xFFFF)
			{
//				data_size	= 0;		/*	Space reserved! */

				if (asm_pass == 1)
				{
					if (list != NULL)
						fprintf(list, "*** Error %d in \"%s\": Address counter overflow (%d)!\n", EC_ACOF, in_fn[file_level], addr);

					fprintf(stderr, "*** Error %d in \"%s\" @%d: Address counter overflow (%d)!\n", EC_ACOF, in_fn[file_level], codeline[file_level], addr);
				}
			}

			break;
		}
	}

//	return (LIST_STRINGS);
	return (LIST_DS);
}


/*	*************************************************************************
 *	Function name:	proc_include
 *	Description:	"INCLUDE" assembler directive processing.
 *	Author(s):		Claude Sylvain
 *	Created:			27 December	2010
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *
 *	Notes:			- "INCLUDE" is not a standard Intel 8080 assembler
 *						  directive.
 *	************************************************************************* */

static int proc_include(char *label, char *equation)
{
#define OpenIncludeFile_TEXT_SIZE_MAX	256


	char	*p_name;
	char	*p_name_path;
	int	i					= 0;
	char	*p_equation		= equation;
	int	quote_detected	= 0;


	/*	Don't do anything, if code section is desactivated.
	 *	*/
//	if (if_true[if_nest] == 0)	return (LIST_ONLY);
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	process_label(label);

	/*	Allocate memory.
	 *	---------------- */	
	p_name 		= (char *) malloc(OpenIncludeFile_TEXT_SIZE_MAX);
	p_name_path = (char *) malloc(OpenIncludeFile_TEXT_SIZE_MAX);

	/*	Go further more only if able to allocate memory.
	 *	------------------------------------------------ */
	if ((p_name == NULL) || (p_name_path == NULL))
	{
		if (asm_pass == 1)
		{
			if (list != NULL)
				fprintf(list, "*** Error %d in \"%s\": Memory allocation error!\n", EC_MAE, in_fn[file_level]);

			fprintf(stderr, "*** Error %d in \"%s\" @%d: Memory allocation error!\n", EC_MAE, in_fn[file_level], codeline[file_level]);
		}

		/*	Free allocated memory.
		 *	---------------------- */	
		free(p_name);
		free(p_name_path);

		return (LIST_ONLY);
	}

	memset(p_name, 0, OpenIncludeFile_TEXT_SIZE_MAX);

	/*	Search for a quote until end of string or begining of a comment.
	 *	---------------------------------------------------------------- */	
	while (*p_equation != '\0')
	{
		if (*p_equation == '"')
		{
			quote_detected	= 1;
			break;
		}

		++p_equation;
	}

	/*	If a quote was found, process with quote.
	 *	----------------------------------------- */	
	if (quote_detected)
	{
		/*	Search for first quote.
		 *	----------------------- */	
		while (*equation != '\0') 
		{
			if (*(equation++) == '"')
				break;
		}

		/*	- If no starting quote character found, display/print error, and
		 *	  and abort include operation.
		 *	----------------------------------------------------------------- */
		if (*equation == '\0')
		{
			if (asm_pass == 1)
			{
				if (list != NULL)
					fprintf(list, "*** Error %d in \"%s\": No starting quote!\n", EC_NSQ, in_fn[file_level]);

				fprintf(stderr, "*** Error %d in \"%s\" @%d: No starting quote!\n", EC_NSQ, in_fn[file_level], codeline[file_level]);
			}

			/*	Free allocated memory.
			 *	---------------------- */	
			free(p_name);
			free(p_name_path);

			return (LIST_ONLY);
		}

		/*	Grap the include file name.
		 *	--------------------------- */	
		while (*equation != '\0') 
		{
			if (*equation == '"')
			{
				p_name[i]	= '\0';		/*	String delimitor. */
				break;
			}

			p_name[i] = *(equation++);

			/*	Update buffer index, and check for overflow.
			 *	-------------------------------------------- */	
			if (++i >= OpenIncludeFile_TEXT_SIZE_MAX)
			{
				if (asm_pass == 1)
				{
					if (list != NULL)
						fprintf(list, "*** Error %d in \"%s\": Buffer overflow!\n", EC_BOF, in_fn[file_level]);

					fprintf(stderr, "*** Error %d in \"%s\" @%d: Buffer overflow!\n", EC_BOF, in_fn[file_level], codeline[file_level]);
				}

				/*	Free allocated memory.
				 *	---------------------- */	
				free(p_name);
				free(p_name_path);

				return (LIST_ONLY);
			}
		}

		/*	- If there is no ending quote, display/print error, and
		 *	  abort include operation.
		 *	------------------------------------------------------- */	 
		if (*equation == '\0')
		{
			if (asm_pass == 1)
			{
				if (list != NULL)
					fprintf(list, "*** Error %d in \"%s\": No ending quote!\n", EC_NEQ, in_fn[file_level]);

				fprintf(stderr, "*** Error %d in \"%s\" @%d: No ending quote!\n", EC_NEQ, in_fn[file_level], codeline[file_level]);
			}

			/*	Free allocated memory.
			 *	---------------------- */	
			free(p_name);
			free(p_name_path);

			return (LIST_ONLY);
		}
	}
	/*	No quote was found, process without quote.
	 *	------------------------------------------ */	
	else
	{
		/*	Grap the include file name.
		 *	--------------------------- */	
		while ((*equation != '\0') && (isspace((int) *equation) == 0))
		{
			p_name[i] = *(equation++);

			/*	Update buffer index, and check for overflow.
			 *	-------------------------------------------- */	
			if (++i >= OpenIncludeFile_TEXT_SIZE_MAX)
			{
				if (asm_pass == 1)
				{
					if (list != NULL)
						fprintf(list, "*** Error %d in \"%s\": Buffer overflow!\n", EC_BOF, in_fn[file_level]);

					fprintf(stderr, "*** Error %d in \"%s\" @%d: Buffer overflow!\n", EC_BOF, in_fn[file_level], codeline[file_level]);
				}

				/*	Free allocated memory.
				 *	---------------------- */	
				free(p_name);
				free(p_name_path);

				return (LIST_ONLY);
			}
		}

		p_name[i]	= '\0';
	}

	/*	Increment File Level, and check for overflow error.
	 *	--------------------------------------------------- */
	if (++file_level <= FILES_LEVEL_MAX)
	{
		int	file_openned	= 1;

		/*	Open include file.
		 *	****************** */	

		strcpy(p_name_path, p_name);
		get_file_from_path(NULL, NULL, 0);		/*	Init. */

		while ((in_fp[file_level] = fopen(p_name_path,"r")) == NULL)
  		{
			if (get_file_from_path(p_name, p_name_path, OpenIncludeFile_TEXT_SIZE_MAX) == -1)
			{
				--file_level;				/*	Restore. */
				file_openned	= 0;

				/*	Display/print error, if possible.
				 *	--------------------------------- */	
				if (asm_pass == 1)
				{
					if (list != NULL)
						fprintf(list, "*** Error %d in \"%s\": Can't open include file (\"%s\")!\n", EC_COIF, in_fn[file_level], p_name);

					fprintf(stderr, "*** Error %d in \"%s\" @%d: Can't open include file (\"%s\")!\n", EC_COIF, in_fn[file_level], codeline[file_level], p_name);
				}

				break;
			}
		}

		/*	Open include file, and check for error.
		 *	--------------------------------------- */
//		if (in_fp[file_level] != NULL)
		if (file_openned)
		{
			/*	Allocate memory for the input file name.
			 *	*/	
			in_fn[file_level]	= (char *) malloc(strlen(p_name_path) + 1);

			/*	Check for memory allocation error.
			 *	--------------------------------- */	
			if (in_fn[file_level] != NULL)
			{
				strcpy(in_fn[file_level], p_name_path);		/*	Save input file name. */
			}
			else
			{
				if (asm_pass == 1)
				{
					if (list != NULL)
						fprintf(list, "*** Error %d in \"%s\": Memory allocation error!\n", EC_MAE, in_fn[file_level]);

					fprintf(stderr, "*** Error %d in \"%s\" @%d: Memory allocation error!\n", EC_MAE, in_fn[file_level], codeline[file_level]);
				}
			}
		}

	}
	else
	{
		--file_level;			/*	Abort "include". */

		if (asm_pass == 1)
		{
			if (list != NULL)
				fprintf(list, "*** Error %d in \"%s\": Include overflow!\n", EC_IOF, in_fn[file_level]);

			fprintf(stderr, "*** Error %d in \"%s\" @%d: Include overflow!\n", EC_IOF, in_fn[file_level], codeline[file_level]);
		}
	}

	/*	Free allocated memory.
	 *	---------------------- */	
	free(p_name);
	free(p_name_path);

	return (LIST_ONLY);
}


/*	*************************************************************************
 *	Function name:	proc_local
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_local(char *label, char *equation)
{

	/*	Don't do anything, if code section is desactivated.
	 *	*/
//	if (if_true[if_nest] == 0)	return (LIST_ONLY);
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	return (LIST_ONLY);
}


#if 0
/*	*************************************************************************
 *	Function name:	proc_endlocal
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	24 December 2010
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_endlocal(char *label, char *equation)
{

	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (if_true[if_nest] == 0)	return (LIST_ONLY);

	return (0);
}
#endif


#if 0
/*	*************************************************************************
 *	Function name:	proc_anop
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	25 December 2010
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	- This function seems to put the current address in the code.
 *	  This is not a standard Intel 8080 assembler directive.
 *	************************************************************************* */

static int proc_anop(char *label, char *equation)
{
	SYMBOL	*Local;


	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (if_true[if_nest] == 0)	return (LIST_ONLY);

	/* Things are very broken if not found.
	 * */
	Local = FindLabel(label);

	if (!Local)
		return (LIST_ONLY);

	Local->Symbol_Value = addr;

	b1 = Local->Symbol_Value & 0x00ff;
	b2 = (Local->Symbol_Value & 0xff00) >> 8;

	return (TEXT);
}
#endif


/*	*************************************************************************
 *	Function name:	proc_equ
 *	Description:	Process "EQU" assembler directive.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int proc_equ(char *label, char *equation)
{
	SYMBOL	*Local;
	int		tmp;

	/*	Don't do anything, if code section is desactivated.
	 *	*/
//	if (if_true[if_nest] == 0)	return (LIST_ONLY);
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	Local = FindLabel(label);

	if (Local)
		tmp = Local->Symbol_Value = exp_parser(equation);
	else
		tmp = exp_parser(equation);

	check_oor(tmp, 0xFFFF);		/*	Check Operand Over Range. */

	b1	= tmp & 0x00FF;
	b2	= (tmp & 0xFF00) >> 8;

	return TEXT;
}


#if 0
/*	*************************************************************************
 *	Function name:	DAC_proc
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	26 December 2010
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int DAC_proc(char *label, char *equation)
{
	SYMBOL         *Local;
//	STACK          *LStack = ByteWordStack;

	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (if_true[if_nest] == 0)	return (LIST_ONLY);

	Local = FindLabel(label);
	addr = Local->Symbol_Value = exp_parser(equation);

	b1 = Local->Symbol_Value & 0x00ff;
	b2 = (Local->Symbol_Value & 0xff00) >> 8;

	return TEXT;
}
#endif


/*	*************************************************************************
 *	Function name:	ORG_proc
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int ORG_proc(char *label, char *equation)
{
	SYMBOL         *Local;

	/*	Don't do anything, if code section is desactivated.
	 *	*/
//	if (if_true[if_nest] == 0)	return (LIST_ONLY);
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	Local = FindLabel(label);

	if (Local)
		addr = Local->Symbol_Value = exp_parser(equation);
	else
		addr = exp_parser(equation);

	b1 = addr & 0x00ff;
	b2 = (addr & 0xff00) >> 8;

	ProcessDumpBin();
	Target.addr = addr;
	return TEXT;
}


/*	*************************************************************************
 *	Function name:	END_proc
 *	Description:	Process "END" assembler directive.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 February 2011
 *
 *	Parameters:		char *label:
 *							...
 *
 *						char *equation:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int END_proc(char *label, char *equation)
{
	/*	Don't do anything, if code section is desactivated.
	 *	*/
//	if (if_true[if_nest] == 0)	return (LIST_ONLY);
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

//	ProcessDumpBin();

	type				= PROCESSED_END;
//	end_processed	=	1;

	return (PROCESSED_END);
}






