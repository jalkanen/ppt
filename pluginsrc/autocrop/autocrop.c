/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : autocrop

    PPT is (C) Janne Jalkanen 1995-1996.

    $Id: autocrop.c,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <exec/memory.h>
#include <proto/graphics.h>
#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "AutoCrop"


/*----------------------------------------------------------------------*/
/* Internal prototypes */

static PERROR BuildBorders( FRAME *frame, WORD *left, WORD *right, struct PPTBase *PPTBase );
ASM ULONG MyHookFunc( REG(a0) struct Hook *, REG(a2) Object *, REG(a1) struct ARUpdateMsg * );

/*----------------------------------------------------------------------*/
/* Global variables */

const char infoblurb[] =
    "Automatically crops away the border\n"
    "of the image.";

const struct TagItem MyTagArray[] = {
    PPTX_Name,          (ULONG)MYNAME,
    /* Other tags go here */
    PPTX_Author,        (ULONG)"Janne Jalkanen, 1996-1998",
    PPTX_InfoTxt,       (ULONG)infoblurb,
    PPTX_NoNewFrame,    TRUE,
    PPTX_ColorSpaces,   ~0,
    PPTX_RexxTemplate,  (ULONG)"XMARGIN=X/N,YMARGIN=Y/N",
    PPTX_ReqPPTVersion, 4,

    PPTX_SupportsGetArgs, TRUE,
    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
/* Code */

struct Values {
    LONG savedx, savedy;
};

struct PreviewData {
    LONG    a4;
    FRAME   *original;
    FRAME   *preview;
};

LONG  xborder, yborder;
WORD  *leftborder = NULL, *rightborder = NULL;
ROWPTR backgroundrow;

struct Hook pwhook = { {0}, MyHookFunc, 0L, NULL };

#pragma msg 186 ignore

struct TagItem xb[] = {
    ARSLIDER_Default, 0,
    AROBJ_Value,    &xborder,
    AROBJ_Label,    "X margin",
    ARSLIDER_Min,   0,
    ARSLIDER_Max,   100,
    AROBJ_PreviewHook, (ULONG)&pwhook,
    TAG_DONE
};

struct TagItem yb[] = {
    ARSLIDER_Default, 0,
    AROBJ_Value,    &yborder,
    AROBJ_Label,    "Y margin",
    ARSLIDER_Min,   0,
    ARSLIDER_Max,   100,
    AROBJ_PreviewHook, (ULONG)&pwhook,
    TAG_DONE
};

struct TagItem win[] = {
    AR_RenderHook,   (ULONG)&pwhook,
    AR_SliderObject, &xb,
    AR_SliderObject, &yb,
    AR_Text,        "Set the margins, please!",
    AR_HelpNode,    "effects.guide/AutoCrop",
    TAG_DONE
};

#pragma msg 186 warn

EFFECTINQUIRE( attr, PPTBase, EffectBase )
{
    return (TagData(attr,MyTagArray));
}

/*
 *  The previewframe has already been cropped
 */

ASM ULONG MyHookFunc( REG(a0) struct Hook *hook,
                      REG(a2) Object *obj,
                      REG(a1) struct ARUpdateMsg *msg )
{
    int xb, yb;
    struct PreviewData *pd = hook->h_Data;
    struct IBox area = msg->aum_Area;
    struct PPTBase *PPTBase = msg->aum_PPTBase;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;

    PUTREG(REG_A4, pd->a4 ); // Restore globals

    SetAPen( msg->aum_RPort, 1 ); // BUG: Assumes black = 1
    RectFill( msg->aum_RPort, area.Left, area.Top, area.Left+area.Width, area.Top+area.Height );

    xb = msg->aum_Values[0] * pd->preview->pix->width / pd->original->pix->width;
    yb = msg->aum_Values[1] * pd->preview->pix->height / pd->original->pix->height;

    area.Top    += yb;
    area.Height -= yb*2;
    area.Left   += xb;
    area.Width  -= xb*2;

    RenderFrame( pd->preview, msg->aum_RPort, &area, 0L );

    return ARR_DONE;
}

VOID GetBackground(FRAME *frame, ROWPTR buf, struct PPTBase *PPTBase )
{
    ROWPTR cp;

    cp = GetPixelRow( frame, 0 );
    bcopy( cp, buf, frame->pix->components );
}

FRAME *DoModify( FRAME *frame, LONG xb, LONG yb, struct PPTBase *PPTBase )
{
    FRAME *newframe = NULL;
    BOOL  errorp = FALSE;
    WORD  height = frame->pix->height;
    int   comps = frame->pix->components * (frame->pix->bits_per_component>>3);
    int i;
    WORD width = frame->pix->width;

    /*
     *  We'll locate the background color here.
     */

    GetBackground( frame, &backgroundrow[0], PPTBase );

    for( i = 1; i < width; i++ ) {
        bcopy( &backgroundrow[0], &backgroundrow[i*comps], comps);
    }

    if( BuildBorders( frame, leftborder, rightborder, PPTBase ) == PERR_OK ) {

        /*
         *  Now, we will find the limits.
         */

        WORD top = 0, bottom = 0, left = width, right = 0;
        WORD newwidth, newheight;

        /*
         *  Top will be the first line to contain non-border data.
         */

        for( i = 0; i < height; i++ ) {
            if( leftborder[i] != width )
                break;
        }

        top = i;

        /*
         *  Bottom. As top, the first line to contain non-border data.
         */

        for( i = height-1; i >= 0; i-- ) {
            if( leftborder[i] != width ) break;
        }

        bottom = i;

        /*
         *  Locate Left & Right borders
         */

        for( i = top; i <= bottom; i++ ) {
            if( leftborder[i] < left ) left = leftborder[i];
            if( rightborder[i] > right) right = rightborder[i];
        }

        // PDebug("Top: %d, Bottom: %d, Left: %d, Right: %d\n",top,bottom,left,right);


        newwidth = right - left + 1 + 2*xb;
        newheight = bottom - top + 1 + 2*yb;

        newframe = MakeFrame( frame );

        if(newframe) {
            newframe->pix->height = newheight;
            newframe->pix->width  = newwidth;

            if(InitFrame( newframe ) == PERR_OK ) {
                WORD drow,srow;

                /*
                 *  Copy the data.
                 */
                // PDebug("Copying data...\n");

                InitProgress( frame,"Copying data...",top,bottom);

                drow = 0;

                for( srow = top-yb; srow <= bottom+yb; srow++ ) {
                    ROWPTR scp;
                    ROWPTR dcp;

                    if(Progress(frame,srow)) {
                        errorp = TRUE;
                        break;
                    }

                    if( srow < 0 ) {
                        scp = backgroundrow;
                    } else if( srow >= frame->pix->height ) {
                        scp = backgroundrow;
                    } else {
                        scp = GetPixelRow( frame, srow );
                    }

                    dcp = GetPixelRow( newframe, drow );

                    if(!scp || !dcp) {
                        // PDebug("ERROR: scp or dcp out of line!\n");
                        errorp = TRUE;
                        SetErrorCode( frame, PERR_INVALIDARGS );
                        break;
                    }

                    /*
                     *  Set up x border
                     */

                    for( i = 0; i < xb; i++ ) {
                        bcopy( backgroundrow, dcp+(i*comps), comps );
                        bcopy( backgroundrow, dcp+(newwidth - i - 1)*comps, comps );
                    }

                    bcopy( &scp[left * comps], dcp+xb*comps, (newwidth-2*xb) * comps );

                    // PDebug("bcopy( src=%08X, dest=%08X, size=%lu\n",
                    //         &scp[left*comps],dcp,newwidth*comps );

                    PutPixelRow( newframe, drow, dcp );

                    drow++;
                }
            } else {
                SetErrorCode( frame, PERR_INITFAILED );
                errorp = TRUE;
            }
        }
    }

    if( errorp ) {
        if( newframe ) {
            RemFrame( newframe );
            newframe = NULL;
        }
    }

    return newframe;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    FRAME *newframe = NULL, *pwframe = NULL;
    WORD  height = frame->pix->height;
    ULONG *args;
    struct Values v = {0}, *opt;
    struct PreviewData pd = {0};

    pd.a4       = GETREG(REG_A4);
    pd.original = frame;

    pwhook.h_Data = &pd;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    xb[0].ti_Data = v.savedx;
    yb[0].ti_Data = v.savedy;

    /*
     *  Allocate and generate tables describing the image borders
     */

    leftborder  = AllocVec(height * sizeof(WORD),MEMF_CLEAR);
    rightborder = AllocVec(height * sizeof(WORD),MEMF_CLEAR);

    backgroundrow = AllocVec( frame->pix->bytes_per_row, MEMF_CLEAR );

    if( leftborder && rightborder && backgroundrow ) {

        /*
         *  Ask about the image size
         */

        if( args = (ULONG *)TagData(PPTX_RexxArgs, tags ) ) {
            if( args[0] )
                xborder = *( (LONG *)args[0]);
            if( args[1] )
                yborder = *( (LONG *)args[1]);
        } else {
            pwframe = ObtainPreviewFrame( frame, NULL );

            /*
             *  We'll pre-calculate the preview frame, so that we don't
             *  have to do it *every* time.
             *  This will result into a pretty crappy image, but
             *  it should do.
             */

            pd.preview = DoModify( pwframe, 0, 0, PPTBase );

            if( AskReqA(frame,win) != PERR_OK ) {
                if( pwframe ) ReleasePreviewFrame( pwframe );
                if( pd.preview ) RemFrame( pd.preview );
                return NULL;
            }
        }

        newframe = DoModify( frame, xborder, yborder, PPTBase );

    } else {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
    }

    /*
     *  Clean up.
     */

    PutOptions( MYNAME, &v, sizeof(struct Values) );

    if(backgroundrow) FreeVec(backgroundrow);
    if(leftborder) FreeVec(leftborder);
    if(rightborder) FreeVec(rightborder);

    if( pwframe ) ReleasePreviewFrame( pwframe );
    if( pd.preview ) RemFrame( pd.preview );
    return newframe;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    struct Values v = {0}, *opt;
    ULONG *args;
    STRPTR buffer;
    PERROR res;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    xb[0].ti_Data = v.savedx;
    yb[0].ti_Data = v.savedy;

    if( args = (ULONG *)TagData(PPTX_RexxArgs, tags ) ) {
        if( args[0] )
            xborder = *( (LONG *)args[0]);
        if( args[1] )
            yborder = *( (LONG *)args[1]);
    }

    buffer = (STRPTR) TagData(PPTX_ArgBuffer, tags);

    /* GetArgs does not use the preview hook, we just want the values. */

    win[0].ti_Tag = TAG_IGNORE;

    if( (res = AskReqA(frame,win)) == PERR_OK ) {
        SPrintF( buffer, "XMARGIN %d YMARGIN %d", xborder, yborder );
    }

    return res;
}



/*
    Assumes initialized borders.
*/
static
PERROR BuildBorders( FRAME *frame, WORD *left, WORD *right, struct PPTBase *PPTBase )
{
    WORD row;
    UBYTE pixel[10]; /* Room for 10 components */
    ROWPTR cp;
    UWORD comps = (UWORD) frame->pix->components;
    int i;
    ULONG width;

    width = frame->pix->width * comps;

    /*
     *  Copy the reference pixel to safety. BUG: Does not check bounds.
     */

    InitProgress( frame, "Analyzing image...", 0, frame->pix->height );

    cp = GetPixelRow( frame, 0 );

    for(i = 0; i < comps; i++ ) {
        pixel[i] = cp[i];
    }

    for( row = 0; row < frame->pix->height; row++ ) {
        ULONG col;

        if(Progress( frame, row ))
            return PERR_BREAK;

        cp = GetPixelRow( frame, row );

        /*
         *  Approach from left... col is left to point at
         *  the first pixel which is NOT in border.
         */

        for( col = 0; col < width; col += comps ) {

            for(i = 0; i < comps; i++ ) {
                if( cp[col+i] != pixel[i] )
                    goto exit1;
            }

        }
exit1:
        left[row] = (WORD)(col / comps);

        /*
         *  Approach from right. If we already reached right border,
         *  then we don't need to make this.
         */

        if( col < width ) {
            for( col = width-comps; col >= 0; col -= comps ) {
                for(i = 0; i < comps; i++ ) {
                    if( cp[col+i] != pixel[i] )
                        goto exit2;
                }
            }
exit2:
            right[row] = (WORD)(col / comps);

        } else {
            right[row] = 0;
        }

    }

    FinishProgress( frame );

    return PERR_OK;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

