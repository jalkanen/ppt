/*
    PROJECT: ppt
    MODULE : main.c

    $Id: main.c,v 1.103 1999/02/14 19:39:53 jj Exp $

    Main PPT code for GUI handling.
*/

/// Includes
#include "defs.h"
#include "misc.h"
#include "version.h"
#include "gui.h"
#include "rexx.h"
#include "render.h"

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#ifndef CLIB_UTILITY_PROTOS_H
#include <clib/utility_protos.h>
#endif

#ifndef CLIB_ASL_PROTOS_H
#include <clib/asl_protos.h>
#endif

#ifndef WORKBENCH_WORKBENCH_H
#include <workbench/workbench.h>
#endif

#ifndef DOS_DOSTAGS_H
#include <dos/dostags.h>
#endif

#ifndef LIBRARIES_ASL_H
#include <libraries/asl.h>
#endif

#ifndef WORKBENCH_STARTUP_H
#include <workbench/startup.h>
#endif

#ifndef LIBRARIES_BGUI_MACROS_H
#include <libraries/bgui_macros.h>
#endif

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

#ifndef PROTO_AMIGAGUIDE_H
#include <proto/amigaguide.h>
#endif

#include <proto/cybergraphics.h>
#include <cybergraphics/cybergraphics.h>

#include <stdlib.h>
#include <sprof.h>

#include <proto/iomod.h>
///

/*------------------------------------------------------------------------*/
/* Internal Defines */

#define FLASHING_DISPRECT    /* Should we flash the rectangle? */

/*------------------------------------------------------------------------*/
/* Types */

/*------------------------------------------------------------------------*/
/// Global variables

GLOBALS *globals;
EXTBASE *globxd;
const char verstr[] = "$VER: PPT v"VERSION" "__AMIGADATE__". "
#if defined(DEBUG_MODE)
    "(debug version - distribution prohibited)";
#else
#if defined(_M68020)
    "(68020+ optimized version)";
#else
    "(68000 version)";
#endif
#endif

int quit = 0;

struct ExtInfoWin extf = {0}, extl = {0}, exts = {0};

struct FramesWindow framew = { 0,0,0,0,TRUE,0};

struct ToolWindow toolw = {0,0,0,0,TRUE,0};

struct PrefsWindow prefsw = {0};

struct SelectWindow selectw = {0};

struct MsgPort *MainIDCMPPort;

BOOL loader_close = TRUE; /* TRUE, if the Loader window should be closed
                             after the first selection. */

#ifdef DEBUG_MODE
BPTR debug_fh, old_fh;
#endif
///

/*------------------------------------------------------------------------*/
/*  Internal variables */

/*------------------------------------------------------------------------*/
/// Internal prototypes

extern int              main(int, char **);
Local void              HandleAppMsg( const struct AppMessage * );
Local int               HandleInfoIDCMP( INFOWIN *, ULONG );
Local void              UpdateDispPrefsWindow( FRAME * );
#ifdef _DCC
Local int               wbmain( struct WBStartup * );
#endif
///

/*------------------------------------------------------------------------*/
/* External prototypes */

Prototype VOID          DoMainList( const FRAME * );
Prototype VOID          UpdateMainWindow( FRAME * );

/*------------------------------------------------------------------------*/
/* Code */

/// Compiler dependant code
#ifdef _DCC
int brk(void) {
    return 0;
}
#endif

#ifdef __SASC
/*
 * Disable SAS control-C handling
 */
void __regargs _CXBRK(void) {}
void __regargs __chkabort(void) {}
#endif
///

/// ZoomIn, ZoomOut
/*
    Zooms into the given rectangle
*/

Local
VOID ZoomIn( FRAME *f, const struct Rectangle r )
{
    D(bug("ZoomIn(%08X)\n",f));
    LOCK(f);
    f->zoombox.Top  = r.MinY;
    f->zoombox.Left = r.MinX;
    f->zoombox.Height = r.MaxY - r.MinY;
    f->zoombox.Width = r.MaxX - r.MinX;
    UNLOCK(f);
}

Local
VOID ZoomOut( FRAME *f )
{
    D(bug("ZoomOut(%08X)\n",f));
    LOCK(f);
    f->zoombox.Top  = 0;
    f->zoombox.Left = 0;
    f->zoombox.Height = f->pix->height;
    f->zoombox.Width = f->pix->width;
    UNLOCK(f);
}
///

/// CheckArea()
BOOL CheckArea( const FRAME *frame )
{
    if(frame->selbox.MinX == ~0) {
        Req( GetFrameWin(frame),NULL,GetStr( MSG_NO_AREA_SELECTED ) );
        return FALSE;
    }
    return TRUE;
}
///

/// HowManyFrames
Local int HowManyFrames(void)
{
    int c = 0;

    struct Node *cn = globals->frames.lh_Head;
    SHLOCKGLOB();
    while(cn->ln_Succ) {
        cn = cn->ln_Succ;
        c++;
    }

    UNLOCKGLOB();

    return c;
}
///

/// ConstructListName, ParseListEntry, and CompareListEntryHook
Prototype STRPTR ConstructListName( FRAME *);

STRPTR ConstructListName( FRAME *frame )
{
    static char entry[NAMELEN+20];

    sprintf( entry, "%c%c\t%s", IsFrameBusy(frame) ? 'B' : ' ',
                                frame->renderobject ? 'R' : ' ',
                                frame->name );

    return entry;
}

Prototype STRPTR ParseListEntry( APTR entry );

STRPTR ParseListEntry( APTR entry )
{
    static char name[NAMELEN+20], *t;

    strncpy( name, entry, NAMELEN+19 );
    if(t = strrchr( name, '\t' ))
        return (t+1);
    else
        return name;
}

SAVEDS ASM
LONG CompareListEntryHook( REG(a0) struct Hook *hook,
                           REG(a2) Object *object,
                           REG(a1) struct lvCompare *lvc )
{
    return( stricmp( ParseListEntry( lvc->lvc_EntryA ),
                     ParseListEntry( lvc->lvc_EntryB ) ) );
}
///

/// SetFrameStatus()
/*
    Sets the display status on the main window.  Currently only does the
    busy thing.
 */

Prototype VOID SetFrameStatus( FRAME *, ULONG );

VOID SetFrameStatus( FRAME *frame, ULONG status )
{
#if 0
    APTR entry;
    char name[NAMELEN+1];

    LockList( framew.Frames );

    if( entry = FirstEntry( framew.Frames ) ) {
        do {
            STRPTR s;

            s = ParseListEntry( entry );
            strncpy( name, s, NAMELEN );
            if( strcmp(name, frame->name) == 0 ) {

            }

            entry = NextEntry( framew.Frames, entry );
        } while(entry);
    }

    UnlockList( framew.Frames );
#else
    DoMainList( frame );
#endif
}
///

/// DoMainList()
/*
    Update main window display list
    frame = currently active frame.
*/

VOID DoMainList( const FRAME *frame )
{
    struct Node *cn = (struct Node *)globals->frames.lh_Head, *nn;
    ULONG entrynum = 0, selentry = ~0;

    DoMethod( framew.Frames, LVM_CLEAR, NULL );

    SHLOCKGLOB();

    while( nn = cn->ln_Succ ) {
        if( (FRAME *)cn == frame )
            selentry = entrynum;

        if( ((FRAME *)cn)->busy != BUSY_LOADING ) {
            DoMethod( framew.Frames, LVM_ADDSINGLE, NULL,
                      ConstructListName( (FRAME *)cn ), LVAP_TAIL );
        }

        entrynum++;
        cn = nn;
    }

    UNLOCKGLOB();

    if( selentry != ~0 ) {
        SetAttrs( framew.Frames, LISTV_Select, selentry, TAG_END );
    } else {
        InternalError( "Big booboo. No such frame?" );
    }

    DoMethod( framew.Frames, LVM_SORT, NULL );

    if( framew.win )
        RefreshList(framew.win, framew.Frames );

}
///
/// KludgeWindowMenus()
Prototype VOID KludgeWindowMenus(VOID);

VOID KludgeWindowMenus(VOID)
{
    if( framew.win )
        CheckMenuItem( MID_FRAMEWINDOW, TRUE );
    else
        CheckMenuItem( MID_FRAMEWINDOW, FALSE );

    if( toolw.win )
        CheckMenuItem( MID_TOOLWINDOW, TRUE );
    else
        CheckMenuItem( MID_TOOLWINDOW, FALSE );

    if( extl.win )
        CheckMenuItem( MID_LOADERS, TRUE );
    else
        CheckMenuItem( MID_LOADERS, FALSE );

    if( extf.win )
        CheckMenuItem( MID_EFFECTS, TRUE );
    else
        CheckMenuItem( MID_EFFECTS, FALSE );

    if( exts.win )
        CheckMenuItem( MID_REXXWINDOW, TRUE );
    else
        CheckMenuItem( MID_REXXWINDOW, FALSE );

    if( selectw.win )
        CheckMenuItem( MID_SELECTWINDOW, TRUE );
    else
        CheckMenuItem( MID_SELECTWINDOW, FALSE );

}
///

/// UpdateMainWindow()
/*
    Makes sure the info texts are most current. If frame == NULL,
    then removes the display data.
    BUG: The menu item disable/enable routines should really be one big
         hook, that is called via the global setup.
*/

VOID UpdateMainWindow( FRAME *frame )
{

    UpdateIWSelbox( frame ); /* NULL is OK */

    if( frame ) {
        FRAME *alpha = NULL;
        ULONG args[8];

        SHLOCK(frame);

#ifdef USE_OLD_ALPHA
        if( frame->alpha ) {
            alpha = FindFrame( frame->alpha );
        }
#endif

        args[0] = (ULONG)frame->pix->width; args[1] = (ULONG)frame->pix->height;
        args[2] = (ULONG)ColorSpaceName(frame->pix->colorspace);
        args[3] = (ULONG)frame->pix->bits_per_component;
#ifdef USE_OLD_ALPHA
        args[4] = (alpha) ? (ULONG) alpha->name : (ULONG)GetStr(MSG_NO_ALPHA);
#endif

        UNLOCK(frame);

        SetGadgetAttrs( GAD(framew.Info), framew.win, NULL,
                        INFO_TextFormat, GetStr(MSG_MAINWIN_INFO),
                        INFO_Args, args, TAG_END );

        // EnableMenuItem( MID_SAVE );
        EnableMenuItem( MID_SAVEAS );
        EnableMenuItem( MID_DELETE );
        EnableMenuItem( MID_RENAME );
        EnableMenuItem( MID_HIDE );

        EnableMenuItem( MID_UNDO );
        EnableMenuItem( MID_CUT );
        EnableMenuItem( MID_COPY );
        if( clipframe )
            EnableMenuItem( MID_PASTE );
        else
            DisableMenuItem( MID_PASTE );

        EnableMenuItem( MID_SELECTALL );
        EnableMenuItem( MID_CUTFRAME );
        EnableMenuItem( MID_PALETTE );
        EnableMenuItem( MID_CROP );
        EnableMenuItem( MID_ZOOMIN );
        EnableMenuItem( MID_ZOOMOUT );
        EnableMenuItem( MID_CORRECTASPECT );
        EnableMenuItem( MID_EDITINFO );

#ifdef USE_OLD_ALPHA
        if (alpha) EnableMenuItem( MID_REMOVEALPHA );
#endif

        EnableMenuItem( MID_PROCESS );
        if( frame->currext ) EnableMenuItem( MID_BREAK );
        EnableMenuItem( MID_RENDER );
        EnableMenuItem( MID_SETRENDER );

        if( frame->renderobject ) {
            EnableMenuItem( MID_CLOSERENDER );
            EnableMenuItem( MID_PALETTE );
        } else {
            DisableMenuItem( MID_CLOSERENDER );
            DisableMenuItem( MID_PALETTE );
        }

    } else {
        SetGadgetAttrs( GAD(framew.Info), framew.win, NULL,
                        INFO_TextFormat, GetStr(MSG_MAIN_NOFRAME),
                        INFO_Args, NULL, TAG_END );

        // DisableMenuItem( MID_SAVE );
        DisableMenuItem( MID_SAVEAS );
        DisableMenuItem( MID_DELETE );
        DisableMenuItem( MID_RENAME );
        DisableMenuItem( MID_HIDE );

        DisableMenuItem( MID_UNDO );
        DisableMenuItem( MID_CUT );
        DisableMenuItem( MID_COPY );
        DisableMenuItem( MID_PASTE );

        DisableMenuItem( MID_SELECTALL );
        DisableMenuItem( MID_CUTFRAME );
        DisableMenuItem( MID_PALETTE );
        DisableMenuItem( MID_CROP );
        DisableMenuItem( MID_ZOOMIN );
        DisableMenuItem( MID_ZOOMOUT );
#ifdef USE_OLD_ALPHA
        DisableMenuItem( MID_REMOVEALPHA );
#endif
        DisableMenuItem( MID_CORRECTASPECT );
        DisableMenuItem( MID_EDITINFO );

        DisableMenuItem( MID_PROCESS );
        DisableMenuItem( MID_BREAK );
        DisableMenuItem( MID_RENDER );
        DisableMenuItem( MID_SETRENDER );

        DisableMenuItem( MID_CLOSERENDER );

        DisableMenuItem( MID_PALETTE );
    }
}
///

/// DisplayFrames()
/*
    Renders all frames attached to it, if need be
*/

VOID DisplayFrames( FRAME *start )
{
    FRAME *cur;

    cur = start;

    do {
        if( cur->reqrender ) DisplayFrame( cur );
    } while( cur = FindFrame(cur->attached) );

#ifdef USE_OLD_ALPHA
    if( cur = FindFrame(start->alpha) ) {
        DisplayFrame( cur );
    }
#endif
}
///

/// AreaDrop()
/*
    This is executed when the user drops something into the window
*/

VOID AreaDrop( FRAME *frame, FRAME *drop )
{
    ULONG ans;

    if( FrameFree( frame ) && FrameFree( drop ) ) {

        ans = Req(GetFrameWin(frame), GetStr(mALPHA_COMPOSITE_CANCEL),
                                      GetStr(mFRAMEDROPPED) );

        switch(ans) {
            case 1: /* Alpha */
                if( AddAlpha( frame, drop ) == PERR_OK )
                    UpdateMainWindow(frame);

                break;

            case 2: /* Composite */
                Composite( frame, drop );
                break;

            case 0: /* Cancelled */
                D(bug("Ignored drop\n"));
                break;
        }
    }
}
///
/// HandleAppMsg()
/*
    Does exactly what the name implies. User may activate this
    function by dropping an icon in PPT main window.
*/

Local
void HandleAppMsg( const struct AppMessage *apm )
{
    struct WBArg *ap;
    int i;
    char name[MAXPATHLEN];

    for( ap = apm->am_ArgList, i = 0; i < apm->am_NumArgs; i++, ap++) {
        NameFromLock( ap->wa_Lock, name, MAXPATHLEN );
        if(strlen(ap->wa_Name) > 0) { /* Currently, no directories are allowed */
            RunLoad(name,NULL,NULL);
        } else {
            Req( NEGNUL, NULL, GetStr( MSG_IS_A_DIRECTORY ), name );
        }
    } /* for */
}
///

/// HandleMenuIDCMP()
/*
    Menus

    from == whoever the message was from, NULL, if from main window/others
    type == see above
*/

Prototype int HandleMenuIDCMP( ULONG, FRAME *, UBYTE );

int HandleMenuIDCMP( ULONG rc, FRAME *frame, UBYTE type )
{
    ULONG res;
    UBYTE *path, tmpbuf[MAXPATHLEN+1], tmpbuf2[MAXPATHLEN+1];
    struct Window *win;
    APTR entry;
    FRAME *newframe;
    int result = 0, threads, frames;

    /*
     *  First, we'll set the frame to point to correct place.
     */

    if(!frame) {
        if( entry = (APTR)FirstSelected( framew.Frames ) ) {
            SHLOCKGLOB();
            frame = (FRAME *) FindName( &globals->frames, ParseListEntry(entry) );
            UNLOCKGLOB();
        }
    }

    win = GetFrameWin(frame);

    /*
     *  Then branch
     */

    switch(rc) {

        /*
         *  Project Menu
         */

        case MID_QUIT:
            if( !DOCONFIRM || Req(win, GetStr(MSG_YESNO_GAD), GetStr(MSG_QUIT)) ) {
                D(bug("\nUser wishes to quit\n"));
                result = HANDLER_QUIT;
            }
            break;

        case MID_DELETE:
            if( FrameFree( frame ) ) {
                res = 1;

                if( DOCONFIRM ) res = Req(win,GetStr(MSG_YESNO_GAD),GetStr(MSG_DELETE), frame->nd.ln_Name);

                if(res) {
                    DeleteFrame(frame);
                    ClearMouseLocation(); /* BUG: Should actually be in frame.c */
                    result = HANDLER_DELETED; /* MARK: exit current loop */
                }
            }
            break;

        case MID_RENAME:
            if( FrameFree( frame ) ) {
                struct TagItem renamestr[] = {
                    ARSTRING_MaxChars,     NAMELEN-1,
                    ARSTRING_InitialString,NULL,
                    AROBJ_Value,           NULL,
                    TAG_DONE
                };
                struct TagItem renamewin[] = {
                    AR_Text,         NULL,
                    AR_StringObject, NULL,
                    TAG_DONE
                };

                renamewin[0].ti_Data = (ULONG) GetStr( mPLEASE_ENTER_NEW_NAME );

                /*
                 *  Set up the rename buffer.
                 */

                strncpy( tmpbuf, frame->name, MAXPATHLEN );
                DeleteNameCount( tmpbuf );
                renamestr[2].ti_Data = (ULONG) tmpbuf;
                renamewin[1].ti_Data = (ULONG) renamestr;
                renamestr[1].ti_Data = (ULONG) tmpbuf;

                BusyAllWindows( globxd );
                switch (AskReqA( frame, renamewin, globxd ) ) {
                    case PERR_OK:
                        /*
                         *  We have to fetch the entry from the main list, since
                         *  it really is not a STRPTR but it has something else attached...
                         */

                        entry = (APTR)FirstSelected(framew.Frames);
                        D(bug("\tProject renamed: '%s' -> '%s'\n",entry, tmpbuf ));
                        MakeFrameName( tmpbuf, tmpbuf, NAMELEN, globxd );

                        /*
                         *  We'll make a backup copy, since ConstructListName will require
                         *  the frame pointer.
                         *  BUG: Should we just use DoMainList()?
                         */

                        LockList( framew.Frames );
                        strcpy( tmpbuf2, frame->name );
                        strcpy( frame->name, tmpbuf );
                        if(DoMethod( framew.Frames, LVM_REPLACE, NULL, entry, ConstructListName(frame) )) {
                            UpdateFrameInfo( frame );
                            RefreshFrameInfo( frame, globxd );
                        } else {
                            D(bug("\tReplaceEntry failed! Not updating...\n"));
                            strcpy( frame->name, tmpbuf2 );
                        }
                        DoMethod( framew.Frames, LVM_UNLOCKLIST, NULL );
                        if( framew.win ) RefreshList( framew.win, framew.Frames );
                        break;

                    case PERR_BREAK:
                        D(bug("****BREAK while in AskReq()****\n"));
                        result = HANDLER_QUIT;
                }
                AwakenAllWindows( globxd );
            }

            break;

        case MID_ABOUT:

            Fortify_CheckAllMemory();
            threads = HowManyThreads();
            frames  = HowManyFrames();

            Req( win, NULL, GetStr(MSG_ABOUT),
                 COPYRIGHT, VERSION, RELEASE,
                 VERSDATE,
#ifdef DEBUG_MODE
                 ISEQ_B"\n* Debug code has been compiled in *\n"
                 "\nDO NOT SPREAD!!!\n"
                 ISEQ_N,
#else
                 "\n",
#endif
                 frames, threads );

            break;

        case MID_PREFS:
            if( !GimmePrefsWindow() ) {
                Req(win,NULL, GetStr(MSG_CANNOT_OPEN_PREFSWINDOW) );
            } else {
                UpdatePrefsWindow( globals->userprefs );
            }
            break;

        case MID_NEW:
            RunLoad( NULL, "Plain", NULL );
            break;

        case MID_LOADNEW:
            if( DoRequest(gvLoadFileReq.Req) == FRQ_OK ) {
                /*
                 *  Fetch the paths and set up the defaults.
                 */

                GetAttr( FRQ_Drawer, gvLoadFileReq.Req, (ULONG *)&path );
                strncpy( globals->userprefs->startupdir, path, MAXPATHLEN );
                GetAttr( FRQ_File, gvLoadFileReq.Req, (ULONG *)&path );
                strncpy( globals->userprefs->startupfile, path, NAMELEN );
                GetAttr( FRQ_Path, gvLoadFileReq.Req, (ULONG *)&path );
                RunLoad( (char *)path, NULL, NULL);
            }

            break;

        case MID_LOADAS:
            if( !extl.win ) {
                res = HandleMenuIDCMP( MID_LOADERS, NULL, FROM_MAINWINDOW );
                loader_close = TRUE;
            } else {
                ActivateWindow( extl.win );
            }
            break;

        case MID_SAVE:
            if( FrameFree( frame ) ) {
                /* BUG: Still to be done... */
            }
            break;

        case MID_SAVEAS:
            if(FrameFree(frame)) {
                RunSave( frame, NULL );
            }
            break;

        case MID_HIDE:
            /*
             *  Toggles existence of this window
             */

            if( frame->disp ) {
                if( frame->disp->win ) {
                    HideDisplayWindow( frame );
                    frame->selstatus = 0;
                    return HANDLER_DELETED; /* Signal: No longer exists */
                } else {
                    ShowDisplayWindow( frame );
                }
            }
            break;

        /*
         *  Edit Menu
         */

        case MID_UNDO:
            if( FrameFree( frame ) ) {
                newframe = UndoFrame( frame );
                if(newframe) {
                    DisplayFrame( newframe );
                    UpdateMainWindow( newframe );
                    result = HANDLER_DELETED; /* MARK: This frame no longer exits */
                }
            }
            break;

        case MID_CUTFRAME: /* Cut to new */
            if( FrameFree( frame ) ) {
                if(CheckArea(frame)) {
                    if( newframe = Edit(frame, EDITCMD_CUTFRAME) ) {
                        AllocInfoWindow( newframe,globxd );
                        AddFrame( newframe );
                        DoMainList( newframe );
                        GuessDisplay( newframe ); /* Install display etc. */
                        UpdateMainWindow( newframe );
                        DisplayFrame( newframe );
                        result = HANDLER_DELETED;
                    }
                }
            }
            break;

#ifdef DEBUG_MODE
        /* BUG: This is a very buggy version, causing enforcer hits.
           Should really call the external module called 'Crop' */

        case MID_CROP: /* Just like da previous, but does not make a new one. */
            if( FrameFree( frame ) ) {
                if(CheckArea(frame)) {
                    if( newframe = Edit(frame, EDITCMD_CROPFRAME) ) {
                        ReplaceFrame(frame, newframe);
                        UpdateInfoWindow( newframe->mywin,globxd);
                        UpdateMainWindow( newframe );
                        DisplayFrame( newframe );
                        CloseRender( newframe,globxd ); /* BUG: Maybe not here? */
                        result = HANDLER_DELETED;
                    }
                }
            }
            break;
#endif

        case MID_CUT: /* Just plain cut */
            if( FrameFree( frame ) ) {
                if(CheckArea(frame)) {
                    if(newframe = Edit(frame, EDITCMD_CUT)) {
                        ReplaceFrame(frame,newframe);
                        DisplayFrame(newframe);
                        EnableMenuItem( MID_PASTE );
                        result = HANDLER_DELETED;
                    }
                }
            }
            break;

        case MID_COPY:
            if( FrameFree( frame ) ) {
                if( CheckArea( frame ) ) {
                    if( Edit( frame, EDITCMD_COPY ) ) {
                        EnableMenuItem( MID_PASTE );
                    }
                }
            }
            break;

        case MID_PASTE:
            if( FrameFree( frame ) ) {
                Edit( frame, EDITCMD_PASTE );
            }
            break;

        case MID_SELECTALL:
            if( FrameFree( frame ) ) {
                RemoveSelectBox( frame );
                SelectWholeImage( frame );
                UpdateIWSelbox(frame);
                DrawSelectBox( frame, 0L );
            }
            break;

        case MID_ZOOMIN:
            if(ObtainFrame( frame, BUSY_READONLY ) ) {
                LOCK(frame);
                if( frame->selbox.MinX != ~0 ) {
                    frame->zooming = TRUE;
                    ZoomIn( frame, frame->selbox );
                    DisplayFrame( frame );
                    frame->zooming = FALSE;
                }
                UNLOCK(frame);
                ReleaseFrame( frame );
            }
            break;

        case MID_ZOOMOUT:
            if( ObtainFrame( frame, BUSY_READONLY ) ) {
                LOCK(frame);
                frame->zooming = TRUE;
                ZoomOut( frame );
                DisplayFrame( frame );
                frame->zooming = FALSE;
                UNLOCK(frame);
                ReleaseFrame( frame );
            }
            break;

        case MID_CORRECTASPECT:
            if( FrameFree( frame ) ) {
                if( frame->disp->Win ) {
                    struct IBox ib;
                    LONG newheight, newwidth;

                    GetAttr( WINDOW_Bounds, frame->disp->Win, (ULONG *)&ib );

                    /*
                     *  Let us assume the width is correct and try to
                     *  adjust the height.
                     */

                    if( frame->pix->DPIY && frame->pix->DPIX ) {
                        newheight = (LONG)frame->pix->height * (LONG)ib.Width * frame->pix->DPIY / ((LONG)frame->pix->width * (LONG)frame->pix->DPIX);
                    } else {
                        newheight = (LONG)frame->pix->height * (LONG)ib.Width / frame->pix->width;
                    }
                    newheight = newheight * globals->maindisp->DPIX / globals->maindisp->DPIY;

                    newwidth  = (LONG)frame->pix->width  * (LONG)ib.Height / frame->pix->height;

                    if( newheight > MAINSCR->Height ) {
                        ib.Width = (WORD)newwidth;
                    } else {
                        ib.Height = (WORD)newheight;
                    }

                    SetAttrs( frame->disp->Win, WINDOW_Bounds, &ib, TAG_DONE );
                }
            }
            break;

        case MID_EDITINFO:
            if( FrameFree( frame ) ) {
                EDITWIN *ew = frame->editwin;

                /* If it doesn't exist, let's create it */
                if(!ew) {
                    if(ew = AllocEditWindow( frame )) {
                        if(GimmeEditWindow( ew ) != PERR_OK ) {
                            FreeEditWindow( ew );
                            break;
                        }
                    }
                }

                if( ew->win ) {
                    WindowToFront( ew->win );
                    ActivateWindow( ew->win );
                } else {
                    OpenEditWindow( ew ); /* Make sure it's open */
                }
            }
            break;

        /*
         *  Display Menu
         */

        case MID_RENDER:
            if( FrameFree( frame ) ) {
                OpenRender( frame, globxd );
                DoRender( frame->renderobject );
                EnableMenuItem( MID_CLOSERENDER );
            }
            break;

        case MID_CLOSERENDER:
            if( FrameFree( frame ) ) {
                CloseRender( frame, globxd );
                DisableMenuItem( MID_CLOSERENDER );
            }
            break;

        case MID_SETRENDER:
            if( FrameFree( frame ) ) {
                struct DispPrefsWindow *dpw;

                LOCK(frame);
                if( frame->dpw == NULL ) {
                    D(bug("Allocating space for dpw\n"));
                    frame->dpw = pmalloc( sizeof( struct DispPrefsWindow ) );
                    if( !frame->dpw ) {
                        D(bug("Memory allocation failed!\n"));
                        break;
                    }

                    bzero( frame->dpw, sizeof( struct DispPrefsWindow) );
                    frame->dpw->frame = frame;
                    D(bug("Space allocated\n"));
                }
                UNLOCK(frame);

                dpw = frame->dpw;

                if( dpw->win == NULL ) {
                    // HitNull();
                    D(bug("Trying to open DispPrefsWindow...\n"));
                    if( dpw->tempdisp = AllocDisplay(globxd) ) {
                        CopyDisplay( frame->disp, dpw->tempdisp );
                        if( GimmeDispPrefsWindow( dpw->tempdisp, frame->dpw ) == NULL ) {
                            D(bug("Failed!"));
                            Req( win, NULL, GetStr(MSG_CANNOT_OPEN_DISPPREFSWINDOW));
                        }
                    } else {
                        D(bug("Couldn't alloc tempdisp\n"));
                        Req( win, NULL, GetStr(MSG_CANNOT_OPEN_DISPPREFSWINDOW));
                    }
                }

                UpdateDispPrefsWindow( frame );
            }
            break;

        case MID_EDITPALETTE:
            if( FrameFree(frame) ) {
                OpenPaletteWindow(frame);
            }
            break;

        case MID_LOADPALETTE:
            if( FrameFree(frame)) {
                DoLoadPalette( frame );
            }
            break;

        case MID_SAVEPALETTE:
            if( FrameFree(frame)) {

                if( DoRequest( gvPaletteSaveReq.Req ) == FRQ_OK ) {
                    GetAttr( FRQ_Path, gvPaletteSaveReq.Req, (ULONG*)&path );
                    SavePalette( frame, path, globxd );
                }

                CloseInfoWindow( frame->mywin, globxd );
            }
            break;

        /*
         *  Process Menu
         */

        case MID_PROCESS:
            if(FrameFree(frame)) {
                if(RunFilter( frame, NULL ) != PERR_OK)
                    Req(win,NULL,GetStr(MSG_PERR_NO_NEW_PROCESS) );
            }
            break;

        case MID_BREAK:
            if(frame) {
                if(frame->currproc) {
                    Signal(&(frame->currproc->pr_Task), SIGBREAKF_CTRL_C);
                } else {
                    if( DOCONFIRM ) Req( win, NULL, GetStr(MSG_NO_CURRENT_PROCESS ));
                    else DisplayBeep(NULL);
                }
            } else {
                if( DOCONFIRM) Req( win, NULL, GetStr(MSG_NO_FRAME_SELECTED) );
                else DisplayBeep( NULL );
            }
            break;

#ifdef USE_OLD_ALPHA
        case MID_REMOVEALPHA:
            if( FrameFree(frame) ) {
                if( frame->alpha ) {
                    if( Req( GetFrameWin(frame), GetStr(MSG_YESNO_GAD),
                        "\nReally remove the current alpha channel?\n" ) )
                    {
                        RemoveAlpha( frame );
                        UpdateMainWindow( frame );
                    }
                } else {
                    Req( GetFrameWin(frame), NULL, "\nThis frame has no alpha channel!\n" );
                }
            }
            break;
#endif

        /*
         *  Windows menu
         */

        case MID_FRAMEWINDOW:
            if(!framew.Win) {
                GimmeMainWindow();
                KludgeWindowMenus();
            }
            if(!framew.win) {
                framew.win = WindowOpen(framew.Win);
                CheckMenuItem( MID_FRAMEWINDOW, TRUE );
            } else {
                WindowClose( framew.Win );
                framew.win = NULL;
                CheckMenuItem( MID_FRAMEWINDOW, FALSE );
            }

            break;

        case MID_TOOLWINDOW:
            if(!toolw.Win) {
                GimmeToolBarWindow();
                KludgeWindowMenus();
            }
            if( !toolw.win ) {
                /* If the window is not open, we'll open it now */
                toolw.win = WindowOpen(toolw.Win);
                CheckMenuItem( MID_TOOLWINDOW, TRUE );
                ClearMouseLocation();
                if( globals->userprefs->mainfont )
                    SetFont( toolw.win->RPort, globals->userprefs->maintf );
                SetAPen( toolw.win->RPort, 1 ); /* Draw in black. BUG: Should use the Dripen array */
            } else {
                /* It must be open, so we'll close it. */
                WindowClose( toolw.Win );
                toolw.win = NULL;
                CheckMenuItem( MID_TOOLWINDOW, FALSE );
            }
            break;

        case MID_LOADERS:
            if( !(extl.Win) ) {
                if( extl.Win = GimmeExtInfoWindow( GetStr(mLOADERS), GetStr(MSG_LOAD_GAD), &extl ) ) {
                    // SetAttrs( extl.Win, BT_HelpNode, "PPT.guide/LoadersWindow", TAG_DONE );

                    if( extl.prefs.initialpos.Height )
                        SetAttrs( extl.Win, WINDOW_Bounds, &extl.prefs.initialpos, TAG_DONE );

                    GetAttr( WINDOW_Window, extl.Win, (ULONG*) &(extl.win) );
                    AddExtEntries( globxd, extl.win, extl.List, NT_LOADER, AEE_ALL );
                    extl.type = NT_LOADER;
                    extl.last = -1;
                    extl.menuid = MID_LOADERS;
                    extl.mylist = &(globals->loaders);
                    KludgeWindowMenus();
                } else {
                    Req(win,NULL,GetStr(MSG_CANNOT_OPEN_INFOWINDOW));
                    return HANDLER_QUIT;
                }
            }

            if( ! extl.win ) {
                extl.win = WindowOpen( extl.Win );
                CheckMenuItem( MID_LOADERS, TRUE );
                loader_close = FALSE;
            } else {
                WindowClose( extl.Win );
                extl.win = NULL;
                CheckMenuItem( MID_LOADERS, FALSE );
            }
            break;

        case MID_EFFECTS:
            if( !extf.Win ) {
                if( extf.Win = GimmeExtInfoWindow( GetStr(mEFFECTS), GetStr(mGO), &extf ) ) {
                    if( extf.prefs.initialpos.Height )
                        SetAttrs( extf.Win, WINDOW_Bounds, &extf.prefs.initialpos, TAG_DONE );

                    GetAttr( WINDOW_Window, extf.Win, (ULONG*)&(extf.win) );
                    AddExtEntries( globxd, extf.win, extf.List, NT_EFFECT,AEE_ALL );
                    extf.type = NT_EFFECT;
                    extf.last = -1;
                    extf.menuid = MID_EFFECTS;
                    extf.mylist = &(globals->effects);
                    KludgeWindowMenus();
                } else {
                    Req(win,NULL,GetStr(MSG_CANNOT_OPEN_INFOWINDOW));
                    return HANDLER_QUIT;
                }
            }

            if( !extf.win ) {
                extf.win = WindowOpen( extf.Win );
                CheckMenuItem( MID_EFFECTS, TRUE );
            } else {
                WindowClose( extf.Win );
                extf.win = NULL;
                CheckMenuItem( MID_EFFECTS, FALSE );
            }
            break;

        case MID_REXXWINDOW:
            if( !exts.Win ) {
                if( exts.Win = GimmeExtInfoWindow( GetStr(mSCRIPTS), GetStr(MSG_EXECUTE_GAD), &exts ) ) {
                    if( exts.prefs.initialpos.Height )
                        SetAttrs( exts.Win, WINDOW_Bounds, &exts.prefs.initialpos, TAG_DONE );

                    GetAttr( WINDOW_Window, exts.Win, (ULONG*)&(exts.win) );
                    AddExtEntries( globxd, exts.win, exts.List, NT_SCRIPT,AEE_ALL );
                    exts.type = NT_SCRIPT;
                    exts.last = -1;
                    exts.menuid = MID_REXXWINDOW;
                    exts.mylist = &(globals->scripts); // BUG:
                    KludgeWindowMenus();
                } else {
                    Req(win,NULL,GetStr(MSG_CANNOT_OPEN_INFOWINDOW));
                    return HANDLER_QUIT;
                }
            }

            if( !exts.win ) {
                exts.win = WindowOpen( exts.Win );
                CheckMenuItem( MID_REXXWINDOW, TRUE );
            } else {
                WindowClose( exts.Win );
                exts.win = NULL;
                CheckMenuItem( MID_REXXWINDOW, FALSE );
            }
            break;

        case MID_SELECTWINDOW:
            if( !selectw.Win ) {
                if( GimmeSelectWindow() == PERR_OK ) {
                    KludgeWindowMenus();
                } else {
                    InternalError("Couldn't open selection window");
                    return HANDLER_QUIT;
                }
            }

            if( !selectw.win ) {
                selectw.win = WindowOpen( selectw.Win );
                CheckMenuItem( MID_SELECTWINDOW, TRUE );
                UpdateIWSelbox(frame);
            } else {
                WindowClose( selectw.Win );
                selectw.win = NULL;
                CheckMenuItem( MID_SELECTWINDOW, FALSE );
            }
            break;



#ifdef DEBUG_MODE
        /*
         *  Debug Menu
         */

        case MID_SAVEMAINPALETTE:
            SaveMainPalette();
            break;

        case MID_MEMCHECK:
            Fortify_OutputAllMemory();
            Fortify_CheckAllMemory();
            break;

        case MID_TESTAR: {
            extern VOID TestAR(VOID);
            BusyAllWindows( globxd );
            TestAR();
            AwakenAllWindows( globxd );
            break;
        }

        case MID_MEMFAIL0:
            Fortify_SetMallocFailRate( 0 );
            break;

        case MID_MEMFAIL10:
            Fortify_SetMallocFailRate( 10 );
            break;

        case MID_MEMFAIL25:
            Fortify_SetMallocFailRate( 25 );
            break;

        case MID_MEMFAIL50:
            Fortify_SetMallocFailRate( 50 );
            break;
#endif

        default:
            // DEBUG("Menu Message %lX received\n",rc);
            break;
    }
    return result;
}

///

/*
    Takes care of correct signal handling from main window. Returns TRUE,
    if we should activate quit signal. Otherwise false.
 */

/// HandleMainIDCMP()

Prototype int HandleMainIDCMP( ULONG );

int HandleMainIDCMP( ULONG rc )
{
    APTR entry;
    static ULONG lastclicked = -1, secs0, ms0;
    ULONG clicked, secs1, ms1;
    FRAME *frame = NULL;

    switch(rc) {
        case WMHI_CLOSEWINDOW:
            WindowClose( framew.Win );
            framew.win = NULL;
            CheckMenuItem( MID_FRAMEWINDOW, FALSE );
            break;

        case GID_MW_LIST: /* Someone is messing with our listview. */

            if( entry = (APTR)FirstSelected( framew.Frames ) ) {
                frame = (FRAME *) FindName( &globals->frames, ParseListEntry(entry) );
            }

            UpdateMainWindow( frame );

            /*
             *  Check for doubleclick
             */

            GetAttr( LISTV_LastClicked, framew.Frames, &clicked );
            if( clicked == lastclicked ) {
                CurrentTime( &secs1, &ms1 );
                if( DoubleClick( secs0, ms0, secs1, ms1 ) && frame ) {
                    ShowDisplayWindow( frame ); /* Make sure it's visible */
                }
            }
            CurrentTime( &secs0, &ms0 );
            lastclicked = clicked;
            break;

        default:
            if(rc > MENUB && rc < 65536)
                return( HandleMenuIDCMP( rc, NULL, FROM_FRAMESWINDOW ) );

            // DEBUG("Message from gadget %lu received\n",rc);
            break;
    }

    return HANDLER_OK;
}

///

/// HandleSelectIDCMP()

/*
    This routine updates the toolbox area display.
    BUG: Should not be here, I think.
    BUG: setting GA_Disable each time is a waste
*/

Prototype void UpdateIWSelbox( FRAME *f );

void UpdateIWSelbox( FRAME *f )
{
    LONG tl, tr, bl, br, cx, cy, cr;
    BOOL  disable;

    if( !selectw.win ) return;

    if( !f ) {
        disable = TRUE;
        tl = tr = bl = br = 0;
    } else {
        if( f->selbox.MinX == ~0 ) {
            disable = FALSE;
            tl = tr = 0;
            bl = f->pix->width-1;
            br = f->pix->height-1;
        } else {
            disable = FALSE;
            tl = f->selbox.MinX;
            tr = f->selbox.MinY;
            bl = f->selbox.MaxX-1;
            br = f->selbox.MaxY-1;
        }
    }

    SetGadgetAttrs( GAD(selectw.TopLeft), selectw.win, NULL,
                    STRINGA_LongVal, tl,
                    GA_Disabled, disable, TAG_DONE );
    SetGadgetAttrs( GAD(selectw.TopRight), selectw.win, NULL,
                    STRINGA_LongVal, tr,
                    GA_Disabled, disable, TAG_DONE );
    SetGadgetAttrs( GAD(selectw.BottomLeft), selectw.win, NULL,
                    STRINGA_LongVal, bl,
                    GA_Disabled, disable, TAG_DONE );
    SetGadgetAttrs( GAD(selectw.BottomRight), selectw.win, NULL,
                    STRINGA_LongVal, br,
                    GA_Disabled, disable, TAG_DONE );

    SetGadgetAttrs( GAD(selectw.Width), selectw.win, NULL,
                    STRINGA_LongVal, abs(bl-tl)+1,
                    GA_Disabled, disable, TAG_DONE );
    SetGadgetAttrs( GAD(selectw.Height), selectw.win, NULL,
                    STRINGA_LongVal, abs(br-tr)+1,
                    GA_Disabled, disable, TAG_DONE );

#ifdef DEBUG_MODE
    if( f ) {
        cx = f->circlex = ((abs(bl-tl)+1)>>1)+tl;
        cy = f->circley = ((abs(br-tr)+1)>>1)+tr;
        cr = f->circleradius = (abs(bl-tl)+1)>>1;
    } else {
        cx = cy = cr = 0;
    }

    SetGadgetAttrs( GAD(selectw.CircleRadius), selectw.win, NULL,
                    STRINGA_LongVal, cr,
                    GA_Disabled, disable, TAG_DONE );

    SetGadgetAttrs( GAD(selectw.CircleX), selectw.win, NULL,
                    STRINGA_LongVal, cx,
                    GA_Disabled, disable, TAG_DONE );

    SetGadgetAttrs( GAD(selectw.CircleY), selectw.win, NULL,
                    STRINGA_LongVal, cy,
                    GA_Disabled, disable, TAG_DONE );
#endif
}

Local
int HandleSelectIDCMP( ULONG rc )
{
    ULONG t;
    FRAME *frame = NULL;
    APTR entry;

    /* BUG: Wastes CPU cycles, as this is not always needed */

    if( entry = (APTR)FirstSelected( framew.Frames ) ) {
        SHLOCKGLOB();
        frame = (FRAME *) FindName( &globals->frames, ParseListEntry(entry) );
        UNLOCKGLOB();
    }

    switch( rc ) {
        case WMHI_CLOSEWINDOW:
            WindowClose( selectw.Win );
            selectw.win = NULL;
            CheckMenuItem( MID_SELECTWINDOW, FALSE );
            break;

        case GID_SELECT_TOPLEFT:
            if( frame ) {
                GetAttr( STRINGA_LongVal, selectw.TopLeft, &t );
                if( t > frame->pix->width ) {
                    t = frame->pix->width;
                }
                RemoveSelectBox( frame );
                frame->selbox.MinX = t;
                ReorientSelbox(&frame->selbox);
                DrawSelectBox( frame, 0L );
                UpdateIWSelbox( frame );
            }
            break;

        case GID_SELECT_TOPRIGHT:
            if( frame ) {
                GetAttr( STRINGA_LongVal, selectw.TopRight, &t );
                if( t > frame->pix->height ) {
                    t = frame->pix->height;
                }
                RemoveSelectBox( frame );
                frame->selbox.MinY = t;
                ReorientSelbox(&frame->selbox);
                DrawSelectBox( frame, 0L );
                UpdateIWSelbox( frame );
            }
            break;

        case GID_SELECT_BOTTOMLEFT:
            if( frame ) {
                GetAttr( STRINGA_LongVal, selectw.BottomLeft, &t );
                ++t;
                if( t > frame->pix->width ) {
                    t = frame->pix->width;
                }
                RemoveSelectBox( frame );
                frame->selbox.MaxX = t;
                ReorientSelbox(&frame->selbox);
                DrawSelectBox( frame, 0L );
                UpdateIWSelbox( frame );
            }
            break;

        case GID_SELECT_BOTTOMRIGHT:
            if( frame ) {
                GetAttr( STRINGA_LongVal, selectw.BottomRight, &t );
                ++t;
                if( t > frame->pix->height ) {
                    t = frame->pix->height;
                }
                RemoveSelectBox( frame );
                frame->selbox.MaxY = t;
                ReorientSelbox(&frame->selbox);
                DrawSelectBox( frame, 0L );
                UpdateIWSelbox( frame );
            }
            break;

        case GID_SELECT_WIDTH:
            if( frame ) {
                LONG newx;

                GetAttr( STRINGA_LongVal, selectw.Width, &t );
                newx = frame->selbox.MinX + t;
                RemoveSelectBox( frame );
                if( newx < frame->pix->width ) {
                    frame->selbox.MaxX = newx;
                } else {
                    frame->selbox.MaxX = frame->pix->width;
                }
                ReorientSelbox( &frame->selbox );
                DrawSelectBox( frame, 0L );
                UpdateIWSelbox( frame );
            }
            break;

        case GID_SELECT_HEIGHT:
            if( frame ) {
                LONG newy;

                GetAttr( STRINGA_LongVal, selectw.Height, &t );
                newy = frame->selbox.MinY + t;
                RemoveSelectBox( frame );
                if( newy < frame->pix->height ) {
                    frame->selbox.MaxY = newy;
                } else {
                    frame->selbox.MaxY = frame->pix->height;
                }
                ReorientSelbox( &frame->selbox );
                DrawSelectBox( frame, 0L );
                UpdateIWSelbox( frame );
            }

            break;

        case GID_SELECT_PAGE:
            if( frame ) {
                GetAttr( PAGE_Active, selectw.Page, &t );
                D(bug("\tNew select method: %lu\n",t));
                RemoveSelectBox( frame );
                if( t ) {
                    D(bug("Changed into circle mode\n"));
                    frame->selectmethod = GINP_LASSO_CIRCLE;
                } else {
                    D(bug("Changed into rectangle mode\n"));
                    frame->selectmethod = GINP_LASSO_RECT;
                }
                DrawSelectBox( frame, 0L );
                UpdateIWSelbox( frame );
            }
            break;

        default:
            if(rc > MENUB && rc < 65536)
                return( HandleMenuIDCMP( rc, NULL, FROM_TOOLWINDOW ));
            break;

    }

    return HANDLER_OK;
}

///

/// HandlePrefsIDCMP()

/*
    Does exactly what the name says: handles all IDCMP messages
    sent by the Prefs window.
    BUG: Display preferences are not handled correctly on Cancel.
    BUG: All values should be read when user hits USE/SAVE
*/

Local
int HandlePrefsIDCMP( ULONG rc )
{
    extern PREFS tmpprefs;
    extern DISPLAY tmpdisp;
    ULONG tmp,m;
    UBYTE *tmp2, buffer[NAMELEN+1];
    PERROR res = PERR_OK;
    BOOL   closed = FALSE;
    APTR   entry;
    struct ToolbarItem *ti;
    static BOOL toolbarchanged = FALSE;

    switch(rc) {

        case GID_PW_TOOLBARLIST:
            if( entry = (APTR)FirstSelected( prefsw.ToolbarList ) ) {
                if(ti = FindInToolbarName(PPTToolbar, entry) ) {
                    SetGadgetAttrs( GAD(prefsw.ToolItemType), prefsw.win, NULL,
                                    MX_Active, ti->ti_Type, TAG_DONE );
                    SetGadgetAttrs( GAD(prefsw.ToolItemFile), prefsw.win, NULL,
                                    STRINGA_TextVal, ti->ti_FileName, NULL );
                } else {
                    InternalError( "Toolbar out of sync!" );
                }
                toolbarchanged = TRUE;
            }
            break;

        case GID_PW_TOTOOLBAR:
            if( entry = (APTR)FirstSelected( prefsw.AvailButtons ) ) {
                struct NewMenu *nm;

                if( nm = FindNewMenuItemName( PPTMenus, entry ) ) {
                    InsertToolItem( PPTToolbar, NULL, nm );
                    RemoveSelected( prefsw.win, prefsw.AvailButtons );
                    toolbarchanged = TRUE;
                }
            }
            break;

        case GID_PW_FROMTOOLBAR:
            if( entry = (APTR)FirstSelected( prefsw.ToolbarList ) ) {
                if(ti = FindInToolbarName(PPTToolbar, entry) ) {
                    RemoveToolItem( PPTToolbar, ti );
                    RemoveSelected( prefsw.win, prefsw.ToolbarList );
                    AddEntrySelect( prefsw.win, prefsw.AvailButtons, entry, LVAP_SORTED );
                    RefreshList( prefsw.win, prefsw.ToolbarList );
                } else {
                    InternalError( "Toolbar out of sync!" );
                }
                toolbarchanged = TRUE;
            }
            break;

        case GID_PW_EXTPRIORITY:
            GetAttr(SLIDER_Level, prefsw.ExtPriority, &tmp);
            tmpprefs.extpriority = tmp;
            break;

        case GID_PW_EXTNICEVAL:
            GetAttr(SLIDER_Level, prefsw.ExtNiceVal, &tmp);
            tmpprefs.extniceval = tmp;
            break;

        case GID_PW_EXTSTACKSIZE:
            GetAttr(STRINGA_LongVal, prefsw.ExtStackSize, &tmp);
            tmpprefs.extstacksize = tmp;
            break;

        case GID_PW_GETFONT:
            BusyAllWindows(globxd);
            if(AslRequestTags(fontreq, ASLFR_Screen, MAINSCR, TAG_DONE )) {
                strcpy(tmpprefs.mfont.ta_Name,fontreq->fo_Attr.ta_Name);
                tmpprefs.mfont.ta_YSize = fontreq->fo_Attr.ta_YSize;
                tmpprefs.mainfont = &(tmpprefs.mfont);
                UpdateFontPrefs(&tmpprefs);
            }
            AwakenAllWindows(globxd);
            break;

        case GID_PW_GETLISTFONT:
            if(AslRequestTags(fontreq, ASLFR_Screen, MAINSCR, TAG_DONE )) {
                strcpy(tmpprefs.lfont.ta_Name,fontreq->fo_Attr.ta_Name);
                tmpprefs.lfont.ta_YSize = fontreq->fo_Attr.ta_YSize;
                tmpprefs.listfont = &(tmpprefs.lfont);
                UpdateFontPrefs(&tmpprefs);
            }
            break;

        case GID_PW_VMDIR:
            GetAttr( STRINGA_TextVal, prefsw.VMDir, (ULONG *)&tmp2 );
            strcpy( tmpprefs.vmdir, tmp2 );
            break;

        case GID_PW_VMBUFSIZE:
            GetAttr( STRINGA_LongVal, prefsw.PageSize, &tmp );
            tmpprefs.vmbufsiz = (ULONG) tmp;
            break;

        case GID_PW_GETVMDIR:
            BusyAllWindows(globxd);
            if(AslRequestTags( filereq, ASLFR_DrawersOnly, TRUE,
                                        ASLFR_InitialDrawer, tmpprefs.vmdir,
                                        ASLFR_Screen, MAINSCR,
                                        TAG_DONE ) )
            {
                strncpy(tmpprefs.vmdir, filereq->fr_Drawer, MAXPATHLEN );
                SetGadgetAttrs( GAD(prefsw.VMDir), prefsw.win, NULL,
                                STRINGA_TextVal, tmpprefs.vmdir, TAG_DONE );
            }
            AwakenAllWindows(globxd);
            break;

        case GID_PW_MAXUNDO:
            GetAttr( STRINGA_LongVal, prefsw.MaxUndo, &m );
            tmpprefs.maxundo = (UWORD)m;
            break;

        case GID_PW_GETDISP:
            if(GetScreenMode( &tmpdisp )) {
                D(bug("Got screen ID %lx, %d x %d x %d\n",tmpdisp.dispid,
                    tmpdisp.width, tmpdisp.height, tmpdisp.depth));

                /*
                 *  Do a CGX check
                 */

                if( CyberGfxBase ) {
                    if( IsCyberModeID( tmpdisp.dispid ) ) {
                        ULONG depth;

                        depth = GetCyberIDAttr( CYBRIDATTR_DEPTH, tmpdisp.dispid );
                        if( depth > 24 ) depth = 24; /* KLUDGE! */
                        if( depth > 8 && tmpdisp.depth < depth ) {
                            tmpdisp.depth = depth;
                            if( DOCONFIRM ) {
                                Req(NEGNUL,NULL,GetStr(mCGX_HI_AND_TRUECOLOR_SCREENS), depth );
                            }
                        }
                    }
                }

                GetNameForDisplayID( tmpdisp.dispid, buffer, NAMELEN );
                SetGadgetAttrs( GAD(prefsw.DispName), prefsw.win, NULL,
                                INFO_TextFormat, buffer, TAG_DONE );
                CheckColorPreview( &tmpdisp );
            } else {
                D(bug("No change\n"));
            }
            break;

        case GID_PW_COLORPREVIEW:
            break;

        case GID_PW_PREVIEWMODE:
            GetAttr( CYC_Active, prefsw.PreviewMode, &tmp );
            tmpprefs.previewmode = tmp;
            SetPreviewSize( &tmpprefs );
            break;

        case GID_PW_CONFIRM:
            GetAttr( GA_Selected, prefsw.Confirm, &tmp );
            tmpprefs.confirm = tmp ? TRUE : FALSE;
            break;

        case GID_PW_FLUSHLIBS:
            GetAttr( GA_Selected, prefsw.FlushLibs, &tmp );
            tmpprefs.expungelibs = tmp ? TRUE : FALSE;
            break;

        case GID_PW_SAVE:
        case GID_PW_USE:
            GetAttr( GA_Selected, prefsw.ColorPreview, &tmp );
            tmpprefs.colorpreview = tmp ? TRUE : FALSE;
            GetAttr( GA_Selected, prefsw.DitherPreview, &tmp );
            tmpprefs.ditherpreview = tmp ? TRUE : FALSE;

            /*
             *  If there was a change in display, try to close the display
             *  and if it succeeds, set up a mark.
             */
            if( memcmp( &tmpdisp, globals->maindisp, sizeof(DISPLAY) ) != 0 ||
                tmpprefs.colorpreview != globals->userprefs->colorpreview ||
                strcmp(tmpprefs.mfontname, globals->userprefs->mfontname) != 0 ||
                strcmp(tmpprefs.lfontname, globals->userprefs->lfontname) != 0 ||
                tmpprefs.mfont.ta_YSize != globals->userprefs->mfont.ta_YSize ||
                tmpprefs.lfont.ta_YSize != globals->userprefs->lfont.ta_YSize )
            {
                if( (res = CloseDisplay()) == PERR_OK )
                    closed = TRUE;
            }

            if( res == PERR_OK ) {
                PREFS kludgeprefs;
                DISPLAY kludgedisp;
                BOOL  kludgevm = FALSE;
                BOOL  redrawframes = FALSE; /* if set, redraws everything */

                /*
                 *  Saves the original preferences, so that if we cannot
                 *  act on them now, they will be restored.
                 */

                if( globals->userprefs->ditherpreview != tmpprefs.ditherpreview ) {
                    redrawframes = TRUE;
                }

                CopyPrefs( globals->userprefs, &kludgeprefs );
                memcpy(&kludgedisp, globals->maindisp, sizeof(DISPLAY));

                if( strcmp( tmpprefs.vmdir, globals->userprefs->vmdir ) != 0 ||
                    tmpprefs.vmbufsiz != globals->userprefs->vmbufsiz)
                {
                    Req( NEGNUL, NULL, GetStr( mSOME_SETTINGS_CANT_BE_USED) );
                    kludgevm = TRUE;
                }

                CopyPrefs( &tmpprefs, globals->userprefs );
                bcopy( &tmpdisp, globals->maindisp, sizeof(DISPLAY) );

                /*
                 *  Toolbar handling.  BUG: Horrible kludge!
                 */

                if( entry = (APTR)DoMethod( prefsw.ToolbarList, LVM_FIRSTENTRY, NULL, 0L ) ) {
                    int item = 0;
                    struct NewMenu *nm;

                    do {
                        nm = FindNewMenuItemName( PPTMenus, entry );
                        if( entry ) {
                            PPTToolbar[item].ti_Type = TIT_TEXT;
                            PPTToolbar[item].ti_GadgetID = (ULONG)nm->nm_UserData;
                            PPTToolbar[item].ti_FileName = NULL;
                            PPTToolbar[item].ti_Label = nm->nm_Label;
                            PPTToolbar[item].ti_Gadget = NULL;
                            item++;
                        }
                        entry = (APTR)DoMethod( prefsw.ToolbarList, LVM_NEXTENTRY, entry, 0L );
                    } while( entry );
                    PPTToolbar[item].ti_Type = TIT_END;
                }

                if( toolbarchanged ) {
                    DisposeObject( toolw.Win );
                    toolw.Win = NULL; toolw.win = NULL;
                }

                if( closed ) {
                    prefsw.win = NULL; // Do not reopen
                    MAINSCR = NULL; /* BUG: Kludge */
                    if(OpenDisplay() != PERR_OK) {
                        /* Restore original screen mode and notify the user */
                        memcpy(globals->maindisp, &kludgedisp, sizeof(DISPLAY) );
                        if( OpenDisplay() != PERR_OK ) {
                            Panic("Unable to open screen and can't even fall back to original!");
                        } else {
                            Req(NEGNUL,NULL, GetStr( mTRY_A_SMALLER_SCREEN ) );
                        }
                    }

                    /*
                     *  Unfortunately, since windows are reopened, we will have
                     *  to set the menus OK again.
                     */

                    KludgeWindowMenus();

                } else {
                    WindowClose( prefsw.Win );
                    prefsw.win = NULL;

                    /* Since there was no need to close the screen, we'll now
                       redraw the windows, if necessary */

                    if( redrawframes ) {
                        struct Node *cn, *nn;

                        cn = globals->frames.lh_Head;
                        while( nn = cn->ln_Succ ) {
                            ((FRAME *)cn)->reqrender = TRUE; /* Force rerender */
                            DisplayFrames( (FRAME *)cn );
                            cn = nn;
                        }
                    }
                }

                if( toolbarchanged ) {
                    HandleMenuIDCMP( MID_TOOLWINDOW, NULL, FROM_PREFSWINDOW );
                    toolbarchanged = FALSE;
                }

                if( rc == GID_PW_SAVE ) {
                    if(SavePrefs( globals, NULL ) != PERR_OK ) {
                        Req(NEGNUL,NULL,GetStr(mCOULDNT_SAVE_PREFERENCES) );
                    }
                }

                if( kludgevm ) {
                    strcpy( globals->userprefs->vmdir, kludgeprefs.vmdir );
                    globals->userprefs->vmbufsiz = kludgeprefs.vmbufsiz;
                }

                return HANDLER_DELETED;
            }
            return 0;

        case GID_PW_CANCEL:
        case WMHI_CLOSEWINDOW:
            WindowClose( prefsw.Win );
            prefsw.win = NULL;
            return HANDLER_DELETED;

        default:
            if(rc > MENUB && rc < 65536)
                return( HandleMenuIDCMP( rc, NULL, FROM_PREFSWINDOW ) );
            break;
    }
    return HANDLER_OK;
}

///

/// HandleInfoIDCMP()
/*
    Infowindow IDCMP handler
*/

Local
int HandleInfoIDCMP( INFOWIN *iw, ULONG rc )
{
    switch(rc) {
        case WMHI_CLOSEWINDOW:
        case GID_IW_CLOSE:
            CloseInfoWindow( iw, globxd );
            break;

        case GID_IW_BREAK:
            HandleMenuIDCMP( MID_BREAK, iw->myframe, FROM_INFOWINDOW );
            break;

        default:
            break;
    }
    return HANDLER_OK;
}
///

/// HandleBackdropIDCMP()
/*
    The backdrop main window.  Does not really do much.  Hum hey, hum hey.
*/
Local
int HandleBackdropIDCMP( ULONG rc )
{
    if(rc > MENUB && rc < 65536)
        return( HandleMenuIDCMP( rc, NULL, FROM_MAINWINDOW ) );

    return HANDLER_OK;
}

///

/*
    Small info window.
    BUG: should also be used for configuring
    BUG: Contains duplicate code.
*/

/// HandleExtInfoIDCMP()

Local
int HandleExtInfoIDCMP( struct ExtInfoWin *ei, ULONG rc )
{
    EXTERNAL *ext;
    APTR entry;
    ULONG clicked, secs1, ms1;
    FRAME *frame = NULL;

//    DEBUG("HandleExtInfoIDCMP(%lu,%lu,%d)\n",rc,*sigmask,type);


    switch(rc) {
        case WMHI_CLOSEWINDOW:
        case GID_XI_CANCEL:
            WindowClose( ei->Win );
            ei->win = NULL;
            CheckMenuItem( ei->menuid, FALSE );
            return HANDLER_OK;

        case GID_XI_INFO:
            if( entry = (APTR)FirstSelected( ei->List ) ) {
                SHLOCKGLOB();
                ext = (EXTERNAL *) FindName( ei->mylist, entry );
                UNLOCKGLOB();
                if(!ext)
                    Panic("HandleExtInfoIDCMP: Lost list contents!");
                ShowExtInfo( globxd, ext, globals->maindisp->win );
            }
            break;

        case GID_XI_EXEC:

            if( entry = (APTR)FirstSelected( framew.Frames ) ) {
                SHLOCKGLOB();
                frame = (FRAME *) FindName( &globals->frames, ParseListEntry(entry) );
                UNLOCKGLOB();
            }

            if( entry = (APTR)FirstSelected( ei->List ) ) {
                char str[80], *path = NULL;
                struct Library *IOModuleBase;

                SHLOCKGLOB();
                ext = (EXTERNAL *) FindName( ei->mylist, entry );
                UNLOCKGLOB();

                switch( ei->type ) {
                    case NT_LOADER:

                        if(IOModuleBase = OpenModule(ext,globxd)) {
                            BOOL nofile;
                            STRPTR pat;

                            nofile = (BOOL) IOInquire( PPTX_NoFile, globxd );
                            pat    = (STRPTR) IOInquire( PPTX_PostFixPattern, globxd );
                            if( nofile == FALSE ) {
                                SetAttrs(gvLoadFileReq.Req,
                                         ASLFR_InitialPattern, pat,
                                         ASLFR_DoPatterns, TRUE,
                                         TAG_DONE);
                                if(DoRequest(gvLoadFileReq.Req) != FRQ_OK) {
                                    CloseModule( IOModuleBase, globxd );
                                    break;
                                }
                                GetAttr(FRQ_Path, gvLoadFileReq.Req, (ULONG*) &path);
                            } else {
                                path = NULL;
                            }
                            CloseModule(IOModuleBase, globxd);
                        }

                        RunLoad( (char *)path, entry, NULL);

                        SetAttrs(gvLoadFileReq.Req,
                                 ASLFR_InitialPattern, "#?",
                                 ASLFR_DoPatterns, FALSE,
                                 TAG_DONE);

                        break;

                    case NT_EFFECT:
                        if( FrameFree( frame ) ) {
                            sprintf(str, "NAME=\"%s\"", entry);
                            RunFilter( frame, str );
                        }
                        break;

                    case NT_SCRIPT:
                        if( frame ) {
                            sprintf(str,"%s %d", ext->diskname, frame->ID);
                        } else {
                            sprintf(str,"%s",ext->diskname);
                        }
                        if( !SendRexxCommand( rxhost, str, NULL ) )
                            Req(NEGNUL,NULL,GetStr(mFAILED_TO_START_SCRIPT) );
                        break;
                }

                if( loader_close && (ei->type == NT_LOADER) ) {
                    return HandleExtInfoIDCMP( ei, WMHI_CLOSEWINDOW );
                }
            }

            break;

        case GID_XI_LIST:

            GetAttr( LISTV_LastClicked, ei->List, &clicked );
            if( clicked == ei->last ) {
                CurrentTime( &secs1, &ms1 );
                if( DoubleClick( ei->secs, ei->ms, secs1, ms1 ) ) {
                    return( HandleExtInfoIDCMP( ei, GID_XI_EXEC ) );
                }
            }
            CurrentTime( &ei->secs, &ei->ms );
            ei->last = clicked;

            if( entry = (APTR)FirstSelected( ei->List ) ) {
                SHLOCKGLOB();
                ext = (EXTERNAL *) FindName( ei->mylist, entry );
                UNLOCKGLOB();

                switch( ei->type ) {
                    case NT_LOADER:
                        if( ((LOADER *)ext)->canload )
                            SetGadgetAttrs( GAD(ei->Exec), ei->win, NULL, GA_Disabled, FALSE, TAG_DONE );
                        else
                            SetGadgetAttrs( GAD(ei->Exec), ei->win, NULL, GA_Disabled, TRUE, TAG_DONE );
                        break;

                    case NT_EFFECT:
                        break;

                    case NT_SCRIPT:
                        break;
                }
            }


            /* BUG: Should check if config is allowed, then activate gadget */
            break;

        case GID_XI_CONFIG:
        default:
            if(rc > MENUB && rc < 65536)
                return( HandleMenuIDCMP( rc, NULL, (ei->type == NT_LOADER) ?
                                        FROM_LOADERINFO : (ei->type == NT_EFFECT ) ?
                                        FROM_EFFECTINFO : FROM_REXXWINDOW ) );
            break;
    }
    return HANDLER_OK;
}

///

/*
    This function turns the box around so that the top left
    co-ordinates are the smallest ones.
 */

Prototype VOID ReorientSelbox( struct Rectangle *r );

VOID ReorientSelbox( struct Rectangle *r )
{
    WORD t;

    if(r->MaxX < r->MinX ) {
        t = r->MaxX;
        r->MaxX = r->MinX;
        r->MinX = t;
    }
    if(r->MaxY < r->MinY ) {
        t = r->MaxY;
        r->MaxY = r->MinY;
        r->MinY = t;
    }
}

/*
    This function calculates where the current mouse coordinates are
    using image coordinates and returns TRUE, if the mouse
    is within the image.
 */

Prototype BOOL CalcMouseCoords( FRAME *, WORD, WORD, WORD *, WORD * );

BOOL CalcMouseCoords( FRAME *frame, WORD mousex, WORD mousey, WORD *x, WORD *y )
{
    BOOL isin = TRUE;
    WORD ht, wt, xloc, yloc;
    struct IBox *ibox;

    GetAttr( AREA_AreaBox, frame->disp->RenderArea, (ULONG *)&ibox );

    xloc = frame->zoombox.Left + ((mousex - ibox->Left) * (LONG)(frame->zoombox.Width)) / (ibox->Width);
    yloc = frame->zoombox.Top + ((mousey - ibox->Top) * (LONG)(frame->zoombox.Height)) / (ibox->Height);

    wt = frame->zoombox.Left+frame->zoombox.Width;
    ht = frame->zoombox.Top+frame->zoombox.Height;

    if(xloc < frame->zoombox.Left )  { xloc = frame->zoombox.Left; isin = FALSE; }
    if(yloc < frame->zoombox.Top  )  { yloc = frame->zoombox.Top; isin = FALSE; }
    if(xloc > wt) { xloc = wt; isin = FALSE; }
    if(yloc > ht) { yloc = ht; isin = FALSE; }

    *x = xloc; *y = yloc;

    return isin;
}


/// HandleQDispWinIDCMP()

/*
    Handle Quickdisplaywindow IDCMP codes.
    selstatus bits:
    bits 7..2 unused
    bit  1    == 1, if button is down; 0, if button is up
    bit  0    == 1, if the display rectangle is visible, 0 otherwise.

    BUG: Get rid of __mulu and __divu() calls
*/

Local
int HandleQDispWindowIDCMP( FRAME *frame, ULONG rc )
{
    struct Rectangle *sb = &frame->selbox;
    DISPLAY *d = frame->disp;
    WORD xloc, yloc;
    WORD mousex, mousey;
    BOOL isin = TRUE;
    APTR   dropentry;
    ULONG top, left;

//    D(bug("HandleQ(%lu)\n",rc));

    /* Sanity checks.  We seem to be getting Enforcer hits without
       these! */

    if( d == NULL ) return 0;
    if( d->win == NULL ) return 0;

    handleMsg:

    mousex = d->win->MouseX;
    mousey = d->win->MouseY;

    switch( rc ) {
        case WMHI_CLOSEWINDOW:
            return HandleMenuIDCMP( MID_CLOSE, frame, FROM_DISPLAYWINDOW );

        case WMHI_INACTIVE:
            /*
             *  Make sure the select box is on the window
             */
            if( sb->MinX != ~0 && !(frame->selstatus & SELF_RECTANGLE) ) {
                DrawSelectBox( frame, 0L );
            }
            ClearMouseLocation();
            break;

        case WMHI_ACTIVE:
            isin = CalcMouseCoords( frame, mousex, mousey, &xloc, &yloc );
            UpdateMouseLocation( xloc, yloc );
            DoMainList( frame );
            UpdateMainWindow( frame );
            break;

        case GID_DW_HIDE:
            return HandleMenuIDCMP( MID_HIDE, frame, FROM_DISPLAYWINDOW );

        /*
         *  This method is called whenever the window has been damaged and
         *  needs to be redrawn or someone dropped a frame name on the window
         */

        case GID_DW_AREA:

            GetAttr( AREA_DropEntry, d->RenderArea, (ULONG *) &dropentry );

            if( dropentry ) {
                FRAME *drop;

                drop = (FRAME *)FindName( &(globals->frames), ParseListEntry(dropentry) );

                SetAttrs( d->RenderArea, AREA_DropEntry, NULL, TAG_END );

                AreaDrop( frame, drop );
            }

            frame->reqrender = TRUE; /* Damage has happened */

            if(rc = DisplayFrame( frame ))
                goto handleMsg;

            break;

        case GID_DW_BOTTOMPROP:
            break;

        case GID_DW_RIGHTPROP:
            break;

        case GID_DW_LOCATION:
            GetAttr( PGA_Top, d->GO_BottomProp, &left );
            GetAttr( PGA_Top, d->GO_RightProp,  &top  );

            if( left != frame->zoombox.Left ||
                top  != frame->zoombox.Top )
            {
                D(bug("New LOCATION!\n"));
                RemoveSelectBox( frame );
                frame->zoombox.Left = left;
                frame->zoombox.Top  = top;
                DisplayFrame( frame );
            }
            break;

#ifdef FLASHING_DISPRECT

        /*
         *  Note that thanks to our custom hook, we will receive INTUITICKS
         *  only when the button is NOT being held down.
         */

        case GID_DW_INTUITICKS:
            if( !IsFrameBusy( frame ) ) {
                if( sb->MinX != ~0 ) {

                    /*
                     *  This kludge tells RemoveSelectBox that it should not
                     *  erase the corner handles
                     */

                    if( frame->selstatus & SELF_BUTTONDOWN ) {
                        RemoveSelectBox( frame );
                    } else {
                        frame->selstatus |= SELF_BUTTONDOWN;
                        RemoveSelectBox( frame );
                        frame->selstatus &= ~SELF_BUTTONDOWN;
                    }

                    if( (frame->disp->selpt >>= 1) == (0xF0F0F0F0>>8) )
                        frame->disp->selpt = 0xF0F0F0F0;
                    DrawSelectBox( frame, DSBF_INTERIM );
                }
            }
            break;
#endif

        case GID_DW_SELECTDOWN:
            isin = CalcMouseCoords( frame, mousex, mousey, &xloc, &yloc );
            if(!isin) break;
            DW_ButtonDown(frame,mousex,mousey,xloc,yloc);
            break;

        case GID_DW_CONTROLSELECTDOWN:
            isin = CalcMouseCoords( frame, mousex, mousey, &xloc, &yloc );
            if(isin) {
                DW_ControlButtonDown(frame,mousex,mousey,xloc,yloc);
            }
            break;

        case GID_DW_SELECTUP:
            isin = CalcMouseCoords( frame, mousex, mousey, &xloc, &yloc );
            DW_ButtonUp(frame,xloc,yloc);
            break;

        case GID_DW_MOUSEMOVE:
            isin = CalcMouseCoords( frame, mousex, mousey, &xloc, &yloc );
            DW_MouseMove(frame,xloc,yloc);
            break;

        default:
            /*
             *  It may happen that the button is still down while the menu
             *  gets disturbed. In this case we'll use this as the select location
             */
            isin = CalcMouseCoords( frame, mousex, mousey, &xloc, &yloc );

            if(rc > MENUB && rc < 65536) {
                if( frame->selstatus & SELF_BUTTONDOWN ) {
                    sb->MaxX = xloc; sb->MaxY = yloc;
                    frame->selstatus &= ~(SELF_BUTTONDOWN);
                    D(bug("Marked panic select end (%d,%d)\n",xloc,yloc));
                }

                return( HandleMenuIDCMP( rc, frame, FROM_DISPLAYWINDOW ));
            }

            break;

    }

    return HANDLER_OK;
}

///

/*
 *  Edit window
 */

/// HandleEditIDCMP()

int HandleEditIDCMP( EDITWIN *ew, ULONG rc )
{
    STRPTR s;
    APTR entry;
    struct Extension *en;

    switch(rc) {
        case WMHI_CLOSEWINDOW:
        case GID_EDIT_CANCEL:
        case GID_EDIT_OK:
            CloseEditWindow( ew );
            return HANDLER_DELETED;

        case WMHI_ACTIVE:
            DoMainList( ew->frame );
            UpdateMainWindow( ew->frame );
            break;

        case GID_EDIT_EXTNEW:
            s = GetStr(mEXTENSION_EMPTY);
            entry = GetStr(mEXTENSION_NEW);
            if(AddExtension( ew->frame, entry, s, strlen(s)+1, EXTF_CSTRING, globxd ) == PERR_OK ) {
                SetGadgetAttrs(GAD(ew->ExtName), ew->win, NULL, GA_Disabled, FALSE, STRINGA_TextVal, entry, TAG_DONE);
                SetGadgetAttrs(GAD(ew->ExtValue), ew->win, NULL, GA_Disabled, FALSE, STRINGA_TextVal, s, TAG_DONE);
                SetGadgetAttrs(GAD(ew->ExtRemove), ew->win, NULL, GA_Disabled, FALSE, TAG_DONE );
                SetGadgetAttrs(GAD(ew->ExtOk), ew->win, NULL, GA_Disabled, FALSE, TAG_DONE );
                AddEntrySelect(ew->win, ew->ExtList, entry, LVAP_SORTED );
                RefreshList( ew->win, ew->ExtList );
            } else {
                InternalError("Failed to add!");
            }
            break;

        case GID_EDIT_EXTREMOVE:
            GetAttr( STRINGA_TextVal, ew->ExtName, (ULONG *) &s );
            if( !DOCONFIRM || Req( GetFrameWin(ew->frame), GetStr(mREMOVE_CANCEL),
                                   GetStr(mSURE_TO_REMOVE_EXTENSION) ) ) {

                RemoveExtension( ew->frame, s, globxd );
                RemoveSelected( ew->win, ew->ExtList );
                SetGadgetAttrs(GAD(ew->ExtRemove), ew->win, NULL, GA_Disabled, TRUE, TAG_DONE );
                SetGadgetAttrs(GAD(ew->ExtOk), ew->win, NULL, GA_Disabled, TRUE, TAG_DONE );
                SetGadgetAttrs(GAD(ew->ExtName), ew->win, NULL, GA_Disabled, TRUE, STRINGA_TextVal, "", TAG_DONE );
                SetGadgetAttrs(GAD(ew->ExtValue), ew->win, NULL, GA_Disabled, TRUE, STRINGA_TextVal, "", TAG_DONE );
                SetGadgetAttrs(GAD(ew->ExtList), ew->win, NULL, LISTV_DeSelect, LISTV_Select_All, TAG_DONE );
            }
            break;

        case GID_EDIT_EXTLIST:
            if( entry = (APTR) FirstSelected( ew->ExtList ) ) {
                D(bug("\tUser clicked on '%s'\n",entry));

                if( en = FindExtension( ew->frame, entry, globxd ) ) {
                    SetGadgetAttrs(GAD(ew->ExtName), ew->win, NULL, GA_Disabled, FALSE, STRINGA_TextVal, entry, TAG_DONE);
                    SetGadgetAttrs(GAD(ew->ExtRemove), ew->win, NULL, GA_Disabled, FALSE, TAG_DONE );
                    SetGadgetAttrs(GAD(ew->ExtOk), ew->win, NULL, GA_Disabled, FALSE, TAG_DONE );
                    if( en->en_Flags & EXTF_CSTRING ) {
                        STRPTR buf;

                        buf = (STRPTR)smalloc( en->en_Length << 1 );
                        if( buf ) {
                            ConvertExtensionToC(en->en_Data, buf, en->en_Length<<1);
                            SetGadgetAttrs(GAD(ew->ExtValue), ew->win, NULL, GA_Disabled, FALSE, STRINGA_TextVal, buf, TAG_DONE);
                            sfree(buf);
                        }
                    } else {
                        SetGadgetAttrs(GAD(ew->ExtValue), ew->win, NULL, GA_Disabled, TRUE, STRINGA_TextVal, GetStr(mEXTENSION_UNEDITABLE), TAG_DONE );
                    }
                } else {
                    InternalError("ExtList out of sync!");
                }
            }
            break;

        case GID_EDIT_EXTNAME:
            break;
        case GID_EDIT_EXTVALUE:
            break;

        case GID_EDIT_EXTOK:
            if( entry = (APTR) FirstSelected( ew->ExtList ) ) {
                if( en = FindExtension( ew->frame, entry, globxd ) ) {
                    STRPTR buf;

                    GetAttr( STRINGA_TextVal, ew->ExtName, (ULONG *)&s );
                    if(ReplaceExtensionName( en, s, globxd ) == PERR_OK ) {
                        ReplaceEntry( ew->win, ew->ExtList, entry, s );
                        SortList( ew->win, ew->ExtList );
                    }

                    GetAttr( STRINGA_TextVal, ew->ExtValue, (ULONG *) &s );
                    if( buf = smalloc( strlen(s)*2 ) ) {
                        ConvertCToExtension( s, buf, strlen(s)*2 );
                        ReplaceExtensionData( en, buf, strlen(buf)+1, globxd );
                        sfree(buf);
                    }
                }
                SetGadgetAttrs(GAD(ew->ExtRemove), ew->win, NULL, GA_Disabled, TRUE, TAG_DONE );
                SetGadgetAttrs(GAD(ew->ExtOk), ew->win, NULL, GA_Disabled, TRUE, TAG_DONE );
                SetGadgetAttrs(GAD(ew->ExtName), ew->win, NULL, GA_Disabled, TRUE, STRINGA_TextVal, "", TAG_DONE );
                SetGadgetAttrs(GAD(ew->ExtValue), ew->win, NULL, GA_Disabled, TRUE, STRINGA_TextVal, "", TAG_DONE );
            } else {
                InternalError("Extlist out of sync!");
            }
            break;

        default:
            if(rc > MENUB && rc < 65536)
                return( HandleMenuIDCMP( rc, ew->frame, FROM_EDITWINDOW ));
            break;
    }

    return HANDLER_OK;
}

///

/*
 *  Handle the palette window IDCMP
 */

/// HandlePaletteWindowIDCMP()

int HandlePaletteWindowIDCMP( FRAME *frame, ULONG rc )
{
    struct PaletteWindow *pw = frame->pw;
    ULONG color;
    ULONG r,g,b,a;
    COLORMAP *colortable;

    switch(rc) {
        case GID_PAL_OK:
            /* BUG: Should check integrity of pointers */
            memcpy( frame->disp->colortable, pw->colortable, sizeof(COLORMAP) * frame->disp->ncolors );
            /* FALLTHROUGH */

        case WMHI_CLOSEWINDOW:
        case GID_PAL_CANCEL:
            /* Return original colormap */
            LoadRGB8( &(pw->scr->ViewPort), frame->disp->colortable, frame->disp->ncolors, globxd );
            ClosePaletteWindow( frame );
            return HANDLER_DELETED;

        case GID_PAL_REMAP:
            memcpy( frame->disp->colortable, pw->colortable, sizeof(COLORMAP) * frame->disp->ncolors );
            frame->disp->saved_cmap = frame->disp->cmap_method;
            frame->disp->cmap_method = CMAP_FORCEOLDPALETTE;
            DoRender(frame->renderobject);
            break;

        case WMHI_ACTIVE:
        case GID_PAL_PALETTE:
            /*
             *  BUG: This is dubious. Should we use the frame display
             *  or the real display colormap from VPort?
             */

            colortable = pw->colortable;

            GetAttr( PALETTE_CurrentColor, pw->Palette, &color );
            // D(bug("\tUser selected color %lu\n",color));
            a = (ULONG)colortable[color].a;
            r = (ULONG)colortable[color].r;
            g = (ULONG)colortable[color].g;
            b = (ULONG)colortable[color].b;

            SetGadgetAttrs( GAD(pw->Slider1), pw->win, NULL, SLIDER_Level, r, TAG_END );
            SetGadgetAttrs( GAD(pw->Slider2), pw->win, NULL, SLIDER_Level, g, TAG_END );
            SetGadgetAttrs( GAD(pw->Slider3), pw->win, NULL, SLIDER_Level, b, TAG_END );
            SetGadgetAttrs( GAD(pw->Slider4), pw->win, NULL, SLIDER_Level, a, TAG_END );

            break;

        case GID_PAL_SLIDER1: // Red
            colortable = pw->colortable;

            GetAttr( PALETTE_CurrentColor, pw->Palette, &color );
            GetAttr( SLIDER_Level, pw->Slider1, &r );
            colortable[color].r = r;
            g = (ULONG)colortable[color].g;
            b = (ULONG)colortable[color].b;

            SetRGB8( &(pw->scr->ViewPort), color, r, g, b, globxd );

            break;

        case GID_PAL_SLIDER2: // Green
            colortable = pw->colortable;

            GetAttr( PALETTE_CurrentColor, pw->Palette, &color );
            GetAttr( SLIDER_Level, pw->Slider2, &g );
            colortable[color].g = g;
            r = (ULONG)colortable[color].r;
            b = (ULONG)colortable[color].b;

            SetRGB8( &(pw->scr->ViewPort), color, r, g, b, globxd );

            break;

        case GID_PAL_SLIDER3: // Blue
            colortable = pw->colortable;

            GetAttr( PALETTE_CurrentColor, pw->Palette, &color );
            GetAttr( SLIDER_Level, pw->Slider3, &b );
            colortable[color].b = b;
            g = (ULONG)colortable[color].g;
            r = (ULONG)colortable[color].r;

            SetRGB8( &(pw->scr->ViewPort), color, r, g, b, globxd );
            break;

        case GID_PAL_SLIDER4: // Alpha
            GetAttr( PALETTE_CurrentColor, pw->Palette, &color );
            GetAttr( SLIDER_Level, pw->Slider4, &a );
            pw->colortable[color].a = a;
            break;

        default:
            if(rc > MENUB && rc < 65536)
                return( HandleMenuIDCMP( rc, frame, FROM_PALETTEWINDOW ));
            break;

    }

    return HANDLER_OK;
}

///

/*
    BUG: Should set the maxcolors amount for future rendering, when we have a
         ham/ham8/ehb display
*/
Local
VOID UpdateDispPrefsWindow( FRAME *frame )
{
    struct DispPrefsWindow *dpw = frame->dpw;
    ULONG type, ncolors;

    GetAttr( CYC_Active, dpw->GO_Type, &type );
    GetAttr( SLIDER_Level, dpw->GO_NColors, &ncolors );

    switch(type) {
        case 0: /* colormapped */
            // SetGadgetAttrs( GAD(dpw->GO_NColors), dpw->win, NULL, SLIDER_Max, 256, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_NColors), dpw->win, NULL, GA_Disabled, FALSE, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_NColorsI), dpw->win, NULL, INDIC_Level, ncolors, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_Dither), dpw->win, NULL, GA_Disabled, FALSE, TAG_END );
            break;

        case 1: /* EHB */
            SetGadgetAttrs( GAD(dpw->GO_NColors), dpw->win, NULL, GA_Disabled, TRUE, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_NColorsI), dpw->win, NULL, INDIC_Level, 64,  TAG_END );
            // SetGadgetAttrs( GAD(dpw->GO_Dither), dpw->win, NULL, GA_Disabled, TRUE, TAG_END );
            break;

        case 2: /* HAM6 */
            // SetGadgetAttrs( GAD(dpw->GO_NColors), dpw->win, NULL, SLIDER_Max, 16, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_NColors), dpw->win, NULL, GA_Disabled, TRUE, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_NColorsI), dpw->win, NULL, INDIC_Level, 16, TAG_END );
            // SetGadgetAttrs( GAD(dpw->GO_Dither), dpw->win, NULL, GA_Disabled, TRUE, TAG_END );
            break;

        case 3: /* HAM8 */
            // SetGadgetAttrs( GAD(dpw->GO_NColors), dpw->win, NULL, SLIDER_Max, 64, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_NColors), dpw->win, NULL, GA_Disabled, TRUE, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_NColorsI), dpw->win, NULL, INDIC_Level, 64, TAG_END );
            // SetGadgetAttrs( GAD(dpw->GO_Dither), dpw->win, NULL, GA_Disabled, TRUE, TAG_END );
            break;

        case 4: /* TrueColor BUG: Is here just for completeness. */
            SetGadgetAttrs( GAD(dpw->GO_NColors), dpw->win, NULL, GA_Disabled, TRUE, TAG_END );
            SetGadgetAttrs( GAD(dpw->GO_NColorsI), dpw->win, NULL, INDIC_Level, 0, TAG_END );
            break;

    }
}

/*
    BUG: This could be cleaned somewhat, since the display prefs have
         changed.
*/

/// HandleDispPrefsWindowIDCMP()

Local
int HandleDispPrefsWindowIDCMP( FRAME *frame, ULONG rc )
{
    struct DispPrefsWindow *dpw = frame->dpw;
    struct ScreenModeRequester *req;
    BOOL res;
    ULONG tmp;
    UBYTE *path, buffer[NAMELEN+1];
    DISPLAY *d = dpw->tempdisp;

    switch(rc) {
        case WMHI_CLOSEWINDOW:
        case GID_DP_CANCEL:
            //WindowClose( dpw->WO_Win );
            DisposeObject( dpw->WO_Win );
            dpw->win = NULL;
            dpw->WO_Win = NULL;
            FreeDisplay( dpw->tempdisp, globxd );
            dpw->tempdisp = NULL;
            return HANDLER_DELETED;

        case WMHI_ACTIVE:
            DoMainList( frame );
            break;

        case GID_DP_GETSCRMODE:

            req = AllocAslRequest( ASL_ScreenModeRequest, NULL );

            res = AslRequestTags( req,
                ASLSM_Screen, MAINSCR,
                ASLSM_TitleText,            GetStr(mSELECT_SCREEN_MODE),
                ASLSM_InitialDisplayID,     d->dispid,
                ASLSM_InitialDisplayWidth,  d->width,
                ASLSM_InitialDisplayHeight, d->height,
                ASLSM_InitialDisplayDepth,  d->depth,
                ASLSM_MaxDepth,             8, /* Filter out deep screenmodes */
                TAG_END );

            if( res ) {
                d->dispid = req->sm_DisplayID;
                // frame->disp->width  = req->sm_DisplayWidth;
                // frame->disp->height = req->sm_DisplayHeight;

                GetNameForDisplayID( d->dispid, buffer, NAMELEN );

                SetGadgetAttrs( GAD(dpw->GO_ScrMode), dpw->win, NULL,
                                INFO_TextFormat, buffer, TAG_END );
            }

            FreeAslRequest( req );
            break;

        case GID_DP_OKRENDER:
        case GID_DP_OK:
            GetAttr( CYC_Active, dpw->GO_Palette, &tmp );
            d->cmap_method = (UBYTE) tmp;
            GetAttr( CYC_Active, dpw->GO_Screen, &tmp );
            d->type = DISPLAY_CUSTOMSCREEN;

            /*
             *  Read attributes for custom screens, if necessary.
             */

            if( d->type == DISPLAY_CUSTOMSCREEN ) {
                GetAttr( SLIDER_Level, dpw->GO_NColors, &tmp );
                d->ncolors = tmp;
                D(bug("New amount of colors is %lu\n",tmp));
                d->depth = GetMinimumDepth( tmp );

                GetAttr( CYC_Active, dpw->GO_Type, &tmp );
                switch(tmp) {
                    case 0: /* Normal color */
                        d->renderq = RENDER_NORMAL;
                        d->dispid &= ~(HAM_KEY);
                        break;
                    case 1: /* EHB */
                        d->renderq = RENDER_EHB;
                        d->dispid |= EXTRAHALFBRITE_KEY;
                        d->dispid &= ~(HAM_KEY);
                        d->ncolors = 32;
                        d->depth   = 6;
                        break;
                    case 2: /* HAM */
                        d->renderq = RENDER_HAM6;
                        d->dispid  |= HAM_KEY;
                        d->ncolors = 16;
                        d->depth   = 6;
                        break;
                    case 3: /* HAM8 */
                        d->renderq = RENDER_HAM8;
                        d->dispid  |= HAM_KEY;
                        d->depth   = 8;
                        d->ncolors = 64;
                        break;
                }

                GetAttr( CYC_Active, dpw->GO_Dither, &tmp );
                d->dither = (tmp == 1) ? DITHER_FLOYD : DITHER_NONE;
                D(bug("New dither mode is %lu\n",tmp));
                GetAttr( GA_Selected, dpw->GO_ForceBW, &tmp );
                d->forcebw = (BOOL)tmp;
                GetAttr( GA_Selected, dpw->GO_DrawAlpha, &tmp );
                d->drawalpha = (BOOL)tmp;
            } else {
                d->renderq = RENDER_QUICK;
            }

            WindowClose( dpw->WO_Win );
            dpw->win = NULL;

            CopyDisplay( d, frame->disp );
            FreeDisplay( d, globxd );
            dpw->tempdisp = NULL;

            /*
             *  Check for render and do!
             */

            if( rc == GID_DP_OKRENDER ) {
                if( FrameFree(frame) ) {
                    if( frame->disp->renderq == RENDER_QUICK)
                        DisplayFrame(frame);
                    else {
                        if(OpenRender( frame, globxd ) == PERR_OK)
                            DoRender( frame->renderobject );
                    }
                }
            }

            return HANDLER_DELETED;

        case GID_DP_TYPE:
            UpdateDispPrefsWindow( frame );
            break;

        case GID_DP_GETPALETTENAME:
            if( DoRequest( gvPaletteOpenReq.Req ) == FRQ_OK ) {
                GetAttr( FRQ_Path, gvPaletteOpenReq.Req, (ULONG*)&path );
                strncpy( d->palettepath, path, 256 );
                SetGadgetAttrs( GAD(dpw->GO_PaletteName), dpw->win, NULL,
                                STRINGA_TextVal, path, TAG_END );
            }
            break;

        case GID_DP_PALETTENAME:
            break;

        default:
            if(rc > MENUB && rc < 65536)
                return( HandleMenuIDCMP( rc, frame, FROM_DISPPREFSWINDOW ));
            break;

    }
    return HANDLER_OK;
}

///

/*
    Handles any PPT internal messages from the other tasks.

    Returns a pointer to the frame, if the displays should
    be refreshed, otherwise NULL.
*/

/// HandleSpecialIDCMP()

Local
FRAME *HandleSpecialIDCMP( struct PPTMessage *mymsg )
{
    PPTREXXARGS *ra = NULL;
    FRAME *currframe = NULL, *newframe;
    INFOWIN *iw = NULL;
    char buff[16];

    switch(mymsg->code) {

        /*
         *  A render has been completed.
         */

        case PPTMSG_RENDERDONE:
            ra = FindRexxWaitItem( mymsg );
            currframe = mymsg->frame;

            if( !currframe ) break;

            CheckPtr( currframe, "main(): render" );
            D(bug("\tFrame %08X reports render complete\n",currframe));
            if( (PERROR)mymsg->data == PERR_OK) {
                struct RenderObject *rdo;

                /*
                 *  When started manually, the render will be shown.
                 *  If started from AREXX, then it will not be shown.
                 */

                rdo = currframe->renderobject;
                if(ra) {
                    ra->rc = ra->rc2 = 0;
                } else {
                    if( rdo && rdo->ActivateDisplay )
                        (*rdo->ActivateDisplay)( rdo );
                    if( currframe->pw && rdo ) {
                        /*
                         *  This must be done so that the palette object
                         *  realizes something has been done with it
                         */

                        ClosePaletteWindow( currframe );
                        OpenPaletteWindow( currframe );

                        if(currframe->disp->saved_cmap != CMAP_NONE) {
                            currframe->disp->cmap_method = currframe->disp->saved_cmap;
                            currframe->disp->saved_cmap = CMAP_NONE;
                        }
                    }
                }
            } else {
                if(ra) {
                    ra->rc = -10;
                    ra->rc2 = (LONG) GetErrorMsg( currframe, globxd );
                }
            }
            ReleaseFrame( currframe );
            ClearProgress( currframe, globxd );
            SetFrameStatus( currframe, 0 );
            break;

        /*
         *  An effect has been completed.
         */

        case PPTMSG_EFFECTDONE:
            D(bug("\tFrame @ addr %08X just died!\n",mymsg->frame));

            /* Check if the frame exists (an error during message parsing could
               cause this not to exist! */

            if( !mymsg->frame ) break;

            /* Determine the rexx wait item and clear any pending input requests */

            ra = FindRexxWaitItem( mymsg );

            ClearFrameInput( mymsg->frame );

            /*
             *  Did we succeed?
             */

            switch( ((struct EffectMessage *)mymsg)->em_Status ) {
                case EMSTATUS_NEWFRAME:
                    /* A new frame was generated in response to the
                       request */
                    newframe = ((struct EffectMessage *)mymsg)->em_NewFrame;
                    D(bug("\t\t...but it seems he spawned a child!\n"));
                    ReplaceFrame( mymsg->frame, newframe );
                    UpdateInfoWindow( newframe->mywin,globxd);
                    UpdateMainWindow( newframe );
                    currframe = newframe;
                    currframe->reqrender = 1;
                    if(ra) {
                        ra->rc = ra->rc2 = 0;
                    }
                    break;

                case EMSTATUS_NOCHANGE:
                    /* No change happened to the frame, but we were still
                       successfull */
                    D(bug("\t\t...and there was no change\n"));
                    currframe = mymsg->frame;
                    if( ra ) {
                        ra->rc = ra->rc2 = 0;
                    }
                    break;

                case EMSTATUS_FAILED:
                    /* Do the cleanup */
                    if(CheckPtr( mymsg->frame, "main(): Effect done, failed" )) {
                        D(bug("\t\t...cleaning up the mess\n"));
                        currframe = mymsg->frame;

                        /*
                         *  If this was a rexx message, pass the message down to
                         *  the REXX program who started the whole thing. Otherwise
                         *  we check if the error message has already been shown
                         *  and then set up the display ourselves.
                         */

                        if(ra) {
                            ra->rc  = -10;
                            D(bug("\terror msg: %s\n",GetErrorMsg(currframe,globxd)));
                            ra->rc2 = (LONG) GetErrorMsg( currframe, globxd );
                        } else {
                            if( currframe->doerror ) {
                                ShowError( currframe,globxd );
                            }
                        }
                    }
            }

            if(CheckPtr( currframe, "main(): currframe" )) {
                ReleaseFrame( currframe );
                SetFrameStatus( currframe, 0 );
                RemoveSimpleAttachments( currframe );
                CloseRender( currframe,globxd ); /* BUG: Maybe not here? */
            }

            break;

        /*
         *  A frame has been saved. Not much to do.
         */

        case PPTMSG_SAVEDONE:
            currframe = mymsg->frame;

            if(!currframe) break;

            ra = FindRexxWaitItem( mymsg );

            if(CheckPtr( currframe, "main(): save" )) {
                D(bug("\tFrame %08X reports save complete\n",currframe));
                ReleaseFrame( currframe );
                ClearProgress( currframe, globxd );

                /*
                 *  Take care of REXX messages
                 */

                if( ra ) {
                    if( mymsg->data == (APTR)PERR_OK ) {
                        ra->rc = ra->rc2 = 0;
                    } else {
                        ra->rc  = -10;
                        ra->rc2 = (LONG) ErrorMsg((PERROR)mymsg->data,globxd);
                    }
                }


                /*
                 *  If the user changed the name, we need to refresh the
                 *  display just in case.
                 */

                UpdateFrameInfo( currframe );
                RefreshFrameInfo( currframe, globxd );
                SetFrameStatus( currframe, 0 );
            }
            break;

        /*
         *  A new frame has been loaded.  Note that it has already
         *  been added to the main list by RunLoad().
         */

        case PPTMSG_LOADDONE:

            if(!CheckPtr( mymsg->frame, "Loader return frame" ) ) {
                Panic("Really low on resources!");
            }

            ra = FindRexxWaitItem( mymsg );

            if(mymsg->data == (APTR) PERR_OK) {
                currframe = mymsg->frame;
                D(bug("\tNew frame loaded @%08X\n",currframe));
                ReleaseFrame( currframe ); // This will mark it non-busy.
                DoMainList( currframe  );
                GuessDisplay( currframe ); /* Install display etc. */
                UpdateMainWindow( currframe );
                /*
                 *  No reqrender, because the area is considered damaged,
                 *  so Intuition will call the handler.  Instead, we'll
                 *  just allocate the windows.
                 */
                currframe->reqrender = 0;
                MakeDisplayFrame( currframe );

                if(ra) { /* Load succesfull */
                    ra->rc = ra->rc2 = 0;
                    sprintf(buff,"%lu",currframe->ID);
                    ra->result = buff;
                }
            } else {
                D(bug("\tLoad failed, removing...\n"));
                /*
                 *  If load fails, we must signal REXX now, because
                 *  we remove the frame at this point, and thus the
                 *  error pointer will be invalid.
                 */
                if(ra) {
                    ra->rc  = -10;
                    ra->rc2 = (LONG)GetErrorMsg(mymsg->frame,globxd);
                    ReplyRexxWaitItem( ra );
                    ra = NULL;
                }
                LOCKGLOB();
                Remove((struct Node *)mymsg->frame);
                UNLOCKGLOB();
                DeleteInfoWindow(mymsg->frame->mywin,globxd);
                RemFrame(mymsg->frame,globxd);
                currframe = NULL;
            }
            break;

        case PPTMSG_START_INPUT:
            SetupFrameForInput( mymsg );
            currframe = mymsg->frame;
            break;

        case PPTMSG_STOP_INPUT:
            FinishFrameInput( mymsg->frame );
            currframe = mymsg->frame;
            break;

        case PPTMSG_OPENINFOWINDOW:
            D(bug("\tOPENINFOWIN\n"));
            iw = mymsg->frame->mywin;
            OpenInfoWindow( iw, globxd );
            break;

        case PPTMSG_CLOSEINFOWINDOW:
            D(bug("\tCLOSEINFOWIN\n"));
            iw = mymsg->frame->mywin;
            CloseInfoWindow( iw, globxd );
            break;

        case PPTMSG_UPDATEINFOWINDOW:
            D(bug("\tUPDATEINFOWIN\n"));
            iw = mymsg->frame->mywin;
            UpdateInfoWindow( iw, globxd );
            break;

        case PPTMSG_UPDATEPROGRESS:
            D(bug("\tUPDATEPROGRESS(%d)\n",((struct ProgressMsg*)mymsg)->done));
            UpdateProgress( mymsg->frame,
                            mymsg->data,
                            ((struct ProgressMsg *)mymsg)->done,
                            globxd );
            break;

        case PPTMSG_START_PREVIEW:
            D(bug("\tSTART_PREVIEW\n"));
            SetupFrameForPreview( mymsg );
            currframe = mymsg->frame;
            break;

        case PPTMSG_STOP_PREVIEW:
            D(bug("\tSTOP_PREVIEW\n"));
            FinishFramePreview( mymsg );
            currframe = mymsg->frame;
            break;

        case PPTMSG_PICK_POINT:
        case PPTMSG_LASSO_RECT:
        case PPTMSG_FIXED_RECT:
        default:
            if(mymsg->msg.mn_Node.ln_Type == NT_REPLYMSG) {
                D(bug("A wonderful reply received\n"));
            } else {
                D(bug("Unknown message %lu received\n",mymsg->code));
            }
            break;
    }

    /*
     *  If this originated from a REXX message, then we must reply to the script
     *  who called us.
     *  Messages that are internal PPT messages should make sure ra is NULL,
     *  otherwise REXX will get confused.  Currently the *DONE - messages
     *  are the only ones that need to fetch the ra.
     */

    if(ra)
        ReplyRexxWaitItem( ra );

    /*
     *  We can't clear the error before it has been sent back to
     *  REXX (in case it was a REXX item so we do it here.
     *  After this the frame should be completely free and ready to
     *  be used so this is the last thing to do.
     */

    if( currframe ) {
        ClearError( currframe );
    }

    return currframe;
}

///

/*
 *  Handles any signals that came from the main window shared port
 *  BUG:  Some routines do not handle HANDLER_DELETED gracefully.
 */

/// MainWindowMessage()

int MainWindowMessage(VOID)
{
    int quit = 0;
    struct Window *SigWin; /* The window who got the signal with
                              the main window IDCMP port*/
    ULONG rc;
    struct Node *cn, *nn;

    // D(bug("MainWindowMessage()\n"));

    while( (SigWin = (struct Window *)DoMethod( globals->WO_main, WM_GET_SIGNAL_WINDOW )) && (quit == 0) ) {

        // D(bug("\tnew sigwin\n"));

        /*
         *  Main backdrop
         */

        if( SigWin == globals->maindisp->win ) {
            // D(bug("\tmainbackwin\n"));
            while ((rc = HandleEvent(globals->WO_main)) != WMHI_NOMORE) {
                quit += HandleBackdropIDCMP( rc );
            }
            continue;
        }


        /*
         *  Main window
         */

        if( SigWin == framew.win ) {

            // D(bug("\tmainwin\n"));
            while ((rc = HandleEvent(framew.Win)) != WMHI_NOMORE) {
                quit += HandleMainIDCMP( rc );
            }
            continue;
        }

        /*
         *  Tool bar window
         */

        if( SigWin == toolw.win ) {
            while ((rc = HandleEvent(toolw.Win)) != WMHI_NOMORE) {
                quit += HandleToolIDCMP( rc );
            }
            continue;
        }

        /*
         *  Selection window
         */

        if( SigWin == selectw.win ) {
            while ((rc = HandleEvent(selectw.Win)) != WMHI_NOMORE) {
                quit += HandleSelectIDCMP( rc );
            }
            continue;
        }

        /*
         *  Loader Info window
         */

        if( extl.Win && SigWin == extl.win ) {
            // D(bug("\tlinfowin\n"));
            while(( rc = HandleEvent( extl.Win )) != WMHI_NOMORE) {
                quit += HandleExtInfoIDCMP( &extl, rc );
            }
            continue;
        }

        /*
         *  Effects Info window
         */

        if( extf.Win && SigWin == extf.win ) {
            // D(bug("\finfowin\n"));
            while(( rc = HandleEvent( extf.Win )) != WMHI_NOMORE) {
                quit += HandleExtInfoIDCMP( &extf, rc );
            }
            continue;
        }

        /*
         *  Scripts window
         */

        if( exts.Win && SigWin == exts.win ) {
            // D(bug("\sinfowin\n"));
            while(( rc = HandleEvent( exts.Win )) != WMHI_NOMORE) {
                quit += HandleExtInfoIDCMP( &exts, rc );
            }
            continue;
        }

        /*
         *  Preferences window
         */

        if(prefsw.Win && SigWin == prefsw.win ) {
            // D(bug("\tprefswin\n"));
            while ((rc = HandleEvent( prefsw.Win )) != WMHI_NOMORE) {
                int res;
                res = HandlePrefsIDCMP( rc );
                if( res == HANDLER_DELETED ) {
                    quit = HANDLER_RESTART;
                } else {
                    quit += res;
                }
            }
            continue;
        }

        /*
         *  Display windows, Info windows & Display prefs windows
         *  If the frame is deleted, the routines are expected to return
         *  HANDLER_DELETED so that we no longer check any ports which might
         *  no longer exist.
         *  This is also the reason we use the cumbersome 'nn' variable
         *  so that when the frame dies we won't try to read it's successor
         *  from the dead frame's ln_Succ - field.
         */

        SHLOCKGLOB();
        cn = globals->frames.lh_Head;
        UNLOCKGLOB();

        while( nn = cn->ln_Succ ) {
            int res;
            FRAME *fr;

            fr = (FRAME *)cn;

#ifdef DEBUG_MODE
            if( !CheckPtr(fr, "main IDCMP loop, frame") ) break;
#endif

            if( fr->disp ) {
                if( fr->disp->Win && (SigWin == fr->disp->win) ) {
                    // D(bug("\tdispwin\n"));
                    while(( rc = HandleEvent( fr->disp->Win ) ) != WMHI_NOMORE) {
                        res = HandleQDispWindowIDCMP( fr, rc );
                        if(res == HANDLER_DELETED)
                            goto exit_frame_main_loop;
                        else
                            quit+=res;
                    }
                    goto exit_frame_main_loop;
                }
            }

            if( fr->dpw ) {
                if( fr->dpw->WO_Win && (SigWin == fr->dpw->win) ) {
                    // D(bug("\dispprefswin\n"));
                    while(( rc = HandleEvent( fr->dpw->WO_Win) ) != WMHI_NOMORE) {
                        res = HandleDispPrefsWindowIDCMP( fr, rc );
                        if(res == HANDLER_DELETED)
                            goto exit_frame_main_loop;
                        else quit += res;
                    }
                    goto exit_frame_main_loop;
                }
            }

            if( fr->pw ) {
                if( fr->pw->Win && (SigWin == fr->pw->win) ) {
                    while(( rc = HandleEvent( fr->pw->Win) ) != WMHI_NOMORE) {
                        res = HandlePaletteWindowIDCMP( fr, rc );
                        if(res == HANDLER_DELETED)
                            goto exit_frame_main_loop;
                        else quit += res;
                    }
                    goto exit_frame_main_loop;
                }
            }

            if( fr->mywin ) {
                if( SigWin == fr->mywin->win ) {
                    D(bug("\tinfowin\n"));
                    while(( rc = HandleEvent( fr->mywin->WO_win) ) !=WMHI_NOMORE ) {
                        res = HandleInfoIDCMP( fr->mywin, rc );
                        if(res == HANDLER_DELETED)
                            goto exit_frame_main_loop;
                        else quit += res;
                    }
                    goto exit_frame_main_loop;
                }
            }

            if( fr->editwin ) {
                if( SigWin == fr->editwin->win ) {
                    while(( rc = HandleEvent( fr->editwin->Win)) != WMHI_NOMORE ) {
                        res = HandleEditIDCMP( fr->editwin, rc );
                        if( res == HANDLER_DELETED)
                            goto exit_frame_main_loop;
                        else
                            quit += res;
                    }
                    goto exit_frame_main_loop;
                }
            }


            /*
             *  We'll keep the RenderObject last, since we don't know
             *  whether it got the message or not (No SigWin).  This way
             *  it won't go through any other handlers.
             */

            if( fr->renderobject ) {
                struct RenderObject *rdo = fr->renderobject;

                // D(bug("\tRenderObject @ %08X\n",rdo));
                if( rdo->HandleDispIDCMP ) {
                    // D(bug("\trenderwin\n"));
                    res = (int)(*rdo->HandleDispIDCMP)( rdo );
                    if( res >= 0 )
                        quit += res;
                    else
                        goto exit_frame_main_loop;
                }
            }

exit_frame_main_loop:
            cn = nn;
        } /* for(all frames) */

    } /* while(SigWin = ...) */

    return quit;
}

///

/*
    Main program. Big surprise there...
*/

int main(int argc, char **argv)
{
    ULONG mainmask, appmask, helpmask = 0; /* Signal masks. */
    ULONG sig; /* The received signal */
    ULONG globsigmask;
    struct AppMessage *apm;
    struct PPTMessage *mymsg;

    if(argv[1]) SetDebugDir(argv[1]);

    DeleteDebugFiles();

#ifdef DEBUG_MODE
    debug_fh = OpenDebugFile( DFT_Main );
    old_fh = SelectOutput( debug_fh );
#endif

#ifdef _DCC
    onbreak(brk); /* Disable DICE ctrl-c handling. */
#endif

    Fortify_EnterScope();
    if(Initialize() != PERR_OK) {
        FreeResources(globals);
        return 5;
    }

    globsigmask = 0L;

    UpdateStartupWindow( GetStr(mINIT_LOADING_IOMODULES) );
    FetchExternals(globals->userprefs->modulepath,NT_LOADER);
    FetchExternals("Contrib/modules/",NT_LOADER);
    UpdateStartupWindow( GetStr(mINIT_LOADING_EFFECTS) );
    FetchExternals(globals->userprefs->modulepath,NT_EFFECT);
    FetchExternals("Contrib/modules/",NT_EFFECT);
    UpdateStartupWindow( GetStr(mINIT_LOADING_SCRIPTS) );
    FetchExternals(globals->userprefs->rexxpath,NT_SCRIPT);
    FetchExternals("Contrib/Rexx/",NT_SCRIPT);

    CloseStartupWindow();

    if( IsListEmpty( &globals->loaders ) ) {
        Req(NEGNUL,GetStr(mWHOOPS),GetStr(mCOULD_NOT_LOCATE_ANY_LOADERS) );
    }

    if( IsListEmpty( &globals->effects ) ) {
        Req(NEGNUL,GetStr(mWHOOPS),GetStr(mCOULD_NOT_LOCATE_ANY_EFFECTS) );
    }

    /*
     *  Open windows, release the main window and enter the handler loop.
     */

    WindowReady(globals->WO_main);

    if( toolw.prefs.initialopen )
        HandleMenuIDCMP( MID_TOOLWINDOW, NULL, FROM_MAINWINDOW );

    if( framew.prefs.initialopen )
        HandleMenuIDCMP( MID_FRAMEWINDOW, NULL, FROM_MAINWINDOW );

    if( extl.prefs.initialopen )
        HandleMenuIDCMP( MID_LOADERS, NULL, FROM_MAINWINDOW );

    if( extf.prefs.initialopen )
        HandleMenuIDCMP( MID_EFFECTS, NULL, FROM_MAINWINDOW );

    if( exts.prefs.initialopen )
        HandleMenuIDCMP( MID_REXXWINDOW, NULL, FROM_MAINWINDOW );

    if( selectw.prefs.initialopen )
        HandleMenuIDCMP( MID_SELECTWINDOW, NULL, FROM_MAINWINDOW );

    /*
     *  Check for old version
     */

    D(Nag());

    /*
     *  Begin Main Event Loop
     */

restart_window:
    GetAttr( WINDOW_SigMask, globals->WO_main, &mainmask );
    GetAttr( WINDOW_AppMask, globals->WO_main, &appmask );
    helpmask = GetHelpSignal();
    globsigmask = (1 << rxhost->port->mp_SigBit) |
                   mainmask |
                   appmask |
                   helpmask |
                   SIGBREAKF_CTRL_C |
                   (1 << globals->mport->mp_SigBit); /* Make the global sigmask */

    while( quit == 0 ) {

        PROFILE_OFF();
        sig = Wait( globsigmask );
        PROFILE_ON();

        // D(bug("MESSAGE(%08X)\n",sig));

        if( sig & SIGBREAKF_CTRL_C ) { /* On break, exit gracefully. */
            quit += 0xF000;
            D(bug("\n**** GLOBAL BREAK ****\n\n"));
        }

        /*
         *   IDCMP Message for Main Window and all those windows who share
         *   an IDCMP port with it.
         */

        if (sig & mainmask) {
            int res;

            res = MainWindowMessage();

            if( res == HANDLER_RESTART )
                goto restart_window;
            else
                quit += res;

        } /* if (sig & mainmask) */

        /*
         *  Appwindow message
         */

        if( sig & appmask ) {
            D(bug("\tappwin\n"));
            while( apm = GetAppMsg( globals->WO_main ) ) {
                HandleAppMsg( apm );
                ReplyMsg( (struct Message *)apm );
            }
        }

        /*
         *  AREXX Message. This is easy...
         */

        if( sig & (1 << rxhost->port->mp_SigBit) )
            HandleRexxCommands(rxhost);

        /*
         *  A message from our externals.  All messages we've
         *  been sending are PPT Message types, so we can freely
         *  abolish them, if it seems that we got a reply.
         */

        if(sig & (1 << globals->mport->mp_SigBit) ) {
            while(mymsg = (struct PPTMessage *)GetMsg( globals->mport )) {
                FRAME *currframe;
                D(bug( "\nSpecialMessage %08X (code %08X%s)\n",
                        mymsg,mymsg->code,
                        mymsg->msg.mn_Node.ln_Type == NT_REPLYMSG ? " IS_REPLY" : ""));

                currframe = HandleSpecialIDCMP( mymsg );

                if( mymsg->msg.mn_Node.ln_Type == NT_REPLYMSG ) {
                    FreePPTMsg( mymsg, globxd );
                } else {
                    ReplyMsg( (struct Message *)mymsg );
                }

                if(currframe) {
                    DisplayFrames( currframe );
                }

                D(bug("Done with handling %08X\n",mymsg));
            }
        }

        /*
         *  The online help system signaled something.
         */

        if(sig & helpmask) {
            struct AmigaGuideMsg *agm;

            while( agm = GetAmigaGuideMsg(helphandle) ) {
                D(bug("AmigaGuideMsg\n"));
                ReplyAmigaGuideMsg( agm );
            }
        }

        if (quit < 0) quit = 0;
    } /* !quit */

finished:

    FreeResources(globals);

    Fortify_LeaveScope();

    D(bug("Exiting...\n"));

#ifdef DEBUG_MODE
    Flush(Output());
    SelectOutput( old_fh );
    Close(debug_fh);
#endif

    return 0;
}

#ifdef _DCC
/*
    Workbench entry point for DICE.
*/

Local
int wbmain(struct WBStartup *wbs)
{
    int ret;

    ret = main(0,NULL);

    return ret;
}
#endif /* _DCC */
