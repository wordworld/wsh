/* bashtypes.h -- <sys/types.h> with special handling for crays. */

/* Copyright (C) 1993 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

#if !defined (__BASHTYPES_H)
#  define __BASHTYPES_H

#if defined (CRAY)
#  define word __word
#endif

#include <sys/types.h>

#if defined (CRAY)
#  undef word
#endif

#ifndef __NT_VC__
#define PIPE pipe
#else
#define PIPE(a) nt_pipe(a, __FILE__, __LINE__)
int nt_pipe(int filedes[2], const char *f, int l);

void nt_addPipeAssoc(int iFDRead, int iFDWrite);
void nt_deletePipeAssoc(int fd);
int nt_getPipeAssoc(int fd);

#endif

#endif /* __BASHTYPES_H */
