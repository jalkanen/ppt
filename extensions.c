/*
    PROJECT: ppt
    MODULE:  extensions.c

    This module contains both extensions and options routines.

    $Id: extensions.c,v 1.1 1997/02/22 21:38:18 jj Exp $
*/

#include "defs.h"
#include "misc.h"

/*-------------------------------------------------------------------*/

struct OptNode {
    struct Node         on_Node;
    STRPTR              on_Name;
    ULONG               on_Length;
    UBYTE               on_Data[0];
};

/*-------------------------------------------------------------------*/

struct List optlist;

/*-------------------------------------------------------------------*/


Local
VOID ReleaseOptNode( struct OptNode *on, EXTBASE *ExtBase )
{
    sfree( on->on_Name );
    sfree( on );
}

Local
struct OptNode *AllocOptNode( STRPTR name, ULONG size, EXTBASE *ExtBase )
{
    struct OptNode *on;

    on = (struct OptNode *)smalloc( sizeof(struct OptNode) + size );
    if( on ) {
        on->on_Name = smalloc( strlen(name)+1 );
        if( on->on_Name ) {
            strcpy( on->on_Name, name );
            on->on_Node.ln_Name = on->on_Name;
            on->on_Length = size;
        } else {
            sfree(on);
            on = NULL;
        }
    }
    return on;
}


Prototype ASM PERROR PutOptions( REG(a0) STRPTR, REG(a1) APTR, REG(d0) ULONG, REG(a6) EXTBASE * );

SAVEDS ASM
PERROR PutOptions( REG(a0) STRPTR name, REG(a1) APTR data,
                   REG(d0) ULONG len, REG(a6) EXTBASE *ExtBase )
{
    PERROR res = PERR_OK;
    struct OptNode *old, *new;
    struct ExecBase *SysBase = ExtBase->lb_Sys;

    D(bug("PutOptions(%s,%08X,%lu)\n",name,data,len));

    if( old = (struct OptNode *)FindName( &optlist,name )) {
        D(bug("Found old at %08X\n",old));
        Remove(old);
        ReleaseOptNode(old,ExtBase);
    }

    if(new = AllocOptNode(name,len,ExtBase)) {
        memcpy(new->on_Data, data, len);
        AddTail(&optlist, (struct Node *)new);
    } else {
        res = PERR_OUTOFMEMORY;
    }

    return res;
}

Prototype ASM APTR GetOptions( REG(a0) STRPTR, REG(a6) EXTBASE *);

SAVEDS ASM
APTR GetOptions( REG(a0) STRPTR name, REG(a6) EXTBASE *ExtBase )
{
    struct OptNode *opt;
    struct ExecBase *SysBase = ExtBase->lb_Sys;

    D(bug("GetOptions(%s)\n",name));

    opt = (struct OptNode *)FindName( &optlist, name );
    if( opt ) {
        D(bug("\tFound opt at %08X\n",opt));
        return opt->on_Data;
    }

    return NULL;
}

Prototype PERROR InitOptions(VOID);

PERROR InitOptions(VOID)
{
    D(bug("InitOptions()\n"));

    NewList( &optlist );

    return PERR_OK;
}

Prototype VOID ExitOptions(VOID);

VOID ExitOptions(VOID)
{
    struct Node *on = optlist.lh_Head, *nn;

    D(bug("ExitOptions()\n"));

    while(nn = on->ln_Succ) {
        D(bug("\tReleasing opt at %08X\n",on));
        ReleaseOptNode( (struct OptNode *)on, globxd );
        on = nn;
    }

}


