/*
    This is a custom BGUI class, based heavily on Jaba's sources

 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>

#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>
#include <graphics/scale.h>
#include <graphics/gfxbase.h>
#include <gadgets/colorwheel.h>
#include <gadgets/gradientslider.h>
#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/bgui.h>
#include <proto/utility.h>
#include <proto/colorwheel.h>

#include <string.h>
#include <stdio.h>

#include "colorgadget.h"


#ifdef _DCC
#define SAVEDS __geta4
#define ASM
#define REG(x) __ ## x
#define REGARGS __regargs
#else
#define SAVEDS __saveds
#define ASM __asm
#define REG(x) register __ ## x
#define REGARGS __regargs
#endif

#ifdef __SLIB
#include <dos.h>
#include "ColorClass_rev.h"

UBYTE versiontag[] = VERSTAG;
UBYTE Copyright[] = VERS " Copyright (C) 1998 Janne Jalkanen";
#endif

const ULONG dpcol_sl2int[] = { SLIDER_Level, STRINGA_LongVal, TAG_END };
const ULONG dpcol_int2sl[] = { STRINGA_LongVal, SLIDER_Level, TAG_END };

struct Library *ColorWheelBase, *GradientSliderBase;

#define OSVERSION(ver)  GfxBase->LibNode.lib_Version >= (ver)
#define GAD(x)      (( struct Gadget * )x)

#define GRADCOLORS       4
#define GADBASE          60000

#define GID_WHEEL        (GADBASE+1)
#define GID_GRADIENTSLIDER (GADBASE+2)

typedef struct {
    struct ColorWheelRGB rgb;
    Object               *ColorWheel;
    Object               *GradientSlider, *RealGradient;
    Object               *Red, *RedS, *RedI;
    Object               *Green, *GreenS, *GreenI;
    Object               *Blue, *BlueS, *BlueI;
    Object               *Sliders;
    WORD                 pens[GRADCOLORS+1];
    int                  numpens;
    struct IBox          GraphBox;
    BOOL                 usewheel;
    struct Screen        *scr;
} MD;


STATIC ASM
ULONG ColorClassNew( REG(a0) Class *cl, REG(a2) Object *obj,
                     REG(a1) struct opSet *ops )
{
    MD *md;
    ULONG rc;
    struct TagItem *tag, *tags = ops->ops_AttrList, *tstate;
    Object *label;

    kprintf("OM_NEW\n");

    /* BUG: make sure this is vertical */

    if ( rc = DoSuperMethodA( cl, obj, ( Msg )ops )) {
        md = ( MD * )INST_DATA( cl, rc );
        bzero(( char * )md, sizeof( MD ));

        kprintf("\tSetting attrs\n");

        /*
         *  Default settings
         */

        md->rgb.cw_Red = md->rgb.cw_Green = md->rgb.cw_Blue = 0L;

        /*
         *  Set up instance data
         */

        tstate = tags;
        while ( tag = NextTagItem( &tstate )) {
            switch ( tag->ti_Tag ) {
                case COLOR_RGB:
                    kprintf("\tCOLOR_RGB is set\n");
                    md->rgb = *((struct ColorWheelRGB *) tag->ti_Data);
                    break;

                case COLOR_ColorWheel:
                    md->usewheel = (BOOL)tag->ti_Data;
                    break;

                case COLOR_Screen:
                    md->scr = (struct Screen *) tag->ti_Data;
                    break;

            } /* switch() */
        } /* while() */

        /*
         *
        /*
        **  See if the object has a label
        **  attached to it.
        **/
        DoMethod(( Object * )rc, OM_GET, BT_LabelObject, &label );
        if ( label ) {
            ULONG place;
            /*
            **  Yes. Query the place because it may
            **  not be PLACE_IN for obvious reasons.
            **/
            DoMethod( label, OM_GET, LAB_Place, &place );
            if ( place == PLACE_IN )
                SetAttrs( label, LAB_Place, PLACE_LEFT, TAG_END );
        }

        /*
         *  Create the objects
         */

        kprintf("\tCreating internal objects\n");

        md->Sliders = VGroupObject,
            VarSpace(10),
            StartMember,
                md->Red = HGroupObject,
                    StartMember,
                        md->RedS = SliderObject,
                            Label("R"),
                            SLIDER_Min, 0,
                            SLIDER_Max, 255,
                            SLIDER_Level, (md->rgb.cw_Red >> 24),
                        EndObject,
                    EndMember,
                    StartMember,
                        md->RedI = StringObject,
                            STRINGA_IntegerMin, 0,
                            STRINGA_IntegerMax, 255,
                            STRINGA_MaxChars,3,
                            STRINGA_MinCharsVisible,3,
                            STRINGA_LongVal, (md->rgb.cw_Red >> 24),
                        EndObject, FixMinWidth,
                    EndMember,
                EndObject, FixMinHeight,
            EndMember,
            StartMember,
                md->Green = HGroupObject,
                    StartMember,
                        md->GreenS = SliderObject,
                            Label("G"),
                            SLIDER_Min, 0,
                            SLIDER_Max, 255,
                            SLIDER_Level, (md->rgb.cw_Red >> 24),
                        EndObject,
                    EndMember,
                    StartMember,
                        md->GreenI = StringObject,
                            STRINGA_IntegerMin, 0,
                            STRINGA_IntegerMax, 255,
                            STRINGA_MaxChars,3,
                            STRINGA_MinCharsVisible,3,
                            STRINGA_LongVal, (md->rgb.cw_Red >> 24),
                        EndObject, FixMinWidth,
                    EndMember,
                EndObject, FixMinHeight,
            EndMember,
            StartMember,
                md->Blue = HGroupObject,
                    StartMember,
                        md->BlueS = SliderObject,
                            Label("B"),
                            SLIDER_Min, 0,
                            SLIDER_Max, 255,
                            SLIDER_Level, (md->rgb.cw_Red >> 24),
                        EndObject,
                    EndMember,
                    StartMember,
                        md->BlueI = StringObject,
                            STRINGA_IntegerMin, 0,
                            STRINGA_IntegerMax, 255,
                            STRINGA_MaxChars,3,
                            STRINGA_MinCharsVisible,3,
                            STRINGA_LongVal, (md->rgb.cw_Red >> 24),
                        EndObject, FixMinWidth,
                    EndMember,
                EndObject, FixMinHeight,
            EndMember,
            VarSpace(10),
        EndObject;

        if( OSVERSION(39) && md->usewheel && ColorWheelBase && GradientSliderBase && md->scr ) {
            int i;
            struct ColorWheelHSB hsb;
            struct ColorWheelRGB rgb = {0};

            ConvertRGBToHSB( &rgb, &hsb );

            for( i = 0; i < GRADCOLORS; i++ ) {
                hsb.cw_Brightness = (ULONG)0xffffffff - (((ULONG)0xffffffff / GRADCOLORS ) * i );
                ConvertHSBToRGB( &hsb, &rgb );
                if( -1 == (md->pens[i] = ObtainBestPen( md->scr->ViewPort.ColorMap,
                                                      rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue,
                                                      TAG_DONE ) ) ) {
                    break;
                }
            }
            md->pens[i] = ~0;
            md->numpens = i;

            md->GradientSlider = ExternalObject,
                GA_ID,          GID_GRADIENTSLIDER,
                EXT_MinWidth,   10,
                EXT_MinHeight,  10,
                EXT_ClassID,    "gradientslider.gadget",
                EXT_NoRebuild,  TRUE,
                GRAD_PenArray,  md->pens,
                PGA_Freedom,    LORIENT_VERT,
            EndObject;

            if( md->GradientSlider )
                GetAttr( EXT_Object, md->GradientSlider, (ULONG *) &md->RealGradient );

            md->ColorWheel = ExternalObject,
                EXT_MinWidth,           30,
                EXT_MinHeight,          30,
                EXT_ClassID,            "colorwheel.gadget",
                WHEEL_Screen,           md->scr,
                WHEEL_GradientSlider,   md->RealGradient,
                WHEEL_Red,              md->rgb.cw_Red,
                WHEEL_Green,            md->rgb.cw_Green,
                WHEEL_Blue,             md->rgb.cw_Blue,
                GA_FollowMouse,         TRUE,
                GA_ID,                  GID_WHEEL,
                EXT_TrackAttr,          WHEEL_Red,
                EXT_TrackAttr,          WHEEL_Green,
                EXT_TrackAttr,          WHEEL_Blue,
                EXT_TrackAttr,          WHEEL_Hue,
                EXT_TrackAttr,          WHEEL_Saturation,
                EXT_TrackAttr,          WHEEL_Brightness,
            EndObject;

            kprintf("ColorWheel = %08lx, GradientSlider= %08lx, RealGrad=%08lx\n",
                     md->ColorWheel, md->GradientSlider, md->RealGradient );
        }

        if( md->Red && md->Blue && md->Green ) {
#if 0
            AddMap( md->RedS, md->RedI, dpcol_sl2int );
            AddMap( md->GreenS, md->GreenI, dpcol_sl2int );
            AddMap( md->BlueS, md->BlueI, dpcol_sl2int );

            AddMap( md->RedI, md->RedS, dpcol_int2sl );
            AddMap( md->GreenI, md->GreenS, dpcol_int2sl );
            AddMap( md->BlueI, md->BlueS, dpcol_int2sl );
#endif
            DoMethod( (Object *)rc, GRM_ADDMEMBER, md->Sliders, TAG_DONE );

            if( md->ColorWheel )
                DoMethod( (Object *)rc, GRM_ADDMEMBER, md->ColorWheel, TAG_DONE );

            if( md->GradientSlider )
                DoMethod( (Object *)rc, GRM_ADDMEMBER, md->GradientSlider, TAG_DONE );

        } else {
            kprintf("\tcreation failed\n");
            rc = 0L;
        }

        return (rc);
    }

    return( 0L );
}


/*
**  Dispose of the object.
**/
STATIC ASM
ULONG ColorClassDispose( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) Msg msg )
{
    MD *md = ( MD * )INST_DATA( cl, obj );
    int i;

    kprintf("OM_DISPOSE\n");

    if( md->GradientSlider ) {
        for( i = 0; i < md->numpens; i++ ) {
            ReleasePen( md->scr->ViewPort.ColorMap, md->pens[i] );
        }
    }

    return( DoSuperMethodA( cl, obj, msg ));
}

/*
**  Get an attribute.
**/
STATIC ASM
ULONG ColorClassGet( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct opGet *opg )
{
    MD          *md = ( MD * )INST_DATA( cl, obj );
    ULONG       rc = 1L;

    kprintf("OM_GET(%lX)\n",opg->opg_AttrID);

    switch ( opg->opg_AttrID) {
        case COLOR_RGB:
            kprintf("\tCOLOR_RGB\n");
            *( (struct ColorWheelRGB *)opg->opg_Storage ) = md->rgb;
            break;

        default:
            rc = DoSuperMethodA( cl, obj, ( Msg )opg );
            break;
    }

    return( rc );
}

/*
**  Set attributes.
**/
STATIC ASM
ULONG ColorClassSet( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct opUpdate *opu)
{
    MD              *md = ( MD * )INST_DATA( cl, obj );
    struct TagItem  *tag,*merk;
    struct BitMap   *bm;
    struct RastPort *rp=NULL;
    ULONG           rc, new;
    struct Window   *win = opu->opu_GInfo->gi_Window;
    struct Requester *req = NULL;

    kprintf("OM_SET()\n");

    /*
    **  First the superclass.
    **/

    rc = DoSuperMethodA( cl, obj, ( Msg )opu );

    /*
    **  Frame thickness change? When the window in which
    **  we are located has WINDOW_AutoAspect set to TRUE
    **  the windowclass distributes the FRM_ThinFrame
    **  attribute to the objects in the window. Here we
    **  simply intercept it to set the selected color
    **  frame thickness.
    **/
#if 0
    if ( tag = FindTagItem( FRM_ThinFrame, opu->opu_AttrList ))
        /*
        **  Set it to the frame.
        **/
        SetAttrs( md->md_Frame, FRM_ThinFrame, tag->ti_Data, TAG_END );
#endif

    if( tag = FindTagItem( COLOR_RGB, opu->opu_AttrList ) ) {
        kprintf("\tCOLOR_RGB\n");
        md->rgb = *((struct ColorWheelRGB *) tag->ti_Data );
        SetGadgetAttrs(GAD(md->RedS), win, req, SLIDER_Level, md->rgb.cw_Red >> 24);
        SetGadgetAttrs(GAD(md->GreenS), win, req, SLIDER_Level, md->rgb.cw_Green >> 24);
        SetGadgetAttrs(GAD(md->BlueS), win, req, SLIDER_Level, md->rgb.cw_Blue >> 24);
    }

    if( tag = FindTagItem( WHEEL_RGB, opu->opu_AttrList ) ) {
        kprintf("\tWHEEL_RGB\n");
        md->rgb = *((struct ColorWheelRGB *) tag->ti_Data );
        SetGadgetAttrs(GAD(md->RedS), win, req, SLIDER_Level, md->rgb.cw_Red >> 24);
        SetGadgetAttrs(GAD(md->GreenS), win, req, SLIDER_Level, md->rgb.cw_Green >> 24);
        SetGadgetAttrs(GAD(md->BlueS), win, req, SLIDER_Level, md->rgb.cw_Blue >> 24);
    }

    return( rc );
}

BOOL IsInBox(WORD x, WORD y, struct IBox *ib)
{
    if( x > ib->Left && x < ib->Left+ib->Width &&
        y > ib->Top  && y < ib->Top+ib->Height )
        return 1;

    return 0;
}


Object *WhichGadget(MD *md, WORD x, WORD y)
{
    struct IBox *bounds;

    GetAttr(BT_HitBox, md->Red, &bounds);
    if(IsInBox(x,y,bounds)) return md->Red;
    GetAttr(BT_HitBox, md->Green, &bounds);
    if(IsInBox(x,y,bounds)) return md->Green;
    GetAttr(BT_HitBox, md->Blue, &bounds);
    if(IsInBox(x,y,bounds)) return md->Blue;

    return NULL;
}


/*
**  Let's go active :)
**/
STATIC ASM
ULONG ColorClassGoActive( REG(a0) Class *cl, REG(a2) Object *obj,
                          REG(a1) struct gpInput *gpi )
{
    MD *md = ( MD * )INST_DATA( cl, obj );
    Object *w;

    kprintf("OM_GOACTIVE\n");

    w = WhichGadget(md, gpi->gpi_Mouse.X, gpi->gpi_Mouse.Y );
    if(w) {
        kprintf("\tCalling sub-object\n");
        return DoMethodA( w, (Msg) gpi );
    }

    return 0L;
}

/*
**  Handle the user input.
**/
STATIC ASM
ULONG ColorClassHandleInput( REG(a0) Class *cl, REG(a2) Object *obj,
                             REG(a1) struct gpInput *gpi )
{
    MD *md = ( MD * )INST_DATA( cl, obj );
    Object *w;

    kprintf("OM_HANDLEINPUT\n");

    w = WhichGadget(md, gpi->gpi_Mouse.X, gpi->gpi_Mouse.Y );
    if(w)
        return DoMethodA( w, (Msg) gpi );

    return 0L;
}

/*
**  Go inactive.
**/
STATIC ASM
ULONG ColorClassGoInactive( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct gpGoInactive *ggi )
{
    kprintf("OM_GOINACTIVE\n");

    return( DoSuperMethodA( cl, obj, ( Msg )ggi ));
}

/*
**  Tell'm our minimum dimensions.
**/
STATIC ASM
ULONG ColorClassDimensions( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct grmDimensions *dim )
{
    MD          *md = ( MD * )INST_DATA( cl, obj );
    ULONG            rc;

    kprintf("OM_DIMENSIONS\n");

    /*
    **  First the superclass.
    **/
    rc = DoSuperMethodA( cl, obj, ( Msg )dim );

    *( dim->grmd_MinSize.Width  ) += 80;
    *( dim->grmd_MinSize.Height ) += 40;

    return( rc );
}

/*
**  Render the object.
**/
STATIC ASM
ULONG ColorClassRender( REG(a0) Class *cl, REG(a2) Object *obj,
                        REG(a1) struct gpRender *gpr )
{
    ULONG rc, fh, fw;
    MD *md = ( MD * )INST_DATA( cl, obj );
    struct IBox *bounds;
    Object *frame;
    struct Window   *win = gpr->gpr_GInfo->gi_Window;

    kprintf("OM_RENDER\n");

    /*
     *   Let superclass render first.  If it returns zero, then we
     *   won't render either
     */

    if ( rc = DoSuperMethodA( cl, obj, ( Msg )gpr )) {
        Object *Win;

//        DoMethod( obj, OM_GET, BT_ParentWindow, &Win, TAG_DONE );
//        DoMethod( Win, WM_ADDUPDATE, GID_WHEEL, obj, NULL, TAG_DONE );
    }

    return rc;
}

STATIC DPFUNC ClassFunc[] = {
    OM_NEW,     (FUNCPTR)ColorClassNew,
    OM_GET,     (FUNCPTR)ColorClassGet,
    OM_SET,     (FUNCPTR)ColorClassSet,
    OM_DISPOSE, (FUNCPTR)ColorClassDispose,
    OM_UPDATE,  (FUNCPTR)ColorClassSet,
    GM_RENDER,  (FUNCPTR)ColorClassRender,
    DF_END
};

Class *InitColorClass(VOID)
{
    ColorWheelBase = OpenLibrary("gadgets/colorwheel.gadget",39L);
    GradientSliderBase = OpenLibrary("gadgets/gradientslider.gadget",39L);

    return BGUI_MakeClass(CLASS_SuperClassBGUI, BGUI_GROUP_GADGET,
                          CLASS_ObjectSize,     sizeof(MD),
                          CLASS_DFTable,        ClassFunc,
                          TAG_DONE );
}

BOOL FreeColorClass( Class *cl )
{
    if( ColorWheelBase ) CloseLibrary( ColorWheelBase );
    if( GradientSliderBase ) CloseLibrary( GradientSliderBase );

    return BGUI_FreeClass( cl );
}


/*
 *  The following code is only compiled for the shared library
 *  version of the class.
 */
#ifdef __SLIB
Class                   *ClassBase;

struct IntuitionBase    *IntuitionBase;
struct GfxBase          *GfxBase;
struct Library          *LayersBase;
struct Library          *UtilityBase;
struct Library          *BGUIBase;
struct Library          *BitMapBase;

/*
 *  Called each time the library is opened. It simply opens
 *  the required libraries and sets up the class.
 */
SAVEDS ASM int __UserLibInit( REG(a6) struct Library *lib )
{
    if ( IntuitionBase = ( struct IntuitionBase * )OpenLibrary( "intuition.library", 37 )) {
        if ( GfxBase = ( struct GfxBase * )OpenLibrary( "graphics.library", 37 )) {
            if ( LayersBase = OpenLibrary( "layers.library", 37 )) {
                if ( UtilityBase = OpenLibrary( "utility.library", 37 )) {
                    if ( BGUIBase = OpenLibrary( BGUINAME, 39 )) {
                        if ( ClassBase = InitColorClass() )
                            return 0;
                        CloseLibrary( BGUIBase );
                    }
                    CloseLibrary( UtilityBase );
                }
                CloseLibrary( LayersBase );
             }
            CloseLibrary(( struct Library * )GfxBase );
        }
        CloseLibrary(( struct Library * )IntuitionBase );
    }
    return( 1 );
}

/*
 *  Called each time the library is closed. It simply closes
 *  the required libraries and frees the class.
 */
SAVEDS ASM void __UserLibCleanup( REG(a6) struct Library *lib )
{
    /*
     *  Actually this can fail...
     */
    FreeColorClass( ClassBase );

    CloseLibrary( BGUIBase );
    CloseLibrary( UtilityBase );
    CloseLibrary( LayersBase );
    CloseLibrary(( struct Library * )GfxBase );
    CloseLibrary(( struct Library * )IntuitionBase );
}

/*
 *  The only callable routine in the library
 */
SAVEDS Class *LIBGetClassPtr( void )
{
    return( ClassBase );
}
#endif  /* __SLIB */
