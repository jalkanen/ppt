/*
    PROJECT: PPT
    MODULE : rexx.h

    $Id: rexx.h,v 1.3 1996/09/30 02:43:50 jj Exp $

    Definitions for AREXX port.
*/


#ifndef REXX_H
#define REXX_H

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
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
} REXXARGS;


/*
    Internal structure for keeping list of available rexx commands.
*/

struct RexxCommand {
    const char *name;
    const char *args;
    void       (*func)( REXXARGS *ra, struct RexxMsg *rm );
};




extern struct List RexxWaitList;
extern struct RexxHost *rxhost;
extern struct Library *RexxSysBase;

#endif /* REXX_H */
