/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : negative.effect

    PPT and this file are (C) Janne Jalkanen 1995-1999.

    $Id: negative.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
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

#define MYNAME      "Negative"


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
    "Make a negative out of the current area.";

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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1999",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"",

    PPTX_ColorSpaces,   CSF_ARGB|CSF_RGB|CSF_GRAYLEVEL,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

#if defined( __GNUC__ )
const BYTE LibName[]="negative.effect";
const BYTE LibIdString[]="negative 1.0";
const UWORD LibVersion=1;
const UWORD LibRevision=0;
ADD2LIST(LIBEffectInquire,__FuncTable__,22);
ADD2LIST(LIBEffectExec,__FuncTable__,22);
/* The following two definitions are only required
   if you link with libinitr */
ADD2LIST(LIBEffectInquire,__LibTable__,22);
ADD2LIST(LIBEffectExec,__LibTable__,22);

/* Make GNU C specific declarations. __UtilityBase is
   required for the libnix integer multiplication */
struct ExecBase *SysBase = NULL;
struct Library *__UtilityBase = NULL;
#endif

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

/*
    This routine is called upon the OpenLibrary() the software
    makes.  You could use this to open up your own libraries
    or stuff.

    Return 0 if everything went OK or something else if things
    failed.
*/

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
    ULONG data;

    data = TagData( attr, MyTagArray );

    return data;
}


EFFECTEXEC(frame, tags, PPTBase, EffectBase)
{
    LONG row, col;
    LONG pixelsize = frame->pix->components;
    UBYTE colorspace = frame->pix->colorspace;

    InitProgress( frame, "Executing Negative...", frame->selbox.MinY, frame->selbox.MaxY );

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        ROWPTR cp, scp;

        if( Progress( frame, row ) ) {
            return NULL;
        }

        if(scp = cp = GetPixelRow( frame, row )) {

            cp += frame->selbox.MinX * pixelsize;

            for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {

                switch(colorspace) {
                    case CS_ARGB:
                        ((ARGBPixel *)cp)->r = 255 - ((ARGBPixel *)cp)->r;
                        ((ARGBPixel *)cp)->g = 255 - ((ARGBPixel *)cp)->g;
                        ((ARGBPixel *)cp)->b = 255 - ((ARGBPixel *)cp)->b;
                        break;

                    case CS_RGB:
                        ((RGBPixel *)cp)->r = 255 - ((RGBPixel *)cp)->r;
                        ((RGBPixel *)cp)->g = 255 - ((RGBPixel *)cp)->g;
                        ((RGBPixel *)cp)->b = 255 - ((RGBPixel *)cp)->b;
                        break;

                    case CS_GRAYLEVEL:
                        *cp = 255 - *cp;
                        break;
                }

                cp += pixelsize;
            } /* for col */

            PutPixelRow( frame, row, scp );
        }
    }

    FinishProgress( frame );

    return frame;
}

EFFECTGETARGS(frame, tags, PPTBase, EffectBase)
{
    STRPTR buffer;

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    buffer[0] = '\0'; /* No arguments necessary */

    return PERR_OK;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

