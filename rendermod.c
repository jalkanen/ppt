/*
    PROJECT: PPT
    MODULE:  rendermod.c

    $Id: rendermod.c,v 1.23 1999/10/02 16:33:07 jj Exp $

    This module and PPT are (C) Copyright Janne Jalkanen 1999.

    Rendering module support code.
*/

#include "defs.h"
#include "misc.h"
#include "render.h"

#include <dos/dostags.h>

/*------------------------------------------------------------------------*/
/* Prototypes */

Prototype PERROR DoRender( struct RenderObject * );
Prototype PERROR OpenRender( FRAME *, EXTBASE * );
Prototype VOID CloseRender( FRAME *, EXTBASE * );

PERROR InitializeColorSpace( struct RenderObject * );
PERROR InitializeDither( struct RenderObject * );
PERROR InitializeMode( struct RenderObject * );
PERROR InitializePalette( struct RenderObject * );
PERROR InitializeDevice( struct RenderObject * );


/*------------------------------------------------------------------------*/

/*
    Allocate the render object and initialize display
*/

PERROR OpenRender( FRAME *source, EXTBASE *PPTBase )
{
    struct RenderObject *rdo = source->renderobject;
    PERROR res = PERR_OK;

    D(bug("OpenRender()\n"));

    LOCK(source);

    /*
     *  If there is already an renderobject, destroy it
     *  and re-allocate.
     */

    if( rdo ) CloseRender( source, PPTBase );

    /*
     *  Allocate and initialize the new render object
     */

    rdo = pzmalloc( sizeof(struct RenderObject) );
    if(rdo) {
        rdo->frame   = source;
        rdo->PPTBase = PPTBase;

        rdo->buffer = pmalloc( (((source->pix->width+15)>>4)<<4) ); /* BUG: should not be here, should check */
        if( (res = AllocColorTable( source )) == PERR_OK ) {

            rdo->colortable = source->disp->colortable;
            res = InitializeDevice( rdo );

        }
    } else {
        res = PERR_OUTOFMEMORY;
    }

    if( res != PERR_OK ) {
        XReq( GetFrameWin(source), NULL, XGetStr(mFAILED_TO_START_RENDER), ErrorMsg( res, PPTBase ) );
        CloseRender( source, PPTBase );
        rdo = NULL;
    }

    D(bug("Returning RenderObject @ %08X\n",rdo));

    source->renderobject = rdo;

    UNLOCK(source);

    return res;
}

/*
    Do the opposite of OpenRender(). Destroy the display object.
*/

VOID CloseRender( FRAME *frame, EXTBASE *PPTBase )
{
    struct RenderObject *rdo = frame->renderobject;

    D(bug("CloseRender(%08X)\n",rdo));

    LOCK(frame);

    if(rdo) {
        /*
         *  Remove the palette window if we're still here
         */

        if( frame->pw ) ClosePaletteWindow( frame );

        if(rdo->DispDeviceD) {
            D(bug("\tDestroying evidence...\n"));
            (*rdo->DispDeviceD)( rdo );
        }

        frame->renderobject = NULL;
        pfree( rdo->buffer ); /* BUG: Should not be here */
        pfree( rdo );
        D(bug("\tRemoved objects\n"));
        SetFrameStatus( frame, 0 );
    } else {
        D(bug("\tNo render object!?!?\n"));
    }

    UNLOCK(frame);
}



/*
    This is the main rendering engine. It requires OpenRender()
    to be called already.

    Call this if you do not wish to have a separate render process. Just
    do a ObtainFrame() before it.
*/

PERROR Render( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;
    FRAME *source = rdo->frame;
    EXTBASE *PPTBase = rdo->PPTBase;
    BOOL hgrams = FALSE; /* Mark if we reserved the histograms ourselves. */
    D(APTR ppp);

    D(bug("Render()\n"));

    if( rdo == NULL )
        return PERR_INVALIDARGS;

    InitProgress( source, XGetStr(mINITIALIZING_RENDER), 0, source->pix->height, rdo->PPTBase );

    /*
     *  Initialize what is left to be initialized
     */

    if( (res=InitializeDither( rdo )) != PERR_OK) goto errorexit;
    if( (res=InitializePalette( rdo )) != PERR_OK) goto errorexit;
    if( (res=InitializeMode( rdo )) != PERR_OK) goto errorexit;

    /*
     *  Make sure the display is open.
     */

    if( (res = (*rdo->DispDeviceOpen)( rdo )) != PERR_OK) goto errorexit;

    /*
     *  We're ready, so begin by making the palette and loading it
     *  into the display device.
     */

    D(bug("===== INITIALIZATION =====\n"));
    D(ppp = StartBench());

    if( (res = (*rdo->Palette)( rdo )) != PERR_OK ) goto errorexit;
    if( (res = (*rdo->LoadCMap)( rdo, rdo->colortable )) != PERR_OK ) goto errorexit;

    D(StopBench(ppp));
    D(bug("===== INIT DONE =====\n"));

    /*
     *  Reset the histogram space, should someone want it to be used
     *  as pixel cache. If it has not been reserved, we'll do it now.
     */

    if(rdo->histograms) {
        bzero( rdo->histograms, rdo->hsize );
    } else {
        if(AllocHistograms( rdo ) != PERR_OK) goto errorexit;
        hgrams = TRUE;
    }

    D(DumpRenderObject(rdo));

    /*
     *  Loop through all lines in the frame and call each mapper
     *  in turn.
     */

    InitProgress( source, XGetStr(mRENDERING), 0, source->pix->height, rdo->PPTBase );

    D(bug("*** Initialization done, entering render loop\n"));

    D(ppp = StartBench());

    rdo->currentrow = 0;

    while( rdo->currentrow < source->pix->height ) {

        // D(bug("\tRendering row %d...\n", rdo->currentrow ));

        if( Progress( source, rdo->currentrow, rdo->PPTBase ) ) {
            res = PERR_BREAK;
            goto errorexit;
        }

        rdo->cp = GetPixelRow( source, rdo->currentrow, rdo->PPTBase );
        rdo->currentcolumn = 0; /* Just in case */

        (*rdo->Dither)( rdo );
        (*rdo->WriteLine)( rdo, rdo->currentrow );

        rdo->currentrow++;
    }

    D(StopBench(ppp));

    D(bug("*** Render done\n"));

    FinishProgress( source, rdo->PPTBase );

errorexit:
    /*
     *  Destroy the objects and on error, do clean up.
     */

    if(rdo->PaletteD) (*rdo->PaletteD)( rdo );
    if(rdo->DitherD)  (*rdo->DitherD)( rdo );

    if(rdo->histograms && hgrams) ReleaseHistograms( rdo );

    if( res != PERR_OK ) {
        CloseRender( source, PPTBase );
    }

    CloseInfoWindow( source->mywin, PPTBase );

    ClearError( source );

    return res;
}


/*
    This is the main entry point for the render task.

    If you wish, you can call Render() just like that, but in
    that case no task is spawned.
*/
SAVEDS ASM VOID RenderMain( REG(a0) UBYTE *argstr )
{
    EXTBASE *PPTBase;
    struct PPTMessage *msg;
    FRAME *frame = NULL;
    PERROR res = PERR_GENERAL;
    ULONG *optarray = NULL;
    struct RenderObject *rdo = NULL;

    D(bug("RenderMain(%s)\n",argstr));

    if( (PPTBase = NewExtBase(TRUE)) == NULL) {
        D(bug("LIB BASE ALLOCATION FAILED\n"));
        return; /* We hang off, deliberately, since we have nothing. */
    }

    /*
     *  Read possible REXX commands
     */

    if(optarray = ParseDOSArgs( argstr, "RENDEROBJECT/A/N", PPTBase ) ) {
        rdo = (struct RenderObject *) *( (ULONG *)optarray[0]) ;
    } else {
        InternalError( "LoadPicture(): REXX message of incorrect format" );
        goto errorexit;
    }

    rdo->PPTBase = PPTBase;
    frame = rdo->frame;

    if(NewTaskProlog(frame,PPTBase) != PERR_OK) goto errorexit;

    if(!CheckPtr(rdo,"RenderMain(): renderobject")) goto errorexit;
    if(!CheckPtr(frame,"RenderMain(): frame")) goto errorexit;

    res = Render( rdo );

errorexit:
    if(optarray)
        FreeDOSArgs( optarray, PPTBase );

    msg = AllocPPTMsg( sizeof( struct PPTMessage ), PPTBase );
    msg->frame = frame;
    msg->code = PPTMSG_RENDERDONE;
    msg->data = (APTR)res;

    /* Send the message */
    SendPPTMsg( globals->mport, msg, PPTBase );

    WaitDeathMessage( PPTBase );

    EmptyMsgPort( PPTBase->mport, PPTBase );

    if( PPTBase ) RelExtBase(PPTBase);
    rdo->PPTBase = globxd; /* Clear up, just in case */

    /* Die */
}

/*
    This takes care on spawning the render.
*/
PERROR DoRender( struct RenderObject *rdo )
{
    char argbuf[ARGBUF_SIZE];
    struct Process *p;
    FRAME *frame = rdo->frame;
    PERROR res = PERR_OK;

    D(bug("DoRender()\n"));

    if(!CheckPtr( frame, "DoRender(): frame"))
        return PERR_INVALIDARGS;

    if(ObtainFrame( frame, BUSY_RENDERING ) == FALSE)
        return PERR_INUSE;

    sprintf(argbuf,"%lu",rdo);

#ifdef DEBUG_MODE
    p = CreateNewProcTags( NP_Entry, RenderMain, NP_Cli, FALSE,
                           NP_Output, frame->debug_handle = OpenDebugFile( DFT_Render ),
                           NP_CloseOutput,TRUE, NP_Name, frame->nd.ln_Name,
                           NP_StackSize, globals->userprefs->extstacksize,
                           NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf, TAG_END );
#else
    p = CreateNewProcTags( NP_Entry, RenderMain, NP_Cli, FALSE,
                           NP_Output, Open("NIL:",MODE_NEWFILE),
                           NP_CloseOutput, TRUE, NP_Name, frame->nd.ln_Name,
                           NP_StackSize, globals->userprefs->extstacksize,
                           NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf, TAG_END );
#endif
    if(!p) {
        Req(NEGNUL,NULL,GetStr(MSG_PERR_NO_NEW_PROCESS) );
        ReleaseFrame( frame );
        res = PERR_INITFAILED;
    } else {
        frame->currproc = p;
        SetFrameStatus( frame, 1 );
    }

    return res;
}


PERROR InitializeColorSpace( struct RenderObject *rdo )
{
    return PERR_OK;
}

PERROR InitializeDither( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;

    D(bug("InitializeDither()\n"));

    switch( rdo->frame->disp->dither ) {
        case DITHER_NONE:
            res = Dither_NoneI( rdo ); /* Initialize */
            break;

        case DITHER_ORDERED:
            res = Dither_OrderedI( rdo );
            break;

        case DITHER_FLOYD:
            res = Dither_FSI( rdo ); /* Initialize */
            break;
    }
    return res;
}

PERROR InitializePalette( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;

    D(bug("InitializePalette()\n"));

    switch( rdo->frame->disp->cmap_method ) {
        case CMAP_MEDIANCUT:
            res = Palette_MedianCutI( rdo );
            break;

        case CMAP_POPULARITY:
            res = Palette_PopularityI( rdo );
            break;

        case CMAP_FORCEPALETTE:
            res = Palette_ForceI( rdo );
            break;

        case CMAP_FORCEOLDPALETTE:
            res = Palette_OldForceI( rdo );
            break;

        default:
            InternalError("Colormap type out of range");
            res = PERR_UNKNOWNTYPE;
            break;
    }
    return res;
}

/*
    Initializes the color fetch mode according to user
    preferences and image color space.
*/
PERROR InitializeMode( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;
    UBYTE renderq = rdo->frame->disp->renderq;
    UBYTE colorspace = rdo->frame->pix->colorspace;

    D(bug("InitializeMode()\n"));

    switch( renderq ) {
        case RENDER_QUICK:
            res = PERR_INITFAILED; /* BUG: should be something else */
            break;

        case RENDER_NORMAL:
            switch(colorspace) {
                case CS_RGB:
                case CS_ARGB:
                    rdo->GetColor = GetColor_Normal;
                    break;

                case CS_GRAYLEVEL:
                    rdo->GetColor = GetColor_NormalGray;
                    break;
            }
            break;

        case RENDER_HAM6:
            rdo->GetColor = GetColor_HAM;
            break;

        case RENDER_HAM8:
            rdo->GetColor = GetColor_HAM8;
            break;

        case RENDER_EHB:
            rdo->GetColor = GetColor_EHB;
            break;
    }

    return res;
}

/*
    BUG: Currently, only screen I/O is supported.
*/
PERROR InitializeDevice( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;

    D(bug("InitializeDevice()\n"));

    res = Device_BitMappedScreenI( rdo );

    return res;
}
