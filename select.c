/*
    PROJECT: ppt
    MODULE:  select.c

    This module contains selection routines.  They're placed
    here for easier profiling.  This code is always run
    on the main task.

    $Id: select.c,v 1.2 1999/03/17 23:09:55 jj Exp $

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

Local VOID DrawSelectRectangle(FRAME * frame, WORD x0, WORD y0, WORD x1, WORD y1, ULONG flags);
Local VOID DrawSelectCircle( FRAME *, WORD, WORD, WORD );

/// RemoveSelectBox()
/*
   Removes the select box, if needed.
 */

Prototype VOID RemoveSelectBox(FRAME *);

VOID RemoveSelectBox(FRAME * frame)
{
    switch (frame->selectmethod) {
    case GINP_LASSO_RECT:
        /*
         *  Does the rectangle exist?
         */
        if (frame->selstatus & SELF_RECTANGLE) {
            struct Rectangle *sb = &frame->selbox;

            /*
             *  Yes, it exists.  If we are in a situation where the user
             *  is holding the LMB down and is moving around in the image,
             *  we won't delete the resize handles, because they don't exist.
             */

            DrawSelectRectangle(frame, sb->MinX, sb->MinY, sb->MaxX, sb->MaxY,
                                (frame->selstatus & SELF_BUTTONDOWN) ? DSBF_INTERIM : 0L);
            frame->selstatus &= ~SELF_RECTANGLE;
        }
        break;

    case GINP_FIXED_RECT:
        if( frame->selstatus & SELF_RECTANGLE ) {
            struct IBox *ib = &frame->fixrect;

            DrawSelectRectangle( frame, ib->Left, ib->Top, ib->Left+ib->Width,
                                 ib->Top+ib->Height, DSBF_FIXEDRECT );
            frame->selstatus &= ~SELF_RECTANGLE;
        }
        break;

    case GINP_LASSO_CIRCLE:
        if( frame->selstatus & SELF_RECTANGLE ) {
            DrawSelectCircle( frame, frame->circlex, frame->circley, frame->circleradius );
            frame->selstatus &= ~SELF_RECTANGLE;
        }
        break;

    default:
        break;
    }
}
///
/// DrawSelectBox
/*
    This function draws a select box onto frame display window.  It knows
    automatically what to do, according to the current status.
 */

Prototype VOID DrawSelectBox( FRAME *frame, ULONG flags );

VOID DrawSelectBox( FRAME *frame, ULONG flags )
{
    switch( frame->selectmethod ) {
      case GINP_LASSO_RECT:
        if( frame->selbox.MinX != ~0 && !(frame->selstatus & SELF_RECTANGLE) ) {
            struct Rectangle *sb = &frame->selbox;

            DrawSelectRectangle(frame, sb->MinX, sb->MinY, sb->MaxX, sb->MaxY, flags);
            frame->selstatus |= SELF_RECTANGLE;
        }
        break;

      case GINP_FIXED_RECT:
        if( !(frame->selstatus & SELF_RECTANGLE) ) {
            struct IBox *ib = &frame->fixrect;

            DrawSelectRectangle( frame, ib->Left, ib->Top, ib->Left+ib->Width,
                                 ib->Top+ib->Height, DSBF_FIXEDRECT );
            frame->selstatus |= SELF_RECTANGLE;
        }

        break;

      case GINP_LASSO_CIRCLE:
        if( !(frame->selstatus & SELF_RECTANGLE) ) {
            DrawSelectCircle( frame, frame->circlex, frame->circley, frame->circleradius );
            frame->selstatus |= SELF_RECTANGLE;
        }
        break;
    }
}
///
/// DrawSelectCircle()
Local
VOID DrawSelectCircle( FRAME *frame, WORD x, WORD y, WORD r )
{
    DISPLAY *d = frame->disp;
    struct Window *win = d->win;
    WORD winwidth, winheight, xoffset, yoffset;
    struct IBox *abox;
    WORD rx,ry;

    D(bug("DrawSelectCircle( x=%d, y=%d, r=%d)\n",x,y,r));

    GetAttr(AREA_AreaBox, frame->disp->RenderArea, (ULONG *) & abox);
    winwidth = abox->Width;
    winheight = abox->Height;
    xoffset = abox->Left;
    yoffset = abox->Top;

    /*
     *  Scale and limit the values to be inside the window
     */

    x -= frame->zoombox.Left;
    y -= frame->zoombox.Top;

    x = (LONG) (MULS16(x, winwidth)) / (WORD) (frame->zoombox.Width);
    y = (LONG) (MULS16(y, winheight)) / (WORD) frame->zoombox.Height;
    rx = (LONG) (MULS16(r, winwidth)) / (WORD) frame->zoombox.Width;
    ry = (LONG) (MULS16(r, winheight)) / (WORD) frame->zoombox.Height;

    /*
     *  Draw!
     */

    x += xoffset;
    y += yoffset;

    SetDrMd(win->RPort, COMPLEMENT);
    SetDrPt(win->RPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

    WaitTOF();

    DrawEllipse( win->RPort, x, y, rx, ry );
}
///

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

Local
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
        if( f->selbox.MinX == ~0 ) {
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
                    UpdateIWSelbox(frame,TRUE);
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

                UpdateIWSelbox( frame,TRUE );
                DrawSelectBox( frame, 0L );
                frame->selstatus &= ~(SELF_BUTTONDOWN|SELF_CONTROLDOWN);
            } else {

                if(xloc == sb->MinX || yloc == sb->MinY) {
                    /*
                     *  Image is too small, so it will be removed.
                     */
                    UnselectImage( frame );
                    UpdateIWSelbox( frame, TRUE );
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
                    UpdateIWSelbox( frame,TRUE );
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
                UpdateIWSelbox( frame, TRUE );
                frame->selstatus &= ~(SELF_BUTTONDOWN|SELF_RECTANGLE);
            } else {
                sb->MinX = frame->circlex - frame->circleradius;
                sb->MinY = frame->circley - frame->circleradius;
                sb->MaxX = frame->circlex + frame->circleradius;
                sb->MaxY = frame->circley + frame->circleradius;
                UpdateIWSelbox( frame,TRUE );
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
                UpdateIWSelbox( frame, FALSE );
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

