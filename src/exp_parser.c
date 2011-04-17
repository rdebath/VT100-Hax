/*	*************************************************************************
 *	Module Name:	exp_parser.c
 *	Description:	Expression Parser module.
 *	Copyright(c):	See below...
 *	Author(s):		Claude Sylvain
 *	Created:			27 December 2010
 *	Last modified:	27 March 2011
 *
 *	Notes:			- This module implement an expression parser using
 *						  DAL (Direct Algebraic Logic) format.
 *						  RPN (Reverse Polish Notation) has been dropped down,
 *						  since January 2011.
 *
 * Ref.:				http://en.wikipedia.org/wiki/Reverse_Polish_notation
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
#include "main.h"
#include "exp_parser.h"


/*	*************************************************************************
 *												 STRUCT
 *	************************************************************************* */

struct operator_t
{
	const char	*name;
	int			op;
};


/*	*************************************************************************
 *												 STRUCT
 *	************************************************************************* */

/*	Expression Parser Stack.
 *	------------------------ */
struct ep_stack_t
{
	struct ep_stack_t	*prev;
	int					word[4];
	int					level;
};


#if 0
/*	*************************************************************************
 *	                                 TYPEDEF
 *	************************************************************************* */

/*	DAL (Direct Algebraic Logic) Expression Parser Stack.
 *	----------------------------------------------------- */
typedef struct dalep_stack_t
{
	int	word[4];
	int	level;
} stack_t;
#endif


/*	*************************************************************************
 *												  ENUM
 *	************************************************************************* */

/*	Expression Parser Stack Operators.
 *	---------------------------------- */	
enum Operators
{
	ADD, SUB, MUL, DIV, AND, OR, LSHIFT, RSHIFT
};


/*	*************************************************************************
 *												 CONST
 *	************************************************************************* */

/*	Private "const".
 *	**************** */

/*	Operators.
 *	---------- */	
static const struct operator_t	operator[]	=
{
	{"AND",	AND},
	{"OR",	OR},
	{"NOT",	0},
	{"LOW",	0},
	{"HIGH",	0},
	{"EQ",	0},
  	{NULL,	0}
};


/*	*************************************************************************
 *	                          FUNCTIONS DECLARATION
 *	************************************************************************* */

/*	Private functions.
 *	****************** */

static int search_operator(char *text, int *text_bp);
static void dpe_util_get_number_base_inc(void);
static int pop(void);
static void push(int);
static int add_stack(void);
static int remove_stack(void);
static int dalep(char *text);
static void eval(void);


/*	*************************************************************************
 *	                                VARIABLES
 *	************************************************************************* */

/*	Private variables.
 *	****************** */

//static stack_t	stack;

/*	- Expression Parser Stack.
 *	- Notes: This is the static base stack.  All other stacks
 *	  are dynamically created as needed.
 *	*/
//static struct ep_stack_t	ep_stack	= {NULL, {0, 0, 0, 0}, 0};
static struct ep_stack_t	ep_stack;

static struct ep_stack_t	*p_ep_stack	= &ep_stack;


/*	*************************************************************************
 *	                           FUNCTIONS DEFINITION
 *	************************************************************************* */


/*	*************************************************************************
 *	Function name:	search_operator
 *	Description:	Search for an Operator.
 *	Author(s):		Claude Sylvain
 *	Created:			28 December 2010
 *	Last modified:
 *
 *	Parameters:		char *text:
 *							Point to text that possibly hold an operator.
 *
 *						int *text_bp:
 *							Point to a variable that receive the number of
 *							text character globed, and have to be bypassed
 *							by the caller.
 *
 *	Returns:			int:
 *							-1		: No operator found.
 *							> 0	: Operator value.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int search_operator(char *text, int *text_bp)
{
	int	rv;
	char	*p_text;
//	int	text_len		= strlen(text);
	int	i				= 0;

	struct operator_t	*p_operator	= (struct operator_t *) operator;


	*text_bp	= 0;

//	p_text	= (char *) malloc(text_len + 1);
	p_text	= (char *) malloc(strlen(text) + 1);

	if (p_text == NULL)
	{
#if 0
		if (asm_pass == 1)
		{
			fprintf(list, "*** Error %d in \"%s\": Memory allocation error!\n", EC_MAE, in_fn[file_level]);
			fprintf(stderr, "*** Error %d in \"%s\" @%d: Memory allocation error!\n", EC_MAE, in_fn[file_level], codeline[file_level]);
		}
#endif
		fprintf(stderr, "*** Error %d: Memory allocation error!\n", EC_MAE);

		return (-1);
	}

	/*	Bypass space.
	 *	------------- */	
	while ((isspace((int) *text) != 0) && (*text != '\0'))
	{
		(*text_bp)++;
		text++;
	}


	/*	Get an uppercase working copy of "text".
	 *	**************************************** */
		
	while (*text != '\0')
	{
		if (isalnum((int) *text) == 0)
			break;

			(*text_bp)++;
			p_text[i++]	= toupper((int) *(text++));
	}

	p_text[i]	= '\0';

#if 0
	if (*p_text == '\0')
	{
		*text_bp	= 0;
		free(p_text);

		return (-1);
	}
#endif

	/*	Search the operator.
	 *	-------------------- */	
	while (p_operator->name != NULL)
	{
		if (strcmp(p_text, p_operator->name) == 0)
			break;

		p_operator++;
	}

	if (p_operator->name == NULL)
	{
		*text_bp	= 0;
		rv			= -1;
	}
	else
		rv	= p_operator->op;

	free(p_text);		/*	Free allocated memory. */

	return (rv);
}


/*	*************************************************************************
 *	Function name:	dpe_util_get_number_base_inc
 *
 *	Description:	- Display/Print Error, associated with
 *						  "util_get_number_base_inc()".
 *
 *	Author(s):		Claude Sylvain
 *	Created:			23 December 2010
 *	Last modified:	1 January 2011
 *	Parameters:		void
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void dpe_util_get_number_base_inc(void)
{
	if (asm_pass == 1)
	{
		if (list != NULL)
			fprintf(list, "*** Error %d in \"%s\": Bad data encoding!\n", EC_BDE, in_fn[file_level]);

		fprintf(stderr, "*** Error %d in \"%s\" @%d: Bad data encoding!\n", EC_BDE, in_fn[file_level], codeline[file_level]);
	}
}


/*	*************************************************************************
 *	Function name:	extract_byte
 *	Description:	Extract Byte.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	31 December 2010
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

int extract_byte(char *text)
{
	int	accum	= 0;
	int	inc	= 0;
	int	c;


	/*	Get numerical base increment, if possible.
	 *	------------------------------------------ */	
	if (isdigit((int) *text))
		inc	= util_get_number_base_inc(text);

	/*	Get the job done only if no error.
 	 *	---------------------------------- */	 
	if (inc != -1)
	{
		while (*text)
		{
			switch (*text)
			{
				case '+':
					text++;

					/*	Get numerical base increment, if possible.
					 *	------------------------------------------ */	
					if (isdigit((int) *text))
						inc	= util_get_number_base_inc(text);

					if (inc == -1)
					{
						dpe_util_get_number_base_inc();
						return (accum);
					}

					break;

				case '-':
					text++;

					/*	Get numerical base increment, if possible.
					 *	------------------------------------------ */	
					if (isdigit((int) *text))
						inc	= util_get_number_base_inc(text);

					if (inc == -1)
					{
						dpe_util_get_number_base_inc();
						return (accum);
					}

					break;

				case '\'':
					text++;
					accum = *text;
					return (accum);

				case '\0':
				case '\n':
				case '\t':
				case '\"':
					return (accum);

				case ' ':
					if (accum)
						return (accum);

					text++;

					/*	Get numerical base increment, if possible.
					 *	------------------------------------------ */	
					if (isdigit((int) *text))
						inc	= util_get_number_base_inc(text);

					if (inc == -1)
					{
						dpe_util_get_number_base_inc();
						return (accum);
					}

					break;

				case ',':
					return (accum);

				default:
					break;
			}

			accum *= inc;
			c		= toupper((int) *text);

			/*	Check/Convert/Accumulate, depending of the numerical base.
			 *	---------------------------------------------------------- */	
			switch (inc)
			{
				/*	Binary base.
				 *	------------ */	
				case 2:
					if ((c >= '0') && (c <= '1'))
					{
						accum += *(text++) - '0';
					}
					else
					{
						/*	Display/Print error, if possible.
						 *	--------------------------------- */
						if (asm_pass == 1)
						{
							if (list != NULL)
								fprintf(list, "*** Error %d in \"%s\": Bad binary digit (%c)!\n", EC_BBD, in_fn[file_level], *text);

							fprintf(stderr, "*** Error %d in \"%s\" @%d: Bad binary digit (%c)!\n", EC_BBD, in_fn[file_level], codeline[file_level], *text);
						}

						return (accum);	/* Punt. */
					}

					break;

				/*	Octal base.
				 *	----------- */	
				case 8:
					/*	Convert/Accumulate.
					 *	------------------- */	
					if ((c >= '0') && (c <= '7'))
					{
						accum += *(text++) - '0';
					}
					else
					{
						/*	Display/Print error, if possible.
						 *	--------------------------------- */
						if (asm_pass == 1)
						{
							if (list != NULL)
								fprintf(list, "*** Error %d in \"%s\": Bad octal digit (%c)!\n", EC_BOC, in_fn[file_level], *text);

							fprintf(stderr, "*** Error %d in \"%s\" @%d: Bad octal digit (%c)!\n", EC_BOC, in_fn[file_level], codeline[file_level], *text);
						}

						return (accum);	/* Punt. */
					}

					break;

				/*	Decimal base.
				 *	------------- */	
				case 10:
					/*	Convert/Accumulate.
					 *	------------------- */	
					if (isdigit(c) != 0)
					{
						accum += *(text++) - '0';
					}
					else
					{
						/*	Display/Print error, if possible.
						 *	--------------------------------- */
						if (asm_pass == 1)
						{
							if (list != NULL)
								fprintf(list, "*** Error %d in \"%s\": Bad decimal digit (%c)!\n", EC_BDD, in_fn[file_level], *text);

							fprintf(stderr, "*** Error %d in \"%s\" @%d: Bad decimal digit (%c)!\n", EC_BDD, in_fn[file_level], codeline[file_level], *text);
						}

						return (accum);	/* Punt. */
					}

					break;

				/*	Hexadecimal base.
				 *	----------------- */	
				case 16:
					/*	Convert/Accumulate.
					 *	------------------- */	
					if (isdigit(c) != 0)
					{
						accum += *(text++) - '0';
					}
					else if ((c >= 'A') && (c <= 'F'))
					{
						accum += (toupper((int) *(text++)) - 'A') + 10;
					}
					else
					{
						/*	Display/Print error, if possible.
						 *	--------------------------------- */
						if (asm_pass == 1)
						{
							if (list != NULL)
								fprintf(list, "*** Error %d in \"%s\": Bad hexadecimal digit (%c)!\n", EC_BHD, in_fn[file_level], *text);

							fprintf(stderr, "*** Error %d in \"%s\" @%d: Bad hexadecimal digit (%c)!\n", EC_BHD, in_fn[file_level], codeline[file_level], *text);
						}

						return (accum);	/* Punt. */
					}

					break;

				default:
					/*	Display/Print error, if possible.
					 *	--------------------------------- */
					if (asm_pass == 1)
					{
						if (list != NULL)
							fprintf(list, "*** Error %d in \"%s\": Bad data (\"%s\")!\n", EC_BD, in_fn[file_level], text);

						fprintf(stderr, "*** Error %d in \"%s\" @%d: Bad data (\"%s\")!\n", EC_BD, in_fn[file_level], codeline[file_level], text);
					}

					return (0);
			}
		}
	}
	else
	{
		dpe_util_get_number_base_inc();
	}

	return (accum);
}


/*	*************************************************************************
 *	Function name:	eval
 *	Description:
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	27 December 2010
 *	Parameters:
 *	Returns:
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void eval(void)
{
	int	a;
	int	b;
	int	op;


	/*	- Evaluation can be done only if there was at least 2 operand
	 *	  and 1  operator.
 	 *	------------------------------------------------------------- */	 
	if (p_ep_stack->level < 3)
		return;

	b	= pop();
	op	= pop();
	a	= pop();

	switch (op)
  	{
		case ADD:
			push(a + b);
			break;

		case SUB:
			push(a - b);
			break;

		case MUL:
			push(a * b);
			break;

		case DIV:
			push(a / b);
			break;

		case AND:
			push(a & b);
			break;

		case OR:
			push(a | b);
			break;

		case LSHIFT:
			push(a << b);
			break;

		case RSHIFT:
			push(a >> b);
			break;

		default:
			break;
	}
}


/*	*************************************************************************
 *	Function name:	add_stack
 *	Description:	Add Stack .
 *	Author(s):		Claude Sylvain
 *	Created:			2 January 2011
 *	Last modified:
 *	Parameters:		void
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: Operation successfull.
 *
 *	Globals:
 *	Notes:
 *	Warning:
 *	************************************************************************* */

static int add_stack(void)
{
	int	rv	= -1;

	/*	Create a new stack.
	 *	*/	
	struct ep_stack_t	*p_ep_stack_new =
		(struct ep_stack_t *) malloc(sizeof (struct ep_stack_t));

	/*	If stack creation was successfull...
	 *	------------------------------------ */	
	if (p_ep_stack_new != NULL)
	{
		/*	Init. previous stack link in the newly created stack.
		 *	*/
		p_ep_stack_new->prev	= p_ep_stack;

		/*	Make the newly created stack the current stack.
		 *	*/
		p_ep_stack				= p_ep_stack_new;

		p_ep_stack->level		= 0;		/*	Init. level. */
		rv							= 0;		/*	Success! */
	}
	else
	{
		if (asm_pass == 1)
		{
			if (list != NULL)
				fprintf(list, "*** Error %d in \"%s\": Memory Allocation Error!\n", EC_MAE, in_fn[file_level]);

			fprintf(stderr, "*** Error %d in \"%s\" @%d: Memory Allocation Error!\n", EC_MAE, in_fn[file_level], codeline[file_level]);
		}
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	remove_stack
 *	Description:	Remove Stack .
 *	Author(s):		Claude Sylvain
 *	Created:			2 January 2011
 *	Last modified:
 *	Parameters:		void
 *
 *	Returns:			int:
 *							-1	: Operation failed.
 *							0	: Operation successfull.
 *
 *	Globals:
 *	Notes:
 *	Warning:
 *	************************************************************************* */

static int remove_stack(void)
{
	int	rv	= -1;

	struct ep_stack_t	*p_ep_stack_prev;

	/*	Remove only if not at stack base level.
	 *	--------------------------------------- */	
	if (p_ep_stack != &ep_stack)
	{
		p_ep_stack_prev	= p_ep_stack->prev;	/*	Memoryse previous stack address. */
		free(p_ep_stack);								/*	Free current stack. */

		/*	Set current stack address to the previous stack address.
		 *	*/
		p_ep_stack			= p_ep_stack_prev;

		rv	= 0;		/*	Success! */
	}
	else
	{
		if (asm_pass == 1)
		{
			if (list != NULL)
				fprintf(list, "*** Error %d in \"%s\": Expression parser stack remove underflow!\n", EC_EPSRUF, in_fn[file_level]);

			fprintf(stderr, "*** Error %d in \"%s\" @%d: Expression parser stack remove underflow!\n", EC_EPSRUF, in_fn[file_level], codeline[file_level]);
		}
	}

	return (rv);
}


/*	*************************************************************************
 *	Function name:	dalep
 *	Description:	DAL (Direct Algebraic Logic) Expression Parser.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	12 March 2011
 *
 *	Parameters:		char *text:
 *							- Point to a string that hold expression to parse
 *							  and evaluate.
 *
 *	Returns:			int:
 *							...
 *
 *	Globals:
 *	Notes:
 *	Warning:  Recursive parseing.
 *	************************************************************************* */

static int dalep(char *text)
{
	while (1)
	{
		switch (*text)
		{
			case '\'':
			{
				text++;
				push(*text++);
				text++;
				break;
			}

			/*	Handle '('.
			 *	----------- */	
			case '(':
			{
				int	p_level	= 0;		/*	Parenthesis Level. */
				int	val;

				text++;			/*	Bypass '(' */
				add_stack();	/*	Add a new Stack. */

#if 0				
				/*	- Recursively Evaluate expression inside parenthise(s),
				 *	  and push result into the stack	.
				 *	*/
				push(dalep(text));
#endif
				/*	Recursively Evaluate expression inside parenthise(s).
				 *	*/
				val	= dalep(text);

				remove_stack();
				push(val);			/*	Push result into the stack	. */

				/*	Bypass expression(s) in parenthise(s).
				 *	-------------------------------------- */
				while (*text != '\0')
				{
					/*	Record additionnal parenthesis level.
					 *	------------------------------------- */	
					if (*text == '(')
						p_level++;

					/*	Check for matching ')'.
					 *	----------------------- */	
					if (*text == ')')
					{
						if (--p_level < 0)
							break;
					}

					text++;
				}

				/*	If no matching ')" found ...
				 *	---------------------------- */	
				if (*text == '\0')
				{
					if (asm_pass == 1)
					{
						if (list != NULL)
							fprintf(list, "*** Error %d in \"%s\": No matching ')'!\n", EC_NMEP, in_fn[file_level]);

						fprintf(stderr, "*** Error %d in \"%s\" @%d: No matching ')'!\n", EC_NMEP, in_fn[file_level], codeline[file_level]);
					}
				}
				/*	Bypass matching ')'.
				 *	-------------------- */		
				else
					text++;

//				remove_stack();
				break;
			}

#if 0
			/*	Handle ')'.
			 *	----------- */	
			case ')':
			{
				int	word_0;

				eval();									/*	Evaluate partial expression. */
				word_0	= p_ep_stack->word[0];	/*	Save result. */
				remove_stack();						/*	Now, we can remove stack. */
				return (word_0);
			}
#endif
			/*	Handle ')'.
			 *	----------- */	
			case ')':
				eval();									/*	Evaluate partial expression. */
				return (p_ep_stack->word[0]);

			case '&':
				push(AND);
				text++;
				break;

			case '|':
				push(OR);
				text++;
				break;

			case '+':
				/*	- If something pushed before, just push "+".
				 *	- Otherwise, we have an unary (+), and we have
			 	 *	  nothing to do.	 
				 *	---------------------------------------------- */	
				if (p_ep_stack->level > 0)
					push(ADD);

				text++;

				break;

			case '-':
				/*	- If something pushed before, just push "-".
				 *	- Otherwise, we have an unary (-), and we have
			 	 *	  to push "0-" on the stack.	 
				 *	---------------------------------------------- */	
				if (p_ep_stack->level > 0)
					push(SUB);
				else
				{
					push(0);
					push(SUB);
				}

				text++;

				break;

			case '<':
				push(LSHIFT);
				text++;
				text++;
				break;

			case '>':
				push(RSHIFT);
				text++;
				text++;
				break;

			case '*':
				push(MUL);
				text++;
				break;

			case '/':
				push(DIV);
				text++;
				break;

			case '$':
				push(addr);
				text++;
				break;

			case ' ':
			case '\t':
				text++;
				break;

			case '\0':
			case '\n':
			case ',':
			case ';':
				eval();
				return (p_ep_stack->word[0]);

			/*	Must be a label or a number.
			 *	---------------------------- */
			default:
			{
				int		text_bp;
				int		op;

				/*	Search for an operator.
				 *	----------------------- */	
				op	= search_operator(text, &text_bp);

				/*	If an operator was found, process it.
				 *	------------------------------------- */	
				if (op != -1)
				{
					push(op);				/*	Push Operator. */
					text	+=	text_bp;		/*	Bypass operator. */
				}
				/*	No operator found.
				 *	Search for a label.
			 	 *	------------------- */
				else
				{
					SYMBOL	*Local;
//					char		label[16];
					char		label[LABEL_SIZE_MAX];
					int		i			= 0;

					memset(label, 0, sizeof (label));

					/*	Get the label or number.
					 *	------------------------ */	
					while (isalnum((int) *text) || (*text == '_'))
						label[i++] = *text++;

					/*	Process label or number only if it exist.
					 *	----------------------------------------- */	
					if (i > 0)
					{
						/*	Could be a number.
						 *	------------------ */
						if (isdigit((int) label[0]))
						{
							push(extract_byte(label));
							eval();
						}
						else
						{
							Local = FindLabel(label);

							if (!Local)
							{
								if (asm_pass == 1)
								{
									if (list != NULL)
										fprintf(list, "*** Error %d in \"%s\": Label not found (%s)!\n", EC_LNF, in_fn[file_level], label);

									fprintf(stderr, "*** Error %d in \"%s\" @%d: Label not found (%s)!\n", EC_LNF, in_fn[file_level], codeline[file_level], label);
								}

								return (0);
							}

							push(Local->Symbol_Value);
							eval();
						}

					}
					/*	Label do not exist!
					 *	------------------- */	
					else
					{
						/*	Display/Print Error, if necessary.
						 *	---------------------------------- */	
						if (asm_pass == 1)
						{
							if (list != NULL)
								fprintf(list, "*** Error %d in \"%s\": Missing field!\n", EC_MF, in_fn[file_level]);

							fprintf(stderr, "*** Error %d in \"%s\" @%d: Missing field!\n", EC_MF, in_fn[file_level], codeline[file_level]);
						}

						return (0);
					}

				}		/*	op != -1 */
			}			/*	default. */

			break;
		}
	}

	return (0);
}


/*	*************************************************************************
 *	Function name:	exp_parser
 *	Description:	Expression Parser main entry point.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	2 January 2011
 *
 *	Parameters:		char *text:
 *							- Point to a string that hold expression to parse
 *							  and evaluate.
 *
 *	Returns:			int:
 *							Resulting expression Value.
 *
 *	Globals:
 *	Notes:
 *
 *	- Expressions can be as simple as [1], or complex like
 *	  "xyzzy"+23/7(45*3)+16<<test3
 * 
 *	- Parseing forms that this parser will handle:
 *			(expression) -- (stack) = (expression) [op] (expression)
 * 
 * - [op] can be:
 *			+	: Add
 *			-	: Subtract
 *			*	: Multiply
 *			/	: Divide
 *			<<	: Shift left
 *			>>	: Shift right
 *			|	: Or
 *			&	: And
 *			$	: Current address counter
 * 
 *	- Expression can have strings, numbers or labels.
 * 
 *	- Strings can be single quote, or double quote.
 *	  i.e.  'AB' or "AB".
 *	- Numbers can be base 10, base 16, base 8 or base 2.
 *	  i.e.  124 (124d), 3Eh, 12q or 01001001b.
 * 
 *	- At the end of the process, return (stack).
 * 
 *	- A typical expression that is seen in assembly is:
 *			LXI	H,<exp>
 *	  where <exp> is:
 *			("EV"+STARTOFTABLE)/4
 *
 *	  This breaks down to:
 *		(stack) = (/ operator) (stack) = 4
 *		(stack) = (+ operator) (stack) = STARTOFTABLE 	(the value of )
 *		(stack push) = "EV" (as a value)
 *	************************************************************************* */

int exp_parser(char *text)
{
	p_ep_stack->level	= 0;

	return (dalep(text));
}


/*	*************************************************************************
 *	Function name:	extract_word
 *	Description:	Extract Word.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	27 December 2010
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

int extract_word(char *text)
{
	int	tmp;

	while (isspace((int) *text))
		text++;

	if (*text == '%')
		return (0);

	if (*text == '\"')
	{
		text++;
		tmp	= *text << 8;
		text++;
		tmp	|= *text;
	}
	else if (*text == '-')
	{
		text++;
		tmp	= extract_byte(text);

		return (-tmp);
	}
	else if (*text == '+')
	{
		text++;
		tmp	= extract_byte(text);

		return (+tmp);
	}
	else
		tmp	= exp_parser(text);

	return (tmp);
}


/*	*************************************************************************
 *	Function name:	push
 *	Description:	Push a value to the stack.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	2 January 2011
 *
 *	Parameters:		int value:
 *							Value to push to the stack.
 *
 *	Returns:			void
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static void push(int value)
{
	p_ep_stack->word[3] = p_ep_stack->word[2];
	p_ep_stack->word[2] = p_ep_stack->word[1];
	p_ep_stack->word[1] = p_ep_stack->word[0];
	p_ep_stack->word[0] = value;

	if (++p_ep_stack->level >= (sizeof (p_ep_stack->word) / sizeof (int)))
	{
		p_ep_stack->level--;

		if (asm_pass == 1)
		{
			if (list != NULL)
				fprintf(list, "*** Error %d in \"%s\": Expression parser stack push overflow!\n", EC_EPSPOF, in_fn[file_level]);

			fprintf(stderr, "*** Error %d in \"%s\" @%d: Expression parser stack push overflow!\n", EC_EPSPOF, in_fn[file_level], codeline[file_level]);
		}
	}
}


/*	*************************************************************************
 *	Function name:	pop
 *	Description:	Pop a value from the stack.
 *	Author(s):		Jay Cotton, Claude Sylvain
 *	Created:			2007
 *	Last modified:	2 January 2011
 *	Parameters:		void
 *
 *	Returns:			int:
 *							Value poped from the stack.
 *
 *	Globals:
 *	Notes:
 *	************************************************************************* */

static int pop(void)
{
	int	value	= p_ep_stack->word[0];

	p_ep_stack->word[0] = p_ep_stack->word[1];
	p_ep_stack->word[1] = p_ep_stack->word[2];
	p_ep_stack->word[2] = p_ep_stack->word[3];
//	p_ep_stack->level--;

	if (--p_ep_stack->level < 0)
	{
		p_ep_stack->level	= 0;

		if (asm_pass == 1)
		{
			if (list != NULL)
				fprintf(list, "*** Error %d in \"%s\": Exression parser stack pop underflow!\n", EC_EPSPUF, in_fn[file_level]);

			fprintf(stderr, "*** Error %d in \"%s\" @%d: Exression parser stack pop underflow!\n", EC_EPSPUF, in_fn[file_level], codeline[file_level]);
		}
	}

	return (value);
}






