/*
    PROJECT: ppt
    MODULE:  external.c

    $Id: external.c,v 1.11 1996/11/17 22:06:36 jj Exp $

    This contains necessary routines to operate on external modules,
    ie loaders and effects.

*/

/*-------------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "gui.h"
#include "misc.h"


#ifndef CLIB_UTILITY_PROTOS_H
#include <clib/utility_protos.h>
#endif

#ifndef PRAGMAS_INTUITION_PRAGMAS_H
#include <pragmas/intuition_pragmas.h>
#endif

#include <proto/locale.h>

#include "proto/module.h"
#include "proto/effect.h"


/*-------------------------------------------------------------------------*/
/* Prototypes*/

Prototype int           AddExtEntries( EXTBASE *, struct Window *, Object *, UBYTE, ULONG );
Prototype void          ShowExtInfo( EXTBASE *, EXTERNAL *, struct Window * );
Prototype int           PurgeExternal( EXTERNAL *, BOOL );
Prototype PERROR        OpenExternal( const char *, UBYTE );
Prototype void          RelExtBase( EXTBASE * );
Prototype EXTBASE       *NewExtBase( BOOL );

/*-------------------------------------------------------------------------*/

#define EXTSIZE (LIB_VECTSIZE * XLIB_FUNCS + sizeof(EXTBASE))

/*-------------------------------------------------------------------------*/
/* Code */

/*
    These two will open and close an external module.
*/

Prototype struct Library *OpenModule( EXTERNAL *, EXTBASE * );

struct Library *OpenModule( EXTERNAL *x, EXTBASE *ExtBase )
{
    char buf[256];
    struct Library *ModuleBase;
    struct DosLibrary *DOSBase = ExtBase->lb_DOS;
    struct ExecBase   *SysBase = ExtBase->lb_Sys;

    // BUG:  should use globals->userprefs->modulepath

    strcpy( buf, globals->userprefs->modulepath );

    SHLOCKGLOB();
    AddPart( buf, x->diskname, 255 );
    UNLOCKGLOB();

    ModuleBase = OpenLibrary( buf, 0L );
    return ModuleBase;
}

/*
 *  Flushes a given library from memory by calling its Expunge-vector.
 *  We can't use the base given by OpenModule because it may be different
 *  for each module.
 */

Prototype void FlushLibrary(STRPTR, EXTBASE *);

void FlushLibrary(STRPTR name, EXTBASE *ExtBase)
{
    struct Library *result;
    struct ExecBase *SysBase = ExtBase->lb_Sys;

    D(bug("\tFlushing %s...\n", name ));

    Forbid();
    if(result=(struct Library *)FindName(&SysBase->LibList,name))
        RemLibrary(result);
    Permit();
}

/*
 *  This closes the module and expunges it from memory, if necessary.
 */

Prototype VOID CloseModule( struct Library *, EXTBASE * );

VOID CloseModule( struct Library *ModuleBase, EXTBASE *ExtBase)
{
    struct ExecBase *SysBase = ExtBase->lb_Sys;
    char buf[40];

    if( ModuleBase ) {
        strncpy( buf, ModuleBase->lib_Node.ln_Name, 39 );
        CloseLibrary( ModuleBase );

        if( globals->userprefs->expungelibs ) {
            FlushLibrary( buf, ExtBase );
        }
    }
}

/*
    This function goes through all external entries, adding them to the listview object
    given. type controls the list to be selected. Returns the # of items added.
    This routine is re-entrant.

    Flags: AEE_*

    BUG: Should use some other method of adding entries... now creates a global reference
*/


SAVEDS int AddExtEntries( EXTBASE *xd, struct Window *win, Object *lv, UBYTE type, ULONG flags )
{
    struct Node *cn;
    int count = 0;
    struct Library *UtilityBase = xd->lb_Utility;

    SHLOCKGLOB();
    switch(type) {
        case NT_EFFECT:
            cn = globals->effects.lh_Head;
            break;
        case NT_LOADER:
            cn = globals->loaders.lh_Head;
            break;
        case NT_SCRIPT:
            cn = globals->scripts.lh_Head;
            break;
        default:
            return 0; /* Unknown node type */
    }

    for(; cn->ln_Succ; cn = cn->ln_Succ ) {
        EXTERNAL *ext;
        BOOL add;

        ext = (EXTERNAL *)cn;
        add = FALSE;

        switch( type ) {
            case NT_LOADER:
                if( flags & AEE_SAVECM && !add) {
                    if( GetTagData(PPTX_SaveColorMapped, NULL, ext->tags))
                        add = TRUE;
                }

                if( flags & AEE_SAVETC && !add) {
                    if( GetTagData(PPTX_SaveTrueColor, NULL, ext->tags))
                        add = TRUE;
                }

                if( flags & AEE_LOAD && !add) {
                    if( GetTagData(PPTX_Load, NULL, ext->tags))
                        add = TRUE;
                }

                break;

            case NT_EFFECT:
                add = TRUE; /* These are always added */
                break;

            case NT_SCRIPT:
                add = TRUE;
                break;
        }

        if(add) {
            AddEntry( NULL,lv,cn->ln_Name,LVAP_TAIL );
            count++;
        }
    }

    UNLOCKGLOB();
    SortList( NULL,lv );
    RefreshList(win,lv);
    return count;
}

/*
    This function shows info about a given external. The parent window
    is win. If win == NULL, then uses main PPT window.
    This routine is re-entrant.
*/

SAVEDS VOID ShowOldExtInfo( EXTBASE *xd, EXTERNAL *x, struct Window *win )
{
    int   ver,rev;
    APTR txt, au;

    SHLOCKGLOB();
    txt  = (APTR)GetTagData( PPTX_InfoTxt, NULL, x->tags );
    au   = (APTR)GetTagData( PPTX_Author, NULL, x->tags );
    ver  = GetTagData( PPTX_Version, -1, x->tags );
    rev  = GetTagData( PPTX_Revision, -1, x->tags );
    UNLOCKGLOB();

    Req(win,NULL, ISEQ_C ISEQ_B "%s V.%ld.%ld\n\n"ISEQ_N"%s\n" ISEQ_N ISEQ_C ISEQ_I "Author: %s\n",
        x->nd.ln_Name, (ULONG)ver, (ULONG)rev, txt ? txt : "", au ? au : "Unknown" );

}

SAVEDS VOID ShowNewExtInfo( EXTBASE *xd, EXTERNAL *x, struct Window *win )
{
    struct Library *ModuleBase;
    int   ver,rev;
    APTR  txt, au;

    D(bug("ShowNewExtInfo()\n"));

    ModuleBase = OpenModule( x, xd ); // BUG!
    if(!ModuleBase) return;

    txt  = (APTR)Inquire( PPTX_InfoTxt, xd );
    au   = (APTR)Inquire( PPTX_Author, xd );
    ver  = ModuleBase->lib_Version;
    rev  = ModuleBase->lib_Revision;

    Req(win,NULL, ISEQ_C ISEQ_B "%s V.%ld.%ld\n\n"ISEQ_N"%s\n" ISEQ_N ISEQ_C ISEQ_I "Author: %s\n",
        x->nd.ln_Name, (ULONG)ver, (ULONG)rev, txt ? txt : "", au ? au : "Unknown" );

    CloseModule( ModuleBase, xd );
}

/*
    An useless stub, I hope, in the future.
*/

VOID ShowExtInfo( EXTBASE *xd, EXTERNAL *x, struct Window *win )
{
    if( x->nd.ln_Type == NT_SCRIPT ) {
        Req(NEGNUL,NULL,"Scripts not supported yet");
    } else {
        if(x->islibrary)
            ShowNewExtInfo( xd, x, win );
        else
            ShowOldExtInfo( xd, x, win );
    }
}


/*
    This routine kills the given external from memory. If the external is in
    use, however, PERR_INUSE is returned. If force == TRUE, then the
    purge is made even if the external is in use.
 */

PERROR PurgeOldExternal( EXTERNAL *who, BOOL force )
{
    static PERROR (* ASM X_Purge)( REG(a6) EXTBASE * );

    if(who->usecount && force == FALSE) /* If someone is using us, don't release */
        return PERR_INUSE;

    /* First, make sure no-one can find us. */

    LOCKGLOB();
    Remove( (struct Node *)who );
    UNLOCKGLOB();

    /* Execute release code, if any */

    X_Purge   = (FPTR)GetTagData( PPTX_Purge,(ULONG)NULL,who->tags);
    if(X_Purge)
        (*X_Purge)( globxd );

    UnLoadSeg(who->seglist);

    sfree(who);

    return PERR_OK;
}

PERROR PurgeNewExternal( EXTERNAL *who, BOOL force )
{
    if(who->usecount && force == FALSE) /* If someone is using us, don't release */
        return PERR_INUSE;

    /* Make sure no-one can find us anymore */

    LOCKGLOB();
    Remove( (struct Node *)who );
    UNLOCKGLOB();

    if( who->nd.ln_Type != NT_SCRIPT ) {
        if( globals->userprefs->expungelibs )
            FlushLibrary( who->diskname, globxd );
    }

    sfree(who);
}

PERROR PurgeExternal( EXTERNAL *w, BOOL f )
{
    if( w->islibrary || w->nd.ln_Type == NT_SCRIPT )
        return PurgeNewExternal( w, f );
    else
        return PurgeOldExternal( w, f );
}


/*
    This routines opens the given external and then executes
    it's Init() - routine. If it is successful, the external
    is then added to the main list.
*/

PERROR InitOldExternal( const char *who )
{
    struct ModuleInfo *m;
    BPTR seglist;
    int version,revision,res = PERR_OK, type;
    EXTERNAL *x;
    char *name;
    static PERROR (* ASM X_Init)( REG(a6) EXTBASE * );
    UWORD kickver, pptver;

//    DEBUG("OpenExternal()\n");
    seglist = NewLoadSeg( who, NULL );
    if(!seglist) {
        D(bug("Unable to open\n"));
        Req(NEGNUL,NULL,ISEQ_C"Could not load '%s':\nIt probably is not object code...\n",who);
        return PERR_WONTOPEN;
    }

    m = (struct ModuleInfo *) BADDR(seglist); /* Make to std pointer */

    // DumpMem( (ULONG)m,32L);

    switch(m->id) {
        case ID_LOADER:
            type = NT_LOADER;
            break;
        case ID_EFFECT:
            type = NT_EFFECT;
            break;
        default:
            Req(NEGNUL,NULL,"'%s' is not a recognised module!",who);
            return PERR_UNKNOWNTYPE;
    }

    // DumpMem( (ULONG)(m->tagarray),64L);
    version  = GetTagData( PPTX_Version, ~0, m->tagarray);
    revision = GetTagData( PPTX_Revision,~0, m->tagarray);
    X_Init   = (FPTR)GetTagData( PPTX_Init,(ULONG)NULL,m->tagarray);
    name     = (char *)GetTagData( PPTX_Name, NULL, m->tagarray);
    if(!name)
        goto nogood;

    kickver  = (UWORD)GetTagData( PPTX_ReqKickVersion, 0, m->tagarray);
    pptver   = (UWORD)GetTagData( PPTX_ReqPPTVersion, 0, m->tagarray);

//    DEBUG("'%s' version:%d.%d.\n",name,version,revision);

    if( kickver > SysBase->LibNode.lib_Version) {
        Req(NEGNUL,NULL,"External %s requires OS %d+",name,kickver);
        goto nogood;
    }

    if( pptver > VERNUM ) {
        Req(NEGNUL,NULL,"External %s requires PPT version %d+",name,pptver);
        goto nogood;
    }

//    DEBUG("Calling X_Init() @ %X\n",X_Init);
    if(X_Init)
        res = (*X_Init)(globxd);

//    DEBUG("INIT DONE\n");

    if(res == PERR_OK || X_Init == NULL) { /* No error occurred, or X_Init() does not exist. */
        x = smalloc( type == NT_LOADER ? sizeof(LOADER) : sizeof(EFFECT) );
        x->seglist    = seglist;
        x->tags       = m->tagarray;
        x->islibrary  = FALSE;
        x->usecount   = 0;
        x->nd.ln_Type = type;
        x->nd.ln_Name = name;
        x->nd.ln_Pri  = (BYTE)GetTagData( PPTX_Priority, 0L, m->tagarray );
        strncpy( x->diskname, FilePart(who), 39 );

        LOCKGLOB();
        if(type == NT_LOADER)
            Enqueue( &globals->loaders, (struct Node *)x );
        else {
            Enqueue( &globals->effects, (struct Node *)x );
        }
        UNLOCKGLOB();
    } else {
    nogood:
        UnLoadSeg( seglist );
        return PERR_INITFAILED;
    }

    return PERR_OK;
}

/*
    Initializes a script.
*/

PERROR InitScript( const char *who )
{
    EXTERNAL *x;

    x = smalloc( sizeof(SCRIPT) );
    x->seglist    = 0L;
    x->tags       = NULL;
    x->islibrary  = FALSE;
    x->usecount   = 0;
    x->nd.ln_Type = NT_SCRIPT;
    x->nd.ln_Name = x->realname;
    x->nd.ln_Pri  = 0;
    strncpy( x->diskname, who, 39 );
    strncpy( x->realname, FilePart(who), 39 );

    LOCKGLOB();
    Enqueue( &globals->scripts, (struct Node *)x );
    UNLOCKGLOB();

    D(bug("\tOpened script OK.\n"));

    return PERR_OK;
}

/*
    Open new style external
*/
PERROR InitNewExternal( const char *who )
{
    struct Library *ModuleBase = NULL;
    UWORD pptver;
    PERROR res = PERR_OK;
    STRPTR name;
    UBYTE type;
    EXTERNAL *x;

    D(bug("NewOpenExternal(%s)\n",who));

    ModuleBase = OpenLibrary( who,0L );
    if(!ModuleBase) {
#if 0
        D(bug("\tFailed to open library!\n"));
        Req(NEGNUL,NULL,
            ISEQ_C"Could not open '%s':\n"
            "It is probably not a proper module!\n",
            who);
        return PERR_WONTOPEN;
#else
        D(bug("\tPassing to InitOldExternal()...\n"));
        return( InitOldExternal( who ) );
#endif
    }

    /*
     *  Determine name
     */

    name = (STRPTR) Inquire( PPTX_Name, globxd );

    /*
     *  Determine whether we can use this or not.
     */

    pptver = (UWORD) Inquire( PPTX_ReqPPTVersion,  globxd );

    if( pptver > VERNUM ) {
        Req(NEGNUL,NULL,"External %s requires PPT version %d+",name,pptver);
        res = PERR_WONTOPEN;
        goto nogood;
    }

    /*
     *  Determine type.  A kludge, at best.  Should probably use
     *  Inquire() somehow.
     */

    if( strcmp( &who[strlen(who)-7], ".effect" ) == 0 )
        type = NT_EFFECT;
    else
        type = NT_LOADER;

    /*
     *  Allocate room and put the necessary info into memory.
     */

    x = smalloc( type == NT_LOADER ? sizeof(LOADER) : sizeof(EFFECT) );
    x->seglist    = 0L;
    x->tags       = NULL;
    x->islibrary  = TRUE;
    x->usecount   = 0;
    x->nd.ln_Type = type;
    x->nd.ln_Name = x->realname;
    x->nd.ln_Pri  = (BYTE)Inquire( PPTX_Priority, globxd );
    strncpy( x->diskname, FilePart(who), 39 );
    strncpy( x->realname, name, 39 );

    LOCKGLOB();
    if(type == NT_LOADER)
        Enqueue( &globals->loaders, (struct Node *)x );
    else {
        Enqueue( &globals->effects, (struct Node *)x );
    }
    UNLOCKGLOB();

    D(bug("\tOpened library OK.\n"));

nogood:
    if(ModuleBase) CloseModule( ModuleBase, globxd );

    return res;
}

PERROR OpenExternal( const char *who, UBYTE type )
{
    if( type == NT_SCRIPT )
        return InitScript( who );

    return InitNewExternal( who );
}


/*---------------------------------------------------------------------------*/

/*
    Opposite of OpenLibBases().
    Note: This routine has no bugs.
*/

SAVEDS ASM VOID CloseLibBases( REG(a6) EXTBASE *xd )
{
    struct Library *LocaleBase;
    struct ExecBase *SysBase;

    SysBase = (struct ExecBase *)SYSBASE();
    LocaleBase = xd->lb_Locale;

    // D(bug("\tCloseLibBases()\n"));

    ClosepptCatalog( xd );
    if(xd->locale)      CloseLocale(xd->locale);

    if(xd->lb_Timer)    {
        CloseDevice( xd->TimerIO );
        DeleteIORequest( xd->TimerIO );
    }

    if(xd->lb_Locale)   CloseLibrary(xd->lb_Locale);
    if(xd->lb_BGUI)     CloseLibrary(xd->lb_BGUI);
    if(xd->lb_Gfx)      CloseLibrary( (struct Library *)xd->lb_Gfx);
    if(xd->lb_Utility)  CloseLibrary(xd->lb_Utility);
    if(xd->lb_Intuition) CloseLibrary( (struct Library *)xd->lb_Intuition);
    if(xd->lb_DOS)      CloseLibrary( (struct Library *)xd->lb_DOS);
    if(xd->lb_GadTools) CloseLibrary(xd->lb_GadTools);
    if(xd->mport)       DeleteMsgPort(xd->mport); /* Exec call. Safe to do. */
}


/*
    Opens up libraries:

    BUG: maybe use a flag system to tell which libs to open?
*/

SAVEDS ASM PERROR OpenLibBases( REG(a6) EXTBASE *xd )
{
    struct ExecBase *SysBase;
    struct Library  *LocaleBase;

    // D(bug("\tOpenLibBases()\n"));

    SysBase = (struct ExecBase *)SYSBASE();

    xd->lib.lib_Version = VERNUM;
    xd->g =             globals;
    xd->mport =         CreateMsgPort(); /* Exec call. Safe to do. */
    xd->lb_Sys =        SysBase;
    xd->lb_DOS =        (struct DosLibrary *)OpenLibrary(DOSNAME,37L);
    xd->lb_Utility =    OpenLibrary("utility.library",37L);
    xd->lb_Intuition =  (struct IntuitionBase *)OpenLibrary("intuition.library",37L);
    // D(bug("\tOpening BGUI..."));
    xd->lb_BGUI =       OpenLibrary(BGUINAME,37L);
    // D(bug("done\n"));
    xd->lb_Gfx =        (struct GfxBase *)OpenLibrary("graphics.library",37L);
    xd->lb_GadTools =   OpenLibrary("gadtools.library",37L);

    if( xd->TimerIO = CreateIORequest( xd->mport, sizeof( struct timerequest ) ) ) {
        OpenDevice("timer.device", UNIT_ECLOCK, xd->TimerIO, 0L );
        xd->lb_Timer = xd->TimerIO->tr_node.io_Device;
    }

    LocaleBase = xd->lb_Locale = OpenLibrary("locale.library",0L);

    if( xd->lb_Locale ) {
        xd->locale  = OpenLocale( NULL );
        OpenpptCatalog( NULL, NULL, xd );
#if 0
        if( !xd->catalog ) {
            D(bug("\tCouldn't open ppt.catalog!\n"));
        }
#endif
    }

    if(!xd->lb_GadTools || !xd->lb_DOS || !xd->lb_Utility ||
       !xd->lb_Intuition || !xd->lb_BGUI || !xd->lb_Gfx)
    {
        CloseLibBases(xd);
        return PERR_GENERAL;
    }

    return PERR_OK;
}


/*
    Allocates a new ExtBase structure.
    if open == TRUE, calls OpenLibBases to allocate new library bases.
*/

SAVEDS EXTBASE *NewExtBase( BOOL open )
{
    EXTBASE *ExtBase = NULL;
    APTR    realptr;
    extern  APTR ExtLibData[];
    APTR    SysBase = SYSBASE();

    D(bug("NewExtBase(%d). Allocating %lu bytes...\n",open,EXTSIZE));

    realptr = pzmalloc( EXTSIZE );

    if(realptr) {
        ExtBase = (EXTBASE *)( (ULONG)realptr + (EXTSIZE - sizeof(EXTBASE)));
        bzero( realptr, EXTSIZE );

        // D(bug("\trealptr = %08X, ExtBase = %08X\n",realptr,ExtBase));

        if(open) {
            if(OpenLibBases( ExtBase ) != PERR_OK) {
                pfree( ExtBase );
                return NULL;
            }
            ExtBase->opened = TRUE;
        }

        // D(bug("\tCreating library jump table...\n"));

        MakeFunctions( ExtBase, ExtLibData, NULL );

        // D(bug("\tdone\n"));
    }
    return ExtBase;
}

/*
    Use to release ExtBase allocated in NewExtBase()
*/

SAVEDS VOID RelExtBase( EXTBASE *xb )
{
    APTR realptr;

    if(xb->opened)
        CloseLibBases(xb);

    D(bug("Releasing ExtBase @ %08X, realptr = %08X\n", xb, (ULONG)xb - (EXTSIZE - sizeof(EXTBASE)) ));
    realptr = (APTR) ((ULONG)xb - (EXTSIZE - sizeof(EXTBASE)));
    pfree(realptr);
}


