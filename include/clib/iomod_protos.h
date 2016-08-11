/*
 *  $Id: iomod_protos.h,v 6.0 1999/11/28 18:50:18 jj Exp jj $
 */
#ifndef CLIB_IOMOD_PROTOS_H
#define CLIB_IOMOD_PROTOS_H

ULONG  IOInquire( ULONG attribute, struct PPTBase *PPTBase );
BOOL   IOCheck( BPTR fh, LONG len, UBYTE *buf, struct PPTBase *PPTBase );
PERROR IOLoad( BPTR fh, FRAME *frame, struct TagItem *tags, struct PPTBase *PPTBase );
PERROR IOSave( BPTR fh, ULONG format, FRAME *frame, struct TagItem *tags, struct PPTBase *PPTBase );
PERROR IOGetArgs( ULONG format, FRAME *frame, struct TagItem *tags, struct PPTBase *PPTBase );

#endif
