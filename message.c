/*
    PROJECT: ppt
    MODULE : message.c

    $Id: message.c,v 1.6 1996/10/25 01:39:11 jj Exp $

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

Prototype VOID              SendInputMsg(FRAME *, ULONG,  APTR);

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

/****** pptsupport/StartInput ******************************************
*
*   NAME
*       StartInput - Start up notification
*
*   SYNOPSIS
*       error = StartInput( frame, method, area )
*       D0                  A0     D0      A1
*
*       PERROR StartInput( FRAME *, ULONG, APTR );
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
*       GINP_PICK_POINT -
*
*       GINP_FIXED_RECT -
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
*       processing on the frame.
*
*   BUGS
*       Is the area really needed?
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
                   REG(a1) APTR  area,
                   REG(d0) ULONG mid,
                   REG(a6) EXTBASE *ExtBase )
{
    struct PPTMessage pmsg, *amsg;

    D(bug("GetInput(f=%08X,a=%08X,m=%ld)\n",frame,area,mid));

    if(!frame) return FALSE; /* Sanity check */

    /*
     *  Initialize the message part
     */

    bzero( &pmsg, sizeof(struct PPTMessage) );
    pmsg.msg.mn_Node.ln_Type = NT_MESSAGE;
    pmsg.msg.mn_Length       = sizeof(struct PPTMessage);
    pmsg.msg.mn_ReplyPort    = ExtBase->mport;

    /*
     *  Send the address of the parent frame, if such a beast
     *  exists, since the parent is what we know this frame by.
     */

    if( frame->parent ) {
        D(bug("\tFrame has a parent @ %08X\n",frame->parent));
        pmsg.frame = frame->parent;
    } else {
        pmsg.frame = frame;
    }

    pmsg.code  = mid + PPTMSG_LASSO_RECT;
    pmsg.data  = area;

    /*
     *  Send the message to main program and return, if it
     *  was succesfull.
     */

    D(bug("\tSending message...\n"));

    Forbid();
    PutMsg( globxd->mport, (struct Message *) &pmsg );
    Permit();

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
             *  be actual user data.  We need to answer to this message.
             */

            if( amsg = (struct PPTMessage *)GetMsg(ExtBase->mport) ) {

                if( amsg->code == PPTMSG_ACK_INPUT ) {
                    if( amsg->data == PERR_OK ) {
                        D(bug("\tMain task says its OK!\n"));
                    } else {
                        D(bug("\tFailed to set up the comm system\n"));
                    }
                    // ReplyMsg( (struct Message *)amsg );
                    return (PERROR) (amsg->data);
                }

                /*
                 *  BUG: Anything else should definitely be an error...
                 */

                ReplyMsg( (struct Message *)amsg );
            }
        }
    }

    return PERR_OK;
}

/****** pptsupport/StopInput ******************************************
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

    pmsg = InitPPTMsg();

    pmsg->code  = PPTMSG_STOP_INPUT;

    if( frame->parent )
        pmsg->frame = frame->parent;
    else
        pmsg->frame = frame;

    DoPPTMsg( globxd->mport, pmsg );

    PurgePPTMsg( pmsg );
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

    RemoveSelectBox( f );

    f->selectmethod = pmsg->code - PPTMSG_LASSO_RECT;
    f->selectdata   = pmsg->data;
    f->selectport   = pmsg->msg.mn_ReplyPort;

    ChangeBusyStatus( f, BUSY_READONLY );

    /*
     *  Modify the initial message that main() will ReplyMsg() to.
     */

    pmsg->code  = PPTMSG_ACK_INPUT;
    pmsg->data  = PERR_OK; /* Signal: successful. */
}


/*
    BUG: Should really do queue up the mouse messages.
*/

VOID SendInputMsg( FRAME *f, ULONG code, APTR msg )
{
    struct PPTMessage *pmsg;

    D(bug("Sending frame input\n"));

    pmsg = InitPPTMsg();

    pmsg->data  = msg;
    pmsg->code  = code;
    pmsg->frame = f;

    DoPPTMsg(f->selectport, pmsg);

    PurgePPTMsg(pmsg);
}


/*
    Clear away any signs of us ever having had a other method than
    LASSO_RECT.
*/
VOID ClearFrameInput( FRAME *f )
{
    struct Library *SysBase = SYSBASE();

    D(bug("ClearFrameInput(%08X)\n",f));

    LOCKGLOB();
    f->selectmethod = GINP_LASSO_RECT;
    f->selectdata   = NULL;
    f->selectport   = NULL;
    UNLOCKGLOB();
}

/*
    Sends a simple message to the external, telling we have finished.

    BUG: No error checking.
    BUG: The busy code should be saved.
    BUG: Obsolete.
*/

VOID FinishFrameInput( FRAME *f )
{
    D(bug("Finished frame input\n"));

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

        while( msg = GetMsg( mp ) ) {
            if( msg->mn_Node.ln_Type == NT_REPLYMSG ) {
                if( ((struct PPTMessage *)msg)->code == code ) {
                    quit = TRUE;
                }
                FreePPTMsg( (struct PPTMessage *) msg, ExtBase );
            } else {
                ReplyMsg( msg );
            }
        }
    }
}

/*------------------------------------------------------------------------*/
/*                              END OF CODE                               */
/*------------------------------------------------------------------------*/

