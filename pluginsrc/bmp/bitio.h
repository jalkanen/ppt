/*\
 * $Id: bitio.h,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
 *
 * bitio.h - bitstream I/O
 *
 * Works for (sizeof(unsigned long)-1)*8 bits.
 *
 * Copyright (C) 1992 by David W. Sanderson.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This software is provided "as is"
 * without express or implied warranty.
 *
 * $Log: bitio.h,v $
 * Revision 1.1  2001/10/25 16:22:59  jalkanen
 * *** empty log message ***
 *
 * Revision 1.1  1999/09/04 19:46:27  jj
 * Initial revision
 *
 * Revision 1.4  1992/11/24  19:37:02  dws
 * Added copyright
 *
 * Revision 1.3  1992/11/17  03:37:59  dws
 * updated comment
 *
 * Revision 1.2  1992/11/10  23:10:22  dws
 * Generalized to handle more than one bitstream at a time.
 *
 * Revision 1.1  1992/11/10  18:33:51  dws
 * Initial revision
 *
\*/

#ifndef _BITIO_H_
#define _BITIO_H_

#include <dos/dos.h>

typedef struct bitstream        *BITSTREAM;

/*
 * pm_bitinit() - allocate and return a BITSTREAM for the given FILE*.
 *
 * mode must be one of "r" or "w", according to whether you will be
 * reading from or writing to the BITSTREAM.
 *
 * Returns 0 on error.
 */

extern BITSTREAM pm_bitinit(BPTR, char *);

/*
 * pm_bitfini() - deallocate the given BITSTREAM.
 *
 * You must call this after you are done with the BITSTREAM.
 * 
 * It may flush some bits left in the buffer.
 *
 * Returns the number of bytes written, -1 on error.
 */

extern int pm_bitfini(BITSTREAM b);

/*
 * pm_bitread() - read the next nbits into *val from the given file.
 * 
 * Returns the number of bytes read, -1 on error.
 */

extern int pm_bitread(BITSTREAM b, unsigned long nbits, unsigned long *val);

/*
 * pm_bitwrite() - write the low nbits of val to the given file.
 * 
 * The last pm_bitwrite() must be followed by a call to pm_bitflush().
 * 
 * Returns the number of bytes written, -1 on error.
 */

extern int pm_bitwrite(BITSTREAM b, unsigned long nbits, unsigned long val);

#endif /* _BITIO_H_ */
