/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : text effect

    PPT and this file are (C) Janne Jalkanen 1995-1999.

    $Id: text.c,v 1.1 2001/10/25 16:23:03 jalkanen Exp $

*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/* Includes */

#define __USE_SYSBASE

//#define EXTERNAL_DEBUG
//#define SLOW_DEBUG_MODE
//#define DEBUG_MODE

#if defined(EXTERNAL_DEBUG)
#define DEBUG_MODE
#endif

#include <pptplugin.h>

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#include <exec/memory.h>
#include <graphics/gfxbase.h>

#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>
#include <libraries/asl.h>
#include <gadgets/colorwheel.h>
#include <gadgets/gradientslider.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/bgui.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/colorwheel.h>

#include <ppt.h>
#include <pragmas/pptsupp_pragmas.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Text"

#define MAXSTRINGLEN    256
#define FONTNAMELEN     40

#define GID_OK          1
#define GID_CANCEL      2
#define GID_FONTREQ     3
#define GID_STRING      4
#define GID_RED         10
#define GID_GREEN       11
#define GID_BLUE        12
#define GID_REDI        13
#define GID_GREENI      14
#define GID_BLUEI       15
#define GID_WHEEL       16
#define GID_GRADIENTSLIDER 17
#define GID_XI          18
#define GID_YI          19

#define GAD(x) ((struct Gadget *)(x))
#define GRADCOLORS      4

#define GFXV39 (GfxBase->LibNode.lib_Version >= 39)

#define RASTPORT_EXTRA_HEIGHT 4
#define RASTPORT_EXTRA_WIDTH  4

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
    "Add text to the image";

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

    PPTX_RexxTemplate,  (ULONG)"STRING/A,FONT,SIZE/N,FLAGS,TOP/N,LEFT/N,COLOR",

    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,

    PPTX_ReqPPTVersion, 6,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

const ULONG dpcol_sl2int[] = { SLIDER_Level, STRINGA_LongVal, TAG_END };
const ULONG dpcol_int2sl[] = { STRINGA_LongVal, SLIDER_Level, TAG_END };

/*
    This is a container for the internal values for
    this object.  It is saved in the Exec() routine
 */

struct Values {
    char        string[MAXSTRINGLEN+1];
    char        fontname[FONTNAMELEN+1];
    ULONG       fontsize;
    struct IBox fontreqloc;
    struct IBox textloc;
    ULONG       red,green,blue;
    UBYTE       fontstyle;
    struct IBox windowloc;
};

struct GUI {
    struct Window   *win;
    Object          *Win, *Ok, *Cancel;
    Object          *String, *Font, *FontReq;
    Object          *Red, *Green, *Blue, *Grey;
    Object          *RedI, *GreenI, *BlueI;
    Object          *ColorWheel, *Gradient, *RealGradient;
    Object          *XI, *YI;
    int             numpens;
    WORD            pens[GRADCOLORS+1];
    char            fonttext[FONTNAMELEN+6];
};

struct FlagName {
    STRPTR          name;
    ULONG           flag;
};

struct load32 {
        UWORD           l32_len;
        UWORD           l32_pen;
        ULONG           l32_red;
        ULONG           l32_grn;
        ULONG           l32_blu;
};

struct Library *DiskfontBase = NULL, *ColorWheelBase = NULL, *GradientSliderBase = NULL;

struct ExtRastPort {
    struct RastPort *RPort;
    UWORD           width, height;
};

/*----------------------------------------------------------------------*/
/* Code */

static const struct FlagName flagnames[] = {
    "ITALIC",       FSF_ITALIC,
    "BOLD",         FSF_BOLD,
    "UNDERLINED",   FSF_UNDERLINED,
    0L
};

/// Startup- and other code
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

SAVEDS ASM int __UserLibInit( REG(a6) struct Library *EffectBase )
{
    struct Library *SysBase = SYSBASE();

    if( DiskfontBase = OpenLibrary("diskfont.library", 0 ) ) {
        GradientSliderBase = OpenLibrary("gadgets/gradientslider.gadget",0L );
        ColorWheelBase = OpenLibrary("gadgets/colorwheel.gadget", 0L );
        return 0;
    }
    return 1;
}


SAVEDS ASM VOID __UserLibCleanup( REG(a6) struct Library *EffectBase )
{
    struct Library *SysBase = SYSBASE();

    if( DiskfontBase ) CloseLibrary( DiskfontBase );
    if( GradientSliderBase ) CloseLibrary( GradientSliderBase );
    if( ColorWheelBase ) CloseLibrary( ColorWheelBase );
}


EFFECTINQUIRE( attr, PPTBase, EffectBase )
{
    return TagData( attr, MyTagArray );
}
///
/// GetASLBounds()
/*
    Temporary kludge until ASLREQ_Bounds is gettable
 */
static
VOID GetASLBounds( Object *Req, struct IBox *bounds, struct PPTBase *PPTBase )
{
    LONG top, left, width, height;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;

    GetAttr( ASLREQ_Left, Req, (ULONG*) &left );
    GetAttr( ASLREQ_Top, Req, (ULONG*)&top );
    GetAttr( ASLREQ_Width, Req, (ULONG*)&width );
    GetAttr( ASLREQ_Height, Req, (ULONG*) &height );

    bounds->Left   = left;
    bounds->Top    = top;
    bounds->Width  = width;
    bounds->Height = height;
}
///

/// FreeRastPort()

VOID FreeRastPort( struct RastPort *rp, UWORD width, UWORD height, struct PPTBase *PPTBase )
{
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    int i;

    D(bug("FreeRastPort( %08X, width=%d,height=%d\n",rp,width,height));

    if( rp ) {
        if( rp->BitMap ) {
            if( GFXV39 ) {
                FreeBitMap( rp->BitMap );
            } else {
                for( i = 0; i < rp->BitMap->Depth && rp->BitMap->Planes[i]; i++ ) {
                    D(bug("\tFreeing Planes[%d] @ %08X\n",i,rp->BitMap->Planes[i]));
                    FreeRaster( rp->BitMap->Planes[i], width, height );
                }
                FreeVec( rp->BitMap );
            }
        }
        FreeVec( rp );
    }
}
///
/// AllocRastPort()
/*
 *  If depth is this will use the values from the parent rastport
 */

struct RastPort *AllocRastPort( struct RastPort *parent, UWORD width, UWORD height, UWORD depth, struct PPTBase *PPTBase )
{
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    struct RastPort *rp;
    struct BitMap *bm;
    PERROR res = PERR_OK;

    D(bug("AllocRastPort(parent=%08x,width=%d,height=%d,depth=%d\n",
           parent,width,height,depth));

    if( parent ) {
        if(depth == 0) depth = parent->BitMap->Depth;
    }

    rp = AllocVec( sizeof( struct RastPort ), MEMF_CLEAR );
    if( rp ) {

        InitRastPort( rp );

        if( GFXV39 ) {
            if( parent ) bcopy( parent, rp, sizeof(struct RastPort) );
            rp->Layer = NULL;
            rp->BitMap = AllocBitMap( width, height, depth, 0L, NULL ); // parent ? parent->BitMap : NULL );
            if( !rp->BitMap ) {
                res = PERR_OUTOFMEMORY;
            }
        } else {
            if( bm = AllocVec( sizeof( struct BitMap ), MEMF_CLEAR ) ) {
                int i;

                D(bug("\tBitmap = %08X\n",bm));

                if( parent ) bcopy( parent, rp, sizeof(struct RastPort) );

                rp->Layer  = NULL;
                rp->BitMap = bm;
                bm->Rows   = height;
                bm->Depth  = depth;
                bm->BytesPerRow = (((width+15)>>4)<<1);
                for(i = 0; i < depth; i++) {
                    bm->Planes[i] = AllocRaster( width,height );
                    D(bug("\t\tbm->Planes[%d] = %08X\n",i,bm->Planes[i]));

                    if(!bm->Planes[i]) {
                        res = PERR_OUTOFMEMORY;
                        break;
                    }
                }
            } else {
                res = PERR_OUTOFMEMORY;
            }
        }

    } else  {
        res = PERR_OUTOFMEMORY;
    }

    if( res != PERR_OK ) {
        FreeRastPort( rp, width, height, PPTBase );
        rp = NULL;
    }

    return rp;
}
///

/// AllocExtRastPort()
struct ExtRastPort *AllocExtRastPort( struct RastPort *parent, UWORD width, UWORD height, UWORD depth, struct PPTBase *PPTBase )
{
    struct ExtRastPort *erp;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    D(bug("AllocExtRastPort( parent=%lX,width=%ld,height=%ld,depth=%ld\n",parent,width,height,depth));

    if( erp = AllocVec( sizeof(struct ExtRastPort), MEMF_CLEAR ) ) {
        if(erp->RPort = AllocRastPort( parent, width, height, depth, PPTBase )) {
            erp->width  = width;
            erp->height = height;
        } else {
            FreeVec( erp );
            erp = NULL;
        }
    }
    return erp;
}
///
/// FreeExtRastPort()
VOID FreeExtRastPort( struct ExtRastPort *erp, struct PPTBase *PPTBase )
{
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    if( erp ) {
        FreeRastPort( erp->RPort, erp->width, erp->height, PPTBase );
        FreeVec( erp );
    }
}
///

/// OpenGUI
PERROR OpenGUI( FRAME *frame, struct GUI *g, struct Values *v, struct PPTBase *PPTBase )
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct ColorWheelRGB rgb;
    struct ColorWheelHSB hsb;
    struct Screen *scr = PPTBase->g->maindisp->scr;
    BOOL   wheel = FALSE;
    UBYTE  colorspace = frame->pix->colorspace;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    LONG   maxtop = frame->pix->height, maxleft = frame->pix->width;

    SPrintF( g->fonttext, "%s %lu", v->fontname, v->fontsize );

    /*
     *  Do sanity checks.  If this is just a GetArgs() call,
     *  the frame that has been given to us is just a temporary
     *  frame of zero size.
     */

    if( maxtop == 0 )  maxtop = MAX_HEIGHT;
    if( maxleft == 0 ) maxleft = MAX_WIDTH;

    if( ColorWheelBase && GradientSliderBase && colorspace != CS_GRAYLEVEL ) {
        int i;

        wheel = TRUE;
        rgb.cw_Red   = 0;
        rgb.cw_Green = 0;
        rgb.cw_Blue  = 0;
        ConvertRGBToHSB( &rgb, &hsb );

        for( i = 0; i < GRADCOLORS; i++ ) {
            hsb.cw_Brightness = (ULONG)0xffffffff - (((ULONG)0xffffffff / GRADCOLORS ) * i );
            ConvertHSBToRGB( &hsb, &rgb );
            if( -1 == (g->pens[i] = ObtainBestPen( scr->ViewPort.ColorMap,
                                                   rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue,
                                                   TAG_DONE ) ) )
            {
                break;
            }
        }
        g->pens[i] = ~0;
        g->numpens = i;

        g->Gradient = ExternalObject,
            GA_ID,          GID_GRADIENTSLIDER,
            EXT_MinWidth,   10,
            EXT_MinHeight,  10,
            EXT_ClassID,    "gradientslider.gadget",
            EXT_NoRebuild,  TRUE,
            GRAD_PenArray,  g->pens,
            PGA_Freedom,    LORIENT_VERT,
        EndObject;

        if( g->Gradient )
            GetAttr( EXT_Object, g->Gradient, (ULONG *) &g->RealGradient );

    }

    g->Win = WindowObject,
        WINDOW_Screen, scr,
        WINDOW_Font,   PPTBase->g->userprefs->mainfont,
        WINDOW_Title,  "Text:",
        WINDOW_ScreenTitle, "PPT Text.effect",
        WINDOW_ScaleWidth, 25,
        WINDOW_LockHeight, TRUE,
        v->windowloc.Height ? TAG_IGNORE : TAG_SKIP, 1,
        WINDOW_Bounds, &v->windowloc,

        WINDOW_NoBufferRP, TRUE,

        WINDOW_CloseOnEsc, TRUE,

        WINDOW_MasterGroup,
            VGroupObject, NormalOffset, NormalSpacing,
                StartMember,
                    InfoObject,
                        INFO_TextFormat, ISEQ_C
                                         "\nEnter the text and place it by moving the\n"
                                         "area around in the image\n",
                        INFO_MinLines, 4,
                        ButtonFrame,
                        FRM_Flags, FRF_RECESSED,
                    EndObject,
                EndMember,
                StartMember,
                    g->String = StringObject,
                        GA_ID, GID_STRING,
                        Label("Text:"), Place( PLACE_LEFT ),
                        STRINGA_MaxChars, MAXSTRINGLEN,
                        STRINGA_TextVal, v->string,
                    EndObject,
                EndMember,
                StartMember,
                    HGroupObject, NormalSpacing, NormalOffset,
                        StartMember,
                            HGroupObject, NarrowSpacing, NormalOffset,
                                StartMember,
                                    g->Font = InfoObject,
                                        Label("Font:"), Place( PLACE_LEFT ),
                                        INFO_TextFormat, g->fonttext,
                                        ButtonFrame, FRM_Flags, FRF_RECESSED,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    g->FontReq = ButtonObject,
                                        GA_ID, GID_FONTREQ,
                                        ButtonFrame,
                                        GetFile,
                                    EndObject, FixMinWidth,
                                EndMember,
                            EndObject,
                        EndMember,
                        StartMember,
                            /*
                             *  This is a separate object in order
                             *  to help in alignment.
                             */

                            HGroupObject, NormalSpacing, NormalOffset,
                                StartMember,
                                    g->XI = StringObject,
                                        GA_ID, GID_XI,
                                        Label("Left"),
                                        STRINGA_MinCharsVisible, 4,
                                        STRINGA_MaxChars,        5,
                                        STRINGA_IntegerMin,      -maxleft,
                                        STRINGA_IntegerMax,      maxleft,
                                        STRINGA_LongVal,         v->textloc.Left,
                                    EndObject, FixMinSize,
                                EndMember,
                                StartMember,
                                    g->YI = StringObject,
                                        GA_ID, GID_YI,
                                        Label("Top"),
                                        STRINGA_MinCharsVisible, 4,
                                        STRINGA_MaxChars,        5,
                                        STRINGA_IntegerMin,      -maxtop,
                                        STRINGA_IntegerMax,      maxtop,
                                        STRINGA_LongVal,         v->textloc.Top,
                                    EndObject, FixMinSize,
                                EndMember,
                            EndObject,
                        EndMember,
                    EndObject, LGO_Align, TRUE,
                EndMember,
                StartMember,
                    HGroupObject, NormalSpacing, NormalOffset,
                        DefaultFrame,
                        FrameTitle("Color"),
                        FRM_TitleLeft, TRUE,
                        StartMember,
                            VGroupObject, NormalSpacing, NormalOffset,
                                VarSpace(50),
                                StartMember,
                                    HGroupObject, NarrowSpacing, NormalOffset,
                                        StartMember,
                                            g->Red = SliderObject,
                                                GA_ID, GID_RED,
                                                Label( (colorspace == CS_GRAYLEVEL) ? "Grey" : "Red"), Place(PLACE_LEFT),
                                                SLIDER_Min, 0,
                                                SLIDER_Max, 255,
                                                SLIDER_Level, v->red,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            g->RedI = StringObject,
                                                GA_ID, GID_REDI,
                                                STRINGA_MinCharsVisible,4,
                                                STRINGA_MaxChars, 3,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                                STRINGA_LongVal, v->red,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject,  FixMinHeight,
                                EndMember,
                                (colorspace == CS_GRAYLEVEL) ? TAG_SKIP : TAG_IGNORE, 3,
                                StartMember,
                                    HGroupObject, NarrowSpacing, NormalOffset,
                                        StartMember,
                                            g->Green = SliderObject,
                                                GA_ID, GID_GREEN,
                                                Label("Green"), Place(PLACE_LEFT),
                                                SLIDER_Min, 0,
                                                SLIDER_Max, 255,
                                                SLIDER_Level, v->green,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            g->GreenI = StringObject,
                                                GA_ID, GID_GREENI,
                                                STRINGA_MinCharsVisible,4,
                                                STRINGA_MaxChars, 3,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                                STRINGA_LongVal, v->green,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject,  FixMinHeight,
                                EndMember,
                                (colorspace == CS_GRAYLEVEL) ? TAG_SKIP : TAG_IGNORE, 3,
                                StartMember,
                                    HGroupObject, NarrowSpacing, NormalOffset,
                                        StartMember,
                                            g->Blue = SliderObject,
                                                GA_ID, GID_BLUE,
                                                Label("Blue"), Place(PLACE_LEFT),
                                                SLIDER_Min, 0,
                                                SLIDER_Max, 255,
                                                SLIDER_Level, v->blue,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            g->BlueI = StringObject,
                                                GA_ID, GID_BLUEI,
                                                STRINGA_MinCharsVisible,4,
                                                STRINGA_MaxChars, 3,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                                STRINGA_LongVal, v->blue,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject,  FixMinHeight,
                                EndMember,
                                VarSpace(50),
                            EndObject,
                        EndMember,
                        (wheel) ? TAG_IGNORE : TAG_SKIP, 3,
                        StartMember,
                            g->ColorWheel = ExternalObject,
                                EXT_MinWidth,           30,
                                EXT_MinHeight,          30,
                                EXT_ClassID,            "colorwheel.gadget",
                                WHEEL_Screen,           scr,
                                /*
                                **      Pass a pointer to the "real" gradient slider
                                **      here.
                                **/
                                WHEEL_GradientSlider,   g->RealGradient,
                                WHEEL_Red,              v->red * 0x01010101,
                                WHEEL_Green,            v->green * 0x01010101,
                                WHEEL_Blue,             v->blue * 0x01010101,
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
                            EndObject, Weight(20),
                        EndMember,
                        (wheel) ? TAG_IGNORE : TAG_SKIP, 3,
                        StartMember,
                            g->Gradient, FixWidth(20),
                        EndMember,
                    EndObject,
                EndMember,
                StartMember,
                    HorizSeparator,
                EndMember,
                StartMember,
                    HGroupObject, NormalOffset, NormalSpacing,
                        StartMember,
                            g->Ok = ButtonObject,
                                GA_ID, GID_OK,
                                Label("Ok"),
                                XenFrame,
                            EndObject,
                        EndMember,
                        VarSpace(50),
                        StartMember,
                            g->Cancel = ButtonObject,
                                GA_ID, GID_CANCEL,
                                Label("Cancel"),
                                XenFrame,
                            EndObject,
                        EndMember,
                    EndObject,
                EndMember,
            EndObject,
    EndObject;

    if( g->Win ) {
        g->FontReq = FontReqObject,
            ASLFO_Screen,   PPTBase->g->maindisp->scr,
            FONTREQ_Name,   v->fontname,
            FONTREQ_Size,   v->fontsize,
            ASLREQ_Bounds,  &v->fontreqloc,
            ASLFO_Flags,    FOF_DOSTYLE,
            ASLFO_MaxHeight,96,
            ASLFO_InitialStyle, v->fontstyle,
            ASLFO_InitialFrontPen, 1,
        EndObject;

        AddMap( g->Red, g->RedI, dpcol_sl2int );
        AddMap( g->RedI,g->Red,  dpcol_int2sl );
        AddMap( g->Green, g->GreenI, dpcol_sl2int );
        AddMap( g->GreenI,g->Green,  dpcol_int2sl );
        AddMap( g->Blue, g->BlueI, dpcol_sl2int );
        AddMap( g->BlueI,g->Blue,  dpcol_int2sl );

        if( g->FontReq ) {
            if( g->win = WindowOpen( g->Win ) ) {
                return PERR_OK;
            }
            DisposeObject( g->FontReq );
        }

        DisposeObject( g->Win );
    }

    return PERR_WINDOWOPEN;
}
///

/// struct TextFont *OpenFontName( STRPTR name, UWORD size, UBYTE style, UBYTE flags, struct PPTBase *PPTBase )

struct TextFont *OpenFontName( STRPTR name, UWORD size, UBYTE style, UBYTE flags, struct PPTBase *PPTBase )
{
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    struct TextAttr ta;
    struct TextFont *tf;

    D(bug("OpenFontName( name=%s, size=%ld, style=%ld, flags=%ld\n",
           name,size,style,flags));

    ta.ta_Name  = name;
    ta.ta_YSize = size;
    ta.ta_Style = style;
    ta.ta_Flags = flags;

    if( !(tf = OpenFont( &ta ) )) {
        D(bug("\tOpening disk font...\n"));
        tf = OpenDiskFont( &ta );
    }

    D(bug("\tReturning TextFont @%lx\n",tf));

    return tf;
}
///

/// RenderText

VOID RenderText( struct ExtRastPort **textrp, STRPTR string, struct PPTBase *PPTBase )
{
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    struct TextFont *tf = (*textrp)->RPort->Font;
    struct TextExtent te;
    UWORD newwidth, newheight;

    D(bug("RenderText(textrp = %lx, string=%s)\n", *textrp, string ));

    TextExtent( (*textrp)->RPort, string, strlen(string), &te );

    newwidth = te.te_Extent.MinX+te.te_Extent.MaxX+1;
    newheight = te.te_Height;

    if( (newwidth >= (*textrp)->width) || (newheight >= (*textrp)->height) )
    {
        struct ExtRastPort *temp;
#if 1
        temp = AllocExtRastPort( NULL, newwidth+RASTPORT_EXTRA_WIDTH,
                                 newheight+RASTPORT_EXTRA_HEIGHT, 1, PPTBase );
#else
        temp = AllocExtRastPort( (*textrp)->RPort, newwidth+RASTPORT_EXTRA_WIDTH,
                                 newheight+RASTPORT_EXTRA_HEIGHT, 1, PPTBase );
#endif
        if( temp ) {
            FreeExtRastPort( *textrp, PPTBase );
            *textrp = temp;
        } else {
            return;
        }
    }

    EraseRect( (*textrp)->RPort, 0, 0, (*textrp)->width-1, (*textrp)->height-1 );
    Move( (*textrp)->RPort, 0, tf->tf_Baseline+2 );
    Text( (*textrp)->RPort, string, strlen(string) );

    D(bug("\tdone\n"));
}
///

/// BOOL HandleGUI( FRAME *frame, struct GUI *g, struct Values *v, struct ExtRastPort **textrp, struct PPTBase *PPTBase )


BOOL HandleGUI( FRAME *frame, struct GUI *g, struct Values *v, struct ExtRastPort **textrp, struct PPTBase *PPTBase )
{
    ULONG signals = 0L, sig, rc;
    BOOL quit = FALSE, res = FALSE;
    struct Library *SysBase = (struct Library*)PPTBase->lb_Sys;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    struct gFixRectMessage textextent = {0};
    ULONG extsigbit;
    struct TextFont *tf = NULL;
    struct TextExtent te;

    tf = OpenFontName( v->fontname, v->fontsize, v->fontstyle, 0, PPTBase );
    SetFont( (*textrp)->RPort, tf );
    SetSoftStyle( (*textrp)->RPort, v->fontstyle ^ tf->tf_Style,
                  FSF_BOLD|FSF_UNDERLINED|FSF_ITALIC );

    TextExtent( (*textrp)->RPort, v->string, strlen(v->string), &te );

    textextent.dim.Width  = te.te_Extent.MinX+te.te_Extent.MaxX+1;
    textextent.dim.Height = te.te_Height;

    StartInput( frame, GINP_FIXED_RECT, &textextent );

    SetAPen( (*textrp)->RPort, 1 );
    SetDrMd( (*textrp)->RPort, JAM1 );

    GetAttr( WINDOW_SigMask, g->Win, &signals );
    extsigbit = (1 << PPTBase->mport->mp_SigBit);

    while(!quit) {
        sig = Wait( signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | extsigbit );

        if( sig & SIGBREAKF_CTRL_C ) {
            SetErrorCode( frame, PERR_BREAK );
            res = FALSE;
            quit = TRUE;
        }

        if( sig & SIGBREAKF_CTRL_F ) {
            ActivateWindow( g->win );
            ScreenToFront( PPTBase->g->maindisp->scr );
        }

        if( sig & extsigbit ) {
            struct gFixRectMessage *gfr;

            if( gfr = (struct gFixRectMessage *)GetMsg( PPTBase->mport ) ) {

                if( gfr->msg.code == PPTMSG_FIXED_RECT ) {
                    D(bug("User picked a point @ (%d,%d)\n",gfr->x, gfr->y));
                    textextent.dim.Left = gfr->x;
                    textextent.dim.Top  = gfr->y;
                    SetGadgetAttrs( GAD(g->XI), g->win, NULL,
                                    STRINGA_LongVal, gfr->x,
                                    TAG_DONE );
                    SetGadgetAttrs( GAD(g->YI), g->win, NULL,
                                    STRINGA_LongVal, gfr->y,
                                    TAG_DONE );

                }

                ReplyMsg( (struct Message *)gfr );
            }
        }

        if( sig & signals ) {
            while(( rc = HandleEvent( g->Win ) ) != WMHI_NOMORE ) {
                char *s;
                struct TextAttr *ta;
                struct ColorWheelRGB rgb;
                struct ColorWheelHSB hsb;
                ULONG tmp;

                switch( rc ) {
                    case GID_OK:
                        D(bug("User clicked OK\n"));
                        /*
                         *  Save attributes
                         */
                        GetAttr( STRINGA_TextVal, g->String, (ULONG *)&s );
                        strncpy( v->string, s, MAXSTRINGLEN );
                        GetASLBounds( g->FontReq, &v->fontreqloc, PPTBase );
                        bcopy( &textextent.dim, &v->textloc, sizeof( struct IBox ) );
                        GetAttr( SLIDER_Level, g->Red, &v->red );
                        GetAttr( SLIDER_Level, g->Green, &v->green );
                        GetAttr( SLIDER_Level, g->Blue, &v->blue );
                        GetAttr( WINDOW_Bounds, g->Win, (ULONG *) &v->windowloc );
                        res = TRUE;
                        if( tf ) {
                            RenderText( textrp, v->string, PPTBase );
                        }
                        /*FALLTHROUGH*/
                    case GID_CANCEL:
                        quit = TRUE;
                        break;

                    case GID_FONTREQ:
                        D(bug("Opening font requester...\n"));
                        WindowBusy( g->Win );
                        if( DoRequest( g->FontReq ) != ASLREQ_OK ) {
                            D(bug("\tfailed\n"));
                            WindowReady(g->Win);
                            break;
                        }

                        WindowReady( g->Win );

                        D(bug("\tdone\n"));

                        GetAttr( FONTREQ_Name, g->FontReq, (ULONG *) &s );
                        GetAttr( FONTREQ_Size, g->FontReq, (ULONG *)&v->fontsize );
                        GetAttr( FONTREQ_TextAttr, g->FontReq, (ULONG *) &ta );
                        strncpy( v->fontname, s, FONTNAMELEN );
                        v->fontstyle = ta->ta_Style;

                        /*
                         *  Fetch the font and set it to the RastPort
                         */

                        D(bug("\tGot attrs, now finding font.\n"));

                        if(tf) CloseFont( tf ); // Close old.
                        if( !(tf = OpenFont( ta ) ) ) {
                            if( !(tf = OpenDiskFont( ta ))) {
                                SetErrorMsg(frame, "Unable to open font");
                                res  = FALSE;
                                quit = TRUE;
                                break;
                            }
                        }

                        D(bug("\tFound font at %9lX\n",tf));

                        SetFont( (*textrp)->RPort, tf );
                        SetSoftStyle( (*textrp)->RPort, ta->ta_Style ^ tf->tf_Style,
                                      FSF_BOLD|FSF_UNDERLINED|FSF_ITALIC );

                        /*
                         *  Set the font name indicator to show the
                         *  correct font.
                         *
                         *  The SetGadgetAttrs() forces a refresh, even though
                         *  the value has not been changed.
                         */

                        D(bug("\tUpdating display to reflect change.\n"));

                        SPrintF( g->fonttext, "%s %lu", v->fontname, v->fontsize );

                        SetGadgetAttrs( GAD(g->Font), g->win, NULL,
                                        INFO_TextFormat, g->fonttext, TAG_DONE );
                        /*FALLTHROUGH*/
                    case GID_STRING:

                        D(bug("String changed, recalculating size...\n"));

                        /*
                         *  Fetch the new string and check its size in pixels
                         */

                        GetAttr( STRINGA_TextVal, g->String, (ULONG *)&s );
                        strncpy( v->string, s, MAXSTRINGLEN );

                        StopInput( frame );
                        TextExtent( (*textrp)->RPort, v->string, strlen(v->string), &te );
                        textextent.dim.Width  = te.te_Extent.MinX+te.te_Extent.MaxX+1;
                        textextent.dim.Height = te.te_Height;
                        StartInput( frame, GINP_FIXED_RECT, &textextent );

                        D(bug("\tdone.\n"));

                        break;

                    case GID_GRADIENTSLIDER:
                        GetAttr(GRAD_CurVal, g->RealGradient, &tmp);
                        GetAttr( WHEEL_HSB, g->ColorWheel, (ULONG *)&hsb );
                        hsb.cw_Brightness = (0xFFFF - tmp) * 0x00010001;
                        ConvertHSBToRGB( &hsb, &rgb );
                        SetGadgetAttrs( GAD(g->Red), g->win, NULL, SLIDER_Level, rgb.cw_Red>>24, TAG_DONE );
                        SetGadgetAttrs( GAD(g->Green), g->win, NULL, SLIDER_Level, rgb.cw_Green>>24, TAG_DONE );
                        SetGadgetAttrs( GAD(g->Blue), g->win, NULL, SLIDER_Level, rgb.cw_Blue>>24, TAG_DONE );
                        break;

                    case GID_WHEEL:
                        GetAttr( WHEEL_RGB, g->ColorWheel, (ULONG *)&rgb );
                        SetGadgetAttrs( GAD(g->Red), g->win, NULL, SLIDER_Level, rgb.cw_Red>>24, TAG_DONE );
                        SetGadgetAttrs( GAD(g->Green), g->win, NULL, SLIDER_Level, rgb.cw_Green>>24, TAG_DONE );
                        SetGadgetAttrs( GAD(g->Blue), g->win, NULL, SLIDER_Level, rgb.cw_Blue>>24, TAG_DONE );
                        break;

                    case GID_RED:
                        if( g->ColorWheel ) {
                            GetAttr( SLIDER_Level, g->Red, (ULONG *)&tmp );
                            GetAttr( WHEEL_RGB, g->ColorWheel, (ULONG *) &rgb );
                            rgb.cw_Red = tmp * 0x01010101;
                            SetGadgetAttrs( GAD(g->ColorWheel), g->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                        }
                        break;

                    case GID_GREEN:
                        if( g->ColorWheel ) {
                            GetAttr( SLIDER_Level, g->Green, (ULONG *)&tmp );
                            GetAttr( WHEEL_RGB, g->ColorWheel, (ULONG *) &rgb );
                            rgb.cw_Green = tmp * 0x01010101;
                            SetGadgetAttrs( GAD(g->ColorWheel), g->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                        }
                        break;

                    case GID_BLUE:
                        if( g->ColorWheel ) {
                            GetAttr( SLIDER_Level, g->Blue, (ULONG *)&tmp );
                            GetAttr( WHEEL_RGB, g->ColorWheel, (ULONG *) &rgb );
                            rgb.cw_Blue = tmp * 0x01010101;
                            SetGadgetAttrs( GAD(g->ColorWheel), g->win, NULL, WHEEL_RGB, &rgb, TAG_DONE );
                        }
                        break;
                    case GID_XI:
                        GetAttr( STRINGA_LongVal, g->XI, (ULONG*)&tmp );
                        StopInput( frame );
                        textextent.dim.Left = tmp;
                        StartInput( frame, GINP_FIXED_RECT, &textextent );
                        break;

                    case GID_YI:
                        GetAttr( STRINGA_LongVal, g->YI, (ULONG*)&tmp );
                        StopInput( frame );
                        textextent.dim.Top = tmp;
                        StartInput( frame, GINP_FIXED_RECT, &textextent );
                        break;
                }
            }
        }
    }

    D(bug("All done, stopping input...\n"));

    StopInput( frame );

    D(bug("Closing font @ 0x%0X\n",tf));

    if( tf ) CloseFont( tf );

    return res;
}

///

/// FRAME *DoText( FRAME *frame, struct Values *v, struct RastPort *textrp, struct PPTBase *PPTBase )

/*
 *  Renders text
 */

FRAME *DoText( FRAME *frame, struct Values *v, struct ExtRastPort *textrp, struct PPTBase *PPTBase )
{
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    UBYTE *buffer;
    struct ExtRastPort *temprp;
    UBYTE cspace = frame->pix->colorspace;
    FRAME *res = frame;
    LONG temprpsize;

    D(bug("DoText(textrp=%lX)\n",textrp));

    InitProgress( frame, "Applying text...", 0, v->textloc.Height );

    // temprpsize = (((v->textloc.Width+15)>>4)<<1);
    temprpsize = (((v->textloc.Width+15)>>4)<<1);

    D(bug("Temprpsize=%ld\n",temprpsize));

    if( temprp = AllocExtRastPort( textrp->RPort, v->textloc.Width, 1, 0, PPTBase ) ) {
        if( buffer = AllocVec( v->textloc.Width + 16, MEMF_CLEAR ) ) {
            ROWPTR cp;
            WORD row;

            /*
             *  Notice the nonstandard upper limits of the for() -loops.
             */

            for( row = 0; row < v->textloc.Height+RASTPORT_EXTRA_HEIGHT-1; row++ ) {
                WORD col;

                D(bug("Row: %ld\n",row));

                if( Progress( frame, row ) ) {
                    FreeVec( buffer );
                    FreeExtRastPort( temprp, PPTBase );
                    return NULL;
                }

                /*
                 *  Boundary checks
                 */

                if( row+v->textloc.Top < 0 || row+v->textloc.Top >= frame->pix->height )
                    continue;

                cp = GetPixelRow( frame, row+v->textloc.Top );

                ReadPixelLine8( textrp->RPort, 0, row, v->textloc.Width, buffer, temprp->RPort );

                for( col = 0; col < v->textloc.Width; col++ ) {
                    /*
                     *  Check, if the bits are on in the buffer and the bit is within
                     *  the image area.
                     */

                    // D(bug("\tCol:%ld : ",col));

                    if( buffer[col] && (col+v->textloc.Left < frame->pix->width) && (col+v->textloc.Left >= 0) ) {
                        WORD xcol;

                        xcol = frame->pix->components*(col+v->textloc.Left);

                        // D(bug("1 (xcol = %ld)\n",xcol));

                        switch( cspace ) {
                            case CS_GRAYLEVEL:
                                cp[xcol] = v->red;
                                break;

                            case CS_RGB:
                                cp[xcol]   = v->red;
                                cp[xcol+1] = v->green;
                                cp[xcol+2] = v->blue;
                                break;

                            case CS_ARGB:
                                cp[xcol+1] = v->red;
                                cp[xcol+2] = v->green;
                                cp[xcol+3] = v->blue;
                                break;

                        }
                    } else {
                        // D(bug("0\n"));
                    }
                }

                PutPixelRow( frame, row+v->textloc.Top, cp );
            } /* for */

            FreeVec( buffer );
        } else {
            SetErrorMsg( frame, "Out of memory allocating temporary buffer" );
            res = NULL;
        }
        FreeExtRastPort( temprp, PPTBase );
    } else {
        SetErrorMsg( frame, "Out of memory allocating temporary memory" );
        res = NULL;
    }

    FinishProgress( frame );

    return res;
}

///


#define PEEKL(x)  (*(LONG *)(x) )
#define STR(x)    ((STRPTR)(x))

/// DecodeFlags()
ULONG DecodeFlags( const char *text, struct PPTBase *PPTBase )
{
    char *orig_text, *s;
    ULONG flags = 0;

    if(orig_text = s = strdup( text )) {
        while( s ) {
            char *d;
            int i;

            if( d = strchr( s, ',' ) ) {
                *d = '\0'; d++;
            }

            for( i = 0; flagnames[i].name; i++ ) {
                if (stricmp( s, flagnames[i].name ) == 0 ) {
                    flags |= flagnames[i].flag;
                    break;
                }
            }

            s = d;
        }
        free( orig_text );
    }

    return flags;
}
///
/// EncodeFlags
/*
    BUG: may overflow buffer
 */

VOID EncodeFlags( ULONG flags, STRPTR buffer, struct PPTBase *PPTBase )
{
    int i;

    buffer[0] = '\0'; /* Empty buffer */

    for( i = 0; flagnames[i].name; i++ ) {
        if( flagnames[i].flag & flags ) {
            strcat(buffer,flagnames[i].name);
            strcat(buffer,",");
        }
    }

    // Remove final comma
    buffer[strlen(buffer)-1] = '\0';
}
///

/// ParseRexxArgs()
PERROR ParseRexxArgs( ULONG *args, struct Values *val, struct PPTBase *PPTBase )
{
    if(!args) return PERR_ERROR;

    if( args[0] ) {
        strncpy( val->string, STR(args[0]), MAXSTRINGLEN );
    }

    if( args[1] ) {
        strncpy( val->fontname, STR(args[1]), FONTNAMELEN );
    }

    if( args[2] ) {
        val->fontsize = PEEKL( args[2] );
    }

    if( args[3] ) {
        val->fontstyle = DecodeFlags( STR(args[3]), PPTBase );
    }

    if( args[4] ) {
        val->textloc.Top = PEEKL(args[4]);
    }

    if( args[5] ) {
        val->textloc.Left = PEEKL(args[5]);
    }

    if( args[6] ) {
        sscanf( STR(args[6]), "%d,%d,%d", &val->red, &val->green, &val->blue );
    }

    return PERR_OK;
}
///
/// SetDefaults()
static
VOID SetDefaults( struct Values *val, struct PPTBase *PPTBase )
{
    bzero( val, sizeof(struct Values) );
    strcpy( val->fontname, "topaz.font" );
    val->fontsize  = 8;
    val->fontstyle = 0L;
    val->fontreqloc.Width  = 200;
    val->fontreqloc.Height = 200;
}
///

/// EffectExec()
EFFECTEXEC( frame, tags, PPTBase, ModuleBase )
{
    FRAME *newframe = NULL;
    ULONG *args;
    struct Values *opt, val = {0};
    struct GUI gui = {0};
    BOOL res = FALSE;
    struct ExtRastPort *textrp;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;

#if defined(EXTERNAL_DEBUG)
    BPTR debug_fh, orig_fh;
    debug_fh = Open("T:text.effect.debug", MODE_NEWFILE);
    orig_fh = SelectOutput( debug_fh );

    D(bug("Frame size: %ldx%ld\n",frame->pix->width,frame->pix->height));
#endif

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    } else {
        SetDefaults( &val, PPTBase );
    }

    textrp = AllocExtRastPort( NULL, 256+RASTPORT_EXTRA_WIDTH, 40+RASTPORT_EXTRA_HEIGHT, 1, PPTBase );
    if(!textrp) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return NULL;
    }

    /*
     *  Check for REXX arguments for this effect.  Every effect should be able
     *  to accept AREXX commands!
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        struct TextFont *tf;

        ParseRexxArgs( args, &val, PPTBase );

        if( tf = OpenFontName( val.fontname, val.fontsize, val.fontstyle, 0, PPTBase ) ) {
            struct TextExtent te;

            /*
             *  Do the rendering
             *  BUG: Extremely badly designed.
             */
            SetFont( textrp->RPort, tf );
            SetSoftStyle( textrp->RPort, val.fontstyle ^ tf->tf_Style,
                          FSF_BOLD|FSF_UNDERLINED|FSF_ITALIC );
            RenderText( &textrp, val.string, PPTBase );
            TextExtent( textrp->RPort, val.string, strlen(val.string), &te );
            val.textloc.Width  = te.te_Extent.MinX+te.te_Extent.MaxX+1;
            val.textloc.Height = te.te_Height;

            D(bug("Rendering text: %s\nfont=%s/%d,flags=%lu\ntop=%d,left=%d,color=%d,%d,%d\n",
                   val.string, val.fontname, val.fontsize, val.fontstyle, val.textloc.Top,
                   val.textloc.Left, val.red,val.green,val.blue));

            res = TRUE; // GO!
        } else {
            SetErrorMsg( frame, "Couldn't open specified font" );
        }

    } else {
        /*
         *  Starts up the GUI.
         */

        if( (res = OpenGUI( frame, &gui, &val, PPTBase )) == PERR_OK ) {
            res = HandleGUI( frame, &gui, &val, &textrp, PPTBase );
            D(bug("Closing windows...\n"));
            if( gui.Win ) DisposeObject( gui.Win );
            D(bug("Closing requesters...\n"));
            if( gui.FontReq ) DisposeObject( gui.FontReq );
        } else {
            SetErrorCode( frame, PERR_WINDOWOPEN );
        }
    }

    /*
     *  Do the actual effect
     */

    if( res ) {
        newframe = DoText(frame, &val, textrp, PPTBase);
    }

    /*
     *  Release resources
     *  Save the options to the PPT internal system
     */

    FreeExtRastPort( textrp, PPTBase );

    PutOptions( MYNAME, &val, sizeof(val));

#if defined(EXTERNAL_DEBUG)
    SelectOutput( orig_fh );
    Close( debug_fh );
#endif

    return newframe;
}
///

EFFECTGETARGS(frame, tags, PPTBase, EffectBase)
{
    STRPTR buffer;
    ULONG  res;
    struct Values val = {0}, *opt;
    struct GUI gui = {0};
    ULONG *origargs;
    struct ExtRastPort *textrp;
    char flagbuffer[80];

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    } else {
        SetDefaults( &val, PPTBase );
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );
    origargs = (ULONG *)TagData( PPTX_RexxArgs, tags );

    ParseRexxArgs( origargs, &val, PPTBase );

    textrp = AllocExtRastPort( NULL, 256+RASTPORT_EXTRA_WIDTH, 40+RASTPORT_EXTRA_HEIGHT, 1, PPTBase );
    if(!textrp) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return NULL;
    }

    if( (res = OpenGUI( frame, &gui, &val, PPTBase )) == PERR_OK ) {
        res = HandleGUI( frame, &gui, &val, &textrp, PPTBase );

        if( gui.Win ) DisposeObject( gui.Win );
        if( gui.FontReq ) DisposeObject( gui.FontReq );

        EncodeFlags( val.fontstyle, flagbuffer, PPTBase );
        SPrintF( buffer, "STRING \"%s\" FONT %s SIZE %ld FLAGS %s TOP %d LEFT %d COLOR %d,%d,%d",
                         val.string, val.fontname, val.fontsize, flagbuffer,
                         val.textloc.Top, val.textloc.Left,
                         val.red,val.green,val.blue );
    }

    FreeExtRastPort( textrp, PPTBase );

    return res ? PERR_OK : PERR_ERROR;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

