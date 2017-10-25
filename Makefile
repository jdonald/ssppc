#
# Makefile - simulator suite make file
#
# This file is a part of the SimpleScalar tool suite written by
# Todd M. Austin as a part of the Multiscalar Research Project.
#  
# The tool suite is currently maintained by Doug Burger and Todd M. Austin.
# 
# Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
#
# This source file is distributed "as is" in the hope that it will be
# useful.  It is distributed with no warranty, and no author or
# distributor accepts any responsibility for the consequences of its
# use. 
#
# Everyone is granted permission to copy, modify and redistribute
# this source file under the following conditions:
#
#    This tool set is distributed for non-commercial use only. 
#    Please contact the maintainer for restrictions applying to 
#    commercial use of these tools.
#
#    Permission is granted to anyone to make or distribute copies
#    of this source code, either as received or modified, in any
#    medium, provided that all copyright notices, permission and
#    nonwarranty notices are preserved, and that the distributor
#    grants the recipient permission for further redistribution as
#    permitted by this document.
#
#    Permission is granted to distribute this file in compiled
#    or executable form under the same conditions that apply for
#    source code, provided that either:
#
#    A. it is accompanied by the corresponding machine-readable
#       source code,
#    B. it is accompanied by a written offer, with no time limit,
#       to give anyone a machine-readable copy of the corresponding
#       source code in return for reimbursement of the cost of
#       distribution.  This written offer must permit verbatim
#       duplication by anyone, or
#    C. it is distributed by someone who received only the
#       executable form, and is accompanied by a copy of the
#       written offer of source code that they received concurrently.
#
# In other words, you are welcome to use, share and improve this
# source file.  You are forbidden to forbid anyone else to use, share
# and improve what you give them.
#
# INTERNET: dburger@cs.wisc.edu
# US Mail:  1210 W. Dayton Street, Madison, WI 53706
#
# $Id: Makefile,v 1.1.1.1 2001/08/10 14:58:21 karu Exp $
#
# $Log: Makefile,v $
# Revision 1.1.1.1  2001/08/10 14:58:21  karu
# creating ss3ppc-linux repository. The only difference between this and
# the original ss3ppc linux is that crosss endian issues are taken
# care of here.
#
#
# Revision 1.1.1.1  2001/08/08 21:28:51  karu
# creating ss3ppc linux repository
#
# Revision 1.14  2001/06/08 00:55:16  karu
# *** empty log message ***
#
# Revision 1.13  2001/06/08 00:47:41  karu
# removed -g in Makefile CC
#
# Revision 1.12  2001/06/08 00:47:00  karu
# fixed sim-cheetah. eio does not work because i am still not sure how much state to commit for system calls.
#
# Revision 1.11  2001/04/11 05:15:25  karu
# Fixed Makefile to work properly in Solaris also
# Fixed sim-cache.c
# Fixed a minor bug in sim-outorder. initializes stack_recover_idx to 0
# in line 5192. the top stack other has a junk value when the branch
# predictor returns from a function call. Dont know how this worked
# in AIX. the bug manifests itself only on Solaris.
#
# Revision 1.10  2000/05/31 03:08:34  karu
# sim-outorder merged
#
# Revision 1.9  2000/05/31 01:59:50  karu
# added support for mis-aligned accesses
#
# Revision 1.8  2000/04/11 00:55:54  karu
# made source platform independent. compiles on linux, solaris and ofcourse aix.
# powerpc-nonnative.def is the def file with no inline assemble code.
# manually create the machine.def symlink if compiling on non-aix OS.
# also syscall.c does not compile because the syscalls haven't been
# resourced with diff. flags and names for different operating systems.
#
# Revision 1.7  2000/04/08 00:52:20  karu
# added support for fsubs, fadds, fmuls etc..
# modified Makefile and added machine.o with machine.def as its dependency
#
# Revision 1.6  2000/04/04 01:31:20  karu
# added a few more commands to debugger
# and fixed a problem in system_configuration. thanks to ramdass for finding it out :-)
#
# Revision 1.5  2000/04/04 00:55:34  karu
# added interactive debugger from code steve gave me - ripped from M machine code
#
# Revision 1.4  2000/04/03 20:03:20  karu
# entire specf95 working .
#
# Revision 1.3  2000/03/28 00:37:11  karu
# added fstatx and added fdivs implementation
#
# Revision 1.2  2000/03/07 09:24:32  karu
# fixed millicode 3280 encoding bug
#
# Revision 1.1.1.1  2000/03/07 05:15:15  karu
# this is the repository created for my own maintanence.
# created when spec95 (lisp and compress worked).
# compress still had the scanf("%i") problem
# DIFF from the repository I am using alongwith ramdass on /projects
# need to merge the two sometime :-)
#
# Revision 1.1.1.1  2000/02/25 21:02:48  karu
# creating cvs repository for ss3-ppc
#
# Revision 1.1.1.1  2000/02/25 19:29:49  karu
# creating ss3-ppc repository
#
# Revision 1.8  1998/08/31 17:05:49  taustin
# added support for MS VC++ NMAKE builds (works with v5.0 SR3 or later)
#
# Revision 1.7  1998/08/27 07:40:03  taustin
# reorganized Makefile: it now works with MS VC++ NMAKE, and many host
#    configurations are supplied in the header; added target configuration
#    support; converted "sim-tests" target to use "-redir:sim" and
#    "-redir:prog" options, this eliminates the need for the "redir"
#    scripts
#
# Revision 1.6  1997/04/16  22:08:40  taustin
# added standalone loader support
#
# Revision 1.5  1997/03/11  01:04:13  taustin
# updated copyright
# CC target now supported
# RANLIB target now supported
# MAKE target now supported
# CFLAGS reorganized
# MFLAGS and MLIBS to improve portability
#
# Revision 1.1  1996/12/05  18:56:09  taustin
# Initial revision
#
# 

##################################################################
#
# Modify the following definitions to suit your build environment,
# NOTE: most platforms should not require any changes
#
##################################################################

#
# Define below C compiler and flags, machine-specific flags and libraries,
# build tools and file extensions, these are specific to a host environment,
# pre-tested environments follow...
#

##
## vanilla Unix, GCC build
##
## NOTE: the SimpleScalar simulators must be compiled with an ANSI C
## compatible compiler.
##
## tested hosts:
##
##	Slackware Linux version 2.0.33, GNU GCC version 2.7.2.2
##	FreeBSD version 3.0-current, GNU egcs version 2.91.50
##	Alpha OSF1 version 4.0, GNU GCC version 2.7.2
##	PA-RISC HPUX version B.10.01, GNU GCC version 2.7-96q3
##	SPARC SunOS version 5.5.1, GNU egcs-2.90.29
##	RS/6000 AIX Unix version 4, GNU GCC version cygnus-2.7-96q4
##	Windows NT version 4.0, Cygnus CygWin/32 beta 19
##
# CC = /lusr/gnu/bin/gcc -mpowerpc -I/usr/include
# CC =  /lusr/gnu/bin/gcc -DPPC_SWAP

CC = gcc

# CC = cc -misalign

# jdonald: MIN_SYSCALL_MODE only matters for x86. Likewise, most of these macros only matter for specific architectures
# jdonald: new modification. I removed MIN_SYSCALL_MODE, since it isn't supposed to be active on Linux, even if that's what the UCSD group set it to
OFLAGS = -g -Wall -DFP_ROUND_CONVERSION_INST -DUNIX -Dlinux -DPPC_SWAP
SIMFASTFLAGS = -Wno-unused-value -Wno-unused-variable -Wno-sequence-point
SIMOUTORDERFLAGS = -Wno-unused-value -Wno-unused-variable
MFLAGS = `./sysprobe -flags`
# SIMFASTFLAGS, SIMOUTORDERFLAGS, TFLAGS: I made these /jdonald

MLIBS  = `./sysprobe -libs` -lm
ENDIAN = `./sysprobe -s`
MAKE = make
AR = ar qcv
AROPT =
RANLIB = ranlib
RM = rm -f
RMDIR = rm -f
LN = ln -s
LNDIR = ln -s
DIFF = diff
OEXT = o
LEXT = a
ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN) # jdonald
EEXT = .exe
TFLAGS = -D__thread=
else
EEXT =
TFLAGS =
endif
CS = ;
X=/

##
## Alpha OSF1 version 4.0, DEC C compiler version V5.2-036
##
#CC = cc -std
#OFLAGS = -O0 -g -w
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## PA-RISC HPUX version B.10.01, c89 HP C compiler version A.10.31.02
##
#CC = c89 +e -D__CC_C89
#OFLAGS = -g
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## SPARC SunOS version 5.5.1, Sun WorkShop C Compiler (acc) version 4.2
##
#CC = /opt/SUNWspro/SC4.2/bin/acc
#OFLAGS = -O0 -g
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## RS/6000 AIX Unix version 4, xlc compiler build
##
#CC = xlc -D__CC_XLC
#OFLAGS = -g
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## WinNT, MS VC++ build
##
## NOTE: requires MS VC++ version 5.0 + service pack 3 or later
## NOTE1: before configuring the simulator, delete the symbolic link "tests/"
##
#CC = cl /Za /nologo
#OFLAGS = /W3 /Zi
#MFLAGS = -DBYTES_LITTLE_ENDIAN -DWORDS_LITTLE_ENDIAN -DFAST_SRL -DFAST_SRA
#MLIBS  =
#ENDIAN = little
#MAKE = nmake /nologo
#AR = lib
#AROPT = -out:
#RANLIB = dir
#RM = del/f/q
#RMDIR = del/s/f/q
#LN = copy
#LNDIR = xcopy/s/e/i
#DIFF = dir
#OEXT = obj
#LEXT = lib
#EEXT = .exe
#CS = &&
#X=\\\\

#
# Compilation-specific feature flags
#
# -DDEBUG	- turns on debugging features
# -DBFD_LOADER	- use libbfd.a to load programs (also required BINUTILS_INC
#		  and BINUTILS_LIB to be defined, see below)
# -DGZIP_PATH	- specifies path to GZIP executable, only needed if SYSPROBE
#		  cannot locate binary
# -DSLOW_SHIFTS	- emulate all shift operations, only used for testing as
#		  sysprobe will auto-detect if host can use fast shifts
#
FFLAGS = -DDEBUG

#
# Point the Makefile to your Simplescalar-based bunutils, these definitions
# should indicate where the include and library directories reside.
# NOTE: these definitions are only required if BFD_LOADER is defined.
#
#BINUTILS_INC = -I../include
#BINUTILS_LIB = -L../lib

#
#


##################################################################
#
# YOU SHOULD NOT NEED TO MODIFY ANYTHING BELOW THIS COMMENT
#
# well, I had to modify plenty. what are you talking about?
#   /jdonald
#
##################################################################

#
# complete flags
#
CFLAGS = $(MFLAGS) $(FFLAGS) $(OFLAGS) $(TFLAGS) $(BINUTILS_INC) $(BINUTILS_LIB)

#
# all the sources
#
SRCS =	main.c sim-fast.c sim-safe.c sim-uop.c sim-trace.c sim-cache.c sim-profile.c \
	sim-eio.c sim-bpred.c sim-cheetah.c sim-outorder.c sim-outorder-x86.c \
	memory.c regs.c cache.c bpred.c ptrace.c eventq.c \
	resource.c endian.c dlite.c symbol.c eval.c options.c range.c \
	eio.c stats.c endian.c misc.c \
	power.c power-x86.c time.c \
	flp.c RCutil.c temperature.c util.c \
	interactive-cmd.c interactive-read.c interactive-types.c \
	target-pisa/pisa.c target-pisa/loader.c target-pisa/syscall.c \
	target-pisa/symbol.c \
	target-alpha/alpha.c target-alpha/loader.c target-alpha/syscall.c \
	target-alpha/symbol.c

HDRS =	syscall.h memory.h regs.h sim.h loader.h cache.h mem-interface.h bpred.h ptrace.h \
	eventq.h resource.h endian.h dlite.h symbol.h eval.h bitmap.h \
	eio.h range.h version.h endian.h misc.h \
	power.h power-x86.h time.h \
	flp.h RC.h util.h \
	target-pisa/pisa.h target-pisa/pisa.def target-pisa/ecoff.h \
	target-alpha/alpha.h target-alpha/alpha.def target-alpha/ecoff.h

#
# common objects
#

# power.$(OEXT) time.$(OEXT) # now linked only with sim-outorder /jdonald

OBJS =	main.$(OEXT) syscall.$(OEXT) memory.$(OEXT) regs.$(OEXT) \
	loader.$(OEXT) endian.$(OEXT) dlite.$(OEXT) symbol.$(OEXT) \
	eval.$(OEXT) options.$(OEXT) stats.$(OEXT) eio.$(OEXT) \
	range.$(OEXT) misc.$(OEXT) \
	machine.$(OEXT) \

#
# programs to build
#
ifeq "$(shell test -f syscall_names.h && echo YES )" "YES" # target-x86 not implemented for sim-eio, sim-bpred, sim-profile, etc. yet
PROGS = sim-fast$(EEXT) sim-uop$(EEXT) sim-outorder-x86$(EEXT) wattch-interact$(EEXT) # removing sim-outorder-x86 for now /jdonald
else
PROGS = sim-fast$(EEXT) sim-safe$(EEXT) sim-uop$(EEXT) sim-trace$(EEXT) sim-eio$(EEXT) \
	sim-bpred$(EEXT) sim-profile$(EEXT) \
	sim-cheetah$(EEXT) sim-cache$(EEXT) sim-outorder$(EEXT) wattch-interact$(EEXT)
endif

#
# all targets, NOTE: library ordering is important...
#

all: $(PROGS)

#	@echo "my work is done here..."
# all: sim-fast$(EEXT) sim-outorder$(EEXT)

# jdonald added specific regs.c and regs.h for both alpha and pisa

config-pisa:
	-$(RM) machine.h machine.c machine.def loader.c symbol.c syscall.c regs.c regs.h syscall_names.h
	$(LN) target-pisa$(X)pisa.h machine.h
	$(LN) target-pisa$(X)pisa.c machine.c
	$(LN) target-pisa$(X)pisa.def machine.def
	$(LN) target-pisa$(X)loader.c loader.c
	$(LN) target-pisa$(X)symbol.c symbol.c
	$(LN) target-pisa$(X)syscall.c syscall.c
	$(LN) target-pisa$(X)regs.c regs.c
	$(LN) target-pisa$(X)regs.h regs.h
	-$(RMDIR) tests
	$(LNDIR) tests-pisa tests

# jdonald: Alpha uses simply the pisa regs.c and regs.h

config-alpha:
	-$(RM) machine.h machine.c machine.def loader.c symbol.c syscall.c regs.c regs.h syscall_names.h
	$(LN) target-alpha$(X)alpha.h machine.h
	$(LN) target-alpha$(X)alpha.c machine.c
	$(LN) target-alpha$(X)alpha.def machine.def
	$(LN) target-alpha$(X)loader.c loader.c
	$(LN) target-alpha$(X)symbol.c symbol.c
	$(LN) target-alpha$(X)syscall.c syscall.c
	$(LN) target-pisa$(X)regs.c regs.c
	$(LN) target-pisa$(X)regs.h regs.h
	-$(RMDIR) tests
	$(LNDIR) tests-alpha tests

config-ppc:
	-$(RM) machine.h machine.c machine.def loader.c symbol.c syscall.c regs.c regs.h syscall_names.h
	$(LN) target-ppc$(X)powerpc.h machine.h
	$(LN) target-ppc$(X)powerpc.c machine.c
	$(LN) target-ppc$(X)powerpc-trace.def machine.def
	$(LN) target-ppc$(X)loader.c loader.c
	$(LN) target-ppc$(X)symbol.c symbol.c
	$(LN) target-ppc$(X)syscall.c syscall.c
	$(LN) target-ppc$(X)regs.c regs.c
	$(LN) target-ppc$(X)regs.h regs.h
	-$(RMDIR) tests
	$(LNDIR) tests-ppc tests

config-arm:
	-$(RM) machine.h machine.c machine.def loader.c symbol.c syscall.c regs.c regs.h syscall_names.h
	$(LN) target-arm$(X)arm.h machine.h
	$(LN) target-arm$(X)arm.c machine.c
	$(LN) target-arm$(X)arm.def machine.def
	$(LN) target-arm$(X)loader.c loader.c
	$(LN) target-arm$(X)symbol.c symbol.c
	$(LN) target-arm$(X)syscall.c syscall.c
	$(LN) target-arm$(X)regs.c regs.c
	$(LN) target-arm$(X)regs.h regs.h
	-$(RMDIR) tests
	$(LNDIR) tests-ppc tests

# config-x86 in ss-x86-sel seems to require make.target and config.h, but
# there doesn't seem to be much content in those files. I'll move the
# make.target text into this Makefile /jdonald

config-x86:
	-$(RM) machine.h machine.c machine.def loader.c symbol.c syscall.c regs.c regs.h syscall_names.h
	$(LN) target-x86$(X)x86.h machine.h
	$(LN) target-x86$(X)x86.c machine.c
	$(LN) target-x86$(X)x86.def machine.def
	$(LN) target-x86$(X)loader.c loader.c
	$(LN) target-x86$(X)symbol.c symbol.c
	$(LN) target-x86$(X)syscall.c syscall.c
	$(LN) target-x86$(X)regs.c regs.c
	$(LN) target-x86$(X)regs.h regs.h
	$(LN) target-x86$(X)syscall_names.h syscall_names.h
	-$(RMDIR) tests
	$(LNDIR) tests-x86 tests

sysprobe$(EEXT):	sysprobe.c
	$(CC) $(FFLAGS) -o sysprobe$(EEXT) sysprobe.c
	@echo endian probe results: $(ENDIAN)
	@echo probe flags: $(MFLAGS)
	@echo probe libs: $(MLIBS)

wattch-interact$(EEXT):	sysprobe$(EEXT) eval.$(OEXT) misc.$(OEXT) stats.$(OEXT) power.$(OEXT) wattch-interact.$(OEXT) cacti/libcacti.$(LEXT)
	$(CC) eval.$(OEXT) misc.$(OEXT) stats.$(OEXT) power.$(OEXT) wattch-interact.$(OEXT) cacti/libcacti.$(LEXT) $(CFLAGS) -lc -lm -o wattch-interact

sim-fast$(EEXT):	sysprobe$(EEXT) sim-fast.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) interactive-cmd.$(OEXT) interactive-read.$(OEXT) interactive-types.$(OEXT)
	$(CC) -o sim-fast$(EEXT) $(CFLAGS) sim-fast.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS) interactive-cmd.$(OEXT) interactive-read.$(OEXT) interactive-types.$(OEXT)

sim-safe$(EEXT):	sysprobe$(EEXT) sim-safe.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-safe$(EEXT) $(CFLAGS) $(SIMFASTFLAGS) sim-safe.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-uop$(EEXT):	sysprobe$(EEXT) sim-uop.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-uop$(EEXT) $(CFLAGS) $(SIMFASTFLAGS) sim-uop.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-trace$(EEXT):	sysprobe$(EEXT) sim-trace.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) interactive-cmd.$(OEXT) interactive-read.$(OEXT) interactive-types.$(OEXT)
	$(CC) -o sim-trace$(EEXT) $(CFLAGS) sim-trace.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS) interactive-cmd.$(OEXT) interactive-read.$(OEXT) interactive-types.$(OEXT)

sim-profile$(EEXT):	sysprobe$(EEXT) sim-profile.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-profile$(EEXT) $(CFLAGS) sim-profile.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-eio$(EEXT):	sysprobe$(EEXT) sim-eio.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-eio$(EEXT) $(CFLAGS) sim-eio.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-bpred$(EEXT):	sysprobe$(EEXT) sim-bpred.$(OEXT) bpred.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-bpred$(EEXT) $(CFLAGS) sim-bpred.$(OEXT) bpred.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-cheetah$(EEXT):	sysprobe$(EEXT) sim-cheetah.$(OEXT) $(OBJS) libcheetah/libcheetah.$(LEXT) libexo/libexo.$(LEXT)
	$(CC) -o sim-cheetah$(EEXT) $(CFLAGS) sim-cheetah.$(OEXT) $(OBJS) libcheetah/libcheetah.$(LEXT) libexo/libexo.$(LEXT) $(MLIBS)

sim-cache$(EEXT):	sysprobe$(EEXT) sim-cache.$(OEXT) cache.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-cache$(EEXT) $(CFLAGS) sim-cache.$(OEXT) cache.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-outorder$(EEXT):	sysprobe$(EEXT) sim-outorder.$(OEXT) cache.$(OEXT) bpred.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) power.$(OEXT) time.$(OEXT) flp.$(OEXT) RCutil.$(OEXT) temperature.$(OEXT) util.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) cacti/libcacti.$(LEXT)
	$(CC) -o sim-outorder$(EEXT) $(CFLAGS) sim-outorder.$(OEXT) cache.$(OEXT) bpred.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) power.$(OEXT) time.$(OEXT) flp.$(OEXT) RCutil.$(OEXT) temperature.$(OEXT) util.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) cacti/libcacti.$(LEXT) $(MLIBS) -lpthread

# jdonald: sel.$(OEXT) originally came right after util.$(OEXT). However, I'm setting this up for the no-SEL mode
sim-outorder-x86$(EEXT):	sysprobe$(EEXT) sim-outorder-x86.$(OEXT) cache.$(OEXT) bpred.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) power-x86.$(OEXT) time.$(OEXT) flp.$(OEXT) RCutil.$(OEXT) temperature.$(OEXT) util.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) cacti/libcacti.$(LEXT)
	$(CC) -o sim-outorder-x86$(EEXT) $(CFLAGS) sim-outorder-x86.$(OEXT) cache.$(OEXT) bpred.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) power-x86.$(OEXT) time.$(OEXT) flp.$(OEXT) RCutil.$(OEXT) temperature.$(OEXT) util.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) cacti/libcacti.$(LEXT) $(MLIBS) -lpthread

exo libexo/libexo.$(LEXT): sysprobe$(EEXT)
	$(MAKE) -C libexo "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" "RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(OFLAGS) $(TFLAGS)" "OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" "RM=$(RM)" libexo.$(LEXT)

cacti cacti/libcacti.$(LEXT): sysprobe$(EEXT)
	$(MAKE) -C cacti "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" "RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(SAFEOFLAGS)" "OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" "RM=$(RM)" libcacti.$(LEXT)

cheetah libcheetah/libcheetah.$(LEXT): sysprobe$(EEXT)
	$(MAKE) -C libcheetah "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" "RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(OFLAGS)" "OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" "RM=$(RM)" libcheetah.$(LEXT)

.c.$(OEXT):
	$(CC) $(CFLAGS) -c $*.c

sim-fast.$(OEXT): sim-fast.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-fast.c

sim-outorder.$(OEXT): sim-outorder.c
	$(CC) $(CFLAGS) $(SIMOUTORDERFLAGS) -c sim-outorder.c

sim-outorder-x86.$(OEXT): sim-outorder-x86.c
	$(CC) $(CFLAGS) $(SIMOUTORDERFLAGS) -c sim-outorder-x86.c
	
sim-safe.$(OEXT): sim-safe.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-safe.c

sim-uop.$(OEXT): sim-uop.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-uop.c

sim-eio.$(OEXT): sim-eio.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-eio.c

sim-trace.$(OEXT): sim-trace.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-trace.c

# the ones below this line can build, but i haven't used or tested them.

sim-bpred.$(OEXT): sim-bpred.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-bpred.c

sim-profile.$(OEXT): sim-profile.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-profile.c
	
sim-cache.$(OEXT): sim-cache.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-cache.c

sim-cheetah.$(OEXT): sim-cheetah.c
	$(CC) $(CFLAGS) $(SIMFASTFLAGS) -c sim-cheetah.c

filelist:
	@echo $(SRCS) $(HDRS) Makefile

diffs:
	-rcsdiff RCS/*

sim-tests sim-tests-nt: sysprobe$(EEXT) $(PROGS)
	$(MAKE) -C tests "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-fast$(EEXT)" \
		"X=$(X)" "CS=$(CS)" \
	$(MAKE) -C tests "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-safe$(EEXT)" \
		"X=$(X)" "CS=$(CS)" \
	$(MAKE) -C tests "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-uop$(EEXT)" \
		"X=$(X)" "CS=$(CS)" \
	$(MAKE) -C tests "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-cache$(EEXT)" \
		"X=$(X)" "CS=$(CS)" \
	$(MAKE) -C tests "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-cheetah$(EEXT)" \
		"X=$(X)" "CS=$(CS)" \
	$(MAKE) -C tests "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-bpred$(EEXT)" \
		"X=$(X)" "CS=$(CS)" \
	$(MAKE) -C tests "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-profile$(EEXT)" \
		"X=$(X)" "CS=$(CS)" "SIM_OPTS=-all" \
	$(MAKE) -C tests "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-outorder$(EEXT)" \
		"X=$(X)" "CS=$(CS)" \

clean:
	-$(RM) *.o *.obj core *~ Makefile.bak sysprobe$(EEXT) $(PROGS)
	$(MAKE) -C cacti "RM=$(RM)" "CS=$(CS)" clean
	$(MAKE) -C libcheetah "RM=$(RM)" "CS=$(CS)" clean
	$(MAKE) -C libexo "RM=$(RM)" "CS=$(CS)" clean

#		$(MAKE) -C tests "RM=$(RM)" "CS=$(CS)" clean

unpure:
	rm -f sim.pure *pure*.o sim.pure.pure_hardlink sim.pure.pure_linkinfo

depend:
	makedepend.local -n -x $(BINUTILS_INC) $(SRCS)


# DO NOT DELETE THIS LINE -- make depend depends on it.

main.$(OEXT): host.h misc.h machine.h machine.def endian.h version.h dlite.h
main.$(OEXT): regs.h memory.h options.h stats.h eval.h loader.h sim.h
sim-fast.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-fast.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h sim.h 
sim-safe.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-safe.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h sim.h
sim-uop.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-uop.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h sim.h
sim-trace.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-trace.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h sim.h
sim-cache.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-cache.$(OEXT): options.h stats.h eval.h cache.h loader.h syscall.h
sim-cache.$(OEXT): dlite.h sim.h
sim-profile.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-profile.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h
sim-profile.$(OEXT): symbol.h sim.h
sim-eio.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-eio.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h eio.h
sim-eio.$(OEXT): range.h sim.h
sim-bpred.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-bpred.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h
sim-bpred.$(OEXT): bpred.h sim.h
sim-cheetah.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-cheetah.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h
sim-cheetah.$(OEXT): libcheetah/libcheetah.h sim.h
sim-outorder.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-outorder.$(OEXT): options.h stats.h eval.h cache.h loader.h syscall.h
sim-outorder.$(OEXT): bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
sim-outorder.$(OEXT): sim.h eio.h
sim-outorder-x86.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-outorder-x86.$(OEXT): options.h stats.h eval.h cache.h loader.h syscall.h
sim-outorder-x86.$(OEXT): bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
sim-outorder-x86.$(OEXT): sim.h eio.h
interactive-cmd.$(OEXT): interactive-cmd.h
interactive-read.$(OEXT): interactive-read.h
interactive-types.$(OEXT): interactive-types.h
memory.$(OEXT): host.h misc.h machine.h machine.def options.h stats.h eval.h
memory.$(OEXT): memory.h
regs.$(OEXT): host.h misc.h machine.h machine.def loader.h regs.h memory.h
regs.$(OEXT): options.h stats.h eval.h
cache.$(OEXT): host.h misc.h machine.h machine.def cache.h memory.h options.h
cache.$(OEXT): stats.h eval.h
bpred.$(OEXT): host.h misc.h machine.h machine.def bpred.h stats.h eval.h
ptrace.$(OEXT): host.h misc.h machine.h machine.def range.h ptrace.h
eventq.$(OEXT): host.h misc.h machine.h machine.def eventq.h bitmap.h
resource.$(OEXT): host.h misc.h resource.h
endian.$(OEXT): endian.h loader.h host.h misc.h machine.h machine.def regs.h
endian.$(OEXT): memory.h options.h stats.h eval.h
dlite.$(OEXT): host.h misc.h machine.h machine.def version.h eval.h regs.h
dlite.$(OEXT): memory.h options.h stats.h sim.h symbol.h loader.h range.h
dlite.$(OEXT): dlite.h
symbol.$(OEXT): host.h misc.h target-pisa/ecoff.h loader.h machine.h
symbol.$(OEXT): machine.def regs.h memory.h options.h stats.h eval.h symbol.h
eval.$(OEXT): host.h misc.h eval.h machine.h machine.def
options.$(OEXT): host.h misc.h options.h
range.$(OEXT): host.h misc.h machine.h machine.def symbol.h loader.h regs.h
range.$(OEXT): memory.h options.h stats.h eval.h range.h
eio.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h options.h
eio.$(OEXT): stats.h eval.h loader.h libexo/libexo.h host.h misc.h machine.h
eio.$(OEXT): syscall.h sim.h endian.h eio.h
stats.$(OEXT): host.h misc.h machine.h machine.def eval.h stats.h
endian.$(OEXT): endian.h loader.h host.h misc.h machine.h machine.def regs.h
endian.$(OEXT): memory.h options.h stats.h eval.h
misc.$(OEXT): host.h misc.h machine.h machine.def
pisa.$(OEXT): host.h misc.h machine.h machine.def eval.h regs.h
loader.$(OEXT): host.h misc.h machine.h machine.def endian.h regs.h memory.h
loader.$(OEXT): options.h stats.h eval.h sim.h eio.h loader.h
loader.$(OEXT): target-pisa/ecoff.h target-ppc/xcoff.h
syscall.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
syscall.$(OEXT): options.h stats.h eval.h loader.h sim.h endian.h eio.h
syscall.$(OEXT): syscall.h
symbol.$(OEXT): host.h misc.h target-pisa/ecoff.h loader.h machine.h
symbol.$(OEXT): machine.def regs.h memory.h options.h stats.h eval.h symbol.h
alpha.$(OEXT): host.h misc.h machine.h machine.def eval.h regs.h
loader.$(OEXT): host.h misc.h machine.h machine.def endian.h regs.h memory.h
loader.$(OEXT): options.h stats.h eval.h sim.h eio.h loader.h
loader.$(OEXT): target-alpha/ecoff.h target-alpha/alpha.h
syscall.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
syscall.$(OEXT): options.h stats.h eval.h loader.h sim.h endian.h eio.h
syscall.$(OEXT): syscall.h
symbol.$(OEXT): host.h misc.h loader.h machine.h machine.def regs.h memory.h
symbol.$(OEXT): options.h stats.h eval.h symbol.h target-alpha/ecoff.h
symbol.$(OEXT): target-alpha/alpha.h
machine.$(OEXT): machine.h host.h misc.h eval.h regs.h machine.def
power.$(OEXT): power.h machine.h cache.h stats.h
power-x86.$(OEXT): power-x86.h machine.h cache.h stats.h
time.$(OEXT): power.h
flp.$(OEXT): flp.h util.h
RCutil.$(OEXT): RC.h flp.h util.h misc.h
temperature.$(OEXT): RC.h flp.h util.h misc.h
util.$(OEXT): util.h misc.h
sel.$(OEXT): host.h misc.h machine.h regs.h memory.h loader.h libexo/libexo.h
sel.$(OEXT): syscall.h sim.h endian.h sel.h
