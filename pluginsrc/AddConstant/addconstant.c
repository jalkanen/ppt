/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : Addconstant.effect

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    $Id: addconstant.c,v 3.2 2000/11/17 03:00:19 jalkanen Exp $
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

#define MYNAME      "AddConstant"


/*----------------------------------------------------------------------*/
/* Internal prototypes */


FRAME *DoAdd( FRAME *frame, struct Values *v, struct PPTBase *PPTBase );

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Adds a constant value in the selected area.";

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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1998",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"RED/N,GREEN/N,BLUE/N,ALPHA/N,GREY/N",

    PPTX_ColorSpaces,   CSF_ARGB|CSF_RGB|CSF_GRAYLEVEL,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

/*
    This is a container for the internal values for
    this object.  It is saved in the Exec() routine
 */

struct Values {
    LONG    Red, Green, Blue, Alpha, Grey;
};

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

ASM ULONG MyHookFunc( REG(a0) struct Hook *hook,
                      REG(a2) Object *obj,
                      REG(a1) struct ARUpdateMsg *msg )
{
    struct Values foo;
    /*
     *  Set up A4 so that globals still work
     */

    PUTREG(REG_A4, (long) hook->h_Data);

    foo.Red = foo.Grey = msg->aum_Values[0];
    foo.Green = msg->aum_Values[1];
    foo.Blue = msg->aum_Values[2];
    foo.Alpha = msg->aum_Values[3];

    DoAdd( msg->aum_Frame, &foo, msg->aum_PPTBase );

    return ARR_REDRAW;
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

#define CLAMP(x) ( ((x) > 255) ? 255 : ((x) < 0) ? 0 : (x) )

FRAME *DoAdd( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    WORD row, col;

    InitProgress( frame, "Adding...", frame->selbox.MinY, frame->selbox.MaxY );

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {

        if( Progress( frame, row ) )
            return NULL;

        switch( frame->pix->colorspace ) {
            case CS_ARGB: {
                ARGBPixel *argb;

                argb = (ARGBPixel *)GetPixelRow( frame, row );

                for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
                    argb[col].r = CLAMP(argb[col].r+v->Red);
                    argb[col].g = CLAMP(argb[col].g+v->Green);
                    argb[col].b = CLAMP(argb[col].b+v->Blue);
                    argb[col].a = CLAMP(argb[col].a+v->Alpha);
                }
                PutPixelRow( frame, row, argb );
                break;
            }

            case CS_RGB: {
                RGBPixel *rgb;

                rgb = (RGBPixel *)GetPixelRow( frame, row );

                for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
                    rgb[col].r   = CLAMP(rgb[col].r+v->Red);
                    rgb[col].g = CLAMP(rgb[col].g+v->Green);
                    rgb[col].b  = CLAMP(rgb[col].b+v->Blue);
                }
                PutPixelRow( frame, row, rgb );
                break;
            }

            case CS_GRAYLEVEL: {
                GrayPixel *g;

                g = (GrayPixel *)GetPixelRow( frame, row );

                for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
                    g[col].g   = CLAMP(g[col].g+v->Grey);
                }
                PutPixelRow( frame, row, g );
                break;
            }
        }
    }

    FinishProgress( frame );

    return frame;
}

struct Hook pwhook = { {0}, MyHookFunc, 0L, NULL };

static
PERROR ParseRexxArgs( struct Values *val, ULONG *args, struct PPTBase *PPTBase )
{
    if( args[0] ) val->Red     = (*(LONG *)args[0] ); // Reads a LONG
    if( args[1] ) val->Green   = (*(LONG *)args[1] ); // Reads a LONG
    if( args[2] ) val->Blue    = (*(LONG *)args[2] ); // Reads a LONG
    if( args[3] ) val->Alpha   = (*(LONG *)args[3] ); // Reads a LONG
    if( args[4] ) val->Grey    = (*(LONG *)args[4] ); // Reads a LONG

    return PERR_OK;
}

static
PERROR HandleGUI( FRAME *frame, struct Values *val, UBYTE colorspace, struct PPTBase *PPTBase )
{
    struct TagItem red[] = { AROBJ_Value, NULL, AROBJ_Label, (ULONG)"Red", ARSLIDER_Min, -255, ARSLIDER_Max, 255, AROBJ_PreviewHook, (ULONG)&pwhook, TAG_DONE };
    struct TagItem green[] = { AROBJ_Value, NULL, AROBJ_Label, (ULONG)"Green", ARSLIDER_Min, -255, ARSLIDER_Max, 255, AROBJ_PreviewHook, (ULONG)&pwhook, TAG_DONE };
    struct TagItem blue[] = { AROBJ_Value, NULL, AROBJ_Label, (ULONG)"Blue", ARSLIDER_Min, -255, ARSLIDER_Max, 255, AROBJ_PreviewHook, (ULONG)&pwhook, TAG_DONE };
    struct TagItem alpha[] = { AROBJ_Value, NULL, AROBJ_Label, (ULONG)"Alpha", ARSLIDER_Min, -255, ARSLIDER_Max, 255, AROBJ_PreviewHook, (ULONG)&pwhook, TAG_DONE };
    struct TagItem grey[] = { AROBJ_Value, NULL, AROBJ_Label, (ULONG)"Grey", ARSLIDER_Min, -255, ARSLIDER_Max, 255, AROBJ_PreviewHook, (ULONG)&pwhook, TAG_DONE };
    PERROR res;

    red[0].ti_Data   = (ULONG) &val->Red;
    green[0].ti_Data = (ULONG) &val->Green;
    blue[0].ti_Data  = (ULONG) &val->Blue;
    alpha[0].ti_Data = (ULONG) &val->Alpha;
    grey[0].ti_Data  = (ULONG) &val->Grey;

    if( colorspace == CS_ARGB || colorspace == CS_RGB ) {
        res = AskReq( frame,
                      AR_SliderObject, &red,
                      AR_SliderObject, &green,
                      AR_SliderObject, &blue,
                      colorspace == CS_ARGB ? TAG_IGNORE : TAG_SKIP, 1,
                      AR_SliderObject, &alpha,
                      AR_Text, "\nPlease select the values to be added\n",
                      TAG_DONE );
    } else {
        res = AskReq( frame,
                      AR_SliderObject, &grey,
                      AR_Text, "\nPlease select the values to be added\n",
                      TAG_DONE );
    }

    return res;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe = NULL;
    ULONG *args;
    struct Values *opt, val = {0};
    PERROR res = PERR_OK;

    pwhook.h_Data = (void *)GETREG(REG_A4); // Save register

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    }

    /*
     *  Check for REXX arguments for this effect.  Every effect should be able
     *  to accept AREXX commands!
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        ParseRexxArgs( &val, args, PPTBase );
    } else {
        FRAME *pwframe;

        pwframe = ObtainPreviewFrame( frame, NULL );

        res = HandleGUI( frame, &val, frame->pix->colorspace, PPTBase );

        if( pwframe ) ReleasePreviewFrame( pwframe );

    }

    /*
     *  Do the actual effect
     */

    if( res == PERR_OK ) {
        newframe = DoAdd(frame,&val, PPTBase);
    }

    /*
     *  Save the options to the PPT internal system
     */

    PutOptions( MYNAME, &val, sizeof(val) );

    return newframe;
}

/*
    BUG: How to handle different color spaces?
 */

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    STRPTR buffer;
    PERROR res = PERR_OK;
    struct Values *opt, val = {0};

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) )
        res = ParseRexxArgs( &val, args, PPTBase );

    if( res == PERR_OK ) {
        UBYTE colorspace = CS_RGB;

        if( (res = HandleGUI( frame, &val, colorspace, PPTBase )) == PERR_OK ) {
            SPrintF( buffer, "RED %d GREEN %d BLUE %d",
                              val.Red, val.Green, val.Blue );
        }
    }

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

