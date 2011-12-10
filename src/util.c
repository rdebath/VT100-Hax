/*	*************************************************************************
 *	Module Name:	util.c
 *	Description:	Utilities.
 *	Copyright(c):	See below...
 *	Author(s):		Claude Sylvain
 *	Created:			23 December 2010
 *	Last modified:	10 December 2011
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
#include "err_code.h"
#include "main.h"


/*	*************************************************************************
 *	                           FUNCTIONS DEFINITION
 *	************************************************************************* */


/*	*************************************************************************
 *	Function name:	islabelchar
 *	Description:	Tell if character can be a label character.
 *	Author(s):		Claude Sylvain
 *	Created:			8 January 2011
 *	Last modified:
 *
 *	Parameters:		int c:
 *							The character to test.
 *
 *	Returns:			int:
 *							0		: Not a label character.
 *							!= 0	: Is a label character.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

int islabelchar(int c)
{
	return ((isalnum(c) != 0) || (c == '_'));
}


/*	*************************************************************************
 *	Function name:	check_evor
 *
 *	Description:	- Check for Expression Value Over Range, and display/print
 *						  an error message if an over range is detected.
 *
 *	Author(s):		Claude Sylvain
 *	Created:			4 December 2011
 *	Last modified:
 *
 *	Parameters:		int value:
 *							Expression Value to check.
 *
 *						int limit:
 *							Expression value Limit.
 *
 *	Returns:			int:
 *							-1	: Over Range.
 *							0	: Good Range.
 *
 *	Globals:			None.
 *	Notes:
 *	************************************************************************* */

int check_evor(int value, int limit)
{
	int	rv	= 0;

	if ((value > limit) && (asm_pass == 1))
	{
		if (list != NULL)
			fprintf(	list, "*** Error %d in \"%s\": Expression value over range (%d)!\n",
				  		EC_EVOR, in_fn[file_level], value);

		fprintf(	stderr, "*** Error %d in \"%s\" @%d: Expression value over range (%d)!\n",
			  		EC_EVOR, in_fn[file_level], codeline[file_level], value);

		rv	= -1;
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	check_oor
 *
 *	Description:	- Check for Operand Over Range, and display/print
 *						  an error message if an over range is detected.
 *
 *	Author(s):		Claude Sylvain
 *	Created:			24 December 2010
 *	Last modified:	28 December 2010
 *
 *	Parameters:		int value:
 *							Value to check.
 *
 *						int limit:
 *							Operand Limit.
 *
 *	Returns:			int:
 *							-1	: Over Range.
 *							0	: Good Range.
 *
 *	Globals:			None.
 *	Notes:
 *	************************************************************************* */

int check_oor(int value, int limit)
{
	int	rv	= 0;

	if ((value > limit) && (asm_pass == 1))
	{
		if (list != NULL)
			fprintf(list, "*** Error %d in \"%s\": Operand over range (%d)!\n", EC_OOR, in_fn[file_level], value);

		fprintf(stderr, "*** Error %d in \"%s\" @%d: Operand over range (%d)!\n", EC_OOR, in_fn[file_level], codeline[file_level], value);

		rv	= -1;
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	FindLabel
 *	Description:	Label processing.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	26 November 2011
 *
 *	Parameters:		char *text:
 *							"text" that possibly contain label.
 *
 *	Returns:			SYMBOL *:
 *							Pointer to the structure that hold symbol.
 *
 *	Globals:
 *	Notes:			Linked list, linear search, string compare real dumb.
 *	************************************************************************* */

SYMBOL *FindLabel(char *text)
{
	char		tmp[SYMBOL_SIZE_MAX];
	SYMBOL	*Local	= Symbols;
	int		i			= 0;


	while (isspace((int) *text))
		text++;

	if (*text == '&')	tmp[i++] = *text++;
	if (*text == '%')	tmp[i++] = *text++;

	while (islabelchar((int) *text) != 0)
	{
		tmp[i] = *text++;

		/*	Check for buffer overflow.
		 *	-------------------------- */	
		if (++i >= sizeof (tmp))
		{
			--i;

			if (asm_pass == 1)
			{
				if (list != NULL)
					fprintf(list, "*** Error %d in \"%s\": \"FindLabel\" buffer overflow!\n", EC_FLBOF, in_fn[file_level]);

				fprintf(stderr, "*** Error %d in \"%s\" @%d: \"FindLabel\" buffer overflow!\n", EC_FLBOF, in_fn[file_level], codeline[file_level]);
			}

			break;
		}
	}

	tmp[i] = '\0';

	while (Local->next)
  	{
		if (!strcmp(Local->Symbol_Name, tmp))
			return (Local);

		Local = (SYMBOL *) Local->next;
	}

	return (NULL);
}


/*	*************************************************************************
 *	Function name:	process_label
 *	Description:	Process Label.
 *	Author(s):		Claude Sylvain
 *	Created:			28 December 2010
 *	Last modified:	10 December 2011
 *
 *	Parameters:		char *label:
 *							Point to a string that hold label.
 *
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

void process_label(char *label)
{
	SYMBOL	*Local = FindLabel(label);

	/* Record the address of the label.
	 * -------------------------------- */
	if (Local)
		Local->Symbol_Value = target.pc;
}


/*	*************************************************************************
 *	Function name:	util_get_number_base_inc
 *	Description:	Get Number Base Increment.
 *	Author(s):		Claude Sylvain
 *	Created:			23 December 2010
 *	Last modified:	26 November 2011
 *
 *	Parameters:		char *text:
 *							Text that contain number.
 *
 *	Returns:			int:
 *							Base Increment (2, 8, 10, 16)
 *							or
 *							(-1) if an error as occured.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

int util_get_number_base_inc(char *text)
{
	int	rv			= -1;
	int	c_num		= 0;
	int	c;


	/*	Search for the last number character.
	 *	------------------------------------- */	
	while ((*text != '\0') && (isalnum((int) *text) != 0))
	{
		text++;
		c_num++;			/*	Keep track of number of characters. */
	}

	/*	Process only if a number exist.
	 *	------------------------------- */	
	if (c_num > 0)
	{
		--text;								/*	Pos. to last character. */
		c	= toupper((int) *text);		/*	Working copy of last character. */

		/*	- If last character is a decimal digit, this mean that
		 *	  the number is a decimal number with no numerical base
	 	 *	  qualifier.
		 * ------------------------------------------------------- */	 
		if (isdigit(c))
		{
			rv	= 10;
		}
		/*	- This is not a number expressed in the default
		 *	  decimal base.
	 	 *	- The last character must be a numerical base
		 *	  specifier (B, Q, D, H).
		 *	----------------------------------------------- */
		else
		{
			/*	- If hold at least 1 digit for the number,
			 *	  go further more.
			 *	------------------------------------------ */	
			if (c_num > 1)
			{
				/*	- Replace the numerical base specifier by a delimitor.
				 *	- Notes: This is necessary to avoid the caller
				 *	  to have to process that character.
				 *	*/
				*text	= '\0';

				/*	Set increment, depending on the numerical specifier.
				 *	---------------------------------------------------- */	
				switch(c)
				{
					case 'B':	rv	= 2;	break;	/*	Binary base. */
					case 'Q':	rv	= 8;	break;	/*	Octal base. */
					case 'D':	rv	= 10;	break;	/*	Decimal base. */
					case 'H':	rv	= 16;	break;	/*	Hexadecimal base. */
					default:					break;	
				}
			}
		}
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	AdvanceTo
 *	Description:
 *	Author(s):		Claude Sylvain
 *	Created:			2007
 *	Last modified:	27 December 2010
 *
 *	Parameters:		char *text:
 *							...
 *
 * 					char x:
 *							...
 *
 *	Returns:			char *:
 *							...
 *
 *	Globals:
 *
 *	Notes:
 *	- For DB, and DW, there may be an unbounded list of hex, dec, and octal
 *	  bytes/words, and quoted strings.
 *	************************************************************************* */

char *AdvanceTo(char *text, char x)
{
	while (*text != '\0')
  	{
		if (*text != x)
			text++;
		else
			return (text);
	}

	return (text--);
}


/*	*************************************************************************
 *	Function name:	AdvancePast
 *	Description:
 *	Author(s):		Claude Sylvain
 *	Created:			2007
 *	Last modified:	23 December 2010
 *
 *	Parameters:		char *text:
 *							...
 *
 * 					char x:
 *							...
 *
 *	Returns:			char *:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

char *AdvancePast(char *text, char x)
{
	text = AdvanceTo(text, x);
	return (++text);
}


/*	*************************************************************************
 *	Function name:	AdvanceToAscii
 *	Description:
 *	Author(s):		Claude Sylvain
 *	Created:			2007
 *	Last modified:	23 December 2010
 *
 *	Parameters:		char *text:
 *							...
 *
 *	Returns:			char *:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

char *AdvanceToAscii(char *text)
{
	/*	i.e. not a space.
	 *	----------------- */	
	while (isspace((int) *text))
		text++;

	return (text);
}


/*	*************************************************************************
 *	Function name:	AdvanceToAscii
 *	Description:
 *	Author(s):		Claude Sylvain
 *	Created:			2007
 *	Last modified:	23 December 2010
 *
 *	Parameters:		char *text:
 *							...
 *
 *	Returns:			char *:
 *							...
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

char *AdvancePastSpace(char *text)
{
	return (AdvanceToAscii(text));
}


/*	*************************************************************************
 *	Function name:	util_is_cs_enable
 *	Description:	Tell if Code Section is Enabled or not.
 *	Author(s):		Claude Sylvain
 *	Created:			12 February 2011
 *	Last modified:
 *	Parameters:		void
 *
 *	Returns:			int:
 *							0	: Code Section innactive.
 *							1	: Code Section active.
 *
 *	Globals:				int if_true[]
 *							int if_nest
 *
 *	Notes:
 *	************************************************************************* */

int util_is_cs_enable(void)
{
	int	rv	= 1;		/*	Enable (default). */
	int	i;

	/*	- Search for disabled code section.
	 *	- If disabled code section is found, set return value
 	 *	  accordingly, and exit loop.
	 *	- Notes: If "if" level is the base level (0), do
	 *	  nothing, and exit with 1 (code section enable).
	 *	  This is normal, since "if" base level is always
	 *	  1.
	 *	----------------------------------------------------- */	 
	for (i = if_nest; i > 0; i--)
	{
		if (if_true[i] == 0)
		{
			rv	= 0;
			break;
		}
	}

	return (rv);
}







