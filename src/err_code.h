/*	*************************************************************************
 *	Module Name:	err_code.h
 *	Description:	Assembler Error Codes.
 *	Copyright(c):	See below...
 *	Author(s):		Claude Sylvain
 *	Created:			11 December 2010
 *	Last modified:	26 November 2011
 *	Notes:
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
 *	************************************************************************* */

#ifndef _ERR_CODE_H
#define _ERR_CODE_H


/*	*************************************************************************
 *											  CONSTANTS
 *	************************************************************************* */

#define	EC_LNF			0		/* Label Not Found. */
#define	EC_DL				1		/* Duplicate Label. */
#define	EC_CNFK			2		/* Can Not Find Keyword. */
#define	EC_BDR			3		/* Bad Destination Register. */
#define	EC_BSR			4		/* Bad Source Register. */
#define	EC_INO			5		/* 'if' Nesting Overflow. */
#define	EC_INU			6		/* 'if' Nesting Underflow. */
#define	EC_NQC			7		/* Not Quoted Character detected. */
#define	EC_MF				8		/* Missing Field. */
#define	EC_DWDNSS		9		/* "DW" Do Not Support String. */
#define	EC_BDE			10		/* Bad Data Encoding. */
#define	EC_BBD			11		/* Bad Binary Digit. */
#define	EC_BOC			12		/* Bad Octal Digit. */
#define	EC_BDD			13		/* Bad Decimal Digit. */
#define	EC_BHD			14		/* Bad Hexadecimal Digit. */
#define	EC_OOR			15		/* Operand Over Range. */
#define	WC_SNA			16		/* String Not Accepted. */
#define	EC_ACOF			17		/* Address Counter OverFlow. */
#define	EC_KTL			18		/* Keyword Too Long. */
#define	EC_ETL			19		/* Equation Too Long. */
#define	EC_LTL			20		/* Label Too Long. */
#define	EC_NAC			21		/* Non ASCII Character. */
#define	EC_SNS			22		/* String Not Supported. */
#define	EC_SLTL			23		/* Source Line Too Long. */
#define	EC_CAM			24		/* Can't Allocate Memory. */
#define	EC_IOF			25		/* Include OverFlow. */
#define	EC_NSQ			26		/* No Starting Quote. */
#define	EC_NEQ			27		/* No Ending Quote. */
#define	EC_BOF			28		/* Buffer OverFlow. */
#define	EC_COIF			29		/* Can't Open Include File. */
#define 	EC_EEP			30		/*	Extra Ending Parenthesis. */
#define 	EC_NMEP			31		/*	No Matching Ending Parenthesis. */
#define 	EC_B16BR			32		/*	Bad 16-Bit Register. */
#define 	EC_MAE			33		/*	Memory Allocation Error. */
#define 	EC_EPSPOF		34		/*	Expression Parser Stack Push OverFlow. */
#define 	EC_BD				35		/*	Bad Data. */
#define 	EC_EIFS			36		/*	Extra Input File Specified. */
#define 	EC_IFNTL			37		/*	Input File Name Too Long. */
#define 	EC_BTS			39		/*	Buffer Too Small. */
#define 	EC_COINF			40		/*	Can't Open INput File. */
#define 	EC_COLF			41		/*	Can't Open Listing File. */
#define 	EC_COBF			42		/*	Can't Open Binary File. */
#define 	EC_EPSRUF		43		/*	Expression Parser Stack Remove Underflow. */
#define 	EC_EPSPUF		44		/*	Expression Parser Stack Pop Underflow. */
#define 	EC_FLBOF			45		/*	"FindLabel()" Buffer OverFlow. */
#define 	EC_RNA			46		/*	Register Not Allowed. */
#define 	EC_NIFSNOOS		47		/*	No Input File Specified; No "-o" Option Specified. */



#endif





