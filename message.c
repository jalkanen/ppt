/*
    PROJECT: ppt
    MODULE : message.c

    $Id: message.c,v 2.3 1997/08/31 20:53:35 jj Exp $

    This module contains code about message handling routines.
*/

#include "defs.h"
#include "misc.h"

/*
 *  If defined, all messages and their types will be dumped onto
 *  the standard output.
 */

#undef DEBUG_MESSAGES

/*------------------------------------------------------------------------*/

#ifdef DEBUG_MESSAGES
#define DB(x)       x;
#else
#define DB(x)
#endif

/*------------------------------------------------------------------------*/

Prototype struct PPTMessage *InitPPTMsg( void );
Prototype VOID              PurgePPTMsg( struct PPTMessage * );
Prototype VOID              DoPPTMsg( struct MsgPort *, struct PPTMessage * );

Prototype ASM PERROR        StartInput( REG(a0) FRAME *, REG(a1) APTR, REG(d0) ULONG, REG(a6) EXTBASE * );
Prototype ASM VOID          StopInput( REG(a0) FRAME *, REG(a6) EXTBASE * );
Prototype VOID              FinishFrameInput( FRAME * );
Prototype VOID              SetupFrameForInput( struct PPTMessage * );
Prototype VOID              ClearFrameInput( FRAME * );

Prototype VOID              SendInputMsg(FRAME *, struct PPTMessage *);

Prototype LONG              EmptyMsgPort( struct MsgPort *, EXTBASE * );

/*------------------------------------------------------------------------*/
/* Code */

/*
    This is a more complex PPTMessage system.  Allows for more configurability
*/

Prototype struct PPTMessage *AllocPPTMsg( ULONG, EXTBASE * );

struct PPTMessage *AllocPPTMsg( ULONG size, EXTBASE *ExtBase )
{
    struct PPTMessage *msg = NULL;

    if(msg = smalloc( size )) {
        bzero( msg, size );
        msg->msg.mn_Node.ln_Type = NT_MESSAGE;
        msg->msg.mn_Length       = size;
        msg->msg.mn_ReplyPort    = ExtBase->mport;
    }

    return msg;
}

Prototype VOID FreePPTMsg( struct PPTMessage *pmsg, EXTBASE *ExtBase );

VOID FreePPTMsg( struct PPTMessage *pmsg, EXTBASE *ExtBase )
{
    if( pmsg ) {
        DB(bug("FreePPTMsg(%08X) : code %lX\n", pmsg, pmsg->code ));
        sfree( pmsg );
    }
}

Prototype VOID SendPPTMsg( struct MsgPort *, struct PPTMessage *, EXTBASE * );

VOID SendPPTMsg( struct MsgPort *mp, struct PPTMessage *pmsg, EXTBASE *ExtBase )
{
    struct ExecBase *SysBase = ExtBase->lb_Sys;

    DB(bug("SendPPTMsg(%08X) : code %lX\n",pmsg, pmsg->code ));

    Forbid();
    PutMsg( mp, (struct Message *) pmsg );
    Permit();
}

/*
    A simple, synchronous PPT messaging system.
*/

/*
    Initialize a PPTMsg.
*/
struct PPTMessage *InitPPTMsg( void )
{
    struct Library *SysBase = SYSBASE();
    struct MsgPort *replyport;
    struct PPTMessage *msg;

    D(bug("InitPPTMsg()\n"));
    if( NULL == (replyport = CreateMsgPort())) {
        D(bug("\tCouldn't make MsgPort!\n"));
        return NULL;
    }

    msg = pzmalloc( sizeof(struct PPTMessage) );
    if(!msg) {
        DeleteMsgPort(replyport);
        return NULL;
    }

    msg->msg.mn_Node.ln_Type = NT_MESSAGE;
    msg->msg.mn_Length = sizeof(struct PPTMessage);
    msg->msg.mn_ReplyPort = replyport;

    return msg;
}

/*
    Remove a PPTMessage allocated by InitPPTMsg()
*/

void PurgePPTMsg( struct PPTMessage *msg )
{
    APTR SysBase = SYSBASE();

    if( msg ) {
        D(bug("PurgePPTMsg(%08x)\n",msg));
        if(msg->msg.mn_ReplyPort)
            DeleteMsgPort(msg->msg.mn_ReplyPort);

        pfree(msg);
    }
}

/*
    Send a simple message asyncronously. Ignore answer.
*/
void DoPPTMsg( struct MsgPort *dest, struct PPTMessage *msg )
{
    APTR SysBase = SYSBASE();

    D(bug("DoPPTMsg(%08X,%08X)\n",dest,msg));

    if( dest && msg ) {
        Forbid();
        PutMsg(dest, (struct Message *)msg);
        Permit();

        D(bug("\tWaiting for answer...\n"));

        WaitPort(msg->msg.mn_ReplyPort); /* Wait for the answer... */

        D(bug("\tGot answer\n"));

        GetMsg(msg->msg.mn_ReplyPort);

        D(bug("\tExiting wait...\n"));
    }
}

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

/*--------------------------------------------------------------------*/

/*
    The frame is set up.
    pmsg points to an unanswered PPTMessage, that originally came from
    the external.
    BUG: No error checking
*/

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
    Send one input message to the external process.
*/

VOID SendInputMsg( FRAME *f, struct PPTMessage *pmsg )
{
    D(bug("\tSending frame input\n"));

    pmsg->frame = f;

    SendPPTMsg( f->selectport, pmsg, globxd );
}


/*
    Clear away any signs of us ever having had a other method than
    LASSO_RECT.
*/
VOID ClearFrameInput( FRAME *f )
{
    struct Library *SysBase = SYSBASE();
    struct IBox clrrect = {0};

    D(bug("ClearFrameInput(%08X)\n",f));

    RemoveSelectBox( f );
    LOCK(f);
    f->selectmethod = GINP_LASSO_RECT;
    f->selectdata   = NULL;
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

VOID FinishFrameInput( FRAME *f )
{
    D(bug("\tFinished frame input\n"));

    ChangeBusyStatus( f, BUSY_CHANGING );

    ClearFrameInput( f );
}


/*
    This empties a message port from all messages. The message count
    is returned.
*/
LONG EmptyMsgPort( struct MsgPort *mp, EXTBASE *ExtBase )
{
    struct Message *msg;
    LONG count = 0;
    struct ExecBase *SysBase = ExtBase->lb_Sys;

    D(bug("EmptyMsgPort()\n"));

    while( msg = GetMsg( mp ) ) {
        if( msg->mn_Node.ln_Type != NT_REPLYMSG ) {
            ReplyMsg( msg );
        } else {
            FreePPTMsg( (struct PPTMessage *)msg, ExtBase );
        }
        ++count;
    }

    return count;
}

/*
    This waits for a deathmessage sent by our master and replies to
    all other messages.  It will also release any replied message.

    BUG:  Redundant code with WaitForReply().
*/

Prototype VOID WaitDeathMessage( EXTBASE * );

VOID WaitDeathMessage( EXTBASE *ExtBase )
{
    struct MsgPort *mp = ExtBase->mport;
    struct ExecBase *SysBase = ExtBase->lb_Sys;
    ULONG sig;
    struct Message *msg;
    BOOL  quit = FALSE;

    D(bug("WaitDeathMessage()\n"));

    while(!quit) {
        sig = Wait( 1 << mp->mp_SigBit | SIGBREAKF_CTRL_C );

        if( sig & SIGBREAKF_CTRL_C ) return;

        while( msg = GetMsg( mp ) ) {
            if( msg->mn_Node.ln_Type == NT_REPLYMSG ) {
                if( ((struct PPTMessage *)msg)->code & PPTMSGF_DONE ) {
                    quit = TRUE;
                }
                FreePPTMsg( (struct PPTMessage *) msg, ExtBase );
            } else {
                ReplyMsg( msg );
            }
        }

    }
}

/*
    Waits for a reply to given message code.
*/

Prototype VOID WaitForReply( ULONG, EXTBASE * );

VOID WaitForReply( ULONG code, EXTBASE *ExtBase )
{
    struct MsgPort *mp = ExtBase->mport;
    struct ExecBase *SysBase = ExtBase->lb_Sys;
    ULONG sig;
    struct Message *msg;
    BOOL  quit = FALSE;

    D(bug("WaitForReply(%ld)\n",code));

    while(!quit) {
        sig = Wait( 1 << mp->mp_SigBit | SIGBREAKF_CTRL_C );

        if( sig & SIGBREAKF_CTRL_C ) return;

        D(bug("\tgot some sort of message\n"));

        while( msg = GetMsg( mp ) ) {
            D(bug("\tnew message\n"));
            if( msg->mn_Node.ln_Type == NT_REPLYMSG ) {
                D(bug("\t\tit is a reply message\n"));
                if( ((struct PPTMessage *)msg)->code == code ) {
                    D(bug("\t\tto our message, it seems\n"));
                    quit = TRUE;
                }
                D(bug("\t\tFreeing message\n"));
                FreePPTMsg( (struct PPTMessage *) msg, ExtBase );
            } else {
                D(bug("\t\ta strange message, replying\n"));
                ReplyMsg( msg );
            }
        }
    }
}

/*------------------------------------------------------------------------*/
/*                              END OF CODE                               */
/*------------------------------------------------------------------------*/

