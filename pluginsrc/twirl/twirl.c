/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : twirl effect

    PPT and this file are (C) Janne Jalkanen 1995-1999.

    $Id: twirl.c,v 1.1 2001/10/25 16:23:03 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

// #define DEBUG_MODE

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <proto/bguifloat.h>
#include <floatgadget.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Twirl"

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif /*M_PI*/

#define DIVISOR         10
#define ANGLE_MIN       -720
#define ANGLE_MAX       720

#define SQR(x) ((x)*(x))
#define GAD(x) ((struct Gadget *) x)

#define GID_OK          1
#define GID_CANCEL      2

#define GID_VAL_START   3
#define GID_X           3
#define GID_XI          4
#define GID_Y           5
#define GID_YI          6
#define GID_RADIUS      7
#define GID_RADIUSI     8
#define GID_MODE        9
#define GID_ANGLE       10
#define GID_ANGLEF      11
#define GID_VAL_END     11

#define GID_PREVIEW_AREA 20

/*----------------------------------------------------------------------*/
/* Internal prototypes */

#define GetStr(a,b) b

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Twirls a part of the image.";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {

    PPTX_Name,          (ULONG) MYNAME,

    PPTX_NoNewFrame,    TRUE,
    PPTX_Author,        (ULONG)"Janne Jalkanen 1999-2000",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"ANGLE/A,XCENTER=CX/N,YCENTER=CY/N,RADIUS/N,INTERPOLATION=INT/K",

    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,

#ifdef _M68881
# ifdef _M68030
#  ifdef _M68060
    PPTX_CPU,           AFF_68060|AFF_68881,
#  else
    PPTX_CPU,           AFF_68030|AFF_68881,
#  endif
# endif
#endif

    PPTX_ReqPPTVersion, 6,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

typedef enum {
    NearestNeighbour = 0,
    Linear,
    Bicubic
} Interpolation_T;

const char *interp_labels[] = {
    "NearestNeighbour",
    "Linear",
#ifdef DEBUG_MODE
    "Bicubic",
#endif
    NULL
};

const ULONG dpcol_sl2int[] = { SLIDER_Level, STRINGA_LongVal, TAG_END };
const ULONG dpcol_int2sl[] = { STRINGA_LongVal, SLIDER_Level, TAG_END };
const ULONG dpcol_sl2fl[] = { SLIDER_Level, FLOAT_LongValue, TAG_END };
const ULONG dpcol_fl2sl[] = { FLOAT_LongValue, SLIDER_Level, TAG_END };

/*
    This is a container for the internal values for
    this object.  It is saved in the Exec() routine
 */

struct Values {
    float           angle;
    int             radius;
    LONG            cx,cy;
    Interpolation_T mode;
    struct IBox     window;
};

struct GUI {
    Object          *Win;
    struct Window   *win;
    Object          *SliderX, *IntegerX;
    Object          *SliderY, *IntegerY;
    Object          *SliderRadius, *IntegerRadius;
    Object          *SliderAngle,  *FloatAngle;
    Object          *Mode;

    Object          *Preview;

    struct IClass   *FloatClass;
};

/// GCC
#if defined( __GNUC__ )
const BYTE LibName[]="twirl.effect";
const BYTE LibIdString[]="twirl_effect 1.0";
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
///

/*----------------------------------------------------------------------*/
/* Code */

/// Misc stuff.
#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

/*
    My replacement for BGUI_NewObject() - routine. It is just a simple
    varargs stub.

    Delete if you don't need it.
*/

Object *MyNewObject( struct PPTBase *PPTBase, ULONG classid, Tag tag1, ... )
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;

    return(BGUI_NewObjectA( classid, (struct TagItem *) &tag1));
}


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
///

/// DoTwirl
FRAME *DoTwirl( FRAME *src, FRAME *dest, struct Values *val, struct PPTBase *PPTBase )
{
    int rows = src->pix->height, cols = src->pix->width;
    int row, col;
    float fangle;
    WORD comps = src->pix->components;

    D(bug("DoTwirl()\n"));

    /*
     *  Calculate the rotating angle
     */

    fangle = (float)val->angle * M_PI / 180.0;     /* convert to radians */

    InitProgress( src, GetStr(MSG_TWIRLING,"Twirling..."), 0, dest->pix->height );

    D(bug("Twirling.  Angle=%.2f (=%.5f rad), radius=%lu. (cx,cy)=(%ld,%ld)\n",
           val->angle, fangle, val->radius, val->cx, val->cy ));

    /*
     *  Do the rotating
     */

    for( row = 0; row < rows; row++ ) {
        ROWPTR cp;

        cp = GetPixelRow( dest, row );

        if(Progress( src, row )) {
            RemFrame(dest);
            SetErrorCode(src,PERR_BREAK);
            return NULL;
        }

        for( col = 0; col < cols; col++ ) {
            float x,y, c_sin, c_cos, dist, xx, yy;
            int i;

            xx = (float)(col - val->cx);
            yy = (float)(row - val->cy);

            dist = sqrt( xx*xx + yy*yy );

            if( dist < val->radius ) {
                float realangle;

                realangle = fangle * (1.0 - dist/val->radius);

                c_sin = sin( realangle );
                c_cos = cos( realangle );

                x = (xx) * c_cos + (yy) * c_sin + val->cx;
                y = -(xx) * c_sin + (yy) * c_cos + val->cy;

                /*
                D(bug("Dist=%.2f => realangle=%.3f. Mapping (%.2f,%.2f) to (%d,%d)\n",
                       dist, realangle, x,y,col,row));
                */

                if( x < -0.49 || y < -0.49 || x > (cols-1) || y > (rows-1) ) {
                    for( i = 0; i < comps; i++ ) {
                        cp[comps*col+i] = 0; /* Black */
                    }
                } else {
                    ROWPTR scp, scp2[5];
                    float a, b, dist;
                    UWORD grows;
                    float sum;
                    int i,j;

                    switch(val->mode) {
                        int xx,yy;

                        case NearestNeighbour:
                            scp = GetPixelRow( src, (int) (y+0.5) );
                            for(i = 0; i < comps; i++) {
                                cp[comps*col+i] = scp[(int)(x+0.5)*comps+i];
                            }
                            break;

                        case Linear:
                            a = x - (int)(x);
                            b = y - (int)(y);
                            grows = GetNPixelRows( src, scp2, (int) (y),2 );
                            xx = (int)(x+0.5);
                            yy = (int)(x+0.5);

                            if(grows == 2) {
                                for(i = 0; i < comps; i++) {
                                    cp[comps*col+i] = (1.0-a)*(1.0-b) * scp2[0][xx*comps+i]
                                            + a*(1.0-b) * scp2[0][(xx+1)*comps+i]
                                            + b*(1.0-a) * scp2[1][(xx)*comps+i]
                                            + a*b*scp2[1][(int)(xx+1)*comps+i];
                                }
                            } else {
                                for(i = 0; i < comps; i++ ) {
                                    cp[col] = (1.0-a)*(1.0-b) * scp2[0][(int)(x+0.5)*comps+i]
                                            + a*(1.0-b) * scp2[0][(int)(x+1.5)*comps+i];
                                }
                            }
                            break;

#if defined(BICUBIC_SUPPORTED)
                        case Bicubic:
                            a = x - (int)(x);
                            b = y - (int)(y);
                            sum = 0.0;
                            grows = GetNPixelRows( src, scp2, (int)(y)-1, 4 );
                            for( i = -1; i <= 2; i++ ) {
                                for(j = -1; j <= 2; j++ ) {
                                    if( scp2[i+1] ) {
                                        float h3, alpha;

                                        if( fabs(j-a) > 0.001 ) // BUG: Arbitrary epsilon
                                            alpha = atan( fabs(i-b) / fabs(j-a) );
                                        else
                                            alpha = M_PI/2;

                                        if( alpha > M_PI/4 ) alpha = M_PI/2-alpha;

                                        dist = sqrt( SQR(a-j) + SQR(b-i) ) * cos(alpha);
                                        // dist = sqrt( SQR(j-a) + SQR(i-b) );
                                        // dist = MAX( (abs(j-a) ),(abs(i-b)) );

                                        if( dist < 1.0 ) {
                                            h3 = 1.0 - 2.0 * SQR(dist) + dist*dist*dist;
                                        } else {
                                            if( dist < 2.0 ) {
                                                h3 = 4.0 - 8.0*dist + 5.0*SQR(dist) - dist*dist*dist;
                                            } else {
                                                h3 = 0.0;
                                            }
                                        }

                                        sum += (h3 * (float)scp2[i+1][(int)(x+0.5)+j]);
                                    }
                                }
                            }

                            // D(bug("Sum is %lf\n",sum));

                            if( sum > 255.0 )
                                cp[col] = 255;
                            else if( sum < 0.0 )
                                cp[col] = 0;
                            else
                                cp[col] = (UBYTE)sum;
                            break;
#endif
                    } /* switch(mode) */

                }

            } else { /* is in radius? */
                ROWPTR cp;

                cp = GetPixel( src, row, col );
                PutPixel( dest, row, col, cp );
            }
        }

        PutPixelRow( dest, row, cp );
    } /* for row */

    FinishProgress( src );

    return dest;
}
///

/// Twirl
FRAME *Twirl( FRAME *src, struct Values *val, struct PPTBase *PPTBase )
{
    FRAME *dest;

    D(bug("DoRotate()\n"));

    /*
     *  Create the new image
     */

    dest = MakeFrame(src);

    if( InitFrame( dest ) != PERR_OK ) {
        RemFrame( dest );
        return NULL;
    }

    dest = DoTwirl( src, dest, val, PPTBase );

    return dest;
}
///

/// OpenGUI
BOOL OpenGUI( FRAME *frame, struct Values *val, struct GUI *g, BOOL preview, struct PPTBase *PPTBase )
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;
    struct Screen *scr = PPTBase->g->maindisp->scr;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;

    ULONG width = frame->pix->width;
    ULONG height = frame->pix->width;

    // For da preview-less pics.
    if( width == 0 ) width = MAX_WIDTH;
    if( height == 0 ) height = MAX_HEIGHT;

    g->Win = WindowObject,
        WINDOW_Screen, scr,
        WINDOW_Title, frame->name,
        // WINDOW_ScreenTitle, "Twirladiduu",
        val->window.Height ? TAG_IGNORE : TAG_SKIP, 1,
        WINDOW_Bounds, &val->window,
        WINDOW_ScaleWidth, 30,
        WINDOW_LockHeight, TRUE,
        WINDOW_CloseOnEsc, TRUE,
        WINDOW_MasterGroup,
            VGroupObject, NormalSpacing, NormalOffset,
                StartMember,
                    InfoObject,
                        INFO_TextFormat, GetStr(MSG_CHOOSE_VALUES,"\033c\033bTwirl\n\033n\nChoose values.  You can also\nuse the mouse to pick them from\nthe frame."),
                        INFO_MinLines, 5,
                        ButtonFrame, FRM_Flags, FRF_RECESSED,
                    EndObject,
                EndMember,

                StartMember,
                    HGroupObject, NormalSpacing, NormalOffset,
                    preview ? TAG_IGNORE : TAG_SKIP, 4,
                    StartMember,
                        g->Preview = AreaObject,
                            AREA_MinWidth, PPTBase->g->userprefs->previewwidth,
                            AREA_MinHeight, PPTBase->g->userprefs->previewheight,
                            GA_ID, GID_PREVIEW_AREA,
                            StringFrame,
                        EndObject, FixMinSize,
                    EndMember,
                    StartMember,
                            VGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                                StartMember,
                                    HGroupObject, NarrowSpacing,
                                        StartMember,
                                            g->SliderX = SliderObject,
                                                GA_ID, GID_X,
                                                Label("X"), Place(PLACE_LEFT),
                                                SLIDER_Min,  0,
                                                SLIDER_Max,  width,
                                                SLIDER_Level, val->cx,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            g->IntegerX = StringObject,
                                                GA_ID, GID_XI,
                                                STRINGA_MinCharsVisible, 6,
                                                STRINGA_MaxChars,        5,
                                                STRINGA_IntegerMin,      0,
                                                STRINGA_IntegerMax,      width,
                                                STRINGA_LongVal,         val->cx,
                                            EndObject, FixMinWidth,
                                        EndMember,
                                    EndObject,
                                EndMember,

                                StartMember,
                                    HGroupObject, NarrowSpacing,
                                        StartMember,
                                            g->SliderY = SliderObject,
                                                GA_ID, GID_Y,
                                                Label("Y"), Place(PLACE_LEFT),
                                                SLIDER_Min,  0,
                                                SLIDER_Max,  height,
                                                SLIDER_Level, val->cy,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            g->IntegerY = StringObject,
                                                GA_ID, GID_YI,
                                                STRINGA_MinCharsVisible, 6,
                                                STRINGA_MaxChars,        5,
                                                STRINGA_IntegerMin,      0,
                                                STRINGA_IntegerMax,      height,
                                                STRINGA_LongVal,         val->cy,
                                            EndObject, FixMinWidth,
                                        EndMember,
                                    EndObject,
                                EndMember,

                                StartMember,
                                    HGroupObject, NarrowSpacing,
                                        StartMember,
                                            g->SliderRadius = SliderObject,
                                                GA_ID, GID_RADIUS,
                                                Label( GetStr(MSG_RADIUS,"Radius") ), Place(PLACE_LEFT),
                                                SLIDER_Min,  0,
                                                SLIDER_Max,  1000, // BUG!
                                                SLIDER_Level, val->radius,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            g->IntegerRadius = StringObject,
                                                GA_ID, GID_RADIUSI,
                                                STRINGA_MinCharsVisible, 6,
                                                STRINGA_MaxChars,        5,
                                                STRINGA_IntegerMin,      0,
                                                STRINGA_IntegerMax,      1000,
                                                STRINGA_LongVal,         val->radius,
                                            EndObject, FixMinWidth,
                                        EndMember,
                                    EndObject,
                                EndMember,

                                StartMember,
                                    HGroupObject, NarrowSpacing,
                                        StartMember,
                                            g->SliderAngle = SliderObject,
                                                GA_ID, GID_ANGLE,
                                                Label( GetStr(MSG_ANGLE,"Angle") ), Place(PLACE_LEFT),
                                                SLIDER_Min,  ANGLE_MIN * DIVISOR,
                                                SLIDER_Max,  ANGLE_MAX * DIVISOR,
                                                SLIDER_Level, (ULONG)(val->angle * DIVISOR),
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            g->FloatAngle = NewObject( g->FloatClass, NULL,
                                                GA_ID, GID_ANGLEF,
                                                STRINGA_MinCharsVisible, 6,
                                                STRINGA_MaxChars,        5,
                                                FLOAT_LongValue,         (ULONG)(val->angle * DIVISOR),
                                                FLOAT_LongMin,           ANGLE_MIN*DIVISOR,
                                                FLOAT_LongMax,           ANGLE_MAX*DIVISOR,
                                                FLOAT_Divisor,           DIVISOR,
                                                FLOAT_Format,            "%.2f",
                                                // STRINGA_Justification,   STRINGRIGHT,
                                            EndObject, FixMinWidth,
                                        EndMember,
                                    EndObject,
                                EndMember,

                                StartMember,
                                    g->Mode = CycleObject,
                                        GA_ID, GID_MODE,
                                        Label("Mode"), Place(PLACE_LEFT),
                                        ButtonFrame,
                                        CYC_Active,    val->mode,
                                        CYC_Labels,    interp_labels,
                                        CYC_Popup,     TRUE,
                                    EndObject,
                                EndMember,

                            EndObject,
                        EndMember,
                     EndObject,
                EndMember,
                StartMember,
                    HorizSeparator,
                EndMember,
                StartMember,
                    HGroupObject, NormalSpacing, NormalOffset,
                        StartMember,
                            ButtonObject,
                                GA_ID, GID_OK,
                                Label( GetStr(MSG_OK,"OK") ), Place(PLACE_IN),
                            EndObject,
                        EndMember,
                        VarSpace(DEFAULT_WEIGHT/2),
                        StartMember,
                            ButtonObject,
                                GA_ID, GID_CANCEL,
                                Label( GetStr(MSG_CANCEL,"Cancel") ), Place(PLACE_IN),
                            EndObject,
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
            EndObject,

    EndObject;

    if( g->Win ) {

        AddMap( g->SliderX, g->IntegerX, dpcol_sl2int );
        AddMap( g->IntegerX,g->SliderX,  dpcol_int2sl );

        AddMap( g->SliderY, g->IntegerY, dpcol_sl2int );
        AddMap( g->IntegerY,g->SliderY,  dpcol_int2sl );

        AddMap( g->SliderRadius, g->IntegerRadius, dpcol_sl2int );
        AddMap( g->IntegerRadius,g->SliderRadius,  dpcol_int2sl );

        AddMap( g->SliderAngle, g->FloatAngle,  dpcol_sl2fl );
        AddMap( g->FloatAngle,  g->SliderAngle, dpcol_fl2sl );

        if( g->win = WindowOpen( g->Win ) ) {
            return TRUE;
        } else {
            DisposeObject( g->Win );
        }
    }

    return FALSE;
}
///

/// DoGadgets
VOID DoGadgets( FRAME *frame, struct GUI *gui, struct Values *val, struct PPTBase *PPTBase)
{
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    LONG tmp;

    GetAttr( SLIDER_Level, gui->SliderX, (ULONG *) &val->cx);
    GetAttr( SLIDER_Level, gui->SliderY, (ULONG *) &val->cy);
    GetAttr( SLIDER_Level, gui->SliderRadius, (ULONG *) &val->radius);

    GetAttr( CYC_Active, gui->Mode, (ULONG *)&val->mode );

    GetAttr( FLOAT_LongValue, gui->FloatAngle, (ULONG *)&tmp );
    val->angle = (float)tmp / (float)DIVISOR;
}
///
/// HandleIDCMP
PERROR HandleIDCMP( FRAME *frame, struct Values *val, struct GUI *g, struct PPTBase *PPTBase )
{
    ULONG sig, sigmask, sigport = 0L;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    FRAME *pwframe = NULL, *tmpframe = NULL;
    PERROR res = PERR_OK;

    if( pwframe = ObtainPreviewFrame( frame, NULL ) ) {
        tmpframe = DupFrame( pwframe, DFF_COPYDATA );
        DoTwirl( pwframe, tmpframe, val, PPTBase );

        if(StartInput(frame, GINP_PICK_POINT, NULL) == PERR_OK)
            sigport = (1 << PPTBase->mport->mp_SigBit);
    }

    GetAttr( WINDOW_SigMask, g->Win, &sigmask );

    for(;;) {

        sig = Wait( sigport|sigmask|SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_F );

        if( sig & SIGBREAKF_CTRL_C ) {
            SetErrorCode( frame, PERR_BREAK );
            DisposeObject( g->Win );
            StopInput(frame);
            res = PERR_BREAK;
            goto errexit;
        }

        if( sig & SIGBREAKF_CTRL_F ) {
            WindowToFront( g->win );
            ActivateWindow( g->win );
        }

        if( sig & sigport ) {
            struct gPointMessage *gp;

            if( gp = (struct gPointMessage*)GetMsg(PPTBase->mport) ) {

                /*
                 *  Ignore all other types of messages, except pickpoints.
                 */

                if( gp->msg.code == PPTMSG_PICK_POINT ) {
                    D(bug("User picked point (%d,%d)\n",gp->x,gp->y));

                    val->cx = gp->x;
                    val->cy = gp->y;

                    SetGadgetAttrs( GAD(g->SliderX), g->win, NULL, SLIDER_Level, val->cx, TAG_DONE );
                    SetGadgetAttrs( GAD(g->SliderY), g->win, NULL, SLIDER_Level, val->cy, TAG_DONE );
                }

                ReplyMsg( (struct Message *)gp );
            }
        }

        if( sig & sigmask ) {
            ULONG rc;

            while(( rc = HandleEvent( g->Win )) != WMHI_NOMORE ) {
                struct IBox *area;

                switch(rc) {
                    case WMHI_CLOSEWINDOW:
                    case GID_CANCEL:
                        D(bug("User cancelled\n"));
                        SetErrorCode( frame, PERR_CANCELED );
                        DisposeObject( g->Win );
                        StopInput(frame);
                        res = PERR_CANCELED;
                        goto errexit;

                    case GID_OK:
                        D(bug("User hit OK\n"));
                        DoGadgets( frame, g, val, PPTBase );
                        GetAttr( WINDOW_Bounds, g->Win, (ULONG *)&val->window );
                        DisposeObject( g->Win );
                        StopInput(frame);
                        res = PERR_OK;
                        goto errexit;

                    case GID_PREVIEW_AREA:
                        if( pwframe ) {
                            GetAttr( AREA_AreaBox, g->Preview, (ULONG *)&area );
                            RenderFrame( tmpframe, g->win->RPort, area, 0L );
                        }
                        break;

                    default:
                        if( rc < GID_VAL_START || rc > GID_VAL_END )
                            break;

                        if( pwframe ) {
                            DoGadgets( frame, g, val, PPTBase );
                            GetAttr( AREA_AreaBox, g->Preview, (ULONG *)&area );

                            /*
                             *  Rescale the values to fit the preview frame.  Don't worry,
                             *  the code above re-reads them before attempting to use them
                             *  on the real frame.
                             */
                            val->cx     = val->cx * pwframe->pix->width / frame->pix->width;
                            val->cy     = val->cy * pwframe->pix->height / frame->pix->height;
                            val->radius = val->radius * pwframe->pix->height / frame->pix->height;

                            DoTwirl( pwframe, tmpframe, val, PPTBase );
                            RenderFrame( tmpframe, g->win->RPort, area, 0L );
                        }
                        break;

                }
            }
        } /* if(sig&sigmask) */
    }

errexit:
    if( tmpframe ) RemFrame( tmpframe );
    if( pwframe ) ReleasePreviewFrame( pwframe );

    return res;
}
///

PERROR ParseRexxArgs( FRAME *frame, ULONG *args, struct Values *val, struct PPTBase *PPTBase )
{
    if( args[0] ) {
        val->angle = atof( (STRPTR) args[0] );
    }

    if( args[1] ) {
        val->cx = *(LONG *)args[1];
    }

    if( args[2] ) {
        val->cy = *(LONG *)args[2];
    }

    if( args[3] ) {
        val->radius = *(LONG *)args[3];
    }

    if( args[4] ) {
        int i;

        for( i = 0; interp_labels[i]; i++ ) {
            if( stricmp( interp_labels[i], (STRPTR) args[4] ) == 0 ) {
                val->mode = i;
                break;
            }
        }

        if( !interp_labels[i] ) {
            SetErrorMsg( frame, "Interpolation scheme not known." );
            return PERR_FAILED;
        }
    }

    return PERR_OK;
}

/// EffectExec
EFFECTEXEC(frame, tags, PPTBase, EffectBase)
{
    struct Library *BGUIFloatBase;
    FRAME *newframe = NULL;
    ULONG *args;
    struct Values *opt, val = {0};
    struct GUI gui = {0};

    val.radius = 50;
    val.mode   = Linear;
    val.angle  = 0.0;

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    }

    /*
     *  Check for REXX arguments for this effect.  Every effect should be able
     *  to accept AREXX commands!
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {

        if( ParseRexxArgs(frame, args, &val, PPTBase) != PERR_OK )
            return NULL;

        newframe = Twirl(frame,&val, PPTBase);

    } else {
        /*
         *  Starts up the GUI.
         */
        val.cx     = frame->pix->width/2;
        val.cy     = frame->pix->height/2;

        if(BGUIFloatBase = OpenLibrary( "Gadgets/bgui_float.gadget", 0L ) ) {
            D(bug("\tOpened BGUI float gadget\n"));
            gui.FloatClass = GetFloatClassPtr();

            if( OpenGUI( frame, &val, &gui, TRUE, PPTBase ) ) {
                if( HandleIDCMP( frame, &val, &gui, PPTBase ) == PERR_OK );
                    newframe = Twirl( frame, &val, PPTBase );
            }

            CloseLibrary( BGUIFloatBase );
        }

    }

    /*
     *  Save the options to the PPT internal system
     */

    PutOptions( MYNAME, &val, sizeof(val) );

    return newframe;
}
///

EFFECTGETARGS(frame, tags, PPTBase, EffectBase)
{
    struct Library *BGUIFloatBase;
    ULONG *args;
    struct Values *opt, val = {0};
    struct GUI gui = {0};
    STRPTR buffer;
    PERROR res = PERR_OK;

    val.radius = 50;
    val.mode   = Linear;
    val.angle  = 0.0;

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    /*
     *  Check for REXX arguments for this effect.  Every effect should be able
     *  to accept AREXX commands!
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( ParseRexxArgs(frame, args, &val, PPTBase) != PERR_OK )
            return NULL;
    }

    /*
     *  Starts up the GUI.
     */
    val.cx     = 0;
    val.cy     = 0;

    if(BGUIFloatBase = OpenLibrary( "Gadgets/bgui_float.gadget", 0L ) ) {
        D(bug("\tOpened BGUI float gadget\n"));
        gui.FloatClass = GetFloatClassPtr();

        if( OpenGUI( frame, &val, &gui, FALSE, PPTBase ) ) {
            if( (res = HandleIDCMP( frame, &val, &gui, PPTBase )) == PERR_OK ) {
                // "ANGLE/A,XCENTER=CX/N,YCENTER=CY/N,RADIUS/N,INTERPOLATION=INT/K",
                SPrintF( buffer, "ANGLE %f CX %d CY %d RADIUS %d INTERPOLATION %s",
                                 val.angle,
                                 val.cx,
                                 val.cy,
                                 val.radius,
                                 interp_labels[val.mode] );
            }
        }

        CloseLibrary( BGUIFloatBase );
    }


    /*
     *  Save the options to the PPT internal system
     */

    PutOptions( MYNAME, &val, sizeof(val) );

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

