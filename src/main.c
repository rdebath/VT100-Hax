/*	*************************************************************************
 *	Module Name:	main.c
 *	Description:	Main module for Intel 8080 Assembler.
 *	Copyright(c):	See below...
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	10 December 2011
 *
 * Notes:
 *						- The assembler assumes that the left column is a label,
 *						  the mid column is an opcode, and the right column is an
 *						  equation.
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
#include "asm_dir.h"
#include "opcode.h"
#include "exp_parser.h"
#include "main.h"


/*	*************************************************************************
 *											  CONSTANTS
 *	************************************************************************* */

#define SRC_LINE_WIDTH_MAX			256		/*	Source Line Maximum Width. */
#define FN_BASE_SIZE					80
#define FN_IN_SIZE					80
#define FN_OUT_SIZE					(FN_BASE_SIZE + 4)


/*	*************************************************************************
 *												 STRUCT
 *	************************************************************************* */

/*	Structure that hold an "-I" Option.
 *	----------------------------------- */
struct option_i_t
{
	char					*path;
	struct option_i_t	*next;
};


/*	*************************************************************************
 *												 CONST
 *	************************************************************************* */

/*	Public const.
 *	************* */

const char	*name_pgm	= "asm8080";		/*	Program Name. */


/*	Local const.
 *	************ */

/*	Program Version.
 *	---------------- */
static const unsigned char	pgm_version_v	= 0;	/*	Version. */
static const unsigned char	pgm_version_sv	= 9;	/*	Sub-Version. */
static const unsigned char	pgm_version_rn	= 6;	/*	Revision Number. */


/*	*************************************************************************
 *	                          FUNCTIONS DECLARATION
 *	************************************************************************* */

/*	Private functions.
 *	****************** */

static void check_new_pc(int count);
static int update_pc(int count);
static void print_symbols_table(void);
static void init(void);
static int process_option_i(char *text);
static int process_option_l(char *text);
static int process_option_o(char *text);
static int check_set_output_fn(void);
static void CloseFiles(void);
static void RewindFiles(void);
static void DumpBin(void);
static void do_asm(void);
static void PrintList(char *text);
static int OpenFiles(void);
static void display_help(void);
static int src_line_parser(char *text);
static void AddLabel(char *text);
static void asm_pass1(void);
static void asm_pass2(void);
static void clean_up(void);
static int process_input_file(char *text);
static int cmd_line_parser(int argc, char *argv[]);


/*	*************************************************************************
 *	                                VARIABLES
 *	************************************************************************* */

/*	Public variables.
 *	***************** */

int	if_true[10];
int	if_nest		= 0;

int	data_size	= 0;

int	b1	= 0;
int	b2	= 0;
int	b3	= 0;
int	b4	= 0;

int asm_pass;			/* Assembler Pass. */

FILE	*list		= NULL;

STACK	*ByteWordStack;

//TARG	Target;
TARG	target;

int	type;

FILE	*in_fp[FILES_LEVEL_MAX];
char	*in_fn[FILES_LEVEL_MAX];		/*	Input File Name. */
int codeline[FILES_LEVEL_MAX];

FILE	*bin;
int	file_level	= 0;

/* Global storage for assembler */

//int	addr;						/*	The program counter. */

SYMBOL	*Symbols;

char Image[1024 * 64];


/*	Private variables.
 *	****************** */

/*	TODO: Size this string dynamically.
 *	*/
static char	fn_base[FN_BASE_SIZE];

static char	*list_file	= NULL;
static char	*bin_file	= NULL;

/*	Single linked list that old all "-I" options.
 *	*/	
static struct	option_i_t	option_i	= {NULL, NULL};


/*	*************************************************************************
 *	                           FUNCTIONS DEFINITION
 *	************************************************************************* */


/*	*************************************************************************
 *	Function name:	check_new_pc
 *	Description:	Check the New Program Counter value.
 *	Author(s):		Claude Sylvain
 *	Created:			4 December 2011
 *	Last modified:	10 December 2011
 *
 *	Parameters:		int count:
 *							Count that will be added to the program counter.
 *
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void check_new_pc(int count)
{
	int	new_pc	= target.pc + count;	/*	Calculate new PC value. */

	/*	- Check if the new program counter is out of range, and
	 *	  manage messages if necessary.
	 *	------------------------------------------------------- */	 
	if (	((target.pc_or != 0) || ((new_pc < 0) || (new_pc > 0x10000))) &&
		  	(asm_pass == 1))
	{
		if (list != NULL)
			fprintf(	list, "*** Error %d in \"%s\": Program counter over range (%d)!\n",
				  		EC_PCOR, in_fn[file_level], target.pc);

		fprintf(	stderr, "*** Error %d in \"%s\" @%d: Program counter over range (%d)!\n",
			  		EC_PCOR, in_fn[file_level], codeline[file_level], target.pc);
	}
}


/*	*************************************************************************
 *	Function name:	update_pc
 *	Description:	Update Program Counter.
 *	Author(s):		Claude Sylvain
 *	Created:			4 December 2011
 *	Last modified:	10 December 2011
 *
 *	Parameters:		int count:
 *							Count to add to the program counter.
 *
 *	Returns:			int:
 *							-1	: - Can not update program counter, because
 *									 of an out of range error.  Program counter
 *									 is resetted to 0x0000.
 *
 *							0	: Program counter updated successfully.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int update_pc(int count)
{
	int	rv	= 0;		/*	Return Value. */

//	addr	+= count;	/*	Update program counter. */
	target.pc	+= count;	/*	Update program counter. */

	if (target.pc <= 0x10000)
		target.mem_size	= target.pc;

	/*	- Check if program counter is out of range, and
	 *	  reset it if necessary.
	 *	----------------------------------------------- */	 
	if ((target.pc < 0) || (target.pc > 0xFFFF))
	{
		target.pc		= 0;
		target.pc_or	= 1;		/*	PC Over Range detected! */
		rv					= -1;		/*	Program counter is out of range. */
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	get_file_from_path
 *	Description:	Get File name From Path.
 *	Author(s):		Claude Sylvain
 *	Created:			31 December 2010
 *	Last modified:	1 January 2010
 *
 *	Parameters:		char *fn:
 *							- Point to a string that hold file name to which
 *							  path must be applied.
 *
 *						char fn_path:
 *							- Point to a buffer that will receive the file
 *							  name prefixed by a path.
 *
 *						size_t fn_path_size:
 *							Size of "fn_path" buffer.
 *
 *	Returns:			void
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

int get_file_from_path(char *fn, char* fn_path, size_t fn_path_size)
{
	static struct option_i_t	*p_option_i_cur;

	int		rv			= -1;

	/*	- If one the the parameter is NULL or 0, we assume that caller
	 *	  want us to initialize.
 	 *	-------------------------------------------------------------- */	 
	if ((fn == NULL) || (fn_path == NULL) || (fn_path_size == 0))
	{
		p_option_i_cur	= &option_i;
		rv					= 0;					/*	Success! */
	}
	/*	Caller want us to add a path to its file name.  So, do it...
	 *	------------------------------------------------------------ */	
	else
	{
		/*	If there is a file name path available, go further more...
		 *	---------------------------------------------------------- */	
		if (p_option_i_cur->next != NULL)
		{
			size_t	fn_len;

			/*	- Calculate the lenght of the string that will be put
			 *	  in the caller buffer.
			 *	*/	 
			fn_len	= strlen(p_option_i_cur->path) + strlen(fn) + 1;

			/*	If caller buffer is enough large, go further more...
			 *	---------------------------------------------------- */	
			if (fn_path_size >= fn_len)
			{
				/*	- Copy file name prefixed by path to the caller
				 *	  buffer.
			 	 *	----------------------------------------------- */	 
				strcpy(fn_path, p_option_i_cur->path);
				strcat(fn_path, fn);

				p_option_i_cur	= p_option_i_cur->next;		/*	Next path. */
				rv					= 0;								/*	Success! */
			}
			/*	Caller buffer too small :-(
			 *	--------------------------- */	
			else
			{
				fprintf(stderr, "*** Error %d: Buffer too small!\n", EC_BTS);
			}
		}
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	print_symbols_table
 *	Description:	Print Symbols Table.
 *	Author(s):		Claude Sylvain
 *	Created:			31 December 2010
 *	Last modified:
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void print_symbols_table(void)
{
#define pst_TAB_LENGHT				8
#define pst_SYMBOL_FIELD_SIZE		(9 * pst_TAB_LENGHT)

	/*	Print symbols table only on second assembly pass.
	 *	------------------------------------------------- */	
	if (asm_pass == 1)
	{
		int		i;
		int		tab_cnt;
		size_t	str_len;
		SYMBOL	*Local	= Symbols;

		fprintf(list, "\n\n");
		fprintf(list, "*******************************************************************************\n");
		fprintf(list, "                                 Symbols table\n");
		fprintf(list, "*******************************************************************************\n");
		fprintf(list, "\n");
		fprintf(list, "Names\t\t\t\t\t\t\t\t\tValues\n");
		fprintf(list, "-----\t\t\t\t\t\t\t\t\t------\n");

		/*	Print symbols name and value.
		 *	----------------------------- */	
		do
		{
			str_len	= strlen(Local->Symbol_Name);

			/*	If there is something to print...
			 *	--------------------------------- */	
			if (str_len > 0)
			{
				/*	- Calculate number of tabulation character to
				 *	  print.
			 	 *	--------------------------------------------- */	 
				tab_cnt	= (pst_SYMBOL_FIELD_SIZE - (int) str_len) / 8;

				/*	- Add "1" if space to fill is not multiple of
				 *	  tabulation size.  This can be seen directly
				 *	  from "str_len".
			 	 *	--------------------------------------------- */	 
				if ((str_len % pst_TAB_LENGHT) != 0)
					tab_cnt++;

				fprintf(list, "%s", Local->Symbol_Name);			/*	Name. */

				/*	Space between fields (filled with "tab").
				 *	----------------------------------------- */	
				for (i = 0; i < tab_cnt; i++)
					fprintf(list, "\t");

				fprintf(list, "%05Xh\n", Local->Symbol_Value);	/*	Value. */
			}

			Local = (SYMBOL *) Local->next;		/*	Select next symbols. */
		}
		while (Local);

		fprintf(list, "\n\n");
	}
}


/*	*************************************************************************
 *	Function name:	OpenFiles
 *	Description:	Open Files.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	26 March 2011
 *	Parameters:		void
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: Operation successfull.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int OpenFiles(void)
{
	/* Remove old output files, if any.
	 *	******************************** */

	if (list_file != NULL)
		remove(list_file);

	remove(bin_file);


	/*	Open files.
	 *	*********** */

	if ((in_fp[0] = fopen(in_fn[0], "r")) == NULL)
  	{
		fprintf(stderr, "*** Error %d: Can't open input file (\"%s\")!\n", EC_COINF, in_fn[0]);
		return (-1);
	}

	/*	If have to output a listing, create it and check for error.
	 *	----------------------------------------------------------- */	
	if ((list_file != NULL) && (list = fopen(list_file, "w")) == NULL)
  	{
		fprintf(stderr, "*** Error %d: Can't open listing file (\"%s\")!\n", EC_COLF, list_file);
		return (-1);
	}

	if ((bin = fopen(bin_file, "wb")) == NULL)
  	{
		fprintf(stderr, "*** Error %d: Can't open binary file (\"%s\")!\n", EC_COBF, bin_file);
		return (-1);
	}

	return (0);
}


/*	*************************************************************************
 *	Function name:	RewindFiles
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	27 December 2010
 *	Parameters:
 *	Returns:	
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void RewindFiles(void)
{
	fseek(in_fp[0], 0, SEEK_SET);
}


/*	*************************************************************************
 *	Function name:	CloseFiles
 *	Description:	Close Files.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	26 March 2011
 *	Parameters:		void
 *	Returns:			void
 *
 *	Globals:			FILE *list
 *						FILE *bin
 *
 *	Notes:
 *	************************************************************************* */

static void CloseFiles(void)
{
	/*	Close files, if necessary.
	 *	-------------------------- */
	if (in_fp[0] != NULL)	fclose(in_fp[0]);		/*	Source. */
	if (list != NULL)			fclose(list);			/*	Listing. */
	if (bin != NULL)			fclose(bin);			/*	Binary. */
}


/*	*************************************************************************
 *	Function name:	src_line_parser
 *	Description:	Break down a source line.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	26 November 2011
 *
 *	Parameters:		char *text:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int src_line_parser(char *text)
{
	char	keyword[16];
	char	keyword_uc[sizeof (keyword)];	/*	OpCode in Upper Case. */
	int	i					= 0;
	int	msg_displayed	= 0;
	char	Equation[80];
	char	label[LABEL_SIZE_MAX];
	int	status;

	/*	If this is a comment, don't do anything.
	 *	---------------------------------------- */	
	if (text[0] == ';')
	{
		/*	TODO: Why "type" and "status" are not the same ???
		 *	*/

		/* Process comment statement in source stream.
		 * */
		type		= COMMENT;

		status	= LIST_ONLY;
	}
	/* Record any symbol and the cop.
	 * ------------------------------ */
  	else
  	{
		keyword_t   *p_keyword;

		memset(label, 0, sizeof (label));
		memset(keyword, 0, sizeof (keyword));
		memset(keyword_uc, 0, sizeof (keyword_uc));
		memset(Equation, 0, sizeof (Equation));


		/*	Grab the label, if any.
		 *	*********************** */

		/*	- Notes: Because we filter comments above, we know that a non
		 *	  blank will be a symbol.
		 * ------------------------------------------------------------- */
		if (isascii(text[0]) != 0)
	  	{
			/*	- FIXME: No check done to see if label start with a
			 *	  character :-/
		 	 *	*/

			/*	TODO: Is there overflow check done in this function.
		 	 * */
			AddLabel(text);

			/*	TODO: Is this standard Intel assembler code?
			 *	-------------------------------------------- */	
			if (*text == '&')	label[i++] = *(text++);
			if (*text == '%')	label[i++] = *(text++);

			while (isalnum((int) *text) || (*text == '_'))
			{
				if (i < (sizeof (label) - 3))
				{
					label[i]	= *(text++);
					i++;
				}
				else
				{
					text++;

					/*	- Display/Print error message, if not already done,
					 *	  and necessary.
					 *	--------------------------------------------------- */
					if (!msg_displayed && (asm_pass == 1))
					{
						msg_displayed	= 1;	/*	No more message. */

						if (list != NULL)
							fprintf(list, "*** Error %d in \"%s\": Label too long (\"%s\")!\n", EC_LTL, in_fn[file_level], label);

						fprintf(stderr, "*** Error %d in \"%s\" @%d: Label too long (\"%s\")!\n", EC_LTL, in_fn[file_level], codeline[file_level], label);
					}
				}
			}
		}
		/*	Not an ASCII character :-/
		 *	-------------------------- */	
		else
		{
			if (asm_pass == 1)
			{
				if (list != NULL)
					fprintf(list, "*** Error %d in \"%s\": Non ASCII character ('%c')!\n", EC_NAC, in_fn[file_level], *text);

				fprintf(stderr, "*** Error %d in \"%s\" @%d: Non ASCII character ('%c')!\n", EC_NAC, in_fn[file_level], codeline[file_level], *text);
			}

			/*	Flush non ASCII characters.
			 *	--------------------------- */	
			while ((isascii(*text) == 0) && (*text != '\0'))
				text++;
		}


		/*	Bypass label delimitors, if necessary.
		 *	************************************** */

		while (isspace((int) *text))
			text++;

		if (*text == ':')
			text++;

		while (isspace((int) *text))
			text++;


		if ((*text == '\0') || (*text == ';'))
		{
			/*	TODO: Why "type" and "status" are not the same ???
			 *	*/

			/* Process comment statement in source stream.
			 * */
			type		= COMMENT;

			return (LIST_ONLY);
		}


		/*	Grab the keyword (assembler directive or opcode).
		 *	************************************************* */

		i					= 0;
		msg_displayed	= 0;

		while (isalnum((int) *text))
		{
			if (i < (sizeof (keyword) - 1))
			{
				keyword[i]		= *text;
				keyword_uc[i]	= toupper((int) *text);
				text++;
				i++;
			}
			else
			{
				text++;

				/*	- Display/Print error message, if not already done,
				 *	  and necessary.
				 *	--------------------------------------------------- */
				if (!msg_displayed && (asm_pass == 1))
				{
					msg_displayed	= 1;	/*	No more message. */

					if (list != NULL)
						fprintf(list, "*** Error %d in \"%s\": Keyword too long (\"%s\")!\n", EC_KTL, in_fn[file_level], keyword);

					fprintf(stderr, "*** Error %d in \"%s\" @%d: Keyword too long (\"%s\")!\n", EC_KTL, in_fn[file_level], codeline[file_level], keyword);
				}
			}
		}

		/*	Bypass delimitors, if necessary.
		 *	-------------------------------- */
		while (isspace((int) *text))
			text++;

		/*	Copy third field to equation buffer.
		 *	************************************	*/

		if ((*text != '\0') && (*text != ';'))
		{
			i					= 0;
			msg_displayed	= 0;

			while ((iscntrl((int) *text) == 0) && (*text != ';'))
			{
				if (i < (sizeof (Equation) - 1))
				{
					Equation[i]	= *(text++);
					i++;
				}
				else
				{
					text++;

					/*	- Display/Print error message, if not already done,
					 *	  and necessary.
					 *	--------------------------------------------------- */
					if (!msg_displayed && (asm_pass == 1))
					{
						msg_displayed	= 1;	/*	No more message. */

						if (list != NULL)
							fprintf(list, "*** Error %d in \"%s\": Equation too long (\"%s\")!\n", EC_ETL, in_fn[file_level], Equation);

						fprintf(stderr, "*** Error %d in \"%s\" @%d: Equation too long (\"%s\")!\n", EC_ETL, in_fn[file_level], codeline[file_level], Equation);
					}
				}
			}
		}

		/*	- Lookup for assembler directives, and call associated
		 *	  function if necessary.
		 *	****************************************************** */

		p_keyword	= (keyword_t *) asm_dir;

		while (p_keyword->Name)
		{
			/*	If keyword is found, call the associated function.
			 *	-------------------------------------------------- */	
			if (!strcmp(p_keyword->Name, keyword_uc))
		  	{
				status	= p_keyword->fnc(label, Equation);
				type		= status;

				if (status == PROCESSED_END)
					return (status);

				break;
			}
		  	else
			{
				p_keyword++;
			}
		}

		/*	If assembler directive was found and processed, exit.
		 *	----------------------------------------------------- */	
		if (p_keyword->Name != NULL)
			return (status);


		/*	- Lookup for opcodes, and call associated
		 *	  function if necessary.
		 *	***************************************** */

		/*	- Check for opcodes search result only if code section
		 *	  is active.
		 *	------------------------------------------------------ */
		if (util_is_cs_enable() == 1)
		{
			p_keyword	= (keyword_t *) OpCodes;

			while (p_keyword->Name)
			{
				/*	If keyword is found, call the associated function.
				 *	-------------------------------------------------- */	
				if (!strcmp(p_keyword->Name, keyword_uc))
				{
					status	= p_keyword->fnc(label, Equation);
					type		= status;
					break;
				}
				else
				{
					p_keyword++;
				}
			}

			/*	if no opcode (and no assembler directive) found...
			 *	-------------------------------------------------- */
			if (p_keyword->Name == NULL)
			{
				type		= COMMENT;
				status	= COMMENT;

				if (asm_pass == 1)
				{
					if (list != NULL)
						fprintf(list, "*** Error %d in \"%s\": Can't find keyword \"%s\"!\n", EC_CNFK, in_fn[file_level], keyword);

					fprintf(stderr, "*** Error %d in \"%s\" @%d: Can't find keyword \"%s\"!\n", EC_CNFK, in_fn[file_level], codeline[file_level], keyword);
				}
			}
		}
		else
		{
			type		= LIST_ONLY;
			status	= LIST_ONLY;
		}

	}

	return (status);
}


/*	*************************************************************************
 *	Function name:	PrintList
 *
 *	Description:	- Output list format.
 *						  [addr] [bn] [bn] [bn] [b4] lab^topcode^taddr^tcomment
 *
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	10 December 2011
 *
 *	Parameters:		char *text:
 *							...
 *
 *	Returns:			void
 *
 *	Globals:			int target.pc
 *						int data_size
 *
 *	Notes:
 *	************************************************************************* */

static void PrintList(char *text)
{
	STACK	*LStack	= ByteWordStack;
	int	space		= 0;

	/*	Don't print list if not in assembler pass 1.
	 *	*/
	if (asm_pass != 1)	return;

	switch (type)
	{
		case COMMENT:
			if (list != NULL)
				fprintf(list, "\t\t\t%s\n", text);

			break;

		case TEXT:
			switch (data_size)
			{
				case 1:
					check_new_pc(data_size);		/*	Check the new PC value. */

					if (list != NULL)
						fprintf(	list, "%6d %04X %02X\t\t%s\n", codeline[file_level],
							  		target.pc, b1, text);

					break;

				case 2:
					check_new_pc(data_size);		/*	Check the new PC value. */

					if (list != NULL)
						fprintf(	list, "%6d %04X %02X %02X\t%s\n",
									codeline[file_level], target.pc, b1, b2, text);

					break;

				case 3:
					check_new_pc(data_size);		/*	Check the new PC value. */

					if (list != NULL)
						fprintf(	list, "%6d %04X %02X %02X %02X\t%s\n",
									codeline[file_level], target.pc, b1, b2, b3, text);

					break;

				case 4:
					check_new_pc(data_size);		/*	Check the new PC value. */

					if (list != NULL)
						fprintf(	list, "%6d %04X %02X %02X %02X %02X\t%s\n",
									codeline[file_level], target.pc, b1, b2, b3, b4, text);

					break;

				/* 0
				 * - */
				default:
					if (list != NULL)
						fprintf(list, "           %02X %02X\t%s\n", b2, b1, text);

					break;
			}
			break;

		case LIST_ONLY:
		case PROCESSED_END:
			if (list != NULL)
				fprintf(list, "\t\t\t%s\n", text);

			break;

		/*	List "DS" (Data Storage).
		 *	------------------------- */	
		case LIST_DS:
			check_new_pc(data_size);		/*	Check the new PC value. */

			if (list != NULL)
				fprintf(	list, "%6d %04X\t\t%s\n", codeline[file_level],
					  		target.pc, text);

			break;

		case LIST_BYTES:
		case LIST_WORDS:
		case LIST_STRINGS:
			data_size	= 0;

			/* - Set "data_size" by counting space that will be
			 *   taken by the list elements.
			 *	------------------------------------------------ */
			while (LStack->next)
			{
				if ((type == LIST_BYTES) || (type == LIST_STRINGS))
					data_size++;
				/*	Assuming this is a "WORD".
				 *	-------------------------- */
				else
					data_size	+= 2;

				LStack = (STACK *) LStack->next;
			}

			check_new_pc(data_size);		/*	Check the new PC value. */

			/*	Restore stack pointer.
			 *	*/
			LStack	= ByteWordStack;

			if (list != NULL)
			{
				fprintf(	list, "%6d %04X\t\t%s\n", codeline[file_level],
					  		target.pc, text);
				fprintf(list, "            ");
			}

			/*	Process all elements in the linked list.
			 *	---------------------------------------- */
			while (LStack->next)
			{
				if ((type == LIST_BYTES) || (type == LIST_STRINGS))
				{
					if (list != NULL)
						fprintf(list, "%02X ", (LStack->word & 0xff));

					space += 3;
				}
				/*	Assuming this is a "WORD".
				 *	-------------------------- */
				else
				{
					if (list != NULL)
						fprintf(list, "%04X ", LStack->word & 0xffff);

					space				+= 5;
				}

				LStack = (STACK *) LStack->next;

				/*	Change line, if necessary.
				 *	-------------------------- */
				if (space >= (4 * 3))
				{
					if (list != NULL)
						fprintf(list, "\n            ");

					space = 0;
				}
			}

			if (list != NULL)
				fprintf(list, "\n");

			break;

		default:
			break;
	}

	/*	Remove after debug.
	 *	------------------- */
	if (list != NULL)
		fflush(list);
}


/*	*************************************************************************
 *	Function name:	ProcessDumpBin
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	10 December 2011
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *
 *	- This code needs to handle target address for the binary.
 *	  When the assembler finds an 'org' statement, we should set
 *	  the output address to 'org' and process a block of binary
 *	  that goes to that 'org' address.  
 *
 *	- Things that control this are, 'org' and 'end'.  The first
 *	  bytes out, will be to the 'org' address or zero if 'org' is
 *	  not issued.  We will continue to push bytes until a new
 *	  'org' is processed or until 'end' is processed.
 *
 *	- The header will have {short count}{short addr} followed by
 *	  count bytes.
 *	************************************************************************* */

void ProcessDumpBin(void)
{
	/*	If there is something to write...
	 *	--------------------------------- */
//	if ((target.pc > 0) && (asm_pass == 1))
	if ((target.mem_size > 0) && (asm_pass == 1))
	{
		/*	Write binary.
		 *	------------- */
#if USE_BINARY_HEADER
		fwrite(&target, sizeof (TARG), 1, bin);	/*	Code size and base address. */
#endif
//		fwrite(&Image, target.pc, 1, bin);			/*	Code. */
		fwrite(&Image, target.mem_size, 1, bin);	/*	Code. */
	}
}


/*	*************************************************************************
 *	Function name:	do_asm
 *	Description:	Assemble source file.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	10 December 2011
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void do_asm(void)
{
	char	*p_text;
	char	*p_text_1;
	int	EmitBin;
	int	eol_found;			/*	End Of Line Found. */


	/*	- Allocated memory for source line buffer, and check for
	 *	  memory allocation error.
	 *	-------------------------------------------------------- */	  
	if ((p_text = (char *) malloc(SRC_LINE_WIDTH_MAX)) == NULL)
	{
		fprintf(stderr, "*** Error %d: Can't allocate memory!\n", EC_CAM);

		return;
	}

	/*	Assemble source file.
	 *	--------------------- */
	while (1)
	{
		EmitBin	= LIST_ONLY;
		type 		= LIST_ONLY;
		codeline[file_level]++;

		/*	- Get a source file line.
		 *	- If not able to get a line...
		 *	- Notes: "fgets()" add a '\0' after the last character,
		 *	  in the buffer.
	 	 *	------------------------------------------------------- */	 
		if (fgets(p_text, SRC_LINE_WIDTH_MAX, in_fp[file_level]) == NULL)
		{
			/*	If source file is an included file, ...
			 *	--------------------------------------- */		  
			if (file_level > 0)
			{
				/*	Close input file handle.
				 *	------------------------ */	
				fclose(in_fp[file_level]);
				in_fp[file_level]	= NULL;

				/*	Free memory allocated for the input file name.
				 *	---------------------------------------------- */	
				free(in_fn[file_level]);
				in_fn[file_level]	= NULL;

				file_level--;
				continue;			/*	Restart assembly process at lower level. */
			}
			/*	- We are at level 0 (main source file), and there is no
			 *	  other file to process, process binary output, if necessary
			 *	  and exit.
			 *	- Notes: Main source file will be close later; elsewhere.
		 	 *	----------------------------------------------------------- */	 
			else
			{
				ProcessDumpBin();

				/*	Print symbols table, if necessary.
				 *	---------------------------------- */	
				if (list != NULL)
					print_symbols_table();

				break;
			}
		}

		/*	Check if we was able to grab all the source line.
		 *	************************************************* */

		p_text_1		= p_text;
		eol_found	= 0;

		/*	Search for End Of Line.
		 *	----------------------- */
		while (*p_text_1 != '\0')
		{
			if (*(p_text_1++) == '\n')
				eol_found	= 1;
		}

		if (!eol_found)
		{
			int	c;

			/*	Bypass remaining of source line.
			 *	Notes: This is necessary to get rid of "codeline[]" phasing error.
			 *	------------------------------------------------------------------ */
			do
				c	= fgetc(in_fp[file_level]);
			while ((c != '\n') && (c != EOF));

			if (asm_pass == 1)
			{
				if (list != NULL)
					fprintf(list, "*** Error %d in \"%s\": Line too long!\n", EC_SLTL, in_fn[file_level]);

				fprintf(stderr, "*** Error %d in \"%s\" @%d: Line too long!\n", EC_SLTL, in_fn[file_level], codeline[file_level]);
			}
		}

		/*	Destroy New Line character, if necessary.
		 *	----------------------------------------- */
		if (eol_found)			
			p_text[strlen(p_text) - 1] = '\0';

		/*	If source line contain something, process it.
		 *	--------------------------------------------- */	
		if (strlen(p_text) > 1)
		{
			EmitBin = src_line_parser(p_text);

			if (util_is_cs_enable() == 0)
				type = LIST_ONLY;
		}

		/*	- When "END" directive was processed, don't print list
		 *	  immediatly.  This will be done later...
	 	 *	------------------------------------------------------ */	 
		if (EmitBin != PROCESSED_END)
			PrintList(p_text);

//		if ((asm_pass == 1) && (util_is_cs_enable() == 1))
		if (util_is_cs_enable() == 1)
			DumpBin();

		/*	If an "END" directive was executed...
		 *	------------------------------------- */	
		if (EmitBin == PROCESSED_END)
		{
			/*	If the "END" directive was found inside an include file...
			 *	---------------------------------------------------------- */
			if ((file_level > 0) && (asm_pass == 1))
			{
				if (list != NULL)
					fprintf(list, "*** Warning %d in \"%s\": 'END' directive found inside an include file!\n", WC_EDFIIF, in_fn[file_level]);

				fprintf(stderr, "*** Warning %d in \"%s\" @%d: 'END' directive found inside an include file!\n", WC_EDFIIF, in_fn[file_level], codeline[file_level]);
			}

			PrintList(p_text);
			ProcessDumpBin();

			/*	Print symbols table, if necessary.
			 *	---------------------------------- */	
			if (list != NULL)
				print_symbols_table();

			/*	If the "END" directive was found inside an include file...
			 *	---------------------------------------------------------- */
			if (file_level > 0)
			{
				int	i;

				/*	- Close all openned included files, and free associated
				 *	  resources.
				 *	------------------------------------------------------- */	
				for (i = file_level; i > 0; i--)
				{
					/*	Close input file handle.
					 *	------------------------ */
					fclose(in_fp[i]);
					in_fp[i]	= NULL;

					/*	Free memory allocated for the input file name.
					 *	---------------------------------------------- */
					free(in_fn[i]);
					in_fn[i]	= NULL;
				}

				file_level	= 0;		/*	We are now at level 0. */
			}

			break;						/*	Terminate assembly process. */
		}

		data_size	=
	  	b1				=
	  	b2				=
	  	b3				=
	  	b4				= 0;
	}

	free(p_text);		/*	Free allocated memory. */
}


/*	*************************************************************************
 *	Function name:		display_help
 *	Description:		Display Help.
 *	Author(s):			Jay Cotton, Claude Sylvain
 *	Created:				2007
 *	Last modified:		26 March 2011
 *	Parameters:			void
 *	Returns:				void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void display_help(void)
{
	printf("Usage: %s <source file> [<options>]\n", name_pgm);
	printf("Options:\n");
	printf("  -h           : Display Help.\n");
	printf("  -I<dir>      : Add directory to the include file search path.\n");
	printf("  -l<filename> : Generate listing file.\n");
	printf("  -o<filename> : Define output file (optionnal).\n");
	printf("  -v           : Display version.\n");
}


/*	*************************************************************************
 *	Function name:		display_version
 *	Description:		Display Version.
 *	Author(s):			Claude Sylvain
 *	Created:				3 January 2011
 *	Last modified:
 *	Parameters:			void
 *	Returns:				void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void display_version(void)
{
	printf(	"%s version %d.%d.%d\n", name_pgm, pgm_version_v,
		  		pgm_version_sv, pgm_version_rn);
}


/*	*************************************************************************
 *	Function name:	AddLabel
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	10 December 2011
 *
 *	Parameters:		char *text:
 *							...
 *
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void AddLabel(char *text)
{
	char   label[LABEL_SIZE_MAX];
	int    i			= 0;
	SYMBOL *Local	= Symbols;
	int    phantom	= 0;


	if (asm_pass == 1)				return;
	if (isspace((int) *text))		return;
	if (util_is_cs_enable() == 0)	return;

	if (*text == '&')
		label[i++] = *text++;

	if (*text == '%')
	{
		label[i++] = *text++;
		phantom++;
	}

	while (isalnum((int) *text) || (*text == '_'))
		label[i++] = *(text++);

	label[i] = '\0';

	if (FindLabel(label))
	{
		if (!phantom)
		{
			if (list != NULL)
			{
				/*	- Notes: Since this error is printed on assembler pass #1 only,
				 *	  it is printed at beginning of the listing file.
				 *	  So, we have to tell where is the error, by adding the
				 *	  line number to the print out.
				 *	*/
				fprintf(list, "*** Error %d in \"%s\" @%d: Duplicate Label (%s)!\n", EC_DL, in_fn[file_level], codeline[file_level], label);
			}

			fprintf(stderr, "*** Error %d in \"%s\" @%d: Duplicate Label (%s)!\n", EC_DL, in_fn[file_level], codeline[file_level], label);
		}

		return;
	}


	/* Now add it to the list.
	 *	*********************** */

	/* find end of list.
	 * ----------------- */
	while (Local->next)
		Local = (SYMBOL *) Local->next;

	strcpy(Local->Symbol_Name, label);
	Local->Symbol_Value	= target.pc;
	Local->next				= (SYMBOL *) calloc(1, sizeof(SYMBOL));
}


/*	*************************************************************************
 *	Function name:	DumpBin
 *	Description:	Dump Binary.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	10 December 2011
 *	Parameters:		void
 *	Returns:			void
 *
 *	Globals:			int type
 *						int data_size
 *						int target.pc
 *
 *	Notes:
 *	************************************************************************* */

static void DumpBin(void)
{
	STACK	*DLStack;
	STACK	*LStack	= ByteWordStack;


	switch (type)
	{
		case TEXT:
			switch (data_size)
			{
				case 1:
#if 0
					Image[Target.count++] = b1;
#endif
					Image[target.pc] = b1;
					update_pc(1);
					break;

				case 2:
#if 0
					Image[Target.count++] = b1;
					Image[Target.count++] = b2;
#endif
					Image[target.pc] = b1;
					update_pc(1);
					Image[target.pc] = b2;
					update_pc(1);
					break;

				case 3:
#if 0
					Image[Target.count++] = b1;
					Image[Target.count++] = b2;
					Image[Target.count++] = b3;
#endif
					Image[target.pc] = b1;
					update_pc(1);
					Image[target.pc] = b2;
					update_pc(1);
					Image[target.pc] = b3;
					update_pc(1);
					break;

				case 4:
#if 0
					Image[Target.count++] = b1;
					Image[Target.count++] = b2;
					Image[Target.count++] = b3;
					Image[Target.count++] = b4;
#endif
					Image[target.pc] = b1;
					update_pc(1);
					Image[target.pc] = b2;
					update_pc(1);
					Image[target.pc] = b3;
					update_pc(1);
					Image[target.pc] = b4;
					update_pc(1);
					break;

				default:
					break;	
			}

//			Target.count	= addr;		/*	Sync. */
			break;

		case LIST_DS:
		{	
			int	i;

#if 0
			/*	Fill space.
			 *	Notes: 0x00 is also known as "NOP" opcode.
			 *	------------------------------------------ */
			for (i = 0; i < data_size; i++)
				Image[Target.count++] = 0x00;
#endif
			/*	Fill space.
			 *	Notes: 0x00 is also known as "NOP" opcode.
			 *	------------------------------------------ */
			for (i = 0; i < data_size; i++)
			{
				Image[target.pc] = 0x00;
				update_pc(1);
			}

//			Target.count	= addr;		/*	Sync. */
			break;
		}

		case LIST_BYTES:
		case LIST_WORDS:
		case LIST_STRINGS:
			/* Don't count last byte/word.
			 *	--------------------------- */
			while (LStack->next)
			{
				if ((type == LIST_BYTES) || (type == LIST_STRINGS))
				{
					if (asm_pass == 1)
						Image[target.pc]	= LStack->word & 0xFF;

					update_pc(1);
//					Target.count	= addr;
				}
				else
				{
					if (asm_pass == 1)
					{
						Image[target.pc]	= LStack->word & 0x00FF;
						update_pc(1);
						Image[target.pc]	= (LStack->word & 0xFF00) >> 8;
						update_pc(1);
					}

//					Target.count	= addr;
				}

				LStack = (STACK *) LStack->next;
			}

			/*	Always keep the stack root.
			 *	--------------------------- */
			LStack	= ByteWordStack;
			DLStack	= LStack = (STACK *) LStack->next;

			if (LStack)
			{
				do
				{
					LStack	= (STACK *) LStack->next;
					free(DLStack);
					DLStack	= LStack;
				} while (LStack);

				LStack			= ByteWordStack;
				LStack->next	= NULL;
			}
			break;

		default:
			break;	
	}
}


/*	*************************************************************************
 *	Function name:	asm_pass1
 *	Description:	Assembler Pass #1.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	10 December 2011
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void asm_pass1(void)
{
	int	i;

	for (i = 0; i < FILES_LEVEL_MAX; i++)
		codeline[i]	= 0;

//	Target.count		= 0;
	target.addr			= 0;
//	addr					= 0;
	target.pc			= 0x0000;
	target.mem_size	= 0;
	target.pc_or		= 0;					/*	No PC Over Range. */
	type					= LIST_ONLY;
	asm_pass 			= 0;

	memset(Image, 0, sizeof (Image));
	do_asm();
}


/*	*************************************************************************
 *	Function name:	asm_pass2
 *	Description:	Assembler Pass #2.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	10 December 2011
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void asm_pass2(void)
{
	int	i;

	for (i = 0; i < FILES_LEVEL_MAX; i++)
		codeline[i]	= 0;

//	Target.count		= 0;
	target.addr			= 0;
//	addr					= 0;
	target.pc			= 0x0000;
	target.mem_size	= 0;
	target.pc_or		= 0;					/*	No PC Over Range. */
	type					= LIST_ONLY;
	asm_pass				= 1;

	memset(Image, 0, sizeof (Image));
	RewindFiles();
	do_asm();
}


/*	*************************************************************************
 *	Function name:	init
 *	Description:	Initialize module.
 *	Author(s):		Claude Sylvain
 *	Created:			28 December 2010
 *	Last modified:	31 December 2010
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void init(void)
{
	int	i;

	/*	Init. "in_fn" array.
	 *	-------------------- */	
	for (i = 0; i < (sizeof (in_fn) / sizeof (char *)); i++)
		in_fn[i] = NULL;
}


/*	*************************************************************************
 *	Function name:	process_option_i
 *	Description:	Process Option "-I".
 *	Author(s):		Claude Sylvain
 *	Created:			31 December 2010
 *	Last modified:	27 March 2011
 *
 *	Parameters:		char *text:
 *							Pointer to text that hold "-I" option.
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: Operation successfull.
 *
 *	Globals:
 *
 *	Notes:			- If a path is specified, path delimitor is not mandatory
 *						  at end of path.
 *	************************************************************************* */

static int process_option_i(char *text)
{
	int		rv					= -1;
	int		have_to_add_ps	= 0;		/*	Have To Add Path Seperator (default == NO). */
	size_t	string_len;

	struct option_i_t	*p_option_i	= &option_i;		/*	Base structure. */


	text			+= 2;								/*	Bypass "-I". */
	string_len	= strlen(text);

	/*	- If last character is not a path seperator, remember we
	 *	  have to add it, and set string length 1 byte larger
	 *	  to make space to that added space seperator.
	 *	-------------------------------------------------------- */
	if (text[string_len - 1] != PATH_SEPARATOR_CHAR)
	{
		have_to_add_ps	= 1;
		string_len++;
	}

	/*	Search for the next available "option_i" structure.
	 *	--------------------------------------------------- */
	while (p_option_i->next != NULL)
		p_option_i	= p_option_i->next;

	if ((p_option_i->path = (char *) malloc(string_len + 1)) != NULL)
	{
		strcpy(p_option_i->path, text);		/*	Save path. */

		/*	Add path seperator, if necessary.
		 *	--------------------------------- */
		if (have_to_add_ps != 0)
			strcat(p_option_i->path, PATH_SEPARATOR_STR);

		/*	Allocate memory for the next "option_i" structure.
		 *	*/	
		p_option_i->next	=
		  	(struct option_i_t *) malloc(sizeof (struct option_i_t));

		if (p_option_i->next != NULL)
		{
			/*	Init. newly created structure "nexẗ" member to NULL.
			 *	---------------------------------------------------- */
			p_option_i			= p_option_i->next;
			p_option_i->path	= NULL;
			p_option_i->next	= NULL;

			rv	= 0;
		}
		else
		{
			fprintf(stderr, "*** Error %d: Can't allocate memory!\n", EC_MAE);
			free(p_option_i->path);		/*	Make structure available. */
		}
	}
	else
	{
		fprintf(stderr, "*** Error %d: Can't allocate memory!\n", EC_MAE);
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	process_option_l
 *	Description:	Process Option "-l".
 *	Author(s):		Claude Sylvain
 *	Created:			26 March 2011
 *	Last modified:	26 November 2011
 *
 *	Parameters:		char *text:
 *							Pointer to text that hold "-l" option.
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: Operation successfull.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int process_option_l(char *text)
{
	const char	*msg_cam	=	"*** Error %d: Can't allocate memory!\n";

	int		rv				= -1;		/*	Return Value (default == -1). */
	size_t	string_len;


	/*	If an "-l" option already processed, alert user, and exit with error.
	 *	--------------------------------------------------------------------- */	
	if (list_file != NULL)
	{
		fprintf(stderr, "*** Warning %d: Extra \"-l\" option specified (\"%s\")!\n", WC_ELOS, text);
		fprintf(stderr, "                This option will be ignored!\n");
		return (-1);
	}

	text			+= 2;					/*	Bypass "-l". */
	string_len	= strlen(text);

	/*	- If a listing file name follow "-l" option, take it as the
	 *	  listing file name.
 	 *	----------------------------------------------------------- */
	if (string_len > 0)
	{
		int		i;
		int		dot_pos		= 0;

		/*	Search for the beginning of an extension.
		 *	----------------------------------------- */
		for (i = 0; i < (string_len - 1); i++)
		{
			if (	(text[i] == '.') && (text[i + 1] != '.') &&
					(text[i + 1] != PATH_SEPARATOR_CHAR))
			{
				dot_pos	= i;
			}
		}

		/*	- If a file extension is present, init. listing file name
		 *	  without adding an extension.
		 *	--------------------------------------------------------- */	 
		if (dot_pos > 0)
		{
			/*	Allocate memory for the listing file name.
			 *	*/
			list_file	= (char *) malloc(string_len + 1);

			/*	Built list file name, if possible.
			 *	---------------------------------- */	
			if (list_file != NULL)
			{
				strcpy(list_file, text);
				rv	= 0;		/*	Success! */
			}
			else
			{
				fprintf(stderr, msg_cam, EC_CAM);
			}
		}
		/*	- No file extension is present, init. listing file name,
		 *	  and add an extension to it.
		 *	-------------------------------------------------------- */	 
		else
		{
			/*	Allocate memory for the listing file name.
			 *	*/
			list_file	= (char *) malloc(string_len + 4 + 1);

			/*	Built list file name, if possible.
			 *	---------------------------------- */	
			if (list_file != NULL)
			{
				strcpy(list_file, text);
				strcat(list_file, ".lst");
				rv	= 0;		/*	Success! */
			}
			else
			{
				fprintf(stderr, msg_cam, EC_CAM);
			}
		}
	}
	/*	- No listing file name follow "-l" option, use the input
	 *	  file base name as the listing base file name.
 	 *	-------------------------------------------------------- */
	else
	{
		/*	If input file base name exist, use it as listing base file name.
		 *	---------------------------------------------------------------- */	
		if (strlen(fn_base) > 0)
		{
			/*	Allocate memory for the listing file name.
			 *	*/
			list_file	= (char *) malloc(strlen(fn_base) + 4 + 1);

			/*	Built list file name, if possible.
			 *	---------------------------------- */	
			if (list_file != NULL)
			{
				strcpy(list_file, fn_base);
				strcat(list_file, ".lst");
				rv	= 0;		/*	Success! */
			}
			else
			{
				fprintf(stderr, msg_cam, EC_CAM);
			}
		}
		/*	No input file base name exist, use default listing file name.
		 *	------------------------------------------------------------- */	
		else
		{
			char	*default_lst_file_name	= "a.lst";

			fprintf(stderr, "*** Warning %d: No input file specified at time \"-l\" option was processed!\n", WC_NIFSATLOP);
			fprintf(stderr, "                Using default name (\"a.lst\")!\n");

			/*	Allocate memory for the listing file name.
			 *	*/
			list_file	= (char *) malloc(strlen(default_lst_file_name) + 1);

			/*	Built list file name, if possible.
			 *	---------------------------------- */	
			if (list_file != NULL)
			{
				strcpy(list_file, default_lst_file_name);
				rv	= 0;		/*	Success! */
			}
			else
			{
				fprintf(stderr, msg_cam, EC_CAM);
			}
		}
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	process_option_o
 *	Description:	Process Option "-o".
 *	Author(s):		Claude Sylvain
 *	Created:			26 March 2011
 *	Last modified:	26 November 2011
 *
 *	Parameters:		char *text:
 *							Pointer to text that hold "-o" option.
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: Operation successfull.
 *
 *	Globals:			char	*bin_file
 *
 *	Notes:
 *	************************************************************************* */

static int process_option_o(char *text)
{
	const char	*msg_cam	=	"*** Error %d: Can't allocate memory!\n";

	int		rv				= -1;		/*	Return Value (default == -1). */
	size_t	string_len;


	/*	If an "-o" option already processed, alert user, and exit with error.
	 *	--------------------------------------------------------------------- */	
	if (bin_file != NULL)
	{
		fprintf(stderr, "*** Warning %d: Extra \"-o\" option specified (\"%s\")!\n", WC_EOOS, text);
		fprintf(stderr, "                This option will be ignored!\n");
		return (-1);
	}

	text			+= 2;					/*	Bypass "-o". */
	string_len	= strlen(text);

	/*	- If a file name follow "-o" option, take it as the
	 *	  output file name.
 	 *	--------------------------------------------------- */
	if (string_len > 0)
	{
		int		i;
		int		dot_pos		= 0;

		/*	Search for the beginning of an extension.
		 *	----------------------------------------- */
		for (i = 0; i < (string_len - 1); i++)
		{
			if (	(text[i] == '.') && (text[i + 1] != '.') &&
					(text[i + 1] != PATH_SEPARATOR_CHAR))
			{
				dot_pos	= i;
			}
		}

		/*	- If a file extension is present, init. output file name
		 *	  without adding an extension.
		 *	-------------------------------------------------------- */
		if (dot_pos > 0)
		{
			/*	Allocate memory for the binary file name.
			 *	*/
			bin_file	= (char *) malloc(string_len + 1);

			/*	Built binary file name, if possible.
			 *	------------------------------------ */	
			if (bin_file != NULL)
			{
				strcpy(bin_file, text);
				rv	= 0;		/*	Success! */
			}
			else
			{
				fprintf(stderr, msg_cam, EC_CAM);
			}
		}
		/*	- No file extension is present, init. binary file name,
		 *	  and add an extension to it.
		 *	------------------------------------------------------- */	 
		else
		{
			/*	Allocate memory for the binary file name.
			 *	*/
			bin_file	= (char *) malloc(string_len + 4 + 1);

			/*	Built binary file name, if possible.
			 *	------------------------------------ */	
			if (bin_file != NULL)
			{
				strcpy(bin_file, text);
				strcat(bin_file, ".bin");
				rv	= 0;		/*	Success! */
			}
			else
			{
				fprintf(stderr, msg_cam, EC_CAM);
			}
		}
	}
	/*	- No binary file name follow "-o" option, use the input
	 *	  file base name as the binary base file name.
 	 *	------------------------------------------------------- */
	else
	{
		/*	If input file base name exist, use it as binary base file name.
		 *	--------------------------------------------------------------- */	
		if (strlen(fn_base) > 0)
		{
			/*	Allocate memory for the binary file name.
			 *	*/
			bin_file	= (char *) malloc(strlen(fn_base) + 4 + 1);

			/*	Built binary file name, if possible.
			 *	------------------------------------ */	
			if (bin_file != NULL)
			{
				strcpy(bin_file, fn_base);
				strcat(bin_file, ".bin");
				rv	= 0;		/*	Success! */
			}
			else
			{
				fprintf(stderr, msg_cam, EC_CAM);
			}
		}
		/*	No input file base name exist, use default binary file name.
		 *	------------------------------------------------------------ */	
		else
		{
			char	*default_bin_file_name	= "a.bin";

			fprintf(stderr, "*** Warning %d: No input file specified at time \"-o\" option was processed!\n", WC_NIFSATOOP);
			fprintf(stderr, "                Using default name (\"a.bin\")!\n");

			/*	Allocate memory for the binary file name.
			 *	*/
			bin_file	= (char *) malloc(strlen(default_bin_file_name) + 1);

			/*	Built binary file name, if possible.
			 *	------------------------------------ */	
			if (bin_file != NULL)
			{
				strcpy(bin_file, default_bin_file_name);
				rv	= 0;		/*	Success! */
			}
			else
			{
				fprintf(stderr, msg_cam, EC_CAM);
			}
		}
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	check_set_output_fn
 *	Description:	Check Output File Name, and Set it if necessary.
 *	Author(s):		Claude Sylvain
 *	Created:			26 March 2011
 *	Last modified:
 *	Parameters:		void
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: Operation successfull.
 *
 *	Globals:			char	*bin_file
 *
 *	Notes:			An error upon calling this function is a fatal error.
 *	************************************************************************* */

static int check_set_output_fn(void)
{
	int	rv	= -1;		/*	Return Value (default == -1). */


	/*	If binary file name already set, do nothing.
	 *	-------------------------------------------- */	
	if (bin_file != NULL)
		return (0);

	/*	If input file base name exist, use it as binary base file name.
	 *	--------------------------------------------------------------- */	
	if (strlen(fn_base) > 0)
	{
		/*	Allocate memory for the binary file name.
		 *	*/
		bin_file	= (char *) malloc(strlen(fn_base) + 4 + 1);

		/*	Built binary file name, if possible.
		 *	------------------------------------ */	
		if (bin_file != NULL)
		{
			strcpy(bin_file, fn_base);
			strcat(bin_file, ".bin");
			rv	= 0;		/*	Success! */
		}
		else
		{
			fprintf(stderr, "*** Error %d: Can't allocate memory!\n", EC_CAM);
		}
	}
	/*	- No input file specified, and no "-o" option specified.
	 *	- This is a fatal error, and must be handled carefully
	 *	  by the caller (main())!
 	 *	-------------------------------------------------------- */	 
	else
	{
		fprintf(	stderr,
			  		"*** Error %d: No input file and no \"-o\" option specified!\n",
				  	EC_NIFSNOOS);
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	clean_up
 *	Description:	Clean Up before exiting.
 *	Author(s):		Claude Sylvain
 *	Created:			27 December 2010
 *	Last modified:	26 March 2011
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void clean_up(void)
{
	struct option_i_t	*p_option_i_cur;
	struct option_i_t	*p_option_i_next;

	free(Symbols);
	free(ByteWordStack);

	/*	- Since "in_fn[0]" is no more used, we do not have to init.
	 *	  it to NULL.
	 *	*/		  
	free(in_fn[0]);


	/*	- Free memory allocated for "-I" option structures.
	 *	- Notes: First structure is static, and only its "path" member
 	 *	  need to be freed.
	 *	************************************************************** */

	p_option_i_next	= option_i.next;	/*	First structure to look for. */

	/*	Notes: "NULL" if no option "-I" found.
	 *	*/	
	free(option_i.path);

	/*	Free memory associated with option "-I" dynamic structures.
	 *	----------------------------------------------------------- */	
	while (p_option_i_next != NULL)
	{
		p_option_i_cur		= p_option_i_next;
		p_option_i_next	= p_option_i_cur->next;
		free(p_option_i_cur->path);
		free(p_option_i_cur);
	}

	/*	Notes: "NULL" if no option "-l" found.
	 *	*/	
	free(list_file);

	free(bin_file);
}


/*	*************************************************************************
 *	Function name:	process_input_file
 *	Description:	Process Input File.
 *	Author(s):		Claude Sylvain
 *	Created:			31 December 2010
 *	Last modified:	27 March 2011
 *
 *	Parameters:		char *text:
 *							Pointer to text that hold input file.
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: Operation successfull.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int process_input_file(char *text)
{
	/*	Tell if Input File was already Processed.
	 *	*/
	static int	if_processed	= 0;

	int		i;
	int		dot_pos	= 0;					/*	Dot Position. */
	size_t	ln			= strlen(text);
	size_t	in_fn_len;


	/*	If input file already processed, exit with error.
	 *	------------------------------------------------- */	
	if (if_processed)
	{
		fprintf(stderr, "*** Error %d: Extra Input file specified (\"%s\")!\n", EC_EIFS, text);
		fprintf(stderr, "              This file will be ignored!\n");
		return (-1);
	}

	if_processed	= 1;					/*	Will be Processed. */

	/*	Search for the beginning of an extension.
	 *	----------------------------------------- */
	for (i = 0; i < (ln - 1); i++)
	{
		if (	(text[i] == '.') && (text[i + 1] != '.') &&
			  	(text[i + 1] != PATH_SEPARATOR_CHAR))
		{
			dot_pos	= i;
		}
	}

	/*	If an extension was found...
	 *	---------------------------- */	
	if (dot_pos > 0)
	{
		/*	Check for input file name maximum lenght.
		 *	----------------------------------------- */
		if (dot_pos <= (sizeof (fn_base) - 4))
		{
			in_fn_len	= ln + 1;
			strncpy(fn_base, text, dot_pos);
			fn_base[dot_pos]	= '\0';
		}
		else
		{
			fprintf(stderr, "*** Error %d: Input file name too long (\"%s\")!\n", EC_IFNTL, text);
			return (-1);
		}
	}
	/*	- If input file name have no extension, take that name
	 *	  as base file name.
	 *	------------------------------------------------------ */	  
	else
	{
		/*	Check for input file name maximum lenght.
		 *	----------------------------------------- */
		if (ln <= (sizeof (fn_base) - 1))
		{
			in_fn_len	= ln + 4 + 1;
			strcpy(fn_base, text);
		}
		else
		{
			fprintf(stderr, "*** Error %d: Input file name too long (\"%s\")!\n", EC_IFNTL, text);
			return (-1);
		}
	}

	/*	Allocate memory for the input file name.
	 *	*/	
	in_fn[0]	= (char *) malloc(in_fn_len);

	/*	Check for memory allocation error.
	 *	--------------------------------- */	
	if (in_fn[0] == NULL)
	{
		fprintf(stderr, "*** Error %d: Memory allocation error!\n", EC_MAE);
		return (-1);
	}

	/*	- If an extension was found, set input file name as the
	 *	  input file name specified in the command line.
	 *	- Otherwise, set the input file name as the input
	 *	  file name specified in the command line + ".asm"
	 *	  extension.	  
	 *	------------------------------------------------------- */	  
	if (dot_pos > 0)
		strcpy(in_fn[0], text);		/*	File name with extension. */
	else
	{
		strcpy(in_fn[0], text);		/*	File name (only). */
		strcat(in_fn[0], ".asm");	/*	Add default extension. */
	}

	return (0);
}


/*	*************************************************************************
 *	Function name:	cmd_line_parser
 *	Description:	Command Line Parser.
 *	Author(s):		Claude Sylvain
 *	Created:			31 December 2010
 *	Last modified:	26 March 2011
 *
 *	Parameters:		int argv:
 *							...
 *
 *						char *argc[]:
 *							...
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: User just one help.  Do not assemble.
 *							1	: Ready to assemble.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int cmd_line_parser(int argc, char *argv[])
{
	int	rv		= -1;
	int	pgm_par_cnt;		/*	Program Parameter Count. */


	/*	Need at least 2 arguments.
	 *	-------------------------- */	
	if (argc > 1)
	{
		pgm_par_cnt		= argc - 1;
		argv++;								/*	Select first program parameter. */

		/*	Process program parameters.
		 *	--------------------------- */	
		while (pgm_par_cnt > 0)
		{
			switch (**argv)
			{
				case '-':
					switch (*(*argv + 1))
					{
						/*	"-I" option.
						 *	------------ */
						case 'I':
							process_option_i(*argv);
							break;

						/*	"-l" option.
						 *	------------ */
						case 'l':
							process_option_l(*argv);
							break;

						/*	"-o" option.
						 *	------------ */
						case 'o':
							process_option_o(*argv);
							break;

						/*	- On "-h" option of unknown option, display
						 *	  help and exit.
						 *	------------------------------------------- */	 
						case 'v':
							display_version();
							rv				= 0;		/*	Just display.  Do not assemble. */
							pgm_par_cnt	= 0;		/*	Force Exit. */
							break;

						/*	- On "-h" option of unknown option, display
						 *	  help and exit.
						 *	------------------------------------------- */	 
						case 'h':
						default:
							display_help();
							rv				= 0;		/*	Just display.  Do not assemble. */
							pgm_par_cnt	= 0;		/*	Force Exit. */
							break;

					}

					break;

				default:
					if (process_input_file(*argv) != 1)
						rv	= 1;

					break;
			}

			pgm_par_cnt--;
			argv++;
		}	
	}
	/*	Just 1 argument.  Need at least 2 arguments.
	 *	Display help.	
	 *	-------------------------------------------- */	
	else
	{
		display_help();
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	main
 *	Description:	Main function.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	28 December 2010
 *
 *	Parameters:		int argv:
 *							...
 *
 *						char *argc[]:
 *							...
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

int main(int argc, char *argv[])
{
	init();						/*	Initialize module. */
	if_true[0]		= 1;		/*	If nesting base level (always TRUE). */ 

	Symbols			= (SYMBOL *) calloc(1, sizeof(SYMBOL));
	ByteWordStack	= (STACK *) calloc(1, sizeof(STACK));

	/*	Check for memory allocation error.
	 *	---------------------------------- */	
	if ((Symbols == NULL) || (ByteWordStack == NULL))
	{
		fprintf(stderr, "*** Error %d: Can't allocate memory!\n", EC_CAM);
		clean_up();

		return (-1);
	}

	memset(Image, 0, sizeof (Image));

	/*	Need at least one parameter.
	 *	---------------------------- */
	if (cmd_line_parser(argc, argv) > 0) 
	{
		/*	Check/Set Output File Name; Open Files.
		 *	If there was an error, abort operation.
	 	 *	--------------------------------------- */	 
		if ((check_set_output_fn() == -1) || (OpenFiles() == -1))
		{
			CloseFiles();
			clean_up();
			return (-1);
		}

		asm_pass1();
		asm_pass2();
		CloseFiles();
	}

	clean_up();					/*	Clean Up module. */

	return (0);
}



