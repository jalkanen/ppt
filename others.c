/*
    PROJECT: ppt
    MODULE : others.c

    $Id: others.c,v 6.0 1999/10/04 00:01:52 jj Exp $

    This module contains those routines that do not
    clearly belong anywhere else.

*/

#include "defs.h"
#include "misc.h"

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#ifndef CLIB_BGUI_PROTOS_H
#include <clib/bgui_protos.h>
#endif


#include <proto/bgui.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/amigaguide.h>

#ifndef LIBRARIES_BGUI_MACROS_H
#include <libraries/bgui_macros.h>
#endif

#ifndef LIBRARIES_AMIGAGUIDE_H
#include <libraries/amigaguide.h>
#endif

#include <sprof.h>

#define REQ_OPEN_BGUI    /* Should ReqA() open bgui.library? */

/*-------------------------------------------------------------------------*/

Prototype ULONG         XSetGadgetAttrs( struct PPTBase *, struct Gadget *, struct Window *, struct Requester *, ULONG, ... );
Prototype ULONG         Req( struct Window *, UBYTE *, UBYTE *, ... );
Prototype ULONG         ReqA( struct Window *, UBYTE *, UBYTE *, ULONG * );
Prototype ULONG         XReq( struct Window *win, UBYTE *gadgets, UBYTE *body, ... );
Prototype VOID          FreeDOSArgs( ULONG *optarray, struct PPTBase *xd );
Prototype void          BusyAllWindows( struct PPTBase * );
Prototype void          AwakenAllWindows( struct PPTBase * );
Prototype int           HowManyThreads(void);

/*-------------------------------------------------------------------------*/

struct Hook HelpHook;

struct NewAmigaGuide nag = {0};

AMIGAGUIDECONTEXT helphandle;

/*-------------------------------------------------------------------------*/

/*
    Help notification hook function and initialization.
*/

Prototype PERROR InitHelp(VOID);

/*
    The main screen MUST be open when calling this function.

    If main screen is reopened, must call this function again!
*/
PERROR InitHelp( VOID )
{
    bzero( &HelpHook, sizeof( struct Hook ) );
    HelpHook.h_Entry    = (HOOKFUNC)CallForHelp;
    HelpHook.h_SubEntry = NULL;
    HelpHook.h_Data     = NULL; /* None needed, yet. */

    nag.nag_Lock = NULL;
    nag.nag_Name = "PROGDIR:Docs/ppt.guide";
    nag.nag_Screen = MAINSCR;

    helphandle = OpenAmigaGuideAsyncA( &nag, NULL );
    if( !helphandle ) return PERR_ERROR;

    return PERR_OK;
}

Prototype ULONG GetHelpSignal(VOID);

ULONG GetHelpSignal(VOID)
{
    if( helphandle )
        return AmigaGuideSignal( helphandle );
    else
        return 0L;
}

Prototype VOID ExitHelp(VOID);

VOID ExitHelp( VOID )
{
    if( helphandle ) {
        CloseAmigaGuide( helphandle );
        D(bug("Closed Amigaguide\n"));
    }
}

Prototype ULONG ASM CallForHelp( REGDECL(a0,struct Hook *), REGDECL(a2,APTR), REGDECL(a1,APTR) );

ULONG SAVEDS ASM CallForHelp( REGPARAM(a0,struct Hook *,hook),
                              REGPARAM(a2,APTR,object),
                              REGPARAM(a1,APTR,message) )
{
    STRPTR node;
    ULONG res = BMHELP_OK;
    char cmd[256];

    GetAttr( BT_HelpNode, object, (ULONG *)&node );

    if( node && helphandle ) {
        strcpy( cmd, "LINK ");
        strcat( cmd, node );
        if(SendAmigaGuideCmdA( helphandle, cmd, NULL )) {
            res = BMHELP_FAILURE;
        }
    } else {
        res = BMHELP_FAILURE;
    }

    return res;
}

/*
 *  A simple help call.
 */

Prototype VOID ShowHelp( STRPTR node );

VOID ShowHelp( STRPTR node )
{
    char cmd[256];

    if( node ) {
        strcpy( cmd, "LINK " );
        strcat( cmd, node );

        SendAmigaGuideCmdA( helphandle, cmd, NULL );
    }
}

/*
    Return the ordinal number of a string in the list. Returns -1 if
    the string was not found or an error occurred.

    The match is done case-insensitively
*/

Prototype int MatchStringList( const UBYTE *, const UBYTE ** );

int MatchStringList( const UBYTE *string, const UBYTE **list )
{
    int i = 0;

    if(string && list) {

        while( list[i] != NULL ) {
            if( stricmp( list[i], string ) == 0 )
                return(i);

            ++i;
        }
    }

    return -1;
}

/*
    Exactly like exec.library/FindName, but makes it case-insensitively.
*/

Prototype struct Node * FindIName( const struct List *, const UBYTE * );

struct Node *FindIName( const struct List *list, const UBYTE *name )
{
    struct Node *nn, *nd = list->lh_Head;

    while( nn = nd->ln_Succ ) {
        if( stricmp( nd->ln_Name, name ) == 0 )
            return nd;
        nd = nn;
    }

    return NULL;
}

/// ParseDOSArgs()
/*
    Front end to DOS ReadArgs(). Used in many places within PPT.

    If there is no newline appended, this will do it.  It is acceptable
    to give a NULL for template.  It is then expected to be an empty
    string.

    Returns a pointer to the argument array or NULL, if failed.
*/

Prototype ULONG *ParseDOSArgs( const UBYTE *line, const UBYTE *template, struct PPTBase *xd );

ULONG *ParseDOSArgs( const UBYTE *line, const UBYTE *template, struct PPTBase *xd )
{
    struct RDArgs *rda;
    UBYTE *buf;
    APTR DOSBase = xd->lb_DOS;
    ULONG *optarray;

    D(bug("ParseDOSArgs( %s, %s )\n",line,template));

    if( template == NULL ) template = "";

    if(rda = AllocDosObject( DOS_RDARGS, NULL ) ) {

        if(buf = pzmalloc( strlen(line) +3 )) {

            /*
             *  Add newline, if necessary
             */

            strncpy( buf,line, strlen(line) );
            if( buf[strlen(buf)-1] != '\n')
                strcat(buf,"\n");

            /*
             *  Allocate room for the options array.
             */

            if( optarray = pzmalloc( 32 * sizeof(ULONG) ) ) { /* BUG: Magic */

                /*
                 *  Initialize DOS object and parse.
                 */

                rda->RDA_Source.CS_Buffer = buf;
                rda->RDA_Source.CS_Length = strlen(buf);
                rda->RDA_Source.CS_CurChr = 0;
                rda->RDA_Flags &= RDAF_NOPROMPT;

                // D(bug("Template=[%s] - args=[%s]\n",template, buf ));

                if(ReadArgs( template, (LONG *)&optarray[2], rda )) {
                    optarray[0] = (ULONG) buf;
                    optarray[1] = (ULONG) rda;
                    return &(optarray[2]);
                } else {
                    D(bug("\tReadArgs() failed\n"));
                }
                pfree( optarray );
            } else {
                D(bug("\tmalloc() failed\n"));
            }
            pfree( buf );
        } else {
            D(bug("\tmalloc() failed\n"));
        }

        FreeDosObject( DOS_RDARGS, rda );
    } else {
        D(bug("\tAllocDosObject() failed\n"));
    }
    return NULL;
}
///
/// FreeDOSArgs()
/*
    Counterpart to the previous one. Safe to call with NULL args.
*/

VOID FreeDOSArgs( ULONG *optarray, struct PPTBase *xd )
{
    APTR DOSBase = xd->lb_DOS;

    D(bug("FreeDOSArgs()\n"));

    if( optarray ) {
        optarray -= 2; /* Countermand offset */
        pfree( (APTR)optarray[0]); /* buf */
        FreeDosObject( DOS_RDARGS, (APTR)optarray[1] );
        pfree( optarray );
    }
}
///

/*
    This is a simple varargs stub for SetGadgetAttrsA(), that uses the struct PPTBase.
    Needed by DICE, since it does not recognize #pragma tagcall.
*/

ULONG XSetGadgetAttrs( struct PPTBase *xd, struct Gadget *gad, struct Window *win, struct Requester *req, ULONG tag1, ... )
{
    APTR IntuitionBase = xd->lb_Intuition;

    return(SetGadgetAttrsA( gad, win, req, (struct TagItem *)&tag1 ));
}

/// ULONG ReqA( struct Window *win, UBYTE *gadgets, UBYTE *body, ULONG *args )

/*
    Show a simple requester. Currently stolen from bgui demos. If win ==
    NULL , then uses WB screen, if it is NEGNUL, then uses main window.If
    gadgets == NULL, then uses default.
    NOTE: This version requires at least bgui.library 38.3, because
          previous releases would crash if the same task opened the library
          twice.
*/


SAVEDS ULONG ReqA( struct Window *win, UBYTE *gadgets, UBYTE *body, ULONG *args )
{
    struct Library          *BGUIBase = NULL;
    struct IntuitionBase    *IntuitionBase;
    struct bguiRequest      req = { 0L };
    ULONG                   res;
    struct PPTBase          *PPTBase;
    struct Screen           *wscr = NULL;

    D(bug("ReqA()..."));

    PPTBase = NewExtBase( TRUE );
    if(!PPTBase) return 0;

    BGUIBase = PPTBase->lb_BGUI;
    IntuitionBase = PPTBase->lb_Intuition;

    if(win == NEGNUL) {
        if( globals->maindisp ) {
            if( MAINWIN ) {
                win = MAINWIN;
            } else {
                win = NULL; wscr = MAINSCR;
            }
        } else {
            win = NULL;
        }
    }

    if(gadgets == NULL) gadgets = XGetStr(MSG_OK_GAD);

    req.br_GadgetFormat     = gadgets;
    req.br_TextFormat       = body;
    req.br_Screen           = wscr;

    if(strchr(gadgets,'|') != NULL)
        req.br_Title        = XGetStr(MSG_PPT_REQUEST);
    else
        req.br_Title        = XGetStr(MSG_PPT_INFORMATION);

    req.br_Flags            = BREQF_XEN_BUTTONS;

    req.br_ReqPos           = POS_CENTERSCREEN;

    // D(bug("\tPutting up the requester\n"));

    if(win)
        ScreenToFront( win->WScreen );

    PROFILE_OFF();
    res =  BGUI_RequestA( win, &req, args );
    PROFILE_ON();

    RelExtBase( PPTBase );

    D(bug("Finished. (User chose %u)\n",res));
    return res;

}
///
/// ULONG Req( struct Window *win, UBYTE *gadgets, UBYTE *body, ... )

SAVEDS ULONG Req( struct Window *win, UBYTE *gadgets, UBYTE *body, ... )
{
    ULONG res;

    if(win) {
        BusyAllWindows(globxd);
    }

    res = ReqA( win, gadgets, body, (ULONG *) ( ((ULONG)&body) + sizeof(void *) ) );

    if(win) {
        AwakenAllWindows(globxd);
    }

    return res;
}
///
/// ULONG XReq( struct Window *win, UBYTE *gadgets, UBYTE *body, ... )

SAVEDS ULONG XReq( struct Window *win, UBYTE *gadgets, UBYTE *body, ... )
{
    return( ReqA( win, gadgets, body, (ULONG *) (&body +1) ) );
}
///

/*
    This will call the appropriate hook for each of the windows
    in the code.
    Currently there are no flags defined.
*/

Prototype VOID DoAllWindows( void (* ASM)(REGDECL(a0,Object *), REGDECL(a1,APTR), REGDECL(a6,struct PPTBase *)), APTR, const ULONG, struct PPTBase *);

SAVEDS
VOID DoAllWindows( VOID (* ASM hookfunc)(REGPARAM(a0,Object *,Win), REGPARAM(a1,APTR,data), REGPARAM(a6,struct PPTBase *,PPTBase)),
                   APTR  hookdata,
                   const ULONG flags,
                   struct PPTBase *PPTBase )
{
    struct Node *cn;

    if(globals->WO_main) { hookfunc( globals->WO_main, hookdata, PPTBase ); }
    if(framew.Win) { hookfunc( framew.Win, hookdata, PPTBase ); }
    if(extf.Win) { hookfunc( extf.Win, hookdata, PPTBase ); }
    if(exts.Win) { hookfunc( exts.Win, hookdata, PPTBase ); }
    if(extl.Win) { hookfunc( extl.Win, hookdata, PPTBase ); }
    if(prefsw.Win) { hookfunc( prefsw.Win, hookdata, PPTBase ); }
    if(toolw.Win){ hookfunc( toolw.Win,hookdata, PPTBase ); }
    if(selectw.Win) { hookfunc( selectw.Win, hookdata, PPTBase ); }

    for( cn = globals->frames.lh_Head; cn->ln_Succ; cn = cn->ln_Succ) {
        FRAME *f;

        f = (FRAME *)cn;

        /*
         *  Quickrender windows
         */

        if(f->disp) {
            if( f->disp->Win && f->disp->win )
                hookfunc( f->disp->Win, hookdata, PPTBase );
        }

        /*
         *  Info windows
         */

        if( f->mywin ) {
            if( f->mywin->win )
                hookfunc( f->mywin->WO_win, hookdata, PPTBase );
        }

        /*
         *  Info edit windows
         */

         if( f->editwin ) {
             if( f->editwin->Win && f->editwin->win ) {
                 hookfunc( f->editwin->Win, hookdata, PPTBase );
             }
         }

    } /* for(all display wins) */

}

/*--------------------------------------------------------------------------*/

VOID ASM Hook_BusyWindow( REGPARAM(a0,Object *,Win),
                          REGPARAM(a1,APTR,foo),
                          REGPARAM(a6,struct PPTBase *,PPTBase) )
{
    WindowBusy( Win ); // Is really DoMethod() - safe to call
}

VOID ASM Hook_AwakenWindow( REGPARAM(a0,Object *,Win),
                            REGPARAM(a1,APTR,foo),
                            REGPARAM(a6,struct PPTBase *,PPTBase) )
{
    WindowReady( Win ); // Is really DoMethod() - safe to call
}

/*
    This routine will put all windows controlled by main code to busy status.

    BUG: Maybe it should use correct way to set the quick dispwin.
*/

SAVEDS VOID BusyAllWindows( struct PPTBase *xd )
{
    // D(bug("\tBusyAllWindows( %08X )\n",xd ));

    DoAllWindows( Hook_BusyWindow, NULL, 0L, xd );
    return;
}


SAVEDS VOID AwakenAllWindows( struct PPTBase *xd )
{
    // D(bug("\tAwakenAllWindows()\n"));

    DoAllWindows( Hook_AwakenWindow, NULL, 0L, xd );
    return;
}

/*--------------------------------------------------------------------------*/

struct dmi_data {
    struct Window   *win;
    ULONG           item;
    BOOL            disable;
};

VOID ASM Hook_DisableMenuItem( REGPARAM(a0,Object *,Win),
                               REGPARAM(a1,APTR,foo),
                               REGPARAM(a6,struct PPTBase *,PPTBase) )
{
    struct dmi_data *d = (struct dmi_data *)foo;

    DisableMenu( Win, d->item, d->disable );
}

Prototype VOID DisableMenuItem( ULONG );

VOID DisableMenuItem( ULONG item )
{
    struct dmi_data d = { 0L, TRUE };
    struct ToolbarItem *tb;

    DoAllWindows( Hook_DisableMenuItem, &d, 0L, globxd );

    if(tb = FindToolbarGID( ToolbarList, item) ) {
        if( tb->ti_Gadget ) {
            SetGadgetAttrs( GAD(tb->ti_Gadget),prefsw.win, NULL, GA_Disabled, TRUE, TAG_DONE);
        }
    }
}

Prototype VOID EnableMenuItem( ULONG );

VOID EnableMenuItem( ULONG item )
{
    struct dmi_data d = { 0L, FALSE };
    struct ToolbarItem *tb;

    d.item = item;
    DoAllWindows( Hook_DisableMenuItem, &d, 0L, globxd );
    if(tb = FindToolbarGID( ToolbarList, item) ) {
        if( tb->ti_Gadget ) {
            SetGadgetAttrs( GAD(tb->ti_Gadget),prefsw.win, NULL, GA_Disabled, FALSE, TAG_DONE);
        }
    }
}

/*--------------------------------------------------------------------------*/

VOID ASM Hook_CheckMenuItem( REGPARAM(a0,Object *,Win),
                             REGPARAM(a1,APTR,foo),
                             REGPARAM(a6,struct PPTBase *,PPTBase) )
{
    struct dmi_data *d = (struct dmi_data *)foo;

    CheckItem( Win, d->item, d->disable );
}

Prototype VOID CheckMenuItem( ULONG item, BOOL state );

VOID CheckMenuItem( ULONG item, BOOL state )
{
    struct dmi_data d;

    d.item = item;
    d.disable = state;
    DoAllWindows( Hook_CheckMenuItem, &d, 0L, globxd );
}

/*--------------------------------------------------------------------------*/

/// int HowManyThreads( void )

/*
    BUG: SHould not use Forbid()
*/

int HowManyThreads( void )
{
    int c = 0;

    struct Node *cn = globals->frames.lh_Head;
    SHLOCKGLOB();
    Forbid();
    while(cn->ln_Succ) {
        if( ((FRAME *)cn)->currproc ) c++;
        cn = cn->ln_Succ;
    }

    Permit();
    UNLOCKGLOB();
    return c;
}
///

/// void Nag(void)

/*
    If the specified time has passed, shows a nag requester
*/

#ifdef DEBUG_MODE
Prototype void Nag(void);

void Nag(void)
{
    struct DateStamp ds;
    LONG   age,day,year, month;

#ifdef __SASC
    DateStamp( &ds ); // Current date

    sscanf( __AMIGADATE__, "(%d.%d.%d)", &day, &month, &year );

    D(bug("Checking date %s = %d days\n",__AMIGADATE__, (year-78)*365+30*month+day));
#else
#error "Can't compile with this compiler"
#endif

    age = (year-78)*365+30*month+day+23 - ds.ds_Days; // Magic number 23 = extra days
    D(bug("Age = %d days\n",age));
    if( age >= NAG_PERIOD) {
        Panic("\nWARNING"
              "\n\n"
              "This program has now expired,\n"
              "so you really should get a new version at\n\n"
              "http://www.iki.fi/~jalkanen/PPT.html\n" );
    }
}
#endif
///
/*-------------------------------------------------------------------------*/
/*                                  END OF CODE                            */
/*-------------------------------------------------------------------------*/
