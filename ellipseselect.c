/*
    PROJECT: ppt
    MODULE:  select.c

    This module contains the rectangular selection routines.  They're placed
    here for easier profiling.  This code is always run
    on the main task.

    $Id: ellipseselect.c,v 6.0 1999/09/08 22:47:17 jj Exp $

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

/*----------------------------------------------------------------------------------*/

/// PPTDrawEllipse()
# if 1
/*
    Local attempt at a fast circle drawing routine, including
    clipping.

    Algorithm from Hearn-Baker: Computer Graphics, 2nd ed.

 */

#define INCLIP(x,y,clip) (((x) > (clip)->MinX) && ((x) < (clip)->MaxX) && ((y) > (clip)->MinY) && ((y) < (clip)->MaxY))
// #define INCLIP(x,y,c) 1

Local VOID INLINE
PPTDrawPoints( struct RastPort *rport, SHORT cx, SHORT cy, SHORT x, SHORT y, struct Rectangle *clip )
{
    if( INCLIP(cx+x,cy+y,clip) ) WritePixel( rport, cx+x,cy+y );
    if( INCLIP(cx-x,cy+y,clip) ) WritePixel( rport, cx-x,cy+y );
    if( INCLIP(cx+x,cy-y,clip) ) WritePixel( rport, cx+x,cy-y );
    if( INCLIP(cx-x,cy-y,clip) ) WritePixel( rport, cx-x,cy-y );
}

VOID PPTDrawEllipse( struct RastPort *rport, SHORT cx, SHORT cy, SHORT rx, SHORT ry, struct Rectangle *clip )
{
    int ry2, rx2;
    int tworx2, twory2;
    int p, px, py, x, y;
    UWORD ptrn = rport->LinePtrn;

    ry2 = ry*ry;
    rx2 = rx*rx;
    twory2 = 2*ry2;
    tworx2 = 2*rx2;
    /* Region 1 */

    x = 0;
    y = ry;
    PPTDrawPoints( rport, cx, cy, x, y, clip );
    p = ry2 - rx2 * ry + (rx2 / 4);
    px = 0;
    py = tworx2 * y;

    while( px < py ) {
        x++;
        px += twory2;
        if( p >= 0 ) {
            y--;
            py -= tworx2;
        }
        if( p < 0 ) {
            p += ry2+px;
        } else {
            p += ry2+px-py;
        }
        if( ptrn & (1 << (x & 0x0F)) )
            PPTDrawPoints( rport, cx, cy, x, y, clip );
    }

    /* Region 2 */

    p = ry2 * x * x + rx2 * (y-1)*(y-1) - rx2*ry2;

    while( y > 0 ) {
        y--;
        py -= tworx2;
        if( p <= 0 ) {
            x++;
            px += twory2;
        }
        if( p > 0 ) {
            p += rx2-py;
        } else {
            p += rx2-py+px;
        }
        if( ptrn & (1 << (y & 0x0F)) )
            PPTDrawPoints( rport, cx, cy, x, y, clip );
    }
}
#endif
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
    struct Rectangle cliparea;

    if(!win) return;

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

    x = (LONG) (MULS16(x, winwidth)+winwidth/2) / (WORD) (frame->zoombox.Width);
    y = (LONG) (MULS16(y, winheight)+winheight/2) / (WORD) frame->zoombox.Height;
    rx = (LONG) (MULS16(r, winwidth)) / (WORD) frame->zoombox.Width - 1;
    ry = (LONG) (MULS16(r, winheight)) / (WORD) frame->zoombox.Height - 1;

    x += xoffset;
    y += yoffset;

    cliparea.MinX = xoffset;
    cliparea.MinY = yoffset;
    cliparea.MaxX = xoffset + abox->Width;
    cliparea.MaxY = yoffset + abox->Height;

    /*
     *  Draw!  (Use DrawEllipse() if you don't want clipping...
     */

    SetDrMd(win->RPort, COMPLEMENT);
    SetDrPt(win->RPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

    WaitTOF();

    PPTDrawEllipse( win->RPort, x, y, rx, ry, &cliparea );
    // DrawEllipse( win->RPort, x, y, rx, ry );
}
///

/// ButtonDown
Local
VOID LassoCircleButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;

    if( IsFrameBusy(frame) ) return;

    /*
     *  If the window has an old circle visible, remove it.
     */

    if( IsAreaSelected(frame) && (selection->selstatus & SELF_DRAWN) ) { /* Delete old. */
        EraseSelection( frame );
    }

    selection->circlex = xloc;
    selection->circley = yloc;
    selection->circleradius = 0;
    selection->selstatus |= SELF_BUTTONDOWN;
    frame->selbox.MinX = 0; /* Signal: area selected. */

    D(bug("Marked circle select begin (%d,%d)\n",xloc,yloc));
}
///
/// ControlButtonDown
Local
VOID LassoCircleControlButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD mousex = msg->mousex;
    WORD mousey = msg->mousey;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;

    if (IsFrameBusy(frame)) return;

    if( (SQR(xloc - selection->circlex) + SQR( yloc - selection->circley )) < SQR(selection->circleradius) ) {
        WORD xx, yy;

        EraseSelection( frame );
        CalcMouseCoords( frame, selection->circlex, selection->circley, &xx, &yy );

        selection->fixoffsetx   = mousex - xx;
        selection->fixoffsety   = mousey - yy;
        selection->selstatus    |= (SELF_BUTTONDOWN|SELF_CONTROLDOWN);
    } else {
        ButtonDown( frame, msg );
    }
}
///

/// MakeBoundingBox
Local
VOID LassoCircleMakeBoundingBox( FRAME *frame )
{
    struct Selection *selection = &frame->selection;
    struct Rectangle *sb = &frame->selbox;

    sb->MinX = selection->circlex - selection->circleradius - 1;
    sb->MinY = selection->circley - selection->circleradius - 1;
    sb->MaxX = selection->circlex + selection->circleradius + 1;
    sb->MaxY = selection->circley + selection->circleradius + 1;
    CLAMP( sb->MinX, 0, frame->pix->width );
    CLAMP( sb->MaxX, 0, frame->pix->width );
    CLAMP( sb->MinY, 0, frame->pix->height );
    CLAMP( sb->MaxY, 0, frame->pix->height );
}
///

/// ButtonUp
Local
VOID LassoCircleButtonUp( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;

    if( IsFrameBusy(frame) ) return;

    if( xloc == selection->circlex && yloc == selection->circley ) {
        /* Too small an image */
        UnselectImage( frame );
        UpdateIWSelbox( frame, TRUE );
        selection->selstatus &= ~(SELF_BUTTONDOWN|SELF_DRAWN);
    } else {
        ULONG radius;
        EraseSelection( frame );

        radius = sqrt( (double)SQR(selection->circlex-xloc) + (double)SQR(selection->circley-yloc) );
        selection->circleradius = radius;

        LassoCircleMakeBoundingBox( frame );

        UpdateIWSelbox( frame,TRUE );
        DrawSelection( frame, 0L );
        selection->selstatus &= ~SELF_BUTTONDOWN;
        D(bug("Marked circle at (%d,%d) with radius %d\n",
               selection->circlex, selection->circley, selection->circleradius ));
     }
}
///
/// LassoCircleMouseMove( FRAME *frame, struct MouseLocationMsg *msg )

Local
VOID LassoCircleMouseMove( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;

    if( IsAreaSelected(frame) && (selection->selstatus & SELF_BUTTONDOWN) ) {
        ULONG radius;

        EraseSelection( frame );

        if( selection->selstatus & SELF_CONTROLDOWN ) {
            WORD xx, yy;

            CalcMouseCoords( frame, selection->fixoffsetx, selection->fixoffsety, &xx, &yy );
            selection->circlex = xloc - xx;
            selection->circley = yloc - yy;

        } else {
            radius = sqrt( (double)SQR(selection->circlex-xloc) + (double)SQR(selection->circley-yloc) );
            selection->circleradius = radius;
        }
        LassoCircleMakeBoundingBox( frame );
        DrawSelection( frame, DSBF_INTERIM );
        UpdateIWSelbox( frame, FALSE );
    }
}
///
/// IsInArea
Local
BOOL LassoCircleIsInArea( FRAME *frame, WORD row, WORD col )
{
    struct Selection *selection = &frame->selection;
    BOOL res    = FALSE;

//    D(bug("...(%d,%d) : x=%d,y=%d,r=%d\n",row,col,selection->circlex,selection->circley,selection->circleradius));
//    D(bug("......distance=%d\n",SQR(col-selection->circlex)+SQR(row-selection->circley)));
    if( (SQR(col - selection->circlex) + SQR( row - selection->circley)) <= SQR(selection->circleradius-1) )
    {
        // D(bug("......is inside radius\n"));
        res = TRUE;
    }

    return res;
}
///
/// LassoCircleDraw() & LassoCircleErase()
Local
VOID LassoCircleDraw( FRAME *frame, ULONG flags )
{
    DrawSelectCircle( frame, frame->selection.circlex,
                             frame->selection.circley,
                             frame->selection.circleradius );
}

Local
VOID LassoCircleErase( FRAME *frame )
{
    DrawSelectCircle( frame, frame->selection.circlex,
                             frame->selection.circley,
                             frame->selection.circleradius );
}
///

/*
 *  BUG: if scaling is non-uniform, will cause problems.
 */

VOID LassoCircleRescale( FRAME *src, FRAME *dst )
{
    dst->selection.circlex = src->selection.circlex * dst->pix->width / src->pix->width;
    dst->selection.circley = src->selection.circley * dst->pix->height / src->pix->height;
    dst->selection.circleradius = src->selection.circleradius * dst->pix->width / src->pix->width;
    LassoCircleMakeBoundingBox( dst );
}


VOID LassoCircleCopy( FRAME *src, FRAME *dst )
{
    dst->selection.circlex = src->selection.circlex;
    dst->selection.circley = src->selection.circley;
    dst->selection.circleradius = src->selection.circleradius;
    LassoCircleMakeBoundingBox( dst );
}

Prototype PERROR InitLassoCircleSelection( FRAME *frame );

PERROR InitLassoCircleSelection( FRAME *frame )
{
    struct Selection *s = &frame->selection;

    s->ButtonDown        = LassoCircleButtonDown;
    s->ButtonUp          = LassoCircleButtonUp;
    s->ControlButtonDown = LassoCircleControlButtonDown;
    s->DrawSelection     = LassoCircleDraw;
    s->EraseSelection    = LassoCircleErase;
    s->MouseMove         = LassoCircleMouseMove;
    s->IsInArea          = LassoCircleIsInArea;
    s->Rescale           = LassoCircleRescale;
    s->Copy              = LassoCircleCopy;

    return PERR_OK;
}


