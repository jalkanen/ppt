/*----------------------------------------------------------------------*/
/*
    PROJECT: PPT
    MODULE : EFFECT.c

    $Id: filter.c,v 1.25 1998/06/29 22:31:28 jj Exp $

    Code containing effects stuff.

*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"
#include "gui.h"
#include "rexx.h"

#include "version.h"

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef DOS_DOSTAGS_H
#include <dos/dostags.h>
#endif

#include <clib/bgui_protos.h>
#include <clib/alib_protos.h>

#include <pragmas/bgui_pragmas.h>

#ifndef PROTO_INTUITION_H
#include <proto/intuition.h>
#endif

#ifndef PROTO_UTILITY_H
#include <proto/utility.h>
#endif

#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif

#include "proto/effect.h"

#include <stdlib.h>


/*----------------------------------------------------------------------*/
/* Defines */


/*----------------------------------------------------------------------*/
/* Global variables */


/*----------------------------------------------------------------------*/
/* Internal prototypes */

Local FRAME *ExecFilter(EXTBASE *, FRAME *, EFFECT *, char *args, BOOL );
Prototype ASM VOID      Filter( REG(a0) UBYTE *, REG(d0) ULONG );
Prototype PERROR    ExecEasyFilter( FRAME *, FPTR, EXTBASE * );

/*----------------------------------------------------------------------*/
/* Code */

/*
    This routine gives a bit more control than RunFilter().  It starts
    up a given effect with the given args, adding the command to
    the rexx list, so that the argument string will be freed
    upon completion of the job.
 */

Prototype PERROR RunFilterCommand( FRAME *, STRPTR, STRPTR );

PERROR
RunFilterCommand( FRAME *frame, STRPTR filtername, STRPTR args )
{
    REXXARGS *ra;
    char buffer[100];
    PERROR res;

    if( ra = SimulateRexxCommand( frame, args ) ) {
        sprintf( buffer, "NAME=%s ARGS=%ld", filtername, ra->process_args );

        if( (res = RunFilter( frame, buffer )) != PERR_OK ) {
            ReplyRexxWaitItem( ra );
        }
    } else {
        res = PERR_OUTOFMEMORY;
    }

    return res;
}

/*
    Main routine for effect execution. This is run from within the main
    task and it spawns the subtask to handle the execution. If argstr != NULL
    sends it to the effect routine.
*/

Prototype PERROR RunFilter( FRAME *frame, UBYTE *argstr );

PERROR RunFilter( FRAME *frame, UBYTE *argstr )
{
    char argbuf[ARGBUF_SIZE];
    struct Process *p;
    PERROR res = PERR_OK;

    D(bug("RunFilter( %08X, %s )\n",frame,(argstr) ? argstr : (STRPTR)"NULL" ));

    LOCKGLOB();

    if(frame->selbox.MinX == ~0) {
        SelectWholeImage( frame );
    }

    D(bug("\tSelBox is (%d,%d)-(%d,%d)\n",frame->selbox.MinX, frame->selbox.MinY,
           frame->selbox.MaxX, frame->selbox.MaxY ));

    /*
     *  Reserve frame and lock windows
     */

    if(ObtainFrame( frame, BUSY_READONLY )) {

        if(argstr)
            sprintf(argbuf,"%lu %s",frame,argstr);
        else
            sprintf(argbuf,"%lu",frame);

#ifdef DEBUG_MODE
        p = CreateNewProcTags( NP_Entry, Filter, NP_Cli, FALSE,
            NP_Output, frame->debug_handle = OpenDebugFile( DFT_Effect ),
            NP_CloseOutput,TRUE, NP_Name, frame->nd.ln_Name,
            NP_StackSize, globals->userprefs->extstacksize,
            NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf, TAG_END );
#else
        p = CreateNewProcTags( NP_Entry, Filter, NP_Cli, FALSE,
            NP_Output, Open("NIL:",MODE_NEWFILE),
            NP_CloseOutput, TRUE, NP_Name, frame->nd.ln_Name,
            NP_StackSize, globals->userprefs->extstacksize,
            NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf, TAG_END );
#endif

        if(!p) {
            ReleaseFrame( frame );
            D(bug("\tCouldn't create a new process.\n"));
            res = PERR_GENERAL;
        } else {
            SetFrameStatus( frame, 1 );
            LOCK(frame);
            frame->currproc = p;
            UNLOCK(frame);
        }
    }

    UNLOCKGLOB();

    return res;
}


/*
    This goes through the whole select area and calls
    the execute routine, which may also be the local Cut / Copy / Whatever.
    BUG: Does not react too well on failed routines.
*/

PERROR ExecEasyFilter( FRAME *frame, FPTR code, EXTBASE *ExtBase )
{
    WORD row,col;
    ROWPTR cp,addr;
    auto int (* ASM X_EasyExec)( REG(d0) FRAME *, REG(a0) UBYTE *r, REG(a1) UBYTE *g,
                                 REG(a2) UBYTE *b, REG(d1) WORD, REG(d2) WORD,
                                 REG(a6) EXTBASE * );

    X_EasyExec = (VOID *) code;

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        cp = GetPixelRow( frame, row, ExtBase );

        if(Progress(frame,row,ExtBase))
            break;

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
            addr = cp + MULS16(col,frame->pix->components);
            switch( frame->pix->colorspace ) {
                case CS_ARGB:
                    (*X_EasyExec)( frame, addr+1, addr+2, addr+3, row, col, ExtBase ); /* BUG: should check */
                    break;

                case CS_RGB:
                    (*X_EasyExec)( frame, addr, (addr+1), (addr + 2), row, col, ExtBase ); /* BUG: should check */
                    break;

                case CS_GRAYLEVEL:
                    (*X_EasyExec)( frame, addr, addr, addr, row, col, ExtBase ); /* BUG: should check */
                    break;
            }
        }

        PutPixelRow( frame, row, cp,ExtBase );

    }
    return PERR_OK;
}

/*
    This is an interface for externals to ExecEasyFilter().
    BUG: Not maybe necessary.
*/

Local
FRAME *EasyFilter( FRAME *frame, EFFECT *effect, EXTBASE *ExtBase )
{
    FPTR X_EasyExec;
    char *X_EasyTitle, buf[80];
    APTR UtilityBase = ExtBase->lb_Utility;

    D(bug("EasyFilter()\n"));

    /* Init variables */
    sprintf(buf,XGetStr(mEXECUTING_EASYEXEC),effect->info.nd.ln_Name);
    X_EasyExec  = (FPTR)GetTagData( PPTX_EasyExec, NULL, effect->info.tags );
    X_EasyTitle = (char *)GetTagData( PPTX_EasyTitle, (ULONG)buf,effect->info.tags);

    /* Execute */

    InitProgress( frame, X_EasyTitle, frame->selbox.MinY, frame->selbox.MaxY, ExtBase );

    ExecEasyFilter( frame, X_EasyExec, ExtBase );

    FinishProgress( frame, ExtBase );

    if( frame->errorcode != PERR_OK )
        return NULL;

    return frame;
}


/*
    Main effect IDCMP control. Returns effect selected or NULL, if nothing
    happened. -1 if the user just wanted to quit...
*/

EFFECT *HandleFilterIDCMP( EXTBASE *ExtBase, struct EffectWindow *fw, ULONG rc )
{
    APTR entry;
    EFFECT *f = NULL;
    APTR    func;
    struct IntuitionBase *IntuitionBase = ExtBase->lb_Intuition;
    struct Library *UtilityBase = ExtBase->lb_Utility;
    struct ExecBase *SysBase = ExtBase->lb_Sys;
    ULONG clicked, secs, ms;

    switch(rc) {
        case WMHI_CLOSEWINDOW:
        case GID_FW_CANCEL:
            D(bug("User wanted to quit filtering\n"));
            f = NEGNUL;
            break;

        case GID_FW_INFO:
            if( entry = (APTR) FirstSelected( fw->GO_List ) ) {
                SHLOCKGLOB();
                f = (EFFECT *) FindName( &globals->effects, entry );
                UNLOCKGLOB();
                ShowExtInfo( ExtBase, &(f->info), fw->win );
                f = NULL; /* We don't want to quit, however */
            }
            break;

        case GID_FW_EXEC:
            /*
             *  Read the current listview object. If you change, change the code
             *  below (doubleclick) as well.
             */
            if( entry = (APTR) FirstSelected( fw->GO_List ) ) {
                LOCKGLOB();
                f = (EFFECT *) FindName( &globals->effects, entry );
                if(f) {
                    f->info.usecount++;
                } else {
                    D(bug("FATAL: FindName() failed!\n"));
                }
                UNLOCKGLOB();
            }
            break;

        case GID_FW_LIST:

            /*
             *  Update gadgets. If the corresponding function can be found
             *  from effect's tag array, then the gadget is activated, otherwise
             *  it is disabled.
             */

            if( entry = (APTR) FirstSelected( fw->GO_List ) ) {
                SHLOCKGLOB();
                f = (EFFECT *) FindName( &globals->effects, entry );
                if( !f->info.islibrary ) {
                    func = (APTR) GetTagData( PPTX_EasyExec, NULL, f->info.tags );
                } else {
                    func = (APTR) TRUE;
                }

                XSetGadgetAttrs( ExtBase,(struct Gadget *)fw->GO_Exec, fw->win, NULL, GA_Disabled, !func, TAG_END );
                XSetGadgetAttrs( ExtBase,(struct Gadget *)fw->GO_Info, fw->win, NULL, GA_Disabled, FALSE, TAG_END );
                UNLOCKGLOB();

                f = NULL;
            }

            /*
             *  Check for a double click
             */

            GetAttr( LISTV_LastClicked, fw->GO_List, &clicked );
            if(clicked == fw->lastclicked) {
                CurrentTime( &secs, &ms );
                if( DoubleClick( fw->lcs, fw->lcm, secs, ms )) {
                    LOCKGLOB();
                    f = (EFFECT *) FindName( &globals->effects, entry );
                    f->info.usecount++;
                    UNLOCKGLOB();
                }
            }
            CurrentTime( &fw->lcs, &fw->lcm );
            fw->lastclicked = clicked;
            break;

        default:
            break;
    }

    return f;
}


/*
    This routine will open a new window, in which one can select the
    effect to be applied.
    This routine runs as it's own process and may be dispatched several times.
*/

SAVEDS ASM VOID Filter( REG(a0) UBYTE *argstr, REG(d0) ULONG len )
{
    struct EffectWindow fw;
    ULONG sigmask, sig, rc, *optarray = NULL;
    int quit = 0;
    EXTBASE *ExtBase = NULL;
    struct Library *BGUIBase;
    struct IntuitionBase *IntuitionBase;
    struct Library *SysBase = SYSBASE();
    FRAME *f = NULL,*newframe = NULL;
    struct Window *win;
    EFFECT *effect = NULL;
    struct PPTMessage *msg;
    UBYTE *filname=NULL,*args = NULL;
    BOOL rexx=FALSE;

    D(bug("Filter()\n"));

    if( (ExtBase = NewExtBase(TRUE)) == NULL) {
        D(bug("LIB BASE ALLOCATION FAILED!\n"));
        goto errorexit;
    }

    /*
     *  Set up variables and check for REXX command
     */

    if( optarray = ParseDOSArgs( argstr, "FRAME/A/N,NAME/K,ARGS/K/N,REXX/S", ExtBase ) ) {
        f = (FRAME *) *( (ULONG *)optarray[0]) ;
        if(optarray[1]) { /* A name was given */
            filname = (UBYTE *)optarray[1];
            SHLOCKGLOB();
            effect  = (EFFECT *)FindIName( &globals->effects, filname );
            UNLOCKGLOB();
            if(!effect) {
                char erbuf[80];

                D(bug("\tFindname(%s) failed\n",filname));
                sprintf(erbuf,XGetStr(mUNKNOWN_EFFECT),filname);
                SetErrorMsg( f, erbuf );
                goto errorexit;
            }
            if( optarray[2] ) args = (UBYTE *)PEEKL(optarray[2]);
            rexx = (BOOL)optarray[3];
        }
    } else {
        InternalError( XGetStr(mFILTER_INCORRECT_ARGS) );

        /*
         *  Attempt a last, desperate extraction of the frame address
         */

        if( NULL == (f = (FRAME *)atol(argstr))) {
            InternalError("Filter(): NULL frame!?!");
        }
        goto errorexit;
    }

    D(bug("\tFRAME = %08X EFFECT = '%s' ARGS = '%s'\n",
           f,(filname) ? (char *)filname : "NULL",(args) ? (char *)args : "NULL"));

    if(NewTaskProlog(f,ExtBase) != PERR_OK) goto errorexit;

    BGUIBase = ExtBase->lb_BGUI;
    IntuitionBase = ExtBase->lb_Intuition;

    /*
     *  If there was no effect sent to us, then we will ask one from the user.
     */

    if(!effect) { /* No effect yet allocated */

        D(bug("\tOpening Effect window\n"));

        bzero(&fw, sizeof(struct EffectWindow));

        win = GimmeFilterWindow( ExtBase, f, &sigmask, &fw );
        if(win) {
            AddExtEntries( ExtBase, win, fw.GO_List, NT_EFFECT, AEE_ALL ); /* Update window display */
            while(!quit) {
                sig = Wait(sigmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F );

                if (sig & SIGBREAKF_CTRL_C ) {
                    D(bug("***** BREAK *****\n"));
                    quit++;
                }

                if( sig & SIGBREAKF_CTRL_F ) {
                    if(!win) win = WindowOpen(fw.WO_Win);
                    if( win ) {
                        WindowToFront( win );
                        ActivateWindow( win );
                    }
                }

                if (sig & sigmask) {
                    while ((rc = HandleEvent(fw.WO_Win)) != WMHI_NOMORE) {
                        if( (effect = HandleFilterIDCMP( ExtBase, &fw , rc )) != NULL)
                            quit++;

                    } /* while (rc) */
                }
            } /* while */
            DisposeObject(fw.WO_Win);
        } else {
            D(bug("\t\tCouldn't open effect window\n"));
            Req(NEGNUL,NULL,XGetStr(mCOULDNT_OPEN_EFFECT_WINDOW) );
        }
    }

    D(bug("\tEffect = %08X\n",effect));

    /* Make the actual execution. */
    if(effect != NEGNUL && effect != NULL) {
        newframe = ExecFilter(ExtBase,f,effect,args,rexx);
    }

    D(bug("\tFilter() done, sending message\n"));

errorexit:

    if(optarray)
        FreeDOSArgs( optarray, ExtBase );

    /*
     *  Prepare to send message to main program
     */

    msg = AllocPPTMsg( sizeof(struct PPTMessage), ExtBase );
    msg->frame = f;
    msg->code = PPTMSG_EFFECTDONE;
    msg->data = (APTR)newframe;

    /* Send the message */
    SendPPTMsg( globals->mport, msg, ExtBase );

    WaitDeathMessage( ExtBase );

    EmptyMsgPort( ExtBase->mport, ExtBase );

    if(ExtBase) RelExtBase(ExtBase);

    /* Die. */
}


/*
    This will take care of the actual effecting.

    Returns a pointer to the new, rendered frame or NULL, if no change was done (for
    example due to an error during rendering or maybe a break).
 */

Local
FRAME *ExecFilter( EXTBASE *ExtBase, FRAME *frame, EFFECT *effect, char *args, BOOL rexx )
{
    char *template;
    ULONG colorspaces, *argitemarray = NULL;
    struct TagItem tags[8];
    BOOL nonewframe;
    FRAME *newframe = NULL, *fr, *res = NULL;
    struct Library *UtilityBase = ExtBase->lb_Utility, *EffectBase = NULL;
    struct ExecBase *SysBase = ExtBase->lb_Sys;
    FPTR easyeffect;

    D(bug("ExecFilter()\n"));

    /*
     *  First, get info on the effect.  If it is not a library,
     *  it must be an old-style easy effect.
     */

    if( effect->info.islibrary ) {
        EffectBase = OpenModule( (EXTERNAL *)effect, ExtBase );
        if(!EffectBase) return NULL; // BUG: No error message
        nonewframe  = (BOOL)EffectInquire( PPTX_NoNewFrame, ExtBase );
        easyeffect  = NULL;
        colorspaces = EffectInquire( PPTX_ColorSpaces, ExtBase );
        template    = (STRPTR) EffectInquire( PPTX_RexxTemplate, ExtBase );
    } else {
        easyeffect = (FPTR)GetTagData( PPTX_EasyExec, NULL, effect->info.tags );
        colorspaces = GetTagData( PPTX_ColorSpaces, CSF_RGB, effect->info.tags );
        nonewframe  = (BOOL)GetTagData( PPTX_NoNewFrame, FALSE, effect->info.tags );
        template    = NULL; // These don't have them.
    }

    /*
     *  Is our current colorspace recognized?
     */

    if( (colorspaces & (1 << frame->pix->colorspace)) == 0) {
        XReq( NEGNUL, NULL, XGetStr(mEFFECT_DOESNT_SUPPORT_CSPACE),
                            effect->info.nd.ln_Name,
                            ColorSpaceNames[frame->pix->colorspace]);
        if(EffectBase) CloseModule(EffectBase,ExtBase);
        return NULL;
    }

    /*
     *  Do check the command template, if it exists and this was a REXX
     *  command.
     */

    if(template && args) {
        if( (argitemarray = ParseDOSArgs( args, template, ExtBase )) == NULL ) {
            char buf[256];
            // XReq( NEGNUL, NULL, "\nInvalid REXX arguments!\n" );
            // SetErrorCode( frame, PERR_INVALIDARGS );
            sprintf(buf,XGetStr(mINVALID_ARGS_EXPECTED_X), template);
            SetErrorMsg( frame, buf );
            if( EffectBase ) CloseModule(EffectBase, ExtBase);
            return NULL;
        }
    }

    /*
     *  Initialize variables and set up tagitem array.
     */

    if(template && args) {
        tags[0].ti_Tag  = PPTX_RexxArgs; tags[0].ti_Data = (ULONG) argitemarray;
    } else {
        tags[0].ti_Tag = TAG_END; tags[0].ti_Data = 0L;
    }
    tags[1].ti_Tag  = TAG_END; tags[1].ti_Data = 0L;


    /*
     *  Do the actual effecting.
     */

    if(easyeffect || EffectBase) {
        D(APTR b);

        ChangeBusyStatus( frame, BUSY_CHANGING );
        LOCK(frame);
        frame->currext = (EXTERNAL *)effect; /* MARK: We're busy with this */
        UNLOCK(frame);

        /*
         *  fr will be either a copy (or if the module requested)
         *  the original frame
         */

        if(!nonewframe) {
            D(bug("Duplicating a new frame for the program\n"));
            fr = newframe = DupFrame( frame, DFF_COPYDATA, ExtBase );
            if( !newframe ) {
                goto errorexit;
            }
        } else {
            fr = frame;
        }

        D(bug("\tExecuting effect %s...\n",effect->info.nd.ln_Name));
        D(bug("====================================\n"));
        D(b = StartBench());

        if( EffectBase ) {
            res = EffectExec( fr, &tags[0], ExtBase );
        } else {
            res = EasyFilter( fr, effect, ExtBase );
        }

        D(StopBench(b));
        D(bug("====================================\n"));
        D(bug("\tExec() returns %08X\n",res));

        if( fr->parent )
            CloseInfoWindow( fr->parent->mywin, ExtBase );
        else
            CloseInfoWindow( fr->mywin, ExtBase );

        if(MasterQuit) /* In case of a master quit enabled, then go down gracefully */
            goto quit;

        LOCK(fr);

        /*
         *  Check for failure. If this command came directly from the user,
         *  we'll show up an error message, if from REXX, we'll pass it down to
         *  the main task to be returned to REXX.
         */

        if(res == NULL) {

            D(bug("\tAn error occurred during the filtering...\n"));

            if( rexx ) {
                CopyError( fr, frame );
            } else {
                if( fr->doerror &&
                    (fr->errorcode != PERR_BREAK) &&
                    (fr->errorcode != PERR_CANCELED) ) {
                        XReq(NEGNUL,NULL,XGetStr(mEFFECT_REPORTS_ERROR),
                            effect->info.nd.ln_Name, frame->nd.ln_Name, GetErrorMsg( fr, ExtBase) );
                    } else {
                        D(bug("\t\tError has already been reported or it is an irrelevant one\n"));
                    }

                /*
                 *  We must also make sure that no error is shown on BREAK
                 *  and CANCELED errors when we return
                 */

                fr->doerror = FALSE; /* Shown */
            }

        }

        UNLOCK(fr);

      quit:
        if(res == NULL && newframe) {
            D(bug("Failed, so I am removing the new frame\n"));
            RemFrame(newframe,ExtBase);
        }

    }

    ClearProgress( frame, ExtBase );

errorexit:
    if(EffectBase) CloseModule( EffectBase, ExtBase );

    if(argitemarray) FreeDOSArgs( argitemarray, ExtBase );
    return res;
}




/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

