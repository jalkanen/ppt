/*
    PROJECT: PPT
    MODULE:  preview.c

    $Id: preview.c,v 4.5 1998/02/26 19:51:32 jj Exp $
*/

#include "defs.h"
#include "misc.h"

#define PPTMSG_PREVIEW_REDRAW   0x300

struct PreviewMessage {
    struct PPTMessage       msg;
    struct RastPort         *rp;
    struct IBox             area;
    WORD                    MouseX,MouseY;
    WORD                    FrameX,FrameY;
};


/*
 *  Returns TRUE if this frame has a preview frame attached,
 *  i.e. it is being processed and the module wishes to show the
 *  preview.
 */

Prototype BOOL HasPreview( FRAME *frame );

BOOL HasPreview( FRAME *frame )
{
    if( frame ) {
        if(frame->parent)
            return (BOOL) (frame->parent->preview.pf_Frame ? TRUE : FALSE);
        else
            return (BOOL) (frame->preview.pf_Frame ? TRUE : FALSE);
    } else {
        return FALSE;
    }
}

/*
 *  Returns TRUE if the frame that was called is actually a preview
 *  frame.
 */

Prototype BOOL IsPreview( FRAME *frame );

BOOL IsPreview( FRAME *frame )
{
    if( frame ) {
        return frame->preview.pf_IsPreview;
    } else {
        return FALSE;
    }
}

Prototype PERROR DoPreview( FRAME *frame );

PERROR DoPreview( FRAME *frame )
{
    PERROR res = PERR_OK;

    D(bug("DoPreview(%08X)\n",frame));

    if( frame->preview.pf_Hook ) {
        struct PreviewMessage   *pm;
        struct IBox             *bounds;

        /*
         *  Request a full redraw from the main object
         */

        pm = (struct PreviewMessage *)AllocPPTMsg( sizeof(struct PreviewMessage), globxd );

        pm->msg.code    = PPTMSG_PREVIEW_REDRAW;
        pm->msg.frame   = frame;
        pm->rp          = frame->disp->win->RPort;
        GetAttr( AREA_AreaBox, frame->disp->RenderArea, (ULONG *)&bounds);
        pm->area        = *bounds;

        SendPPTMsg( frame->selectport,pm,globxd );
    } else {
#if 0
        res = QuickDisplayFrame( frame->pf_Frame );
#endif
    }

    return res;
}

/*
    BUG: Redraws false stuff
 */

Prototype VOID RedrawPreview( FRAME *pwframe, EXTBASE *ExtBase );

VOID RedrawPreview( FRAME *pwframe, EXTBASE *ExtBase )
{
    D(bug("RedrawPreview( %08X, xb=%08X )\n",pwframe, ExtBase ));

    if( pwframe->preview.pf_Hook ) {
        CallHook( pwframe->preview.pf_Hook, (Object *)pwframe, ~0 );
    } else {
        /*
         *  Send message to the main window
         */
        struct IBox *ibox;

        GetAttr(AREA_AreaBox, pwframe->preview.pf_RenderArea, (ULONG *)&ibox );
        QuickRender( pwframe, pwframe->preview.pf_win->RPort,
                     ibox->Top, ibox->Left,
                     ibox->Height, ibox->Width, ExtBase );

#if 0
        DoMethod( pwframe->parent->disp->Win, WM_REPORT_ID, GID_DW_AREA,
                  WMRIF_TASK, globals->maintask, TAG_DONE );
#endif
    }
}

Prototype VOID RedrawPreviewRow( FRAME *pwframe, ULONG row, EXTBASE *ExtBase );

VOID RedrawPreviewRow( FRAME *pwframe, ULONG row, EXTBASE *ExtBase )
{
    D(bug("RedrawPreviewRow( %08X, xb=%08X )\n",pwframe, ExtBase ));

    if( pwframe->preview.pf_Hook ) {
        // BUG: Not the proper msg
        CallHook( pwframe->preview.pf_Hook, (Object *)pwframe, row );
    } else {
        struct IBox *ibox;

        GetAttr(AREA_AreaBox, pwframe->preview.pf_RenderArea, (ULONG *)&ibox );
        QuickRender( pwframe, pwframe->preview.pf_win->RPort,
                     ibox->Top, ibox->Left,
                     ibox->Height, ibox->Width, ExtBase );
#if 0
        /*
         *  Send message to the main window
         */
        DoMethod( pwframe->parent->disp->Win, WM_REPORT_ID, GID_DW_AREA,
                  WMRIF_TASK, globals->maintask, TAG_DONE );
#endif
    }
}

/*
    Frames must be of the same colorspace!
    Make sure frames have their sizes correctly.

    BUG: Does not check it.
    BUG: No error checks
    BUG: Does not really belong here
*/

SAVEDS ASM
PERROR ScaleFrame( REG(a0) FRAME *src, REG(a1) FRAME *dst,
                   REG(d0) ULONG opts,
                   REG(a6) EXTBASE *ExtBase )
{
    WORD row, col;
    WORD orow, ocol;
    WORD comps = src->pix->components;

    if(!CheckPtr(src,"ScaleFrame(): illegal src")) return PERR_ERROR;

    if(!CheckPtr(dst,"ScaleFrame(): illegal dst")) return PERR_ERROR;

    for( row = 0; row < dst->pix->height; row++ ) {
        ROWPTR cp, dcp;

        orow = row * src->pix->height / dst->pix->height;

        cp = GetPixelRow(src, orow, ExtBase);
        dcp = GetPixelRow(dst,row, ExtBase);

        if( !cp || !dcp ) {
            SetErrorCode( src, PERR_NOTINIMAGE );
            return PERR_NOTINIMAGE;
        }

        for( col = 0; col < dst->pix->width; col++ ) {
            int i;

            ocol = col * src->pix->width / dst->pix->width;

            for( i = 0; i < comps; i++ ) {
                dcp[comps*col+i] = cp[comps*ocol+i];
            }
        }

        PutPixelRow( dst, row, dcp, ExtBase );
    }

    return PERR_OK;
}

/****u* pptsupport/ObtainPreviewFrameA ******************************************
*
*   NAME
*       ObtainPreviewFrameA - Create a preview frame for an external.
*       ObtainPreviewFrame - varargs version.
*
*   SYNOPSIS
*       preview = ObtainPreviewFrameA( frame, tags );
*       D0                             A0     A1
*
*       FRAME *ObtainPreviewFrameA( FRAME *, struct TagItem * );
*
*   FUNCTION
*       Obtains a preview frame (a small version of the original image,
*       also known as a thumbnail).
*
*       The size of the thumbnail depends on the preferences set
*       by the user.
*
*   INPUTS
*       Available tags are:
*
*       PREV_PreviewHook (struct Hook *) - Specifying this tag means
*           that the external module is prepared to handle the entire
*           drawing of the preview by itself.  Don't do this unless
*           you really know what you're doing.
*
*   RESULT
*       preview - the preview frame.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       This entry incomplete.
*
*   SEE ALSO
*       ReleasePreviewFrame(), example code in externals/src/,
*       AskReqA()
*
*****************************************************************************
*
*
*    Creates a preview frame, when used, will be rendered in stead
*    of the actual frame.
*
*    Tags:
*        PREV_PreviewHook (HOOKPTR) - The external will completely take care
*            of rendering the imagery.
*/

Prototype ASM FRAME *ObtainPreviewFrameA( REG(a0) FRAME *,REG(a1) struct TagItem *,REG(a6) EXTBASE * );

SAVEDS ASM
FRAME *ObtainPreviewFrameA( REG(a0) FRAME *frame,
                            REG(a1) struct TagItem *tags,
                            REG(a6) EXTBASE *ExtBase )
{
    FRAME *pwframe, *tmpframe;
    struct Library *UtilityBase = ExtBase->lb_Utility;
    struct Rectangle sb;
    WORD pw = globals->userprefs->previewwidth,
         ph = globals->userprefs->previewheight;

    D(bug("ObtainPreviewFrameA( frame=%08X, tags=%08X )\n",frame,tags));

    if(!CheckPtr(frame,"ObtainPreviewFrame():frame"))
        return NULL;

    if( ph <= 0 || pw <= 0 || globals->userprefs->previewmode == PWMODE_OFF )
        return NULL; /* Previews are disabled or broken */

    if(pwframe = MakeFrame( frame->parent ? frame->parent : frame, ExtBase )) {

        /*
         *  Set up the preview frame to be a bit smaller than
         *  what we used to have.
         *
         *  Note that the frame->parent is the old frame and shall
         *  always be used.
         */

        pwframe->pix->width  = pw;
        if( frame->pix->DPIX && frame->pix->DPIY ) {
            pwframe->pix->height = (pwframe->pix->width * frame->pix->height * frame->pix->DPIY / (frame->pix->width * frame->pix->DPIX) );
        } else {
            pwframe->pix->height = (pwframe->pix->width * frame->pix->height / (frame->pix->width) );
        }
#if 0
        if( pwframe->pix->height < ph/2 ) {
            pwframe->pix->height = ph;
            pwframe->pix->width = (pwframe->pix->height * frame->pix->width / frame->pix->height );
        }
#endif

        D(bug("\tPreview frame is %dx%d\n",pwframe->pix->width,pwframe->pix->height ));

        /*
         *  Calculate the selbox size
         */

        if( frame->selbox.MinX != ~0 ) {
            sb.MinX = frame->selbox.MinX * pwframe->pix->width / frame->pix->width;
            sb.MinY = frame->selbox.MinY * pwframe->pix->height / frame->pix->height;
            sb.MaxX = frame->selbox.MaxX * pwframe->pix->width / frame->pix->width;
            sb.MaxY = frame->selbox.MaxY * pwframe->pix->height / frame->pix->height;
        } else {
            sb.MinX = sb.MinY = 0;
            sb.MaxX = frame->pix->width-1;
            sb.MaxY = frame->pix->height-1;
        }

        pwframe->preview.pf_IsPreview = TRUE;

        SetVMemMode( pwframe, VMEM_NEVER, ExtBase );

        if( InitFrame( pwframe, ExtBase ) == PERR_OK ) {
            if(ScaleFrame( frame, pwframe, 0L, ExtBase ) == PERR_OK ) {
                struct Hook *hook;
                struct PPTMessage *pmsg;

                /*
                 *  Make a copy to the tmpframe
                 */

                if(tmpframe = DupFrame( pwframe, DFF_COPYDATA, ExtBase )) {

                    /*
                     *  Set up the frame for PW frame
                     */

                    LOCK(frame);

                    ChangeBusyStatus(pwframe,BUSY_READONLY);
                    pwframe->selbox = tmpframe->selbox = sb;

                    if( hook = (struct Hook *)GetTagData( PREV_PreviewHook, NULL, tags ) ) {
                        pwframe->preview.pf_Hook = hook;
                    }

                    if( frame->parent ) {
                        LOCK(frame->parent);
                        frame->parent->preview.pf_Frame = pwframe;
                        frame->parent->preview.pf_TempFrame = tmpframe;
                        UNLOCK(frame->parent);
                    } else {
                        frame->preview.pf_Frame = pwframe;
                        frame->preview.pf_TempFrame = tmpframe;
                    }

                    UNLOCK(frame);

                    pmsg        = AllocPPTMsg( sizeof(struct PPTMessage), ExtBase );
                    pmsg->code  = PPTMSG_START_PREVIEW;
                    if( frame->parent )
                        pmsg->frame = frame->parent;
                    else
                        pmsg->frame = frame;

                    SendPPTMsg( globxd->mport, pmsg, ExtBase );

                    WaitForReply( PPTMSG_START_PREVIEW, ExtBase );
                } else {
                    RemFrame( pwframe, ExtBase );
                    pwframe = NULL;
                }
            } else {
                RemFrame(pwframe, ExtBase);
                pwframe = NULL;
            }
        } else {
            RemFrame(pwframe, ExtBase);
            pwframe = NULL;
        }
    }

    D(bug("\tReturning preview frame at %08X\n",pwframe));

    return pwframe;
}


/****u* pptsupport/ReleasePreviewFrame ******************************************
*
*   NAME   
*       ReleasePreviewFrame - releases a preview frame.
*
*   SYNOPSIS
*       ReleasePreviewFrame( preview );
*                            A0
*
*       VOID ReleasePreviewFrame( FRAME * );
*
*   FUNCTION
*       Releases the small image acquired by ObtainPreviewFrame().
*
*   INPUTS
*       preview - The preview frame.
*
*   RESULT
*       N/A
*
*   EXAMPLE
*
*   NOTES
*       The frame is freed completely after this call, so you'd better
*       not access the memory in any way.
*
*   BUGS
*
*   SEE ALSO
*       ObtainPreviewFrameA()
*
*****************************************************************************
*
*
*    Release preview frame obtained by ObtainPreviewFrame()
*/
Prototype ASM VOID ReleasePreviewFrame( REG(a0) FRAME *, REG(a6) EXTBASE *);

SAVEDS ASM
VOID ReleasePreviewFrame( REG(a0) FRAME *pwframe,
                          REG(a6) EXTBASE *ExtBase )
{
    FRAME *frame;
    struct PPTMessage *pmsg;

    D(bug("ReleasePreviewFrame( %08X )\n",pwframe));

    if( !CheckPtr(pwframe,"ReleasePreviewFrame(): pwframe")) return;

    frame = pwframe->parent;

    if( !CheckPtr(frame,"ReleasePreviewFrame(): parent frame")) return;

    pmsg         = AllocPPTMsg( sizeof(struct PPTMessage), ExtBase );
    pmsg->code   = PPTMSG_STOP_PREVIEW;
    if( frame->parent )
        pmsg->frame  = frame->parent;
    else
        pmsg->frame  = frame;

    SendPPTMsg( globxd->mport, pmsg, ExtBase );

    WaitForReply( PPTMSG_STOP_PREVIEW, ExtBase );

    LOCK(pwframe);
    RemFrame( pwframe, ExtBase );
    LOCK(frame->preview.pf_TempFrame);
    RemFrame( frame->preview.pf_TempFrame, ExtBase );

    LOCK(frame);
    frame->preview.pf_Hook  = NULL;
    frame->preview.pf_Frame = NULL;
    frame->preview.pf_TempFrame = NULL;
    frame->preview.pf_IsPreview    = FALSE;
    UNLOCK(frame);
}

/****u* pptsupport/RenderFrame ******************************************
*
*   NAME
*       RenderFrame - renders a frame to the specified rastport. (V4)
*
*   SYNOPSIS
*       error = RenderFrame( frame, rport, location, flags );
*       D0                   A0     A1     A2        D0
*
*       PERROR RenderFrame( FRAME *, struct RastPort *, struct IBox *, ULONG );
*
*   FUNCTION
*       Use this function to render the frame into a RastPort.  PPT will
*       automatically use the correct coloring routine.
*
*       This is the internal previewing routine, so the results may not be
*       very accurate.  Also, the current previewing preferences may
*       change the result.
*
*   INPUTS
*       frame - the usual.
*       rport - the RastPort of the window you want the frame to be rendered
*           into.
*       location - the location and size of the image.
*       flags - reserved for future use.
*
*   RESULT
*       Standard error code.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       Does not do very extensive error checking at the moment.
*
*   SEE ALSO
*
*****************************************************************************
*
*   A simple interface to the QuickRender() routines.
*   BUG: Should do a bit more error checking.
*/

Prototype ASM PERROR RenderFrame( REG(a0) FRAME *, REG(a1) struct RastPort *, REG(a2) struct IBox *, REG(d0) ULONG, REG(a6) EXTBASE * );

SAVEDS ASM
PERROR RenderFrame( REG(a0) FRAME *frame,
                    REG(a1) struct RastPort *rport,
                    REG(a2) struct IBox *loc,
                    REG(d0) ULONG flags,
                    REG(a6) EXTBASE *ExtBase )
{
    D(bug("QuickRender( %08X, rp=%08X, Top=%d, Left=%d, Ht=%d, Wt=%d )\n",
           frame, rport, loc->Top, loc->Left, loc->Height, loc->Width ));
    return QuickRender( frame, rport, loc->Top, loc->Left, loc->Height, loc->Width, ExtBase );
}

/*
 *  Currently these two just lock the main window, nothing else.
 *
 *  Actually, they don't even do that, thanks to the small
 *  window previews we use, since locking the windows will
 *  cause severe problems with externals that expect to get
 *  messages from the windows...
 */

Prototype VOID SetupFrameForPreview( struct PPTMessage *pmsg );

VOID SetupFrameForPreview( struct PPTMessage *pmsg )
{
    FRAME *frame = pmsg->frame;

    LOCK(frame);

    RemoveSelectBox( frame );
    // WindowBusy( frame->disp->Win );

    UNLOCK(frame);

    pmsg->data = PERR_OK; /* Signal: successful */
}

Prototype VOID FinishFramePreview( struct PPTMessage *pmsg );

VOID FinishFramePreview( struct PPTMessage *pmsg )
{
    FRAME *frame = pmsg->frame;

    LOCK(frame);

    // WindowReady( frame->disp->Win );

    UNLOCK(frame);

    pmsg->data = PERR_OK;
}


