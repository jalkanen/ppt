/*
    PROJECT: ppt
    MODULE:  extensions.c

    This module contains both extensions and options routines.

    $Id: extensions.c,v 1.9 1999/10/02 16:33:07 jj Exp $
*/

#include "defs.h"
#include "misc.h"
#include "gui.h"

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
/* Extensions editing */

Prototype EDITWIN *AllocEditWindow( FRAME *frame );

EDITWIN *AllocEditWindow( FRAME *frame )
{
    EDITWIN *ew;

    if(ew = pmalloc( sizeof(EDITWIN) )) {
        bzero(ew, sizeof(EDITWIN) );
        ew->frame = frame;
        frame->editwin = ew;
    }

    return ew;
}

Prototype VOID FreeEditWindow( EDITWIN *ew );

VOID FreeEditWindow( EDITWIN *ew )
{
    if(ew) {
        if(ew->frame) ew->frame->editwin = NULL;
        pfree( ew );
    }
}

Prototype PERROR GimmeEditWindow( EDITWIN *ew );

PERROR GimmeEditWindow( EDITWIN *ew )
{
    PERROR res = PERR_OK;

    D(bug("GimmeEditWindow( %08X )\n",ew));

    ew->Win = WindowObject,
        WINDOW_MenuStrip,   PPTMenus,
        WINDOW_SharedPort,  MainIDCMPPort,
        WINDOW_Font,        globals->userprefs->mainfont,
        WINDOW_Screen,      MAINSCR,
        WINDOW_ScaleWidth,  25,
        WINDOW_MasterGroup,
            VGroupObject, NormalVOffset, NormalHOffset, NormalSpacing,
                StartMember,
                    HGroupObject, NormalSpacing,
                        StartMember,
                            ew->ExtList = ListviewObject,
                                GA_ID, GID_EDIT_EXTLIST,
                                Label("Extensions"), Place(PLACE_ABOVE),
                                BT_HelpNode,"PPT.guide/Extensions",
                                BT_HelpHook,&HelpHook,
                            EndObject, Weight(DEFAULT_WEIGHT/2),
                        EndMember,
                        StartMember,
                            VGroupObject, NormalSpacing, NormalVOffset, NormalHOffset,
                                VarSpace(50),
                                StartMember,
                                    VGroupObject, NormalSpacing, NormalVOffset, NormalHOffset,
                                        FrameTitle("Edit Extension"),
                                        DefaultFrame,
                                        StartMember,
                                            ew->ExtName = StringObject,
                                                GA_ID, GID_EDIT_EXTNAME,
                                                Label("Name"),
                                                GA_Disabled, TRUE,
                                                STRINGA_MaxChars, 40,
                                                GA_TabCycle, TRUE,
                                                BT_HelpNode,"PPT.guide/Extensions",
                                                BT_HelpHook,&HelpHook,
                                            EndObject,  FixMinHeight,
                                        EndMember,
                                        StartMember,
                                            ew->ExtValue = StringObject,
                                                GA_ID, GID_EDIT_EXTVALUE,
                                                Label("Text"),
                                                GA_Disabled, TRUE,
                                                STRINGA_MaxChars, 255,
                                                GA_TabCycle, TRUE,
                                                BT_HelpNode,"PPT.guide/Extensions",
                                                BT_HelpHook,&HelpHook,
                                            EndObject, FixMinHeight,
                                        EndMember,
                                        StartMember,
                                            HGroupObject, NormalSpacing,
                                                StartMember,
                                                    ew->ExtOk = GenericDButton( "Add", GID_EDIT_EXTOK ),
                                                EndMember,
                                                StartMember,
                                                    ew->ExtNew = GenericButton( "New", GID_EDIT_EXTNEW ),
                                                EndMember,
                                                StartMember,
                                                    ew->ExtRemove = GenericDButton( "Remove", GID_EDIT_EXTREMOVE ),
                                                EndMember,
                                            EndObject, FixMinHeight,
                                        EndMember,
                                    EndObject,
                                EndMember,
                                VarSpace(50),

                            EndObject,
                        EndMember,
                    EndObject,
                EndMember,
                StartMember,
                    HorizSeparator,
                EndMember,
                StartMember,
                    HGroupObject, WideSpacing, NarrowVOffset, NarrowHOffset,
                        VarSpace(50),
                        StartMember,
                            GenericButton( GetStr(MSG_OK_GAD), GID_EDIT_OK ),
                        EndMember,
                        VarSpace(50),
#if 0
                        StartMember,
                            GenericButton( GetStr(MSG_CANCEL_GAD), GID_EDIT_CANCEL ),
                        EndMember,
#endif
                    EndObject, FixMinHeight,
                EndMember,
            EndObject,
    EndObject;

    if(ew->Win) {
        struct Node *cn;

        UpdateFrameInfo( ew->frame ); /* Update window title */
        RefreshFrameInfo( ew->frame, globxd );
        /*
         *  Add extensions text
         */

        for( cn = extlist.lh_Head; cn->ln_Succ; cn = cn->ln_Succ ) {
            DoMethod( ew->ExtList, LVM_ADDSINGLE, NULL, cn->ln_Name, LVAP_TAIL, 0L );
            // AddEntry( NULL, ew->ExtList, cn->ln_Name, LVAP_TAIL );
        }

        DoMethod( ew->ExtList, LVM_SORT, 0L );
        // SortList( NULL, ew->ExtList );

    } else {
        D(bug("Failed to open edit win\n"));
        res = PERR_WINDOWOPEN;
    }

    return res;
}

Prototype VOID DeleteEditWindow( EDITWIN *ew );

VOID DeleteEditWindow( EDITWIN *ew )
{
    if( ew->Win ) {
        DisposeObject(ew->Win);
        ew->Win = NULL;
    }
}

/*
 *  Is protected agains multiple openings
 */

Prototype PERROR OpenEditWindow( EDITWIN *ew );

PERROR OpenEditWindow( EDITWIN *ew )
{
    D(bug("OpenEditWindow(%08X)\n",ew));

    if( ew->win == NULL ) {
        if( ew->win = WindowOpen( ew->Win ) ) {
            return PERR_OK;
        } else {
            D(bug("\tCouldn't open window!\n"));
            return PERR_WINDOWOPEN;
        }
    }
    return PERR_OK;
}

Prototype VOID CloseEditWindow( EDITWIN *ew );

VOID CloseEditWindow( EDITWIN *ew )
{
    if( ew->win ) {
        WindowClose( ew->Win );
        ew->win = NULL;
    }
}

/*
    Removes all special \xxx markers
 */
Prototype VOID ConvertCToExtension( UBYTE *c, UBYTE *e, ULONG len );

VOID ConvertCToExtension( UBYTE *c, UBYTE *e, ULONG len )
{
    int i = 0, j = 0;

    D(bug("ConvertCToExtension( '%s', %08x, %d )\n",c,e,len));

    while(c[i] && j < len-1) {
        if(c[i] == '\\') {
            switch(c[++i]) {
              case 'n':
                e[j++] = 10;
                ++i;
                break;
              case '\\':
                e[j++] = '\\';
                ++i;
                break;
              case 'r':
                ++i;
                break;
              default:
                break;
            }
        } else {
            e[j++] = c[i++];
        }
    }
    e[j] = '\0';
    D(bug("\tresult='%s'\n",e));
}


Prototype VOID ConvertExtensionToC( UBYTE *e, UBYTE *c, ULONG len );

VOID ConvertExtensionToC( UBYTE *e, UBYTE *c, ULONG len )
{
    int i = 0, j = 0;

    D(bug("ConvertExtensionToC( '%s', %08x, %d )\n",e,c,len));

    while(e[i] && j < len-1) {
        switch(e[i]) {
            case 10:
                if( j < len-3 ) {
                    c[j++] = '\\';
                    c[j++] = 'n';
                }
                break;
            case '\\':
                if( j < len-2 ) {
                    c[j++] = '\\';
                    c[j++] = '\\';
                }
                break;
            default:
                c[j++] = e[i];
                break;
        }
        i++;
    }
    c[j] = '\0';
    D(bug("\tresult='%s'\n",c));
}

/// Extensions
/*-------------------------------------------------------------------*/
/* Extensions */

Local
VOID ReleaseExtension( struct Extension *en, EXTBASE *PPTBase )
{
    sfree( en->en_Name );
    if( en->en_Data ) pfree( en->en_Data );
    sfree( en );
}

Local
struct Extension *AllocExtension( STRPTR name, ULONG size, EXTBASE *PPTBase )
{
    struct Extension *on;

    on = (struct Extension *)smalloc( sizeof(struct Extension) );
    if( on ) {
        bzero( on, sizeof(struct Extension) );
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

Prototype PERROR ReplaceExtensionName( struct Extension *en, STRPTR newname, EXTBASE *PPTBase );

PERROR ReplaceExtensionName( struct Extension *en, STRPTR newname, EXTBASE *PPTBase )
{
    PERROR res = PERR_OK;

    if( en->en_Name ) sfree( en->en_Name );

    if( en->en_Name = smalloc( strlen(newname)+1 ) ) {
        strcpy(en->en_Name, newname );
        en->en_Node.ln_Name = en->en_Name;
    } else {
        /* BUG: should probably retain old name */
        res = PERR_OUTOFMEMORY;
    }

    return res;
}


Prototype PERROR ReplaceExtensionData( struct Extension *en, APTR newdata, ULONG size, EXTBASE *PPTBase );

PERROR ReplaceExtensionData( struct Extension *en, APTR newdata, ULONG size, EXTBASE *PPTBase )
{
    PERROR res = PERR_OK;

    if( en->en_Data ) pfree( en->en_Data );
    if(en->en_Data = pmalloc( size )) {
        en->en_Length = size;
        memcpy( en->en_Data, newdata, size );
    } else {
        /* BUG: Should probably retain old data */
        res = PERR_OUTOFMEMORY;
    }
    return res;
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
*           Note that if you use any of these, you should also set
*           the EXTF_CSTRING flag...
*
*       data - pointer to your data.
*       len  - length of your data chunk.
*       flags - flags that describe the format of this data chunk.
*           Possible values are:
*
*           EXTF_PRIVATE - The format is private and PPT will not
*               even try to guess it.
*           EXTF_CSTRING - this extension is a standard C format
*               string with a NUL at the end.  PPT will allow editing
*               this kind of extension in the edit window.
*
*           As always, all unused bits should be set to zero.
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
*
*   SEE ALSO
*       GetOptions(),PutOptions(),FindExtension()
*
***********************************************************************
*
*
*/

Prototype PERROR ASM AddExtension( REGDECL(a0,FRAME *), REGDECL(a1,STRPTR), REGDECL(a2,APTR), REGDECL(d0,ULONG), REGDECL(d1,ULONG), REGDECL(a6,EXTBASE *) );

SAVEDS ASM
PERROR AddExtension( REG(a0) FRAME *frame, REG(a1) STRPTR name, REG(a2) APTR data,
                     REG(d0) ULONG len,    REG(d1) ULONG flags, REG(a6) EXTBASE *PPTBase )
{
    struct Extension *new, *old;
    PERROR res = PERR_OK;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    D(bug("AddExtension(%08X (=%u),'%s',%08X,%lu,%lu)\n",frame,frame->ID,name,data,len,flags));

    ObtainSemaphore( &extsemaphore );

    if( old = FindExtension( frame, name, PPTBase ) ) {
        D(bug("\tremoved old extension @ %08X\n",old));
        Remove( old );
        ReleaseExtension( old, PPTBase );
    }

    if( new = AllocExtension( name, len, PPTBase )) {
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

Prototype struct Extension * ASM FindExtension( REGDECL(a0,FRAME *), REGDECL(a1,STRPTR), REGDECL(a6,EXTBASE *) );

SAVEDS ASM
struct Extension *FindExtension( REG(a0) FRAME *frame, REG(a1) STRPTR name, REG(a6) EXTBASE *PPTBase )
{
    struct Extension *xn = (struct Extension *) &extlist;

    D(bug("FindExtension( id=%d, '%s' )\n",frame->ID, name ));

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

Prototype PERROR ASM RemoveExtension( REGDECL(a0,FRAME *), REGDECL(a1,STRPTR), REGDECL(a6,EXTBASE *) );

SAVEDS ASM
PERROR RemoveExtension( REG(a0) FRAME *frame, REG(a1) STRPTR name, REG(a6) EXTBASE *PPTBase )
{
    struct Extension *xn;
    PERROR res = PERR_OK;

    ObtainSemaphore( &extsemaphore );

    if( xn = FindExtension( frame, name, PPTBase ) ) {
        Remove( xn );
        ReleaseExtension( xn, PPTBase );
    } else {
        res = PERR_UNKNOWNTYPE;
    }

    ReleaseSemaphore( &extsemaphore );

    return res;
}
///

/// Options
/*-------------------------------------------------------------------*/
/* Options */

Local
VOID ReleaseOptNode( struct OptNode *on, EXTBASE *PPTBase )
{
    sfree( on->on_Name );
    sfree( on );
}

Local
struct OptNode *AllocOptNode( STRPTR name, ULONG size, EXTBASE *PPTBase )
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


Prototype PERROR ASM PutOptions( REG(a0) STRPTR, REG(a1) APTR, REG(d0) ULONG, REG(a6) EXTBASE * );

SAVEDS ASM
PERROR PutOptions( REG(a0) STRPTR name, REG(a1) APTR data,
                   REG(d0) ULONG len, REG(a6) EXTBASE *PPTBase )
{
    PERROR res = PERR_OK;
    struct OptNode *old, *new;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    D(bug("PutOptions(%s,%08X,%lu)\n",name,data,len));

    ObtainSemaphore( &optsemaphore );

    if( old = (struct OptNode *)FindName( &optlist,name )) {
        D(bug("Found old at %08X\n",old));
        Remove(old);
        ReleaseOptNode(old,PPTBase);
    }

    if(new = AllocOptNode(name,len,PPTBase)) {
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

Prototype APTR ASM GetOptions( REG(a0) STRPTR, REG(a6) EXTBASE *);

SAVEDS ASM
APTR GetOptions( REG(a0) STRPTR name, REG(a6) EXTBASE *PPTBase )
{
    struct OptNode *opt;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

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
///

