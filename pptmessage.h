/*
    PROJECT: ppt
    MODULE:  message.h

    $Revision: 1.2 $
        $Date: 1997/05/27 22:20:56 $
      $Author: jj $

    This file contains declarations for the message passing
    system.
*/

#ifndef PPTMESSAGE_H
#define PPTMESSAGE_H

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif

#ifndef PPT_H
#include <ppt.h>
#endif

/*------------------------------------------------------------------*/
/* Interprocess communication */

/*
 *  The base message type.  Theoretically this is private, but if
 *  you read it, do not expect anything
 */
struct PPTMessage {
    struct Message  msg;
    FRAME           *frame;         /* Who does this message concern? */
    ULONG           code;           /* Unique code.  See below. */
    APTR            data;           /* code-dependent.  Not recommended reading for small children. */
};


/*
 *   Message codes for the PPTMessage.code field.
 */

/*!!PRIVATE*/

#define PPTMSGF_DONE            0x80000000 /* Set, if the message contains a death msg */

#define PPTMSG_EFFECTDONE       (PPTMSGF_DONE + 0x01)
#define PPTMSG_LOADDONE         (PPTMSGF_DONE + 0x02)
#define PPTMSG_RENDERDONE       (PPTMSGF_DONE + 0x03)
#define PPTMSG_SAVEDONE         (PPTMSGF_DONE + 0x04)

/*!!PUBLIC*/

#define PPTMSG_LASSO_RECT       0x10L
#define PPTMSG_PICK_POINT       0x11L
#define PPTMSG_FIXED_RECT       0x12L

/*!!PRIVATE*/

#define PPTMSG_STOP_INPUT       0x100L
#define PPTMSG_ACK_INPUT        0x101L

#define PPTMSG_OPENINFOWINDOW   0x200L
#define PPTMSG_CLOSEINFOWINDOW  0x201L
#define PPTMSG_UPDATEINFOWINDOW 0x202L
#define PPTMSG_UPDATEPROGRESS   0x203L        /* Is really a ProgressMsg (see below) */


/*
 *  This message contains enough info to update the progress
 *  display.
 */
struct ProgressMsg {
    struct PPTMessage pmsg;
    ULONG  done;
    UBYTE  text[256];
};

/*!!PUBLIC */

/*------------------------------------------------------------------*/
/* The input handler system */

/* Codes for StartInput() */

#define GINP_LASSO_RECT      0
#define GINP_PICK_POINT      1
#define GINP_FIXED_RECT      2

/*
 *  These messages are sent to you after you have called
 *  StartInput().
 */


/* PPTMSG_PICK_POINT */

struct gPointMessage {
    struct PPTMessage   msg;
    WORD                x, y;
};

/* PPTMSG_LASSO_RECT */

struct gRectMessage {
    struct PPTMessage   msg;
    struct IBox         dim;
};

/* PPTMSG_FIXED_RECT */

struct gFixRectMessage {
    struct PPTMessage   msg;
    WORD                x, y;   /* Topleft coords */
    struct IBox         dim;    /* Dimensions of the fixed box */
};

#endif /* PPT_MESSAGE_H */

