/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : shift

    PPT and this file are (C) Janne Jalkanen 1996-1998

    $Id: shift.c,v 1.1 2001/10/25 16:23:03 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Shift"

struct Values {
    LONG xshift, yshift;
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
    "Shifts image to any direction,\n"
    "filling left-over areas with black\n";

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

    PPTX_Author,    (ULONG)"Janne Jalkanen 1996-2000",
    PPTX_InfoTxt,   (ULONG)infoblurb,

    PPTX_ColorSpaces, ~0,

    PPTX_NoNewFrame, TRUE,
    PPTX_ReqPPTVersion, 4,

    PPTX_RexxTemplate, (ULONG)"X=XSHIFT/N,Y=YSHIFT/N",

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

struct Values v;
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

BOOL DoModify( FRAME *src, FRAME *dest, struct Values *v, struct PPTBase *PPTBase )
{
    int mult,len;
    LONG width;
    WORD row,newrow;

    mult = src->pix->components * (src->pix->bits_per_component >> 3);
    len = (src->pix->width - abs(v->xshift)) * mult;
    width = src->pix->width * mult;

    for( newrow = 0; newrow < dest->pix->height; newrow++ ) {
        ROWPTR scp, dcp;

        if( Progress( src, newrow ) ) {
            SetErrorCode( src, PERR_BREAK );
            return FALSE;
        }

        dcp = GetPixelRow( dest, newrow );

        row = (WORD)((LONG)newrow - v->yshift);

        /*
         *  Send empty row if we're out of bounds
         */

        if( row < 0 || row >= src->pix->height ) {
            memset( dcp, 0, width );
        } else {
            scp = GetPixelRow( src, row );

            /*
             *  The length of the row to be copied.
             */

            if( v->xshift < 0 ) {
                /* Left */
                memmove( dcp, (UBYTE *)( (ULONG)scp + abs(v->xshift)*mult), len );
                memset( dcp + len, 0, width-len );
            } else {
                /* Right */
                memmove( (UBYTE *)( (ULONG)dcp + abs(v->xshift)*mult), scp, len );
                memset( dcp, 0, width-len );
            }
        }

        PutPixelRow( dest, newrow, dcp );
    }

    return TRUE;
}

FRAME *Shift( FRAME *src, struct Values *v, struct PPTBase *PPTBase )
{
    FRAME *dest;

    if(dest = MakeFrame(src)) {

        if( InitFrame(dest) == PERR_OK ) {

            /*
             *  Ready, go for the copying part
             */

            InitProgress( src, "Shifting...", 0, dest->pix->height );

            if( !DoModify( src, dest, v, PPTBase ) ) {
                RemFrame(dest);
                return NULL;
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

    foo.xshift = (LONG)msg->aum_Values[0] * msg->aum_Frame->pix->width / origwidth;
    foo.yshift = (LONG)msg->aum_Values[1] * msg->aum_Frame->pix->height / origheight;

    if( tmp = MakeFrame( msg->aum_Frame ) ) {
        if( InitFrame( tmp ) == PERR_OK ) {
            DoModify( msg->aum_Frame, tmp, &foo, PPTBase );
            RenderFrame( tmp, msg->aum_RPort, &msg->aum_Area, 0L );
        }
        RemFrame( tmp );
    }

    return ARR_DONE;
}

struct Hook pwhook = { {0}, MyHookFunc, 0L, NULL };

struct TagItem xobj[] = {
    ARSLIDER_Min, -MAX_WIDTH,
    ARSLIDER_Max, MAX_WIDTH,
    ARSLIDER_Default, 0,
    AROBJ_Value,  (ULONG)&v.xshift,
    AROBJ_Label,  (ULONG)"X:",
    AROBJ_PreviewHook, (ULONG)&pwhook,
    TAG_END,      0L
};

struct TagItem yobj[] = {
    ARSLIDER_Min, -MAX_HEIGHT,
    ARSLIDER_Max, MAX_HEIGHT,
    ARSLIDER_Default, 0,
    AROBJ_Value,  (ULONG)&v.yshift,
    AROBJ_Label,  (ULONG)"Y:",
    AROBJ_PreviewHook, (ULONG)&pwhook,
    TAG_END,      0L
};

struct TagItem win[] = {
    AR_Text, (ULONG)"\nPlease enter the shifting amounts:\n",
    AR_SliderObject, (ULONG)xobj,
    AR_SliderObject, (ULONG)yobj,
    AR_HelpNode,     (ULONG)"effects.guide/Shift",
    AR_RenderHook,   (ULONG)&pwhook,
    TAG_END,        0L
};

VOID ParseRxArgs(ULONG *args, struct Values *v)
{
    if( args ) {
        if( args[0] )
            v->xshift = (*(LONG *)args[0] );
        else
            v->xshift = 0;

        if( args[1] )
            v->yshift = (*(LONG *)args[1] );
        else
            v->yshift = 0;
    }
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe = NULL, *pwframe;
    ULONG *args;
    PERROR res = PERR_OK;
    struct Values *opt;

    pwhook.h_Data = (void *)GETREG(REG_A4); // Save register

    if( opt = GetOptions(MYNAME) ) {
        v = *opt;
    }

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );

    if( args ) {
        ParseRxArgs( args, &v );
    } else {
        xobj[0].ti_Data = -frame->pix->width;
        xobj[1].ti_Data = origwidth = frame->pix->width;
        xobj[2].ti_Data = v.xshift;

        yobj[0].ti_Data = -frame->pix->height;
        yobj[1].ti_Data = origheight = frame->pix->height;
        yobj[2].ti_Data = v.yshift;

        pwframe = ObtainPreviewFrame( frame, NULL );

        if( (res = AskReqA( frame, win )) != PERR_OK ) {
            SetErrorCode( frame, res );
            if( pwframe ) ReleasePreviewFrame( pwframe );
            return NULL;
        }
        if( pwframe ) ReleasePreviewFrame( pwframe );
    }

    if(res == PERR_OK)
        newframe = Shift(frame, &v, PPTBase);

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

    if( opt = GetOptions(MYNAME) ) {
        v = *opt;
    }

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );
    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args ) {
        ParseRxArgs( args, &v );
    }

    xobj[0].ti_Data = -MAX_WIDTH;
    xobj[1].ti_Data = origwidth = MAX_WIDTH;
    xobj[2].ti_Data = v.xshift;

    yobj[0].ti_Data = -MAX_HEIGHT;
    yobj[1].ti_Data = origheight = MAX_HEIGHT;
    yobj[2].ti_Data = v.yshift;

    if( (res = AskReqA( frame, win )) != PERR_OK ) {
        SetErrorCode( frame, res );
        return res;
    }

    if(res == PERR_OK) {
        SPrintF(buffer,"XSHIFT %d YSHIFT %d",
                       v.xshift, v.yshift );
    }

    PutOptions( MYNAME, &v, sizeof(v) );

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

