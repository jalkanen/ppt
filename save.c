/*
    PROJECT: ppt
    MODULE : save.c

    Code for saving pictures.

    $Id: save.c,v 1.2 1995/07/23 21:26:50 jj Exp $
*/

#include <defs.h>
#include <misc.h>

#include <gui.h>

#include <libraries/asl.h>
#include <dos/dostags.h>

#include <clib/utility_protos.h>
#include <clib/bgui_protos.h>
#include <clib/alib_protos.h>

#include <pragma/utility_pragmas.h>
#include <pragma/bgui_pragmas.h>
#include <pragma/intuition_pragmas.h>
#include <pragma/graphics_pragmas.h>

#include <stdlib.h>

extern    void      SavePicture( __A0 UBYTE *, __D0 ULONG );
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

    if(frame->mywin)
        PutIWToSleep(frame->mywin);

    if( frame->selbox.MinX != ~0 && frame->selstatus & 0x01) {
        DrawSelectBox( frame, frame->selbox.MinX, frame->selbox.MinY,
                              frame->selbox.MaxX, frame->selbox.MaxY );
        frame->selstatus = 0xFF;
    }

    if(argstr)
        sprintf(argbuf,"%lu %s",frame, argstr);
    else
        sprintf(argbuf,"%lu",frame);

#ifdef DEBUG_MODE
    p = CreateNewProcTags( NP_Entry, SavePicture, NP_Cli, TRUE, NP_Output, Open("dtmp:save.log",MODE_NEWFILE),
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
        if(frame->mywin) AwakenIW(frame->mywin);
        ReleaseFrame( frame );
        frame->selstatus = 0;
        return PERR_INITFAILED;
    }

    frame->currproc = p;
    UNLOCKGLOB();

    return PERR_OK;
}

void CleanRender( FRAME *frame, struct RastPort *rp, EXTBASE *xb )
{
    int i;

    for(i = 0; i < rp->BitMap->Depth; i++) {
        if(rp->BitMap->Planes[i])
            FreeRaster( rp->BitMap->Planes[i],
                        (ULONG)frame->pix->width,
                        (ULONG)frame->pix->height );
    }

    if(rp->BitMap) pfree(rp->BitMap);
    pfree( rp );
}

struct RastPort *DummyRender( FRAME *frame, EXTBASE *xb )
{
    struct RastPort *rp;
    struct BitMap   *bm;
    WORD height = frame->pix->height, width = frame->pix->width;
    BYTE depth,i;
    UBYTE *ctable;

    /*
     *  Initialization phase
     */

    depth = frame->disp->depth; /* BUG! */
    rp = pzmalloc( sizeof(struct RastPort) );
    bm = pzmalloc( sizeof(struct BitMap) );
    InitBitMap( bm, depth, width, height );
    for(i = 0; i < depth; i++) {
        if(!(bm->Planes[i] = AllocRaster( width, height ))) { /* BUG: should tell! */
            CleanRender( frame, rp, xb );
            return NULL;
        }
    }

    InitRastPort( rp );
    rp->BitMap = bm;

    /*
     *  Render phase
     */

    if(Frame2Bitmap( frame, rp, ctable, xb ) != PERR_OK) {
        CleanRender( frame, rp, xb );
        return NULL;
    }

    return rp;
}

/*
    Dispatches the external saver routine.
*/

Local
void DoTheSave( FRAME *frame, LOADER *ld, UBYTE mode, EXTDATA *xd )
{
    volatile char errbuf[ERRBUFLEN];
    volatile int res;
    volatile struct TagItem tags[] = { PPTX_ErrMsg, &errbuf[0], TAG_DONE };
    volatile APTR UtilityBase = xd->lb_Utility, DOSBase = xd->lb_DOS;
    volatile BPTR fh;
    volatile ULONG colorspaces;

    D(bug("DoTheSave(%s,%s,%d)\n",frame->nd.ln_Name, ld->info.nd.ln_Name, mode));

    /*
     *  Open save file
     */

    fh = Open( frame->fullname, MODE_NEWFILE );
    if(!fh) {
        XReq( GetFrameWin(frame), NULL, "\nUnable to open write file\n" );
        return;
    }

    /*
     *  Select which routine to call, SaveTrueColor or SaveColorMapped
     */

    if(mode == SAVE_TRUECOLOR) {
        volatile auto __D0 int (*X_SaveT)( __D0 BPTR, __A0 FRAME *, __A1 struct TagItem *, __A6 EXTDATA * );

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
        volatile DISPLAY *realdisp;
        volatile auto __D0 int (*X_SaveC)( __D0 BPTR, __A0 FRAME *, __A1 struct TagItem *, __A2 DISPLAY *, __A6 EXTDATA * );

        X_SaveC = (FPTR) GetTagData( PPTX_SaveColorMapped, NULL, ld->info.tags );

        D(bug("\tSaving colormapped image\n-+-+-+-+-+-+\n"));

        if(frame->disp->scr == MAINSCR) {
            XReq( GetFrameWin(frame), NULL, "\nYou do not have a rendered image.\n"
                                            "Set the correct format in Render Options Menu.\n" );
            return;
        }

//        realdisp = DummyRender( frame, xd );
        realdisp = frame->disp;

        res = (*X_SaveC)(fh, frame, &tags[0], realdisp, xd );

        D(bug("+-+-+-+-+-+-\n"));
    }

    if(MasterQuit)
        goto errexit;

    /*
     *  Show error message, if needed.
     */

    if( frame->lasterror )
        frame->lasterror = PERR_OK;
    else {
        if(res != PERR_OK && res != PERR_CANCELED) {
            XReq( GetFrameWin(frame), NULL, "\nError while saving : %s\n", ErrorMsg( res ));
        }
    }

    /*
     *  Close file and delete it if anything went wrong
     */

errexit:

    Close(fh);
    if(res != PERR_OK) {
        if(DeleteFile( frame->fullname ) == FALSE)
            XReq( GetFrameWin(frame), NULL, "Warning:\n\nUnable to remove file '%s'", frame->fullname );
    }
}

void DoSaveMXGadgets( FRAME *frame, struct SaveWin *gads, LOADER *ld, EXTDATA *xd )
{
    ULONG activemode;
    APTR IntuitionBase = xd->lb_Intuition, UtilityBase = xd->lb_Utility;

    GetAttr( MX_Active, gads->Mode, &activemode );

    /*
     *  Set the gadgets correctly. First, we need to set the gadget
     *  into enabled state. BUG: should be done just once.
     */

    XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, GA_Disabled, FALSE, TAG_END );

    /* CASE Truecolor */

    if( GetTagData( PPTX_SaveTrueColor, NULL, ld->info.tags ) ) {
        XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, MX_EnableButton, SAVE_TRUECOLOR, TAG_END );
    } else {
        XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, MX_DisableButton, SAVE_TRUECOLOR, TAG_END );
        if( activemode == SAVE_TRUECOLOR )
            XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, MX_Active, SAVE_COLORMAPPED, TAG_END );
    }

    /* CASE Colormapped */

    if( GetTagData( PPTX_SaveColorMapped, NULL, ld->info.tags ) ) {
        XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, MX_EnableButton, SAVE_COLORMAPPED, TAG_END );
    } else {
        XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, MX_DisableButton, SAVE_COLORMAPPED, TAG_END );
        if( activemode == SAVE_COLORMAPPED )
            XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, MX_Active, SAVE_TRUECOLOR, TAG_END );
    }
}

Local
HandleSaveIDCMP( FRAME *frame, struct SaveWin *gads, ULONG rc, EXTDATA *xd )
{
    LOADER *ld;
    STRPTR file;
    APTR entry;
    ULONG activemode;
    APTR SysBase = xd->lb_Sys, IntuitionBase = xd->lb_Intuition,
         UtilityBase = xd->lb_Utility, DOSBase = xd->lb_DOS;

    switch(rc) {
        case GID_SW_CANCEL:
        case IDCMP_CLOSEWINDOW:
            return 1;
        case GID_SW_SAVE:
            if( entry = (APTR) FirstSelected( gads->Format )) {
                SHLOCKGLOB();
                ld = (LOADER *) FindName( &globals->loaders, entry );
                UNLOCKGLOB();
                GetAttr( MX_Active, gads->Mode, &activemode );
                WindowClose( gads->Win );
                DoTheSave( frame, ld, activemode, xd );
                return 1;
            }
            break;
        case GID_SW_GETFILE:
            if(DoRequest( gads->Frq ) == FRQ_OK) {
                GetAttr( FRQ_Path, gads->Frq, &file );
                strcpy(frame->fullname, file);
                D(bug("Got new file name %s\n",frame->fullname ));
                XSetGadgetAttrs( xd, (struct Gadget *)gads->Name, gads->win, NULL,
                                 INFO_TextFormat, frame->fullname, TAG_END );
            }
            break;
        case GID_SW_SAVERS:
            if( entry = (APTR) FirstSelected( gads->Format )) {
                SHLOCKGLOB();
                ld = (LOADER *) FindName( &globals->loaders, entry );
                UNLOCKGLOB();

                DoSaveMXGadgets(frame, gads, ld, xd );
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

__geta4 void SavePicture( __A0 UBYTE *argvect, __D0 ULONG len )
{
    char path[256], *s;
    struct SaveWin gads;
    ULONG sigmask, sig, rc, sttags[] = { ASLFR_InitialDrawer, NULL, TAG_END };
    BOOL quit = FALSE;
    EXTBASE *xd;
    FRAME *frame;
    struct PPTMessage *msg;
    APTR IntuitionBase,SysBase, DOSBase;
    UBYTE *dpath = NULL, *args;
    LOADER *loader = NULL;
    UBYTE activemode;
    ULONG *optarray = NULL;

    xd = NewExtBase(TRUE);

    IntuitionBase = xd->lb_Intuition; SysBase = xd->lb_Sys; DOSBase = xd->lb_DOS;

    /*
     *  Extract any information the REXX program sent us.
     */

    // D(bug("\tARGV = '%s'\n", argvect ));
    if( optarray = ParseDOSArgs( argvect, "FRAME/A/N,PATH/K,FORMAT/K,TYPE/N/K,ARGS/K", xd )) {
        frame = (FRAME *) *( (ULONG *)optarray[0]);
        if( optarray[1] ) { /* PATH existed */
            dpath = (UBYTE *)optarray[1];
            loader = (LOADER *) FindName( &globals->loaders, (UBYTE *)optarray[2] );
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

    D(bug("SavePicture(%08X, '%s', ARGS = '%s')\n",frame, dpath ? dpath  : "NULL",
                                                      args ? args : "NULL"));

    if(!frame) goto errorexit;

    if( loader ) {

        if(dpath)
            strncpy( frame->fullname, dpath, MAXPATHLEN );

        D(bug("DoTheSave: frame = %08X, loader = '%s', activemode = %d, path = %s\n",
               frame, loader->info.nd.ln_Name, activemode, dpath ));

        DoTheSave( frame, loader, activemode, xd );

        goto errorexit;
    }

    UpdateProgress(frame,"Waiting user input",0,xd);

    if(GimmeSaveWindow(frame, xd, &gads )) {

        /*
         *  Initialize the save requester.
         */

        strcpy(path, frame->fullname);
        s = PathPart(path);
        *s = '\0';
        sttags[1] = (ULONG)path;
        SetAttrsA( gads.Frq, (struct TagItem *)sttags );
        sttags[0] = ASLFR_InitialFile;
        sttags[1] = (ULONG) FilePart(frame->fullname);
        SetAttrsA( gads.Frq, (struct TagItem *)sttags );

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

    msg = InitPPTMsg();
    msg->frame = frame;
    msg->code = PPTMSG_SAVEDONE;

    /* Send the message */
    DoPPTMsg( globals->mport, msg );

    PurgePPTMsg( msg ); /* Remove it */

    RelExtBase(xd);
    /* Die. */

}


/*----------------------------------------------------------------------*/
/*                             END OF CODE                              */
/*----------------------------------------------------------------------*/

