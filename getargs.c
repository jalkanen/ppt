/*
    PROJECT: ppt
    MODULE:  getargs.c

    $Id: getargs.c,v 1.1 2000/05/21 18:41:48 jj Exp $
 */

#include "defs.h"
#include "misc.h"

#ifndef DOS_DOSTAGS_H
#include <dos/dostags.h>
#endif

#include <proto/module.h>
#include <proto/iomod.h>
#include <proto/effect.h>

#define ARGBUFFER_SIZE 4096

/*--------------------------------------------------------------------*/
/* Local prototypes */

Local SAVEDS ASM VOID GetArgs( REGPARAM(a0,UBYTE *,argstr), REGPARAM(d0,ULONG,len) );

/*--------------------------------------------------------------------*/

/*
    Note: assumes frame is valid, and so is external.
    BUG: does not do argument checking.
 */

Prototype PERROR RunGetArgs( FRAME *frame, EXTERNAL *ext, UBYTE *argstr );

PERROR RunGetArgs( FRAME *frame, EXTERNAL *ext, UBYTE *argstr )
{
    struct Process *p;
    char argbuf[ARGBUF_SIZE];
    PERROR res = PERR_OK;

    if( ObtainFrame( frame, BUSY_READONLY ) ) {

        if( argstr )
            sprintf( argbuf, "P_FRAME %lu P_MODULE %lu P_ARGS %s", frame, ext, argstr );
        else
            sprintf( argbuf, "P_FRAME %lu P_MODULE %lu", frame, ext );

#ifdef DEBUG_MODE
        p = CreateNewProcTags( NP_Entry, GetArgs, NP_Cli, FALSE,
            NP_Output, frame->debug_handle = OpenDebugFile( DFT_GetArgs ),
            NP_CloseOutput,TRUE, NP_Name, frame->nd.ln_Name,
            NP_StackSize, globals->userprefs->extstacksize,
            NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf, TAG_END );
#else
        p = CreateNewProcTags( NP_Entry, GetArgs, NP_Cli, FALSE,
            NP_Output, Open("NIL:",MODE_NEWFILE),
            NP_CloseOutput, TRUE, NP_Name, frame->nd.ln_Name,
            NP_StackSize, globals->userprefs->extstacksize,
            NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf, TAG_END );

#endif

        if(p) {
            SetFrameStatus( frame, 1 );
            LOCK(frame);
            frame->currproc = p;
            UNLOCK(frame);
        } else {
            ReleaseFrame( frame );
            D(bug("\tCouldn't create new process\n"));
            res = PERR_ERROR;
        }
    }

    return res;
}

Local
SAVEDS ASM VOID GetArgs( REGPARAM(a0,UBYTE *,argstr),
                         REGPARAM(d0,ULONG,len) )
{
    ULONG *optarray = NULL;
    EXTBASE *PPTBase = NULL;
    struct Library *BGUIBase;
    struct IntuitionBase *IntuitionBase;
    struct Library *SysBase = SYSBASE();
    FRAME *frame = NULL;
    struct GetArgsMessage *gamsg = NULL;
    UBYTE *args = NULL;
    STRPTR argbuffer = "";
    EXTERNAL *module = NULL;
    PERROR res = PERR_OK;

    D(bug("GetArgs()\n"));

    if( (PPTBase = NewExtBase(TRUE)) == NULL) {
        D(bug("LIB BASE ALLOCATION FAILED!\n"));
        res = PERR_INITFAILED;
        goto errorexit;
    }

    /*
     *  Set up variables and check for REXX command
     */

    if( optarray = ParseDOSArgs( argstr, "P_FRAME/A/N,P_MODULE/A/N/K,P_ARGS/K/N", PPTBase ) ) {

        frame = (FRAME *) *( (ULONG *)optarray[0]) ;

        if(optarray[1]) { /* A name was given */
            module = (EXTERNAL *) PEEKL(optarray[1]);


            if( optarray[2] ) {
                args = (UBYTE *)PEEKL(optarray[2]);
            }

        }
    } else {
        InternalError( XGetStr(mFILTER_INCORRECT_ARGS) );
        goto errorexit;
    }

    D(bug("\tFRAME = %08X PLUGIN = '%s' ARGS = >%s<\n",
           frame, module->realname ,(args) ? (char *)args : "NULL"));

    if(NewTaskProlog(frame,PPTBase) != PERR_OK) goto errorexit;

    BGUIBase = PPTBase->lb_BGUI;
    IntuitionBase = PPTBase->lb_Intuition;

    if( argbuffer = smalloc( ARGBUFFER_SIZE ) ) {
        if( args ) strncpy( argbuffer, args, ARGBUFFER_SIZE-1 );

        res = GetModuleArgs( frame, module, argbuffer, PPTBase );
    } else {
        D(bug("\tfailed allocing argbuffer\n"));
        res = PERR_OUTOFMEMORY;
    }

    D(bug("\tFilter() done, sending message\n"));

errorexit:

    if( res != PERR_OK ) {
        D(bug("\tGetArgs() failed\n"));

        if( frame->doerror &&
            (frame->errorcode != PERR_BREAK) &&
            (frame->errorcode != PERR_CANCELED) ) {
                D(bug("\tReporting error\n"));
                XReq( NEGNUL, NULL, XGetStr(mEFFECT_REPORTS_ERROR),
                      module->nd.ln_Name,
                      frame->nd.ln_Name,
                      GetErrorMsg( frame, PPTBase ) );
            }

        frame->doerror = FALSE;
    }

    if(optarray)
        FreeDOSArgs( optarray, PPTBase );

    /*
     *  Prepare to send message to main program
     */

    gamsg = (struct GetArgsMessage *)AllocPPTMsg( sizeof(struct GetArgsMessage), PPTBase );
    gamsg->gam_PMsg.frame = frame;
    gamsg->gam_PMsg.code  = PPTMSG_GETARGSDONE;
    gamsg->gam_Error = res;

    if( argbuffer ) {
        strcpy( gamsg->gam_Result, argbuffer );
        sfree( argbuffer );
    }

    /* Send the message */
    SendPPTMsg( globals->mport, gamsg, PPTBase );

    WaitDeathMessage( PPTBase );

    EmptyMsgPort( PPTBase->mport, PPTBase );

    if(PPTBase) RelExtBase(PPTBase);

    /* Die. */

}

Prototype PERROR GetModuleArgs( FRAME *, EXTERNAL *, STRPTR, struct PPTBase *);

PERROR GetModuleArgs( FRAME *frame, EXTERNAL *external, STRPTR original, struct PPTBase *PPTBase )
{
    PERROR res = PERR_OK;
    struct TagItem tags[] = {
        PPTX_ArgBuffer, NULL,
        PPTX_RexxArgs,  NULL,
        TAG_DONE, 0L
    };
    ULONG *argitemarray = NULL;
    STRPTR template;
    struct Library *ModuleBase;

    D(bug("GetModuleArgs(frame=%08X,external=>%s<,buffer=>%s<\n",
           frame,external->realname,original ));

    tags[0].ti_Data = (ULONG)original;

    if( ModuleBase = OpenModule( external, 0L, PPTBase ) ) {
        BOOL getargs;

        getargs = (BOOL) Inquire( PPTX_SupportsGetArgs, PPTBase );

        if( getargs ) {
            template = (STRPTR) Inquire( PPTX_RexxTemplate, PPTBase );

            /*
             *  Complain about the arguments only if the original
             *  string does exist.  An empty string equals no
             *  arguments (this is to ease some parts of data processing).
             */

            if( template && original && (strlen(original) > 0) ) {
                if( NULL == (argitemarray = ParseDOSArgs( original, template, PPTBase )) ) {
                    char buf[256];

                    SetErrorCode( frame, PERR_INVALIDARGS );
                    sprintf(buf,XGetStr(mINVALID_ARGS_EXPECTED_X), template);
                    SetErrorMsg( frame, buf );
                    if( ModuleBase ) CloseModule(ModuleBase, PPTBase);
                    return PERR_INVALIDARGS;
                }
                tags[1].ti_Data = (ULONG) argitemarray;
            }

            if( external->nd.ln_Type == NT_EFFECT ) {
                struct Library *EffectBase = ModuleBase;

                res = EffectGetArgs( frame, tags, PPTBase );
            } else if( external->nd.ln_Type == NT_LOADER ) {
                struct Library *IOModuleBase = ModuleBase;
                ULONG type;

                /*
                 *  BUG: This isn't actually OK.
                 */

                type = 0L;

                res = IOGetArgs( type, frame, tags, PPTBase );
            }

        } else {
            D(bug("\tEffect does not support getargs\n"));
            SetErrorCode( frame, PERR_MISSINGCODE );
            return PERR_MISSINGCODE;
        }
        CloseModule( ModuleBase, PPTBase );

    } else {
        D(bug("Failed to open module\n"));
        SetErrorCode( frame, PERR_MISSINGCODE );
        return PERR_ERROR;
    }

    D(bug("\tdone, res=%lu\n",res));

    return res;
}

