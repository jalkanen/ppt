/*
    PROJECT: ppt
    MODULE : message.c

    $Id: message.c,v 1.2 1995/08/29 00:02:22 jj Exp $

    This module contains code about message handling routines.
*/

#include <defs.h>
#include <misc.h>

/*------------------------------------------------------------------------*/

Prototype struct PPTMessage *InitPPTMsg( void );
Prototype void PurgePPTMsg( struct PPTMessage * );
Prototype void DoPPTMsg( struct MsgPort *, struct PPTMessage * );

/*------------------------------------------------------------------------*/
/* Code */

/*
    Initialize a PPTMsg.
*/
struct PPTMessage *InitPPTMsg( void )
{
    struct Library *SysBase = SYSBASE();
    struct MsgPort *replyport;
    struct PPTMessage *msg;

    D(bug("InitPPTMsg()\n"));
    replyport = CreateMsgPort();
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

    D(bug("PurgePPTMsg(%08x)\n",msg));
    if(msg->msg.mn_ReplyPort)
        DeleteMsgPort(msg->msg.mn_ReplyPort);

    pfree(msg);
}

/*
    Send a simple message asyncronously. Ignore answer.
*/
void DoPPTMsg( struct MsgPort *dest, struct PPTMessage *msg )
{
    APTR SysBase = SYSBASE();

    Forbid();
    PutMsg(dest, (struct Message *)msg);
    Permit();

    D(bug("\tWaiting for answer...\n"));

    WaitPort(msg->msg.mn_ReplyPort); /* Wait for the answer... */
    GetMsg(msg->msg.mn_ReplyPort);

    D(bug("\tGot answer, now exiting\n"));
}
/*------------------------------------------------------------------------*/
/*                              END OF CODE                               */
/*------------------------------------------------------------------------*/

