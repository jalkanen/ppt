/*
    PROJECT: ppt

    $Id: errors.c,v 1.1 1995/11/24 23:19:12 jj Exp $

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
Prototype VOID        ShowError( REG(A0) FRAME *frame );
Prototype UBYTE      *GetErrorMsg( FRAME * );
Prototype VOID        ClearError( FRAME * );


/*----------------------------------------------------------------------*/
/* Code */

/*
    This returns a pointer to an error message.
*/

__D0 UBYTE *ErrorMsg( __D0 ULONG code )
{
    switch(code) {
        case PERR_UNKNOWNTYPE:
            return "Unknown type!";
        case PERR_INUSE:
            return "Object is in use.";
        case PERR_INITFAILED:
            return "Initialization phase failed.";
        case PERR_CANCELED:
            return "Cancelled operation";
        case PERR_OUTOFMEMORY:
            return "Out of memory!";
        case PERR_BREAK:
            return "User break.";
        case PERR_WONTOPEN:
            return "Cannot open resource!";
        case PERR_FAILED:
            return "A generic error occurred";
        case PERR_MISSINGCODE:
            return "No code?!";
        case PERR_INVALIDARGS:
            return "Invalid arguments!";
        case PERR_WINDOWOPEN:
            return "Couldn't open window";
        case PERR_FILEOPEN:
            return "Couldn't open file";
        case PERR_FILEREAD:
            return "File read error";
        case PERR_FILEWRITE:
            return "Error while writing file";
        case PERR_FILECLOSE:
            return "Error while closing file";
        case PERR_OK:
            return "Soft Error: Everything was really OK?!";
        default:
            return "Unknown Error Code";
    }
}


/*
    Return a message string describing what went wrong. It knows
    the difference between error codes and strings.
*/

UBYTE *GetErrorMsg( FRAME *frame )
{
    UBYTE *msg = "";

    if(frame) {
        if( strlen( frame->errormsg ) != 0 )
            msg = frame->errormsg;
        else
            msg = ErrorMsg( frame->errorcode );
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
*
*   BUGS
*       Behaviour with SetErrorMsg() is not properly defined.
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
        if(!CheckPtr(frame, "SetErrorCode(): frame")) return;
        frame->errorcode = error;
        frame->doerror   = TRUE; /* Signal: This error has not yet been shown */
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
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       This entry still very incomplete.
*       The behaviour with SetErrorCode() is not properly defined
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
        if(!CheckPtr(frame, "SetErrorMsg(): frame")) return;
        frame->errorcode = PERR_FAILED;
        strncpy( frame->errormsg, error, ERRBUFLEN-1 );
        frame->doerror = TRUE; /* Signal: This error has not yet been shown */
    }
}

/*
    BUG: Not yet complete
*/
SAVEDS ASM VOID ShowError( REG(A0) FRAME *frame )
{
    D(bug("ShowError()\n"));

    if( frame ) {
        XReq(NEGNUL,NULL,ISEQ_C"%s\n", GetErrorMsg( frame ) );
        ClearError( frame );
    }
}



/*----------------------------------------------------------------------*/
/*                             END OF CODE                              */
/*----------------------------------------------------------------------*/

