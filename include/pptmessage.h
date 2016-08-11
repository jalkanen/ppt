/*
    PROJECT: ppt
    MODULE:  message.h

    $Revision: 1.7 $
        $Date: 1999/01/13 22:54:38 $
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


#define PPTMSG_LASSO_RECT       0x10L
#define PPTMSG_PICK_POINT       0x11L
#define PPTMSG_FIXED_RECT       0x12L
#define PPTMSG_LASSO_CIRCLE     0x13L


/*------------------------------------------------------------------*/
/* The input handler system */

/* Codes for StartInput() */

#define GINP_LASSO_RECT      0
#define GINP_PICK_POINT      1
#define GINP_FIXED_RECT      2
#define GINP_LASSO_CIRCLE    3

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

/* PPTMSG_LASSO_CIRCLE */

struct gCircleMessage {
    struct PPTMessage   msg;
    WORD                x,y;    /* Coordinates of the center of the sphere */
    WORD                radius; /* Radius of the sphere */
};

#endif /* PPT_MESSAGE_H */

