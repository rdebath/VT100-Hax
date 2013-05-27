# *****************************************************************************
# Filename:			configure.ac
# Description:
# Copyright(c):	See below.
# Author(s):		Jay Cotton, Claude Sylvain
# Created:			26 Mars 2011
# Last modified:	26 May 2013
#
# Notes:				- Process this file with autoconf to produce a configure
#						  script.
# *****************************************************************************

# *****************************************************************************
# Copyright (c) <2007-2013> <jay.cotton@oracle.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# *****************************************************************************


#AC_PREREQ(2.60)

#AC_INIT(asm8080, 2.0 , lbmgmusic@gmail.com)
#AC_INIT(asm8080)
#AC_INIT(src)
AC_INIT([asm8080], [1.0.10])

#AC_CONFIG_SRCDIR([asm_dir.c], [exp_parser.c], [main.c], [opcode.c], [util.c])
#AC_CONFIG_HEADER([_dummy.h asm_dir.h err_code.h exp_parser.h main.h opcode.h project.h util.h war_code.h])

#AM_INIT_AUTOMAKE(asm8080, main)
#AM_INIT_AUTOMAKE(asm8080, 0.9.2)
AM_INIT_AUTOMAKE([1.9])


dnl Create a configuration header
#AM_CONFIG_HEADER([config.h])


dnl Initialize maintainer mode
		  AM_MAINTAINER_MODE


# Checks for programs.
# --------------------
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET


AM_PROG_CC_C_O


# Checks for libraries.
#
AM_PROG_LIBTOOL

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

#AC_CONFIG_FILES([Makefile src/Makefile])


#AC_OUTPUT(app/Makefile)
#AC_OUTPUT(src/Makefile Makefile)
#AC_OUTPUT
AC_OUTPUT(Makefile src/Makefile)


#CPPFLAGS="-g -O0 -Wall"
#CPPFLAGS="-O2"

#CPPFLAGS="-o2"
#CFLAGS="-O2"
#CFLAGS="-g -O0"
#CFLAGS=""

#LDFLAGS="-g"


AC_MSG_RESULT([
** Configuration summary for $PACKAGE $VERSION:
   PREFIX:                   $PREFIX
   CPPFLAGS:                 $CPPFLAGS
   CFLAGS:                   $CFLAGS
   LDFLAGS:                  $LDFLAGS
   LIBS:                     $LIBS
])





