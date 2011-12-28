/*	*************************************************************************
 *	Module Name:	asm_dir.c
 *	Description:	Assembler Directive.
 *	Copyright(c):	See below...
 *	Author(s):		Claude Sylvain
 *	Created:			24 December 2010
 *	Last modified:	27 December 2011
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
#include "msg.h"
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
static int proc_equ(char *, char *);
static int proc_set(char *label, char *equation);
static int proc_org(char *, char *);
static int proc_end(char *, char *);


/*	*************************************************************************
 *												 CONST
 *	************************************************************************* */

/*	Public "const".
 *	*************** */

/*	Assembler Directives.
 *	--------------------- */ 	
const keyword_t	asm_dir[] =
{
	{"EQU", proc_equ},				{"DB", proc_db},
	{"DW", proc_dw},					{"END", proc_end},
  	{"INCLUDE", proc_include},
	{"ORG", proc_org},				{"DS", proc_ds},
	{"IF", proc_if},
	{"ELSE", proc_else},				{"ENDIF", proc_endif},
	{"SET", proc_set},				{"LOCAL", proc_local},
	{0, NULL}
};


/*	*************************************************************************
 *	                           FUNCTIONS DEFINITION
 *	************************************************************************* */


/*	*************************************************************************
 *	Function name:	proc_if
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	27 December 2011
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

	if (++if_nest < (sizeof (if_true) / sizeof (int)))
	{
		if_true[if_nest] = exp_parser(equation);
	}
	else
	{
		if_nest--;
		msg_error("\"IF\" nesting overflow!", EC_INO);
	}

	return (LIST_ONLY);
}


/*	*************************************************************************
 *	Function name:	proc_else
 *	Description:	"ELSE" assembler directive processing.
 *	Author(s):		Claude Sylvain
 *	Created:			24 December	2010
 *	Last modified:	27 December 2011
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
 *	Last modified:	27 December 2011
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

		msg_error("\"IF\" nesting underflow!", EC_INU);
	}

	return (LIST_ONLY);
}


/*	*************************************************************************
 *	Function name:	proc_db
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	27 December 2011
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
	STACK		*LStack	= ByteWordStack;
	int		value;


	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	process_label(label);

	b1		= target.pc & 0x00FF;
	b2		= (target.pc & 0xFF00) >> 8;
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
					msg_warning("Missing quote!", WC_MQ);

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

			default:
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
 *	Last modified:	27 December 2011
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
	STACK		*LStack	= ByteWordStack;
	int		value;


	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	process_label(label);

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
					msg_warning("Missing quote!", WC_MQ);

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
			case '+':
				equation++;
				break;

			default:
				value		= exp_parser(equation);
				equation = AdvanceTo(equation, ',');

				/*	Stock value and make space in stack.
				 *	------------------------------------ */	
				LStack->word	= value;
				LStack->next	= (STACK *) calloc(1, sizeof(STACK));
				LStack			= (STACK *) LStack->next;

				break;
		}
	}

	return (LIST_WORDS);
}


/*	*************************************************************************
 *	Function name:	proc_ds
 *
 *	Description:	- Process DS (Define a block of Storage) assembler
 *						  directive.
 *
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	24 December 2011
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
	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	process_label(label);

	data_size	= exp_parser(equation);	/*	Get memory to reserve. */
	check_evor(data_size, 0xFFFF);		/*	Check Expression Value Over Range. */

	/*	Process Intel Hexadecimal object file.
	 *	*/
	ProcessDumpHex(0);

	/*	- If "data_size" is positive, update "target.pc_org" the normal
	 *	  way.
	 *	- Otherwise, just set "target.pc_org" to "target.pc", since
	 *	  "target.pc" is not updated when "data_size" is negative.
	 * --------------------------------------------------------------- */	  	  
	if (data_size >= 0)
	{
		/*	- Adjust "target.pc_org" to bypass the "DS" memory section.
		 *	- Notes: This is necessary since "DS" memory section do not contain
		 *	  any useful information.
		 *	*/
		target.pc_org	= (target.pc + data_size) & 0xFFFF;
	}
	else
		target.pc_org	= target.pc;

	return (LIST_DS);
}


/*	*************************************************************************
 *	Function name:	proc_include
 *	Description:	"INCLUDE" assembler directive processing.
 *	Author(s):		Claude Sylvain
 *	Created:			27 December	2010
 *	Last modified:	27 December 2011
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
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	/*	Allocate memory.
	 *	---------------- */	
	p_name 		= (char *) malloc(OpenIncludeFile_TEXT_SIZE_MAX);
	p_name_path = (char *) malloc(OpenIncludeFile_TEXT_SIZE_MAX);

	/*	Go further more only if able to allocate memory.
	 *	------------------------------------------------ */
	if ((p_name == NULL) || (p_name_path == NULL))
	{
		msg_error("Memory allocation error!", EC_MAE);

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
			msg_error("No starting quote!", EC_NSQ);

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
				msg_error("Buffer overflow!", EC_BOF);

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
			msg_error("No ending quote!", EC_NEQ);

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
				msg_error("Buffer overflow!", EC_BOF);

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
				msg_error_s("Can't open include file!", EC_COIF, p_name);
				break;
			}
		}

		/*	Open include file, and check for error.
		 *	--------------------------------------- */
		if (file_openned)
		{
			/*	Allocate memory for the input file name.
			 *	*/	
			in_fn[file_level]	= (char *) malloc(strlen(p_name_path) + 1);

			/*	Check for memory allocation error.
			 *	--------------------------------- */	
			if (in_fn[file_level] != NULL)
				strcpy(in_fn[file_level], p_name_path);	/*	Save input file name. */
			else
				msg_error("Memory allocation error!", EC_MAE);
		}

	}
	else
	{
		--file_level;			/*	Abort "include". */

		msg_error("Include overflow!", EC_IOF);
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
 *	Last modified:	24 December 2011
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
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	process_label(label);

	return (LIST_ONLY);
}


/*	*************************************************************************
 *	Function name:	proc_equ
 *	Description:	Process "EQU" assembler directive.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	27 December 2011
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
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	tmp	= exp_parser(equation);

	check_oor(tmp, 0xFFFF);		/*	Check Operand Over Range. */

	/*	If in first assembly pass, add and initialize label.
	 *	---------------------------------------------------- */	
	if (asm_pass == 0)
	{
		/*	- Notes: Do not display error message if no symbol
		 *	  found, since we are in first assembler pass.
	 	 *	-------------------------------------------------- */	 
		Local = FindLabel(label);

		/*	If symbol was found...
		 *	---------------------- */	
		if (Local != NULL)
		{
#if 0
			/*	If previously defined as a "SET", we can re-define it.
			 *	------------------------------------------------------ */	
			if (Local->Symbol_Type == SYMBOL_NAME_SET)
			{
				Local->Symbol_Value	= tmp;
				Local->Symbol_Type	= SYMBOL_NAME_EQU;
			}
#endif
		}
		/*	If symbol do not exist, add it.
		 *	------------------------------- */	
		else
		{
			/*	Add Symbol.
			 *	If able to add it, set its value.
		 	 *	--------------------------------- */	 
			if (AddLabel(label) != -1)
			{
				Local = FindLabel(label);

				if (Local)
				{
					Local->Symbol_Value	= tmp;
					Local->Symbol_Type	= SYMBOL_NAME_EQU;
				}
			}
		}
	}
	/*	- We assume we are in second assembler pass...
	 *   Check for phasing error and/or label/name duplicated.
	 *	- In case there is phasing error and/or label/name
	 *	  duplicated, synchronize the label.
	 *	------------------------------------------------------- */	
	else
	{
		/*	- Notes: Since we are in assembler second pass, if no
		 *	  symbol is found, an error message is displayed.
	 	 *	------------------------------------------------------ */	 
		Local = FindLabel(label);

		/*	If symbol was found...
		 *	---------------------- */	
		if (Local != NULL)
		{
			/*	If previously defined as a "EQU" name...
			 *	---------------------------------------- */	
			if (Local->Symbol_Type == SYMBOL_NAME_EQU)
			{
				/*	- If this is the same "EQU" as defined in the symbols list,
				 *	  check for phasing error.
				 *	----------------------------------------------------------- */	
				if (	(strcmp(in_fn[file_level], Local->src_filename) == 0) &&
					  	(codeline[file_level] == Local->code_line))
				{
					/*	If there is a phasing error.
					 *	---------------------------- */	
					if (tmp != Local->Symbol_Value)
					{
						msg_error_s("Phasing error!", EC_PE, label);

						/*	- When there is a phasing error, we assume that
						 *	  the actual "EQU" value is the best one.
						 *	  So, update the symbol value.
					 	 *	*/	 
						Local->Symbol_Value	= tmp;
					}
				}
				/*	"EQU" was already defined.
				 *	Display an error message, since "EQU" can not be re-defined.
				 *	------------------------------------------------------------ */	 
				else
				{
					msg_error_s("\"EQU\" already exist!", EC_EAE, label);
				}
			}
			/*	If previously defined as a "SET", we can re-define it.
			 *	------------------------------------------------------ */	
			else if (Local->Symbol_Type == SYMBOL_NAME_SET)
			{
				Local->Symbol_Value	= tmp;
				Local->Symbol_Type	= SYMBOL_NAME_EQU;
				Local->code_line		= codeline[file_level];

				/*	Update source file name.
				 *	************************ */

				Local->src_filename	=
					(char *) realloc(Local->src_filename, strlen(in_fn[file_level]) + 1);

				if (Local->src_filename != NULL)
				{
					strcpy(Local->src_filename, in_fn[file_level]);
				}
				else
				{
					msg_error_s("Can't allocate memory!", EC_CAM, label);

					/*	Get it pointing something.
					 *	*/	
					Local->src_filename	= empty_string;
				}
			}
			/*	If already defined as a name, we can not re-defined it.
			 *	------------------------------------------------------- */	
			else if (Local->Symbol_Type == SYMBOL_NAME)
			{
				msg_error_s("Already defined as a \"Name\"!", EC_ADAN, label);
			}
			/*	- We assume that symbol was already defined as a "label",
			 *	  and can not be re-defined.
		 	 *	--------------------------------------------------------- */
			else
			{
				msg_warning_s("Symbol already used as \"label\"!", WC_SAUAL, label);
			}
		}
	}

	b1	= tmp & 0x00FF;
	b2	= (tmp & 0xFF00) >> 8;

	return (TEXT);
}


/*	*************************************************************************
 *	Function name:	proc_set
 *	Description:	Process "SET" assembler directive.
 *	Author(s):		Claude Sylvain
 *	Created:			25 December 2011
 *	Last modified:	27 December 2011
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

static int proc_set(char *label, char *equation)
{
	SYMBOL	*Local;
	int		tmp;

	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	tmp	= exp_parser(equation);

	check_oor(tmp, 0xFFFF);		/*	Check Operand Over Range. */

	/*	If in first assembly pass, add and initialize label.
	 *	---------------------------------------------------- */	
	if (asm_pass == 0)
	{
		/*	- Notes: Do not display error message if no symbol
		 *	  found, since we are in first assembler pass.
	 	 *	-------------------------------------------------- */	 
		Local = FindLabel(label);

		/*	If Symbol already exist...
		 *	-------------------------- */
		if (Local != NULL)
		{
			/*	If Symbol is the "name" of a "SET", we can re-define it.
			 *	-------------------------------------------------------- */	
			if (Local->Symbol_Type == SYMBOL_NAME_SET)
			{
				Local->Symbol_Value	= tmp;
				Local->code_line		= codeline[file_level];

				/*	Update source file name.
				 *	************************ */

				Local->src_filename	=
					(char *) realloc(Local->src_filename, strlen(in_fn[file_level]) + 1);

				if (Local->src_filename != NULL)
				{
					strcpy(Local->src_filename, in_fn[file_level]);
				}
				else
				{
					msg_error_s("Can't allocate memory!", EC_CAM, label);

					/*	Get it pointing something.
					 *	*/	
					Local->src_filename	= empty_string;
				}
			}
		}
		/*	Name do not still exist.
		 *	Add it.
		 *	------------------------ */
		else
		{
			/*	Add symbol.
			 *	If able to add it, set its value.
			 *	--------------------------------- */	 
			if (AddLabel(label) != -1)
			{
				Local = FindLabel(label);

				if (Local != NULL)
				{
					Local->Symbol_Value	= tmp;
					Local->Symbol_Type	= SYMBOL_NAME_SET;
				}
			}
		}
	}
	/*	- We assume we are in second assembler pass...
	 *   Check for phasing error and/or label/name duplicated.
	 *	- In case there is phasing error and/or label/name
	 *	  duplicated, synchronize the label.
	 *	------------------------------------------------------- */	
	else
	{
		Local = FindLabel(label);

		/*	If Symbol already exist...
		 *	-------------------------- */
		if (Local != NULL)
		{
			/*	- If Symbol is the "name" of an "EQU", we can
			 *	  not re-define it.
			 *	--------------------------------------------- */	
			if (Local->Symbol_Type == SYMBOL_NAME_EQU)
			{
				msg_error_s("Already defined as an \"EQU\"!", EC_ADAE, label);
			}
			/*	If previously defined as a "SET" name...
			 *	---------------------------------------- */	
			else if (Local->Symbol_Type == SYMBOL_NAME_SET)
			{
				/*	- If this is the same "SET" as defined in the symbols list,
				 *	  check for phasing error.
				 *	----------------------------------------------------------- */	
				if (	(strcmp(in_fn[file_level], Local->src_filename) == 0) &&
					  	(codeline[file_level] == Local->code_line))
				{
					/*	If there is a phasing error.
					 *	---------------------------- */	
					if (tmp != Local->Symbol_Value)
					{
						msg_error_s("Phasing error!", EC_PE, label);

						/*	- When there is a phasing error, we assume that
						 *	  the actual "EQU" value is the best one.
						 *	  So, update the symbol value.
					 	 *	*/	 
						Local->Symbol_Value	= tmp;
					}
				}
				/*	"SET" was already defined, and can be re-defined.
				 *	So, re-defined it.	
				 *	------------------------------------------------- */	 
				else
				{
					Local->Symbol_Value	= tmp;
					Local->code_line		= codeline[file_level];

					/*	Update source file name.
					 *	************************ */

					Local->src_filename	=
						(char *) realloc(Local->src_filename, strlen(in_fn[file_level]) + 1);

					if (Local->src_filename != NULL)
					{
						strcpy(Local->src_filename, in_fn[file_level]);
					}
					else
					{
						msg_error_s("Can't allocate memory!", EC_CAM, label);

						/*	Get it pointing something.
						 *	*/	
						Local->src_filename	= empty_string;
					}
				}
			}
			/*	If symbol is a "name", do not re-define it.
			 *	------------------------------------------- */	
			else if (Local->Symbol_Type == SYMBOL_NAME)
			{
				msg_error_s("Already defined as a \"Name\"", EC_ADAN, label);
			}
			/*	- If symbol is not a "name", we assume that this is
			 *	  a "label"...
	 		 *	--------------------------------------------------- */	 
			else
			{
				msg_warning_s("Symbol already used as \"Label\"!", WC_SAUAL, label);
			}
		}
	}

	b1	= tmp & 0x00FF;
	b2	= (tmp & 0xFF00) >> 8;

	return (TEXT);
}


/*	*************************************************************************
 *	Function name:	proc_org
 *	Description:	ORG directive Processing.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	27 December 2011
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

static int proc_org(char *label, char *equation)
{
	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	/*	- Process Intel Hexadecimal object file.
	 *	- Notes: This must be done before setting PC accordingly
	 *	  to the "ORG" assembler directive.
	 */
	ProcessDumpHex(0);

	set_pc(exp_parser(equation));		/*	Set the program counter. */

	process_label(label);

	/*	Check for Program Counter Over Range.
	 *	------------------------------------- */	
	if ((target.addr < 0) || (target.addr > 0xFFFF))
		msg_error("Program counter over range!", EC_PCOR);

	target.pc_org	= target.pc;
	
	b1	= target.pc & 0x00FF;
	b2	= (target.pc & 0xFF00) >> 8;

	return (TEXT);
}


/*	*************************************************************************
 *	Function name:	proc_end
 *	Description:	Process "END" assembler directive.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	24 December 2011
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

static int proc_end(char *label, char *equation)
{
	/*	Don't do anything, if code section is desactivated.
	 *	*/
	if (util_is_cs_enable() == 0)	return (LIST_ONLY);

	type	= PROCESSED_END;

	return (PROCESSED_END);
}






