/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : scaler

    PPT is (C) Janne Jalkanen 1995-2000.

    $Id: scale.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

#undef DEBUG_MODE

#include <pptplugin.h>
#include <string.h>

#include <math.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "Scale"

/* Need to recreate some bgui macros to use my own tag routines.
   These are not really needed for SAS/C.
*/

#define HGroupObject          MyNewObject( PPTBase, BGUI_GROUP_GADGET
#define VGroupObject          MyNewObject( PPTBase, BGUI_GROUP_GADGET, GROUP_Style, GRSTYLE_VERTICAL
#define ButtonObject          MyNewObject( PPTBase, BGUI_BUTTON_GADGET
#define CheckBoxObject        MyNewObject( PPTBase, BGUI_CHECKBOX_GADGET
#define WindowObject          MyNewObject( PPTBase, BGUI_WINDOW_OBJECT
#define SeparatorObject       MyNewObject( PPTBase, BGUI_SEPERATOR_GADGET
#define WindowOpen(wobj)      (struct Window *)DoMethod( wobj, WM_OPEN )
#define StringObject          MyNewObject( PPTBase, BGUI_STRING_GADGET
#define SliderObject          MyNewObject( PPTBase, BGUI_SLIDER_GADGET
#define CycleObject           MyNewObject( PPTBase, BGUI_CYCLE_GADGET
#define InfoObject            MyNewObject( PPTBase, BGUI_INFO_GADGET

#define GAD(x)                ((struct Gadget *)x)

#define GID_OK              1
#define GID_CANCEL          2
#define GID_WIDTHI          3
#define GID_WIDTHS          4
#define GID_HEIGHTI         5
#define GID_HEIGHTS         6
#define GID_RELWIDTHI       7
#define GID_RELWIDTHS       8
#define GID_RELHEIGHTS      9
#define GID_RELHEIGHTI      10
#define GID_MODE            11
#define GID_LOCKAR          12
#define GID_WIDTHT          13
#define GID_HEIGHTT         14
#define GID_WIDTH_DIV2      15
#define GID_WIDTH_MUL2      16
#define GID_HEIGHT_DIV2     17
#define GID_HEIGHT_MUL2     18
#define GID_HEIGHT_EQ       19
#define GID_WIDTH_EQ        20

#define GID_PRESETBASE      100
#define GID_PRESETEND       (GID_PRESETBASE+30)

/*----------------------------------------------------------------------*/
/* Internal type definitions */

typedef enum { SCALE_QUICK, SCALE_AVERAGE } SCALE_T;
typedef enum { UNIT_PIXELS, UNIT_PERCENT, UNIT_MM } UNIT_T;
/*
    Note that the LONGs below are actually just WORDs.
*/
typedef struct {
    float       neww,       /* In pixels */
                newh;
    float       relh,
                relw;
    float       mmh,
                mmw;
    SCALE_T     mode;       /* Scaling mode */
    UNIT_T      units;      /* Currently displayed units */
    BOOL        lockar;
    struct IBox winpos;
} SCALEARGS;

typedef struct {
    Object      *Win;
    struct Window *win;
    Object      *Ok, *Cancel, *Mode, *LockAR;
    Object      *WidthInteger, *HeightInteger;
    Object      *WidthType, *HeightType;
} OBJECTS;

struct Sizes {
    UWORD       width,height;
};

/*----------------------------------------------------------------------*/
/* Internal prototypes */

PERROR DoQuickScale( FRAME *src, FRAME *dest, SCALEARGS *sa, struct PPTBase *PPTBase );
PERROR DoAverageScale( FRAME *src, FRAME *dest, SCALEARGS *sa, struct PPTBase *PPTBase );
PERROR OpenGUI( FRAME *frame, OBJECTS *objs, SCALEARGS *sa,
                SCALEARGS *, SCALEARGS *,struct PPTBase *PPTBase );

/*----------------------------------------------------------------------*/
/* Global variables */

const char *mode_labels[] = {
    "Quick",
    "Average",
    NULL
};

const ULONG slider2int[] = { SLIDER_Level, STRINGA_LongVal, TAG_END };
const ULONG int2slider[] = { STRINGA_LongVal, SLIDER_Level, TAG_END };
const ULONG type2type[] = { CYC_Active, CYC_Active, TAG_END };

const char infoblurb[] =
    "Scales an image.\n"
    "Two methods (quick&average) supported\n";

const struct TagItem MyTagArray[] = {
    PPTX_Name,          (ULONG)MYNAME,
    /* Other tags go here */
    PPTX_NoNewFrame,    TRUE,
    PPTX_Author,        (ULONG)"Janne Jalkanen 1995-2000",
    PPTX_InfoTxt,       (ULONG)infoblurb,
    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,
    PPTX_RexxTemplate,  (ULONG)"NW=NEWWIDTH/N,NH=NEWHEIGHT/N,MODE/K,PERCENT/S",
    PPTX_ReqPPTVersion, 3,
#ifdef _M68020
    PPTX_CPU,           AFF_68020,
#endif

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

/// Menus and presets
const struct NewMenu scalemenus[] = {
    Title("Project"),
        Item("Cancel", "Q", GID_CANCEL ),
    Title("Presets"),
        Item("160x100", "1", (GID_PRESETBASE+0)),
        Item("320x200", "2", (GID_PRESETBASE+1)),
        Item("320x256", "3", (GID_PRESETBASE+2)),
        Item("640x400", "4", (GID_PRESETBASE+3)),
        Item("640x480", "5", (GID_PRESETBASE+4)),
        Item("640x512", "6", (GID_PRESETBASE+5)),
        Item("800x600", "7", (GID_PRESETBASE+6)),
        Item("1024x768", "8", (GID_PRESETBASE+7)),
        Item("1280x1024", "9", (GID_PRESETBASE+8)),
    End
};

const struct Sizes scalepresets[] = {
    160,100,
    320,200,
    320,256,
    640,400,
    640,480,
    640,512,
    800,600,
    1024,768,
    1280,1024
};
///

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif


/// MyNewObject
/*
    My replacement for BGUI_NewObject() - routine.
    Delete if you don't need it.
*/

Object *MyNewObject( struct PPTBase *PPTBase, ULONG classid, Tag tag1, ... )
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;

    return(BGUI_NewObjectA( classid, (struct TagItem *) &tag1));
}
///
/// Converters
/*
    Convert pixel values into percentages
*/
VOID Pixels2Percents( FRAME *frame, SCALEARGS *sa )
{
    PIXINFO *p = frame->pix;

    sa->relh = (100.0 * sa->newh) / (float)p->height;
    sa->relw = (100.0 * sa->neww) / (float)p->width;
}

/*
    Convert percentages into pixel values
*/
VOID Percents2Pixels( FRAME *frame, SCALEARGS *sa )
{
    PIXINFO *p = frame->pix;

    sa->newh = (sa->relh * (float)p->height) / 100.0;
    sa->neww = (sa->relw * (float)p->width) / 100.0;
}

VOID Pixels2Millimeters( FRAME *frame, SCALEARGS *sa )
{
    PIXINFO *p = frame->pix;

    if( p->DPIX == 0 || p->DPIY == 0 ) {
        D(bug("Error: DPIX && DPIY == 0\n"));
        return;
    }

    sa->mmh = (2.54 * sa->newh) / (float)(p->DPIY);
    sa->mmw = (2.54 * sa->neww) / (float)(p->DPIX);
}

VOID Millimeters2Pixels( FRAME *frame, SCALEARGS *sa )
{
    PIXINFO *p = frame->pix;

    sa->newh = ((float)p->DPIY * sa->mmh) / 2.54;
    sa->neww = ((float)p->DPIX * sa->mmw) / 2.54;

}

#if 0
VOID
ConvertUnits( FRAME *frame, SCALEARGS *sa )
{
    switch( sa->units ) {
        case UNIT_PIXELS:
            Pixels2Percents( frame, sa );
            Pixels2Millimeters( frame, sa );
            break;

        case UNIT_PERCENT:
            Percents2Pixels( frame, sa );
            Pixels2Millimeters( frame, sa );
            break;

        case UNIT_MM:
            Millimeters2Pixels( frame, sa );
            Pixels2Percents( frame, sa );
            break;

        default:
            break;
    }
}
#endif
///
const UBYTE *unit_labels[] = {
    "pixels",
    "percent",
    "mm",
    NULL
};

/// UpdateDisplay
/*
 *  Redraws the display
 */

VOID
UpdateDisplay( OBJECTS *objs, SCALEARGS *sa, struct PPTBase *PPTBase )
{
    float h,w;
    APTR IntuitionBase = PPTBase->lb_Intuition;
    struct TagItem tags[] = {
        STRINGA_LongVal, 0L,
        TAG_DONE, 0L
    };

    switch( sa->units ) {
        case UNIT_PIXELS:
            h = sa->newh;
            w = sa->neww;
            break;

        case UNIT_PERCENT:
            h = sa->relh;
            w = sa->relw;
            break;

        case UNIT_MM:
        default:
            h = sa->mmh;
            w = sa->mmw;
            break;
    }

    tags[0].ti_Data = (ULONG) (w);

    SetGadgetAttrsA( GAD(objs->WidthInteger), objs->win, NULL,
                     tags );

    tags[0].ti_Data = (ULONG) (h);

    SetGadgetAttrsA( GAD(objs->HeightInteger), objs->win, NULL,
                     tags );
}
///
/// OpenGUI
PERROR OpenGUI( FRAME *frame, OBJECTS *objs, SCALEARGS *sa, SCALEARGS *smin, SCALEARGS *smax, struct PPTBase *PPTBase )
{
    PERROR result = PERR_WINDOWOPEN;
    APTR IntuitionBase = PPTBase->lb_Intuition;

    objs->Win = WindowObject,
        WINDOW_Screen, PPTBase->g->maindisp->scr,
        WINDOW_Title, frame->name,
        WINDOW_ScaleWidth, 20,
        TAG_SKIP, (sa->winpos.Height == 0) ? 1 : 0,
        WINDOW_Bounds, &sa->winpos,
        WINDOW_MenuStrip, scalemenus,
        WINDOW_MasterGroup,
            VGroupObject, Spacing(4), HOffset(4), VOffset(4),

                /*
                 *  BUG: Really should have a simple blurb here
                 */

                StartMember,
                    InfoObject, NormalOffset,
                        INFO_TextFormat,ISEQ_C"Set new image size:",
                    EndObject,
                EndMember,

                /*
                 *  The two slider gadgets and their integers.
                 */
                StartMember,
                    HGroupObject, Spacing(4), NormalOffset,
                        StartMember,
                            objs->WidthInteger = StringObject, GA_ID, GID_WIDTHI,
                                RidgeFrame,
                                Label("Width"),
                                STRINGA_MinCharsVisible, 5,
                                STRINGA_MaxChars, 5,
                                STRINGA_LongVal, 0,
                                STRINGA_Justification, STRINGLEFT,
                                STRINGA_IntegerMin, 1,
                                STRINGA_IntegerMax, 32766L,
                            EndObject, FixMinWidth,
                        EndMember,
                        StartMember,
                            objs->WidthType = CycleObject, GA_ID, GID_WIDTHT,
                                CYC_Labels, unit_labels,
                                CYC_Active, sa->units,
                                CYC_Popup,  TRUE,
                                ButtonFrame,
                            EndObject,
                        EndMember,
                        StartMember,
                            ButtonObject, GA_ID, GID_WIDTH_DIV2,
                                Label("/2"),
                            EndObject, Weight(DEFAULT_WEIGHT/3),
                        EndMember,
                        StartMember,
                            ButtonObject, GA_ID, GID_WIDTH_MUL2,
                                Label("x2"),
                            EndObject, Weight(DEFAULT_WEIGHT/3),
                        EndMember,
                        StartMember,
                            ButtonObject, GA_ID, GID_WIDTH_EQ,
                                Label("=="),
                            EndObject, Weight(DEFAULT_WEIGHT/3),
                        EndMember,
                    EndObject,
                EndMember,

                StartMember,
                    HGroupObject, Spacing(4), NormalOffset,
                        StartMember,
                            objs->HeightInteger = StringObject, GA_ID, GID_HEIGHTI,
                                RidgeFrame,
                                Label( "Height" ),
                                STRINGA_MinCharsVisible, 5,
                                STRINGA_MaxChars, 5,
                                STRINGA_Justification, STRINGLEFT,
                                STRINGA_LongVal, 0,
                                STRINGA_IntegerMin, 1L,
                                STRINGA_IntegerMax, 32766L,
                            EndObject, FixMinWidth,
                        EndMember,
                        StartMember,
                            objs->HeightType = CycleObject, GA_ID, GID_HEIGHTT,
                                CYC_Labels, unit_labels,
                                CYC_Active, sa->units,
                                CYC_Popup,  TRUE,
                                ButtonFrame,
                            EndObject,
                        EndMember,
                        StartMember,
                            ButtonObject, GA_ID, GID_HEIGHT_DIV2,
                                Label("/2"),
                            EndObject, Weight(DEFAULT_WEIGHT/3),
                        EndMember,
                        StartMember,
                            ButtonObject, GA_ID, GID_HEIGHT_MUL2,
                                Label("x2"),
                            EndObject, Weight(DEFAULT_WEIGHT/3),
                        EndMember,
                        StartMember,
                            ButtonObject, GA_ID, GID_HEIGHT_EQ,
                                Label("=="),
                            EndObject, Weight(DEFAULT_WEIGHT/3),
                        EndMember,
                    EndObject,
                EndMember,

                StartMember,
                    SeparatorObject, SEP_Horiz, TRUE, EndObject,
                EndMember,

                /*
                 *  The two gadgets.
                 */

                StartMember,
                    HGroupObject, Spacing(4), NormalOffset,
                        StartMember,
                            objs->Mode = CycleObject, GA_ID, GID_MODE,
                                Label( "Mode" ),
                                CYC_Labels, mode_labels,
                                CYC_Active, sa->mode,
                                XenFrame,
                            EndObject,
                        EndMember,
                    EndObject,
                EndMember,

                StartMember,
                    HGroupObject, NormalSpacing, NormalOffset,
                        VarSpace(5),
                        StartMember,
                            objs->LockAR = CheckBoxObject, GA_ID, GID_LOCKAR,
                                Label( "Lock Aspect Ratio?" ),
                                Place(PLACE_RIGHT),
                                ButtonFrame,
                                GA_Selected, sa->lockar,
                            EndObject, FixMinHeight, FixMinWidth,
                        EndMember,
                        VarSpace(5),
                    EndObject, Weight(1),
                EndMember,

                StartMember,
                    SeparatorObject, SEP_Horiz, TRUE, EndObject,
                EndMember,

                /*
                 *  OK/Cancel gadgets
                 */

                StartMember,
                    HGroupObject, Spacing(20), NormalOffset,
                        StartMember,
                            objs->Ok = ButtonObject, GA_ID, GID_OK,
                                Label( "Ok" ), XenFrame,
                            EndObject,
                        EndMember,

                        VarSpace( DEFAULT_WEIGHT/2 ),

                        StartMember,
                            objs->Cancel = ButtonObject, GA_ID, GID_CANCEL,
                                Label( "Cancel" ), XenFrame,
                            EndObject,
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
            EndObject, /* MasterVGroup*/
        EndObject;

    if( objs->Win ) {
        /*
         *  Connect types together
         */

        AddMap( objs->WidthType, objs->HeightType, type2type );
        AddMap( objs->HeightType, objs->WidthType, type2type );

        /*
         *  Open window and return.
         */

        objs->win = (struct Window *)WindowOpen( objs->Win );

        if(objs->win) {
            UpdateDisplay(objs,sa,PPTBase);
            result = PERR_OK;
        } else {
            DisposeObject( objs->Win );
            objs->Win = NULL;
        }
    }

    return result;
}
///
/// SetHeight
VOID
SetHeight( FRAME *frame, OBJECTS *objs, SCALEARGS *sa, LONG new, UNIT_T units, struct PPTBase *PPTBase )
{
    float old;

    old = sa->newh;

    switch( units ) {
        case UNIT_PIXELS:
            sa->newh = (float)new;
            Pixels2Percents(frame,sa);
            Pixels2Millimeters(frame,sa);
            break;

        case UNIT_PERCENT:
            sa->relh = (float)new;
            Percents2Pixels(frame,sa);
            Pixels2Millimeters(frame,sa);
            break;

        case UNIT_MM:
            sa->mmh = (float)new;
            Millimeters2Pixels(frame,sa);
            Pixels2Percents(frame,sa);
            break;
    }

    if( sa->lockar ) {
        sa->neww = (sa->neww * sa->newh)/old;
        Pixels2Millimeters(frame,sa);
        Pixels2Percents(frame,sa);
        if( objs ) UpdateDisplay( objs, sa, PPTBase );
    }
}
///
/// SetWidth
VOID
SetWidth( FRAME *frame, OBJECTS *objs, SCALEARGS *sa, LONG new, UNIT_T units, struct PPTBase *PPTBase )
{
    float old;

    old = sa->neww;

    switch( units ) {
        case UNIT_PIXELS:
            sa->neww = (float)new;
            Pixels2Percents(frame,sa);
            Pixels2Millimeters(frame,sa);
            break;

        case UNIT_PERCENT:
            sa->relw = (float)new;
            Percents2Pixels(frame,sa);
            Pixels2Millimeters(frame,sa);
            break;

        case UNIT_MM:
            sa->mmw = (float)new;
            Millimeters2Pixels(frame,sa);
            Pixels2Percents(frame,sa);
            break;
    }

    if( sa->lockar ) {
        sa->newh = (sa->newh * sa->neww)/old;
        Pixels2Millimeters(frame,sa);
        Pixels2Percents(frame,sa);
        if( objs ) UpdateDisplay( objs, sa, PPTBase );
    }

}
///
/// GetScaleArgs()
/*
    Open the scale window.

    Wait for user, update gadgets
    On exit, fill scaleargs with current values and return proper error code
*/
PERROR GetScaleArgs( FRAME *frame, SCALEARGS *sa, struct PPTBase *PPTBase )
{
    APTR IntuitionBase = PPTBase->lb_Intuition, SysBase = PPTBase->lb_Sys;
    PERROR res = PERR_OK;
    SCALEARGS smin, smax;
    OBJECTS objs;
    ULONG tmp;

    /*
     *  Make sure values are current
     */

    Pixels2Percents( frame, sa );
    Pixels2Millimeters( frame, sa );

    /*
     *  Create window object
     */

    if( OpenGUI( frame, &objs, sa, &smin, &smax, PPTBase ) == PERR_OK ) {
        ULONG sigmask, sig;
        BOOL quit = FALSE;

        GetAttr( WINDOW_SigMask, objs.Win, &sigmask );

        while(!quit) {
            sig = Wait( sigmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F );

            if( sig & SIGBREAKF_CTRL_F ) {
                WindowToFront( objs.win );
                ActivateWindow( objs.win );
            }

            if( sig & SIGBREAKF_CTRL_C ) {
                quit = TRUE;
                res = PERR_BREAK;
            }

            if( sig & sigmask ) {
                ULONG rc, w, h, t;

                while( (rc = HandleEvent( objs.Win )) != WMHI_NOMORE ) {
                    switch(rc) {
                        BOOL tlock;

                        case WMHI_CLOSEWINDOW:
                        case GID_CANCEL:
                            quit = TRUE;
                            res  = PERR_CANCELED;
                            break;

                        case GID_OK:
                            quit = TRUE;
                            res  = PERR_OK;
                            GetAttr( WINDOW_Bounds, objs.Win, (ULONG *)&sa->winpos );
                            break;

                        case GID_WIDTHI:
                            GetAttr( STRINGA_LongVal, objs.WidthInteger, &w );
                            SetWidth( frame, &objs, sa, w, sa->units, PPTBase );
                            // ConvertUnits( frame, sa );
                            break;

                        case GID_HEIGHTI:
                            GetAttr( STRINGA_LongVal, objs.HeightInteger, &h );
                            SetHeight( frame, &objs, sa, h, sa->units, PPTBase );
                            // ConvertUnits( frame, sa );
                            break;

                        case GID_LOCKAR:
                            GetAttr( GA_Selected, objs.LockAR, &t );
                            sa->lockar = t;
                            break;

                        case GID_HEIGHTT:
                        case GID_WIDTHT:
                            GetAttr( CYC_Active, objs.WidthType, &w );
                            sa->units = w;
                            UpdateDisplay( &objs, sa, PPTBase );
                            break;

                        case GID_WIDTH_DIV2:
                            GetAttr( STRINGA_LongVal, objs.WidthInteger, &w );
                            SetWidth( frame, &objs, sa, w/2, sa->units, PPTBase );
                            UpdateDisplay( &objs, sa, PPTBase );
                            break;

                        case GID_WIDTH_MUL2:
                            GetAttr( STRINGA_LongVal, objs.WidthInteger, &w );
                            SetWidth( frame, &objs, sa, w*2, sa->units, PPTBase );
                            UpdateDisplay( &objs, sa, PPTBase );
                            break;

                        case GID_HEIGHT_DIV2:
                            GetAttr( STRINGA_LongVal, objs.HeightInteger, &h );
                            SetHeight( frame, &objs, sa, h/2, sa->units, PPTBase );
                            UpdateDisplay( &objs, sa, PPTBase );
                            break;

                        case GID_HEIGHT_MUL2:
                            GetAttr( STRINGA_LongVal, objs.HeightInteger, &h );
                            SetHeight( frame, &objs, sa, h*2, sa->units, PPTBase );
                            UpdateDisplay( &objs, sa, PPTBase );
                            break;

                        case GID_HEIGHT_EQ:
                            tlock = sa->lockar;
                            sa->lockar = FALSE;
                            SetHeight( frame, &objs, sa, frame->pix->height, UNIT_PIXELS, PPTBase );
                            sa->lockar = tlock;
                            UpdateDisplay( &objs, sa, PPTBase );
                            break;

                        case GID_WIDTH_EQ:
                            tlock = sa->lockar;
                            sa->lockar = FALSE;
                            SetWidth( frame, &objs, sa, frame->pix->width, UNIT_PIXELS, PPTBase );
                            sa->lockar = tlock;
                            UpdateDisplay( &objs, sa, PPTBase );
                            break;

                        default:
                            if( rc >= GID_PRESETBASE && rc <= GID_PRESETEND ) {
                                sa->newh = (float)scalepresets[rc-GID_PRESETBASE].height;
                                sa->neww = (float)scalepresets[rc-GID_PRESETBASE].width;
                                Pixels2Percents(frame,sa);
                                Pixels2Millimeters(frame,sa);
                                UpdateDisplay(&objs,sa,PPTBase);
                            }
                            break;
                    }
                }
            }
        } /* while */
    } else {
        D(bug("Window open failed\n"));
        res = PERR_WONTOPEN;
    }

    /*
     *  Read the scale mode from the window.
     */

    GetAttr( CYC_Active, objs.Mode, &tmp );
    sa->mode = (tmp == 0) ? SCALE_QUICK : SCALE_AVERAGE;

    if( objs.Win ) DisposeObject( objs.Win );

    return res;
}
///
/// ParseRxArgs
#define PEEKL(x) ( * ((ULONG *) (x) ) )
/*
    Parse REXX message
*/
PERROR ParseRxArgs( FRAME *frame, SCALEARGS *sa, ULONG *ra )
{
    sa->lockar = FALSE;

    /*
     *  First, units of arguments.
     */

    if( ra[3] ) {
        sa->units = UNIT_PERCENT;
    } else {
        sa->units = UNIT_PIXELS;
    }

    /*
     *  Check height
     */
    if( ra[1] )
        SetHeight( frame, NULL, sa, PEEKL(ra[1]), sa->units, NULL );
    else
        SetHeight( frame, NULL, sa, frame->pix->height, UNIT_PIXELS, NULL );

    /*
     *  Check width
     */

    if( ra[0] )
        SetWidth( frame, NULL, sa, PEEKL(ra[0]), sa->units, NULL );
    else
        SetWidth( frame, NULL, sa, frame->pix->width, UNIT_PIXELS, NULL );

    /*
     *  Get scaling mode
     *  May be: QUICK or AVERAGE
     */

    if( ra[2] ) {
        if( stricmp( (STRPTR)ra[2], "QUICK" ) == 0 )
            sa->mode = SCALE_QUICK;
        else {
            if( stricmp( (STRPTR)ra[2], "AVERAGE" ) == 0 ) {
                sa->mode = SCALE_AVERAGE;
            } else {
                return PERR_INVALIDARGS;
            }
        }
    } else {
        sa->mode = SCALE_QUICK;
    }

    D(bug("\tScaling mode is %s\n", (sa->mode == SCALE_QUICK) ? "QUICK" : "AVERAGE" ));

    return PERR_OK;
}
///

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData(attr, MyTagArray );
}

static
VOID SetDefaults( FRAME *frame, SCALEARGS *sa, struct PPTBase *PPTBase )
{
    SCALEARGS *saved;

    sa->newh = frame->pix->height;
    sa->neww = frame->pix->width;
    sa->units = UNIT_PIXELS;
    sa->lockar = TRUE;

    if( sa->newh <= 0 ) sa->newh = 1;
    if( sa->neww <= 0 ) sa->neww = 1;

    if( saved = GetOptions(MYNAME) )
        *sa = *saved;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *rxargs;
    SCALEARGS sa = {0}, *saved;
    PERROR res;
    STRPTR buffer;

    SetDefaults( frame, &sa, PPTBase );

    if(rxargs = (ULONG *)TagData( PPTX_RexxArgs, tags )) {
        /*
         *  Read arguments from REXX command
         */
        res = ParseRxArgs( frame, &sa, rxargs );
    } else {
        res = GetScaleArgs( frame, &sa, PPTBase );
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    SPrintF( buffer, "NEWWIDTH %lu NEWHEIGHT %lu MODE %s%s",
                      (ULONG)((sa.units == UNIT_PERCENT) ? sa.relw : sa.neww),
                      (ULONG)((sa.units == UNIT_PERCENT) ? sa.relh : sa.newh),
                      mode_labels[sa.mode],
                      (sa.units == UNIT_PERCENT) ? " PERCENT" : "" );
    return res;
}

/*
    If REXX
        Read values from command
    else
        GetScaleArgs()

    Open new frame of required size
    DoScale().
*/

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe = NULL;
    ULONG *rxargs;
    SCALEARGS sa = {0}, *saved;
    PERROR res;

    D(bug(MYNAME": Exec()\n"));

    /*
     *  Initialize and fetch any defaults for sa
     */

    SetDefaults( frame, &sa, PPTBase );

    if(rxargs = (ULONG *)TagData( PPTX_RexxArgs, tags )) {
        /*
         *  Read arguments from REXX command
         */
        res = ParseRxArgs( frame, &sa, rxargs );
    } else {
        res = GetScaleArgs( frame, &sa, PPTBase );
    }

    if(res != PERR_OK) {
        SetErrorCode( frame, res );
#ifdef _DCC
        prela4();
#endif
        return NULL;
    }

    D(bug("\tNew frame size is %d * %d\n",sa.neww, sa.newh));

    /*
     *  Allocate the new frame based on scaleargs
     */

    newframe = MakeFrame( frame );
    if(newframe) {
        newframe->pix->height = sa.newh;
        newframe->pix->width  = sa.neww;
        if( (res = InitFrame( newframe )) == PERR_OK ) {
            /*
             *  Initialize the progress displays.
             */

            InitProgress( frame, "Scaling...", 0, (ULONG)sa.newh );
            D(bug("\tInitialize OK\n"));

            /*
             *  Dispatch the correct scaler routine
             */

            switch( sa.mode ) {
                case SCALE_QUICK:
                    res = DoQuickScale( frame, newframe, &sa, PPTBase );
                    break;
                case SCALE_AVERAGE:
                    res = DoAverageScale( frame, newframe, &sa, PPTBase );
                    break;
            }

            if(res == PERR_OK) {
                FinishProgress( frame );

                /*
                 *  Update the select box
                 */

                newframe->selbox.MinX = frame->selbox.MinX * sa.neww / frame->pix->width;
                newframe->selbox.MaxX = frame->selbox.MaxX * sa.neww / frame->pix->width;
                newframe->selbox.MinY = frame->selbox.MinY * sa.newh / frame->pix->height;
                newframe->selbox.MaxY = frame->selbox.MaxY * sa.newh / frame->pix->height;
            }

        }
    }

    PutOptions(MYNAME,&sa,sizeof(SCALEARGS));

    if( res != PERR_OK ) {
        SetErrorCode( frame, res );
        if(newframe) {
            RemFrame(newframe);
            newframe = NULL;
        }
    }

    return newframe;
}



/*
    Do the actual scaling. This is the quick version, which
    actually is pretty much the same as the PPT internal quick scaler.
*/

PERROR DoQuickScale( FRAME *src, FRAME *dest, SCALEARGS *sa, struct PPTBase *PPTBase )
{
    WORD row, srow;
    WORD components = (WORD)src->pix->components;

    D(bug("\tDoQuickScale( src = %08X, dest = %08X )\n",src,dest));

    for( row = 0; row < dest->pix->height; row++ ) {
        WORD col;
        ROWPTR scp, dcp;

        if( Progress( src, row ) )
            return PERR_BREAK;

        srow = MULS16(row,src->pix->height) / dest->pix->height;
        scp = GetPixelRow( src, srow );
        dcp = GetPixelRow( dest, row );

        for( col = 0; col < dest->pix->width; col++ ) {
            LONG scol;
            LONG dcol = col * components;
            WORD comps;

            scol = components * ( MULS16(col,src->pix->width) / dest->pix->width);

            for( comps = 0; comps < components; comps++ ) {
                dcp[ dcol++ ] = scp[ scol++ ];
            }
        }
        PutPixelRow( dest, row, dcp );
    }

    return PERR_OK;
}


extern struct Library *MathIeeeDoubBasBase, *MathIeeeDoubTransBase,
               *MathIeeeSingBasBase, *MathIeeeSingTransBase;


#define SCALE 4096
#define HALFSCALE 2048

/*
    This routine is based on the netpbm distribution.
    BUG: No error checking
*/
PERROR DoAverageScale( FRAME *src, FRAME *dest, SCALEARGS *sa, struct PPTBase *PPTBase )
{
    UBYTE *xelrow = NULL, *tempxelrow, *newxelrow;
    LONG *rs = NULL, *gs = NULL, *bs = NULL, *as = NULL;
    WORD cols = src->pix->width, newcols = dest->pix->width,
         rows = src->pix->height,newrows = dest->pix->height;
    LONG rowsread, fracrowleft, needtoreadrow, fracrowtofill;
    UBYTE *xP, *nxP;
    APTR SysBase = PPTBase->lb_Sys;
    WORD col, row;
    LONG sxscale, syscale;
    UBYTE maxval = 255;
    PERROR res = PERR_OK;

    syscale = newrows * SCALE / rows;
    sxscale = newcols * SCALE / cols;

    D(bug("DoAverageScale()\n"));

    D(bug("\tsxscale = %d, syscale = %d\n", sxscale, syscale));

    tempxelrow = AllocVec( cols * src->pix->components, 0L );
    if(!tempxelrow) { res = PERR_OUTOFMEMORY; goto errorexit; }

    as = (LONG *) AllocVec( cols * sizeof(LONG), 0L );
    rs = (LONG *) AllocVec( cols * sizeof(LONG), 0L );
    gs = (LONG *) AllocVec( cols * sizeof(LONG), 0L );
    bs = (LONG *) AllocVec( cols * sizeof(LONG), 0L );
    if(!as || !rs || !gs || !bs) { res = PERR_OUTOFMEMORY; goto errorexit; }


    rowsread = 0;
    fracrowleft = syscale;
    needtoreadrow = 1;
    for ( col = 0; col < cols; ++col )
        as[col] = rs[col] = gs[col] = bs[col] = HALFSCALE;
    fracrowtofill = SCALE;

    /*
     *  Start scaling
     */

    for( row = 0; row < newrows; row++ ) {

        newxelrow = GetPixelRow( dest, row );

        if( Progress( src, row ) ) {
            res = PERR_BREAK;
            goto errorexit;
        }


        while ( fracrowleft < fracrowtofill ) {
            if ( needtoreadrow )
                if ( rowsread < rows ) {
                    xelrow = GetPixelRow( src, rowsread );
                    ++rowsread;
                    /* needtoreadrow = 0; */
                }
            switch ( src->pix->colorspace ) {
                case CS_RGB:
                    for ( col = 0, xP = xelrow; col < cols; ++col ) {
                        rs[col] += fracrowleft * *xP++;
                        gs[col] += fracrowleft * *xP++;
                        bs[col] += fracrowleft * *xP++;
                    }
                    break;

                case CS_ARGB:
                    for ( col = 0, xP = xelrow; col < cols; ++col ) {
                        as[col] += fracrowleft * *xP++;
                        rs[col] += fracrowleft * *xP++;
                        gs[col] += fracrowleft * *xP++;
                        bs[col] += fracrowleft * *xP++;
                    }
                    break;

                default:
                    for ( col = 0, xP = xelrow; col < cols; ++col, ++xP )
                        gs[col] += fracrowleft * (*xP);
                    break;
            }
            fracrowtofill -= fracrowleft;
            fracrowleft = syscale;
            needtoreadrow = 1;
        }

        /*
         *  Now fracrowleft is >= fracrowtofill, so we can produce a row.
         */

        if ( needtoreadrow )
            if ( rowsread < rows ) {
                xelrow = GetPixelRow( src, rowsread );
                ++rowsread;
                needtoreadrow = 0;
            }
        switch ( src->pix->colorspace ) {
            case CS_RGB:
                for ( col = 0, xP = xelrow, nxP = tempxelrow;
                      col < cols; ++col ) {
                    register long r, g, b;

                    r = rs[col] + fracrowtofill * *xP++;
                    g = gs[col] + fracrowtofill * *xP++;
                    b = bs[col] + fracrowtofill * *xP++;
                    r /= SCALE;
                    if ( r > maxval ) r = maxval;
                    g /= SCALE;
                    if ( g > maxval ) g = maxval;
                    b /= SCALE;
                    if ( b > maxval ) b = maxval;
                    *nxP++ = r;
                    *nxP++ = g;
                    *nxP++ = b;

                    rs[col] = gs[col] = bs[col] = HALFSCALE;
                }
                break;

            case CS_ARGB:
                for ( col = 0, xP = xelrow, nxP = tempxelrow;
                      col < cols; ++col ) {
                    register long a, r, g, b;

                    a = as[col] + fracrowtofill * *xP++;
                    r = rs[col] + fracrowtofill * *xP++;
                    g = gs[col] + fracrowtofill * *xP++;
                    b = bs[col] + fracrowtofill * *xP++;
                    a /= SCALE;
                    if ( a > maxval ) a = maxval;
                    r /= SCALE;
                    if ( r > maxval ) r = maxval;
                    g /= SCALE;
                    if ( g > maxval ) g = maxval;
                    b /= SCALE;
                    if ( b > maxval ) b = maxval;
                    *nxP++ = a;
                    *nxP++ = r;
                    *nxP++ = g;
                    *nxP++ = b;

                    as[col] = rs[col] = gs[col] = bs[col] = HALFSCALE;
                }
                break;

            default:
                for ( col = 0, xP = xelrow, nxP = tempxelrow;
                col < cols; ++col, ++xP, ++nxP ) {
                    register long g;

                    g = gs[col] + fracrowtofill * (*xP);
                    g /= SCALE;
                    if ( g > maxval ) g = maxval;
                    *nxP = g;
                    gs[col] = HALFSCALE;
                }
                break;
        }

        fracrowleft -= fracrowtofill;
        if ( fracrowleft == 0 ) {
            fracrowleft = syscale;
            needtoreadrow = 1;
        }

        fracrowtofill = SCALE;

        /* Now scale X from tempxelrow into newxelrow and write it out. */

        {
        register long a, r, g, b;
        register long fraccoltofill, fraccolleft;
        register int needcol;

        nxP = newxelrow;
        fraccoltofill = SCALE;
        a = r = g = b = HALFSCALE;
        needcol = 0;

        for ( col = 0, xP = tempxelrow; col < cols; ++col, xP+=src->pix->components ) {
            fraccolleft = sxscale;
            while ( fraccolleft >= fraccoltofill ) {
                if ( needcol ) {
                    nxP += src->pix->components;
                    a = r = g = b = HALFSCALE;
                }
                switch ( src->pix->colorspace ) {
                    case CS_RGB:
                        r += fraccoltofill * xP[0];
                        g += fraccoltofill * xP[1];
                        b += fraccoltofill * xP[2];
                        r /= SCALE;
                        if ( r > maxval ) r = maxval;
                        g /= SCALE;
                        if ( g > maxval ) g = maxval;
                        b /= SCALE;
                        if ( b > maxval ) b = maxval;
                        *nxP     = r;
                        *(nxP+1) = g;
                        *(nxP+2) = b;
                        break;

                    case CS_ARGB:
                        a += fraccoltofill * xP[0];
                        r += fraccoltofill * xP[1];
                        g += fraccoltofill * xP[2];
                        b += fraccoltofill * xP[3];

                        a /= SCALE;
                        if ( a > maxval ) a = maxval;
                        r /= SCALE;
                        if ( r > maxval ) r = maxval;
                        g /= SCALE;
                        if ( g > maxval ) g = maxval;
                        b /= SCALE;
                        if ( b > maxval ) b = maxval;
                        *nxP     = a;
                        *(nxP+1) = r;
                        *(nxP+2) = g;
                        *(nxP+3) = b;
                        break;

                    default:
                        g += fraccoltofill * *xP;
                        g /= SCALE;
                        if ( g > maxval ) g = maxval;
                        *nxP = g;
                        break;
                }
                fraccolleft -= fraccoltofill;
                fraccoltofill = SCALE;
                needcol = 1;
            }

            if ( fraccolleft > 0 ) {
                if ( needcol ) {
                    nxP += src->pix->components;
                    a = r = g = b = HALFSCALE;
                    needcol = 0;
                }
                switch ( src->pix->colorspace ) {
                    case CS_RGB:
                        r += fraccolleft * xP[0];
                        g += fraccolleft * xP[1];
                        b += fraccolleft * xP[2];
                        break;

                    case CS_ARGB:
                        a += fraccolleft * xP[0];
                        r += fraccolleft * xP[1];
                        g += fraccolleft * xP[2];
                        b += fraccolleft * xP[3];
                        break;

                    default:
                        g += fraccolleft * *xP;
                        break;
                }
                fraccoltofill -= fraccolleft;
            }
        }
        if ( fraccoltofill > 0 ) {
            xP -= src->pix->components;
            switch ( src->pix->colorspace ) {
                case CS_RGB:
                    r += fraccoltofill * xP[0];
                    g += fraccoltofill * xP[1];
                    b += fraccoltofill * xP[2];
                    break;

                case CS_ARGB:
                    a += fraccoltofill * xP[0];
                    r += fraccoltofill * xP[1];
                    g += fraccoltofill * xP[2];
                    b += fraccoltofill * xP[3];
                    break;

                default:
                    g += fraccoltofill * *xP;
                    break;
            }
        }
        if ( ! needcol ) {
            switch ( src->pix->colorspace ) {
                case CS_RGB:
                    r /= SCALE;
                    if ( r > maxval ) r = maxval;
                    g /= SCALE;
                    if ( g > maxval ) g = maxval;
                    b /= SCALE;
                    if ( b > maxval ) b = maxval;
                    nxP[0] = r;
                    nxP[1] = g;
                    nxP[2] = b;
                    break;

                case CS_ARGB:
                    a /= SCALE;
                    if ( a > maxval ) a = maxval;
                    r /= SCALE;
                    if ( r > maxval ) r = maxval;
                    g /= SCALE;
                    if ( g > maxval ) g = maxval;
                    b /= SCALE;
                    if ( b > maxval ) b = maxval;
                    nxP[0] = a;
                    nxP[1] = r;
                    nxP[2] = g;
                    nxP[3] = b;
                    break;

                default:
                    g /= SCALE;
                    if ( g > maxval ) g = maxval;
                    *nxP = g;
                    break;
            }
        }
        }
        PutPixelRow( dest, row, newxelrow );
    }

errorexit:

    if(as) FreeVec(as);
    if(rs) FreeVec(rs);
    if(gs) FreeVec(gs);
    if(bs) FreeVec(bs);

    if(tempxelrow) FreeVec(tempxelrow);

    return PERR_OK;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

