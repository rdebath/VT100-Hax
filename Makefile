## -*- Makefile -*-
##
## Project: src.prd
## User: jacotton
## Time: 07/10/30 09:53:34
## Makefile created by Sun WorkShop.
##
## This file is generated automatically -- DO NOT EDIT.
##



project: asm 

##### Compilers and tools definitions shared by all build objects #####
CFLAGS=-g -xsb 


###### Target: src ######
TARGETDIR_SRC=output
OBJS_SRC = \
	$(TARGETDIR_SRC)/asm.o 


# Link or archive
output/src: $(OBJS_SRC) 
	$(LINK.c)  $(CFLAGS_SRC) $(CPPFLAGS_SRC) -o output/src $(OBJS_SRC) -L/usr/openwin/lib -L/usr/dt/lib -lXm -lXt -lXext -lX11 -lm -lsocket -lnsl -lthread -lpthread -lintl -lgen -lposix4 -ldl -lcurses 


# Compile source files into .o's
$(TARGETDIR_SRC)/asm.o: asm.c
	$(COMPILE.c) $(CFLAGS_SRC) $(CPPFLAGS_SRC) -o $(TARGETDIR_SRC)/asm.o asm.c


###### clean target: deletes generated files ######
clean:
	$(RM) \
	output/src \
	$(TARGETDIR_SRC)/asm.o 
	$(RM) -r SunWS_cache/sb_* 

# Enable dependency checking
.KEEP_STATE:
.KEEP_STATE_FILE: /export/home/ASM8080/src/.make.state.Makefile.jacotton.src
