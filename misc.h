/*
    PROJECT: ppt
    MODULE: misc.h

    Miscallaneous defines that should NOT be put into a
    pre-compiled area.
*/

#ifndef MISC_H
#define MISC_H

#define XLIB_FUNCS      25

#define InternalError( txt )   Debug_InternalError( txt, __FILE__, __LINE__ )

/*------------------------------------------------------------------*/
/* This is for memory debugging. I really would prefer Enforcer,
   though */

#include <fortify.h>

#ifdef FORTIFY

#define pmalloc(x) malloc(x)
#define pfree(x)   free(x)

#define CheckPtr( ptr, txt )   Debug_CheckPtr( txt, ptr, __FILE__, __LINE__ )

extern BOOL Debug_CheckPtr( const char *, APTR, const char *file, int );

#else

#define pmalloc(x) AllocVec( (x), 0L )
#define pfree(x)   FreeVec( (x) )

#define CheckPtr( ptr,txt )   1

#endif

#endif /* MISC_H */
