/*
    PROJECT: ppt
    MODULE:  external.c

    $Id: external.c,v 2.16 1998/10/25 22:13:36 jj Exp $

    This contains necessary routines to operate on external modules,
    ie loaders and effects.

*/

/*-------------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "gui.h"
#include "misc.h"

#include "version.h"

#ifndef CLIB_UTILITY_PROTOS_H
#include <clib/utility_protos.h>
#endif

#ifndef PRAGMAS_INTUITION_PRAGMAS_H
#include <pragmas/intuition_pragmas.h>
#endif

#include <dos/dostags.h>

#include <proto/locale.h>

#include "proto/module.h"
#include "proto/effect.h"
#include "powerup/proto/ppc.h"

#include "libraries/ExecutiveAPI.h"

/*-------------------------------------------------------------------------*/
/* Prototypes*/

Prototype int           AddExtEntries( EXTBASE *, struct Window *, Object *, UBYTE, ULONG );
Prototype void          ShowExtInfo( EXTBASE *, EXTERNAL *, struct Window * );
Prototype PERROR        PurgeExternal( EXTERNAL *, BOOL );
Prototype PERROR        OpenExternal( const char *, UBYTE );
Prototype void          RelExtBase( EXTBASE * );
Prototype EXTBASE       *NewExtBase( BOOL );

/*-------------------------------------------------------------------------*/

#define EXTSIZE (LIB_VECTSIZE * XLIB_FUNCS + sizeof(EXTBASE))
#define AFF_PROCESSORS (AFF_68010|AFF_68020|AFF_68030|AFF_68040|AFF_68060)
#define AFF_FPUS (AFF_68881|AFF_68882|AFF_FPU40)

/*-------------------------------------------------------------------------*/
/* Code */

/*
    Set the NICE value of a task
*/

Prototype PERROR SetTaskNice( struct Task *task, LONG nice, EXTBASE *ExtBase );

PERROR SetTaskNice( struct Task *task, LONG nice, EXTBASE *ExtBase )
{
    struct ExecBase *SysBase = ExtBase->lb_Sys;
    struct ExecutiveMessage em = {0};
    struct MsgPort *executiveport;
    ULONG  sig, quit = 0;

    em.message.mn_ReplyPort = ExtBase->mport;
    em.message.mn_Length = sizeof(struct ExecutiveMessage);
    em.command = EXAPI_CMD_SET_NICE;
    em.task    = task;
    em.value1  = nice;

    Forbid();

    if( executiveport = FindPort(EXECUTIVEAPI_PORTNAME) ) {
        PutMsg( executiveport, (struct Message *) &em );
        Permit();
        while(!quit) {
            sig = Wait( SIGBREAKF_CTRL_C|(1<<ExtBase->mport->mp_SigBit));
            if( sig & SIGBREAKF_CTRL_C ) {
                return PERR_BREAK;
            }
            if( sig & (1<<ExtBase->mport->mp_SigBit)) {
                struct Message *msg;

                while( msg = GetMsg( ExtBase->mport ) ) {
                    if( msg->mn_Node.ln_Type == NT_REPLYMSG ) {
                        quit = TRUE; /* BUG: Does not check for errors */
                    } else {
                        ReplyMsg( msg ); /* Mikä lie */
                    }
                }
            }
        }
    } else {
        Permit();
    }

    return PERR_OK;

}


/*
    This is called whenever a new task is started.
*/

Prototype PERROR NewTaskProlog( FRAME *frame, EXTBASE * );

PERROR NewTaskProlog( FRAME *frame, EXTBASE *ExtBase )
{
    return SetTaskNice( FindTask(NULL), 10, ExtBase );
}

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

#if 0
    strcpy( buf, globals->userprefs->modulepath );

    SHLOCKGLOB();
    AddPart( buf, x->diskname, 255 );
    UNLOCKGLOB();

    ModuleBase = OpenLibrary( buf, 0L );
#else
    ModuleBase = OpenLibrary( x->diskname, 0L );
#endif
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

                if( (flags & AEE_SAVECM) && !add ) {
                    if( ((LOADER *)ext)->saveformats & CSF_LUT )
                        add = TRUE;
                }

                if( (flags & AEE_SAVETC) && !add ) {
                    if( ((LOADER *)ext)->saveformats & (CSF_GRAYLEVEL|CSF_RGB) )
                        add = TRUE;
                }

                if( (flags & AEE_LOAD) && !add ) {
                    if( ((LOADER *)ext)->canload )
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
            DoMethod(lv,LVM_ADDSINGLE,NULL,cn->ln_Name, LVAP_TAIL, 0L);
            count++;
        }
    }

    UNLOCKGLOB();
    SortList( win,lv );

    return count;
}

/*
    This function shows info about a given external. The parent window
    is win. If win == NULL, then uses main PPT window.
    This routine is re-entrant.
*/

Local
SAVEDS VOID ShowOldExtInfo( EXTBASE *ExtBase, EXTERNAL *x, struct Window *win )
{
    int   ver,rev;
    APTR txt, au;

    SHLOCKGLOB();
    txt  = (APTR)GetTagData( PPTX_InfoTxt, NULL, x->tags );
    au   = (APTR)GetTagData( PPTX_Author, NULL, x->tags );
    ver  = GetTagData( PPTX_Version, -1, x->tags );
    rev  = GetTagData( PPTX_Revision, -1, x->tags );
    UNLOCKGLOB();

    Req(win,NULL,
        XGetStr( mEXTERNAL_INFO_FORMAT ),
        x->nd.ln_Name, (ULONG)ver, (ULONG)rev, txt ? txt : "", au ? au : XGetStr(mAUTHOR_UNKNOWN) );

}

Local
const char *AFF2CPU( char *buf, ULONG flags )
{
    strcpy(buf, "");

    if (!flags) {
        strcpy(buf, "68000");
        return buf;
    }
    if (flags & AFF_68010)
        strcat(buf, "68010,");

    if (flags & AFF_68020)
        strcat(buf, "68020,");

    if (flags & AFF_68030)
        strcat(buf, "68030,");

    if (flags & AFF_68040)
        strcat(buf, "68040,");

    if (flags & AFF_68060)
        strcat(buf, "68060,");

    if (flags & AFF_68881)
        strcat(buf, "68881,");

    if (flags & AFF_68882)
        strcat(buf, "68882,");

    if (flags & AFF_PPC)
        strcat(buf, "PowerPC,");

    buf[strlen(buf) - 1] = '\0';        // Remove last comma.

    return buf;
}

Local
SAVEDS VOID ShowNewExtInfo( EXTBASE *ExtBase, EXTERNAL *x, struct Window *win )
{
    struct Library *ModuleBase;
    int   ver,rev;
    APTR  txt, au;
    char  cpu[256];

    D(bug("ShowNewExtInfo()\n"));

    ModuleBase = OpenModule( x, ExtBase ); // BUG!
    if(!ModuleBase) return;

    txt  = (APTR)Inquire( PPTX_InfoTxt, ExtBase );
    au   = (APTR)Inquire( PPTX_Author, ExtBase );
    ver  = ModuleBase->lib_Version;
    rev  = ModuleBase->lib_Revision;

    Req(win,NULL,
        XGetStr( mEXTERNAL_INFO_FORMAT ),
        x->nd.ln_Name,
        (ULONG)ver, (ULONG)rev,
        AFF2CPU( cpu, Inquire(PPTX_CPU, ExtBase) ),
        txt ? txt : "",
        au ? au : XGetStr(mAUTHOR_UNKNOWN) );

    CloseModule( ModuleBase, ExtBase );
}

/// ShowExtInfo()
/*
    An useless stub, I hope, in the future.
*/

VOID ShowExtInfo( EXTBASE *xd, EXTERNAL *x, struct Window *win )
{
    if( x->nd.ln_Type == NT_SCRIPT ) {
        char buf[256], *prg;

        if( SysBase->LibNode.lib_Version >= 40 ) {
            prg = "MultiView";
        } else {
            if( !(prg = getenv("PAGER") ) )
                prg = "More";
        }

        sprintf(buf, "%s %s PUBSCREEN=PPT", prg, x->diskname );

        SystemTags( buf, SYS_Asynch, TRUE,
                    SYS_Input, Open("NIL:", MODE_OLDFILE ),
                    SYS_Output, Open("NIL:", MODE_NEWFILE ),
                    TAG_DONE );
    } else {
        if(x->islibrary)
            ShowNewExtInfo( xd, x, win );
        else
            ShowOldExtInfo( xd, x, win );
    }
}
///

/// PurgeOldExternal()
Local
PERROR PurgeOldExternal( EXTERNAL *who, BOOL force )
{
#if 0
    auto PERROR (* ASM X_Purge)( REG(a6) EXTBASE * );
#endif

    if(who->usecount && force == FALSE) /* If someone is using us, don't release */
        return PERR_INUSE;

    /* First, make sure no-one can find us. */

    LOCKGLOB();
    Remove( (struct Node *)who );
    UNLOCKGLOB();

    /* Execute release code, if any */

#if 0
    X_Purge   = (FPTR)GetTagData( PPTX_Purge,(ULONG)NULL,who->tags);
    if(X_Purge)
        (*X_Purge)( globxd );
#endif

    UnLoadSeg(who->seglist);

    sfree(who);

    return PERR_OK;
}
///
/// PurgeNewExternal()
Local
PERROR PurgeNewExternal( EXTERNAL *who, BOOL force )
{
    if(who->usecount && force == FALSE) /* If someone is using us, don't release */
        return PERR_INUSE;

    /* Make sure no-one can find us anymore */

    LOCKGLOB();
    Remove( (struct Node *)who );
    UNLOCKGLOB();

    if( who->nd.ln_Type != NT_SCRIPT ) {
        if( globals->userprefs->expungelibs || force )
            FlushLibrary( FilePart(who->diskname), globxd );
    }

    sfree(who);

    return PERR_OK;
}
///

/// PurgeExternal()
/*
    Purges an external from memory, removing it from our
    lists, as well.

    Returns PERR_INUSE, if the external is currently used
    by someone.

    If force == TRUE, then will remove even if someone is
    using us and forces a flush as well.
 */

PERROR PurgeExternal( EXTERNAL *w, BOOL f )
{
    if( w->islibrary || w->nd.ln_Type == NT_SCRIPT )
        return PurgeNewExternal( w, f );
    else {
        return PurgeOldExternal( w, f );
    }
}
///

/// InitOldExternal()
/*
    This routines opens the given external and then executes
    it's Init() - routine. If it is successful, the external
    is then added to the main list.
*/

PERROR InitOldExternal( const char *who, BPTR seglist )
{
    struct ModuleInfo *m;
    int version,revision,res = PERR_OK, type;
    EXTERNAL *x;
    char *name;
    static PERROR (* ASM X_Init)( REG(a6) EXTBASE * );
    UWORD kickver, pptver;

//    DEBUG("OpenExternal()\n");
    seglist = NewLoadSeg( who, NULL );
    if(!seglist) {
        D(bug("Unable to open\n"));
        Req(NEGNUL,NULL, GetStr(mNO_OBJECT_CODE),who);
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
            Req(NEGNUL,NULL,GetStr(mNOT_A_PPT_MODULE),who);
            return PERR_UNKNOWNTYPE;
    }

    // DumpMem( (ULONG)(m->tagarray),64L);
    version  = GetTagData( PPTX_Version, ~0, m->tagarray);
    revision = GetTagData( PPTX_Revision,~0, m->tagarray);
//    X_Init   = (FPTR)GetTagData( PPTX_Init,(ULONG)NULL,m->tagarray);
    name     = (char *)GetTagData( PPTX_Name, NULL, m->tagarray);
    if(!name)
        goto nogood;

    kickver  = (UWORD)GetTagData( PPTX_ReqKickVersion, 0, m->tagarray);
    pptver   = (UWORD)GetTagData( PPTX_ReqPPTVersion, 0, m->tagarray);

//    DEBUG("'%s' version:%d.%d.\n",name,version,revision);

    if( kickver > SysBase->LibNode.lib_Version) {
        Req(NEGNUL,NULL,GetStr(mREQUIRES_OS_X),name,kickver);
        goto nogood;
    }

    if( pptver > VERNUM ) {
        Req(NEGNUL,NULL,GetStr(mREQUIRES_PPT_X),name,pptver);
        goto nogood;
    }

//    DEBUG("Calling X_Init() @ %X\n",X_Init);
//    if(X_Init)
//        res = (*X_Init)(globxd);

//    DEBUG("INIT DONE\n");

    if(res == PERR_OK || X_Init == NULL) { /* No error occurred, or X_Init() does not exist. */
        x = smalloc( type == NT_LOADER ? sizeof(LOADER) : sizeof(EFFECT) );
        bzero( x, type == NT_LOADER ? sizeof(LOADER) : sizeof(EFFECT) );
        x->seglist    = seglist;
        x->tags       = m->tagarray;
        x->islibrary  = FALSE;
        x->usecount   = 0;
        x->nd.ln_Type = type;
        x->nd.ln_Name = name;
        x->nd.ln_Pri  = (BYTE)GetTagData( PPTX_Priority, 0L, m->tagarray );
        strncpy( x->diskname, who, MAXPATHLEN+NAMELEN );
        strncpy( x->realname, (STRPTR)GetTagData( PPTX_Name, (ULONG)"", m->tagarray ), NAMELEN );

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
///
/// InitScript()
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
    strncpy( x->diskname, who, MAXPATHLEN+NAMELEN );
    strncpy( x->realname, FilePart(who), 39 );

    LOCKGLOB();
    Enqueue( &globals->scripts, (struct Node *)x );
    UNLOCKGLOB();

    D(bug("\tOpened script OK.\n"));

    return PERR_OK;
}
///
/// InitNewExternal()
/*
    Open new style external
*/
PERROR InitNewExternal( char *who, struct Library *ModuleBase )
{
    UWORD pptver;
    ULONG cpuflags, thiscpu;
    PERROR res = PERR_OK;
    STRPTR name;
    UBYTE type;
    EXTERNAL *x, *prev;

    D(bug("NewOpenExternal(%s)\n",who));

    /*
     *  Determine name
     */

    name = (STRPTR) Inquire( PPTX_Name, globxd );

    /*
     *  Determine whether we can use this or not.
     *
     *  From SysBase->AttnFlags we read only the lower byte, as
     *  the 68060 is the last CPU probably to ever support the AmigaOS
     *  as we know it.
     *  Note that the 68000 does not have any bits set in AttnFlags
     */

    pptver = (UWORD) Inquire( PPTX_ReqPPTVersion,  globxd );

    if( pptver > VERNUM ) {
        Req(NEGNUL,NULL,GetStr(mREQUIRES_PPT_X),name,(ULONG)pptver);
        res = PERR_WONTOPEN;
        goto nogood;
    }

    cpuflags = Inquire( PPTX_CPU, globxd );
#if 1
    thiscpu  = (ULONG)(SysBase->AttnFlags & 0xFF);
    if( globxd->lb_PPC ) thiscpu |= AFF_PPC;
#else
    thiscpu  = AFF_68010|AFF_68020|AFF_68030|AFF_68040|AFF_68881|AFF_68882|AFF_FPU40|AFF_68060;
    D(bug("\tMy CPU=%d, external code is optimized for %d\n",thiscpu,cpuflags));
#endif

    if( cpuflags && ( ((thiscpu & AFF_PROCESSORS) < (cpuflags & AFF_PROCESSORS)) ||
                      ((thiscpu & AFF_FPUS) < (cpuflags & AFF_FPUS)) ) )
    {
        D(bug("\tExternal %s does not support this CPU\n",name));
        res = PERR_OK;
        goto nogood;
    }

    /*
     *  Determine type.  A kludge, at best.  Should probably use
     *  Inquire() somehow.
     *  BUG: Do something about this.
     */

    if( strcmp( &who[strlen(who)-7], ".effect" ) == 0 )
        type = NT_EFFECT;
    else
        type = NT_LOADER;

    /*
     *  Determine if it already exists and if it does, which
     *  one we should use
     */

    if( prev = (EXTERNAL *)FindName( type == NT_EFFECT ? &globals->effects : &globals->loaders, name) ) {
        struct Library *save;
        ULONG oldcpuflags;

        save = ModuleBase;

        ModuleBase = OpenModule( prev, globxd );
        if(!ModuleBase) Panic("OpenModule() failed in InitExternal()");

        oldcpuflags = Inquire(PPTX_CPU, globxd);

        D(bug("%s : %lu <-> %s : %lu\n", prev->diskname, (ULONG)oldcpuflags,
                                         who, (ULONG)cpuflags ));

        CloseModule(ModuleBase,globxd);
        ModuleBase = save;

        if( oldcpuflags >= cpuflags ) {
            /* Skip the new one */
            D(bug("\t%s is a better match for the hardware\n",prev->diskname));
            res = PERR_OK;
            goto nogood;
        } else {
            /* Remove the old one */
            D(bug("\t%s is a better match for the hardware\n",who));
            PurgeExternal( prev, TRUE );
        }

    }


    /*
     *  Allocate room and put the necessary info into memory.
     */

    x = smalloc( type == NT_LOADER ? sizeof(LOADER) : sizeof(EFFECT) );
    bzero( x, type == NT_LOADER ? sizeof(LOADER) : sizeof(EFFECT) );

    x->seglist    = 0L;
    x->tags       = NULL;
    x->islibrary  = TRUE;
    x->usecount   = 0;
    x->nd.ln_Type = type;
    x->nd.ln_Name = x->realname;
    x->nd.ln_Pri  = (BYTE)Inquire( PPTX_Priority, globxd );
    strncpy( x->diskname, who, MAXPATHLEN+NAMELEN );
    strncpy( x->realname, name, NAMELEN );

    /*
     *  Set up loader specific stuphs.  Mainly this means
     *  caching some values so that we don't have to
     *  constantly load a file while attempting to recognize a file.
     */

    if( type == NT_LOADER ) {
        STRPTR s;

        ((LOADER *)x)->saveformats = Inquire( PPTX_ColorSpaces, globxd );
        ((LOADER *)x)->canload = Inquire( PPTX_Load, globxd );
        if( s = (STRPTR) Inquire( PPTX_PreferredPostFix, globxd ) ) {
            strncpy( ((LOADER *)x)->prefpostfix, s, NAMELEN );
        }
        if( s = (STRPTR) Inquire( PPTX_PostFixPattern, globxd ) ) {
            strncpy( ((LOADER *)x)->postfixpat, s, MAXPATTERNLEN );
        }
    }

    LOCKGLOB();
    if(type == NT_LOADER)
        Enqueue( &globals->loaders, (struct Node *)x );
    else {
        Enqueue( &globals->effects, (struct Node *)x );
    }
    UNLOCKGLOB();

    D(bug("\tOpened library OK.\n"));

nogood:

    return res;
}
///

/// OpenExternal()
PERROR OpenExternal( const char *who, UBYTE type )
{
    struct Library *ModuleBase;
    struct Library *PPCLibBase = globxd->lb_PPC;
    PERROR res;
    BPTR seglist;

    if( type == NT_SCRIPT ) {
        res = InitScript( who );
    } else if( ModuleBase = OpenLibrary( who, 0L ) ) {
        res = InitNewExternal( who, ModuleBase );
        CloseLibrary( ModuleBase );
    } else if( seglist = NewLoadSeg( who, NULL ) ) {
        res = InitOldExternal( who, seglist );
    } else {
        D(bug("Unable to open\n"));
        Req(NEGNUL,NULL, GetStr(mNO_OBJECT_CODE),who);
        res = PERR_WONTOPEN;
    }

    return res;
}
///

/*---------------------------------------------------------------------------*/

/// CloseLibBases()
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

    if(xd->lb_PPC)      CloseLibrary(xd->lb_PPC);
    if(xd->lb_CyberGfx) CloseLibrary(xd->lb_CyberGfx);
    if(xd->lb_Locale)   CloseLibrary(xd->lb_Locale);
    if(xd->lb_BGUI)     CloseLibrary(xd->lb_BGUI);
    if(xd->lb_Gfx)      CloseLibrary( (struct Library *)xd->lb_Gfx);
    if(xd->lb_Utility)  CloseLibrary(xd->lb_Utility);
    if(xd->lb_Intuition) CloseLibrary( (struct Library *)xd->lb_Intuition);
    if(xd->lb_DOS)      CloseLibrary( (struct Library *)xd->lb_DOS);
    if(xd->lb_GadTools) CloseLibrary(xd->lb_GadTools);
    if(xd->mport)       DeleteMsgPort(xd->mport); /* Exec call. Safe to do. */
}
///
/// OpenLibBases()
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

    xd->lb_CyberGfx = OpenLibrary("cybergraphics.library",40L);
    xd->lb_PPC = OpenLibrary("ppc.library",45L);

    if(!xd->lb_GadTools || !xd->lb_DOS || !xd->lb_Utility ||
       !xd->lb_Intuition || !xd->lb_BGUI || !xd->lb_Gfx)
    {
        CloseLibBases(xd);
        return PERR_GENERAL;
    }

    return PERR_OK;
}
///
/// NewExtBase()
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

    // D(bug("NewExtBase(%d). Allocating %lu bytes...\n",open,EXTSIZE));

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
///
/// RelExtBase()
/*
    Use to release ExtBase allocated in NewExtBase()
*/

SAVEDS VOID RelExtBase( EXTBASE *xb )
{
    APTR realptr;

    if(xb->opened)
        CloseLibBases(xb);

    // D(bug("Releasing ExtBase @ %08X, realptr = %08X\n", xb, (ULONG)xb - (EXTSIZE - sizeof(EXTBASE)) ));
    realptr = (APTR) ((ULONG)xb - (EXTSIZE - sizeof(EXTBASE)));
    pfree(realptr);
}
///

