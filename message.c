/*
    PROJECT: ppt
    MODULE : message.c

    $Id: message.c,v 6.1 1999/10/02 16:33:07 jj Exp $

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


Prototype VOID              SendInputMsg(FRAME *, struct PPTMessage *);

Prototype LONG              EmptyMsgPort( struct MsgPort *, EXTBASE * );

/*------------------------------------------------------------------------*/
/* Code */

/*
    This is a more complex PPTMessage system.  Allows for more configurability
*/

Prototype struct PPTMessage *AllocPPTMsg( ULONG, EXTBASE * );

struct PPTMessage *AllocPPTMsg( ULONG size, EXTBASE *PPTBase )
{
    struct PPTMessage *msg = NULL;

    if(msg = smalloc( size )) {
        bzero( msg, size );
        msg->msg.mn_Node.ln_Type = NT_MESSAGE;
        msg->msg.mn_Length       = size;
        msg->msg.mn_ReplyPort    = PPTBase->mport;
    }

    return msg;
}

Prototype VOID FreePPTMsg( struct PPTMessage *pmsg, EXTBASE *PPTBase );

VOID FreePPTMsg( struct PPTMessage *pmsg, EXTBASE *PPTBase )
{
    if( pmsg ) {
        DB(bug("FreePPTMsg(%08X) : code %lX\n", pmsg, pmsg->code ));
        sfree( pmsg );
    }
}

Prototype VOID SendPPTMsg( struct MsgPort *, struct PPTMessage *, EXTBASE * );

VOID SendPPTMsg( struct MsgPort *mp, struct PPTMessage *pmsg, EXTBASE *PPTBase )
{
    struct ExecBase *SysBase = PPTBase->lb_Sys;

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


/*--------------------------------------------------------------------*/

/*
    Send one input message to the external process.
*/

VOID SendInputMsg( FRAME *f, struct PPTMessage *pmsg )
{
    D(bug("\tSending frame input\n"));

    pmsg->frame = f;

    SendPPTMsg( f->selection.selectport, pmsg, globxd );
}


/*
    This empties a message port from all messages. The message count
    is returned.
*/
LONG EmptyMsgPort( struct MsgPort *mp, EXTBASE *PPTBase )
{
    struct Message *msg;
    LONG count = 0;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    D(bug("EmptyMsgPort()\n"));

    while( msg = GetMsg( mp ) ) {
        if( msg->mn_Node.ln_Type != NT_REPLYMSG ) {
            ReplyMsg( msg );
        } else {
            FreePPTMsg( (struct PPTMessage *)msg, PPTBase );
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

VOID WaitDeathMessage( EXTBASE *PPTBase )
{
    struct MsgPort *mp = PPTBase->mport;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
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
                FreePPTMsg( (struct PPTMessage *) msg, PPTBase );
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

VOID WaitForReply( ULONG code, EXTBASE *PPTBase )
{
    struct MsgPort *mp = PPTBase->mport;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
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
                FreePPTMsg( (struct PPTMessage *) msg, PPTBase );
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

