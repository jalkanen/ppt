/*
    PROJECT: PPT
    MODULE : infowin.c

    $Id: infowin.c,v 1.2 1995/08/20 18:39:59 jj Exp $

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
*/

PERROR AllocInfoWindow( FRAME *frame, EXTBASE *xd )
{
    INFOWIN *iw;

    D(bug("AllocInfoWindow()\n"));

    iw = frame->mywin;

    if(!iw) {
        D(bug("Object does not exist, creating...\n"));
        iw = pmalloc( sizeof( INFOWIN ) );
        if(!iw) {
            Req(NEGNUL,NULL,"Unable to allocate info window");
            return PERR_OUTOFMEMORY;
        }

        bzero(iw, sizeof(INFOWIN) );
        iw->myframe = frame;
        frame->mywin = iw;
        if(GimmeInfoWindow( xd, iw ) == NULL) {
            frame->mywin = NULL;
            pfree(iw);
            Req(NEGNUL,NULL,"Unable to create info window");
            return PERR_WINDOWOPEN;
        }
    }
    return PERR_OK;
}

/*
    Open an info window. Will ignore NULL args.

    BUG: Does not fail.
*/

PERROR OpenInfoWindow( INFOWIN *iw, EXTBASE *xd )
{

    D(bug("OpenInfoWindow(%08X)\n",iw));
    if( iw ) {
        /*
         *  Are the window object and window pointer OK?
         */
        if( iw->WO_win && !iw->win ) {
            if( iw->win = WindowOpen( iw->WO_win )) {
                /* NULL CODE */
            }
        }
    }
    return PERR_OK;
}

/*
    Closes an info window. Will ignore NULL args.
*/

VOID CloseInfoWindow( INFOWIN *iw, EXTBASE *xd )
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

void DeleteInfoWindow( INFOWIN *iw, EXTBASE *xd )
{
    FRAME *frame;
    APTR   IntuitionBase = xd->lb_Intuition;

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
                WINDOW_ScaleWidth, 25,
                WINDOW_Font, globals->userprefs->mainfont,
                WINDOW_Screen, scr,
                WINDOW_SharedPort, MAINWIN->UserPort,
                WINDOW_HelpFile, "PROGDIR:docs/ppt.guide",
                WINDOW_HelpNode, "Infowindow",
                posntag, posnval, /* Window position */
                WINDOW_MasterGroup,
                    MyVGroupObject, HOffset(4), VOffset(4), Spacing(4),
                        StartMember,
                            iw->GO_status = MyInfoObject,
                                LAB_Label,"Status:", LAB_Place, PLACE_LEFT,
                                ButtonFrame, FRM_Flags, FRF_RECESSED,
                                INFO_TextFormat, "�idle�",
                            EndObject,
                        EndMember,
                        StartMember,
                            iw->GO_progress = MyProgressObject,
                                Label("Done:"), Place(PLACE_LEFT),
                                ButtonFrame, FRM_Flags, FRF_RECESSED,
                                PROGRESS_Min, MINPROGRESS,
                                PROGRESS_Max, MAXPROGRESS,
                                PROGRESS_Done, 0L,
                            EndObject,
                        EndMember,
                        StartMember,
                            MyHGroupObject, Spacing(4),
                                StartMember,
                                    iw->GO_Break=MyGenericButton("_Break",GID_IW_BREAK),
                                EndMember,
                            EndObject, /* Vgroup */
                        EndMember,
                    EndObject, /* Master Vgroup */
                EndObject; /* WindowObject */
    } /* does object exist? */


    UNLOCKGLOB();
    return( iw->WO_win );
}
///

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

#if 0
    ULONG args[6];
    FRAME *f;
    APTR IntuitionBase = xd->lb_Intuition;

    if(iw == NULL)
        return;

    if(!CheckPtr( iw, "UpdateInfoWindow: IW" ))
        return;

    f = (FRAME *)iw->myframe;
    args[0] = (ULONG) f->pix->width;
    args[1] = (ULONG) f->pix->height;
    args[2] = (ULONG) f->pix->origdepth;
    if(f->origtype)
        args[3] = (ULONG) f->origtype->info.nd.ln_Name;
    else
        args[3] = (ULONG) "Unknown";
    args[4] = (ULONG) PICSIZE( f->pix);

    args[5] = (ULONG) f->fullname;

    XSetGadgetAttrs(xd, (struct Gadget *)iw->GO_File, iw->win, NULL, INFO_Args, &args[5], TAG_END );
    XSetGadgetAttrs(xd, (struct Gadget *)iw->GO_Info, iw->win, NULL, INFO_Args, &args[0], TAG_END );
#endif
}

void UpdateIWSelbox( FRAME *f )
{
#if 0
    ULONG args[6];
    INFOWIN *iw = f->mywin;

    if(iw) {
        args[0] = (ULONG)f->selbox.MinX;
        args[1] = (ULONG)f->selbox.MinY;
        args[2] = (ULONG)f->selbox.MaxX;
        args[3] = (ULONG)f->selbox.MaxY;
        args[4] = (ULONG)f->selbox.MinX;
        args[5] = (ULONG)f->selbox.MinX;

        SetGadgetAttrs( (struct Gadget *)iw->GO_Selbox, iw->win, NULL, INFO_Args, &args[0], TAG_END );
    }
#endif
}


