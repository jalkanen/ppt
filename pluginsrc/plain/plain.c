/*
    Plain RGB loader

    $Id: plain.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/

#include <pptplugin.h>
#include "plain.h"
#include <stdio.h>

/*------------------------------------------------------------------*/

#define MYNAME          "Plain"

/*------------------------------------------------------------------*/

const char infoblurb[] = "Use to create even color surfaces\n";

#pragma msg 186 ignore

const struct TagItem MyTagArray[] = {
    PPTX_Load,          TRUE,
    PPTX_NoFile,        TRUE,
    PPTX_ColorSpaces,   CSF_NONE,
    PPTX_Name,          MYNAME,
    PPTX_Author,        "Janne Jalkanen, 1995-1998",
    PPTX_InfoTxt,       infoblurb,
    PPTX_ReqPPTVersion, 3,
    TAG_DONE
};

/*------------------------------------------------------------------*/


#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif


IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

ULONG r=0,g=0,b=0;
ULONG height, width;


struct TagItem heightslider[] = {
    AROBJ_Label, "Height",
    AROBJ_Value, &height,
    ARSLIDER_Min, 10L,
    ARSLIDER_Max, 2048L,
    ARSLIDER_Default, 200L,
    TAG_END, 0L
};


struct TagItem widthslider[] = {
    AROBJ_Label, "Width",
    AROBJ_Value, &width,
    ARSLIDER_Min, 10L,
    ARSLIDER_Max, 2048L,
    ARSLIDER_Default, 320L,
    TAG_END, 0L };

struct TagItem redslider[] = {
    AROBJ_Label, "Red",
    AROBJ_Value, &r,
    ARSLIDER_Min, 0L,
    ARSLIDER_Max, 255L,
    ARSLIDER_Default, 0L,
    TAG_END, 0L };

struct TagItem greenslider[] = {
    AROBJ_Label, "Green",
    AROBJ_Value, &g,
    ARSLIDER_Min, 0L,
    ARSLIDER_Max, 255L,
    ARSLIDER_Default, 0L,
    TAG_END, 0L };

struct TagItem blueslider[] = {
    AROBJ_Label, "Blue",
    AROBJ_Value, &b,
    ARSLIDER_Min, 0L,
    ARSLIDER_Max, 255L,
    ARSLIDER_Default, 0L,
    TAG_END, 0L };


struct TagItem window[] = {
    AR_Title, MYNAME,
    AR_Text,ISEQ_C"\nSelect image width and height\n"
            "and the RGB values for it\n",
    AR_SliderObject,heightslider,
    AR_SliderObject, widthslider,
    AR_SliderObject, redslider,
    AR_SliderObject, greenslider,
    AR_SliderObject, blueslider,
    AR_HelpNode,     "effects.guide/Plain",
    TAG_END, 0L };

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    struct Library *UtilityBase = PPTBase->lb_Utility;
    struct Library *DOSBase = PPTBase->lb_DOS, *SysBase = PPTBase->lb_Sys;
    ROWPTR cp;
    PERROR res = PERR_OK;
    ULONG crow = 0;

    /*
     *  Allocate & Initialize load object
     */

    if((res = AskReqA( NULL, window )) == PERR_OK ) {

        /*
         *  Get picture size and initialize the Frame
         */

        frame->pix->height = height;
        frame->pix->width = width;
        frame->pix->components = 3;
        frame->pix->origdepth = 24;
        frame->pix->colorspace = CS_RGB;

        InitProgress( frame, "Creating...", 0, height );

        if((res = InitFrame( frame )) != PERR_OK) {
            /* Do clean up */
            return res;
        }

        while( crow < height ) {
            ROWPTR dcp;
            WORD col;

            if(Progress(frame, crow)) {
                res = PERR_BREAK;
                break;
            }

            cp = GetPixelRow( frame, crow );
            dcp = cp;

            /*
             *  Get the actual row of pixels, then decompress it into cp
             */

            for( col = 0; col < width; col++ ) {
                *dcp++ = r;
                *dcp++ = g;
                *dcp++ = b;
            }

            PutPixelRow( frame, crow, cp );

            crow++;

        } /* while */

        FinishProgress( frame );
    }
errorexit:
    /*
     *  Release allocated resources
     */


    return res;
}

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    return FALSE;
}

IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    return PERR_MISSINGCODE;
}

