/*
    PROJECT: PPT
    MODULE : infowin.c

    $Id: infowin.c,v 1.14 1999/10/02 16:33:07 jj Exp $

    This module contains code for handling infowindows.
 */

/*-------------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"
#include "gui.h"

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#ifndef PRAGMAS_BGUI_PRAGMAS_H
#include <pragmas/bgui_pragmas.h>
#endif

#ifndef PRAGMAS_INTUITION_PRAGMAS_H
#include <pragmas/intuition_pragmas.h>
#endif

/*-------------------------------------------------------------------------*/
/* Local defines */

// #define DO_NOT_OPEN_WINDOW    /* Do not open the info window ever. */

/*-------------------------------------------------------------------------*/

Prototype void          UpdateInfoWindow( INFOWIN *iw, struct PPTBase *xd );
Prototype Object *      GimmeInfoWindow( struct PPTBase *, INFOWIN * );
Prototype PERROR        AllocInfoWindow( FRAME *, struct PPTBase * );
Prototype void          DeleteInfoWindow( INFOWIN *, struct PPTBase * );
Prototype PERROR        OpenInfoWindow( INFOWIN *iw, struct PPTBase *xd );
Prototype VOID          CloseInfoWindow( INFOWIN *iw, struct PPTBase *xd );
Prototype VOID          EnableInfoWindow( INFOWIN *, struct PPTBase * );
Prototype VOID          DisableInfoWindow( INFOWIN *, struct PPTBase * );

/*-------------------------------------------------------------------------*/

/*
    Allocate a new info window. The info window pointer in the
    frame is updated.

    BUG: If the infowindow exists, does not check for the existence
         of the window object.
*/

PERROR AllocInfoWindow( FRAME *frame, struct PPTBase *PPTBase )
{
    INFOWIN *iw;
    APTR SysBase = PPTBase->lb_Sys;

    D(bug("AllocInfoWindow()\n"));

    iw = frame->mywin;

    if(!iw) {
        D(bug("Object does not exist, creating...\n"));
        iw = pmalloc( sizeof( INFOWIN ) );
        if(!iw) {
            Req(NEGNUL,NULL,XGetStr(MSG_COULD_NOT_ALLOC_INFOWINDOW));
            return PERR_OUTOFMEMORY;
        }

        /*
         *  Do the initialization
         */

        bzero(iw, sizeof(INFOWIN) );
        InitSemaphore( &(iw->lock) );

        LOCK(iw);

        LOCK(frame);
        iw->myframe = frame;
        frame->mywin = iw;

        if(GimmeInfoWindow( PPTBase, iw ) == NULL) {
            frame->mywin = NULL;
            pfree(iw);
            Req(NEGNUL,NULL,XGetStr(MSG_COULD_NOT_ALLOC_INFOWINDOW));
            return PERR_WINDOWOPEN;
        }

        UNLOCK(frame);
        UNLOCK(iw);

    } else {
        D(bug("Infowindow already existed\n"));
    }

    return PERR_OK;
}

/*
    Open an info window. Will ignore NULL args.
*/

PERROR OpenInfoWindow( INFOWIN *iw, struct PPTBase *PPTBase )
{
    PERROR res = PERR_OK;
    APTR SysBase = PPTBase->lb_Sys;

    D(bug("OpenInfoWindow(%08X)\n",iw));
    if( iw ) {
        Object *Win;
        struct Window *win;

        /*
         *  Are the window object and window pointer OK?
         */

        if( !CheckPtr( iw, "Open(): bad iw" ) ) return PERR_FAILED;

        SHLOCK(iw);
        Win = iw->WO_win;
        win = iw->win;
        UNLOCK(iw);

        if( Win && !win ) {

#ifndef DO_NOT_OPEN_WINDOW

            /*
             *  If this is the main task, do the thing, if not, send
             *  a message to the main task.
             */

            if( FindTask(NULL) == globals->maintask ) {
                LOCK(iw);

                /*
                 *  Make sure all of the attributes are correct and we're
                 *  going to open on the PPT screen (which may have changed
                 *  due to a preferences change).
                 */

                UpdateInfoWindow( iw, PPTBase );
                SetAttrs( Win, WINDOW_Screen, MAINSCR, TAG_DONE );

                D(bug("\tAttempting open...\n"));

                if( (iw->win = WindowOpen( iw->WO_win )) == NULL) {
                    res = PERR_WINDOWOPEN;
                }
                D(bug("\tWindow opened at %08X\n",iw->win));
                UNLOCK(iw);
            } else {
                struct PPTMessage *msg;

                msg = AllocPPTMsg( sizeof( struct PPTMessage ), PPTBase );
                SHLOCK(iw);
                msg->frame = iw->myframe;
                UNLOCK(iw);
                msg->code  = PPTMSG_OPENINFOWINDOW;
                msg->data  = 0L;
                SendPPTMsg( globals->mport, msg, PPTBase );
                WaitForReply( PPTMSG_OPENINFOWINDOW, PPTBase );
            }
#endif
        }
    }
    return res;
}

/*
    Closes an info window. Will ignore NULL args.
*/

VOID CloseInfoWindow( INFOWIN *iw, struct PPTBase *PPTBase )
{
    APTR SysBase = PPTBase->lb_Sys;

    D(bug("CloseInfoWindow(%08X)\n",iw));

    if( iw ) {

        if( !CheckPtr( iw, "Close(): bad iw" ) ) return;

        /*
         *  Do the window AND the window object exist?
         */


        if( (struct Process *)FindTask(NULL) == globals->maintask ) {
            LOCK(iw);
            if( iw->WO_win && iw->win ) {
                WindowClose( iw->WO_win );
                iw->win = NULL;
            }
            UNLOCK(iw);
        } else {
            struct PPTMessage *msg;

            msg = AllocPPTMsg( sizeof( struct PPTMessage ), PPTBase );
            SHLOCK(iw);
            msg->frame = iw->myframe;
            UNLOCK(iw);
            msg->code  = PPTMSG_CLOSEINFOWINDOW;
            msg->data  = 0L;
            SendPPTMsg( globals->mport, msg, PPTBase );
            WaitForReply( PPTMSG_CLOSEINFOWINDOW, PPTBase );
        }
    }
}

/*
    Remove an infowindow. This will also remove all traces of it
    from the parent frame. Safe to call with NULL iw
*/

void DeleteInfoWindow( INFOWIN *iw, struct PPTBase *PPTBase )
{
    FRAME *frame;
    APTR   IntuitionBase = PPTBase->lb_Intuition,
           SysBase = PPTBase->lb_Sys;

    D(bug("DeleteInfoWindow( %08X )\n",iw));
    LOCKGLOB();

    if(iw) {
        LOCK(iw); // No need to unlock, will be destroyed

        frame = iw->myframe;

        if(iw->WO_win)
            DisposeObject( iw->WO_win );

        if(frame) {
            LOCK(frame);
            frame->mywin = NULL;
            UNLOCK(frame);
        }

        pfree(iw);
    }

    UNLOCKGLOB();
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
GimmeInfoWindow( struct PPTBase *PPTBase, INFOWIN *iw )
{
    FRAME *f;
    APTR SysBase = PPTBase->lb_Sys;
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
                WINDOW_Title,       f->nd.ln_Name,
                WINDOW_ScreenTitle, std_ppt_blurb,
                WINDOW_SizeGadget,  TRUE,
                WINDOW_ScaleWidth,  40,
                WINDOW_Font,        globals->userprefs->mainfont,
                WINDOW_Screen,      scr,
                WINDOW_SharedPort,  MainIDCMPPort,
                WINDOW_HelpFile,    "PROGDIR:docs/ppt.guide",
                WINDOW_HelpNode,    "Infowindow",
                WINDOW_LockHeight,  TRUE,
                WINDOW_SmartRefresh,FALSE,
                WINDOW_Activate,    FALSE,
                posntag,            posnval, /* Window position */
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
                                // Label(""), Place(PLACE_LEFT),
                                ButtonFrame, FRM_Flags, FRF_RECESSED,
                                PROGRESS_Min, MINPROGRESS,
                                PROGRESS_Max, MAXPROGRESS,
                                PROGRESS_Done, 0L,
                            EndObject, FixHeight(14),
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
void UpdateInfoWindow( INFOWIN *iw, struct PPTBase *PPTBase )
{
    APTR IntuitionBase = PPTBase->lb_Intuition;
    APTR SysBase = PPTBase->lb_Sys;
    struct TagItem tags[3] = {
        WINDOW_Title, NULL,
        WINDOW_ScreenTitle, NULL,
        TAG_DONE, 0L
    };

    if( iw != NULL ) {
        FRAME *f;

        if( FindTask(NULL) == globals->maintask ) {

            LOCK(iw);

            f = iw->myframe;

            if(f) {
                SHLOCK(f);
                tags[0].ti_Data = (ULONG) f->disp->title;
                tags[1].ti_Data = (ULONG) f->disp->scrtitle;
                SetAttrsA( iw->WO_win, tags );
                UNLOCK(f);
            }
            UNLOCK(iw);
        } else {
            struct PPTMessage *msg;

            msg = AllocPPTMsg( sizeof( struct PPTMessage ), PPTBase );
            msg->frame = iw->myframe;
            msg->code  = PPTMSG_UPDATEINFOWINDOW;
            msg->data  = 0L;
            SendPPTMsg( globals->mport, msg, PPTBase );
        }
    }
}


/*
 *  Disables the info window gadgets from accepting input.
 *  Use with caution!
 */

VOID DisableInfoWindow( INFOWIN *iw, struct PPTBase *PPTBase )
{
    APTR IntuitionBase = PPTBase->lb_Intuition;
    APTR SysBase = PPTBase->lb_Sys;
    struct TagItem tags[3] = {
        GA_Disabled, TRUE,
        TAG_DONE, 0L
    };

    if( iw ) {
        LOCK(iw);
        SetGadgetAttrsA( GAD(iw->GO_Break),iw->win, NULL, tags );
        UNLOCK(iw);
    }
}

/*
 *  Reverses the effect of DisableInfoWindow()
 */

VOID EnableInfoWindow( INFOWIN *iw, struct PPTBase *PPTBase )
{
    APTR IntuitionBase = PPTBase->lb_Intuition;
    APTR SysBase = PPTBase->lb_Sys;
    struct TagItem tags[3] = {
        GA_Disabled, FALSE,
        TAG_DONE, 0L
    };

    if( iw ) {
        LOCK(iw);
        SetGadgetAttrsA( GAD(iw->GO_Break),iw->win, NULL, tags );
        UNLOCK(iw);
    }
}

