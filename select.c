/*
    PROJECT: ppt
    MODULE:  select.c

    This module contains selection routines.  They're placed
    here for easier profiling.  This code is always run
    on the main task.

    $Id: select.c,v 1.3 1999/05/30 18:15:40 jj Exp $

 */

#include "defs.h"
#include "misc.h"

#include <proto/timer.h>

#include <math.h>

#define SQR(x) ((x)*(x))

#define SELBOX_REFRESH_SPEED 4      /* Times per second */

#ifndef GRAPHICS_GFXMACROS_H
#include <graphics/gfxmacros.h>
#endif

/*-------------------------------------------------------------------------*/
/* Internal prototypes */


/// UpdateIWSelbox()
/*
    This routine updates the select window selected area display.
    final == TRUE, if this is a final update.
*/

Prototype void UpdateIWSelbox( FRAME *f, BOOL final );

void UpdateIWSelbox( FRAME *f, BOOL final )
{
    LONG tl, tr, bl, br, cx, cy, cr;
    static struct EClockVal evold = {0};

    if( !selectw.win ) return;

    if( !final ) {
        ULONG etick;
        struct EClockVal ev;

        etick = ReadEClock( &ev );
        if( (ev.ev_lo - evold.ev_lo) < etick/SELBOX_REFRESH_SPEED && (ev.ev_hi == evold.ev_hi) ) {
            return;
        }
        evold = ev;
    }

    if( !f ) {
        tl = tr = bl = br = 0;
    } else {
        if( !IsAreaSelected(f) ) {
            tl = tr = 0;
            bl = f->pix->width-1;
            br = f->pix->height-1;
        } else {
            tl = f->selbox.MinX;
            tr = f->selbox.MinY;
            bl = f->selbox.MaxX-1;
            br = f->selbox.MaxY-1;
        }
    }

    SetGadgetAttrs( GAD(selectw.TopLeft), selectw.win, NULL,
                    STRINGA_LongVal, tl,
                    TAG_DONE );
    SetGadgetAttrs( GAD(selectw.TopRight), selectw.win, NULL,
                    STRINGA_LongVal, tr,
                    TAG_DONE );
    SetGadgetAttrs( GAD(selectw.BottomLeft), selectw.win, NULL,
                    STRINGA_LongVal, bl,
                    TAG_DONE );
    SetGadgetAttrs( GAD(selectw.BottomRight), selectw.win, NULL,
                    STRINGA_LongVal, br,
                    TAG_DONE );

    SetGadgetAttrs( GAD(selectw.Width), selectw.win, NULL,
                    STRINGA_LongVal, abs(bl-tl)+1,
                    TAG_DONE );
    SetGadgetAttrs( GAD(selectw.Height), selectw.win, NULL,
                    STRINGA_LongVal, abs(br-tr)+1,
                    TAG_DONE );

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
                    TAG_DONE );

    SetGadgetAttrs( GAD(selectw.CircleX), selectw.win, NULL,
                    STRINGA_LongVal, cx,
                    TAG_DONE );

    SetGadgetAttrs( GAD(selectw.CircleY), selectw.win, NULL,
                    STRINGA_LongVal, cy,
                    TAG_DONE );
#endif
}
///

/*------------------------------------------------------------------------*/
/* New style input handler interfaces */

VOID ClearSelectMethod( FRAME *frame )
{
    struct IBox clrrect = {0};
    struct Selection *s = &frame->selection;

    s->ButtonDown        = NULL;
    s->ControlButtonDown = NULL;
    s->ButtonUp          = NULL;
    s->MouseMove         = NULL;
    s->DrawSelection     = NULL;
    s->EraseSelection    = NULL;
    s->IsInArea          = NULL;

    s->fixrect           = clrrect;
}

/*
 *  Used selection modes are:
 *    GINP_LASSO_RECT
 *    GINP_LASSO_ELLIPSE
 *    GINP_FIXED_RECT
 */

Prototype VOID ChangeSelectMethod( FRAME *frame, ULONG mode );

VOID ChangeSelectMethod( FRAME *frame, ULONG mode )
{
    PERROR err;

    ClearSelectMethod( frame );

    frame->selection.selectmethod = mode;

    switch( mode ) {
        case GINP_LASSO_RECT:
            err = InitLassoRectSelection( frame );
            break;

        case GINP_FIXED_RECT:
            err = InitFixedRectSelection( frame );
            break;

        case GINP_LASSO_CIRCLE:
            err = InitLassoCircleSelection( frame );
            break;

        case GINP_PICK_POINT:
            err = InitPickPointSelection( frame );
            break;

        default:
            InternalError( "Can't handle that selection mode" );
            break;
    }

    if( err != PERR_OK ) {
        /* BUG: missing code */
    }
}

Prototype VOID DrawSelection( FRAME *frame, ULONG flags );

VOID DrawSelection( FRAME *frame, ULONG flags )
{
    struct Selection *selection = &frame->selection;

    if( !(selection->selstatus & SELF_DRAWN ) && selection->DrawSelection ) {
        selection->DrawSelection( frame, flags );
        selection->selstatus |= SELF_DRAWN;
    }
}

Prototype VOID EraseSelection( FRAME *frame );

VOID EraseSelection( FRAME *frame )
{
    struct Selection *selection = &frame->selection;

    /*
     *  Is there a selection?
     */

    if( (selection->selstatus & SELF_DRAWN) && selection->EraseSelection ) {
        selection->EraseSelection( frame );
        selection->selstatus &= ~SELF_DRAWN;
    }
}

Prototype VOID ButtonDown( FRAME *frame, struct MouseLocationMsg *msg );

VOID ButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;

    if( selection->selstatus & SELF_BUTTONDOWN ) {
        D(bug("Discarded BUTTON_DOWN in panic...\n"));
        return;
    }

    if( selection->ButtonDown ) {
        selection->ButtonDown(frame,msg);
    }
}

Prototype VOID ControlButtonDown( FRAME *frame, struct MouseLocationMsg *msg );

VOID ControlButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;

    if( selection->ControlButtonDown ) {
        selection->ControlButtonDown(frame,msg);
    }
}

Prototype VOID ButtonUp( FRAME *frame, struct MouseLocationMsg *msg );

VOID ButtonUp( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;

    if( selection->ButtonUp ) {
        selection->ButtonUp( frame, msg );
    }
}

Prototype VOID MouseMove( FRAME *frame, struct MouseLocationMsg *msg );

VOID MouseMove( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;

    if( selection->MouseMove ) {
        selection->MouseMove( frame, msg );
    }

    /*
     *  The mouse location is clipped to exlude the values just
     *  outside the border.
     */

    UpdateMouseLocation( msg->xloc >= frame->pix->width ? frame->pix->width-1 : msg->xloc ,
                         msg->yloc >= frame->pix->height ? frame->pix->height-1 : msg->yloc );

}

Prototype BOOL IsInArea( FRAME *frame, struct MouseLocationMsg *msg );

BOOL IsInArea( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;

    if( selection->IsInArea ) {
        return selection->IsInArea( frame, msg );
    }
}




