/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : bitfield

    PPT and this file are (C) Janne Jalkanen 1995-2000.

    $Id: bitfield.c,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

// #define DEBUG_MODE

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Bitfield"

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
    "Extracts bitfields out of the image.";

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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1998-2000",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"LOW/N,HIGH/N",

    PPTX_ColorSpaces,   CSF_ARGB|CSF_RGB|CSF_GRAYLEVEL,

    PPTX_SupportsGetArgs,TRUE,

    TAG_END, 0L
};

/*
    These are all bit values.
 */

struct Values {
    int low, high;
    int scale;
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
 *  BUG: Scale should be user-settable
 */

FRAME *DoModify( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    UWORD row, col, comps = frame->pix->components;
    UBYTE mask;

    mask = ~((0xFF << (v->high+1)) | (0xFF >> (8 - v->low) ));

    v->scale = 7 - v->high;

    D(bug("Hi=%ld, Lo=%ld, Mask = %lu\n", v->high, v->low, mask ));

    InitProgress( frame, "Extracting bitfields...", frame->selbox.MinY, frame->selbox.MaxY );

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        ROWPTR cp, scp;

        if( Progress( frame, row ) )
            return NULL;

        scp = cp = GetPixelRow( frame, row );

        cp = cp + frame->selbox.MinX*comps;

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
            int i;

            for( i = 0; i < comps; i++ ) {
                *cp = (*cp & mask) << v->scale;
                cp++;
            }
        }

        PutPixelRow( frame, row, scp );
    }

    FinishProgress( frame );

    return frame;
}

PERROR
ParseRexxArgs(FRAME *frame, ULONG *args, struct Values *val, struct PPTBase *PPTBase)
{
    if( args[0] ) {
        val->low = (*(LONG *)args[0] ); // Reads a LONG
        if( val->low < 0 ) val->low = 0; else if( val->low > 7 ) val->low = 7;
    }

    if( args[1] ) {
        val->high = (*(LONG *)args[1] );
        if( val->high < 0 ) val->high = 0; else if( val->high > 7 ) val->high = 7;
    }

    return PERR_OK;
}

PERROR DoGUI(FRAME *frame, struct Values *val, struct PPTBase *PPTBase)
{
    struct TagItem win[] = { AR_SliderObject, NULL, AR_SliderObject, NULL, AR_Text, (ULONG)"\nSelect the high and low bits.\n", TAG_DONE };
    struct TagItem low[] = { AROBJ_Value, 0, ARSLIDER_Default, 0, AROBJ_Label, (ULONG)"Low:", ARSLIDER_Min, 0, ARSLIDER_Max, 7, TAG_DONE };
    struct TagItem high[] = { AROBJ_Value, 0, ARSLIDER_Default, 0, AROBJ_Label, (ULONG)"High:", ARSLIDER_Min, 0, ARSLIDER_Max, 7, TAG_DONE };

    win[0].ti_Data = (ULONG) low;
    win[1].ti_Data = (ULONG) high;

    low[0].ti_Data  = (ULONG) &val->low;
    high[0].ti_Data = (ULONG) &val->high;

    low[1].ti_Data  = val->low;
    high[1].ti_Data = val->high;

    return AskReqA( frame, win );
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    struct Values *opt, val = {0};
    PERROR res = PERR_OK;
    STRPTR buffer;

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        ParseRexxArgs( frame, args, &val, PPTBase );
    }

    res = DoGUI( frame, &val, PPTBase );

    if( res == PERR_OK ) {
        SPrintF(buffer,"LOW %d HIGH %d", val.low, val.high );
    }

    return res;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    struct Values *opt, val = {0};
    PERROR res = PERR_OK;
    FRAME *newframe = NULL;

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    }

    /*
     *  Check for REXX arguments for this effect.  Every effect should be able
     *  to accept AREXX commands!
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        ParseRexxArgs( frame, args, &val, PPTBase );
    } else {
        /*
         *  Starts up the GUI.
         */
         res = DoGUI( frame, &val, PPTBase );
    }

    /*
     *  Do the actual effect
     */

    if( res == PERR_OK ) {
        newframe = DoModify(frame, &val, PPTBase);
    }

    /*
     *  Save the options to the PPT internal system
     */

    PutOptions( MYNAME, &val, sizeof(val) );

    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

