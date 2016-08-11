/*
 *  $Id: effect_protos.h,v 3.0 1999/11/28 18:49:54 jj Exp jj $
 */
#ifndef CLIB_EFFECT_PROTOS_H
#define CLIB_EFFECT_PROTOS_H

#ifndef PPT_H
#include "ppt.h"
#endif

ULONG EffectInquire( ULONG attribute, struct PPTBase *PPTBase );
FRAME *EffectExec( FRAME *, struct TagItem *, struct PPTBase *PPTBase );
PERROR EffectGetArgs( FRAME *, struct TagItem *, struct PPTBase *PPTBase );

#endif

