#!/bin/csh -f

#
# redir - redirect stdout and stderr to files
#
# This file is a part of the SimpleScalar tool suite written by
# Todd M. Austin as a part of the Multiscalar Research Project.
#  
# The tool suite is currently maintained by Doug Burger and Todd M. Austin.
# 
# Copyright (C) 1994, 1995, 1996, 1997 by Todd M. Austin
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
# $Id: redir.sh,v 1.1.1.1 2001/08/10 14:58:22 karu Exp $
#
# $Log: redir.sh,v $
# Revision 1.1.1.1  2001/08/10 14:58:22  karu
# creating ss3ppc-linux repository. The only difference between this and
# the original ss3ppc linux is that crosss endian issues are taken
# care of here.
#
#
# Revision 1.1.1.1  2001/08/08 21:28:53  karu
# creating ss3ppc linux repository
#
# Revision 1.1.1.1  2000/03/07 05:15:17  karu
# this is the repository created for my own maintanence.
# created when spec95 (lisp and compress worked).
# compress still had the scanf("%i") problem
# DIFF from the repository I am using alongwith ramdass on /projects
# need to merge the two sometime :-)
#
# Revision 1.1.1.1  2000/02/25 21:02:51  karu
# creating cvs repository for ss3-ppc
#
# Revision 1.1  1998/08/27 15:52:34  taustin
# Initial revision
#
# 

if ( $#argv < 3 ) then
    echo Usage: redir \<stdout_file\> \<stderr_file\> \<command and args...\>
    exit 1
endif

set out = $1
shift
set err = $1
shift

($* >! $out) >&! $err
exit $status
