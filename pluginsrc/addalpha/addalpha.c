/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : AddAlpha

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    Adds an alpha channel to an image.

    $Id: addalpha.c,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "AddAlpha"


typedef enum {
    USE_ALPHA,
    USE_INTENSITY
} Method_T;

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
    "This effect can be used to add an alpha channel\n"
    "to the image.  The alpha channel should be a grayscale\n"
    "image, but if the alpha is an RGB image, the existing\n"
    "alpha channel or the intensity can be used.";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name,          (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,        (ULONG)"Janne Jalkanen 1997-1998",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"ALPHA/A/N",

    PPTX_ColorSpaces,   CSF_RGB|CSF_ARGB,

    PPTX_NoNewFrame,    TRUE,

    TAG_END, 0L
};


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

/*
    This assumes that

    a) frame is already ARGB and allocated
    b) alpha is already correct size
    c) frame and orig are the same size
*/

#define RGB(x) ((RGBPixel *)(x))
#define ARGB(x) ((ARGBPixel *)(x))


PERROR AddAlpha( FRAME *orig, FRAME *frame, FRAME *alpha, Method_T method, struct PPTBase *PPTBase )
{
    WORD row, col;

    InitProgress( frame, "Adding alpha...", 0, frame->pix->height );

    for( row = 0; row < frame->pix->height; row++ ) {
        ARGBPixel   *cp;
        ROWPTR      acp, ocp;
        UBYTE       aspace = alpha->pix->colorspace;
        WORD        arow, acol;

        if( Progress( frame, row ) ) {
            return PERR_BREAK;
        }

        arow = row % alpha->pix->height;

        cp = (ARGBPixel *)GetPixelRow( frame, row );
        ocp= GetPixelRow( orig, row );
        acp= GetPixelRow( alpha, arow );

        for( col = 0; col < frame->pix->width; col++ ) {
            UBYTE a;

            acol = col % frame->pix->width;

            switch( method ) {
                case USE_ALPHA:
                    a = ARGB(acp)[acol].a;
                    break;
                case USE_INTENSITY:
                    switch( aspace ) {
                        case CS_GRAYLEVEL:
                            a = acp[acol];
                            break;
                        case CS_RGB:
                            a = (RGB(acp)[acol].r + RGB(acp)[acol].g + RGB(acp)[acol].b)/3;
                            break;
                        case CS_ARGB:
                            a = (ARGB(acp)[acol].r + ARGB(acp)[acol].g + ARGB(acp)[acol].b)/3;
                            break;
                    }
                    break;
            }

            if( orig->pix->colorspace == CS_ARGB ) {
                cp[col] = ARGB(ocp)[col];
            } else {
                cp[col].r = RGB(ocp)[col].r;
                cp[col].g = RGB(ocp)[col].g;
                cp[col].b = RGB(ocp)[col].b;
            }

            cp[col].a = a;
        }

        PutPixelRow( frame, row, cp );
    }

    FinishProgress( frame );

    return PERR_OK;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe = NULL;
    ULONG *args;
    FRAME *alpha = NULL;
    ID    alphaid;
    Method_T method = USE_INTENSITY;

    /*
     *  Fetch rexx arguments
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        alphaid = *((ULONG *) args[0]);

        if(!(alpha = FindFrame( alphaid ))) {
            SetErrorMsg(frame, "Unknown frame ID.");
            return NULL;
        }
    } else {
        SetErrorMsg( frame, "This effect can only be started by Drag&Drop or AREXX" );
        return NULL;
    }

    /*
     *  Check the possible combinations.
     */

    if( alpha->pix->colorspace == CS_ARGB ) {
        ULONG answer;

        answer = AskReq( frame,
                       AR_Text, "\nThe source image you have specified already\n"
                                "has an alpha channel.  Would you like to copy\n"
                                "this alpha channel to the destination image?\n",
                       AR_Positive, "Yes",
                       AR_Negative, "No",
                       TAG_DONE);

        if( answer == PERR_OK )
            method = USE_ALPHA;
    }


    /*
     *  Do the adding
     */

    if(newframe = MakeFrame( frame )) {
        newframe->pix->colorspace = CS_ARGB;
        newframe->pix->components = 4;

        if( InitFrame( newframe ) == PERR_OK ) {
            if(AddAlpha( frame, newframe, alpha, method, PPTBase ) != PERR_OK ) {
                /* Failed */

                RemFrame( newframe );
                newframe = NULL;
            }
        }
    }

    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

