/*
    PROJECT: ppt
    MODULE:  misc.h

    $Id: misc.h,v 1.17 1998/09/05 11:31:12 jj Exp $

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

/*------------------------------------------------------------------*/
/* This is for memory debugging. I really would prefer Enforcer,
   though */

#include "fortify.h"

/*
 *  Standard allocation functions for large allocations
 */

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

/*
 *  Pool allocation functions.  If we're really paranoid, we will use
 *  the fortification functions as well.
 */

#if defined(PARANOID_MALLOC) && defined(FORTIFY)
#define smalloc(x)  malloc(x)
#define sfree(x)    free(x)
#else
#define smalloc(x)  SMalloc(x)
#define sfree(x)    SFree(x)
#endif

#endif /* MISC_H */
