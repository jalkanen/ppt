/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : ColorCount

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    $Id: colorcount.c,v 3.1 2000/11/17 03:04:21 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#undef DEBUG_MODE

#include <pptplugin.h>

#include <exec/memory.h>

#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "ColorCount"


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
    "Counts the colors present\n"
    "in the image.";

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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1998-1999",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"VAR/K",

    PPTX_ColorSpaces,   CSF_RGB|CSF_ARGB|CSF_GRAYLEVEL,

    // PPTX_NoNewFrame,    TRUE,
    PPTX_NoChangeFrame, TRUE,
    PPTX_ReqPPTVersion, 5,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

#if defined( __GNUC__ )
const BYTE LibName[]="colorcount.effect";
const BYTE LibIdString[]="colorcount 1.0";
const UWORD LibVersion=1;
const UWORD LibRevision=0;
ADD2LIST(LIBEffectInquire,__FuncTable__,22);
ADD2LIST(LIBEffectExec,__FuncTable__,22);
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
    return 0;
}


LIBCLEANUP
{
}


EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    ULONG data;

    data = TagData( attr, MyTagArray );

    return data;
}

ULONG ReqA( struct PPTBase *PPTBase, UBYTE *gadgets, UBYTE *body, ULONG *args )
{
    struct Library          *BGUIBase = PPTBase->lb_BGUI;
    struct bguiRequest      req = { 0L };
    ULONG                   res;
    struct Screen           *wscr = NULL;

    wscr = PPTBase->g->maindisp->scr;

    req.br_GadgetFormat     = gadgets;
    req.br_TextFormat       = body;
    req.br_Screen           = wscr;
    req.br_Title            = "ColorCount results";
    req.br_Flags            = BREQF_XEN_BUTTONS;
    req.br_ReqPos           = POS_CENTERSCREEN;

    res =  BGUI_RequestA( NULL, &req, args );

    return res;
}

ULONG Req( struct PPTBase *PPTBase, UBYTE *gadgets, UBYTE *body, ... )
{
    return( ReqA( PPTBase, gadgets, body, (ULONG *) (&body +1) ) );
}


#define NUMCOLORS   (1<<24)
#define NUMBYTES    (NUMCOLORS/8)

LONG CountColors( FRAME *frame, struct PPTBase *PPTBase )
{
    UWORD col,row;
    UBYTE cspace = frame->pix->colorspace;
    UBYTE *colors;
    LONG  no = 0, nBytes = NUMBYTES, numPasses = 1, pass;
    ULONG colorMin, colorMax;

    D(bug("CountColors()\n"));

    /*
     *  Iterate how much memory we can afford to allocate
     */

    if( cspace == CS_GRAYLEVEL ) nBytes = 256/8;

    while( !(colors = AllocVec(nBytes,0L))) {
        nBytes    /= 2;
        numPasses *= 2;

        if( numPasses > 256 ) {
            SetErrorCode( frame, PERR_OUTOFMEMORY );
            return -1L;
        }
    }

    D(bug("\t%Allocated %lu bytes for %lu passes\n",nBytes, numPasses));

    InitProgress( frame, "Counting colors...", 0, numPasses*(frame->selbox.MaxY-frame->selbox.MinY) );

    for( pass = 0; pass < numPasses; pass++ ) {

        bzero( colors, nBytes );

        colorMin = pass * nBytes * 8;
        colorMax = (pass+1) * nBytes * 8 - 1;

        D(bug("\tHunting for colors %06lx - %06lx\n",colorMin, colorMax ));

        for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
            ROWPTR cp;

            if( Progress( frame, pass * (frame->selbox.MaxY-frame->selbox.MinY) + (row-frame->selbox.MinY) ) ) {
                no = -1L;
                goto errexit;
            }

            cp = GetPixelRow( frame, row );

            for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
                ULONG offset = 0;
                UBYTE orig;
                RGBPixel *rgb;

                switch( cspace ) {

                    case CS_RGB:
                        rgb = (RGBPixel *)&cp[3*col];
                        offset = ( (rgb->r)<<16)|((rgb->g)<<8)|(rgb->b);
                        break;

                    case CS_ARGB:
                        offset = *((ULONG *)(&cp[col*4])) & 0xFFFFFF;
                        break;

                    case CS_GRAYLEVEL:
                        offset = ((UBYTE *)cp)[col];
                        break;
                }

                D(bug("\tColor %06X\n",offset));

                if( offset < colorMin || offset > colorMax ) continue;

                offset -= colorMin;

                orig = colors[offset/8];
                if( !(orig & (1<< (offset%8)) )) {
                    D(bug("\t\tCounted as color %06X\n",offset));
                    colors[offset/8] |= 1<< (offset % 8);
                    no++;
                }
            }
        } /* row */
    }

    FinishProgress( frame );

errexit:

    FreeVec( colors );

    return no;
}

EFFECTEXEC(frame, tags, PPTBase, EffectBase)
{
    LONG nColors;
    ULONG *args;
    char buf[40], *varname;

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        varname = (char *)args[0];
    }

    /*
     *  Do the actual effect
     */

    nColors = CountColors( frame, PPTBase );
    if( nColors >= 0 ) {
        if( !args ) {
            CloseProgress( frame );
            Req( PPTBase, "OK",
                 ISEQ_C ISEQ_B ISEQ_KEEP "\nCountColors: "ISEQ_N"%s\n\n"
                 ISEQ_N "Counted %ld different %s.\n",
                 frame->name,
                 nColors,
                 (frame->pix->colorspace == CS_GRAYLEVEL) ? "shades" : "colors" );
        } else {
            sprintf(buf,"%ld",nColors);
            SetRexxVariable( frame, varname, buf );
        }
    } else {
        frame = NULL;
    }

    return frame;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    return PERR_OK;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

