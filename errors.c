/*
    PROJECT: ppt

    $Id: errors.c,v 1.3 1996/01/08 23:38:03 jj Exp $

    Error handling routines.
*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <defs.h>
#include <misc.h>

/*----------------------------------------------------------------------*/
/* Prototypes */

Prototype VOID        SetErrorCode( REG(A0) FRAME *frame, REG(D0) PERROR error );
Prototype VOID        SetErrorMsg( REG(A0) FRAME *frame, REG(A1) UBYTE *error );
Prototype VOID        ShowError( REG(A0) FRAME *frame, REG(A6) EXTBASE * );
Prototype UBYTE      *GetErrorMsg( FRAME *, EXTBASE * );
Prototype VOID        ClearError( FRAME * );
Prototype VOID        CopyError( FRAME *, FRAME * );
Prototype REG(D0) UBYTE *ErrorMsg( REG(D0) ULONG, REG(A6) EXTBASE * );

/*----------------------------------------------------------------------*/
/* Code */

/*
    This returns a pointer to an error message.
*/

REG(D0) UBYTE *ErrorMsg( REG(D0) ULONG code, REG(A6) EXTBASE *ExtBase )
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

UBYTE *GetErrorMsg( FRAME *frame, EXTBASE *ExtBase )
{
    UBYTE *msg = "";

    if(frame) {
        if( strlen( frame->errormsg ) != 0 )
            msg = frame->errormsg;
        else
            msg = ErrorMsg( frame->errorcode, ExtBase );
    }

    return msg;
}

/*
    Clear the current error message.
*/
VOID ClearError( FRAME *frame )
{
    if(frame) {
        frame->errormsg[0] = '\0';
        frame->errorcode   = PERR_OK;
        frame->doerror     = FALSE;
    }
}

/*
    A kludgish way to copy error messages from one frame to another one.
*/

VOID CopyError( FRAME *source, FRAME *dest )
{
    if( source != dest ) {
        dest->errorcode = source->errorcode;
        dest->doerror   = source->doerror;
        strncpy( dest->errormsg, source->errormsg, ERRBUFLEN-1 );
    }
}

/****** pptsupport/SetErrorCode ******************************************
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

SAVEDS ASM VOID SetErrorCode( REG(A0) FRAME *frame, REG(D0) PERROR error )
{
    D(bug("SetErrorCode(%08X, error=%lu)\n",frame,error));

    if(CheckPtr(frame,"SetErrorCode: frame")) {
        if( frame->doerror == FALSE ) {
            frame->errorcode = error;
            frame->doerror   = TRUE; /* Signal: This error has not yet been shown */
        } else {
            D(bug("\tAn error was already pending; didn't add this one\n"));
        }
    }
}

/****** pptsupport/SetErrorMsg ******************************************
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

SAVEDS ASM VOID SetErrorMsg( REG(A0) FRAME *frame, REG(A1) UBYTE *error )
{
    D(bug("SetErrorMsg(%08X, error='%s')\n",frame,error));

    if( CheckPtr(frame,"SetErrorMsg(): frame") && error) {
        if( frame->doerror == FALSE ) {
            frame->errorcode = PERR_FAILED;
            strncpy( frame->errormsg, error, ERRBUFLEN-1 );
            frame->doerror = TRUE; /* Signal: This error has not yet been shown */
        } else {
            D(bug("\tAn error was already pending; didn't add this one\n"));
        }
    }
}

/*
    BUG: Not yet complete
*/
SAVEDS ASM VOID ShowError( REG(A0) FRAME *frame, REG(A6) EXTBASE *ExtBase )
{
    D(bug("ShowError()\n"));

    if( frame ) {
        XReq(NEGNUL,NULL,ISEQ_C"%s\n", GetErrorMsg( frame, ExtBase) );
        ClearError( frame );
    }
}



/*----------------------------------------------------------------------*/
/*                             END OF CODE                              */
/*----------------------------------------------------------------------*/

