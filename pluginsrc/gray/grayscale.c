/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : grayscale

    PPT and this file are (C) Janne Jalkanen 1998.

    $Id: grayscale.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "Greyscale"

/*----------------------------------------------------------------------*/
/* Internal prototypes */

/*----------------------------------------------------------------------*/
/* Global variables */

#pragma msg 186 ignore

const char blurb[] =
    "This converts a color picture to\n"
    "a greyscaled one by calculating\n"
    "a weighted average of main components\n";

const struct TagItem MyTagArray[] = {
    PPTX_NoNewFrame, TRUE,
    PPTX_Name, MYNAME,
    /* Other tags go here */
    PPTX_Author, "Janne Jalkanen, 1995-1999",
    PPTX_InfoTxt, blurb,
    PPTX_ColorSpaces, CSF_RGB|CSF_ARGB,
    PPTX_RexxTemplate, "",
    PPTX_SupportsGetArgs, TRUE,
    TAG_END, 0L
};

#if defined( __GNUC__ )
const BYTE LibName[]="grayscale.effect";
const BYTE LibIdString[]="grayscale.effect 1.3";
const UWORD LibVersion=1;
const UWORD LibRevision=3;
ADD2LIST(LIBEffectInquire,__FuncTable__,22);
ADD2LIST(LIBEffectExec,__FuncTable__,22);
/* The following two definitions are only required
   if you link with libinitr */
ADD2LIST(LIBEffectInquire,__LibTable__,22);
ADD2LIST(LIBEffectExec,__LibTable__,22);
ADD2LIST(-1,__LibTable__,20); /* End marker for MakeLibrary() */

/* Make GNU C specific declarations. __UtilityBase is
   required for the libnix integer multiplication */
struct ExecBase *SysBase = NULL;
struct Library *__UtilityBase = NULL;
#endif


/*----------------------------------------------------------------------*/
/* Code */

LIBINIT
{
#if defined(__GNUC__)
    SysBase = SYSBASE();

    if( NULL == (__UtilityBase = OpenLibrary("utility.library",37L))) {
        return 1L;
    }

#endif
    return 0;
}

LIBCLEANUP
{
#if defined(__GNUC__)
    if( __UtilityBase ) CloseLibrary(__UtilityBase);
#endif
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    UWORD beginx, endx;
    UWORD row;
    UBYTE comps;
    FRAME *newframe;

    beginx = frame->selbox.MinX;
    endx   = frame->selbox.MaxX;

    InitProgress( frame, "Grayscaling...", frame->selbox.MinY, frame->selbox.MaxY );

    if(frame->selbox.MinY == 0 && beginx == 0 && endx == frame->pix->width &&
       frame->selbox.MaxY == frame->pix->height)
        {
            PIXINFO *p;

            newframe = MakeFrame( frame );
            if(!newframe) {
                return NULL;
            }

            p = newframe->pix;

            p->components = 1;
            p->colorspace = CS_GRAYLEVEL;
            p->origdepth  = 8;
            if(InitFrame( newframe ) != PERR_OK) {
                RemFrame(newframe);
                return NULL;
            }
            comps = 1;
        } else {
            newframe = DupFrame( frame, DFF_COPYDATA );
            if(!newframe)
                return NULL;

            comps = frame->pix->components;
        }

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++) {
        WORD col;
        ROWPTR scp, dcp, cp;

        scp = GetPixelRow( frame, row );
        dcp = GetPixelRow( newframe, row );
        cp  = dcp + MULU16(beginx, comps);
        scp += MULU16(beginx,comps);

        if(Progress(frame, row)) {
            RemFrame(newframe);
            newframe = NULL;
            goto quit;
        }

        /*
         *  Inner loop. This should be fairly quick anyways...
         */

        for( col = beginx; col < endx; col++ ) {
            UWORD val;
            UBYTE r,g,b;

            if( frame->pix->colorspace == CS_ARGB ) scp++; /* Skip alpha */

            r = *scp++; g = *scp++; b = *scp++;
            val = ( (r * (UWORD)30) + (g * (UWORD)59) + (b * (UWORD)11) ) / (UWORD) 100;

            switch( newframe->pix->colorspace ) {
                case CS_RGB:
                    cp[0] = cp[1] = cp[2] = (UBYTE)val;
                    break;
                case CS_ARGB:
                    cp[1] = cp[2] = cp[3] = (UBYTE)val;
                    break;
                case CS_GRAYLEVEL:
                    *cp = (UBYTE)val;
                    break;
            }

            cp += comps;

        }
        PutPixelRow( newframe, row, dcp );
    }

quit:
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

