/*
    PROJECT: ppt
    MODULE:  freeselect.c

    Freeform selection routines.

    $Id: freeselect.c,v 1.1 1999/07/12 19:55:58 jj Exp $

 */

#include "defs.h"
#include "misc.h"

#include <proto/timer.h>

#include <math.h>

#ifndef GRAPHICS_GFXMACROS_H
#include <graphics/gfxmacros.h>
#endif

/*------------------------------------------------------------------------*/

Local
VOID DrawArea( FRAME *frame, Point *coords, WORD nPoints )
{
    DISPLAY *d = frame->disp;
    struct RastPort *rPort = d->win->RPort;
    struct IBox *abox;
    WORD x,y, winwidth, winheight, xoffset, yoffset;
    int i;

    GetAttr( AREA_AreaBox, d->RenderArea, (ULONG *) &abox );
    winwidth  = abox->Width;
    winheight = abox->Height;
    xoffset   = abox->Left;
    yoffset   = abox->Top;

    SetDrMd(rPort, COMPLEMENT);
    SetDrPt(rPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

    x = MULS16((coords[0].x - frame->zoombox.Left),winwidth) / frame->zoombox.Width + xoffset;
    y = MULS16((coords[0].y - frame->zoombox.Top),winheight) / frame->zoombox.Height + yoffset;

    Move( rPort, x, y );

    for( i = 1; i < nPoints; i++ ) {
        x = MULS16((coords[i].x - frame->zoombox.Left),winwidth) / frame->zoombox.Width + xoffset;
        y = MULS16((coords[i].y - frame->zoombox.Top),winheight) / frame->zoombox.Height + yoffset;

        Draw( rPort, x, y );
    }
}

Local
VOID FreeDraw( FRAME *frame, ULONG flags )
{
    struct Selection *selection = &frame->selection;

    DrawArea( frame, selection->vertices, selection->nVertices );
}

Prototype PERROR InitFreeSelection( FRAME *frame );

PERROR InitFreeSelection( FRAME *frame )
{
    struct Selection *s = &frame->selection;

    D(bug("InitFixedRectSelection(%08X)\n",frame));

    //s->ButtonDown        = FixedRectButtonDown;
    //s->ButtonUp          = FixedRectButtonUp;
    // s->ControlButtonDown = LassoRectControlButtonDown;
    s->DrawSelection     = FreeDraw;
    //s->EraseSelection    = FixedRectErase;
    //s->MouseMove         = FixedRectMouseMove;
    //s->IsInArea          = LassoRectIsInArea;
    //s->Rescale           = LassoRectRescale;
    //s->Copy              = LassoRectCopy;

    return PERR_OK;
}


