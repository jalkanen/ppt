/*
    PROJECT: ppt
    MODULE:  memory.c

    This contains the code for different memory allocation schemes
    and stuff.

    $Id: memory.c,v 1.2 1998/02/26 19:51:56 jj Exp $
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

Prototype ASM APTR F_pzmalloc( REG(d0) ULONG, REG(a0) char *, REG(d1) int );

ASM APTR F_pzmalloc( REG(d0) ULONG size, REG(a0) char *file, REG(d1) int line )
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

    if( SysBase->LibNode.lib_Version >= 39 ) {
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
    if( (SysBase->LibNode.lib_Version >= 39) && (poolhead) ) {
        DeletePool( poolhead );
        poolhead = NULL;
    }
}

Prototype APTR SMalloc( ULONG );

APTR SMalloc( ULONG size )
{
    struct SMemBlock *t;

    // D(bug("\tSMalloc(%ld)\n",size));

    if( (SysBase->LibNode.lib_Version >= 39) && (poolhead) ) {
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

    if( (SysBase->LibNode.lib_Version >= 39) && (poolhead) ) {
        t = (struct SMemBlock *) ((UBYTE *)q - sizeof( struct SMemBlock ));
        ObtainSemaphore( &poollock );
        FreePooled( poolhead, t, t->size );
        ReleaseSemaphore( &poollock );
    } else {
        pfree( q );
    }
}
