/*
    PROJECT: ppt
    MODULE : others.c

    $Id: others.c,v 1.22 1997/12/06 22:51:01 jj Exp $

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

Prototype ULONG         XSetGadgetAttrs( EXTBASE *, struct Gadget *, struct Window *, struct Requester *, ULONG, ... );
Prototype ULONG         Req( struct Window *, UBYTE *, UBYTE *, ... );
Prototype ULONG         ReqA( struct Window *, UBYTE *, UBYTE *, ULONG * );
Prototype ULONG         XReq( struct Window *win, UBYTE *gadgets, UBYTE *body, ... );
Prototype VOID          FreeDOSArgs( ULONG *optarray, EXTBASE *xd );
Prototype void          BusyAllWindows( EXTBASE * );
Prototype void          AwakenAllWindows( EXTBASE * );
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

Prototype ULONG ASM CallForHelp( REG(a0) struct Hook *, REG(a2) APTR, REG(a1) APTR );

ULONG SAVEDS ASM CallForHelp( REG(a0) struct Hook *hook,
                              REG(a2) APTR object,
                              REG(a1) APTR message )
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

/*
    Front end to DOS ReadArgs(). Used in many places within PPT.

    If there is no newline appended, this will do it.  It is acceptable
    to give a NULL for template.  It is then expected to be an empty
    string.

    Returns a pointer to the argument array or NULL, if failed.
*/

Prototype ULONG *ParseDOSArgs( const UBYTE *line, const UBYTE *template, EXTBASE *xd );

ULONG *ParseDOSArgs( const UBYTE *line, const UBYTE *template, EXTBASE *xd )
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

/*
    Counterpart to the previous one. Safe to call with NULL args.
*/

VOID FreeDOSArgs( ULONG *optarray, EXTBASE *xd )
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


/*
    This is a simple varargs stub for SetGadgetAttrsA(), that uses the EXTBASE.
    Needed by DICE, since it does not recognize #pragma tagcall.
*/

ULONG XSetGadgetAttrs( EXTBASE *xd, struct Gadget *gad, struct Window *win, struct Requester *req, ULONG tag1, ... )
{
    APTR IntuitionBase = xd->lb_Intuition;

    return(SetGadgetAttrsA( gad, win, req, (struct TagItem *)&tag1 ));
}


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
    EXTBASE *               ExtBase;
    struct Screen           *wscr = NULL;

    D(bug("ReqA()..."));

    ExtBase = NewExtBase( TRUE );
    if(!ExtBase) return 0;

    BGUIBase = ExtBase->lb_BGUI;
    IntuitionBase = ExtBase->lb_Intuition;

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

    RelExtBase( ExtBase );

    D(bug("Finished. (User chose %u)\n",res));
    return res;

}

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

SAVEDS ULONG XReq( struct Window *win, UBYTE *gadgets, UBYTE *body, ... )
{
    return( ReqA( win, gadgets, body, (ULONG *) (&body +1) ) );
}

/*
    This will call the appropriate hook for each of the windows
    in the code.
    Currently there are no flags defined.
*/

Prototype VOID DoAllWindows( void (* ASM)(REG(a0) Object *, REG(a1) APTR, REG(a6) EXTBASE *), APTR, const ULONG, EXTBASE *);

SAVEDS
VOID DoAllWindows( VOID (* ASM hookfunc)(REG(a0) Object *Win, REG(a1) APTR data, REG(a6) EXTBASE *ExtBase),
                   APTR  hookdata,
                   const ULONG flags,
                   EXTBASE *ExtBase )
{
    struct Node *cn;

    if(globals->WO_main) { hookfunc( globals->WO_main, hookdata, ExtBase ); }
    if(framew.Win) { hookfunc( framew.Win, hookdata, ExtBase ); }
    if(extf.Win) { hookfunc( extf.Win, hookdata, ExtBase ); }
    if(exts.Win) { hookfunc( exts.Win, hookdata, ExtBase ); }
    if(extl.Win) { hookfunc( extl.Win, hookdata, ExtBase ); }
    if(prefsw.Win) { hookfunc( prefsw.Win, hookdata, ExtBase ); }
    if(toolw.Win){ hookfunc( toolw.Win,hookdata, ExtBase ); }
    if(selectw.Win) { hookfunc( selectw.Win, hookdata, ExtBase ); }

    for( cn = globals->frames.lh_Head; cn->ln_Succ; cn = cn->ln_Succ) {
        FRAME *f;

        f = (FRAME *)cn;

        /*
         *  Quickrender windows
         */

        if(f->disp) {
            if( f->disp->Win && f->disp->win )
                hookfunc( f->disp->Win, hookdata, ExtBase );
        }

        /*
         *  Info windows
         */

        if( f->mywin ) {
            if( f->mywin->win )
                hookfunc( f->mywin->WO_win, hookdata, ExtBase );
        }

        /*
         *  Info edit windows
         */

         if( f->editwin ) {
             if( f->editwin->Win && f->editwin->win ) {
                 hookfunc( f->editwin->Win, hookdata, ExtBase );
             }
         }

    } /* for(all display wins) */

}

/*--------------------------------------------------------------------------*/

ASM
VOID Hook_BusyWindow( REG(a0) Object *Win, REG(a1) APTR foo, REG(a6) EXTBASE *ExtBase )
{
    WindowBusy( Win ); // Is really DoMethod() - safe to call
}

ASM
VOID Hook_AwakenWindow( REG(a0) Object *Win, REG(a1) APTR foo, REG(a6) EXTBASE *ExtBase )
{
    WindowReady( Win ); // Is really DoMethod() - safe to call
}

/*
    This routine will put all windows controlled by main code to busy status.

    BUG: Maybe it should use correct way to set the quick dispwin.
*/

SAVEDS VOID BusyAllWindows( EXTBASE *xd )
{
    // D(bug("\tBusyAllWindows( %08X )\n",xd ));

    DoAllWindows( Hook_BusyWindow, NULL, 0L, xd );
    return;
}


SAVEDS VOID AwakenAllWindows( EXTBASE *xd )
{
    // D(bug("\tAwakenAllWindows()\n"));

    DoAllWindows( Hook_AwakenWindow, NULL, 0L, xd );
    return;
}

/*--------------------------------------------------------------------------*/

struct dmi_data {
    ULONG item;
    BOOL  disable;
};

ASM
VOID Hook_DisableMenuItem( REG(a0) Object *Win,
                           REG(a1) APTR foo,
                           REG(a6) EXTBASE *ExtBase )
{
    struct dmi_data *d = (struct dmi_data *)foo;

    DisableMenu( Win, d->item, d->disable );
}

Prototype VOID DisableMenuItem( ULONG );

VOID DisableMenuItem( ULONG item )
{
    struct dmi_data d = { 0L, TRUE };

    d.item = item;
    DoAllWindows( Hook_DisableMenuItem, &d, 0L, globxd );
}

Prototype VOID EnableMenuItem( ULONG );

VOID EnableMenuItem( ULONG item )
{
    struct dmi_data d = { 0L, FALSE };

    d.item = item;
    DoAllWindows( Hook_DisableMenuItem, &d, 0L, globxd );
}

/*--------------------------------------------------------------------------*/

ASM
VOID Hook_CheckMenuItem( REG(a0) Object *Win,
                         REG(a1) APTR foo,
                         REG(a6) EXTBASE *ExtBase )
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

/*
    If the specified time has passed, shows a nag requester
*/

Prototype void Nag(void);

void Nag(void)
{
    struct DateStamp ds;
    struct DateTime  dt = {0};
    LONG   age,day,year;
    char buf[40],mon[20],*arg;

#ifdef __SASC
    DateStamp( &ds ); // Current date
    arg = strtok(__DATE__," ");
    strcpy(mon,arg);
    arg = strtok(NULL," ");
    day = atoi(arg);
    arg = strtok(NULL," ");
    year= atoi(arg)-1900; /* BUG: No year 2000 compliance */

    sprintf(buf,"%d-%s-%d",day,mon,year);

    dt.dat_Format = FORMAT_DOS;
    dt.dat_StrDate = buf;
    dt.dat_StrTime = NULL;

    D(bug("Checking date %s\n",buf));
#else
#error "Can't compile with this compiler"
#endif

    if( StrToDate( &dt ) ) {
        age = dt.dat_Stamp.ds_Days - ds.ds_Days;
        if( age >= NAG_PERIOD) {
            Req(NEGNUL,"Understood",
                "\n"
                ISEQ_C ISEQ_B "WARNING" ISEQ_N
                "\n\n"
                ISEQ_C "This program is now %d days old\n"
                "so you really should get a new version at\n\n"
                "http://www.iki.fi/~jalkanen/PPT.html\n",
                age);
        }
    }
}

/*-------------------------------------------------------------------------*/
/*                                  END OF CODE                            */
/*-------------------------------------------------------------------------*/
