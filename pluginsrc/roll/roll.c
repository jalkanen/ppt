/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : roll

    PPT and this file are (C) Janne Jalkanen 1997-2000.

    $Id: roll.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $

*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <string.h>
#include <stdlib.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Roll"

struct Values {
    LONG xroll, yroll;
};

/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Rolls the image to any direction.";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name,  (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,    (ULONG)"Janne Jalkanen 1997-2000",
    PPTX_InfoTxt,   (ULONG)infoblurb,

    PPTX_ColorSpaces, ~0,

    PPTX_NoNewFrame, TRUE,

    PPTX_RexxTemplate, (ULONG)"X=XROLL/N,Y=YROLL/N",

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

struct Values v = {0};

LONG origwidth, origheight;


/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif


EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

FRAME *Roll( FRAME *src, struct Values *val, struct PPTBase *PPTBase )
{
    WORD row;
    FRAME *dest;
    int mult, len;
    LONG width;
    LONG xroll, yroll;

    xroll = val->xroll % src->pix->width;
    yroll = val->yroll % src->pix->height;

    mult    = src->pix->components * (src->pix->bits_per_component >> 3);
    len     = (src->pix->width - abs(xroll)) * mult;
    width   = src->pix->width * mult;

    if(dest = MakeFrame(src)) {

        if( InitFrame(dest) == PERR_OK ) {
            WORD newrow;

            /*
             *  Ready, go for the copying part
             */

            InitProgress( src, "Rolling...", 0, dest->pix->height );

            for( newrow = 0; newrow < dest->pix->height; newrow++ ) {
                ROWPTR scp, dcp;

                if( Progress( src, newrow ) ) {
                    SetErrorCode( src, PERR_BREAK );
                    break;
                }

                dcp = GetPixelRow( dest, newrow );

                row = (WORD)((LONG)newrow - yroll);

                if( row < 0 ) row += src->pix->height;
                if( row >= src->pix->height ) row -= src->pix->height;

                scp = GetPixelRow( src, row );

                /*
                 *  The length of the row to be copied.
                 */

                if( xroll < 0 ) {
                    /* Left */
                    memmove( dcp, (UBYTE *)( (ULONG)scp + abs(xroll)*mult), len );
                    memmove( dcp + len, scp, width-len );
                } else {
                    /* Right */
                    memmove( (UBYTE *)( (ULONG)dcp + abs(xroll)*mult), scp, len );
                    memmove( dcp, scp+len, width-len );
                }

                PutPixelRow( dest, newrow, dcp );
            }

            FinishProgress(src);
        } else {
            SetErrorCode( src, PERR_INITFAILED );
            RemFrame( dest );
            dest = NULL;
        }
    }

    return dest;
}

ASM ULONG MyHookFunc( REG(a0) struct Hook *hook,
                      REG(a2) Object *obj,
                      REG(a1) struct ARUpdateMsg *msg )
{
    struct Values foo;
    FRAME *tmp;
    struct PPTBase *PPTBase = msg->aum_PPTBase;

    /*
     *  Set up A4 so that globals still work
     */

    PUTREG(REG_A4, (long) hook->h_Data);

    foo.xroll = (LONG)msg->aum_Values[0] * msg->aum_Frame->pix->width / origwidth;
    foo.yroll = (LONG)msg->aum_Values[1] * msg->aum_Frame->pix->height / origheight;

    tmp = Roll( msg->aum_Frame, &foo, PPTBase );
    if( tmp ) {
        CopyFrameData( tmp, msg->aum_Frame, 0L );
        RemFrame( tmp );
    }

    return ARR_REDRAW;
}

struct Hook pwhook = { {0}, MyHookFunc, 0L, NULL };

struct TagItem xobj[] = {
    ARSLIDER_Min, -100,
    ARSLIDER_Max, 100,
    ARSLIDER_Default, 0,
    AROBJ_Value,  (ULONG)&v.xroll,
    AROBJ_Label,  (ULONG)"X:",
    AROBJ_PreviewHook, (ULONG)&pwhook,
    TAG_END,      0L
};

struct TagItem yobj[] = {
    ARSLIDER_Min, -100,
    ARSLIDER_Max, 100,
    ARSLIDER_Default, 0,
    AROBJ_Value,  (ULONG)&v.yroll,
    AROBJ_Label,  (ULONG)"Y:",
    AROBJ_PreviewHook, (ULONG)&pwhook,
    TAG_END,      0L
};

struct TagItem win[] = {
    AR_Text, (ULONG)"\nPlease select the roll amounts:\n",
    AR_SliderObject, (ULONG)xobj,
    AR_SliderObject, (ULONG)yobj,
    AR_HelpNode,     (ULONG)"effects.guide/Roll",
    // AR_RenderHook,   (ULONG)&pwhook,
    TAG_END,        0L
};

VOID ParseRxArgs( ULONG *args, struct Values * v )
{
    if( args[0] )
        v->xroll = (*(LONG *)args[0] );
    else
        v->xroll = 0;

    if( args[1] )
        v->yroll = (*(LONG *)args[1] );
    else
        v->yroll = 0;

}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe = NULL;
    ULONG *args;
    PERROR res = PERR_OK;
    struct Values *opt;

    pwhook.h_Data = (void *)GETREG(REG_A4); // Save register

    if( opt = GetOptions( MYNAME ) ) {
        v = *opt;
    }

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );
    if( args ) {
        ParseRxArgs( args, &v );
    } else {
        FRAME *pwframe;

        xobj[0].ti_Data = -frame->pix->width;
        xobj[1].ti_Data = origwidth = frame->pix->width;
        xobj[2].ti_Data = v.xroll;

        yobj[0].ti_Data = -frame->pix->height;
        yobj[1].ti_Data = origheight = frame->pix->height;
        yobj[2].ti_Data = v.yroll;

        pwframe = ObtainPreviewFrame( frame, NULL );

        if( (res = AskReqA( frame, win )) != PERR_OK ) {
            SetErrorCode( frame, res );
            if( pwframe ) ReleasePreviewFrame( pwframe );
            return NULL;
        }
        if( pwframe ) ReleasePreviewFrame( pwframe );
    }

    if(res == PERR_OK)
        newframe = Roll(frame, &v, PPTBase);

    PutOptions( MYNAME, &v, sizeof(v) );

    return newframe;
}


EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    PERROR res = PERR_OK;
    struct Values *opt;
    STRPTR buffer;

    pwhook.h_Data = (void *)GETREG(REG_A4); // Save register

    if( opt = GetOptions( MYNAME ) ) {
        v = *opt;
    }

    buffer = (STRPTR)TagData( PPTX_ArgBuffer, tags );
    args = (ULONG *)TagData( PPTX_RexxArgs, tags );

    if( args ) {
        ParseRxArgs( args, &v );
    }

    xobj[0].ti_Data = -MAX_WIDTH;
    xobj[1].ti_Data = origwidth = MAX_WIDTH;
    xobj[2].ti_Data = v.xroll;

    yobj[0].ti_Data = -MAX_HEIGHT;
    yobj[1].ti_Data = origheight = MAX_HEIGHT;
    yobj[2].ti_Data = v.yroll;

    if( (res = AskReqA( frame, win )) != PERR_OK ) {
        SetErrorCode( frame, res );
        return res;
    }

    if(res == PERR_OK)
    {
        SPrintF(buffer,"XROLL %d YROLL %d",
                       v.xroll, v.yroll );
    }

    PutOptions( MYNAME, &v, sizeof(v) );

    return res;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

