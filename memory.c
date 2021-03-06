/*
    PROJECT: ppt
    MODULE:  memory.c

    This contains the code for different memory allocation schemes
    and stuff.

    $Id: memory.c,v 1.4 1999/03/17 23:08:53 jj Exp $
*/

#include "defs.h"
#include "misc.h"

/*--------------------------------------------------------------------------*/
/* Types */

struct SMemBlock {
    ULONG       size;
    UBYTE       data[0];
};

/*--------------------------------------------------------------------------*/
/* Globals */

Local struct SignalSemaphore poollock;
Local void   *poolhead;

/*--------------------------------------------------------------------------*/

/*
    Allocate and zero reserved memory area.
            AllocPooled
    This is normally #defined as pmalloc()
*/

Prototype APTR ASM F_pzmalloc( REGDECL(d0,ULONG), REGDECL(a0,char *), REGDECL(d1,int) );

APTR ASM F_pzmalloc( REGPARAM(d0,ULONG,size), REGPARAM(a0,char *,file), REGPARAM(d1,int,line) )
{
    APTR area;

    area = Fortify_malloc( size, file, line );
    if(area)
        bzero( area, size );

    return area;
}

Prototype PERROR OpenPool(VOID);

PERROR OpenPool( VOID )
{
    PERROR res = PERR_OK;

    if( SYSV39 ) {
        if(!(poolhead = CreatePool( MEMF_PUBLIC, POOL_PUDDLESIZE, POOL_THRESHSIZE )))
            res = PERR_OUTOFMEMORY;

        InitSemaphore( &poollock );
    } else {
        poolhead = NULL;
    }
    return res;
}

Prototype VOID ClosePool(VOID);

VOID ClosePool( VOID )
{
    if( SYSV39 && (poolhead) ) {
        DeletePool( poolhead );
        poolhead = NULL;
    }
}

Prototype APTR SMalloc( ULONG );

APTR SMalloc( ULONG size )
{
    struct SMemBlock *t;

    // D(bug("\tSMalloc(%ld)\n",size));

    if( SYSV39 && (poolhead) ) {
        ObtainSemaphore( &poollock );
        t = (struct SMemBlock *)AllocPooled( poolhead, size + sizeof(struct SMemBlock) );
        ReleaseSemaphore( &poollock );

        t->size = size;

        return (APTR) &(t->data);
    } else {
        return pmalloc( size );
    }
}

Prototype VOID SFree( APTR );

VOID SFree( APTR q )
{
    struct SMemBlock *t;

    // D(bug("\tSFree()\n"));

    if( SYSV39 && (poolhead) ) {
        t = (struct SMemBlock *) ((UBYTE *)q - sizeof( struct SMemBlock ));
        ObtainSemaphore( &poollock );
        FreePooled( poolhead, t, t->size + sizeof(struct SMemBlock) );
        ReleaseSemaphore( &poollock );
    } else {
        pfree( q );
    }
}
