#
# This is a generic effect makefile for GCC and GNU make
#
# $Id: Makefile,v 3.1 2000/11/17 03:04:21 jalkanen Exp $
#

NAME = sample_effect

# Source files

SRCS=$(NAME).c
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

# Other stuff. Nothing of major importance.

# The MAINDIR directive tells where your PPT main directory
# is, so you should set it up before you try to install.

MAINDIR=work:source/c/ppt
INSTDIR=modules
INCDIR=$(MAINDIR)/include

# The CLIBDIR should point to where you have installed the
# CLib 37x package from Aminet by Andreas Kleinert

CLIBDIR	= ade:lib/libnix

EXE=$(NAME).effect
EXE020=$(NAME)_020.effect

CC 	= gcc
CFLAGS 	= -O -Wall -fbaserel -noixemul -msmall-code -fomit-frame-pointer -I$(INCDIR) -I/include -g
LDFLAGS	= -noixemul -nostartfiles -fbaserel


all : M000

M000 : $(OBJS)
	$(CC) $(LDFLAGS) $(CLIBDIR)/libinit.o $(OBJS) -o $(EXE)

M020 :
	sc buildprj CPU=68020 NOUTILLIB PROGNAME=$(EXE020) OBJNAME=020/

clean:
	Delete \#?.lnk \#?~ 020/\#?.o 000/\#?.o \#?.o

clobber: clean
	Delete $(EXE) $(EXE020)

install : M000 M020
    	slink $(EXE) TO $(MAINDIR)/$(INSTDIR)/$(EXE) STRIPDEBUG NOICONS
	slink $(EXE020) TO $(MAINDIR)/$(INSTDIR)/$(EXE020) STRIPDEBUG NOICONS

installdbg : M000
	copy $(EXE) TO $(MAINDIR)/$(INSTDIR) CLONE
	Delete $(MAINDIR)/$(INSTDIR)/$(EXE020)
