/*
    PROJECT: ppt
    MODULE:  extensions.c

    This module contains both extensions and options routines.

    $Id: extensions.c,v 1.3 1997/02/23 18:22:59 jj Exp $
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

struct List optlist, extlist;
struct SignalSemaphore optsemaphore, extsemaphore;

/*-------------------------------------------------------------------*/
/* Extensions */

Local
VOID ReleaseExtension( struct Extension *en, EXTBASE *ExtBase )
{
    sfree( en->en_Name );
    if( en->en_Data ) pfree( en->en_Data );
    sfree( en );
}

Local
struct Extension *AllocExtension( STRPTR name, ULONG size, EXTBASE *ExtBase )
{
    struct Extension *on;

    on = (struct Extension *)smalloc( sizeof(struct Extension) );
    if( on ) {
        on->en_Name = smalloc( strlen(name)+1 );
        if( on->en_Name ) {
            if( on->en_Data = pmalloc( size ) ) {
                strcpy( on->en_Name, name );
                on->en_Node.ln_Name = on->en_Name;
                on->en_Length = size;
            } else {
                sfree( on->en_Name );
                sfree( on );
                on = NULL;
            }
        } else {
            sfree(on);
            on = NULL;
        }
    }
    return on;
}

/****u* pptsupport/AddExtension *******************************************
*
*   NAME
*       AddExtension -- Add one extension to a frame (V3)
*
*   SYNOPSIS
*       success = AddExtension( frame, name, data, len, flags );
*
*       PERROR AddExtension( FRAME *, STRPTR, APTR, ULONG, ULONG )
*       D0                   A0       A1      A2    D0     D1
*
*   FUNCTION
*       Adds a transparent extension to the image.  For example, you
*       could use this to save Author information for an image.
*
*       Extensions are collectively deleted during exiting PPT.  If you
*       specify an existing extension, the old one is unceremoniously
*       deleted.
*
*       By default, this data is assumed to be an editable string. If
*       your extension contains something else than pure ASCII data,
*       then set up EXTF_NOTASTRING.
*
*   INPUTS
*       frame - the frame handle.
*       name - a NUL-terminated string that is an unique identifier
*           for your option chunk.  The name is case-sensitive. PPT
*           understands and uses the following pre-defined names:
*
*           EXTNAME_AUTHOR - A string that contains author data.
*           EXTNAME_ANNO - An annotation string.
*           EXTNAME_DATE - A date string.
*
*       data - pointer to your data.
*       len  - length of your data chunk.
*       flags - flags that describe the format of this data chunk.
*           Possible values are:
*
*           EXTF_NOTASTRING - this extension is not a string.
*
*   RESULT
*       success - PERR_OK if everything went OK and extension was
*           successfully saved in memory, PERR_OUTOFMEMORY, if
*           there was not enough memory to save the extension.
*
*   EXAMPLE
*
*   NOTES
*       This function allocates new space for each data item and
*       copies your data to a safe location.  Please note that it is
*       quite unhealthy to save pointers to your own data.
*
*   BUGS
*       Extensions are not yet used too much.
*       String extensions cannot be edited yet.
*
*   SEE ALSO
*       GetOptions(),PutOptions(),FindExtension()
*
***********************************************************************
*
*
*/

Prototype ASM PERROR AddExtension( REG(a0) FRAME *, REG(a1) STRPTR, REG(a2) APTR, REG(d0) ULONG, REG(d1) ULONG, REG(a6) EXTBASE *);

SAVEDS ASM
PERROR AddExtension( REG(a0) FRAME *frame, REG(a1) STRPTR name, REG(a2) APTR data,
                     REG(d0) ULONG len,    REG(d1) ULONG flags, REG(a6) EXTBASE *ExtBase )
{
    struct Extension *new, *old;
    PERROR res = PERR_OK;
    struct ExecBase *SysBase = ExtBase->lb_Sys;

    D(bug("AddExtension(%08X (=%u),%s,%08X,%lu,%lu)\n",frame,frame->ID,name,data,len,flags));

    ObtainSemaphore( &extsemaphore );

    if( old = FindExtension( frame, name, ExtBase ) ) {
        D(bug("\tremoved old extension @ %08X\n",old));
        Remove( old );
        ReleaseExtension( old, ExtBase );
    }

    if( new = AllocExtension( name, len, ExtBase )) {
        new->en_FrameID = frame->ID;
        new->en_Flags   = flags;
        memcpy( new->en_Data, data, len );
        AddTail( &extlist, (struct Node *)new );
    } else {
        D(bug("\tfailed to alloc!\n"));
        res = PERR_OUTOFMEMORY;
    }

    ReleaseSemaphore( &extsemaphore );
    return res;
}

/****u* pptsupport/FindExtension *******************************************
*
*   NAME
*       FindExtension -- Finds an extension from a frame (V3)
*
*   SYNOPSIS
*       data = FindExtension( frame, name );
*
*       struct Extension *AddExtension( FRAME *, STRPTR )
*       D0                            A0       A1
*
*   FUNCTION
*       Finds a previously set extension from a frame.
*
*   INPUTS
*       frame - the frame handle.
*       name - a NUL-terminated string that is an unique identifier
*           for your option chunk.  The name is case-sensitive. PPT
*           understands and uses the following pre-defined names:
*
*           EXTNAME_AUTHOR - A string that contains author data.
*           EXTNAME_ANNO - An annotation string.
*           EXTNAME_DATE - A date string.
*
*   RESULT
*       data - pointer to the extension node.  Consider everything
*           read only!
*           The information you want is at data->en_Data.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       GetOptions(),PutOptions(),AddExtension()
*
***********************************************************************
*
*
*/

Prototype ASM struct Extension *FindExtension( REG(a0) FRAME *, REG(a1) STRPTR, REG(a6) EXTBASE * );

SAVEDS ASM
struct Extension *FindExtension( REG(a0) FRAME *frame, REG(a1) STRPTR name, REG(a6) EXTBASE *ExtBase )
{
    struct Extension *xn = (struct Extension *)extlist.lh_Head;

    D(bug("FindExtension( id=%d, %s )\n",frame->ID, name ));

    ObtainSemaphoreShared( &extsemaphore );

    while( xn = (struct Extension *)FindName( (struct List *)xn, name ) ) {
        if( frame->ID == xn->en_FrameID ) {
            D(bug("\tFound at %08X\n",xn));
            ReleaseSemaphore( &extsemaphore );
            return xn;
        }
    }

    D(bug("\tNot found!\n"));

    ReleaseSemaphore( &extsemaphore );

    return NULL;
}

/****u* pptsupport/RemoveExtension *******************************************
*
*   NAME
*       RemoveExtension -- Removes an extension from a frame (V3)
*
*   SYNOPSIS
*       success = RemoveExtension( frame, name );
*
*       PERROR RemoveExtension( FRAME *, STRPTR )
*       D0                      A0       A1
*
*   FUNCTION
*       Finds and removes previously set extension from a frame.
*
*   INPUTS
*       frame - the frame handle.
*       name - a NUL-terminated string that is an unique identifier
*           for your option chunk.  The name is case-sensitive. PPT
*           understands and uses the following pre-defined names:
*
*           EXTNAME_AUTHOR - A string that contains author data.
*           EXTNAME_ANNO - An annotation string.
*           EXTNAME_DATE - A date string.
*
*   RESULT
*       success - PERR_OK, if the extension was found and successfully
*           deleted; PERR_UNKNOWNTYPE, if the extension was not found.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AddExtension(),FindExtension()
*
***********************************************************************
*
*
*/

Prototype ASM PERROR RemoveExtension( REG(a0) FRAME *, REG(a1) STRPTR, REG(a6) EXTBASE * );

SAVEDS ASM
PERROR RemoveExtension( REG(a0) FRAME *frame, REG(a1) STRPTR name, REG(a6) EXTBASE *ExtBase )
{
    struct Extension *xn;
    PERROR res = PERR_OK;

    ObtainSemaphore( &extsemaphore );

    if( xn = FindExtension( frame, name, ExtBase ) ) {
        Remove( xn );
        ReleaseExtension( xn, ExtBase );
    } else {
        res = PERR_UNKNOWNTYPE;
    }

    ReleaseSemaphore( &extsemaphore );

    return res;
}


/*-------------------------------------------------------------------*/
/* Options */

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

/****u* pptsupport/PutOptions *******************************************
*
*   NAME
*       PutOptions -- Saves options from an external (V3)
*
*   SYNOPSIS
*       success = PutOptions( name, data, len );
*
*       PERROR PutOptions( STRPTR, APTR, ULONG )
*       D0                 A0      A1    D0
*
*   FUNCTION
*       This function can be used to save a chunk of memory during
*       different invocations of your external.  For example, if
*       you wish to snapshot your window's location, you could
*       make it a structure, and then pass it to this function.
*       Then you can use GetOptions() to retrieve your data.
*
*       The data given is copied, so it is safe to release the
*       container after this call.
*
*       If a previous option chunk with the same name is found,
*       this chunk is replaced with the new one.  No warning
*       is given.
*
*       The data chunks are collectively removed during exiting
*       PPT.
*
*   INPUTS
*       name - a NUL-terminated string that is an unique identifier
*           for your option chunk.  The name is case-sensitive.
*       data - pointer to your data.
*       len  - length of your data chunk.
*
*   RESULT
*       success - PERR_OK if everything went OK and options were
*           successfully saved in memory, PERR_OUTOFMEMORY, if
*           there was not enough memory to save the options.
*
*   EXAMPLE
*       This describes how to save your window location.
*
*       struct IBox windim;
*
*       windim->Left = mywin->LeftEdge;
*       windim->Top  = mywin->TopEdge;
*       windim->Height = mywin->Height;
*       windim->Width = mywin->Width;
*
*       PutOptions("my_external", &windim, sizeof(struct IBox) );
*
*   NOTES
*       This function allocates new space for each data item and
*       copies your data to a safe location.  Please note that it is
*       quite unhealthy to save pointers to your own data.
*
*   BUGS
*       The options are not yet saved on disk.  In the future using
*       this function guarantees that your options are saved on disk
*       and you'll get the same data next time PPT is launched.
*
*   SEE ALSO
*       GetOptions()
*
***********************************************************************
*
*
*/


Prototype ASM PERROR PutOptions( REG(a0) STRPTR, REG(a1) APTR, REG(d0) ULONG, REG(a6) EXTBASE * );

SAVEDS ASM
PERROR PutOptions( REG(a0) STRPTR name, REG(a1) APTR data,
                   REG(d0) ULONG len, REG(a6) EXTBASE *ExtBase )
{
    PERROR res = PERR_OK;
    struct OptNode *old, *new;
    struct ExecBase *SysBase = ExtBase->lb_Sys;

    D(bug("PutOptions(%s,%08X,%lu)\n",name,data,len));

    ObtainSemaphore( &optsemaphore );

    if( old = (struct OptNode *)FindName( &optlist,name )) {
        D(bug("Found old at %08X\n",old));
        Remove(old);
        ReleaseOptNode(old,ExtBase);
    }

    if(new = AllocOptNode(name,len,ExtBase)) {
        memcpy(&(new->on_Data[0]), data, len);
        AddTail(&optlist, (struct Node *)new);
    } else {
        res = PERR_OUTOFMEMORY;
    }

    ReleaseSemaphore( &optsemaphore );

    return res;
}

/****u* pptsupport/GetOptions *******************************************
*
*   NAME
*       GetOptions -- get an option chunk saved by PutOptions().
*
*   SYNOPSIS
*       data = GetOptions( name );
*
*       APTR GetOptions( STRPTR );
*       D0               A0
*
*   FUNCTION
*       Returns a data chunk saved using PutOptions() or NULL,
*       if no such name was found.
*
*   INPUTS
*       name - pointer to a NUL-terminated string containing the
*           (case-sensitive) name for the requested datachunk.
*
*   RESULT
*       data - pointer to a saved chunk or NULL, if not found.
*
*   EXAMPLE
*       struct IBox *ib;
*       struct NewWindow *newwin;
*
*       ib = (struct IBox *)GetOptions("my_external");
*       if( ib ) {
*           newwin->LeftEdge = ib->Left;
*           newwin->TopEdge  = ib->Top;
*           newwin->Height   = ib->Height;
*           newwin->Width    = ib->Width;
*       }
*
*   NOTES
*       This returns a pointer to the internal saved object.
*       Please do not tamper with it, consider it READ ONLY!
*
*   BUGS
*
*   SEE ALSO
*       PutOptions();
*
***********************************************************************
*
*
*/

Prototype ASM APTR GetOptions( REG(a0) STRPTR, REG(a6) EXTBASE *);

SAVEDS ASM
APTR GetOptions( REG(a0) STRPTR name, REG(a6) EXTBASE *ExtBase )
{
    struct OptNode *opt;
    struct ExecBase *SysBase = ExtBase->lb_Sys;

    D(bug("GetOptions(%s)\n",name));

    ObtainSemaphoreShared( &optsemaphore );

    opt = (struct OptNode *)FindName( &optlist, name );
    if( opt ) {
        D(bug("\tFound opt at %08X\n",opt));
        ReleaseSemaphore( &optsemaphore );
        return &(opt->on_Data[0]);
    }

    ReleaseSemaphore( &optsemaphore );

    return NULL;
}

Prototype PERROR InitOptions(VOID);

PERROR InitOptions(VOID)
{
    D(bug("InitOptions()\n"));

    NewList( &optlist );
    NewList( &extlist );

    InitSemaphore( &optsemaphore );
    InitSemaphore( &extsemaphore );

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

    on = extlist.lh_Head;

    while( nn = on->ln_Succ) {
        D(bug("\tReleasing ext at %08X\n",on));
        ReleaseExtension( (struct Extension *)on, globxd );
        on = nn;
    }

}


