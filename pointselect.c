/*
    PROJECT: ppt
    MODULE:  rectangleselect.c

    This module contains the point selection routines.  They're placed
    here for easier profiling.  This code is always run
    on the main task.

    $Id: pointselect.c,v 1.3 1999/09/05 17:08:59 jj Exp $

 */

#include "defs.h"
#include "misc.h"

#ifndef GRAPHICS_GFXMACROS_H
#include <graphics/gfxmacros.h>
#endif

/*-------------------------------------------------------------------------*/

/// DrawPickPointSelector
Local
VOID DrawPickPointSelector(FRAME *frame, WORD mousex, WORD mousey)
{
    WORD winbottom, winright, xoffset, yoffset;
    struct Window *win = frame->disp->win;

    if( frame->disp->RenderArea ) {
        struct IBox *abox;
        GetAttr(AREA_AreaBox, frame->disp->RenderArea, (ULONG *) & abox);
        winright = abox->Width+abox->Left;
        winbottom = abox->Height+abox->Top;
        xoffset = abox->Left;
        yoffset = abox->Top;
    } else return;

    CLAMP(mousex,xoffset,winright);
    CLAMP(mousey,yoffset,winbottom);

    SetDrMd(win->RPort, COMPLEMENT);

    SetDrPt(win->RPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

    Move( win->RPort, mousex, yoffset );
    Draw( win->RPort, mousex, winbottom );

    Move( win->RPort, xoffset, mousey );
    Draw( win->RPort, winright, mousey );
}
///

/// PickPointButtonDown
Local
VOID PickPointButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;
    struct gPointMessage *gp;

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
}
///
/// PickPointMouseMove
Local
VOID PickPointMouseMove( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    static WORD ox = 0,oy = 0;

    /* Erase */

    if( selection->selstatus & SELF_DRAWN ) {
        DrawPickPointSelector(frame, ox, oy);
    }

    /* Redraw */

    DrawPickPointSelector(frame, msg->mousex, msg->mousey);
    selection->selstatus |= SELF_DRAWN;
    ox = msg->mousex;
    oy = msg->mousey;
}
///

#if 0
/// PickPointControlButtonDown
Local
VOID PickPointControlButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD mousex = msg->mousex;
    WORD mousey = msg->mousey;
    struct Rectangle *cb = &frame->disp->handles;
}
///
/// PickPointButtonUp
Local
VOID PickPointButtonUp( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc   = msg->xloc;
    WORD yloc   = msg->yloc;
    struct Rectangle *sb = &frame->selbox;
}
///
/// PickPointIsInArea
Local
BOOL PickPointIsInArea( FRAME *frame, struct MouseLocationMsg *msg )
{
}
///

/// PickPointDraw() & PickPointErase()
Local
VOID PickPointDraw( FRAME *frame, ULONG flags )
{
}

Local
VOID PickPointErase( FRAME *frame )
{
}
///
#endif

Prototype PERROR InitPickPointSelection( FRAME *frame );

PERROR InitPickPointSelection( FRAME *frame )
{
    struct Selection *s = &frame->selection;

    D(bug("InitPickPointSelection(%08X)\n",frame));

    s->ButtonDown        = PickPointButtonDown;
    s->ButtonUp          = NULL;
    s->ControlButtonDown = NULL;
    s->DrawSelection     = NULL;
    s->EraseSelection    = NULL;
    s->MouseMove         = PickPointMouseMove;
    s->IsInArea          = NULL;

    return PERR_OK;
}


