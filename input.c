/*
    PROJECT: ppt
    MODULE:  input.c

    This file contains the input-handlers of PPT.

    $Id: input.c,v 1.1 1998/09/20 21:35:36 jj Exp $

 */

/*------------------------------------------------------------------------*/

#include "defs.h"
#include "misc.h"

#include <math.h>

#define SQR(x) ((x)*(x))

/*------------------------------------------------------------------------*/

Prototype ASM PERROR        StartInput( REG(a0) FRAME *, REG(a1) APTR, REG(d0) ULONG, REG(a6) EXTBASE * );
Prototype ASM VOID          StopInput( REG(a0) FRAME *, REG(a6) EXTBASE * );

/*------------------------------------------------------------------------*/

/// StartInput()

/****i* pptsupport/StartInput ******************************************
*
*   NAME
*       StartInput - Start up notification
*
*   SYNOPSIS
*       error = StartInput( frame, method, initialmsg )
*       D0                  A0     D0      A1
*
*       PERROR StartInput( FRAME *, ULONG, struct PPTMessage * );
*
*   FUNCTION
*       This function starts up input handling for a frame. Its main
*       purpose is to provide interactivity for the external effect
*       modules.
*
*       When you call this function, PPT will temporarily unlock the
*       frame and allow resizing the image window, zooming, etc.
*
*       There are several methods at your disposal:
*
*       GINP_PICK_POINT - initialmsg is ignored and may be NULL.
*           Every time the user clicks the mouse within the image,
*           you'll get a gPointMessage with the x and y fields
*           containing image co-ordinates.
*
*       GINP_FIXED_RECT - initialmsg should be a pointer to
*           a struct gFixRextMessage, whose field dim should
*           be initialized to a sensible value.  When the user moves
*           the cursor in the frame, he'll see a rectangle moving around
*           and when he places it somewhere, you'll be notified.
*
*       You will be notified of input events until the StopInput()
*       function is called.
*
*   INPUTS
*       frame - as usual
*       method - Any of the following:
*           GINP_PICK_POINT
*           GINP_FIXED_RECT
*       area - Depends on the method. See above.
*
*   RESULT
*       error - Standard PPT error code. PERR_OK if the input
*           handler was started up OK.
*
*   EXAMPLE
*
*   NOTES
*       You MUST call StopInput() when you are ready to start up your own
*       processing on the frame.  Otherwise you'll keep getting messages
*       even when you don't want them and the user might get confused.
*
*   BUGS
*
*   SEE ALSO
*       StopInput()
*
*****************************************************************************
*
*    BUG: No error checking
*/

SAVEDS ASM
PERROR StartInput( REG(a0) FRAME *frame,
                   REG(a1) struct PPTMessage *initialmsg,
                   REG(d0) ULONG mid,
                   REG(a6) EXTBASE *ExtBase )
{
    struct PPTMessage *pmsg, *amsg;

    D(bug("StartInput(f=%08X,i=%08X,m=%ld)\n",frame,initialmsg,mid));

    if(!CheckPtr(frame, "StartInput(): invalid frame"))
        return FALSE; /* Sanity check */

    /*
     *  The type of the message will tell us what size of the message we'll need.
     *  We'll now allocate and initialize all type-relevant fields.
     */

    switch(mid) {
        case GINP_FIXED_RECT:
            pmsg = AllocPPTMsg( sizeof(struct gFixRectMessage), ExtBase );
            ((struct gFixRectMessage *)pmsg)->dim = ((struct gFixRectMessage *)initialmsg)->dim;
            D(bug("\tSending out a gFixRect()\n"));
            break;

        default:
            pmsg = AllocPPTMsg( sizeof(struct PPTMessage), ExtBase );
            D(bug("\tIgnored input type %X\n",mid));
            break;
    }

    /*
     *  We could set this one also in the switch() - clause above, but
     *  since it is quite easy to derive, we'll do it here.
     */

    pmsg->code  = PPTMSG_START_INPUT;
    // pmsg->code  = mid + PPTMSG_LASSO_RECT;
    pmsg->data  = (APTR)mid;

    /*
     *  Send the address of the parent frame, if such a beast
     *  exists, since the parent is what we know this frame by.
     */

    if( frame->parent ) {
        D(bug("\tFrame has a parent @ %08X\n",frame->parent));
        pmsg->frame = frame->parent;
    } else {
        pmsg->frame = frame;
    }

    /*
     *  Send the message to main program and return, if it
     *  was succesfull.
     */

    SendPPTMsg( globxd->mport, pmsg, ExtBase );

    /*
     *  Wait for authorization reply from the main task
     */

    D(bug("\tWaiting for reply...\n"));

    for(;;) {
        ULONG sig, sigmask;

        sigmask = 1 << ExtBase->mport->mp_SigBit;

        sig = Wait( sigmask|SIGBREAKF_CTRL_C );

        if( sig & SIGBREAKF_CTRL_C ) {
            SetErrorCode( frame, PERR_BREAK );
            return PERR_BREAK;
        }

        if( sig & sigmask ) {

            /*
             *  We'll listen only to the first message. All others must
             *  be actual user data.  We'll return right after we've
             *  gotten the message.
             *
             *  In case any messages manage to get this far, we'll
             *  quietly reply to them.
             */

            if( amsg = (struct PPTMessage *)GetMsg(ExtBase->mport) ) {

                if( amsg->msg.mn_Node.ln_Type == NT_REPLYMSG ) {
                    if( amsg->code == PPTMSG_START_INPUT ) {
                        PERROR res;

                        if( (res = (PERROR)amsg->data) == PERR_OK ) {
                            D(bug("\tMain task says its OK!\n"));
                        } else {
                            D(bug("\tFailed to set up the comm system\n"));
                        }

                        FreePPTMsg( amsg, ExtBase );
                        return res;
                    }
                } else {
                    ReplyMsg( (struct Message *) amsg );
                }

            }
        }
    }

    return PERR_OK;
}
///

/// StopInput()

/****i* pptsupport/StopInput ******************************************
*
*   NAME
*       StopInput - Stop sending input messages.
*
*   SYNOPSIS
*       StopInput( frame )
*                  A0
*
*       VOID StopInput( FRAME * );
*
*   FUNCTION
*       This function stops the input handler started by StartInput()
*       and locks the frame, making it safe to process.
*
*       It is important to call this function before starting up your
*       own processing. If you call StartInput() and call GetPixelRow()
*       some conflicts will occur, which will cause undefined behaviour.
*       It might work. Or it might crash the system.
*
*   INPUTS
*       frame - the usual thing. Must be the same as in StartInput().
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       Should provide protection against forgotten StopInputs().
*
*   SEE ALSO
*       StartInput()
*
*****************************************************************************
*
*/

/*
    Used to signal the main task that the external no longer
    requires signals.
*/

SAVEDS ASM
VOID StopInput( REG(a0) FRAME *frame, REG(a6) EXTBASE *ExtBase )
{
    struct PPTMessage *pmsg;

    D(bug("StopInput(frame=%08X)\n",frame));

    if(!CheckPtr(frame,"StopInput(): Invalid frame")) return;

    pmsg = AllocPPTMsg( sizeof(struct PPTMessage), ExtBase );

    pmsg->code  = PPTMSG_STOP_INPUT;

    if( frame->parent )
        pmsg->frame = frame->parent;
    else
        pmsg->frame = frame;

    D(bug("Sending to main task\n"));
    SendPPTMsg( globxd->mport, pmsg, ExtBase );

    D(bug("Waiting for reply\n"));
    WaitForReply( PPTMSG_STOP_INPUT, ExtBase );
}
///

/*------------------------------------------------------------------------*/

/// Input exit and init codes from the main task side
/*
    The frame is set up.
    pmsg points to an unanswered PPTMessage, that originally came from
    the external.
    BUG: No error checking
*/

Prototype VOID SetupFrameForInput( struct PPTMessage * );

VOID SetupFrameForInput( struct PPTMessage *pmsg )
{
    FRAME *f = pmsg->frame;

    D(bug("SetupFrameForInput()\n"));

    if( !CheckPtr(f, "SFFI()"))
        return;

    LOCK(f);

    RemoveSelectBox( f );

    f->selectmethod = (ULONG)pmsg->data;
    f->selectport   = pmsg->msg.mn_ReplyPort;

    if( f->selectmethod == GINP_FIXED_RECT ) {
        f->fixrect = ((struct gFixRectMessage *)pmsg)->dim;
        DrawSelectBox( f, DSBF_FIXEDRECT);
    }

    ChangeBusyStatus( f, BUSY_READONLY );

    UNLOCK(f);

    /*
     *  Modify the initial message that main() will ReplyMsg() to.
     */

    pmsg->data  = PERR_OK; /* Signal: successful. */
}

/*
    Clear away any signs of us ever having had a other method than
    LASSO_RECT.
*/
Prototype VOID ClearFrameInput( FRAME * );

VOID ClearFrameInput( FRAME *f )
{
    struct Library *SysBase = SYSBASE();
    struct IBox clrrect = {0};

    D(bug("ClearFrameInput(%08X)\n",f));

    RemoveSelectBox( f );
    LOCK(f);
    f->selectmethod = GINP_LASSO_RECT;
    f->selectport   = NULL;
    f->fixrect      = clrrect;
    UNLOCK(f);
}

/*
    Sends a simple message to the external, telling we have finished.

    BUG: No error checking.
    BUG: The busy code should be saved.
    BUG: Obsolete.
*/

Prototype VOID FinishFrameInput( FRAME * );

VOID FinishFrameInput( FRAME *f )
{
    D(bug("\tFinished frame input\n"));

    ChangeBusyStatus( f, BUSY_CHANGING );

    ClearFrameInput( f );
}
///

/*------------------------------------------------------------------------*/
/* The input handlers */

/*
    Local handlers for HandleQDispWinIDCMP();

    mousex,mousey = mouse location at the moment of the event
    xloc, yloc    = the image coordinates under the mouse cursor
*/

/// ButtonDown()
Prototype VOID DW_ButtonDown( FRAME *frame, WORD mousex, WORD mousey, WORD xloc, WORD yloc );

VOID DW_ButtonDown( FRAME *frame, WORD mousex, WORD mousey, WORD xloc, WORD yloc )
{
    struct Rectangle *sb = &frame->selbox;
    struct gPointMessage *gp;
    UWORD boxsize = 5;
    struct Rectangle *cb = &frame->disp->handles;

    /*
     *  Discard this, if the button seems to be already down
     */

    if( frame->selstatus & SELF_BUTTONDOWN ) {
        D(bug("Discarded BUTTON_DOWN in panic...\n"));
        return;
    }

    switch(frame->selectmethod) {

        case GINP_LASSO_RECT:

            if( IsFrameBusy(frame) ) break;

            /*
             *  If the window has an old display rectangle visible, remove it.
             */

            if(sb->MinX != ~0 && (frame->selstatus & SELF_RECTANGLE) ) { /* Delete old. */
                BOOL corner = FALSE;

                RemoveSelectBox( frame );

                /*
                 *  Now, if the mouse was on the image handles, we will
                 *  start resizing the select box.  The box is always
                 *  in the right orientation here.
                 */

                if( mousex >= cb->MinX && mousex < cb->MinX+boxsize && mousey >= cb->MinY && mousey < cb->MinY+boxsize ) {
                    // Top left corner
                    sb->MinX = sb->MaxX; sb->MinY = sb->MaxY;
                    corner = TRUE;
                } else if(mousex >= cb->MinX && mousex < cb->MinX+boxsize && mousey >= cb->MaxY-boxsize && mousey < cb->MaxY ) {
                    // Top right corner
                    sb->MinX = sb->MaxX; sb->MinY = sb->MinY;
                    corner = TRUE;
                } else if(mousex >= cb->MaxX-boxsize && mousex < cb->MaxX && mousey >= cb->MinY && mousey < cb->MinY+boxsize ) {
                    // Bottom left corner
                    sb->MinX = sb->MinX; sb->MinY = sb->MaxY;
                    corner = TRUE;
                } else if(mousex >= cb->MaxX-boxsize && mousex < cb->MaxX && mousey >= cb->MaxY-boxsize && mousey < cb->MaxY ) {
                    // Bottom right corner
                    corner = TRUE;
                }

                if( corner ) {
                    sb->MaxX = xloc; sb->MaxY = yloc;
                    DrawSelectBox( frame, DSBF_INTERIM );
                    UpdateIWSelbox(frame);
                    frame->selstatus |= SELF_BUTTONDOWN;
                    D(bug("Picked image handle (%d,%d)\n",xloc,yloc));
                    break;
                }
            }

            /*
             *  Now, if there was no previous selectbox drawn, we will start
             *  a new area to be drawn.
             */

            sb->MinX = xloc;
            sb->MinY = yloc;
            sb->MaxX = sb->MinX;
            sb->MaxY = sb->MinY;
            D(bug("Marked select begin (%d,%d)\n",xloc,yloc));

            frame->selstatus |= SELF_BUTTONDOWN;
            break;

        case GINP_PICK_POINT:

            /*
             *  We have a point. Tell the external about the user selection
             */

            gp = (struct gPointMessage *) AllocPPTMsg(sizeof(struct gPointMessage), globxd);
            if( gp ) {
                D(bug("PICKPOINT msg sent: (%d,%d)\n",xloc,yloc));
                gp->msg.code = PPTMSG_PICK_POINT;
                gp->x = xloc;
                gp->y = yloc;
                SendInputMsg( frame, gp );
            } else {
                Panic("Can't alloc gPointMsg\n");
            }
            break;

        case GINP_FIXED_RECT:
            if( mousex >= cb->MinX && mousex <= cb->MaxX && mousey >= cb->MinY && mousey <= cb->MaxY ) {

                frame->fixoffsetx = mousex - cb->MinX;
                frame->fixoffsety = mousey - cb->MinY;

                frame->selstatus |= SELF_BUTTONDOWN;
            }
            break;

        case GINP_LASSO_CIRCLE:
            frame->circlex = xloc;
            frame->circley = yloc;
            frame->circleradius = 1;
            frame->selstatus |= SELF_BUTTONDOWN;
            D(bug("Marked select begin (%d,%d)\n",xloc,yloc));
            break;

        default:
            D(bug("Unknown method %ld\n"));
            break;
    }
}
///

/// ControlButtonDown()
Prototype VOID DW_ControlButtonDown( FRAME *frame, WORD mousex, WORD mousey, WORD xloc, WORD yloc );

VOID DW_ControlButtonDown( FRAME *frame, WORD mousex, WORD mousey, WORD xloc, WORD yloc )
{
    // struct Rectangle *sb = &frame->selbox;
    struct Rectangle *cb = &frame->disp->handles;

    switch( frame->selectmethod ) {
        case GINP_LASSO_RECT:
            if (IsFrameBusy(frame)) break;

            if( mousex >= cb->MinX && mousey <= cb->MaxX && mousey >= cb->MinY && mousey <= cb->MaxY ) {
                /*
                 *  Yes, it is within the area
                 */

                RemoveSelectBox( frame );
                frame->fixoffsetx   = mousex - cb->MinX;
                frame->fixoffsety   = mousey - cb->MinY;
                frame->selstatus    |= (SELF_BUTTONDOWN|SELF_CONTROLDOWN);
            } else {
                DW_ButtonDown( frame, mousex, mousey, xloc, yloc );
            }
            break;

        case GINP_LASSO_CIRCLE:
            /* BUG: Requires code, too */
            break;

        default:
            DW_ButtonDown( frame, mousex, mousey, xloc, yloc );
    }
}
///

/// ButtonUp()

Prototype VOID DW_ButtonUp( FRAME *frame, WORD xloc, WORD yloc );

VOID DW_ButtonUp( FRAME *frame, WORD xloc, WORD yloc )
{
    struct Rectangle *sb = &frame->selbox;
    struct gFixRectMessage *gfr;

    switch( frame->selectmethod ) {

        case GINP_LASSO_RECT:

            if( IsFrameBusy(frame) ) break;

            if( frame->selstatus & SELF_CONTROLDOWN ) {
                WORD xx, yy, w, h;

                RemoveSelectBox( frame );

                /*
                 *  Calculate the rectangle location - redrawing it.
                 */

                w = sb->MaxX - sb->MinX;
                h = sb->MaxY - sb->MinY;

                CalcMouseCoords( frame, frame->fixoffsetx, frame->fixoffsety, &xx, &yy );

                if( xloc-xx < 0 ) {
                    sb->MinX = 0;
                } else if( xloc-xx+w > frame->pix->width ) {
                    sb->MinX = frame->pix->width-w;
                } else {
                    sb->MinX = xloc-xx;
                }

                if( yloc-yy < 0 ) {
                    sb->MinY = 0;
                } else if( yloc-yy+h > frame->pix->height ) {
                    sb->MinY = frame->pix->height-h;
                } else {
                    sb->MinY = yloc-yy;
                }

                sb->MaxX = sb->MinX + w;
                sb->MaxY = sb->MinY + h;

                UpdateIWSelbox( frame );
                DrawSelectBox( frame, 0L );
                frame->selstatus &= ~(SELF_BUTTONDOWN|SELF_CONTROLDOWN);
            } else {

                if(xloc == sb->MinX || yloc == sb->MinY) {
                    /*
                     *  Image is too small, so it will be removed.
                     */
                    UnselectImage( frame );
                    UpdateIWSelbox( frame );
                    frame->selstatus &= ~(SELF_BUTTONDOWN|SELF_RECTANGLE);
                } else {
                    /*
                     *  Remove the old display rectangle, if needed.
                     */

                    RemoveSelectBox( frame );

                    /*
                     *  Reorient the rectangle and put up the new one.
                     */

                    sb->MaxX = xloc;
                    sb->MaxY = yloc;
                    ReorientSelbox( sb );
                    UpdateIWSelbox(frame);
                    DrawSelectBox( frame, 0L );
                    frame->selstatus &= ~(SELF_BUTTONDOWN);
                    D(bug("Marked select end (%d,%d)\n",xloc,yloc));
                }
            }
            break;

        case GINP_FIXED_RECT:
            gfr = (struct gFixRectMessage *) AllocPPTMsg(sizeof(struct gFixRectMessage), globxd);

            if( gfr ) {
                WORD xx, yy;

                CalcMouseCoords( frame, frame->fixoffsetx, frame->fixoffsety, &xx, &yy );
                frame->selstatus &= ~SELF_BUTTONDOWN;

                D(bug("FIXEDRECT msg sent: (%d,%d)\n",xloc-xx,yloc-yy));
                gfr->msg.code = PPTMSG_FIXED_RECT;
                gfr->x = xloc - xx;
                gfr->y = yloc - yy;
                SendInputMsg( frame, gfr );
            } else {
                Panic("Can't alloc gFixRectMsg\n");
            }
            break;

        case GINP_LASSO_CIRCLE:
            if( xloc == frame->circlex && yloc == frame->circley ) {
                /* Too small an image */
                UnselectImage( frame );
                UpdateIWSelbox( frame );
                frame->selstatus &= ~(SELF_BUTTONDOWN|SELF_RECTANGLE);
            } else {
                sb->MinX = frame->circlex - frame->circleradius;
                sb->MinY = frame->circley - frame->circleradius;
                sb->MaxX = frame->circlex + frame->circleradius;
                sb->MaxY = frame->circley + frame->circleradius;
                UpdateIWSelbox(frame);
                frame->selstatus &= ~SELF_BUTTONDOWN;
                D(bug("Marked circle at (%d,%d) with radius %d\n",
                       frame->circlex, frame->circley, frame->circleradius ));
            }
            break;

        default:
            break;
    }
}
///

/// MouseMove()
Prototype VOID DW_MouseMove( FRAME *frame, WORD xloc, WORD yloc );

VOID DW_MouseMove( FRAME *frame, WORD xloc, WORD yloc )
{
    struct Rectangle *sb = &frame->selbox;
    struct IBox *gfr = &frame->fixrect;
    WORD xx, yy;

    switch( frame->selectmethod ) {
        case GINP_LASSO_RECT:
            if( sb->MinX != ~0 && (frame->selstatus & SELF_BUTTONDOWN) ) {
                /* Overdraw previous */
                RemoveSelectBox( frame );

                if( frame->selstatus & SELF_CONTROLDOWN ) {
                    WORD xx, yy, w, h;

                    w = sb->MaxX - sb->MinX;
                    h = sb->MaxY - sb->MinY;

                    CalcMouseCoords( frame, frame->fixoffsetx, frame->fixoffsety, &xx, &yy );

                    if( xloc-xx < 0 ) {
                        sb->MinX = 0;
                    } else if( xloc-xx+w > frame->pix->width ) {
                        sb->MinX = frame->pix->width-w;
                    } else {
                        sb->MinX = xloc-xx;
                    }

                    if( yloc-yy < 0 ) {
                        sb->MinY = 0;
                    } else if( yloc-yy+h > frame->pix->height ) {
                        sb->MinY = frame->pix->height-h;
                    } else {
                        sb->MinY = yloc-yy;
                    }

                    sb->MaxX = sb->MinX + w;
                    sb->MaxY = sb->MinY + h;

                } else {
                    /* Redraw new */
                    sb->MaxX = xloc; sb->MaxY = yloc;
                }
                DrawSelectBox( frame, DSBF_INTERIM );
                UpdateIWSelbox(frame);
            }

            break;

        case GINP_PICK_POINT:
            break;

        case GINP_FIXED_RECT:

            if( frame->selstatus & SELF_BUTTONDOWN ) {
                /*
                 *  Remove previous, if needed
                 *  BUG: May conflict with the lasso_rect stuff.
                 */

                RemoveSelectBox( frame );

                CalcMouseCoords( frame, frame->fixoffsetx, frame->fixoffsety, &xx, &yy );

                gfr->Left = xloc-xx; gfr->Top = yloc-yy;

                DrawSelectBox( frame, DSBF_FIXEDRECT );
            }

            break;

        case GINP_LASSO_CIRCLE:
            if( frame->selstatus & SELF_BUTTONDOWN ) {
                RemoveSelectBox( frame );
                frame->circleradius = sqrt( (double)SQR(frame->circlex-xloc) + (double)SQR(frame->circley-yloc) );
                DrawSelectBox( frame, 0L );
            }
            break;

        default:
            break;
    }

    /*
     *  The mouse location is clipped to exlude the values just
     *  outside the border.
     */

    UpdateMouseLocation( xloc >= frame->pix->width ? frame->pix->width-1 : xloc ,
                         yloc >= frame->pix->height ? frame->pix->height-1 : yloc );

}
///
