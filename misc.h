/*
    PROJECT: ppt
    MODULE:  misc.h

    $Id: misc.h,v 1.14 1997/10/19 16:52:32 jj Exp $

    Miscallaneous defines that should NOT be put into a
    pre-compiled area.
*/

#ifndef MISC_H
#define MISC_H

#define XLIB_FUNCS      41

#define InternalError( txt )   Debug_InternalError( txt, __FILE__, __LINE__ )

/*------------------------------------------------------------------*/
/* Temporary defines that do not warrant a full recompile now.
   Should be moved to defs.h / ppt_real.h when debugged. */

#define ATTACH_SIMPLE       0x01
#define ATTACH_ALPHA        0x02

#define DSBF_INTERIM        (1<<0)
#define DSBF_FIXEDRECT      (1<<1)

/*------------------------------------------------------------------*/
/* This is for memory debugging. I really would prefer Enforcer,
   though */

#include "fortify.h"

#ifdef FORTIFY

#define pmalloc(x) malloc(x)
#define pfree(x)   free(x)
#define pzmalloc(x) F_pzmalloc(x, __FILE__, __LINE__ )

#define CheckPtr( ptr, txt )   Debug_CheckPtr( txt, ptr, __FILE__, __LINE__ )

extern BOOL Debug_CheckPtr( const char *, const APTR, const char *file, int );

#else

#define pmalloc(x) AllocVec( (x), 0L )
#define pfree(x)   FreeVec( (x) )
#define pzmalloc(x) AllocVec( (x), MEMF_CLEAR )

#define CheckPtr( ptr,txt )   1

#endif

#define smalloc(x)  SMalloc(x)
#define sfree(x)    SFree(x)


#endif /* MISC_H */
