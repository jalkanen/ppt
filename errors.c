/*
    PROJECT: ppt

    $Id: errors.c,v 1.9 1999/10/02 16:33:07 jj Exp $

    Error handling routines.
*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"

/*----------------------------------------------------------------------*/
/* Prototypes */

Prototype VOID ASM   SetErrorCode( REGDECL(a0,FRAME *), REGDECL(d0,PERROR) );
Prototype VOID ASM   SetErrorMsg( REGDECL(a0,FRAME *), REGDECL(a1,UBYTE *) );
Prototype VOID ASM   ShowError( REGDECL(a0,FRAME *), REGDECL(a6,EXTBASE *) );
Prototype UBYTE      *GetErrorMsg( FRAME *, EXTBASE * );
Prototype VOID        ClearError( FRAME * );
Prototype VOID        CopyError( FRAME *, FRAME * );
Prototype UBYTE * ASM ErrorMsg( REGDECL(d0,ULONG), REGDECL(a6,EXTBASE *) );

/*----------------------------------------------------------------------*/
/* Code */

/*
    This returns a pointer to an error message.
*/

UBYTE *
ASM ErrorMsg( REGPARAM(d0,ULONG,code), REGPARAM(a6,struct PPTBase *,PPTBase) )
{
    switch(code) {
        case PERR_UNKNOWNTYPE:
            return XGetStr(MSG_PERR_UNKNOWNTYPE);
        case PERR_INUSE:
            return XGetStr(MSG_PERR_INUSE);
        case PERR_INITFAILED:
            return XGetStr(MSG_PERR_INITFAILED);
        case PERR_CANCELED:
            return XGetStr(MSG_PERR_CANCELED);
        case PERR_OUTOFMEMORY:
            return XGetStr(MSG_PERR_OUTOFMEMORY);
        case PERR_BREAK:
            return XGetStr(MSG_PERR_BREAK);
        case PERR_WONTOPEN:
            return XGetStr(MSG_PERR_WONTOPEN);
        case PERR_FAILED:
            return XGetStr(MSG_PERR_FAILED);
        case PERR_MISSINGCODE:
            return XGetStr(MSG_PERR_MISSINGCODE);
        case PERR_INVALIDARGS:
            return XGetStr(MSG_PERR_INVALIDARGS);
        case PERR_WINDOWOPEN:
            return XGetStr(MSG_PERR_WINDOWOPEN);
        case PERR_FILEOPEN:
            return XGetStr(MSG_PERR_FILEOPEN);
        case PERR_FILEREAD:
            return XGetStr(MSG_PERR_FILEREAD);
        case PERR_FILEWRITE:
            return XGetStr(MSG_PERR_FILEWRITE);
        case PERR_FILECLOSE:
            return XGetStr(MSG_PERR_FILECLOSE);
        case PERR_UNKNOWNCOLORSPACE:
            return XGetStr(MSG_PERR_UNKNOWNCSPACE);
        case PERR_OK:
            return XGetStr(MSG_PERR_OK);
        default:
            return "Unknown Error Code";
    }
}


/*
    Return a message string describing what went wrong. It knows
    the difference between error codes and strings.
*/

UBYTE *GetErrorMsg( FRAME *frame, EXTBASE *PPTBase )
{
    UBYTE *msg = "";

    if(frame) {
        SHLOCK(frame);

        if( strlen( frame->errormsg ) != 0 )
            msg = frame->errormsg;
        else
            msg = ErrorMsg( frame->errorcode, PPTBase );

        UNLOCK(frame);
    }

    return msg;
}

/*
    Clear the current error message.
*/
VOID ClearError( FRAME *frame )
{
    if(frame) {
        LOCK(frame);

        frame->errormsg[0] = '\0';
        frame->errorcode   = PERR_OK;
        frame->doerror     = FALSE;

        UNLOCK(frame);
    }
}

/*
    A kludgish way to copy error messages from one frame to another one.
*/

VOID CopyError( FRAME *source, FRAME *dest )
{
    if( source != dest ) {
        LOCK(dest);
        SHLOCK(source);
        dest->errorcode = source->errorcode;
        dest->doerror   = source->doerror;
        strncpy( dest->errormsg, source->errormsg, ERRBUFLEN-1 );
        UNLOCK(source);
        UNLOCK(dest);
    }
}

/****u* pptsupport/SetErrorCode ******************************************
*
*   NAME
*       SetErrorCode -- Set the error code for the frame.
*
*   SYNOPSIS
*       SetErrorCode( frame, error )
*                     A0     D0
*
*       VOID SetErrorCode( FRAME *, PERROR );
*
*   FUNCTION
*       Set the error code for the frame. Upon returning from the
*       external, PPT will display the proper error message.
*
*   INPUTS
*       frame - obvious
*       error - a PPT specific error code. For the definition
*           of the different PERR_* error codes, please see ppt.h.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       You cannot set the error code more than once.
*
*   BUGS
*
*   SEE ALSO
*       SetErrorMsg(), ppt.h
*
******************************************************************************
*
*/

VOID
SAVEDS ASM SetErrorCode( REGPARAM(a0,FRAME *,frame),
                         REGPARAM(d0,PERROR,error) )
{
    D(bug("SetErrorCode(%08X, error=%lu)\n",frame,error));

    if(CheckPtr(frame,"SetErrorCode: frame")) {
        LOCK(frame);
        if( frame->doerror == FALSE ) {
            frame->errorcode = error;
            frame->doerror   = TRUE; /* Signal: This error has not yet been shown */
        } else {
            D(bug("\tAn error was already pending; didn't add this one\n"));
        }
        UNLOCK(frame);
    }
}

/****u* pptsupport/SetErrorMsg ******************************************
*
*   NAME
*       SetErrorMsg -- Set an error message string
*
*   SYNOPSIS
*       SetErrorMsg( frame, msg )
*                    A0     A1
*
*       VOID SetErrorMsg( FRAME *, UBYTE * );
*
*   FUNCTION
*       Set up a custom error message for the frame.
*
*   INPUTS
*       frame - your frame handle
*       msg - pointer to a NUL-terminated string, which contains
*           the error message. It is copied to an internal buffer,
*           so you needn't guarantee it's validity after exiting
*           your external module.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       You cannot set the error code more than once.
*
*   BUGS
*
*   SEE ALSO
*       SetErrorCode()
*
******************************************************************************
*
*/

VOID
SAVEDS ASM SetErrorMsg( REGPARAM(a0,FRAME *,frame),
                        REGPARAM(a1,UBYTE *,error) )
{
    D(bug("SetErrorMsg(%08X, error='%s')\n",frame,error));

    if( CheckPtr(frame,"SetErrorMsg(): frame") && error) {
        LOCK(frame);
        if( frame->doerror == FALSE ) {
            frame->errorcode = PERR_FAILED;
            strncpy( frame->errormsg, error, ERRBUFLEN-1 );
            frame->doerror = TRUE; /* Signal: This error has not yet been shown */
        } else {
            D(bug("\tAn error was already pending; didn't add this one\n"));
        }
        UNLOCK(frame);
    }
}

/*
    BUG: Not yet complete
*/
VOID
SAVEDS ASM ShowError( REGPARAM(a0,FRAME *,frame),
                      REGPARAM(a6,EXTBASE *,PPTBase) )
{
    D(bug("ShowError()\n"));

    if( frame ) {
        XReq(NEGNUL,NULL,
            ISEQ_C ISEQ_HIGHLIGHT"\nERROR:\n\n"
            ISEQ_TEXT"%s\n", GetErrorMsg( frame, PPTBase) );
        ClearError( frame );
    }
}



/*----------------------------------------------------------------------*/
/*                             END OF CODE                              */
/*----------------------------------------------------------------------*/

