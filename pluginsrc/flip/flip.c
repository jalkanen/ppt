/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : flip

    PPT and this file are (C) Janne Jalkanen 1998-2000.

    $Id: flip.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

//#define DEBUG_MODE 1

#pragma msg 186 ignore

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "Flip"


#define GID_X       1
#define GID_Y       2
#define GID_OK      3
#define GID_CANCEL  4

/* Need to recreate some bgui macros to use my own tag routines. */

#define HGroupObject          MyNewObject( PPTBase, BGUI_GROUP_GADGET
#define VGroupObject          MyNewObject( PPTBase, BGUI_GROUP_GADGET, GROUP_Style, GRSTYLE_VERTICAL
#define ButtonObject          MyNewObject( PPTBase, BGUI_BUTTON_GADGET
#define CheckBoxObject        MyNewObject( PPTBase, BGUI_CHECKBOX_GADGET
#define WindowObject          MyNewObject( PPTBase, BGUI_WINDOW_OBJECT
#define SeperatorObject       MyNewObject( PPTBase, BGUI_SEPERATOR_GADGET
#define WindowOpen(wobj)      (struct Window *)DoMethod( wobj, WM_OPEN )


struct WinInfo {
    Object  *Win,
            *X,*Y,
            *OK,
            *Cancel;
    ULONG   sigmask;
    struct Window *win;
};

struct Values {
    BOOL x, y;
};

/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables */

const char infoblurb[] =
    "This is a simple filter, who can flip\n"
    "the picture both in X and in Y directions\n"
    "\n";

const struct TagItem FlipTagArray[] = {
    PPTX_Name,          MYNAME,
    PPTX_NoNewFrame,    TRUE,
    PPTX_InfoTxt,       infoblurb,
    PPTX_Author,        "Janne Jalkanen 1996-2000",
    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,
    PPTX_RexxTemplate,  "X/S,Y/S",

    PPTX_SupportsGetArgs,TRUE,
    TAG_END, 0L
};

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

/*
    My replacement for BGUI_NewObject() - routine.
*/

Object *MyNewObject( struct PPTBase *PPTBase, ULONG classid, Tag tag1, ... )
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;

    return(BGUI_NewObjectA( classid, (struct TagItem *) &tag1));
}



struct Window *GimmeWin( struct PPTBase *PPTBase, struct WinInfo *w, FRAME *f, struct Values *v )
{
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;

    w->Win = WindowObject,
        WINDOW_Screen, PPTBase->g->maindisp->scr,
        WINDOW_Title, f->nd.ln_Name,
        WINDOW_Font, PPTBase->g->userprefs->mainfont,
        WINDOW_MasterGroup,
            VGroupObject, Spacing(4), HOffset(4), VOffset(4),
                StartMember,
                    HGroupObject, Spacing(8), HOffset(4), VOffset(4),
                        StartMember,
                            w->X = XenKeyCheckBox( "Flip _X", v->x, GID_X ),
                        EndMember,
                        StartMember,
                            w->Y = XenKeyCheckBox( "Flip _Y", v->y, GID_Y ),
                        EndMember,
                    EndObject,
                EndMember,
                StartMember,
                    HorizSeperator,
                EndMember,
                StartMember,
                    HGroupObject, Spacing(4), HOffset(4), VOffset(4),
                        StartMember,
                            w->OK = XenButton( "Flip", GID_OK ),
                        EndMember,
                        StartMember,
                            w->Cancel = XenButton( "Cancel", GID_CANCEL ),
                        EndMember,
                    EndObject,
                EndMember,
            EndObject, /* Master VGROUP */
        EndObject;

    if(w->Win) {
        if( w->win = WindowOpen( w->Win ) ) {
            GetAttr( WINDOW_SigMask, w->Win, &(w->sigmask) );
        }
    }

    return w->win;
}

/*
    The actual do-it routine. Returns 0 for success, something else for failure.
    BUG: could easily be made a lot faster
    BUG: Does not work correctly on selbox areas.
*/

int DoTheFlip( struct PPTBase *PPTBase, FRAME *src, FRAME *dest, struct Values *v )
{
    UWORD crow,col,beginX,beginY,endX,endY;
    UBYTE pixellen,i;
    ROWPTR cc, csrc, cdst;
    int res = 1;

    beginX = src->selbox.MinX;
    endX   = src->selbox.MaxX;
    beginY = src->selbox.MinY;
    endY   = src->selbox.MaxY;
    pixellen = src->pix->components;
    InitProgress( src, "Flipping...", beginY, endY );

    D(bug("Flipping %s %s\n", v->x ? "X" : "", v->y ? "Y" : "" ));

    for(crow = beginY; crow < endY; crow++) {
        WORD rrow;

        csrc = GetPixelRow( src, crow );
        csrc += beginX*pixellen;

        if(v->y)
            rrow = endY - crow + beginY - 1;
        else
            rrow = crow;

        cdst = GetPixelRow( dest, rrow );

        if( Progress( src, crow ) )
            goto exitflip;

        if(v->x) {
            cc = cdst + (endX-1) * pixellen; /* cc points at the end of line. */

            for(col = beginX; col < endX ; col++) {
                /* Copy one pixel */
                for(i = 0; i < pixellen; i++) {
                    *cc++ = *csrc++;
                }
                cc -= (pixellen << 1);
            }
        } else {
            CopyMem( csrc, cdst + beginX*pixellen, pixellen * (endX-beginX) );
        }

        PutPixelRow( dest, rrow, cdst );

    }

    res = 0;

    FinishProgress( src );

exitflip:
    return res;
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, FlipTagArray );
}

/*
 *  BUG: Sloppy code, should be rewritten with AskReq()
 */

PERROR DoGUI(FRAME *frame, struct Values *val,struct PPTBase *PPTBase)
{
    ULONG signals, sig, rc;
    PERROR res = PERR_ERROR;
    BOOL quit = FALSE;
    struct WinInfo w;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;

    if(GimmeWin( PPTBase, &w, frame, val )) {
        signals = w.sigmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F;
        while(!quit) {
            sig = Wait(signals);

            if(sig & SIGBREAKF_CTRL_C) {
                res = PERR_BREAK;
                quit++;
            }

            if(sig & SIGBREAKF_CTRL_F) {
                WindowToFront( w.win );
                ActivateWindow( w.win );
            }

            if(sig & w.sigmask ) {
                while(( rc = HandleEvent( w.Win)) != WMHI_NOMORE) {
                    switch(rc) {
                        ULONG t;

                        case WMHI_CLOSEWINDOW:
                        case GID_CANCEL:
                            quit++;
                            res = PERR_CANCELED;
                            WindowClose( w.Win );
                            break;

                        case GID_OK:
                            GetAttr( GA_Selected, w.X, &t );
                            val->x = (BOOL)t;
                            GetAttr( GA_Selected, w.Y, &t );
                            val->y = (BOOL)t;
                            WindowClose( w.Win );
                            res = PERR_OK;
                            quit++;
                            break;
                    } /* switch */
                }
            } /* mainsignals */
        } /* !quit */
        DisposeObject(w.Win);
    } else {
        SetErrorCode( frame, PERR_WINDOWOPEN );
        res = PERR_WINDOWOPEN;
    }

    return res;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    PERROR res;
    ULONG *args;
    struct Values val = {TRUE,TRUE}, *opt;
    STRPTR argbuf;

    if( opt = GetOptions( MYNAME ) ) {
        val = *opt;
    }

    argbuf = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );
    if(args) {
        val.x = args[0];
        val.y = args[1];
    }

    if( (res = DoGUI(frame,&val,PPTBase)) == PERR_OK ) {
        SPrintF( argbuf, "%s %s", val.x ? "X" : "", val.y ? "Y" : "" );
    } else {
        SetErrorCode( frame, PERR_WINDOWOPEN );
    }

    return res;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    PERROR res;
    FRAME *newframe = NULL;
    ULONG *args;
    struct Values val = {TRUE,TRUE}, *opt;

    if( opt = GetOptions( MYNAME ) ) {
        val = *opt;
    }

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );
    if(args) {
        val.x = args[0];
        val.y = args[1];

        if(newframe = DupFrame(frame, DFF_COPYDATA)) {
            res = DoTheFlip( PPTBase, frame, newframe, &val );
            if(res) {
                RemFrame(newframe);
                newframe = NULL;
            }
        }
        return newframe;
    }

    if( (res = DoGUI(frame,&val,PPTBase)) == PERR_OK ) {

        if(newframe = DupFrame(frame, DFF_COPYDATA)) {
            res = DoTheFlip( PPTBase, frame, newframe, &val );
            if(res) {
                SetErrorCode( frame, res );
                RemFrame(newframe);
                newframe = NULL;
            }
        } /* if */

    } else {
        SetErrorCode( frame, PERR_WINDOWOPEN );
    }

    PutOptions( MYNAME, &val, sizeof(val) );

    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

