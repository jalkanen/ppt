/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : colormix

    PPT and this file are (C) Janne Jalkanen 1995-1997.

    $Id: colormix.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

#include <floatgadget.h>
#include <proto/bguifloat.h>
#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "ColorMix"

#define GID_OK              1
#define GID_CANCEL          2

#define GID_SLIDER_START    10
#define GID_RED_RED         10
#define GID_RED_GREEN       11
#define GID_RED_BLUE        12
#define GID_RED_ALPHA       13

#define GID_GREEN_RED       20
#define GID_GREEN_GREEN     21
#define GID_GREEN_BLUE      22
#define GID_GREEN_ALPHA     23

#define GID_BLUE_RED        30
#define GID_BLUE_GREEN      31
#define GID_BLUE_BLUE       32
#define GID_BLUE_ALPHA      33

#define GID_ALPHA_RED       40
#define GID_ALPHA_GREEN     41
#define GID_ALPHA_BLUE      42
#define GID_ALPHA_ALPHA     43
#define GID_SLIDER_END      43

#define GID_PREVIEW_AREA    50

#define MIX_DIVISOR         1000
#define MIX_MIN             0
#define MIX_MAX             9000
#define MIX_DEFAULT         1000

#define MIX_DIVISORF        ((float)MIX_DIVISOR)

/*----------------------------------------------------------------------*/
/* Internal prototypes */

BOOL DoModify( FRAME *frame, struct Values *v, struct PPTBase *PPTBase );


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Combines the image colors so that the\n"
    "new color is a linear combination of the\n"
    "old color.";

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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1999-2000",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"R=RED,G=GREEN,B=BLUE,A=ALPHA",

    PPTX_ColorSpaces,   CSF_RGB|CSF_ARGB,

#ifdef _M68020
    PPTX_CPU,           AFF_68020,
#endif

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

const ULONG dpcol_sl2fl[] = { SLIDER_Level, FLOAT_LongValue, TAG_END };
const ULONG dpcol_fl2sl[] = { FLOAT_LongValue, SLIDER_Level, TAG_END };

struct GUI {
    struct Window   *win;
    Object          *Win;
    Object          *RedRed, *RedGreen, *RedBlue, *RedAlpha;
    Object          *RedRedI, *RedGreenI, *RedBlueI, *RedAlphaI;

    Object          *GreenRed, *GreenGreen, *GreenBlue, *GreenAlpha;
    Object          *GreenRedI, *GreenGreenI, *GreenBlueI, *GreenAlphaI;

    Object          *BlueRed, *BlueGreen, *BlueBlue, *BlueAlpha;
    Object          *BlueRedI, *BlueGreenI, *BlueBlueI, *BlueAlphaI;

    Object          *AlphaRed, *AlphaGreen, *AlphaBlue, *AlphaAlpha;
    Object          *AlphaRedI, *AlphaGreenI, *AlphaBlueI, *AlphaAlphaI;

    Object          *Preview;

};

/*
 *  Note that we use integer values instead of floating point, because
 *  floating point might be too slow.  Anyhow, the conversion is not
 *  that difficult to make.
 */

struct Combine {
    LONG r,g,b,a;
};

struct Values {
    struct Combine  red;
    struct Combine  green;
    struct Combine  blue;
    struct Combine  alpha;
    struct IBox     winpos;
};

struct Library *BGUIFloatBase = NULL;

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
    if( BGUIFloatBase = OpenLibrary("Gadgets/bgui_float.gadget",0L) )
        return 0;

    return 1;
}


LIBCLEANUP
{
    if( BGUIFloatBase ) CloseLibrary( BGUIFloatBase );
}


EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

#define FloatObject NewObject( FloatClass, NULL

#define QuickFloatObject( lv ) NewObject( FloatClass, NULL, \
                                          FLOAT_LongValue, lv, \
                                          FLOAT_LongMin, MIX_MIN, \
                                          FLOAT_LongMax, MIX_MAX, \
                                          FLOAT_Divisor, MIX_DIVISOR, \
                                          FLOAT_Format,  "%.2f", \
                                          STRINGA_MinCharsVisible, 5, \
                                          STRINGA_MaxChars, 12, \
                                          TAG_DONE )

Object *OpenGUI( FRAME *frame, struct GUI *gui, struct Values *v, struct PPTBase *PPTBase )
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    BOOL alpha = FALSE;
    Object *alphaGroup = NULL;
    struct IClass *FloatClass;

    if( !(FloatClass = GetFloatClassPtr())) return NULL;

    if( frame->pix->colorspace == CS_ARGB ) alpha = TRUE;

    if( alpha ) {
        alphaGroup = VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
            FrameTitle("Alpha"),
            NeXTFrame,
            StartMember,
                HGroupObject, NarrowSpacing,
                    StartMember,
                        gui->AlphaRed = SliderObject,
                            GA_ID, GID_ALPHA_RED,
                            Label("R"),
                            SLIDER_Min, MIX_MIN,
                            SLIDER_Max, MIX_MAX,
                            SLIDER_Level, v->alpha.r,
                        EndObject,
                    EndMember,
                    StartMember,
                        gui->AlphaRedI = QuickFloatObject( v->alpha.r ),
                        Weight(1),
                    EndMember,
                EndObject,
            EndMember,

            StartMember,
                HGroupObject, NarrowSpacing,
                    StartMember,
                        gui->AlphaGreen = SliderObject,
                            GA_ID, GID_ALPHA_GREEN,
                            Label("G"),
                            SLIDER_Min, MIX_MIN,
                            SLIDER_Max, MIX_MAX,
                            SLIDER_Level, v->alpha.g,
                        EndObject,
                    EndMember,
                    StartMember,
                        gui->AlphaGreenI = QuickFloatObject( v->alpha.g ),
                        Weight(1),
                    EndMember,
                EndObject,
            EndMember,

            StartMember,
                HGroupObject, NarrowSpacing,
                    StartMember,
                        gui->AlphaBlue = SliderObject,
                            GA_ID, GID_ALPHA_BLUE,
                            Label("B"),
                            SLIDER_Min, MIX_MIN,
                            SLIDER_Max, MIX_MAX,
                            SLIDER_Level, v->alpha.b,
                        EndObject,
                    EndMember,
                    StartMember,
                        gui->AlphaBlueI = QuickFloatObject( v->alpha.b ),
                        Weight(1),
                    EndMember,
                EndObject,
            EndMember,

            StartMember,
                HGroupObject, NarrowSpacing,
                    StartMember,
                        gui->AlphaAlpha = SliderObject,
                            GA_ID, GID_ALPHA_ALPHA,
                            Label("A"),
                            SLIDER_Min, MIX_MIN,
                            SLIDER_Max, MIX_MAX,
                            SLIDER_Level, v->alpha.a,
                        EndObject,
                    EndMember,
                    StartMember,
                        gui->AlphaAlphaI = QuickFloatObject( v->alpha.a ),
                        Weight(1),
                    EndMember,
                EndObject,
            EndMember,
        EndObject;

    }

    gui->Win = WindowObject,
            WINDOW_Screen,      PPTBase->g->maindisp->scr,
            WINDOW_Title,       frame->name,
            WINDOW_ScreenTitle, "ColorMix (C) Janne Jalkanen 1997-2000.",
            WINDOW_ScaleWidth,  60,
            WINDOW_LockHeight,  TRUE,
            WINDOW_NoBufferRP,  TRUE,
            WINDOW_MasterGroup,
                VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                    StartMember,
                        HGroupObject, NarrowSpacing,
                            StartMember,
                                InfoObject,
                                    INFO_TextFormat, ISEQ_C"\nUse the sliders to determine how\n"
                                                     "the new colors should be calculated.\n",
                                    INFO_MinLines,   4,
                                    ButtonFrame,
                                    FRM_Flags, FRF_RECESSED,
                                EndObject,
                            EndMember,
                            StartMember,
                                gui->Preview = AreaObject,
                                    AREA_MinWidth, PPTBase->g->userprefs->previewwidth,
                                    AREA_MinHeight, PPTBase->g->userprefs->previewheight,
                                    GA_ID, GID_PREVIEW_AREA,
                                    StringFrame,
                                EndObject, FixMinSize,
                            EndMember,
                        EndObject,
                    EndMember,
                    StartMember,
                        HGroupObject, WideSpacing,
                            StartMember,
                                VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                                    FrameTitle("Red"),
                                    NeXTFrame,
                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->RedRed = SliderObject,
                                                    GA_ID, GID_RED_RED,
                                                    Label("R"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->red.r,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->RedRedI = QuickFloatObject(v->red.r),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->RedGreen = SliderObject,
                                                    GA_ID, GID_RED_GREEN,
                                                    Label("G"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->red.g,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->RedGreenI = QuickFloatObject( v->red.g ),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->RedBlue = SliderObject,
                                                    GA_ID, GID_RED_BLUE,
                                                    Label("B"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->red.b,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->RedBlueI = QuickFloatObject( v->red.b ),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,


                                    alpha ? TAG_IGNORE : TAG_SKIP, 2,
                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->RedAlpha = SliderObject,
                                                    GA_ID, GID_RED_ALPHA,
                                                    Label("A"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->red.a,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->RedAlphaI = QuickFloatObject( v->red.a ),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                EndObject,
                            EndMember,


                            /******* GREEN *******/
                            StartMember,
                                VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                                    FrameTitle("Green"),
                                    NeXTFrame,
                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->GreenRed = SliderObject,
                                                    GA_ID, GID_GREEN_RED,
                                                    Label("R"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->green.r,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->GreenRedI = QuickFloatObject( v->green.r ),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->GreenGreen = SliderObject,
                                                    GA_ID, GID_GREEN_GREEN,
                                                    Label("G"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->green.g,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->GreenGreenI = QuickFloatObject(v->green.g),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->GreenBlue = SliderObject,
                                                    GA_ID, GID_GREEN_BLUE,
                                                    Label("B"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->green.b,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->GreenBlueI = QuickFloatObject(v->green.b),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                    alpha ? TAG_IGNORE : TAG_SKIP, 2,

                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->GreenAlpha = SliderObject,
                                                    GA_ID, GID_GREEN_ALPHA,
                                                    Label("A"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->green.a,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->GreenAlphaI = QuickFloatObject( v->green.a ),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                EndObject,
                            EndMember,

                            /******* BLUE *******/
                            StartMember,
                                VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                                    FrameTitle("Blue"),
                                    NeXTFrame,
                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->BlueRed = SliderObject,
                                                    GA_ID, GID_BLUE_RED,
                                                    Label("R"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->blue.r,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->BlueRedI = QuickFloatObject( v->blue.r ),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->BlueGreen = SliderObject,
                                                    GA_ID, GID_BLUE_GREEN,
                                                    Label("G"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->blue.g,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->BlueGreenI = QuickFloatObject(v->blue.g),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->BlueBlue = SliderObject,
                                                    GA_ID, GID_BLUE_BLUE,
                                                    Label("B"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->blue.b,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->BlueBlueI = QuickFloatObject(v->blue.b),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                    alpha ? TAG_IGNORE : TAG_SKIP, 2,
                                    StartMember,
                                        HGroupObject, NarrowSpacing,
                                            StartMember,
                                                gui->BlueAlpha = SliderObject,
                                                    GA_ID, GID_BLUE_ALPHA,
                                                    Label("A"),
                                                    SLIDER_Min, MIX_MIN,
                                                    SLIDER_Max, MIX_MAX,
                                                    SLIDER_Level, v->blue.a,
                                                EndObject,
                                            EndMember,
                                            StartMember,
                                                gui->BlueAlphaI = QuickFloatObject(v->blue.a),
                                                Weight(1),
                                            EndMember,
                                        EndObject,
                                    EndMember,

                                EndObject,
                            EndMember,

                            /******* ALPHA *******/

                            (alphaGroup) ? TAG_IGNORE : TAG_SKIP, 2,
                            GROUP_Member, alphaGroup,
                            TAG_DONE, 0L, // EndMember,

                        EndObject,
                    EndMember,

                    StartMember,
                        HorizSeparator,
                    EndMember,
                    StartMember,
                        HGroupObject,
                            StartMember,
                                ButtonObject, GA_ID, GID_OK,
                                    Label("Ok"),
                                EndObject,
                            EndMember,
                            VarSpace(50),
                            StartMember,
                                ButtonObject, GA_ID, GID_CANCEL,
                                    Label("Cancel"),
                                EndObject,
                            EndMember,
                        EndObject,
                    EndMember,
                EndObject,
        EndObject;


    if( gui->Win ) {

        /*
         *  mappings
         */

        AddMap( gui->RedRed, gui->RedRedI, dpcol_sl2fl );
        AddMap( gui->RedRedI, gui->RedRed, dpcol_fl2sl );
        AddMap( gui->RedGreen, gui->RedGreenI, dpcol_sl2fl );
        AddMap( gui->RedGreenI, gui->RedGreen, dpcol_fl2sl );
        AddMap( gui->RedBlue, gui->RedBlueI, dpcol_sl2fl );
        AddMap( gui->RedBlueI, gui->RedBlue, dpcol_fl2sl );

        AddMap( gui->BlueRed, gui->BlueRedI, dpcol_sl2fl );
        AddMap( gui->BlueRedI, gui->BlueRed, dpcol_fl2sl );
        AddMap( gui->BlueGreen, gui->BlueGreenI, dpcol_sl2fl );
        AddMap( gui->BlueGreenI, gui->BlueGreen, dpcol_fl2sl );
        AddMap( gui->BlueBlue, gui->BlueBlueI, dpcol_sl2fl );
        AddMap( gui->BlueBlueI, gui->BlueBlue, dpcol_fl2sl );

        AddMap( gui->GreenRed, gui->GreenRedI, dpcol_sl2fl );
        AddMap( gui->GreenRedI, gui->GreenRed, dpcol_fl2sl );
        AddMap( gui->GreenGreen, gui->GreenGreenI, dpcol_sl2fl );
        AddMap( gui->GreenGreenI, gui->GreenGreen, dpcol_fl2sl );
        AddMap( gui->GreenBlue, gui->GreenBlueI, dpcol_sl2fl );
        AddMap( gui->GreenBlueI, gui->GreenBlue, dpcol_fl2sl );

        if( alpha ) {
            AddMap( gui->RedAlpha, gui->RedAlphaI, dpcol_sl2fl );
            AddMap( gui->RedAlphaI, gui->RedAlpha, dpcol_fl2sl );

            AddMap( gui->BlueAlpha, gui->BlueAlphaI, dpcol_sl2fl );
            AddMap( gui->BlueAlphaI, gui->BlueAlpha, dpcol_fl2sl );

            AddMap( gui->GreenAlpha, gui->GreenAlphaI, dpcol_sl2fl );
            AddMap( gui->GreenAlphaI, gui->GreenAlpha, dpcol_fl2sl );

            AddMap( gui->AlphaRed, gui->AlphaRedI, dpcol_sl2fl );
            AddMap( gui->AlphaRedI, gui->AlphaRed, dpcol_fl2sl );
            AddMap( gui->AlphaGreen, gui->AlphaGreenI, dpcol_sl2fl );
            AddMap( gui->AlphaGreenI, gui->AlphaGreen, dpcol_fl2sl );
            AddMap( gui->AlphaBlue, gui->AlphaBlueI, dpcol_sl2fl );
            AddMap( gui->AlphaBlueI, gui->AlphaBlue, dpcol_fl2sl );
            AddMap( gui->AlphaAlpha, gui->AlphaAlphaI, dpcol_sl2fl );
            AddMap( gui->AlphaAlphaI, gui->AlphaAlpha, dpcol_fl2sl );
        }

        /*
         *  Window
         */

        if( gui->win = WindowOpen( gui->Win ) ) {
            /* Ok */
        } else {
            DisposeObject( gui->Win );
            gui->Win = NULL;
        }
    }

    return gui->Win;
}

#pragma msg 88 ignore

VOID ReadAttrs( struct GUI *gui, struct Values *values, struct PPTBase *PPTBase )
{
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    GetAttr( SLIDER_Level, gui->RedRed, &(values->red.r) );
    GetAttr( SLIDER_Level, gui->RedGreen, &(values->red.g) );
    GetAttr( SLIDER_Level, gui->RedBlue, &(values->red.b) );
    GetAttr( SLIDER_Level, gui->RedAlpha, &(values->red.a) );

    GetAttr( SLIDER_Level, gui->GreenRed, &(values->green.r) );
    GetAttr( SLIDER_Level, gui->GreenGreen, &(values->green.g) );
    GetAttr( SLIDER_Level, gui->GreenBlue, &(values->green.b) );
    GetAttr( SLIDER_Level, gui->GreenAlpha, &(values->green.a) );

    GetAttr( SLIDER_Level, gui->BlueRed, &(values->blue.r) );
    GetAttr( SLIDER_Level, gui->BlueGreen, &(values->blue.g) );
    GetAttr( SLIDER_Level, gui->BlueBlue, &(values->blue.b) );
    GetAttr( SLIDER_Level, gui->BlueAlpha, &(values->blue.a) );

    GetAttr( SLIDER_Level, gui->AlphaRed, &(values->alpha.r) );
    GetAttr( SLIDER_Level, gui->AlphaGreen, &(values->alpha.g) );
    GetAttr( SLIDER_Level, gui->AlphaBlue, &(values->alpha.b) );
    GetAttr( SLIDER_Level, gui->AlphaAlpha, &(values->alpha.a) );
}
#pragma msg 88 warn

/*
 *  Also takes care of the preview
 */

BOOL HandleIDCMP( FRAME *frame, struct GUI *gui, struct Values *values, struct PPTBase *PPTBase )
{
    BOOL quit = FALSE, res = TRUE;
    ULONG signals = 0L, sig, rc;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    FRAME *pwframe = NULL, *tmpframe = NULL;

    if( pwframe = ObtainPreviewFrame( frame, NULL ) ) {
        tmpframe = DupFrame( pwframe, DFF_COPYDATA );
    }

    GetAttr( WINDOW_SigMask, gui->Win, &signals );

    while(!quit) {
        sig = Wait( signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F );

        if( sig & SIGBREAKF_CTRL_F ) {
            WindowToFront( gui->win );
            ActivateWindow( gui->win );
        }

        if( sig & SIGBREAKF_CTRL_C ) {
            SetErrorCode( frame, PERR_BREAK );
            res = FALSE;
            quit = TRUE;
        }

        if( sig & signals ) {
            while( (rc = HandleEvent( gui->Win )) != WMHI_NOMORE ) {
                struct IBox *area;

                switch( rc ) {
                    case WMHI_CLOSEWINDOW:
                    case GID_CANCEL:
                        res = FALSE;
                        quit = TRUE;
                        break;

                    case GID_OK:
                        quit = TRUE;
                        ReadAttrs( gui, values, PPTBase );
                        break;

                    default:
                        if( rc < GID_SLIDER_START || rc > GID_SLIDER_END )
                            break;
                        /* FALLTHROUGH OK */
                    case GID_PREVIEW_AREA:
                        if( pwframe && tmpframe ) {
                            ReadAttrs( gui, values, PPTBase );
                            GetAttr( AREA_AreaBox, gui->Preview, (ULONG *)&area );
                            CopyFrameData( pwframe, tmpframe, 0L );
                            DoModify( tmpframe, values, PPTBase );
                            RenderFrame( tmpframe, gui->win->RPort, area, 0L );
                        }
                        break;
                }
            } /* while */
        }
    } /* !quit */


    if( tmpframe ) RemFrame( tmpframe );

    if( pwframe )  ReleasePreviewFrame( pwframe );

    return res;
}

#define MIX(color, value) (((color) * (value)) / (MIX_DIVISOR))
#define CLAMP(x,min,max)  (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))

BOOL DoModify( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    WORD row;
    BYTE cspace = frame->pix->colorspace;

    InitProgress( frame, "Mixing colors...", 0, frame->pix->height );

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        ROWPTR cp;
        WORD col;

        if( Progress( frame, row ) ) {
            SetErrorCode( frame, PERR_BREAK );
            return FALSE;
        }

        cp = GetPixelRow( frame, row );

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
            if( cspace == CS_RGB ) {
                UBYTE r,g,b;
                ULONG newr, newg, newb;

                r = ((RGBPixel *)cp)[col].r;
                g = ((RGBPixel *)cp)[col].g;
                b = ((RGBPixel *)cp)[col].b;

                newr = MIX(r,v->red.r) + MIX(g,v->red.g) + MIX(b,v->red.b);
                newg = MIX(r,v->green.r) + MIX(g,v->green.g) + MIX(b,v->green.b);
                newb = MIX(r,v->blue.r) + MIX(g,v->blue.g) + MIX(b,v->blue.b);

                ((RGBPixel*)cp)[col].r = CLAMP(newr,0,255);
                ((RGBPixel*)cp)[col].g = CLAMP(newg,0,255);
                ((RGBPixel*)cp)[col].b = CLAMP(newb,0,255);

            } else {
                UBYTE r,g,b,a;
                ULONG newr, newg, newb, newa;

                r = ((ARGBPixel *)cp)[col].r;
                g = ((ARGBPixel *)cp)[col].g;
                b = ((ARGBPixel *)cp)[col].b;
                a = ((ARGBPixel *)cp)[col].a;

                newr = MIX(r,v->red.r) + MIX(g,v->red.g) + MIX(b,v->red.b) + MIX(a,v->red.a);
                newg = MIX(r,v->green.r) + MIX(g,v->green.g) + MIX(b,v->green.b) + MIX(a,v->green.a);
                newb = MIX(r,v->blue.r) + MIX(g,v->blue.g) + MIX(b,v->blue.b) + MIX(a,v->blue.a);
                newa = MIX(r,v->blue.r) + MIX(g,v->blue.g) + MIX(b,v->blue.b) + MIX(a,v->alpha.a);

                ((ARGBPixel*)cp)[col].r = CLAMP(newr,0,255);
                ((ARGBPixel*)cp)[col].g = CLAMP(newg,0,255);
                ((ARGBPixel*)cp)[col].b = CLAMP(newb,0,255);
                ((ARGBPixel*)cp)[col].a = CLAMP(newa,0,255);
            }
        }


        PutPixelRow( frame, row, cp );
    }

    FinishProgress( frame );

    return TRUE;
}

VOID FixValues( struct Values *v, struct PPTBase *PPTBase )
{

}

PERROR ParseRexxArgs( ULONG *args, struct Values *v, struct PPTBase *PPTBASE )
{
    char argbuf[40];
    float red,green,blue,alpha;

    if( args[0] ) { // RED
        strncpy( argbuf, (char *)args[0], 39 );
        if(sscanf( argbuf, "%f,%f,%f,%f", &red, &green, &blue, &alpha ) < 3) {
            return FALSE;
        }
        v->red.r = red * MIX_DIVISOR;
        v->red.g = green * MIX_DIVISOR;
        v->red.b = blue * MIX_DIVISOR;
        v->red.a = alpha * MIX_DIVISOR;
    }

    if( args[1] ) { // GREEN
        strncpy( argbuf, (char *)args[1], 39 );
        if(sscanf( argbuf, "%f,%f,%f,%f", &red, &green, &blue, &alpha ) < 3) {
            return FALSE;
        }
        v->green.r = red * MIX_DIVISOR;
        v->green.g = green * MIX_DIVISOR;
        v->green.b = blue * MIX_DIVISOR;
        v->green.a = alpha * MIX_DIVISOR;
    }
    if( args[2] ) { // BLUE
        strncpy( argbuf, (char *)args[2], 39 );
        if(sscanf( argbuf, "%f,%f,%f,%f", &red, &green, &blue, &alpha ) < 3) {
            return FALSE;
        }
        v->blue.r = red * MIX_DIVISOR;
        v->blue.g = green * MIX_DIVISOR;
        v->blue.b = blue * MIX_DIVISOR;
        v->blue.a = alpha * MIX_DIVISOR;
    }
    if( args[3] ) { // ALPHA
        strncpy( argbuf, (char *)args[3], 39 );
        if(sscanf( argbuf, "%f,%f,%f,%f", &red, &green, &blue, &alpha ) < 3) {
            return FALSE;
        }
        v->alpha.r = red * MIX_DIVISOR;
        v->alpha.g = green * MIX_DIVISOR;
        v->alpha.b = blue * MIX_DIVISOR;
        v->alpha.a = alpha * MIX_DIVISOR;
    }

    return TRUE;
}


EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct GUI mygui = {0};
    struct Values v = {0}, *opt;
    ULONG *args;

    v.red.r = v.green.g = v.blue.b = v.alpha.a = MIX_DEFAULT;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( ParseRexxArgs( args, &v, PPTBase ) ) {
            if(!DoModify( frame, &v, PPTBase ) ) {
                frame = NULL;
            }
        } else {
            SetErrorCode( frame, PERR_INVALIDARGS );
            return NULL;
        }


    } else {
        if( OpenGUI( frame, &mygui, &v, PPTBase ) ) {
            if( HandleIDCMP( frame, &mygui, &v, PPTBase ) ) {
                WindowClose( mygui.Win );

                if( !DoModify( frame, &v, PPTBase ) ) {
                    frame = NULL;
                }
            }

            DisposeObject( mygui.Win );
        } else {
            frame = NULL;
        }
    }

    if( frame ) {
        PutOptions( MYNAME, &v, sizeof(v) );
    }

    return frame;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct GUI mygui = {0};
    struct Values v = {0}, *opt;
    ULONG *args;
    STRPTR buffer;
    PERROR res = PERR_OK;

    v.red.r = v.green.g = v.blue.b = v.alpha.a = MIX_DEFAULT;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    buffer = (STRPTR)TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( !ParseRexxArgs( args, &v, PPTBase ) ) {
            SetErrorCode( frame, PERR_INVALIDARGS );
            return PERR_INVALIDARGS;
        }
    }

    if( OpenGUI( frame, &mygui, &v, PPTBase ) ) {
        if( HandleIDCMP( frame, &mygui, &v, PPTBase ) ) {
            DisposeObject( mygui.Win );
            // "R=RED,G=GREEN,B=BLUE,A=ALPHA",

            SPrintF( buffer, "RED %f,%f,%f,%f GREEN %f,%f,%f,%f BLUE %f,%f,%f,%f ALPHA %f,%f,%f,%f",
                             v.red.r/MIX_DIVISORF,   v.red.g/MIX_DIVISORF,   v.red.b/MIX_DIVISORF,   v.red.a/MIX_DIVISORF,
                             v.green.r/MIX_DIVISORF, v.green.g/MIX_DIVISORF, v.green.b/MIX_DIVISORF, v.green.a/MIX_DIVISORF,
                             v.blue.r/MIX_DIVISORF,  v.blue.g/MIX_DIVISORF,  v.blue.b/MIX_DIVISORF,  v.blue.a/MIX_DIVISORF,
                             v.alpha.r/MIX_DIVISORF, v.alpha.g/MIX_DIVISORF, v.alpha.b/MIX_DIVISORF, v.alpha.a/MIX_DIVISORF);

        } else {
            res = PERR_CANCELED;
        }
    } else {
        res = PERR_WINDOWOPEN;
    }

    return res;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

