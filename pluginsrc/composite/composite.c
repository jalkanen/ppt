/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : Composite

    PPT and this file are (C) Janne Jalkanen 1995-1996.

    $Id: composite.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines & types */

#define GID_OK          1
#define GID_CANCEL      2
#define GID_METHOD      3
#define GID_RATIO       4
#define GID_TILE        5

typedef enum {
    Direct,
    Minimum,
    Maximum,
    Mix,
    TransparentBlack,
    Add,
    Subtract,
    Multiply
} MethodID;

struct Values {
    MethodID        method;
    ULONG           ratio;
    struct IBox     bounds;
    BOOL            tile;
};

#define MYNAME      "Composite"

#define PEEKL(x)    (*( (ULONG *)(x) ))

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
    "Image compositing\n";
/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {

    PPTX_Name, (ULONG)MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author, (ULONG)"Janne Jalkanen",
    PPTX_InfoTxt, (ULONG)infoblurb,

    PPTX_RexxTemplate, (ULONG)"WITH/A/N,TOP/N,LEFT/N,METHOD/K,RATIO/N,TILE/S",

    PPTX_ColorSpaces, (ULONG)CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,

    TAG_END, 0L
};

const char *method_labels[] = {
    "Direct",
    "Minimum",
    "Maximum",
    "Mix",
    "Transparent Black",
    "Add",
    "Subtract",
    "Multiply",
    NULL
};

const ULONG sl2ind[] = { SLIDER_Level, INDIC_Level, TAG_END };

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct IntuitionBase *IntuitionBase;
struct Library *BGUIBase;

Object *Win = NULL, *Method, *InfoTxt, *Ratio, *RatioI, *OKButton, *Tile;
struct Window *win;

char wtitle[80];

struct Values v;

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

BOOL GimmeWindow(FRAME *frame, FRAME *with, struct PPTBase *PPTBase )
{
    strcpy(wtitle,MYNAME": ");
    strcat(wtitle, frame->name);

    Win = WindowObject,
        WINDOW_Screen,      PPTBase->g->maindisp->scr,
        WINDOW_Title,       wtitle,
        v.bounds.Top != ~0 ? TAG_IGNORE : TAG_SKIP, 1,
        WINDOW_Bounds,      &v.bounds,
        WINDOW_Position,    POS_CENTERSCREEN,
        WINDOW_Font,        PPTBase->g->userprefs->mainfont,
        WINDOW_MasterGroup,
            VGroupObject, Spacing(4), HOffset(4), VOffset(4),
                StartMember,
                    InfoTxt = InfoObject,
                        INFO_TextFormat, ISEQ_C"Please position the source\n"
                                               "over the destination...",
                        INFO_MinLines, 3,
                    EndObject,
                EndMember,
                StartMember,
                    InfoObject,
                        Label("Destination:"),
                        ButtonFrame,
                        FRM_Flags, FRF_RECESSED,
                        INFO_TextFormat, frame->name,
                    EndObject,
                EndMember,
                StartMember,
                    InfoObject,
                        Label("Source:"),
                        ButtonFrame,
                        FRM_Flags, FRF_RECESSED,
                        INFO_TextFormat, with ? with->name : "None",
                    EndObject,
                EndMember,
                StartMember,
                    Method = CycleObject, GA_ID, GID_METHOD,
                        Label("Method:"),
                        Place(PLACE_LEFT),
                        ButtonFrame,
                        CYC_Active, v.method,
                        CYC_Labels, method_labels,
                        CYC_Popup,  TRUE,
                    EndObject,
                EndMember,
                StartMember,
                    HGroupObject,
                        StartMember,
                            Ratio = SliderObject, GA_ID, GID_RATIO,
                                Label("Ratio:"),
                                Place(PLACE_LEFT),
                                SLIDER_Min,   0,
                                SLIDER_Max,   255,
                                SLIDER_Level, v.ratio,
                                PGA_Freedom,  FREEHORIZ,
                            EndObject,
                        EndMember,
                        StartMember,
                            RatioI = IndicatorObject,
                                INDIC_Min, 0,
                                INDIC_Max, 255,
                                INDIC_Level, v.ratio,
                                INDIC_Justification, IDJ_RIGHT,
                                INDIC_FormatString, "%3ld",
                            EndObject, FixMinWidth,
                        EndMember,
                    EndObject,
                EndMember,
                StartMember,
                    Tile = CheckBoxObject,
                        GA_ID, GID_TILE,
                        Label("Tile source?"), Place(PLACE_LEFT),
                        GA_Selected, v.tile,
                    EndObject, FixMinSize,
                EndMember,
                StartMember,
                    HGroupObject, Spacing(6),
                        StartMember,
                            OKButton = ButtonObject,
                                GA_ID, GID_OK,
                                Label("OK"),
                                GA_Disabled, FALSE,
                            EndObject,
                        EndMember,
                        StartMember,
                            Button("Cancel", GID_CANCEL),
                        EndMember,
                    EndObject,
                EndMember,
            EndObject,
        EndObject;

    if(Win) {

        AddMap( Ratio, RatioI, sl2ind );
        AddCondit( Method, Ratio,  CYC_Active, Mix, GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( Method, RatioI, CYC_Active, Mix, GA_Disabled, FALSE, GA_Disabled, TRUE );

        if( v.method != Mix ) {
            SetGadgetAttrs( (struct Gadget *)Ratio, NULL, NULL, GA_Disabled, TRUE, TAG_DONE );
            SetGadgetAttrs( (struct Gadget *)RatioI, NULL, NULL, GA_Disabled, TRUE, TAG_DONE );
        }

        win = WindowOpen(Win);
        if(!win) {
            SetErrorCode(frame,PERR_WINDOWOPEN);
            DisposeObject(Win);
            return FALSE;
        }
    } else
        return FALSE;

    return TRUE;
}


/*
    BUG: Darned slow
*/

#define VECTOR_LEN(x,y,z) (((x)*(x)) + ((y)*(y)) + ((z)*(z)))
#define CLAMP_UP(x,v) ( ((x) > (v)) ? (v) : (x))
#define CLAMP_DOWN(x,v) ( ((x) < (v)) ? (v) : (x))

FRAME *DoComposite( FRAME *dest, FRAME *src, struct gFixRectMessage *gfr,
                    MethodID how, WORD mixratio, BOOL tile, struct PPTBase *PPTBase )
{
    WORD row, col;
    WORD dcomps = dest->pix->components,
         scomps = src->pix->components;
    WORD dest_has_alpha = 0, src_has_alpha = 0;
    WORD top, bottom, left, right;

    D(bug("Doing the composite.  gfr = %d,%d,%d,%d\n",
           gfr->x, gfr->y, gfr->dim.Width, gfr->dim.Height));

    /*
     *  Initialize
     */

    if( src->pix->colorspace == CS_ARGB )
        src_has_alpha = 1;

    if( dest->pix->colorspace == CS_ARGB )
        dest_has_alpha = 1;

    if( tile ) {
        top = dest->selbox.MinY;
        left = dest->selbox.MinX;
        bottom = dest->selbox.MaxY;
        right = dest->selbox.MaxX;
    } else {
        top = gfr->y;
        bottom = gfr->y + gfr->dim.Height;
        left = gfr->x;
        right = gfr->x + gfr->dim.Width;
    }

    InitProgress( dest, "Compositing...", top, bottom );

    for( row = top; row < bottom; row++ ) {
        ROWPTR scp, dcp;
        WORD srow;

        srow = (row - gfr->y) % gfr->dim.Height;
        if( srow < 0 && tile ) srow += gfr->dim.Height;

        scp = GetPixelRow(src,  srow);
        dcp = GetPixelRow(dest, row);

        if( !dcp ) continue; /* Skip all areas that are outside */

        if( Progress(dest, row) )
            return NULL;

        for( col = left; col < right; col++ ) {
            LONG a, tr,tg,tb;
            UBYTE *s, *d;
            WORD scol;

            /*
             *  Sanitation check.  Let's not overwrite innocent
             *  memory.
             */

            if( col < 0 || col >= dest->pix->width ) continue;

            scol = ( col - gfr->x ) % gfr->dim.Width;
            if( scol < 0 && tile ) scol += gfr->dim.Width;

            if( src_has_alpha )
                a = (LONG)scp[scomps*scol];

            d = &dcp[dcomps*col+dest_has_alpha];
            s = &scp[scomps*scol+src_has_alpha];

            tr = *(d+0);
            tg = *(d+1);
            tb = *(d+2);

            switch( how ) {
                case Direct:
                    tr = *s; tg = *(s+1); tb = *(s+2);
                    break;

                case Minimum:
                    if( VECTOR_LEN(tr, tg, tb) > VECTOR_LEN(*s, *(s+1), *(s+2)) ) {
                        tr = *s; tg = *(s+1); tb = *(s+2);
                    }
                    break;

                case Maximum:
                    if( VECTOR_LEN(tr, tg, tb) < VECTOR_LEN(*s, *(s+1), *(s+2)) ) {
                        tr = *s; tg = *(s+1); tb = *(s+2);
                    }
                    break;

                case Mix:
                    tr = (mixratio * (*s) + (255-mixratio) * tr ) / 255;
                    tg = (mixratio * (*(s+1)) + (255-mixratio) * tg ) / 255;
                    tb = (mixratio * (*(s+2)) + (255-mixratio) * tb ) / 255;
                    break;

                case TransparentBlack:
                    if( *s || *(s+1) || *(s+2) ) {
                        tr = *s; tg = *(s+1); tb = *(s+2);
                    }
                    break;

                case Add:
                    tr = CLAMP_UP(*(s+0) + tr, 255);
                    tg = CLAMP_UP(*(s+1) + tg, 255);
                    tb = CLAMP_UP(*(s+2) + tb, 255);
                    break;

                case Subtract:
                    tr = CLAMP_DOWN( tr - *(s+0), 0 );
                    tg = CLAMP_DOWN( tg - *(s+1), 0 );
                    tb = CLAMP_DOWN( tb - *(s+2), 0 );
                    break;

                case Multiply:
                    tr = (tr * *(s+0)) / 256;
                    tg = (tg * *(s+1)) / 256;
                    tb = (tb * *(s+2)) / 256;
                    break;
            }

            if( src_has_alpha ) {
                tr = ((255-a) * tr + a * *(d+0) ) /256;
                tg = ((255-a) * tg + a * *(d+1) ) /256;
                tb = ((255-a) * tb + a * *(d+2) ) /256;
            }

            *d     = tr;
            if( dest->pix->colorspace != CS_GRAYLEVEL ) {
                *(d+1) = tg;
                *(d+2) = tb;
            }

#if 0
            for( pix = 0; pix < colors; pix++ ) {
                UBYTE s,d;
                LONG  t;

                d = dcp[dcomps*col+pix+dest_has_alpha];
                s = scp[scomps*(col-gfr->x)+pix+src_has_alpha];
                t = (LONG)d;

                switch(how) {
                    case Direct:
                        t = s;
                        break;

                    case Minimum:
                        if( d > s ) t = s;
                        break;

                    case Maximum:
                        if( d < s ) t = s;
                        break;

                    case Mix:
                        t = (mixratio*s + (255-mixratio)*d)/255;
                        break;

                    case TransparentBlack:
                        if( s != 0 ) t = s;
                        break;

                    case Add:
                        t = d+s;
                        if( t > 255 ) t = 255;
                        break;

                    case Subtract:
                        t = d-s;
                        if( t < 0 ) t = 0;
                        break;

                    case Multiply:
                        t = ((LONG)d * (LONG)s) / 256;
                        break;

                }

                if( src_has_alpha )
                    t = ((255-a) * t + a*d)/255;

                dcp[dcomps*col+pix+dest_has_alpha] = (UBYTE)t;
            }
#endif
        }
        PutPixelRow(dest,row,dcp);
    }

    FinishProgress(dest);

    return dest;
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

/*
    BUG: The separation between rexx messages and PPT internal call
         is really crap.
*/

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    ULONG sig, rc, *args;
    BOOL quit = FALSE, reallyrexx = FALSE;
    FRAME *newframe = NULL, *with = NULL;
    struct gFixRectMessage gfr = {0};
    ULONG fc, wc;
    struct Values *av;

    D(bug(MYNAME": Exec()\n"));

    /*
     *  Defaults
     */
    v.ratio = 128;
    v.method = Direct;
    v.bounds.Top = v.bounds.Left = ~0;
    v.bounds.Width = 200; v.bounds.Height = 100;
    v.tile = FALSE;

    if( av = GetOptions(MYNAME) ) {
        v = *av;
    }

    /*
     *  Copy to local variables
     */

    BGUIBase        = PPTBase->lb_BGUI;
    IntuitionBase   = (struct IntuitionBase *)PPTBase->lb_Intuition;
    DOSBase         = PPTBase->lb_DOS;
    SysBase         = PPTBase->lb_Sys;

    /*
     *  Parse AREXX message, which has to exist.
     *  BUG: If necessary, should wait for D&D from the main window.
     *  BUG: Should make sanity checks!
     */

    args = (ULONG *) TagData( PPTX_RexxArgs, tags );

    if( args ) {

        /* WITH */
        if( args[0] ) {
            with = FindFrame( (ID) PEEKL(args[0]) );
            if(!with) {
                SetErrorMsg(frame,"Unknown frame ID for WITH parameter");
                return NULL;
            }
        }

        /* TOP */
        if( args[1] ) {
            gfr.y = (WORD) PEEKL(args[1]);
            reallyrexx = TRUE;
        }

        /* LEFT */
        if( args[2] ) {
            gfr.x = (WORD) PEEKL(args[2]);
            reallyrexx = TRUE;
        }

        /* METHOD */
        if( args[3] ) {
            int i;

            for( i = 0; method_labels[i]; i++ ) {
                if(stricmp( method_labels[i], (char *)args[3] ) == 0 ) {
                    v.method = i;
                    reallyrexx = TRUE;
                    break;
                }
            }
        }

        /* RATIO */
        if( v.method == Mix ) {
            if( args[4] ) {
                v.ratio = PEEKL( args[4] );
            } else {
                SetErrorCode(frame,PERR_INVALIDARGS);
            }
        }

        /* TILE */
        if( args[5] ) {
            v.tile = TRUE;
        } else {
            v.tile = FALSE;
        }

    } else {
        SetErrorMsg(frame,"Image compositing can be used with Drag&Drop (or REXX) only");
        return NULL;
    }

    /*
     *  Make some sanity checks
     */

    if( frame->pix->width < with->pix->width ||
        frame->pix->height < with->pix->height ) {
            SetErrorMsg(frame,"You cannot composite a larger picture on a smaller one!");
            return NULL;
        }

    fc = frame->pix->colorspace;
    wc = with->pix->colorspace;

    if( ! (wc == fc || (fc == CS_ARGB && wc == CS_RGB) || (fc == CS_RGB && wc == CS_ARGB ))) {
        SetErrorMsg(frame, "Only images of the same color space can be composited");
        return NULL;
    }

    gfr.dim.Left   = 0;
    gfr.dim.Top    = 0;
    gfr.dim.Height = with->pix->height;
    gfr.dim.Width  = with->pix->width;

    /*
     *  Open window and start parsing
     */

    if( reallyrexx == FALSE ) {
        if( GimmeWindow(frame, with, PPTBase) ) {
            ULONG sigmask, gimask = 0L;

            GetAttr( WINDOW_SigMask, Win, &sigmask );

            StartInput(frame, GINP_FIXED_RECT, (struct PPTMessage *) &gfr);

            gimask = (1 << PPTBase->mport->mp_SigBit);

            while( !quit ) {
                sig = Wait( sigmask|gimask|SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_F );

                if( sig & SIGBREAKF_CTRL_C ) {
                    D(bug("BREAK!\n"));
                    SetErrorCode( frame, PERR_BREAK );
                    quit = TRUE;
                    break;
                }

                if( sig & SIGBREAKF_CTRL_F ) {
                    WindowToFront(win);
                    ActivateWindow(win);
                }

                if( sig & gimask ) {
                    struct gFixRectMessage *pmsg;

                    if(pmsg = (struct gFixRectMessage *)GetMsg( PPTBase->mport )) {
                        if( pmsg->msg.code == PPTMSG_FIXED_RECT ) {
                            D(bug("User picked a point @ (%d,%d)\n",pmsg->x, pmsg->y));
                            gfr.x = pmsg->x; gfr.y = pmsg->y;
                            // SetGadgetAttrs( ( struct Gadget *)OKButton, win, NULL, GA_Disabled, FALSE );
                        }
                        ReplyMsg( (struct Message *)pmsg );
                    }
                }

                if( sig & sigmask ) {
                    while( (rc = HandleEvent( Win )) != WMHI_NOMORE ) {
                        ULONG t;

                        switch(rc) {
                            case GID_OK:
                                GetAttr( CYC_Active, Method, (ULONG *)&v.method );
                                GetAttr( SLIDER_Level, Ratio, &v.ratio );

                                /*
                                 *  Save the window attributes for later retrieval.
                                 */

                                GetAttr( WINDOW_Bounds, Win, (ULONG *) &v.bounds );
                                GetAttr( GA_Selected, Tile, &t );

                                StopInput( frame );
                                v.tile = (BOOL) t;

                                WindowClose(Win);
                                newframe = DoComposite( frame, with, &gfr, v.method,
                                                        (WORD) v.ratio, v.tile, PPTBase );
                                quit = TRUE;
                                break;

                            case GID_CANCEL:
                                quit = TRUE;
                                StopInput( frame );
                                break;
                        }
                    }
                }
            }
        }
    } else {
        /* gfr is already set up */
        newframe = DoComposite( frame, with, &gfr,
                                v.method, (WORD) v.ratio, v.tile,
                                PPTBase );
    }

    if(Win) DisposeObject(Win);

    if( newframe ) {
        PutOptions( MYNAME, &v, sizeof(struct Values) );
    }

    D(bug("Returning %08X...\n",newframe));
    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

