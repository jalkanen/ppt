/*
    PROJECT: ppt
    MODULE:  rectangleselect.c

    This module contains the rectangular selection routines.  They're placed
    here for easier profiling.

    $Id: rectangleselect.c,v 1.2 1999/08/01 16:48:54 jj Exp $

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

/// DrawSelectRectangle
/*
   Draws a select box in the given frame displaywindow. Corners
   are at (x0,y0) and (x1,y1), relative to inner window top left
   corner.
   BUG: Won't work if pixsize > 16K
   BUG: Uses __divu

   flags can contain any of the following flags:

   DSBF_INTERIM : This is an interim draw, i.e. the image is still being
   drawn.  In this case, the resizing handles are not drawn.

   DSBF_FIXEDRECT : Draws a fixed-size rectangle. This means that
   the image contains also connecting beams between corners.
 */

// Local
VOID DrawSelectRectangle(FRAME * frame, WORD x0, WORD y0, WORD x1, WORD y1, ULONG flags)
{
    DISPLAY *d = frame->disp;
    struct Window *win = d->win;
    WORD winwidth, winheight, xoffset, yoffset;
    BOOL lv = FALSE, rv = FALSE, th = FALSE, bh = FALSE;

    if (x0 == ~0 || !d)
        return;

    if (x0 == x1 || y0 == y1)
        return;

    if (!win ) return;

    /*
     *  Get the window size. If this is the bgui window on the main screen,
     *  then read it from the areabox, else assume we have a gzzwindow.
     */

    if (frame->disp->RenderArea) {
        struct IBox *abox;
        GetAttr(AREA_AreaBox, frame->disp->RenderArea, (ULONG *) & abox);
        winwidth = abox->Width;
        winheight = abox->Height;
        xoffset = abox->Left;
        yoffset = abox->Top;
    } else {
        winwidth = win->GZZWidth;
        winheight = win->GZZHeight;
        xoffset = 0;
        yoffset = 0;
    }

    /*
     *  Scale and limit the values to be inside the window
     */

    x0 -= frame->zoombox.Left;
    x1 -= frame->zoombox.Left;
    y0 -= frame->zoombox.Top;
    y1 -= frame->zoombox.Top;


    /*
     *  Do clipping
     */

    if (x1 < 0 || x0 > frame->zoombox.Width)
        return;                 // not visible

    if (y1 < 0 || y0 > frame->zoombox.Height)
        return;                 // not visible

    if (x0 >= 0)
        lv = TRUE;
    else
        x0 = 0;
    if (x1 <= frame->zoombox.Width)
        rv = TRUE;
    else
        x1 = frame->zoombox.Width;
    if (y0 >= 0)
        th = TRUE;
    else
        y0 = 0;
    if (y1 <= frame->zoombox.Height)
        bh = TRUE;
    else
        y1 = frame->zoombox.Height;


#if 1
    /*
     *  Dice generates better code with this.
     */
    x0 = (LONG) (MULS16(x0, winwidth)) / (WORD) (frame->zoombox.Width);
    x1 = (LONG) (MULS16(x1, winwidth)) / (WORD) (frame->zoombox.Width);

    y0 = (LONG) (MULS16(y0, winheight)) / (WORD) frame->zoombox.Height;
    y1 = (LONG) (MULS16(y1, winheight)) / (WORD) frame->zoombox.Height;
#else
    {
        x0 = ((LONG) x0 * winwidth) / frame->zoombox.Width;
        x1 = ((LONG) x1 * winwidth) / frame->zoombox.Width;
        y0 = ((LONG) y0 * winheight) / frame->zoombox.Height;
        y1 = ((LONG) y1 * winheight) / frame->zoombox.Height;
    }
#endif

    /*
     *  Draw!
     */

    {
        WORD boxsize = 5;       // BUG: Should calculate according to window size.

        x0 += xoffset;
        x1 += xoffset;
        y0 += yoffset;
        y1 += yoffset;

        SetDrMd(win->RPort, COMPLEMENT);

        if (!(flags & DSBF_FIXEDRECT))
            SetDrPt(win->RPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

        // WaitTOF();

        // Left vertical beam

        if (lv) {
            Move(win->RPort, x0, y0);
            Draw(win->RPort, x0, y1);
        }
        // Right vertical beam

        if (rv) {
            Move(win->RPort, x1, y0);
            Draw(win->RPort, x1, y1);

        }
        // Top horizontal beam

        if (th) {
            Move(win->RPort, x0, y0);
            Draw(win->RPort, x1, y0);
        }
        // Bottom horizontal beam

        if (bh) {
            Move(win->RPort, x0, y1);
            Draw(win->RPort, x1, y1);
        }
        SetDrPt(win->RPort, 0xFFFF);

        if (flags & DSBF_FIXEDRECT) {

            /*
             *  Fixed rectangle: draw all areas
             */

            if( lv && rv && th && bh ) {
                Move( win->RPort, x0, y0 );
                Draw( win->RPort, x1, y1 );
                Move( win->RPort, x0, y1 );
                Draw( win->RPort, x1, y0 );
            }

        } else {
            /*
             *  A lasso rectangle : draw the resizing handles as well
             */

            if (lv && !(flags & DSBF_INTERIM)) {
                if (th) {
                    RectFill(win->RPort, x0, y0, x0 + boxsize, y0 + boxsize);
                }
                if (bh) {
                    RectFill(win->RPort, x0, y1 - boxsize, x0 + boxsize, y1);
                }
            }
            if (rv && !(flags & DSBF_INTERIM)) {
                if (th) {
                    RectFill(win->RPort, x1 - boxsize, y0, x1, y0 + boxsize);
                }
                if (bh) {
                    RectFill(win->RPort, x1 - boxsize, y1 - boxsize, x1, y1);
                }
            }
        }

        /*
         *  Update the handles of the image.  This information is
         *  needed when we need to check if the user has hit the
         *  buttons or not.
         */

        d->handles.MinX = x0;
        d->handles.MaxX = x1;
        d->handles.MinY = y0;
        d->handles.MaxY = y1;
    }
}
///
/// LassoRectButtonDown
Local
VOID LassoRectButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD mousex = msg->mousex;
    WORD mousey = msg->mousey;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;
    struct Rectangle *sb = &frame->selbox;

    if( IsFrameBusy(frame) ) return;

    /*
     *  If the window has an old display rectangle visible, remove it.
     */

    if( IsAreaSelected(frame) && (selection->selstatus & SELF_DRAWN) ) { /* Delete old. */
        BOOL corner = FALSE;
        struct Rectangle *cb = &frame->disp->handles;
        UWORD boxsize = 5;

        EraseSelection( frame );

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
            DrawSelection( frame, DSBF_INTERIM );
            UpdateIWSelbox(frame,TRUE);
            frame->selection.selstatus |= SELF_BUTTONDOWN;
            D(bug("Picked image handle (%d,%d)\n",xloc,yloc));
            return;
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

    selection->selstatus |= SELF_BUTTONDOWN;
}
///
/// LassoRectControlButtonDown
Local
VOID LassoRectControlButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD mousex = msg->mousex;
    WORD mousey = msg->mousey;
    struct Rectangle *cb = &frame->disp->handles;

    if (IsFrameBusy(frame)) return;

    if( mousex >= cb->MinX && mousex <= cb->MaxX && mousey >= cb->MinY && mousey <= cb->MaxY ) {
        /*
         *  Yes, it is within the area
         */

        EraseSelection( frame );
        selection->fixoffsetx   = mousex - cb->MinX;
        selection->fixoffsety   = mousey - cb->MinY;
        selection->selstatus    |= (SELF_BUTTONDOWN|SELF_CONTROLDOWN);
    } else {
        ButtonDown( frame, msg );
    }
}
///
/// LassoRectButtonUp
Local
VOID LassoRectButtonUp( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;
    struct Rectangle *sb = &frame->selbox;

    if( IsFrameBusy(frame) ) return;

    if( selection->selstatus & SELF_CONTROLDOWN ) {
        WORD xx, yy, w, h;

        EraseSelection( frame );

        /*
         *  Calculate the rectangle location - redrawing it.
         */

        w = sb->MaxX - sb->MinX;
        h = sb->MaxY - sb->MinY;

        CalcMouseCoords( frame, selection->fixoffsetx, selection->fixoffsety, &xx, &yy );

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

        UpdateIWSelbox( frame,TRUE );
        DrawSelection( frame, 0L );
        selection->selstatus &= ~(SELF_BUTTONDOWN|SELF_CONTROLDOWN);
    } else {

        if(xloc == sb->MinX || yloc == sb->MinY) {
            /*
             *  Image is too small, so it will be removed.
             */
            UnselectImage( frame );
            UpdateIWSelbox( frame, TRUE );
            selection->selstatus &= ~(SELF_BUTTONDOWN|SELF_DRAWN);
        } else {
            /*
             *  Remove the old display rectangle, if needed.
             */

            EraseSelection( frame );

            /*
             *  Reorient the rectangle and put up the new one.
             */

            sb->MaxX = xloc;
            sb->MaxY = yloc;
            ReorientSelbox( sb );
            UpdateIWSelbox( frame,TRUE );
            DrawSelection( frame, 0L );
            selection->selstatus &= ~(SELF_BUTTONDOWN);
            D(bug("Marked select end (%d,%d)\n",xloc,yloc));
        }
    }
}
///
/// LassoRectMouseMove
Local
VOID LassoRectMouseMove( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;
    struct Rectangle *sb = &frame->selbox;

    if( IsAreaSelected(frame) && (selection->selstatus & SELF_BUTTONDOWN) ) {
        /* Overdraw previous */
        EraseSelection( frame );

        if( selection->selstatus & SELF_CONTROLDOWN ) {
            WORD xx, yy, w, h;

            w = sb->MaxX - sb->MinX;
            h = sb->MaxY - sb->MinY;

            CalcMouseCoords( frame, selection->fixoffsetx, selection->fixoffsety, &xx, &yy );

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
        DrawSelection( frame, DSBF_INTERIM );
        UpdateIWSelbox( frame, FALSE );
    }
}
///
/// LassoRectIsInArea
/*
 *  This function may be called from other tasks as well, which is why
 *  profiling must be turned off.
 */
Local
BOOL LassoRectIsInArea( FRAME *frame, WORD row, WORD col )
{
    BOOL res = FALSE;

    if( col >= frame->selbox.MinX && col < frame->selbox.MaxX &&
        row >= frame->selbox.MinY && row < frame->selbox.MaxY )
    {
        res = TRUE;
    }

    return res;
}
///

/// LassoRectDraw() & LassoRectErase()
Local
VOID LassoRectDraw( FRAME *frame, ULONG flags )
{
    DrawSelectRectangle( frame,
                         frame->selbox.MinX, frame->selbox.MinY,
                         frame->selbox.MaxX, frame->selbox.MaxY,
                         flags );
}

Local
VOID LassoRectErase( FRAME *frame )
{
    DrawSelectRectangle( frame,
                         frame->selbox.MinX, frame->selbox.MinY,
                         frame->selbox.MaxX, frame->selbox.MaxY,
                         (frame->selection.selstatus & SELF_BUTTONDOWN) ? DSBF_INTERIM : 0L );
}
///

/*
 *  Rescales the selection from src to dst, by using the image
 *  sizes as a basis.
 */

VOID LassoRectRescale( FRAME *src, FRAME *dst )
{
    dst->selbox.MinX = src->selbox.MinX * dst->pix->width / src->pix->width;
    dst->selbox.MaxX = src->selbox.MaxX * dst->pix->width / src->pix->width;
    dst->selbox.MinY = src->selbox.MinY * dst->pix->height / src->pix->height;
    dst->selbox.MaxY = src->selbox.MaxY * dst->pix->height / src->pix->height;
}

VOID LassoRectCopy( FRAME *src, FRAME *dst )
{
    dst->selbox = src->selbox;
}

Prototype PERROR InitLassoRectSelection( FRAME *frame );

PERROR InitLassoRectSelection( FRAME *frame )
{
    struct Selection *s = &frame->selection;

    D(bug("InitLassoRectSelection(%08X)\n",frame));

    s->ButtonDown        = LassoRectButtonDown;
    s->ButtonUp          = LassoRectButtonUp;
    s->ControlButtonDown = LassoRectControlButtonDown;
    s->DrawSelection     = LassoRectDraw;
    s->EraseSelection    = LassoRectErase;
    s->MouseMove         = LassoRectMouseMove;
    s->IsInArea          = LassoRectIsInArea;
    s->Rescale           = LassoRectRescale;
    s->Copy              = LassoRectCopy;

    return PERR_OK;
}

/*---------------------------------------------------------------------*/
/* Fixed rectangle selection */

Local VOID FixedRectDraw( FRAME *, ULONG );

/// FixedRectButtonDown
Local
VOID FixedRectButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD mousex = msg->mousex;
    WORD mousey = msg->mousey;
    struct Rectangle *cb = &frame->disp->handles;

    // D(bug("FixedRectButtonDown()\n"));

    if( mousex >= cb->MinX && mousex <= cb->MaxX && mousey >= cb->MinY && mousey <= cb->MaxY ) {

        selection->fixoffsetx = mousex - cb->MinX;
        selection->fixoffsety = mousey - cb->MinY;

        selection->selstatus |= SELF_BUTTONDOWN;
    }
}
///
/// FixedRectButtonUp
Local
VOID FixedRectButtonUp( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;
    struct gFixRectMessage *gfr;

    gfr = (struct gFixRectMessage *) AllocPPTMsg(sizeof(struct gFixRectMessage), globxd);

    if( gfr ) {
        WORD xx, yy;

        CalcMouseCoords( frame, selection->fixoffsetx, selection->fixoffsety, &xx, &yy );
        selection->selstatus &= ~SELF_BUTTONDOWN;

        D(bug("FIXEDRECT msg sent: (%d,%d)\n",xloc-xx,yloc-yy));
        gfr->msg.code = PPTMSG_FIXED_RECT;
#if 1
        gfr->x = xloc - xx;
        gfr->y = yloc - yy;
#else
        gfr->x = selection->fixrect.Left;
        gfr->y = selection->fixrect.Top;
#endif
        SendInputMsg( frame, gfr );
    } else {
        Panic("Can't alloc gFixRectMsg\n");
    }
}
///
/// FixedRectMouseMove
Local
VOID FixedRectMouseMove( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;
    struct IBox *gfr = &selection->fixrect;

    // D(bug("FixedRectMouseMove(%s)\n", selection->selstatus & SELF_BUTTONDOWN ? "DOWN" : "UP" ));

    if( selection->selstatus & SELF_BUTTONDOWN ) {
        WORD xx, yy;
        /*
         *  Remove previous, if needed
         *  BUG: May conflict with the lasso_rect stuff.
         */

        EraseSelection( frame );

        CalcMouseCoords( frame, selection->fixoffsetx, selection->fixoffsety, &xx, &yy );

        gfr->Left = xloc-xx; gfr->Top = yloc-yy;

        DrawSelection( frame, DSBF_INTERIM );
    }
}
///
/// FixedRectDraw
Local
VOID FixedRectDraw( FRAME *frame, ULONG flags )
{
    struct IBox *ib = &frame->selection.fixrect;

    // D(bug("FixedRectDraw(frame=%08X,flags=%08lu)\n",frame,flags));

    DrawSelectRectangle( frame, ib->Left, ib->Top, ib->Left+ib->Width,
                         ib->Top+ib->Height, flags | DSBF_FIXEDRECT );
}
///
/// FixedRectErase
Local
VOID FixedRectErase( FRAME *frame )
{
    // D(bug("FixedRectErase(frame=%08X)\n",frame));

    FixedRectDraw( frame, 0L );
}
///
Prototype PERROR InitFixedRectSelection( FRAME *frame );

PERROR InitFixedRectSelection( FRAME *frame )
{
    struct Selection *s = &frame->selection;

    D(bug("InitFixedRectSelection(%08X)\n",frame));

    s->ButtonDown        = FixedRectButtonDown;
    s->ButtonUp          = FixedRectButtonUp;
    // s->ControlButtonDown = LassoRectControlButtonDown;
    s->DrawSelection     = FixedRectDraw;
    s->EraseSelection    = FixedRectErase;
    s->MouseMove         = FixedRectMouseMove;
    s->IsInArea          = LassoRectIsInArea;
    s->Rescale           = LassoRectRescale;
    s->Copy              = LassoRectCopy;

    return PERR_OK;
}

