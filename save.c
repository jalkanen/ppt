/*
    PROJECT: ppt
    MODULE : save.c

    Code for saving pictures.

    $Id: save.c,v 2.14 1998/08/27 18:57:53 jj Exp $
*/

#include "defs.h"
#include "misc.h"

#include "gui.h"

#include "version.h"

#include <libraries/asl.h>
#include <dos/dostags.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <clib/bgui_protos.h>
#include <clib/alib_protos.h>

#include <pragmas/bgui_pragmas.h>

#include <proto/iomod.h>

#include <stdlib.h>

extern    ASM VOID  SavePicture( REG(a0) UBYTE *, REG(d0) ULONG );
Prototype PERROR    RunSave( FRAME *, UBYTE * );

/*----------------------------------------------------------------------*/

/*
    Save modes.  Note that these correspond to the
    SAVEF_#? tags in ppt.h
*/

#define SAVE_TRUECOLOR      0
#define SAVE_COLORMAPPED    1


#define DEFAULT_ANNOTATION "This image generated on an Amiga with PPT v"VERSION"."

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
                           NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf,
                           NP_StackSize, globals->userprefs->extstacksize, TAG_END );
#else
    p = CreateNewProcTags( NP_Entry, SavePicture, NP_Cli, FALSE, NP_Output, Open("NIL:",MODE_NEWFILE),
                           NP_CloseOutput, TRUE, NP_Name, frame->nd.ln_Name,
                           NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf,
                           NP_StackSize, globals->userprefs->extstacksize, TAG_END );
#endif
    if(!p) {
        Req( GetFrameWin( frame ),NULL,"Couldn't spawn a new process");
        ReleaseFrame( frame );
        frame->selstatus = 0;
        return PERR_INITFAILED;
    } else {
        SetFrameStatus( frame, 1 );
    }

    frame->currproc = p;
    UNLOCKGLOB();

    return PERR_OK;
}


/*
    Dispatches the external saver routine.

    BUG: This could use some cleaning up...
*/

Local
PERROR DoTheSave( FRAME *frame, LOADER *ld, UBYTE mode, STRPTR argstr, EXTBASE *ExtBase )
{
    volatile char filename[MAXPATHLEN];
    volatile PERROR res = PERR_FAILED;
    volatile struct TagItem tags[2];
    volatile APTR UtilityBase = ExtBase->lb_Utility, DOSBase = ExtBase->lb_DOS;
    volatile BPTR fh = NULL;
    struct   Library *IOModuleBase = NULL;
    ULONG format, *argarray = NULL;
    BOOL     delete = FALSE;

    D(bug("DoTheSave(%s,%s,%d)\n",frame->nd.ln_Name, ld->info.nd.ln_Name, mode));

    /*
     *  Open up the module
     */

    IOModuleBase = OpenModule( ld, ExtBase );
    if(!IOModuleBase) {
        InternalError("Unable to open module for saving!");
        return PERR_ERROR;
    }

    /*
     *  Build REXX message, if any.
     */

    if( argstr ) {
        STRPTR template;

        template = (STRPTR) IOInquire( PPTX_RexxTemplate, ExtBase );
        if( template ) {
            if( argarray = ParseDOSArgs( argstr, template, ExtBase ) ) {
                tags[0].ti_Tag = PPTX_RexxArgs;
                tags[0].ti_Data = (ULONG)argarray;
                tags[1].ti_Tag = TAG_DONE;
            } else {
                SetErrorCode( frame, PERR_INVALIDARGS );
                res = PERR_INVALIDARGS;
                goto errexit;
            }
        }
    } else {
        tags[0].ti_Tag = TAG_DONE;
    }


    /*
     *  Open save file
     */

    strcpy(filename, frame->path);
    AddPart(filename, frame->name, MAXPATHLEN);

    DeleteNameCount(filename);

    fh = Open( filename, MODE_NEWFILE );
    if(!fh) {
        SetErrorMsg(frame, "Unable to open write file" );
        res = PERR_FILEOPEN;
        goto errexit;
    }

    /*
     *  Check the colorspace, if it matches
     */

    res = PERR_FAILED;

    if( mode == SAVE_TRUECOLOR ) {
        if( (1 << frame->pix->colorspace) & ld->saveformats ) {
            res = PERR_OK;
            format = (1<<frame->pix->colorspace);
        }
    } else {
        if( ld->saveformats & CSF_LUT ) {
            if(frame->renderobject == NULL) {
                XReq( GetFrameWin(frame), NULL, "\nYou do not have a rendered image.\n"
                                                "Set the correct format in Render Options Menu.\n" );
                delete = TRUE;
                goto errexit;
            }

            res = PERR_OK;
            format = CSF_LUT;
        }
    }

    if( res == PERR_OK ) {
        res = IOSave( fh, format, frame, tags, ExtBase );
    } else {
        char errorbuf[MAXPATHLEN];
        D(bug("Colorspace diff: is %lu, should be %lu\n",(1<<frame->pix->colorspace),format));
        sprintf( errorbuf, "Saver module %s does not know how "
                           "to handle a %s colorspace!",
                           ld->info.nd.ln_Name,
                           ColorSpaceNames[frame->pix->colorspace]);
        SetErrorMsg( frame, errorbuf );
        res = PERR_FAILED;
        delete = TRUE;
        goto errexit;
    }

    if(MasterQuit) {
        res = PERR_OK; /* For quick exit */
        delete = TRUE;
        goto errexit;
    }

    /*
     *  Show error message, if needed.
     *  Kludge together some probable responses
     */

errexit:
    if( res != PERR_OK ) {
        if( frame->doerror && frame->errorcode != PERR_BREAK && frame->errorcode != PERR_CANCELED)
        {
            if( res == PERR_WARNING ) {
                ULONG r;
                r = XReq( GetFrameWin(frame), "Ignore|Remove Saved File",
                          ISEQ_C"\nWARNING!\n"
                          "While I was attempting to save '%s',\n"
                          "I got this warning message:\n\n"
                          ISEQ_I"%s\n",
                          frame->name, GetErrorMsg( frame, ExtBase ));

                if( r == 0 ) delete = TRUE;
            } else {
                XReq( GetFrameWin(frame), "Understood",
                      ISEQ_C"\nERROR!\n"
                      "While I was attempting to save '%s',\n"
                      "I got this error message:\n\n"
                      ISEQ_I"%s\n",
                      frame->name, GetErrorMsg( frame, ExtBase ));
                delete = TRUE;
            }
        } else {
            /*
             *  Either a break or user cancelled operation.  In both
             *  cases we remove the saved image.
             */
            delete = TRUE;
        }
        ClearError( frame );
    } else {
        frame->origtype = ld;
    }

    /*
     *  Close file and delete it if anything went wrong
     */

    if( argarray ) FreeDOSArgs( argarray, ExtBase );

    if(fh) Close(fh);

    if(IOModuleBase) CloseModule( IOModuleBase, ExtBase );

    CloseInfoWindow( frame->mywin, ExtBase );
    if(delete) {
        if(DeleteFile( filename ) == FALSE)
            XReq( GetFrameWin(frame), NULL, "Warning:\n\nUnable to remove file '%s'", filename );
    }

    return res;
}

/*
    Updates the visuals on the save window.
*/
void DoSaveMXGadgets( FRAME *frame, struct SaveWin *gads, LOADER *ld, EXTBASE *ExtBase )
{
    ULONG activemode;
    struct IntuitionBase *IntuitionBase = ExtBase->lb_Intuition;

    GetAttr( MX_Active, gads->Mode, &activemode );

    /*
     *  Set the gadgets correctly. First, we need to set the gadget
     *  into enabled state. BUG: should be done just once.
     */

//    XSetGadgetAttrs( ExtBase, (struct Gadget *)gads->Mode, gads->win, NULL,
//                     GA_Disabled, FALSE, TAG_END );

    /* CASE Truecolor */

    if( ld->saveformats & (CSF_GRAYLEVEL|CSF_RGB) ) {
        XSetGadgetAttrs( ExtBase, GAD(gads->Mode), gads->win, NULL,
                         MX_EnableButton, SAVE_TRUECOLOR, TAG_END );
    } else {
        XSetGadgetAttrs( ExtBase, GAD(gads->Mode), gads->win, NULL,
                         MX_DisableButton, SAVE_TRUECOLOR, TAG_END );
        if( activemode == SAVE_TRUECOLOR ) {
            XSetGadgetAttrs( ExtBase, GAD(gads->Mode), gads->win, NULL,
                             MX_Active, SAVE_COLORMAPPED, TAG_END );
        }
    }

    /* CASE Colormapped */

    if( ld->saveformats & CSF_LUT ) {
        XSetGadgetAttrs( ExtBase, GAD(gads->Mode), gads->win, NULL,
                         MX_EnableButton, SAVE_COLORMAPPED, TAG_END );
    } else {
        XSetGadgetAttrs( ExtBase, GAD(gads->Mode), gads->win, NULL,
                         MX_DisableButton, SAVE_COLORMAPPED, TAG_END );
        if( activemode == SAVE_COLORMAPPED ) {
            XSetGadgetAttrs( ExtBase, GAD(gads->Mode), gads->win, NULL,
                             MX_Active, SAVE_TRUECOLOR, TAG_END );
        }
    }
}

Local
HandleSaveIDCMP( FRAME *frame, struct SaveWin *gads, ULONG rc, PERROR *res, EXTBASE *ExtBase )
{
    LOADER *ld;
    STRPTR file;
    UBYTE *s, *e;
    APTR entry;
    ULONG activemode;
    APTR SysBase = ExtBase->lb_Sys, IntuitionBase = ExtBase->lb_Intuition,
         DOSBase = ExtBase->lb_DOS;
    char tmppath[MAXPATHLEN+1];
    struct TagItem t[3] = {
        ASLFR_InitialFile, NULL,
        ASLFR_InitialDrawer, NULL,
        TAG_END
    };

    switch(rc) {
        case GID_SW_CANCEL:
        case WMHI_CLOSEWINDOW:
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
                    MakeFrameName( e, frame->name, NAMELEN, ExtBase );
                }

                /*
                 *  Close window and save
                 */

                WindowClose( gads->Win );
                *res = DoTheSave( frame, ld, activemode, NULL, ExtBase );
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

            strncpy(tmppath, file, MAXPATHLEN);
            s = PathPart(tmppath);
            *s = '\0';
            t[1].ti_Data = (ULONG) tmppath;
            t[0].ti_Data = (ULONG) FilePart(file);
            SetAttrsA( gads->Frq, t );

            if(DoRequest( gads->Frq ) == FRQ_OK) {
                GetAttr( FRQ_Path, gads->Frq, (ULONG *)&file );
                XSetGadgetAttrs( ExtBase, (struct Gadget *)gads->Name, gads->win, NULL,
                                 STRINGA_TextVal, file, TAG_END );
            }
            break;

        case GID_SW_SAVERS:
            if( entry = (APTR) FirstSelected( gads->Format )) {
                SHLOCKGLOB();
                ld = (LOADER *) FindName( &globals->loaders, entry );
                UNLOCKGLOB();

                if(ld) {
                    DoSaveMXGadgets(frame, gads, ld, ExtBase );

                    /*
                     *  Change the postfix according to the loader.
                     */

                    if( ld->prefpostfix[0] ) {
                        GetAttr( STRINGA_TextVal, gads->Name, (ULONG *)&file );
                        strncpy(tmppath,file,MAXPATHLEN);
                        if(s = strrchr(tmppath,'.')) {
                            if(ld->prefpostfix[0] == '.') {
                                strcpy( s, ld->prefpostfix );
                            } else {
                                strcpy( s+1, ld->prefpostfix );
                            }
                        } else {
                            strcat(tmppath,ld->prefpostfix);
                        }
                        XSetGadgetAttrs( ExtBase, (struct Gadget *)gads->Name, gads->win, NULL,
                                         STRINGA_TextVal, tmppath, TAG_END );
                    }
                } else {
                    InternalError("Couldn't locate saver!");
                    *res = PERR_ERROR;
                    return 1;
                }
            }
            break;

        default:
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
    struct SaveWin gads = {0};
    ULONG sigmask, sig, rc;
    BOOL quit = FALSE;
    EXTBASE *ExtBase;
    FRAME *frame = NULL;
    struct PPTMessage *msg;
    APTR IntuitionBase,SysBase, DOSBase;
    UBYTE *dpath = NULL, *args = NULL, *name = NULL;
    LOADER *loader = NULL;
    UBYTE activemode = 0;
    ULONG *optarray = NULL;
    struct Extension *ext;
    PERROR res = PERR_OK;

    ExtBase = NewExtBase(TRUE);

    IntuitionBase = ExtBase->lb_Intuition; SysBase = ExtBase->lb_Sys; DOSBase = ExtBase->lb_DOS;

    /*
     *  Extract any information the REXX program sent us.
     */

    // D(bug("\tARGV = '%s'\n", argvect ));
    if( optarray = ParseDOSArgs( argvect, "FRAME/A/N,PATH/K,FORMAT/K,TYPE/N/K,ARGS/K,NAME/K", ExtBase )) {
        frame = (FRAME *) *( (ULONG *)optarray[0]);

        if( optarray[1] || optarray[5] ) { /* PATH or NAME existed */
            dpath = (UBYTE *)optarray[1];
            name  = (UBYTE *)optarray[5];
            loader = (LOADER *) FindIName( &globals->loaders, (UBYTE *)optarray[2] );
            if(!loader) {
                D(bug("\tFindName() failed\n"));
                res = PERR_ERROR;
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

    if(!frame) {
        res = PERR_ERROR;
        goto errorexit;
    }

    if((res = NewTaskProlog(frame,ExtBase)) != PERR_OK) goto errorexit;


    /*
     *  Set up default extensions
     */

    if( !(ext = FindExtension(frame,EXTNAME_ANNO,ExtBase))) {
        AddExtension( frame, EXTNAME_ANNO, DEFAULT_ANNOTATION, strlen(DEFAULT_ANNOTATION)+1, 0L,ExtBase);
    }

    if( !(ext = FindExtension(frame,EXTNAME_DATE,ExtBase))) {
        struct DateStamp ds;
        struct DateTime  dt;
        UBYTE dbuf[40],tbuf[40],buf[80];

        DateStamp(&ds);
        dt.dat_Stamp   = ds;
        dt.dat_Format  = FORMAT_DOS;
        dt.dat_Flags   = 0L;
        dt.dat_StrDay  = NULL;
        dt.dat_StrDate = dbuf;
        dt.dat_StrTime = tbuf;
        sprintf(buf,"Created on %s, %s",dbuf, tbuf);
        AddExtension(frame, EXTNAME_DATE, buf, strlen(buf)+1, 0L, ExtBase );
    }

    /*
     *  If the user already gave us all the information we needed,
     *  then let's rock-n-roll!
     */

    if( loader ) {

        if(dpath)
            strncpy( frame->path, dpath, MAXPATHLEN );

        if(name)
            strncpy( frame->name, name, NAMELEN );

        D(bug("DoTheSave: frame = %08X, loader = '%s', activemode = %d\n"
              "path = %s, name = %s\n",
               frame, loader->info.nd.ln_Name, activemode, dpath, name ));

        res = DoTheSave( frame, loader, activemode, args, ExtBase );

        goto errorexit;
    }

    /*
     *  If not, then begin the arduous waiting for signals...
     */

    if(GimmeSaveWindow(frame, ExtBase, &gads )) {
        ULONG sigport;

        /*
         *  Fetch sigmask and begin main loop
         */

        GetAttr(WINDOW_SigMask, gads.Win, &sigmask);
        sigport = 1<<ExtBase->mport->mp_SigBit;

        while(!quit) {
            sig = Wait( sigmask|SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_F|sigport );

            /*
             *   Break signal?
             */

            if(sig & SIGBREAKF_CTRL_C) {
                quit = TRUE;
                D(bug("\tgot break signal\n"));
                res = PERR_BREAK;
                break;
            }

            if( sig & SIGBREAKF_CTRL_F ) {
                WindowToFront( gads.win );
                ActivateWindow( gads.win );
            }

            /*
             *  Real signal?
             */

            if(sig & sigmask) {
                while(( rc = HandleEvent( gads.Win )) != WMHI_NOMORE) {
                    quit += HandleSaveIDCMP( frame, &gads, rc, &res, ExtBase );
                }
            }

            /*
             *  Message from the main task
             */

            if( sig & sigport ) {
                struct Message *msg;

                while( msg = GetMsg( ExtBase->mport ) ) {
                    if( msg->mn_Node.ln_Type == NT_REPLYMSG ) {
                        D(bug("\tA reply message received.  Strange, I don't"
                              " remember sending any messages at all...\n"));
                        FreePPTMsg( (struct PPTMessage *) msg, ExtBase );
                    } else {
                        D(bug("\tA strange message received, just replying...\n"));
                        ReplyMsg( msg );
                    }
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
        FreeDOSArgs( optarray, ExtBase );

    msg = AllocPPTMsg( sizeof( struct PPTMessage), ExtBase );
    msg->frame = frame;
    msg->code = PPTMSG_SAVEDONE;
    msg->data = (void *)res;

    /* Send the message */
    SendPPTMsg( globals->mport, msg, ExtBase );

    WaitDeathMessage(ExtBase);

    EmptyMsgPort( ExtBase->mport, ExtBase );

    if(ExtBase) RelExtBase(ExtBase);
    /* Die. */

}


/*----------------------------------------------------------------------*/
/*                             END OF CODE                              */
/*----------------------------------------------------------------------*/

