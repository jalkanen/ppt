/*
    PROJECT: ppt
    MODULE : save.c

    Code for saving pictures.

    $Id: save.c,v 1.8 1996/11/23 00:46:37 jj Exp $
*/

#include "defs.h"
#include "misc.h"

#include "gui.h"

#include <libraries/asl.h>
#include <dos/dostags.h>

#include <clib/utility_protos.h>
#include <clib/bgui_protos.h>
#include <clib/alib_protos.h>

#include <pragmas/utility_pragmas.h>
#include <pragmas/bgui_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#include <pragmas/graphics_pragmas.h>

#include <stdlib.h>

extern    ASM VOID  SavePicture( REG(a0) UBYTE *, REG(d0) ULONG );
Prototype PERROR    RunSave( FRAME *, UBYTE * );

/*----------------------------------------------------------------------*/

/* Save modes */
#define SAVE_TRUECOLOR      0
#define SAVE_COLORMAPPED    1


/*----------------------------------------------------------------------*/
/* Code */

/*
    Main dispatcher for saving operation. This is run from within the main
    task. If argstr != NULL, sends it over to the main task.

*/
PERROR RunSave( FRAME *frame, UBYTE *argstr )
{
    struct Process *p;
    char argbuf[ARGBUF_SIZE];


    /*
     *  Handle necessary windows, etc. Remove the selbox.
     */

    if(ObtainFrame( frame, BUSY_SAVING ) == FALSE)
        return PERR_INITFAILED; /* BUG: Should be something more intelligent */

    LOCKGLOB();

    RemoveSelectBox( frame );

    if(argstr)
        sprintf(argbuf,"%lu %s",frame, argstr);
    else
        sprintf(argbuf,"%lu",frame);

    D(bug("Running save(%s)\n",argbuf));

#ifdef DEBUG_MODE
    p = CreateNewProcTags( NP_Entry, SavePicture, NP_Cli, FALSE, NP_Output, OpenDebugFile( DFT_Save ),
                           NP_CloseOutput,TRUE, NP_Name, frame->nd.ln_Name,
                           NP_Priority, -1, NP_Arguments, argbuf,
                           NP_StackSize, EXTERNAL_STACKSIZE, TAG_END );
#else
    p = CreateNewProcTags( NP_Entry, SavePicture, NP_Cli, FALSE, NP_Output, Open("NIL:",MODE_NEWFILE),
                           NP_CloseOutput, TRUE, NP_Name, frame->nd.ln_Name,
                           NP_Priority, -1, NP_Arguments, argbuf,
                           NP_StackSize, EXTERNAL_STACKSIZE, TAG_END );
#endif
    if(!p) {
        Req( GetFrameWin( frame ),NULL,"Couldn't spawn a new process");
        ReleaseFrame( frame );
        frame->selstatus = 0;
        return PERR_INITFAILED;
    }

    frame->currproc = p;
    UNLOCKGLOB();

    return PERR_OK;
}


/*
    Dispatches the external saver routine.
*/

Local
void DoTheSave( FRAME *frame, LOADER *ld, UBYTE mode, EXTBASE *xd )
{
    volatile char filename[MAXPATHLEN];
    volatile int res;
    volatile struct TagItem tags[] = { TAG_DONE };
    volatile APTR UtilityBase = xd->lb_Utility, DOSBase = xd->lb_DOS;
    volatile BPTR fh;
    volatile ULONG colorspaces;

    D(bug("DoTheSave(%s,%s,%d)\n",frame->nd.ln_Name, ld->info.nd.ln_Name, mode));

    /*
     *  Open save file
     */

    strcpy(filename, frame->path);
    AddPart(filename, frame->name, MAXPATHLEN);

    DeleteNameCount(filename);

    fh = Open( filename, MODE_NEWFILE );
    if(!fh) {
        XReq( GetFrameWin(frame), NULL, "\nUnable to open write file\n" );
        return;
    }

    /*
     *  Select which routine to call, SaveTrueColor or SaveColorMapped
     */

    if(mode == SAVE_TRUECOLOR) {
        volatile auto int (* ASM X_SaveT)( REG(d0) BPTR, REG(a0) FRAME *, REG(a1) struct TagItem *, REG(a6) EXTBASE * );

        X_SaveT = (FPTR) GetTagData( PPTX_SaveTrueColor, NULL, ld->info.tags );

        colorspaces = GetTagData( PPTX_ColorSpaces, CSF_RGB, ld->info.tags );
        if( (colorspaces & (1<<frame->pix->colorspace)) == 0) {
            D(bug("Colorspace diff: is %lu, should be %lu\n",(1<<frame->pix->colorspace),colorspaces));
            XReq( GetFrameWin(frame), NULL, "\nThis saver cannot handle this colorspace!\n");
        } else {
            D(bug("\tSaving true color image...\n-+-+-+-+-+\n"));
            res = (*X_SaveT)(fh, frame, &tags[0], xd );
            D(bug("+-+-+-+-+-\n"));
        }
    } else { /* COLORMAPPED */
        volatile auto int (* ASM X_SaveC)( REG(d0) BPTR, REG(a0) FRAME *, REG(a1) struct TagItem *, REG(a6) EXTBASE * );

        X_SaveC = (FPTR) GetTagData( PPTX_SaveColorMapped, NULL, ld->info.tags );

        D(bug("\tSaving colormapped image\n-+-+-+-+-+-+\n"));

        if(frame->renderobject == NULL) {
            XReq( GetFrameWin(frame), NULL, "\nYou do not have a rendered image.\n"
                                            "Set the correct format in Render Options Menu.\n" );
            return;
        }

        res = (*X_SaveC)(fh, frame, &tags[0], xd );

        D(bug("+-+-+-+-+-+-\n"));
    }

    if(MasterQuit)
        goto errexit;

    /*
     *  Show error message, if needed.
     *  Kludge together some probable responses
     */


    if( res != PERR_OK ) {
        if( frame->doerror && frame->errorcode != PERR_BREAK && frame->errorcode != PERR_CANCELED)
        {
            if( res == PERR_WARNING ) {
                ULONG r;
                r = XReq( GetFrameWin(frame), "Ignore|Remove Saved File",
                          ISEQ_C"\nWARNING!\n"
                          "While I was attempting to save '%s',\n"
                          "I got this warning message:\n"
                          ISEQ_I"%s",
                          frame->name, GetErrorMsg( frame, xd ));

                if( r == 0 ) res = PERR_FAILED;
            } else {
                XReq( GetFrameWin(frame), "Understood",
                      ISEQ_C"\nERROR!\n"
                      "While I was attempting to save '%s',\n"
                      "I got this error message:\n"
                      ISEQ_I"%s",
                      frame->name, GetErrorMsg( frame, xd ));
            }
        }
    } else {
        if( res == PERR_BREAK || res == PERR_CANCELED) res = PERR_FAILED;
    }


    ClearError( frame );

    /*
     *  Close file and delete it if anything went wrong
     */

errexit:

    Close(fh);

    CloseInfoWindow( frame->mywin, xd );
    if(res == PERR_FAILED) {
        if(DeleteFile( filename ) == FALSE)
            XReq( GetFrameWin(frame), NULL, "Warning:\n\nUnable to remove file '%s'", filename );
    }
}

/*
    Updates the visuals on the save window.
    BUG:  BGUI problem:  the MX_Enable/DisableButton do not work if argument
          > 0.
*/
void DoSaveMXGadgets( FRAME *frame, struct SaveWin *gads, LOADER *ld, EXTBASE *xd )
{
    ULONG activemode;
    struct IntuitionBase *IntuitionBase = xd->lb_Intuition;
    struct Library *UtilityBase = xd->lb_Utility;

    GetAttr( MX_Active, gads->Mode, &activemode );

    /*
     *  Set the gadgets correctly. First, we need to set the gadget
     *  into enabled state. BUG: should be done just once.
     */

//    XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL,
//                     GA_Disabled, FALSE, TAG_END );

    /* CASE Truecolor */

    if( GetTagData( PPTX_SaveTrueColor, NULL, ld->info.tags ) ) {
        XSetGadgetAttrs( xd, GAD(gads->Mode), gads->win, NULL,
                         MX_EnableButton, SAVE_TRUECOLOR, TAG_END );
    } else {
        XSetGadgetAttrs( xd, GAD(gads->Mode), gads->win, NULL,
                         MX_DisableButton, SAVE_TRUECOLOR, TAG_END );
        if( activemode == SAVE_TRUECOLOR ) {
            XSetGadgetAttrs( xd, GAD(gads->Mode), gads->win, NULL,
                             MX_Active, SAVE_COLORMAPPED, TAG_END );
        }
    }

    /* CASE Colormapped */

    if( GetTagData( PPTX_SaveColorMapped, NULL, ld->info.tags ) ) {
        XSetGadgetAttrs( xd, GAD(gads->Mode), gads->win, NULL,
                         MX_EnableButton, SAVE_COLORMAPPED, TAG_END );
    } else {
        XSetGadgetAttrs( xd, GAD(gads->Mode), gads->win, NULL,
                         MX_DisableButton, SAVE_COLORMAPPED, TAG_END );
        if( activemode == SAVE_COLORMAPPED ) {
            XSetGadgetAttrs( xd, GAD(gads->Mode), gads->win, NULL,
                             MX_Active, SAVE_TRUECOLOR, TAG_END );
        }
    }
}

Local
HandleSaveIDCMP( FRAME *frame, struct SaveWin *gads, ULONG rc, EXTBASE *xd )
{
    LOADER *ld;
    STRPTR file;
    UBYTE *s, *e;
    APTR entry;
    ULONG activemode;
    APTR SysBase = xd->lb_Sys, IntuitionBase = xd->lb_Intuition,
         DOSBase = xd->lb_DOS;
    char tmppath[MAXPATHLEN];
    struct TagItem t[3] = {
        ASLFR_InitialFile, NULL,
        ASLFR_InitialDrawer, NULL,
        TAG_END
    };

    switch(rc) {
        case GID_SW_CANCEL:
        case IDCMP_CLOSEWINDOW:
            return 1;

        case GID_SW_SAVE:
            if( entry = (APTR) FirstSelected( gads->Format )) {

                /*
                 *  Fetch the loader and mode
                 */

                SHLOCKGLOB();
                ld = (LOADER *) FindName( &globals->loaders, entry );
                UNLOCKGLOB();
                GetAttr( MX_Active, gads->Mode, &activemode );

                /*
                 *  Get the path, split into parts and save it.  If the name
                 *  has been changed, try to do something about it.
                 */

                GetAttr( STRINGA_TextVal, gads->Name, (ULONG *)&file );

                s = PathPart(file);
                e = FilePart(file);

                bzero( frame->path, MAXPATHLEN ); /* BUG: Clumsy */
                strncpy( frame->path, file, (s-file) );

                if( strcmp( e, frame->name ) != 0 ) {
                    MakeFrameName( e, frame->name, NAMELEN, xd );
                }

                /*
                 *  Close window and save
                 */

                WindowClose( gads->Win );
                DoTheSave( frame, ld, activemode, xd );
                return 1;
            }
            break;

        case GID_SW_FILE:
            break;

        case GID_SW_GETFILE:

            /*
             *  First, make sure the path is correct.
             */

            GetAttr( STRINGA_TextVal, gads->Name, (ULONG *)&file );

            strcpy(tmppath, file);
            s = PathPart(tmppath);
            *s = '\0';
            t[1].ti_Data = (ULONG) tmppath;
            t[0].ti_Data = (ULONG) FilePart(file);
            SetAttrsA( gads->Frq, t );

            if(DoRequest( gads->Frq ) == FRQ_OK) {
                GetAttr( FRQ_Path, gads->Frq, (ULONG *)&file );
                XSetGadgetAttrs( xd, (struct Gadget *)gads->Name, gads->win, NULL,
                                 STRINGA_TextVal, file, TAG_END );
            }
            break;

        case GID_SW_SAVERS:
            if( entry = (APTR) FirstSelected( gads->Format )) {
                SHLOCKGLOB();
                ld = (LOADER *) FindName( &globals->loaders, entry );
                UNLOCKGLOB();

                if(ld) {
                    DoSaveMXGadgets(frame, gads, ld, xd );
                } else {
                    InternalError("Couldn't locate saver!");
                }
            }
            break;
    }
    return 0;
}

/*
    The main entry point for saving pictures. Opens up a window which
    should contain a) path b)name, c) format d) save truecolor/cmapped

    The control is then given to the actual saver routine, who can
    set his own options.

    BUG: REXX control handling could be prettier.
*/

SAVEDS ASM VOID SavePicture( REG(a0) UBYTE *argvect, REG(d0) ULONG len )
{
    struct SaveWin gads;
    ULONG sigmask, sig, rc;
    BOOL quit = FALSE;
    EXTBASE *xd;
    FRAME *frame;
    struct PPTMessage *msg;
    APTR IntuitionBase,SysBase, DOSBase;
    UBYTE *dpath = NULL, *args = NULL, *name = NULL;
    LOADER *loader = NULL;
    UBYTE activemode;
    ULONG *optarray = NULL;

    xd = NewExtBase(TRUE);

    IntuitionBase = xd->lb_Intuition; SysBase = xd->lb_Sys; DOSBase = xd->lb_DOS;

    /*
     *  Extract any information the REXX program sent us.
     */

    // D(bug("\tARGV = '%s'\n", argvect ));
    if( optarray = ParseDOSArgs( argvect, "FRAME/A/N,PATH/K,FORMAT/K,TYPE/N/K,ARGS/K,NAME/K", xd )) {
        frame = (FRAME *) *( (ULONG *)optarray[0]);

        if( optarray[1] || optarray[5]) { /* PATH or NAME existed */
            dpath = (UBYTE *)optarray[1];
            name  = (UBYTE *)optarray[5];
            loader = (LOADER *) FindIName( &globals->loaders, (UBYTE *)optarray[2] );
            if(!loader) {
                D(bug("\tFindName() failed\n"));
                goto errorexit;
            }
            activemode = *( (ULONG *)optarray[3] ) ? TRUE : FALSE;
            args = (UBYTE *)optarray[4];
        }

    } else {
        InternalError( "REXX Message of incorrect format" );
        goto errorexit;
    }

    D(bug("SavePicture(%08X, '%s':'%s', ARGS = '%s')\n",frame, dpath ? dpath  : (STRPTR)"NULL",
                                                        name ? name : (STRPTR)"NULL",
                                                        args ? args : (STRPTR)"NULL"));

    if(!frame) goto errorexit;

    if( loader ) {

        if(dpath)
            strncpy( frame->path, dpath, MAXPATHLEN );

        if(name)
            strncpy( frame->name, name, NAMELEN );

        D(bug("DoTheSave: frame = %08X, loader = '%s', activemode = %d\n"
              "path = %s, name = %s\n",
               frame, loader->info.nd.ln_Name, activemode, dpath, name ));

        DoTheSave( frame, loader, activemode, xd );

        goto errorexit;
    }

    UpdateProgress(frame,"Waiting user input",0,xd);

    if(GimmeSaveWindow(frame, xd, &gads )) {

        /*
         *  Fetch sigmask and begin main loop
         */

        GetAttr(WINDOW_SigMask, gads.Win, &sigmask);
        while(!quit) {
            sig = Wait( sigmask | SIGBREAKF_CTRL_C );

            /*
             *   Break signal?
             */

            if(sig & SIGBREAKF_CTRL_C) {
                quit = TRUE;
                D(bug("\tgot break signal\n"));
                break;
            }

            /*
             *  Real signal?
             */

            if(sig & sigmask) {
                while(( rc = HandleEvent( gads.Win )) != WMHI_NOMORE) {
                    quit += HandleSaveIDCMP( frame, &gads, rc, xd );
                }
            }

        } /* while(!quit) */
        D(bug("\tDisposing\n"));
        DisposeObject( gads.Win );
        DisposeObject( gads.Frq );
    }

    /*
     *  Prepare to send message to main program
     */

errorexit:
    if(optarray)
        FreeDOSArgs( optarray, xd );

    msg = AllocPPTMsg( sizeof( struct PPTMessage), xd );
    msg->frame = frame;
    msg->code = PPTMSG_SAVEDONE;

    /* Send the message */
    SendPPTMsg( globals->mport, msg, xd );

    WaitDeathMessage(xd);

    EmptyMsgPort( xd->mport, xd );

    if(xd) RelExtBase(xd);
    /* Die. */

}


/*----------------------------------------------------------------------*/
/*                             END OF CODE                              */
/*----------------------------------------------------------------------*/

