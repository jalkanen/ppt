/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : color change

    PPT and this file are (C) Janne Jalkanen 1997-1999.

    $Id: colorchange.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

#undef DEBUG_MODE

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "ColorChange"

#define DEFAULT_TOLERANCE   0L

#define GID_OK              1
#define GID_CANCEL          2
#define GID_FROM_RED       10
#define GID_FROM_REDI      11
#define GID_FROM_GREEN     12
#define GID_FROM_GREENI    13
#define GID_FROM_BLUE      14
#define GID_FROM_BLUEI     15
#define GID_TO_RED         20
#define GID_TO_REDI        21
#define GID_TO_GREEN       22
#define GID_TO_GREENI      23
#define GID_TO_BLUE        24
#define GID_TO_BLUEI       25
#define GID_TOLERANCE      30
#define GID_TOLERANCEI     31
#define GID_PREVIEW        40

#define GID_LAST           40

#define GAD(x) ((struct Gadget *)(x))
#define SQR(x) ((x)*(x))
#define CLAMP(x,a,b) if( (x) < (a)) (x) = (a); else if( (x) > (b) ) (x) = (b);

struct Values {
    struct {
        LONG r,g,b;
    } from;
    struct {
        LONG r,g,b;
    } to;
    LONG tolerance;
    struct IBox winpos;
};


/*----------------------------------------------------------------------*/
/* Internal prototypes */

FRAME *DoModify(FRAME *frame, struct Values *v, struct PPTBase *PPTBase);


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

const ULONG dpcol_sl2int[] = { SLIDER_Level, STRINGA_LongVal, TAG_END };
const ULONG dpcol_int2sl[] = { STRINGA_LongVal, SLIDER_Level, TAG_END };

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Changes any color off an image\n"
    "to something else";

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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1997-1999",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"SRCRED/N/A,SRCGREEN/N/A,SRCBLUE/N/A,"
                               "DESTRED/N/A,DESTGREEN/N/A,DESTBLUE/N/A,TOLERANCE/N",

    PPTX_ColorSpaces,   CSF_RGB|CSF_ARGB,

    PPTX_ReqPPTVersion, 3,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
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

BOOL HandleGUI( FRAME *frame, struct Values *v, BOOL dopreview, struct PPTBase *PPTBase )
{
    struct Window *win;
    Object *Win, *FR, *FG, *FB, *TR, *TG, *TB;
    Object *FRI, *FGI, *FBI, *TRI, *TGI, *TBI;
    Object *Tol, *TolI, *PW;
    BOOL res = FALSE;
    ULONG sig, sigmask, sigport = 0L;
    struct Library *BGUIBase = PPTBase->lb_BGUI;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    char wtitle[80] = MYNAME": ";
    FRAME *previewframe = NULL, *tmpframe = NULL;

    D(bug("Creating window object...\n"));

    strcat(wtitle,frame->nd.ln_Name);


    if( dopreview ) {
        /*
         *  Make the preview object to be rendered in the preview window
         */

        if( !(previewframe = ObtainPreviewFrame( frame, NULL ))) {
            SetErrorMsg( frame, "Couldn't obtain preview" );
            return FALSE;
        }

        if( !(tmpframe = DupFrame( previewframe, 0L ) ) ) {
            ReleasePreviewFrame( previewframe );
            SetErrorMsg( frame, "Couldn't obtain preview" );
            return FALSE;
        }
    }

    Win = WindowObject,
        WINDOW_Screen, PPTBase->g->maindisp->scr,
        WINDOW_Font,   PPTBase->g->userprefs->mainfont,
        WINDOW_Title,  wtitle,
        WINDOW_ScaleWidth, 25,
        TAG_SKIP,      (v->winpos.Height == 0) ? 1 : 0,
        WINDOW_Bounds, &(v->winpos),
        WINDOW_ScreenTitle, "ColorChange",
        WINDOW_MasterGroup,
            VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                StartMember,
                    InfoObject,
                        INFO_TextFormat, ISEQ_C"Please select source and destination colors",
                        ButtonFrame, FRM_Flags, FRF_RECESSED,
                        INFO_MinLines, 1,
                    EndObject,
                EndMember,
                StartMember,
                    HGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                        StartMember,
                            VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                                RidgeFrame,
                                FrameTitle("Source"),
                                VarSpace(50),
                                StartMember,
                                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                                        StartMember,
                                            FR = SliderObject, GA_ID, GID_FROM_RED,
                                                Label("R"),
                                                SLIDER_Min,     0,
                                                SLIDER_Max,     255,
                                                SLIDER_Level,   v->from.r,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            FRI = StringObject, GA_ID, GID_FROM_REDI,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                                STRINGA_LongVal, v->from.r,
                                                STRINGA_MinCharsVisible,3,
                                                STRINGA_MaxChars,   6,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject, FixMinHeight,
                                EndMember,
                                StartMember,
                                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                                        StartMember,
                                            FG = SliderObject, GA_ID, GID_FROM_GREEN,
                                                Label("G"),
                                                SLIDER_Min,     0,
                                                SLIDER_Max,     255,
                                                SLIDER_Level,   v->from.g,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            FGI = StringObject, GA_ID, GID_FROM_GREENI,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                                STRINGA_MinCharsVisible,3,
                                                STRINGA_MaxChars,   6,
                                                STRINGA_LongVal, v->from.g,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject, FixMinHeight,
                                EndMember,
                                StartMember,
                                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                                        StartMember,
                                            FB = SliderObject, GA_ID, GID_FROM_BLUE,
                                                Label("B"),
                                                SLIDER_Min,     0,
                                                SLIDER_Max,     255,
                                                SLIDER_Level,   v->from.b,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            FBI = StringObject, GA_ID, GID_FROM_BLUEI,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                                STRINGA_MinCharsVisible,3,
                                                STRINGA_MaxChars,   6,
                                                STRINGA_LongVal, v->from.b,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject, FixMinHeight,
                                EndMember,
                                VarSpace(50),
                            EndObject,
                        EndMember,

                        /*
                         *  Skip this, if no preview requested
                         */

                        dopreview ? TAG_IGNORE : TAG_SKIP, 3,
                        StartMember,
                            VGroupObject, NarrowSpacing,
                                RidgeFrame,
                                FrameTitle("Preview"),
                                VarSpace(50),
                                StartMember,
                                    PW = AreaObject,
                                        GA_ID, GID_PREVIEW,
                                        ButtonFrame, FRM_Flags, FRF_RECESSED,
                                        AREA_MinWidth, PPTBase->g->userprefs->previewwidth,
                                        AREA_MinHeight,PPTBase->g->userprefs->previewheight,
                                    EndObject, FixMinSize,
                                EndMember,
                                VarSpace(50),
                            EndObject, FixMinWidth,
                        EndMember,

                        StartMember,
                            VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                                RidgeFrame,
                                FrameTitle("Destination"),
                                VarSpace(50),
                                StartMember,
                                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                                        StartMember,
                                            TR = SliderObject, GA_ID, GID_TO_RED,
                                                Label("R"),
                                                SLIDER_Min,     0,
                                                SLIDER_Max,     255,
                                                SLIDER_Level,   v->to.r,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            TRI = StringObject, GA_ID, GID_TO_REDI,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                                STRINGA_MinCharsVisible,3,
                                                STRINGA_LongVal, v->to.r,
                                                STRINGA_MaxChars,   6,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject, FixMinHeight,
                                EndMember,
                                StartMember,
                                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                                        StartMember,
                                            TG = SliderObject, GA_ID, GID_TO_GREEN,
                                                Label("G"),
                                                SLIDER_Min,     0,
                                                SLIDER_Max,     255,
                                                SLIDER_Level,   v->to.g,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            TGI = StringObject, GA_ID, GID_TO_GREENI,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                                STRINGA_MinCharsVisible,3,
                                                STRINGA_MaxChars,   6,
                                                STRINGA_LongVal, v->to.g,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject, FixMinHeight,
                                EndMember,
                                StartMember,
                                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                                        StartMember,
                                            TB = SliderObject, GA_ID, GID_TO_BLUE,
                                                Label("B"),
                                                SLIDER_Min,     0,
                                                SLIDER_Max,     255,
                                                SLIDER_Level,   v->to.b,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            TBI = StringObject, GA_ID, GID_TO_BLUEI,
                                                STRINGA_MinCharsVisible,3,
                                                STRINGA_LongVal, v->to.b,
                                                STRINGA_MaxChars,   6,
                                                STRINGA_IntegerMin, 0,
                                                STRINGA_IntegerMax, 255,
                                            EndObject, Weight(1),
                                        EndMember,
                                    EndObject, FixMinHeight,
                                EndMember,
                                VarSpace(50),
                            EndObject,
                        EndMember,
                    EndObject,
                EndMember,
                StartMember,
                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                        StartMember,
                            Tol = SliderObject, GA_ID, GID_TOLERANCE,
                                Label("Tolerance"),
                                SLIDER_Min,     0,
                                SLIDER_Max,     255,
                                SLIDER_Level,   v->tolerance,
                            EndObject,
                        EndMember,
                        StartMember,
                            TolI = StringObject, GA_ID, GID_TOLERANCEI,
                                STRINGA_MinCharsVisible,3,
                                STRINGA_LongVal, v->tolerance,
                                STRINGA_IntegerMin, 0,
                                STRINGA_MaxChars,   6,
                                STRINGA_IntegerMax, 255,
                            EndObject, Weight(1),
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
                StartMember,
                    HorizSeparator,
                EndMember,
                StartMember,
                    HGroupObject, WideSpacing, NarrowVOffset, NarrowHOffset,
                        StartMember,
                            ButtonObject, GA_ID, GID_OK,
                                Label("OK"),
                            EndObject,
                        EndMember,
                        StartMember,
                            ButtonObject, GA_ID, GID_CANCEL,
                                Label("Cancel"),
                            EndObject,
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
            EndObject,
        EndObject;

    D(bug("\tdone!\n"));

    if(Win) {
        /*
         *  Bindings
         */

        AddMap( FR, FRI, dpcol_sl2int );
        AddMap( FG, FGI, dpcol_sl2int );
        AddMap( FB, FBI, dpcol_sl2int );
        AddMap( TR, TRI, dpcol_sl2int );
        AddMap( TG, TGI, dpcol_sl2int );
        AddMap( TB, TBI, dpcol_sl2int );

        AddMap( FRI, FR, dpcol_int2sl );
        AddMap( FGI, FG, dpcol_int2sl );
        AddMap( FBI, FB, dpcol_int2sl );
        AddMap( TRI, TR, dpcol_int2sl );
        AddMap( TGI, TG, dpcol_int2sl );
        AddMap( TBI, TB, dpcol_int2sl );

        AddMap( Tol, TolI, dpcol_sl2int );
        AddMap( TolI, Tol, dpcol_int2sl );

        if( win = WindowOpen(Win) ) {
            BOOL quit = FALSE;
            /*
             *  IDCMP Loop
             */

            D(bug("Window opened, entering handler loop\n"));

            GetAttr( WINDOW_SigMask, Win, &sigmask );
            if(StartInput(frame, GINP_PICK_POINT, NULL) == PERR_OK)
                sigport = (1 << PPTBase->mport->mp_SigBit);


            if( dopreview ) {
                /*
                 *  Initialize the temporary frame.
                 */

                CopyFrameData( previewframe, tmpframe, 0L );
                DoModify( tmpframe, v, PPTBase );
            }

            while(!quit) {
                sig = Wait( sigmask | sigport | SIGBREAKF_CTRL_F | SIGBREAKF_CTRL_C );

                if( sig & SIGBREAKF_CTRL_C ) {
                    quit = TRUE;
                    break;
                }

                if( sig & SIGBREAKF_CTRL_F ) {
                    WindowToFront( win );
                    ActivateWindow( win );
                }

                if( sig & sigport ) {
                    struct gPointMessage *gp;

                    if( gp = (struct gPointMessage*)GetMsg(PPTBase->mport)) {
                        if( gp->msg.code == PPTMSG_PICK_POINT ) {
                            D(bug("User picked point (%d,%d)\n",gp->x,gp->y));

                            if( frame->pix->colorspace == CS_RGB ) {
                                RGBPixel *cp;

                                cp = (RGBPixel *)GetPixelRow( frame, gp->y );
                                SetGadgetAttrs( GAD(FR), win, NULL, SLIDER_Level, cp[gp->x].r, TAG_DONE );
                                SetGadgetAttrs( GAD(FG), win, NULL, SLIDER_Level, cp[gp->x].g, TAG_DONE );
                                SetGadgetAttrs( GAD(FB), win, NULL, SLIDER_Level, cp[gp->x].b, TAG_DONE );
                            } else {
                                ARGBPixel *cp;

                                cp = (ARGBPixel *)GetPixelRow( frame, gp->y );
                                SetGadgetAttrs( GAD(FR), win, NULL, SLIDER_Level, cp[gp->x].r, TAG_DONE );
                                SetGadgetAttrs( GAD(FG), win, NULL, SLIDER_Level, cp[gp->x].g, TAG_DONE );
                                SetGadgetAttrs( GAD(FB), win, NULL, SLIDER_Level, cp[gp->x].b, TAG_DONE );
                            }
                        }

                        ReplyMsg( (struct Message *)gp );
                    }
                }

                if( sig & sigmask ) {
                    ULONG rc;
                    struct IBox *area;

                    while(( rc = HandleEvent( Win )) != WMHI_NOMORE ) {
                        switch( rc ) {

                          default:
                            if( rc < GID_LAST ) {
                                GetAttr( SLIDER_Level, FR, (ULONG *)&v->from.r );
                                GetAttr( SLIDER_Level, FG, (ULONG *)&v->from.g );
                                GetAttr( SLIDER_Level, FB, (ULONG *)&v->from.b );
                                GetAttr( SLIDER_Level, TR, (ULONG *)&v->to.r );
                                GetAttr( SLIDER_Level, TG, (ULONG *)&v->to.g );
                                GetAttr( SLIDER_Level, TB, (ULONG *)&v->to.b );
                                GetAttr( SLIDER_Level, Tol, (ULONG *)&v->tolerance );
                                GetAttr( WINDOW_Bounds, Win, (ULONG *)&v->winpos);
                                if( rc == GID_OK) {
                                    res = quit = TRUE;
                                } else {
                                    if( dopreview ) {
                                        CopyFrameData( previewframe, tmpframe, 0L );
                                        DoModify( tmpframe, v, PPTBase );
                                        GetAttr( AREA_AreaBox, PW, (ULONG *)&area );
                                        RenderFrame( tmpframe, win->RPort, area, 0L );
                                    }
                                }
                            }
                            break;

                          case GID_PREVIEW:
                            if( dopreview ) {
                                GetAttr( AREA_AreaBox, PW, (ULONG *)&area );
                                RenderFrame( tmpframe, win->RPort, area, 0L );
                            }
                            break;

                          case WMHI_CLOSEWINDOW:
                          case GID_CANCEL:
                            quit = TRUE;
                            break;
                        }
                    } /* while() */
                } /* sig & sigmask */

            } /* while() */

            if(sigport) StopInput( frame );

            DisposeObject(Win);
        } else {
            SetErrorCode( frame, PERR_WINDOWOPEN );
        }
    }

    if( previewframe ) ReleasePreviewFrame( previewframe );
    if( tmpframe ) RemFrame( tmpframe );

    return res;
}

BOOL HandleRexxArgs( FRAME *frame, struct Values *v,
                     ULONG *args, struct PPTBase *PPTBase )
{
    v->from.r = * (LONG *)args[0];
    v->from.g = * (LONG *)args[1];
    v->from.b = * (LONG *)args[2];
    v->to.r = * (LONG *)args[3];
    v->to.g = * (LONG *)args[4];
    v->to.b = * (LONG *)args[5];

    if( args[6] )
        v->tolerance = *(LONG *)args[6];
    else
        v->tolerance = DEFAULT_TOLERANCE;

    return TRUE;
}

INLINE
ULONG RGBDistance( UBYTE r1, UBYTE g1, UBYTE b1, UBYTE r2, UBYTE g2, UBYTE b2 )
{
    return (ULONG)(SQR(r1-r2) + SQR(g1-g2) + SQR(b1-b2));
}


FRAME *DoModify(FRAME *frame, struct Values *v, struct PPTBase *PPTBase)
{
    WORD row, col;
    UBYTE cspace = frame->pix->colorspace;
    UBYTE comps = frame->pix->components;
    ULONG tolerance = SQR(v->tolerance);

    InitProgress(frame,"Changing colors...", frame->selbox.MinY, frame->selbox.MaxY );

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        ROWPTR cp;

        Progress(frame, row);

        cp = (ROWPTR)GetPixelRow( frame, row );

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
            LONG r,g,b;

            if( cspace == CS_RGB ) {
                RGBPixel *t;

                t = (RGBPixel *) &cp[col*comps];
                if(RGBDistance(t->r, t->g, t->b, v->from.r, v->from.g, v->from.b) <= tolerance) {
                    r = v->to.r + (t->r - v->from.r);
                    g = v->to.g + (t->g - v->from.g);
                    b = v->to.b + (t->b - v->from.b);
                    CLAMP(r,0,255);
                    CLAMP(g,0,255);
                    CLAMP(b,0,255);
                    t->r = r;
                    t->g = g;
                    t->b = b;
                }

            } else {
                ARGBPixel *t;

                t = (ARGBPixel *) &cp[col*comps];
                if(RGBDistance(t->r, t->g, t->b, v->from.r, v->from.g, v->from.b) <= tolerance) {
                    r = v->to.r + (t->r - v->from.r);
                    g = v->to.g + (t->g - v->from.g);
                    b = v->to.b + (t->b - v->from.b);
                    CLAMP(r,0,255);
                    CLAMP(g,0,255);
                    CLAMP(b,0,255);
                    t->r = r;
                    t->g = g;
                    t->b = b;
                }
            }

        }

        PutPixelRow( frame, row, cp );
    }

    FinishProgress(frame);

    return frame;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe = NULL;
    ULONG *args;
    struct Values v = {0}, *ov;
    BOOL argsok;

    if( ov = GetOptions(MYNAME) ) {
        v = *ov;
    }

    if( args = (ULONG *)TagData(PPTX_RexxArgs,tags)) {
        argsok = HandleRexxArgs(frame, &v, args, PPTBase);
    } else {
        argsok = HandleGUI(frame, &v, TRUE, PPTBase);
    }

    if(argsok) {
        newframe = DoModify( frame, &v, PPTBase );
        PutOptions(MYNAME, &v, sizeof(struct Values) );
    }

    return newframe;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    struct Values v = {0}, *ov;
    ULONG *args;
    STRPTR buffer;
    BOOL ok;

    if( ov = GetOptions(MYNAME) ) {
        v = *ov;
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *)TagData(PPTX_RexxArgs,tags))
        HandleRexxArgs(frame, &v, args, PPTBase);

    ok = HandleGUI( frame, &v, FALSE, PPTBase );

    if( ok ) {
        SPrintF( buffer, "SRCRED %d SRCGREEN %d SRCBLUE %d DESTRED %d DESTGREEN %d DESTBLUE %d TOLERANCE %d",
                          v.from.r, v.from.g, v.from.b,
                          v.to.r,   v.to.g,   v.to.b,
                          v.tolerance);
    }

    return ok ? PERR_OK : PERR_ERROR;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

