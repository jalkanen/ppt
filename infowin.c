/*
    PROJECT: PPT
    MODULE : infowin.c

    $Id: infowin.c,v 1.5 1996/01/08 23:40:30 jj Exp $

    This module contains code for handling infowindows.
 */

#include <defs.h>
#include <misc.h>
#include <gui.h>

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#include <pragma/bgui_pragmas.h>
#include <pragma/exec_pragmas.h>
#include <pragma/intuition_pragmas.h>

/*-------------------------------------------------------------------------*/

Prototype void          UpdateInfoWindow( INFOWIN *iw, EXTDATA *xd );
Prototype Object *      GimmeInfoWindow( EXTDATA *, INFOWIN * );
Prototype void          UpdateIWSelbox( FRAME *f );
Prototype PERROR        AllocInfoWindow( FRAME *, EXTBASE * );
Prototype void          DeleteInfoWindow( INFOWIN *, EXTBASE * );
Prototype PERROR        OpenInfoWindow( INFOWIN *iw, EXTBASE *xd );
Prototype VOID          CloseInfoWindow( INFOWIN *iw, EXTBASE *xd );

/*-------------------------------------------------------------------------*/

/*
    Allocate a new info window. The info window pointer in the
    frame is updated.

    BUG: If the infowindow exists, does not check for the existence
         of the window object.
*/

PERROR AllocInfoWindow( FRAME *frame, EXTBASE *ExtBase )
{
    INFOWIN *iw;

    D(bug("AllocInfoWindow()\n"));

    iw = frame->mywin;

    if(!iw) {
        D(bug("Object does not exist, creating...\n"));
        iw = pmalloc( sizeof( INFOWIN ) );
        if(!iw) {
            Req(NEGNUL,NULL,XGetStr(MSG_COULD_NOT_ALLOC_INFOWINDOW));
            return PERR_OUTOFMEMORY;
        }

        bzero(iw, sizeof(INFOWIN) );
        iw->myframe = frame;
        frame->mywin = iw;
        if(GimmeInfoWindow( ExtBase, iw ) == NULL) {
            frame->mywin = NULL;
            pfree(iw);
            Req(NEGNUL,NULL,XGetStr(MSG_COULD_NOT_ALLOC_INFOWINDOW));
            return PERR_WINDOWOPEN;
        }
    }
    return PERR_OK;
}

/*
    Open an info window. Will ignore NULL args.
*/

PERROR OpenInfoWindow( INFOWIN *iw, EXTBASE *ExtBase )
{
    PERROR res = PERR_OK;

    D(bug("OpenInfoWindow(%08X)\n",iw));
    if( iw ) {
        /*
         *  Are the window object and window pointer OK?
         *  BUG: Should create the infowindow, if it does not exist
         */
        if( iw->WO_win && !iw->win ) {
            if( (iw->win = WindowOpen( iw->WO_win )) == NULL) {
                res = PERR_WINDOWOPEN;
            }
        }
    }
    return res;
}

/*
    Closes an info window. Will ignore NULL args.
*/

VOID CloseInfoWindow( INFOWIN *iw, EXTBASE *ExtBase )
{
    D(bug("CloseInfoWindow(%08X)\n",iw));

    if( iw ) {
        /*
         *  Do the window AND the window object exist?
         */
        if( iw->WO_win && iw->win ) {
            WindowClose( iw->WO_win );
            iw->win = NULL;
        }
    }
}

/*
    Remove an infowindow. This will also remove all traces of it
    from the parent frame. Safe to call with NULL iw
*/

void DeleteInfoWindow( INFOWIN *iw, EXTBASE *ExtBase )
{
    FRAME *frame;
    APTR   IntuitionBase = ExtBase->lb_Intuition;

    D(bug("DeleteInfoWindow( %08X )\n",iw));
    if(iw) {
        frame = iw->myframe;

        if(iw->WO_win)
            DisposeObject( iw->WO_win );

        if(frame)
            frame->mywin = NULL;

        pfree(iw);
    }
}


/*
    Opens one info window. Please note
    that iw should be filled already, otherwise trouble might arise.
    Returns NULL in case of failure, otherwise pointer to the newly
    created object.

    Please note that this routine does NOT open the info window
*/

/// GimmeInfoWindow()

Object *
GimmeInfoWindow( EXTDATA *xd, INFOWIN *iw )
{
    EXTBASE *ExtBase = xd; /* BUG */
    FRAME *f;
    APTR BGUIBase = xd->lb_BGUI, SysBase = xd->lb_Sys, IntuitionBase = xd->lb_Intuition;
    struct Screen *scr;
    struct Window *win;
    ULONG posntag, posnval; /* A kludge */
    struct IBox posrelbox;

    D(bug("GimmeInfoWindow( %08X )\n",iw));

    if(!CheckPtr(iw,"GIW()")) return NULL;

    f = (FRAME *)iw->myframe;
    if(!CheckPtr(f,"GIW()")) return NULL;

    win = GetFrameWin( f );

    /*
     *  If a window exists, open the new window over it
     *  If it does not, open at the center of the new screen.
     */

    if( win == MAINWIN ) {
        posntag = WINDOW_Position;
        posnval = POS_CENTERSCREEN;
        scr     = MAINSCR;
    } else {
        posntag = WINDOW_PosRelBox;
        posrelbox.Left   = win->LeftEdge;
        posrelbox.Width  = win->Width;
        posrelbox.Top    = win->TopEdge;
        posrelbox.Height = win->Height;
        posnval = (ULONG) &posrelbox;
        scr = f->disp->scr; /* BUG: Should check for disp */
    }


    SHLOCKGLOB(); /* We don't want anyone else to muck about here */
    if( iw->WO_win == NULL ) { /* Do we exist yet? */
        iw->WO_win = MyWindowObject,
                WINDOW_Title, f->nd.ln_Name,
                WINDOW_ScreenTitle, std_ppt_blurb,
                WINDOW_SizeGadget, TRUE,
                WINDOW_MenuStrip, PPTMenus,
                WINDOW_ScaleWidth, 40,
                WINDOW_Font, globals->userprefs->mainfont,
                WINDOW_Screen, scr,
                WINDOW_SharedPort, MainIDCMPPort,
                WINDOW_HelpFile, "PROGDIR:docs/ppt.guide",
                WINDOW_HelpNode, "Infowindow",
                posntag, posnval, /* Window position */
                WINDOW_MasterGroup,
                    MyVGroupObject, HOffset(4), VOffset(4), Spacing(4),
                        StartMember,
                            iw->GO_status = MyInfoObject,
                                // LAB_Label,"Status:", LAB_Place, PLACE_LEFT,
                                ButtonFrame, FRM_Flags, FRF_RECESSED,
                                INFO_TextFormat, XGetStr(MSG_IDLE_GAD),
                            EndObject,
                        EndMember,
                        StartMember,
                            iw->GO_progress = MyProgressObject,
                                Label(""), Place(PLACE_LEFT),
                                ButtonFrame, FRM_Flags, FRF_RECESSED,
                                PROGRESS_Min, MINPROGRESS,
                                PROGRESS_Max, MAXPROGRESS,
                                PROGRESS_Done, 0L,
                            EndObject,
                        EndMember,
                        StartMember,
                            MyHGroupObject, Spacing(4),
                                VarSpace(50),
                                StartMember,
                                    iw->GO_Break=MyGenericButton(XGetStr(MSG_BREAK_GAD),GID_IW_BREAK),
                                EndMember,
                                VarSpace(50),
                            EndObject, Weight(1),
                        EndMember,
                    EndObject, /* Master Vgroup */
                EndObject; /* WindowObject */
    } /* does object exist? */


    UNLOCKGLOB();
    return( iw->WO_win );
}
///

/*
    Updates the infowindow attributes. Currently does not do anything more
    than update the window/screen titles.
*/
void UpdateInfoWindow( INFOWIN *iw, EXTDATA *xd )
{
    APTR IntuitionBase = xd->lb_Intuition;
    struct TagItem tags[3] = {
        WINDOW_Title, NULL,
        WINDOW_ScreenTitle, NULL,
        TAG_DONE, 0L
    };
    FRAME *f = iw->myframe;

    if(f) {
        tags[0].ti_Data = f->disp->title;
        tags[1].ti_Data = f->disp->scrtitle;
        SetAttrsA( iw->WO_win, tags );
    }
}

/*
    This routine updates the toolbox area display.
    It is not used at the moment.
*/

void UpdateIWSelbox( FRAME *f )
{

}


