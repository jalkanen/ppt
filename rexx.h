/*
    PROJECT: PPT
    MODULE : rexx.h

    $Id: rexx.h,v 6.0 1999/11/28 18:17:32 jj Exp $

    Definitions for AREXX port.
*/


#ifndef REXX_H
#define REXX_H

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef PPT_H
#include <ppt_real.h>
#endif

#include <rexx/storage.h>
#include <rexx/rxslib.h>

#define REXX_EXTENSION "prx"

/*
    For STEM handling
*/

struct StemItem {
    struct MinNode nd;
    UBYTE *name;        /* This is allocated dynamically for each value. */
    UBYTE *value;       /* Ditto. */
};

struct Stem {
    struct MinList list;
    UBYTE name[80];     /* Name for this stem. NO dot follows. */
};


/*
    RexxHost structure. Stolen from ARexxBox.
*/

struct RexxHost
{
    struct MsgPort *port;
    UBYTE           portname[ 80 ];
    LONG            replies;
    struct RDArgs * rdargs;
    LONG            flags;
    APTR            userdata;
};



/*
    The do-it-all rexx common structure.
*/

typedef struct {
    struct Node nd;
    ULONG *args; /* Array from ReadArgs() */
    LONG   rc;
    LONG   rc2;
    UBYTE *result;
    struct Process *proc; /* if this is != NULL, then the command is not immediate. */
    FRAME  *frame;
    struct RexxMsg *msg;
    struct Stem *stem; /* NULL, if it was not needed. */
    APTR   process_args; /* Holds the argument string for the process command */
    BOOL   free_result; /* TRUE, if the result field was smalloc()ed and must
                           be de-allocated on exit. */
} PPTREXXARGS;


/*
    Internal structure for keeping list of available rexx commands.
*/

struct RexxCommand {
    const char *name;
    const char *args;
    void       (*func)( PPTREXXARGS *ra, struct RexxMsg *rm );
};


/*
    For some weird reason, SAS/C seems to complain about RexxSysBase
    being of strange type...
*/
#pragma msg 72 ignore

extern struct List RexxWaitList;
extern struct RexxHost *rxhost;
extern struct RxsLib *RexxSysBase;

#pragma msg 72 warn

#endif /* REXX_H */
