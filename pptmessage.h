/*
    PROJECT: ppt
    MODULE:  message.h

    $Revision: 6.0 $
        $Date: 1999/11/28 18:16:34 $
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

/*
 *  Effect done.  The em_NewFrame field points to the new frame which
 *  was created during the effect (it may be NULL), and em_Status
 *  tells what actually happened.  See below for codes.
 */

#define PPTMSG_EFFECTDONE       (PPTMSGF_DONE + 0x01)

struct EffectMessage {
    struct PPTMessage   em_PMsg;
    FRAME               *em_NewFrame;
    ULONG               em_Status;
};

#define EMSTATUS_FAILED         0L  /* Effect failed.  em_NewFrame = NULL */
#define EMSTATUS_NEWFRAME       1L  /* Effect succeeded. em_NewFrame = new frame */
#define EMSTATUS_NOCHANGE       2L  /* Effect succeeded, but there was no change,
                                       thus em_NewFrame = NULL */

#define PPTMSG_LOADDONE         (PPTMSGF_DONE + 0x02)
#define PPTMSG_RENDERDONE       (PPTMSGF_DONE + 0x03)
#define PPTMSG_SAVEDONE         (PPTMSGF_DONE + 0x04)
#define PPTMSG_GETARGSDONE      (PPTMSGF_DONE + 0x05)

struct GetArgsMessage {
    struct PPTMessage   gam_PMsg;
    PERROR              gam_Error;          /* Standard error code */
    UBYTE               gam_Result[1023];   /* Contains result from the external module */
};

/*!!PUBLIC*/

#define PPTMSG_LASSO_RECT       0x10L
#define PPTMSG_PICK_POINT       0x11L
#define PPTMSG_FIXED_RECT       0x12L
#define PPTMSG_LASSO_CIRCLE     0x13L

/*!!PRIVATE*/

#define PPTMSG_LASSO_FREE       0x14L

#define PPTMSG_STOP_INPUT       0x100L
#define PPTMSG_START_INPUT      0x101L

#define PPTMSG_OPENINFOWINDOW   0x200L
#define PPTMSG_CLOSEINFOWINDOW  0x201L
#define PPTMSG_UPDATEINFOWINDOW 0x202L
#define PPTMSG_UPDATEPROGRESS   0x203L        /* Is really a ProgressMsg (see below) */

#define PPTMSG_START_PREVIEW    0x300L
#define PPTMSG_STOP_PREVIEW     0x301L

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
#define GINP_LASSO_CIRCLE    3
/*!!PRIVATE*/
#define GINP_LASSO_FREE      4
/*!!PUBLIC*/

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

