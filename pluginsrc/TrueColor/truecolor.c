/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : truecolor

    PPT and this file are (C) Janne Jalkanen 1998.

    $Id: truecolor.c,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "TrueColor"


/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables */

const char blurb[] =
    "Create a Truecolor image from a\n"
    "grayscale image.\n";

const struct TagItem MyTagArray[] = {
    PPTX_Name, (ULONG)MYNAME,
    /* Other tags go here */
    PPTX_Author, (ULONG)"Janne Jalkanen 1996-1999",
    PPTX_InfoTxt, (ULONG)blurb,
    PPTX_ColorSpaces, CSF_GRAYLEVEL|CSF_ARGB,
    PPTX_NoNewFrame, TRUE,
    PPTX_RexxTemplate, (ULONG)"",
    PPTX_SupportsGetArgs, TRUE,
    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
/* Code */

EFFECTINQUIRE(tag,PPTBase,EffectBase)
{
    return( TagData(tag,MyTagArray) );
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe;
    WORD row;

#ifdef DEBUG_MODE
    PDebug(MYNAME": Exec()\n");
#endif

    newframe = MakeFrame( frame );

    if(!newframe) {
        return NULL;
    }

    newframe->pix->components = 3;
    newframe->pix->colorspace = CS_RGB;
    newframe->pix->origdepth  = 24;

    if( InitFrame(newframe) != PERR_OK ) {
        RemFrame(newframe);
        return NULL;
    }


    InitProgress( frame, "Truecoloring...", 0, frame->pix->height );

    for(row = 0; row < frame->pix->height; row++) {
        ROWPTR tcp;
        RGBPixel  *dcp;
        GrayPixel *gcp;
        ARGBPixel *scp;
        WORD col;

        if(Progress( frame, row )) {
            RemFrame(newframe);
            newframe = NULL;
            break;
        }

        dcp = (RGBPixel *)GetPixelRow( newframe, row );
        tcp = (ROWPTR) dcp;

        if( frame->pix->colorspace == CS_GRAYLEVEL ) {
            gcp = (GrayPixel *)GetPixelRow( frame, row );
            for(col = 0; col < frame->pix->width; col++) {
                dcp->r = gcp->g;
                dcp->g = gcp->g;
                dcp->b = gcp->g;
                dcp++; gcp++;
            }
        } else { /* CS_ARGB */
            scp = (ARGBPixel *)GetPixelRow( frame, row );

            for(col = 0; col < frame->pix->width; col++) {
                dcp->r = scp->r;
                dcp->g = scp->g;
                dcp->b = scp->b;
                scp++;
                dcp++;
            }
        }

        PutPixelRow( newframe, row, tcp);
    }

    ClearProgress(frame);

    return newframe;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    return PERR_OK;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

