/*
    PROJECT: ppt
    MODULE:  freeselect.c

    Freeform selection routines.

    $Id: freeselect.c,v 6.0 1999/09/08 22:48:17 jj Exp $

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
VOID FreeMakeBoundingBox( FRAME *frame )
{
    struct Selection *selection = &frame->selection;
    struct Rectangle rect = {32767,32767,-1,1};
    int i;

    for(i = 0; i < selection->nVertices; i++ ) {
        WORD x, y;

        x = selection->vertices[i].x;
        y = selection->vertices[i].y;

        if( x < rect.MinX ) rect.MinX = x;
        if( x > rect.MaxX ) rect.MaxX = x;

        if( y < rect.MinY ) rect.MinY = y;
        if( y > rect.MaxY ) rect.MaxY = y;
    }

    D(bug("Calculated bounding box: (%d,%d)-(%d,%d)\n",
           rect.MinX,rect.MinY, rect.MaxX, rect.MaxY ));

    frame->selbox = rect;
}

Local
VOID FreeButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc = msg->xloc;
    WORD yloc = msg->yloc;

    if( IsFrameBusy(frame) ) return;

    if( IsAreaSelected(frame) && (selection->selstatus & SELF_DRAWN) ) { /* Delete old. */
        EraseSelection( frame );
    }

    /*
     *  Just mark the area as started.
     */

    selection->vertices[0].x = xloc;
    selection->vertices[0].y = yloc;
    selection->nVertices = 1;

    selection->selstatus |= SELF_BUTTONDOWN;

    frame->selbox.MinX = 0;

    D(bug("Marked freearea start (%d,%d)\n",xloc,yloc));
}

Local
VOID FreeMouseMove( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc = msg->xloc;
    WORD yloc = msg->yloc;

    if( IsAreaSelected(frame) && (selection->selstatus & SELF_BUTTONDOWN) ) {
        EraseSelection( frame );

        selection->vertices[selection->nVertices].x = xloc;
        selection->vertices[selection->nVertices].y = yloc;

        selection->nVertices++;

        D(bug("Vertex %d: (%d,%d)\n", selection->nVertices-1, xloc, yloc ));

        // BUG: Uses lots of time.
        FreeMakeBoundingBox( frame );
        DrawSelection( frame, DSBF_INTERIM );
        UpdateIWSelbox( frame, FALSE );
    }
}

Local
VOID FreeButtonUp( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc = msg->xloc;
    WORD yloc = msg->yloc;

    if( IsFrameBusy(frame) ) return;

    EraseSelection( frame );

    // Make sure it's closing.
    selection->vertices[selection->nVertices] = selection->vertices[0];
    selection->nVertices++;

    FreeMakeBoundingBox( frame );

    UpdateIWSelbox( frame, TRUE );
    DrawSelection( frame, 0L );

    selection->selstatus &= ~SELF_BUTTONDOWN;

    D(bug("Marked final vertex %d at (%d,%d)\n",selection->nVertices-1,
          selection->vertices[0].x, selection->vertices[0].y ));
}

Local
VOID DrawArea( FRAME *frame, Point *coords, WORD nPoints )
{
    struct Selection *selection = &frame->selection;
    DISPLAY *d = frame->disp;
    struct RastPort *rPort = d->win->RPort;
    struct IBox *abox;
    WORD x,y, winwidth, winheight, xoffset, yoffset;
    int i;

    if( selection->nVertices > 2 ) {
        GetAttr( AREA_AreaBox, d->RenderArea, (ULONG *) &abox );
        winwidth  = abox->Width;
        winheight = abox->Height;
        xoffset   = abox->Left;
        yoffset   = abox->Top;

        SetDrMd(rPort, COMPLEMENT);
        SetDrPt(rPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

        x = MULS16((selection->vertices[0].x - frame->zoombox.Left),winwidth) / frame->zoombox.Width + xoffset;
        y = MULS16((selection->vertices[0].y - frame->zoombox.Top),winheight) / frame->zoombox.Height + yoffset;

        Move( rPort, x, y );

        for( i = 1; i < selection->nVertices; i++ ) {
            x = MULS16((selection->vertices[i].x - frame->zoombox.Left),winwidth) / frame->zoombox.Width + xoffset;
            y = MULS16((selection->vertices[i].y - frame->zoombox.Top),winheight) / frame->zoombox.Height + yoffset;

            Draw( rPort, x, y );
        }
    }
}

Local
VOID FreeDraw( FRAME *frame, ULONG flags )
{
    struct Selection *selection = &frame->selection;

    DrawArea( frame, selection->vertices, selection->nVertices );
}

Local
VOID FreeErase( FRAME *frame )
{
    struct Selection *selection = &frame->selection;

    DrawArea( frame, selection->vertices, selection->nVertices );
}

Local
BOOL FreeIsInArea( FRAME *frame, WORD row, WORD col )
{
    ROWPTR cp;
    /*
    cp = GetPixelRow( frame->selection.mask, row );
    if( cp[col] )
        return TRUE;
    */
    return FALSE;
}

Prototype PERROR InitFreeSelection( FRAME *frame );

PERROR InitFreeSelection( FRAME *frame )
{
    struct Selection *selection = &frame->selection;

    D(bug("InitFreeSelection(%08X)\n",frame));
    /*
    if( !frame->mask || NOTSAMESIZE( frame->mask, frame ) ) {
        frame->mask = MakeFrame( frame );
        frame->pix->components = 1;
        frame->pix->colorspace = CS_UNKNOWN;

        if( InitFrame( frame->mask ) != PERR_OK )
            return PERR_ERROR;
    }
    */
    if( !selection->vertices ) {
        if( NULL == (selection->vertices = smalloc( sizeof(Point) * 1024 ))) {
            Panic( "Can't alloc vertices!!!" );
        }
    }

    selection->ButtonDown        = FreeButtonDown;
    selection->ButtonUp          = FreeButtonUp;
    // s->ControlButtonDown = LassoRectControlButtonDown;
    selection->DrawSelection     = FreeDraw;
    selection->EraseSelection    = FreeErase;
    selection->MouseMove         = FreeMouseMove;
    selection->IsInArea          = FreeIsInArea;
    //s->Rescale           = LassoRectRescale;
    //s->Copy              = LassoRectCopy;

    return PERR_OK;
}


