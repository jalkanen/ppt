/*
    PROJECT: ppt
    MODULE:  external.c

    $Id: external.c,v 1.2 1995/09/06 23:35:32 jj Exp $

    This contains necessary routines to operate on external modules,
    ie loaders and effects.

*/

/*-------------------------------------------------------------------------*/
/* Includes */

#include <defs.h>
#include <gui.h>
#include <misc.h>


#ifndef CLIB_UTILITY_PROTOS_H
#include <clib/utility_protos.h>
#endif

#ifndef PRAGMAS_INTUITION_PRAGMAS_H
#include <pragma/intuition_pragmas.h>
#endif



/*-------------------------------------------------------------------------*/
/* Prototypes*/

Prototype int           AddExtEntries( EXTDATA *, struct Window *, Object *, UBYTE, ULONG );
Prototype void          ShowExtInfo( EXTDATA *, EXTERNAL *, struct Window * );
Prototype int           PurgeExternal( EXTERNAL *, BOOL );
Prototype int           OpenExternal( const char * );
Prototype void          RelExtBase( EXTBASE * );
Prototype EXTBASE       *NewExtBase( BOOL );

/*-------------------------------------------------------------------------*/

#define EXTSIZE (LIB_VECTSIZE * XLIB_FUNCS + sizeof(EXTBASE))

/*-------------------------------------------------------------------------*/
/* Code */

/*
    This function goes through all external entries, adding them to the listview object
    given. type controls the list to be selected. Returns the # of items added.
    This routine is re-entrant.

    Flags: AEE_SaversOnly : Add just those loaders able to save.

    BUG: Should use some other method of adding entries... now creates a global reference
*/


__geta4 int AddExtEntries( EXTDATA *xd, struct Window *win, Object *lv, UBYTE type, ULONG flags )
{
    struct Node *cn;
    int count = 0;
    struct Library *IntuitionBase = xd->lb_Intuition, *UtilityBase = xd->lb_Utility;

    SHLOCKGLOB();
    switch(type) {
        case NT_EFFECT:
            cn = globals->effects.lh_Head;
            break;
        case NT_LOADER:
            cn = globals->loaders.lh_Head;
            break;
        default:
            return 0; /* Unknown node type */
    }

    for(; cn->ln_Succ; cn = cn->ln_Succ ) {
        if( flags & AEE_SaversOnly ) {
            LOADER *ld = (LOADER *)cn;
            if( !(GetTagData(PPTX_SaveColorMapped, NULL, ld->info.tags) ||
                  GetTagData(PPTX_SaveTrueColor, NULL, ld->info.tags) ) )
                  continue; /* Skip this entry */
        }
        AddEntry( NULL,lv,cn->ln_Name,LVAP_TAIL );

        count++;
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

__geta4 void ShowExtInfo( EXTDATA *xd, EXTERNAL *x, struct Window *win )
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



/*
    This routine kills the given external from memory. If the external is in
    use, however, PERR_INUSE is returned. If force == TRUE, then the
    purge is made even if the external is in use.
 */

int PurgeExternal( EXTERNAL *who, BOOL force )
{
    static __D0 int (*X_Purge)( __A6 EXTDATA * );

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

    pfree(who);

    return PERR_OK;
}



/*
    This routines opens the given external and then executes
    it's Init() - routine. If it is successful, the external
    is then added to the main list.
*/

int OpenExternal( const char *who )
{
    struct ModuleInfo *m;
    BPTR seglist;
    int version,revision,res = PERR_OK, type;
    EXTERNAL *x;
    char *name;
    static __D0 int (*X_Init)( __A6 EXTDATA * );
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

    if( kickver > SysBase->lib_Version) {
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
        x = pmalloc( type == NT_LOADER ? sizeof(LOADER) : sizeof(EFFECT) );
        x->seglist    = seglist;
        x->tags       = m->tagarray;
        x->usecount   = 0;
        x->nd.ln_Type = type;
        x->nd.ln_Name = name;
        x->nd.ln_Pri  = (BYTE)GetTagData( PPTX_Priority, 0L, m->tagarray );

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
    Opposite of OpenLibBases().
    Note: This routine has no bugs.
*/

__geta4 void CloseLibBases( __A6 EXTDATA *xd )
{
    struct Library *SysBase;

    SysBase = (struct Library *)SYSBASE();

    D(bug("CloseLibBases()\n"));

    if(xd->lb_BGUI)     CloseLibrary(xd->lb_BGUI);
    if(xd->lb_Gfx)      CloseLibrary(xd->lb_Gfx);
    if(xd->lb_Utility)  CloseLibrary(xd->lb_Utility);
    if(xd->lb_Intuition) CloseLibrary(xd->lb_Intuition);
    if(xd->lb_DOS)      CloseLibrary(xd->lb_DOS);
    if(xd->lb_GadTools) CloseLibrary(xd->lb_GadTools);
    if(xd->mport)       DeleteMsgPort(xd->mport); /* Exec call. Safe to do. */
}


/*
    Opens up libraries:

    BUG: maybe use a flag system to tell which libs to open?
*/

__geta4 __D0 int OpenLibBases( __A6 EXTBASE *xd )
{
    struct Library *SysBase;

    D(bug("OpenLibBases()\n"));

    SysBase = (struct Library *)SYSBASE();

    xd->lib.lib_Version = VERNUM;
    xd->g =             globals;
    xd->mport =         CreateMsgPort(); /* Exec call. Safe to do. */
    xd->lb_Sys =        SysBase;
    xd->lb_DOS =        OpenLibrary(DOSNAME,37L);
    xd->lb_Utility =    OpenLibrary("utility.library",37L);
    xd->lb_Intuition =  OpenLibrary("intuition.library",37L);
    D(bug("\tOpening BGUI..."));
    xd->lb_BGUI =       OpenLibrary(BGUINAME,37L);
    D(bug("done\n"));
    xd->lb_Gfx =        OpenLibrary("graphics.library",37L);
    xd->lb_GadTools =   OpenLibrary("gadtools.library",37L);

    if(!xd->lb_GadTools || !xd->lb_DOS || !xd->lb_Utility ||
       !xd->lb_Intuition || !xd->lb_BGUI || !xd->lb_Gfx)
    {
        CloseLibBases(xd);
        return PERR_GENERAL;
    }

    D(bug("\tLibraries OK\n"));

    return PERR_OK;
}


/*
    Allocates a new ExtBase structure.
    if open == TRUE, calls OpenLibBases to allocate new library bases.
*/

__geta4 EXTBASE *NewExtBase( BOOL open )
{
    EXTBASE *ExtBase = NULL;
    APTR    realptr;
    extern  APTR ExtLibData[];

    D(bug("NewExtBase(%d). Allocating %lu bytes...\n",open,EXTSIZE));

    realptr = pzmalloc( EXTSIZE );

    if(realptr) {
        ExtBase = (EXTBASE *)( (ULONG)realptr + (EXTSIZE - sizeof(EXTBASE)));
        bzero( realptr, EXTSIZE );

        D(bug("\trealptr = %08X, ExtBase = %08X\n",realptr,ExtBase));

        if(open) {
            if(OpenLibBases( ExtBase ) != PERR_OK) {
                pfree( ExtBase );
                return NULL;
            }
            ExtBase->opened = TRUE;
        }

        D(bug("\tCreating library jump table...\n"));

        MakeFunctions( ExtBase, ExtLibData, NULL );

        D(bug("\tdone\n"));
    }
    return ExtBase;
}

/*
    Use to release ExtBase allocated in NewExtBase()
*/

__geta4 void RelExtBase( EXTBASE *xb )
{
    APTR realptr;

    if(xb->opened)
        CloseLibBases(xb);

    D(bug("Releasing ExtBase @ %08X, realptr = %08X\n", xb, (ULONG)xb - (EXTSIZE - sizeof(EXTBASE)) ));
    realptr = (APTR) ((ULONG)xb - (EXTSIZE - sizeof(EXTBASE)));
    pfree(realptr);
}


