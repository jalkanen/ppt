/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : Transparency

    PPT and this file are (C) Janne Jalkanen 1995-1999.

    $Id: transparency.c,v 1.1 2001/10/25 16:23:03 jalkanen Exp $

*/
/*----------------------------------------------------------------------*/

#undef DEBUG_MODE

/// Definitions and includes
/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#define GAD(x)  ((struct Gadget *)x)

#include <gadgets/colorwheel.h>
#include <gadgets/gradientslider.h>
#include <graphics/gfxbase.h>
#include <libraries/gadtools.h>

#include <proto/colorwheel.h>
#include <proto/graphics.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Transparency"

#define GID_OK              1
#define GID_CANCEL          2
#define GID_TRANSPARENCY    3
#define GID_TRANSPARENCYI   4
#define GID_WHEEL           5
#define GID_RED             6
#define GID_REDI            7
#define GID_GREEN           8
#define GID_GREENI          9
#define GID_BLUE            10
#define GID_BLUEI           11
#define GID_GRADIENTSLIDER  12
#define GID_BLACK           13
#define GID_WHITE           14
#define GID_BACKGROUND      15
#define GID_ALL             16
#define GID_TOLERANCE       17
#define GID_TOLERANCEI      18

#define GRADCOLORS          4

/*----------------------------------------------------------------------*/
/* Internal type definitions */

/*
    Contains also window pointers and library bases.
 */
struct Objects {
    Object              *Win;
    struct Window       *win;
    Object              *Trans, *TransI, *Gradient, *RealGradient;
    Object              *Wheel, *Red, *Green, *Blue;
    Object              *RedI, *GreenI, *BlueI, *Cycle;
    Object              *All, *Tolerance, *ToleranceI;
    struct Library      *ColorWheelBase, *GradientSliderBase;
    int                 numpens;
    WORD                pens[GRADCOLORS+1];
    char                wtitle[80];
};

struct Values {
    ULONG               transp;
    ULONG               r,g,b;
    ULONG               tolerance;
    ULONG               mode;
    struct IBox         window;
    BOOL                background;
};

#define MODE_ALL        0   /* All colors are set */
#define MODE_EXACT      1   /* Only exact matches are allowed */

/*
    For LoadRGB32()
 */

struct load32 {
        UWORD           l32_len;
        UWORD           l32_pen;
        ULONG           l32_red;
        ULONG           l32_grn;
        ULONG           l32_blu;
};

/*----------------------------------------------------------------------*/
/* Internal prototypes */


///

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Sets up transparency information.";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name,              (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,            (ULONG)"Janne Jalkanen, 1997-2000",
    PPTX_InfoTxt,           (ULONG)infoblurb,

    PPTX_RexxTemplate,      (ULONG)"R=RED/N,G=GREEN/N,B=BLUE/N,TRANS=T/N,BACKGROUND/S,ALL/S,TOLERANCE=TOL/N",

    PPTX_ColorSpaces,       CSF_RGB|CSF_ARGB,

    PPTX_SupportsGetArgs,   TRUE,

    PPTX_NoNewFrame,        TRUE,

    TAG_END, 0L
};

const ULONG dpcol_sl2int[] = { SLIDER_Level, STRINGA_LongVal, TAG_END };
const ULONG dpcol_int2sl[] = { STRINGA_LongVal, SLIDER_Level, TAG_END };

struct NewMenu mymenus[] = {
    Title( "Project" ),
        Item("Quit",        "Q",    GID_CANCEL ),
    Title( "Presets" ),
        Item("Black",       "1",     GID_BLACK ),
        Item("White",       "2",     GID_WHITE ),
        Item("Background",  "0",     GID_BACKGROUND ),
        Item("All",         "A",     GID_ALL ),
    End
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

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

/// DoTransparency
#define SQR(x) ((x)*(x))

FRAME *DoTransparency( FRAME *frame, struct Values *v, struct PPTBase *PPTBase)
{
    FRAME *newframe = NULL;
    BOOL  hasalpha = FALSE;
    ULONG tol;

    tol = SQR(v->tolerance);

    if( frame->pix->colorspace == CS_ARGB )
        hasalpha = TRUE;

    // PDebug(MYNAME": Exec()\n");

    newframe = MakeFrame( frame );
    if(newframe) {
        newframe->pix->colorspace = CS_ARGB;
        newframe->pix->components = 4;

        if( InitFrame( newframe ) == PERR_OK ) {
            WORD row;

            InitProgress(frame,"Setting up transparency...",0,frame->pix->height);

            for( row = 0; row < frame->pix->height; row++ ) {
                RGBPixel *cp;
                ARGBPixel *dcp, *acp;
                WORD col;
                BOOL isin;

                if( Progress(frame, row) ) {
                    RemFrame(newframe);
                    newframe = NULL;
                    break;
                }

                cp = (RGBPixel *)GetPixelRow( frame, row );
                acp = (ARGBPixel *)cp;
                dcp = (ARGBPixel *)GetPixelRow( newframe, row );

                /*
                 *  Loopety loop.  If the pixel to be set is exactly
                 *  the color, then set the transparency.
                 */

                for( col = 0; col < frame->pix->width; col++ ) {
                    ULONG dist;

                    /*
                     *  Check if we're inside selbox
                     */

                    if( row >= frame->selbox.MinY && row < frame->selbox.MaxY &&
                        col >= frame->selbox.MinX && col < frame->selbox.MaxX ){
                        isin = TRUE;
                    } else {
                        isin = FALSE;
                    }

                    /*
                     *  Set the alpha channel, if we're inside the selbox
                     */
                    if( hasalpha ) {
                        dist = SQR((WORD)((WORD)acp->r - v->r))+
                               SQR((WORD)((WORD)acp->g - v->g))+
                               SQR((WORD)((WORD)acp->b - v->b));

                        if( (v->mode == MODE_ALL || dist <= tol) && isin)
                        {
                            dcp->a = (UBYTE) v->transp;
                        } else {
                            dcp->a = acp->a; /* Retain transparency */
                        }
                        dcp->r = acp->r;
                        dcp->g = acp->g;
                        dcp->b = acp->b;
                        acp++;
                        dcp++;
                    } else {
                        dist = SQR((WORD)((WORD)cp->r - v->r))+
                               SQR((WORD)((WORD)cp->g - v->g))+
                               SQR((WORD)((WORD)cp->b - v->b));

                        if( (v->mode == MODE_ALL || dist <= tol) && isin)
                        {
                            dcp->a = (UBYTE) v->transp;
                        } else {
                            dcp->a = 0; /* No transparency */
                        }
                        dcp->r = cp->r;
                        dcp->g = cp->g;
                        dcp->b = cp->b;
                        cp++;
                        dcp++;
                    }
                }

                PutPixelRow( newframe, row, dcp );
            }

            FinishProgress(frame);

        } else {
            RemFrame(newframe);
            newframe = NULL;
        }
    }

    return newframe;

}
///
/// DoGadgets
VOID DoGadgets( FRAME *frame, struct Objects *o, struct Values *v, struct PPTBase *PPTBase)
{
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    ULONG tmp;

    GetAttr( SLIDER_Level, o->Trans, &v->transp);
    GetAttr( SLIDER_Level, o->Red, &v->r);
    GetAttr( SLIDER_Level, o->Green, &v->g);
    GetAttr( SLIDER_Level, o->Blue, &v->b);
    GetAttr( GA_Selected,  o->All, &tmp );
    GetAttr( SLIDER_Level, o->Tolerance, &v->tolerance );
    if( tmp ) v->mode = MODE_ALL; else v->mode = MODE_EXACT;
}
///
/// OpenGUI
/*
    The following cases apply:

    Make black transparent
    Make white transparent
    Make any color transparent
    Make background transparent
 */
PERROR OpenGUI(FRAME *frame, struct Objects *o, struct Values *v, struct PPTBase *PPTBase)
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;
    struct Screen *scr = PPTBase->g->maindisp->scr;
    struct Library *ColorWheelBase = o->ColorWheelBase;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    BOOL   V39 = FALSE;
    struct ColorWheelRGB rgb;
    struct ColorWheelHSB hsb;

    sprintf(o->wtitle, MYNAME ": %s", frame->name );

    if( PPTBase->lb_Gfx->LibNode.lib_Version >= 39 ) {
        int i;

        V39 = TRUE;
        rgb.cw_Red   = 0;
        rgb.cw_Green = 0;
        rgb.cw_Blue  = 0;
        ConvertRGBToHSB( &rgb, &hsb );

        for( i = 0; i < GRADCOLORS; i++ ) {
            hsb.cw_Brightness = (ULONG)0xffffffff - (((ULONG)0xffffffff / GRADCOLORS ) * i );
            ConvertHSBToRGB( &hsb, &rgb );
            if( -1 == (o->pens[i] = ObtainBestPen( scr->ViewPort.ColorMap,
                                                  rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue,
                                                  TAG_DONE ) ) ) {
                break;
            }
        }
        o->pens[i] = ~0;
        o->numpens = i;

        o->Gradient = ExternalObject,
            GA_ID,          GID_GRADIENTSLIDER,
            EXT_MinWidth,   10,
            EXT_MinHeight,  10,
            EXT_ClassID,    "gradientslider.gadget",
            EXT_NoRebuild,  TRUE,
            GRAD_PenArray,  o->pens,
            PGA_Freedom,    LORIENT_VERT,
        EndObject;

        if( o->Gradient )
            GetAttr( EXT_Object, o->Gradient, (ULONG *) &o->RealGradient );
    }

    o->Win = WindowObject,
        WINDOW_Screen,      scr,
        WINDOW_Title,       o->wtitle,
        WINDOW_ScreenTitle, "Transparency: Click on the image to choose a color",
        v->window.Height ? TAG_IGNORE : TAG_SKIP, 1,
        WINDOW_Bounds,      &v->window,

        WINDOW_ScaleWidth,  30,
        WINDOW_NoBufferRP,  TRUE,
        WINDOW_MenuStrip,   mymenus,
        // WINDOW_HelpFile,    "Docs/effects.guide",
        // WINDOW_HelpNode,    "Transparency",
        // WINDOW_HelpText,    infoblurb,
        WINDOW_CloseOnEsc,  TRUE,
        WINDOW_MasterGroup,
            VGroupObject, Spacing(4), HOffset(6), VOffset(4),
                StartMember,
                    HGroupObject, Spacing(6),
                        StartMember,
                            VGroupObject, Spacing(4),
                                StartMember,
                                    InfoObject,
                                        INFO_TextFormat, ISEQ_C "Please, do select\n"
                                                         "which color should be\n"
                                                         "transparent:",
                                        INFO_MinLines, 3,
                                        ButtonFrame, FRM_Flags, FRF_RECESSED,
                                    EndObject,
                                    LGO_NoAlign, TRUE,
                                EndMember,
                                VarSpace(25),
                                StartMember,
                                    VGroupObject,
                                        StartMember,
                                            HGroupObject, Spacing(3),
                                                StartMember,
                                                    o->Red = SliderObject,
                                                        GA_ID, GID_RED,
                                                        Label("Red"), Place(PLACE_LEFT),
                                                        SLIDER_Min, 0,
                                                        SLIDER_Max, 255,
                                                        SLIDER_Level, v->r,
                                                    EndObject,
                                                EndMember,
                                                StartMember,
                                                    o->RedI = StringObject,
                                                        GA_ID, GID_REDI,
                                                        STRINGA_MinCharsVisible,4,
                                                        STRINGA_MaxChars, 3,
                                                        STRINGA_IntegerMin, 0,
                                                        STRINGA_IntegerMax, 255,
                                                        STRINGA_LongVal, v->r,
                                                    EndObject, Weight(1),
                                                EndMember,
                                            EndObject, FixMinHeight,
                                        EndMember,
                                        StartMember,
                                            HGroupObject, Spacing(3),
                                                StartMember,
                                                    o->Green = SliderObject,
                                                        GA_ID, GID_GREEN,
                                                        Label("Green"), Place(PLACE_LEFT),
                                                        SLIDER_Min, 0,
                                                        SLIDER_Max, 255,
                                                        SLIDER_Level, v->g,
                                                    EndObject,
                                                EndMember,
                                                StartMember,
                                                    o->GreenI = StringObject,
                                                        GA_ID, GID_GREENI,
                                                        STRINGA_MinCharsVisible,4,
                                                        STRINGA_MaxChars, 3,
                                                        STRINGA_IntegerMin, 0,
                                                        STRINGA_IntegerMax, 255,
                                                        STRINGA_LongVal, v->g,
                                                    EndObject, Weight(1),
                                                EndMember,
                                            EndObject, FixMinHeight,
                                        EndMember,
                                        StartMember,
                                            HGroupObject, Spacing(3),
                                                StartMember,
                                                    o->Blue = SliderObject,
                                                        GA_ID, GID_BLUE,
                                                        Label("Blue"), Place(PLACE_LEFT),
                                                        SLIDER_Min, 0,
                                                        SLIDER_Max, 255,
                                                        SLIDER_Level, v->b,
                                                    EndObject,
                                                EndMember,
                                                StartMember,
                                                    o->BlueI = StringObject,
                                                        GA_ID, GID_BLUEI,
                                                        STRINGA_MinCharsVisible,4,
                                                        STRINGA_MaxChars, 3,
                                                        STRINGA_IntegerMin, 0,
                                                        STRINGA_IntegerMax, 255,
                                                        STRINGA_LongVal, v->b,
                                                    EndObject, Weight(1),
                                                EndMember,
                                            EndObject,  FixMinHeight,
                                        EndMember,
                                     EndObject,
                                EndMember,
                                VarSpace(25),
                            EndObject,  LGO_NoAlign, TRUE,
                        EndMember,
                        (V39) ? TAG_IGNORE : TAG_SKIP, 2,
                        StartMember,
                            o->Wheel = ExternalObject,
                                EXT_MinWidth,           30,
                                EXT_MinHeight,          30,
                                EXT_ClassID,            "colorwheel.gadget",
                                WHEEL_Screen,           scr,
                                /*
                                **      Pass a pointer to the "real" gradient slider
                                **      here.
                                **/
                                WHEEL_GradientSlider,   o->RealGradient,
                                WHEEL_Red,              v->r * 0x01010101,
                                WHEEL_Green,            v->g * 0x01010101,
                                WHEEL_Blue,             v->b * 0x01010101,
                                GA_FollowMouse,         TRUE,
                                GA_ID,                  GID_WHEEL,
                                /*
                                **      These attributes of the colorwheel are
                                **      tracked and reset to the object after
                                **      it has been rebuild. This way the current
                                **      colorwheel internals will not be lost
                                **      after the object is re-build.
                                **/
                                EXT_TrackAttr,          WHEEL_Red,
                                EXT_TrackAttr,          WHEEL_Green,
                                EXT_TrackAttr,          WHEEL_Blue,
                                EXT_TrackAttr,          WHEEL_Hue,
                                EXT_TrackAttr,          WHEEL_Saturation,
                                EXT_TrackAttr,          WHEEL_Brightness,
                            EndObject,
                        EndMember,
                        (V39) ? TAG_IGNORE : TAG_SKIP, 3,
                        StartMember,
                            o->Gradient, FixWidth(20),
                        EndMember,
                    EndObject, LGO_NoAlign, TRUE,
                EndMember,
                StartMember,
                    HGroupObject,
                        StartMember,
                            o->Trans = SliderObject,
                                GA_ID, GID_TRANSPARENCY,
                                Label("Transparency:"), Place(PLACE_LEFT),
                                SLIDER_Min, 0,
                                SLIDER_Max, 255,
                                SLIDER_Level, v->transp,
                            EndObject,
                        EndMember,
                        StartMember,
                            o->TransI = StringObject,
                                GA_ID, GID_TRANSPARENCYI,
                                STRINGA_MinCharsVisible,4,
                                STRINGA_MaxChars, 3,
                                STRINGA_IntegerMin, 0,
                                STRINGA_IntegerMax, 255,
                                STRINGA_LongVal, v->transp,
                            EndObject, Weight(1),
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
                StartMember,
                    HGroupObject, NormalSpacing,
                        StartMember,
                            o->All = CheckBoxObject,
                                GA_ID, GID_ALL,
                                Label("All colors?"),
                                GA_Selected, (v->mode == MODE_ALL) ? TRUE : FALSE,
                            EndObject, Weight(1),
                        EndMember,
                        StartMember,
                            o->Tolerance = SliderObject,
                                GA_ID, GID_TOLERANCE,
                                Label("Tolerance:"), Place(PLACE_LEFT),
                                SLIDER_Min, 0,
                                SLIDER_Max, 255,
                                SLIDER_Level, v->tolerance,
                            EndObject,
                        EndMember,
                        StartMember,
                            o->ToleranceI = StringObject,
                                GA_ID, GID_TOLERANCEI,
                                STRINGA_MinCharsVisible, 4,
                                STRINGA_MaxChars, 3,
                                STRINGA_IntegerMin, 0,
                                STRINGA_IntegerMax, 255,
                                STRINGA_LongVal, v->tolerance,
                            EndObject, Weight(1),
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
                StartMember,
                    HorizSeparator,
                EndMember,
                StartMember,
                    HGroupObject,
                        StartMember,
                            ButtonObject,
                                GA_ID, GID_OK,
                                Label("OK"), Place(PLACE_IN),
                            EndObject,
                        EndMember,
                        VarSpace(DEFAULT_WEIGHT/2),
                        StartMember,
                            ButtonObject,
                                GA_ID, GID_CANCEL,
                                Label("Cancel"), Place(PLACE_IN),
                            EndObject,
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
            EndObject,
    EndObject;

    if( o->Win ) {

        /*
         *  Attach the gadgets unto each other
         */

        AddMap( o->Trans, o->TransI, dpcol_sl2int );
        AddMap( o->TransI,o->Trans,  dpcol_int2sl );
        AddMap( o->Red, o->RedI, dpcol_sl2int );
        AddMap( o->RedI,o->Red,  dpcol_int2sl );
        AddMap( o->Green, o->GreenI, dpcol_sl2int );
        AddMap( o->GreenI,o->Green,  dpcol_int2sl );
        AddMap( o->Blue, o->BlueI, dpcol_sl2int );
        AddMap( o->BlueI,o->Blue,  dpcol_int2sl );
        AddMap( o->Tolerance, o->ToleranceI, dpcol_sl2int );
        AddMap( o->ToleranceI,o->Tolerance,  dpcol_int2sl );

        AddCondit( o->All, o->Red, GA_Selected, FALSE,
                   GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( o->All, o->RedI, GA_Selected, FALSE,
                   GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( o->All, o->Green, GA_Selected, FALSE,
                   GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( o->All, o->GreenI, GA_Selected, FALSE,
                   GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( o->All, o->Blue, GA_Selected, FALSE,
                   GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( o->All, o->BlueI, GA_Selected, FALSE,
                   GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( o->All, o->ToleranceI, GA_Selected, FALSE,
                   GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( o->All, o->Tolerance, GA_Selected, FALSE,
                   GA_Disabled, FALSE, GA_Disabled, TRUE );
        if(V39) {
            AddCondit( o->All, o->Wheel, GA_Selected, FALSE,
                       GA_Disabled, FALSE, GA_Disabled, TRUE );
            AddCondit( o->All, o->Gradient, GA_Selected, FALSE,
                       GA_Disabled, FALSE, GA_Disabled, TRUE );
        }

        if( o->win = WindowOpen(o->Win) ) {
            /* This sets the other gadgets disabled or enabled */
            SetGadgetAttrs( (struct Gadget *)o->All, o->win, NULL,
                            GA_Selected, (v->mode == MODE_ALL) ? TRUE : FALSE, TAG_DONE );
        } else {
            DisposeObject(o->Win);
        }
    }

    return (BOOL)(o->win ? PERR_OK : PERR_WINDOWOPEN);
}
///
/// GetBackGround
VOID GetBackGround( FRAME *frame, struct Values *v, struct PPTBase *PPTBase)
{
    if( frame->pix->colorspace == CS_RGB ) {
        RGBPixel cp;
        GetBackgroundColor(frame, (ROWPTR)&cp);
        v->r = cp.r;
        v->g = cp.g;
        v->b = cp.b;
    } else {
        ARGBPixel cp;
        GetBackgroundColor(frame, (ROWPTR)&cp);
        v->r = cp.r;
        v->g = cp.g;
        v->b = cp.b;
    }

    v->background = TRUE;
}
///
/// HandleIDMCP
PERROR HandleIDCMP( FRAME *frame, struct Objects *o, struct Values *v, struct PPTBase *PPTBase )
{
    ULONG sig, sigmask, sigport = 0L;
    struct Library *ColorWheelBase = o->ColorWheelBase;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    BOOL V39 = FALSE;

    if( PPTBase->lb_Gfx->LibNode.lib_Version >= 39 && ColorWheelBase )
        V39 = TRUE;

    if(StartInput(frame, GINP_PICK_POINT, NULL) == PERR_OK)
        sigport = (1 << PPTBase->mport->mp_SigBit);

    GetAttr( WINDOW_SigMask, o->Win, &sigmask );
    for(;;) {

        sig = Wait( sigport|sigmask|SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_F );

        if( sig & SIGBREAKF_CTRL_C ) {
            SetErrorCode( frame, PERR_BREAK );
            DisposeObject( o->Win );
            StopInput(frame);
            return PERR_BREAK;
        }

        if( sig & SIGBREAKF_CTRL_F ) {
            WindowToFront( o->win );
            ActivateWindow( o->win );
        }

        if( sig & sigport ) {
            struct gPointMessage *gp;

            gp = (struct gPointMessage*)GetMsg(PPTBase->mport);

            /*
             *  Ignore all other types of messages, except pickpoints.
             */

            if( gp->msg.code == PPTMSG_PICK_POINT ) {
                D(bug("User picked point (%d,%d)\n",gp->x,gp->y));

                if( frame->pix->colorspace == CS_RGB ) {
                    RGBPixel *cp;

                    cp = (RGBPixel *)GetPixelRow( frame, gp->y );
                    SetGadgetAttrs( GAD(o->Red), o->win, NULL, SLIDER_Level, cp[gp->x].r, TAG_DONE );
                    SetGadgetAttrs( GAD(o->Green), o->win, NULL, SLIDER_Level, cp[gp->x].g, TAG_DONE );
                    SetGadgetAttrs( GAD(o->Blue), o->win, NULL, SLIDER_Level, cp[gp->x].b, TAG_DONE );
                } else {
                    ARGBPixel *cp;

                    cp = (ARGBPixel *)GetPixelRow( frame, gp->y );
                    SetGadgetAttrs( GAD(o->Trans), o->win, NULL, SLIDER_Level, cp[gp->x].a, TAG_DONE );
                    SetGadgetAttrs( GAD(o->Red), o->win, NULL, SLIDER_Level, cp[gp->x].r, TAG_DONE );
                    SetGadgetAttrs( GAD(o->Green), o->win, NULL, SLIDER_Level, cp[gp->x].g, TAG_DONE );
                    SetGadgetAttrs( GAD(o->Blue), o->win, NULL, SLIDER_Level, cp[gp->x].b, TAG_DONE );
                }

                v->background = FALSE;
            }

            ReplyMsg( (struct Message *)gp );
        }

        if( sig & sigmask ) {
            ULONG rc, tmp;
            struct ColorWheelRGB rgb;
            struct ColorWheelHSB hsb;

            while(( rc = HandleEvent( o->Win )) != WMHI_NOMORE ) {
                switch(rc) {
                    case WMHI_CLOSEWINDOW:
                    case GID_CANCEL:
                        D(bug("User cancelled\n"));
                        SetErrorCode( frame, PERR_CANCELED );
                        DisposeObject( o->Win );
                        StopInput(frame);
                        return PERR_CANCELED;

                    /*
                     *  Can't happen if running under <V39, but let's be
                     *  sure anyway
                     */

                    case GID_GRADIENTSLIDER:
                        if( V39 ) {
                            GetAttr(GRAD_CurVal, o->RealGradient, &tmp);
                            // D(bug("New value: %d\n",tmp));
                            GetAttr( WHEEL_HSB, o->Wheel, (ULONG *)&hsb );
                            hsb.cw_Brightness = (0xFFFF - tmp) * 0x00010001;
                            ConvertHSBToRGB( &hsb, &rgb );
                            SetGadgetAttrs( GAD(o->Red), o->win, NULL, SLIDER_Level, rgb.cw_Red>>24, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Green), o->win, NULL, SLIDER_Level, rgb.cw_Green>>24, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Blue), o->win, NULL, SLIDER_Level, rgb.cw_Blue>>24, TAG_DONE );
                            v->background = FALSE;
                        }
                        break;

                    /*
                     *  Ditto
                     */

                    case GID_WHEEL:
                        if( V39 ) {
                            GetAttr( WHEEL_RGB, o->Wheel, (ULONG *) &rgb );
                            SetGadgetAttrs( GAD(o->Red), o->win, NULL, SLIDER_Level, rgb.cw_Red>>24, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Green), o->win, NULL, SLIDER_Level, rgb.cw_Green>>24, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Blue), o->win, NULL, SLIDER_Level, rgb.cw_Blue>>24, TAG_DONE );
                            v->background = FALSE;
                        }
                        break;

                    case GID_RED:
                        if( V39 ) {
                            GetAttr( SLIDER_Level, o->Red, (ULONG *)&tmp );
                            GetAttr( WHEEL_RGB, o->Wheel, (ULONG *) &rgb );
                            rgb.cw_Red = tmp * 0x01010101;
                            SetGadgetAttrs( GAD(o->Wheel), o->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                            v->background = FALSE;
                        }
                        break;

                    case GID_GREEN:
                        if( V39 ) {
                            GetAttr( SLIDER_Level, o->Green, (ULONG *)&tmp );
                            GetAttr( WHEEL_RGB, o->Wheel, (ULONG *) &rgb );
                            rgb.cw_Green = tmp * 0x01010101;
                            SetGadgetAttrs( GAD(o->Wheel), o->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                            v->background = FALSE;
                        }
                        break;

                    case GID_BLUE:
                        if( V39 ) {
                            GetAttr( SLIDER_Level, o->Blue, (ULONG *)&tmp );
                            GetAttr( WHEEL_RGB, o->Wheel, (ULONG *) &rgb );
                            rgb.cw_Blue = tmp * 0x01010101;
                            SetGadgetAttrs( GAD(o->Wheel), o->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                            v->background = FALSE;
                        }
                        break;

                    case GID_BACKGROUND:
                        GetBackGround( frame, v, PPTBase );

                        if( V39 ) {
                            rgb.cw_Red   = v->r * 0x01010101;
                            rgb.cw_Green = v->g * 0x01010101;
                            rgb.cw_Blue  = v->b * 0x01010101;
                            SetGadgetAttrs( GAD(o->Wheel), o->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                            DoMethod( o->Win, WM_REPORT_ID, GID_WHEEL, 0L ); // A bug in OS?
                        } else {
                            SetGadgetAttrs( GAD(o->Red), o->win, NULL, SLIDER_Level,v->r, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Green), o->win, NULL, SLIDER_Level,v->g, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Blue), o->win, NULL, SLIDER_Level,v->b, TAG_DONE );
                        }

                        break;

                    case GID_BLACK:
                        rgb.cw_Red = rgb.cw_Green = rgb.cw_Blue = 0L;
                        if( V39 ) {
                            SetGadgetAttrs( GAD(o->Wheel), o->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                            DoMethod( o->Win, WM_REPORT_ID, GID_WHEEL, 0L ); // A bug in OS?
                        } else {
                            SetGadgetAttrs( GAD(o->Red), o->win, NULL, SLIDER_Level,rgb.cw_Red>>24, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Green), o->win, NULL, SLIDER_Level,rgb.cw_Green>>24, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Blue), o->win, NULL, SLIDER_Level,rgb.cw_Blue>>24, TAG_DONE );
                        }
                        v->background = FALSE;
                        break;

                    case GID_WHITE:
                        rgb.cw_Red = rgb.cw_Green = rgb.cw_Blue = 0xFFFFFFFF;
                        if (V39 ) {
                            SetGadgetAttrs( GAD(o->Wheel), o->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                            DoMethod( o->Win, WM_REPORT_ID, GID_WHEEL, 0L ); // A bug in OS?
                        } else {
                            SetGadgetAttrs( GAD(o->Red), o->win, NULL, SLIDER_Level,rgb.cw_Red>>24, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Green), o->win, NULL, SLIDER_Level,rgb.cw_Green>>24, TAG_DONE );
                            SetGadgetAttrs( GAD(o->Blue), o->win, NULL, SLIDER_Level,rgb.cw_Blue>>24, TAG_DONE );
                        }
                        v->background = FALSE;
                        break;

                    case GID_OK:
                        D(bug("User hit OK\n"));
                        DoGadgets( frame, o, v, PPTBase );
                        GetAttr( WINDOW_Bounds, o->Win, (ULONG *)&v->window );
                        DisposeObject( o->Win );
                        StopInput(frame);
                        return PERR_OK;
                }
            }
        } /* if(sig&sigmask) */
    }
}
///
/// SetDefaults
VOID
SetDefaults( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    struct Values *opt;
    struct Values defaults = {255, 255,255,255, 0, MODE_EXACT, FALSE };

    *v = defaults;

    if( opt = GetOptions( MYNAME ) )
        *v = *opt;
}
///
/// SetRexxArgs
PERROR
SetRexxArgs( FRAME *frame, ULONG *args, struct Values *v, struct PPTBase *PPTBase )
{
    if( args[0] )
        v->r = * ((ULONG *)args[0]);
    else
        v->r = 0;

    if( args[1] )
        v->g = * ((ULONG *)args[1]);
    else
        v->g = 0;

    if( args[2] )
        v->b = * ((ULONG *)args[2]);
    else
        v->b = 0;

    if( args[3] )
        v->transp = * ((ULONG *)args[3]);
    else
        v->transp = 255;

    if( args[4] )
        GetBackGround( frame, v, PPTBase );

    if( args[5] )
        v->tolerance = * ((ULONG *)args[5]);
    else
        v->tolerance = 0;

    return PERR_OK;
}
///

/// DoGUI
PERROR DoGUI( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    struct Objects o;
    int i;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    PERROR res;

    o.GradientSliderBase = OpenLibrary("gadgets/gradientslider.gadget", 39L );
    o.ColorWheelBase = OpenLibrary("gadgets/colorwheel.gadget", 39L );

    if( (res = OpenGUI(frame, &o, v, PPTBase)) == PERR_OK ) {
        res = HandleIDCMP(frame, &o, v, PPTBase);
    }

    if( o.GradientSliderBase ) CloseLibrary( o.GradientSliderBase );
    if( o.ColorWheelBase ) CloseLibrary( o.ColorWheelBase );

    if( GfxBase->LibNode.lib_Version >= 39 ) {
        for( i = 0; i < o.numpens; i++ )
            ReleasePen( PPTBase->g->maindisp->scr->ViewPort.ColorMap, o.pens[i] );
    }

    return res;
}
///
/// EffectExec
EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    FRAME *newframe = NULL;
    struct Values  v;

    SetDefaults( frame, &v, PPTBase );

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        SetRexxArgs( frame, args, &v, PPTBase );
        newframe = DoTransparency( frame, &v, PPTBase );
    } else {

        if( DoGUI(frame, &v, PPTBase) == PERR_OK ) {
           newframe = DoTransparency( frame, &v, PPTBase );
        }

    }

    PutOptions( MYNAME, &v, sizeof(v) );

    return newframe;
}
///
/// EffectGetArgs
EFFECTGETARGS(frame, tags, PPTBase, EffectBase)
{
    ULONG *args;
    STRPTR buffer;
    struct Values v;
    PERROR res;

    SetDefaults( frame, &v, PPTBase );

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        SetRexxArgs( frame, args, &v, PPTBase );
    }

    buffer = (STRPTR)TagData(PPTX_ArgBuffer,tags);

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        SetRexxArgs( frame, args, &v, PPTBase );
    }

    /* BUG: For some reason, loses background information here. */

    if( (res = DoGUI(frame, &v, PPTBase)) == PERR_OK ) {
        SPrintF(buffer,"RED %d GREEN %d BLUE %d TRANS %d %s %s TOLERANCE %d",
                        v.r, v.g, v.b, v.transp,
                        v.background ? "BACKGROUND" : "",
                        v.mode == MODE_ALL ? "ALL" : "",
                        v.tolerance );
    }

    PutOptions( MYNAME, &v, sizeof(v) );

    return res;
}
///

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

