/*
    PROJECT: ppt
    MODULE:  rectangleselect.c

    This module contains the point selection routines.  They're placed
    here for easier profiling.  This code is always run
    on the main task.

    $Id: pointselect.c,v 1.1 1999/05/30 18:14:39 jj Exp $

 */

#include "defs.h"
#include "misc.h"

/*-------------------------------------------------------------------------*/

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
/// PickPointMouseMove
Local
VOID PickPointMouseMove( FRAME *frame, struct MouseLocationMsg *msg )
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
    s->MouseMove         = NULL;
    s->IsInArea          = NULL;

    return PERR_OK;
}


