/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : convolute

    PPT and this file are (C) Janne Jalkanen 2000.

    $Id: convolute.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

// #define DEBUG_MODE 1

#include <pptplugin.h>
#include <stdarg.h>
#include <string.h>

#include "convolute.h"
#include <libraries/asl.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME "Convolute"

/* Need to recreate some bgui macros to use my own tag routines. */

#define HGroupObject          MyNewObject( xd, BGUI_GROUP_GADGET
#define VGroupObject          MyNewObject( xd, BGUI_GROUP_GADGET, GROUP_Style, GRSTYLE_VERTICAL
#define ButtonObject          MyNewObject( xd, BGUI_BUTTON_GADGET
#define CheckBoxObject        MyNewObject( xd, BGUI_CHECKBOX_GADGET
#define WindowObject          MyNewObject( xd, BGUI_WINDOW_OBJECT
#define SeperatorObject       MyNewObject( xd, BGUI_SEPERATOR_GADGET
#define WindowOpen(wobj)      (struct Window *)DoMethod( wobj, WM_OPEN )
#define StringObject          MyNewObject( xd, BGUI_STRING_GADGET
#define InfoObject            MyNewObject( xd, BGUI_INFO_GADGET
#define FileReqObject         MyNewObject( xd, BGUI_FILEREQ_OBJECT

/* This file defines the temporary file name for GETARGS */

#define TEMPFILENAME "T:ppt_convolute.tmp"

/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables */

const char info_blurb[] =
    "This is a generic convolution filter\n"
    "that is able to use up to 7x7 masks\n"
    "(check menus for nice presets)\n";

const struct TagItem MyTagArray[] = {
    PPTX_Name,          (ULONG)MYNAME,
    /* Other tags go here */
    PPTX_Author,        (ULONG)"Janne Jalkanen, 1996-2000",
    PPTX_InfoTxt,       (ULONG)info_blurb,
    PPTX_NoNewFrame,    TRUE,
    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,
    PPTX_RexxTemplate,  (ULONG)"FILE/A",
    PPTX_ReqPPTVersion, 3L,
    PPTX_SupportsGetArgs,TRUE,
#ifdef _M68020
    PPTX_CPU,           (ULONG)AFF_68020,
#endif
#ifdef _PPC
    PPTX_CPU,           (ULONG)AFF_PPC,
#endif
    TAG_END, 0L
};

const struct NewMenu Menus[] = {
    Title("Project"),
        Item("Clear",NULL, GID_CLEAR),
        Item("Load",NULL, GID_LOAD),
        Item("Save",NULL, GID_SAVE),
        ItemBar,
        Item("Convolute!",NULL,GID_OK),
        Item("Quit",NULL, GID_CANCEL),
    Title("Presets"),
        Item("Average",NULL,NULL),
            SubItem("Normal 3x3",NULL,GID_AVG_33),
            SubItem("Normal 5x5",NULL,GID_AVG_55),
            SubItem("Normal 7x7",NULL,GID_AVG_77),
            SubItem("Gauss4 3x3",NULL,GID_BLUR4_33),
            SubItem("Gauss8 3x3",NULL,GID_BLUR8_33),
        Item("Edge Detect",NULL,NULL),
            SubItem("Laplace 4",NULL,GID_LAPLACE_4),
            SubItem("Laplace 8",NULL,GID_LAPLACE_8),
            SubItem("Roberts 1",NULL,GID_ROBERTS_1),
            SubItem("Roberts 2",NULL,GID_ROBERTS_2),
        Item("Line Detect", NULL, NULL),
            SubItem("Prewitt 1",NULL,GID_PREWITT_1),
            SubItem("Prewitt 2",NULL,GID_PREWITT_2),
            SubItem("Sobel 1",NULL,GID_SOBEL_1),
            SubItem("Sobel 2",NULL,GID_SOBEL_2),
            SubItem("Kirsch 1",NULL,GID_KIRSCH_1),
            SubItem("Kirsch 2",NULL,GID_KIRSCH_2),
    End
};

#ifdef __PPC__
#include <powerup/ppclib/interface.h>
// #include <powerup/pragmas/ppc_pragmas.h>

struct TagItem *__MyTagArray = MyTagArray;
void *__LIBEffectExec = EffectExec;
void *__LIBEffectInquire = EffectExec;

extern _m68kDoMethodA();

__inline ULONG DoMethodA( Object *obj, Msg msg )
{
    struct Caos c;

    c.caos_Un.Function = (APTR)_m68kDoMethodA;
    c.M68kCacheMode = IF_CACHEFLUSHALL;
    c.PPCCacheMode  = IF_CACHEFLUSHALL;
    c.a0 = (ULONG)obj;
    c.a1 = (ULONG)msg;

    return PPCCallM68k( &c );
}

ULONG DoMethod(Object *obj, ULONG MethodID, ... )
{
    return DoMethodA( obj, (Msg)&MethodID );
}

#endif

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
#ifndef __PPC__
void __regargs __chkabort(void) {}
#endif
void __regargs _CXBRK(void) {}
#endif

/*
    My replacement for BGUI_NewObject() - routine.
    Delete if you don't need it.
*/

Object *MyNewObject( struct PPTBase *xd, ULONG classid, Tag tag1, ... )
{
    struct Library *BGUIBase = xd->lb_BGUI;

    return(BGUI_NewObjectA( classid, (struct TagItem *) &tag1));
}



int GetConvArgs( FRAME *frame, struct TagItem *tags, struct PPTBase *xd, struct convargs *cargs )
{
    Object *Win, *wtgroup, *weights[7][7], *Bias, *Frq, *Name, *Div;
    ULONG sigmask, sig, rc;
    int quit = FALSE, res = PERR_OK,i,j;
    struct Window *win;
    struct IntuitionBase *IntuitionBase = xd->lb_Intuition;
    struct ExecBase *SysBase = xd->lb_Sys;
    struct Library *BGUIBase = xd->lb_BGUI;
    char *name = "Unknown";

#ifdef DEBUG_MODE
    PDebug("\tCreating window object\n");
#endif

    Win = WindowObject,
        WINDOW_Screen,    xd->g->maindisp->scr,
        WINDOW_Title,     frame->nd.ln_Name,
        WINDOW_Font,      xd->g->userprefs->mainfont,
        WINDOW_ScreenTitle, "Convolute Tool",
        WINDOW_ScaleWidth, 20,
        WINDOW_MenuStrip, Menus,
        TAG_SKIP,         (cargs->winpos.Height == 0) ? 1 : 0,
        WINDOW_Bounds,    &(cargs->winpos),
        WINDOW_NoBufferRP, TRUE,
        WINDOW_MasterGroup,
            VGroupObject, Spacing(4), HOffset(6), VOffset(4),
                StartMember,
                    Name = InfoFixed("Name:","",NULL,1),
                EndMember,
                StartMember,
                    HGroupObject, Spacing(4), HOffset(4),
                        StartMember,
                            /* Grid */
                            wtgroup = VGroupObject,
                                StartMember,
                                    HGroupObject,
                                        StartMember,
                                            weights[0][0] = Integer(NULL,0,3,GID_W00),
                                        EndMember,
                                        StartMember,
                                            weights[0][1] = Integer(NULL,0,3,GID_W01),
                                        EndMember,
                                        StartMember,
                                            weights[0][2] = Integer(NULL,0,3,GID_W02),
                                        EndMember,
                                        StartMember,
                                            weights[0][3] = Integer(NULL,0,3,GID_W03),
                                        EndMember,
                                        StartMember,
                                            weights[0][4] = Integer(NULL,0,3,GID_W04),
                                        EndMember,
                                        StartMember,
                                            weights[0][5] = Integer(NULL,0,3,GID_W05),
                                        EndMember,
                                        StartMember,
                                            weights[0][6] = Integer(NULL,0,3,GID_W06),
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject,
                                        StartMember,
                                            weights[1][0] = Integer(NULL,0,3,GID_W10),
                                        EndMember,
                                        StartMember,
                                            weights[1][1] = Integer(NULL,0,3,GID_W11),
                                        EndMember,
                                        StartMember,
                                            weights[1][2] = Integer(NULL,0,3,GID_W12),
                                        EndMember,
                                        StartMember,
                                            weights[1][3] = Integer(NULL,0,3,GID_W13),
                                        EndMember,
                                        StartMember,
                                            weights[1][4] = Integer(NULL,0,3,GID_W14),
                                        EndMember,
                                        StartMember,
                                            weights[1][5] = Integer(NULL,0,3,GID_W15),
                                        EndMember,
                                        StartMember,
                                            weights[1][6] = Integer(NULL,0,3,GID_W16),
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject,
                                        StartMember,
                                            weights[2][0] = Integer(NULL,0,3,GID_W20),
                                        EndMember,
                                        StartMember,
                                            weights[2][1] = Integer(NULL,0,3,GID_W21),
                                        EndMember,
                                        StartMember,
                                            weights[2][2] = Integer(NULL,0,3,GID_W22),
                                        EndMember,
                                        StartMember,
                                            weights[2][3] = Integer(NULL,0,3,GID_W23),
                                        EndMember,
                                        StartMember,
                                            weights[2][4] = Integer(NULL,0,3,GID_W24),
                                        EndMember,
                                        StartMember,
                                            weights[2][5] = Integer(NULL,0,3,GID_W25),
                                        EndMember,
                                        StartMember,
                                            weights[2][6] = Integer(NULL,0,3,GID_W26),
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject,
                                        StartMember,
                                            weights[3][0] = Integer(NULL,0,3,GID_W30),
                                        EndMember,
                                        StartMember,
                                            weights[3][1] = Integer(NULL,0,3,GID_W31),
                                        EndMember,
                                        StartMember,
                                            weights[3][2] = Integer(NULL,0,3,GID_W32),
                                        EndMember,
                                        StartMember,
                                            weights[3][3] = Integer(NULL,1,3,GID_W33),
                                        EndMember,
                                        StartMember,
                                            weights[3][4] = Integer(NULL,0,3,GID_W34),
                                        EndMember,
                                        StartMember,
                                            weights[3][5] = Integer(NULL,0,3,GID_W35),
                                        EndMember,
                                        StartMember,
                                            weights[3][6] = Integer(NULL,0,3,GID_W36),
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject,
                                        StartMember,
                                            weights[4][0] = Integer(NULL,0,3,GID_W40),
                                        EndMember,
                                        StartMember,
                                            weights[4][1] = Integer(NULL,0,3,GID_W41),
                                        EndMember,
                                        StartMember,
                                            weights[4][2] = Integer(NULL,0,3,GID_W42),
                                        EndMember,
                                        StartMember,
                                            weights[4][3] = Integer(NULL,0,3,GID_W43),
                                        EndMember,
                                        StartMember,
                                            weights[4][4] = Integer(NULL,0,3,GID_W44),
                                        EndMember,
                                        StartMember,
                                            weights[4][5] = Integer(NULL,0,3,GID_W45),
                                        EndMember,
                                        StartMember,
                                            weights[4][6] = Integer(NULL,0,3,GID_W46),
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject,
                                        StartMember,
                                            weights[5][0] = Integer(NULL,0,3,GID_W50),
                                        EndMember,
                                        StartMember,
                                            weights[5][1] = Integer(NULL,0,3,GID_W51),
                                        EndMember,
                                        StartMember,
                                            weights[5][2] = Integer(NULL,0,3,GID_W52),
                                        EndMember,
                                        StartMember,
                                            weights[5][3] = Integer(NULL,0,3,GID_W53),
                                        EndMember,
                                        StartMember,
                                            weights[5][4] = Integer(NULL,0,3,GID_W54),
                                        EndMember,
                                        StartMember,
                                            weights[5][5] = Integer(NULL,0,3,GID_W55),
                                        EndMember,
                                        StartMember,
                                            weights[5][6] = Integer(NULL,0,3,GID_W56),
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject,
                                        StartMember,
                                            weights[6][0] = Integer(NULL,0,3,GID_W60),
                                        EndMember,
                                        StartMember,
                                            weights[6][1] = Integer(NULL,0,3,GID_W61),
                                        EndMember,
                                        StartMember,
                                            weights[6][2] = Integer(NULL,0,3,GID_W62),
                                        EndMember,
                                        StartMember,
                                            weights[6][3] = Integer(NULL,0,3,GID_W63),
                                        EndMember,
                                        StartMember,
                                            weights[6][4] = Integer(NULL,0,3,GID_W64),
                                        EndMember,
                                        StartMember,
                                            weights[6][5] = Integer(NULL,0,3,GID_W65),
                                        EndMember,
                                        StartMember,
                                            weights[6][6] = Integer(NULL,0,3,GID_W66),
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject, Spacing(4),
                                        StartMember,
                                            Bias = Integer("Bias:",0,4,GID_BIAS),
                                        EndMember,
                                        StartMember,
                                            Div = Integer("Div:",1,4,GID_DIV),
                                        EndMember,
                                    EndObject,
                                EndMember,
                            EndObject, Weight(75),
                        EndMember,
                        StartMember,
                            VGroupObject, Spacing(4),
                                StartMember,
                                    XenButton("Load",GID_LOAD),
                                EndMember,
                                StartMember,
                                    XenButton("Save",GID_SAVE),
                                EndMember,
                                StartMember,
                                    XenButton("Clear",GID_CLEAR),
                                EndMember,
                                StartMember,
                                    XenButton("Convolute!",GID_OK),
                                EndMember,
                                StartMember,
                                    XenButton("Cancel",GID_CANCEL),
                                EndMember,
                            EndObject, Weight(10),
                        EndMember,
                    EndObject,
                EndMember,
            EndObject, /* MasterVGroup */
        EndObject; /* Window */

    if(Win) {
#ifdef DEBUG_MODE
        PDebug("\tSucceeded in creating window\n");
#endif

        for(i = 0; i < 7; i++) {
            for(j = 0; j < 7; j++) {
                SetGadgetAttrs( (struct Gadget *)weights[i][j], NULL, NULL,
                                STRINGA_LongVal, cargs->weights[i][j], TAG_DONE );
            }
        }
        SetGadgetAttrs( (struct Gadget *)Bias, NULL, NULL, STRINGA_LongVal, cargs->bias, TAG_DONE);
        SetGadgetAttrs( (struct Gadget *)Div, NULL, NULL, STRINGA_LongVal, cargs->div, TAG_DONE);
        SetGadgetAttrs( (struct Gadget *)Name, NULL, NULL, INFO_TextFormat, cargs->name, TAG_DONE);

        if( win = WindowOpen( Win ) ) {
#ifdef DEBUG_MODE
            PDebug("\tOpened window OK\n");
#endif
            Frq = FileReqObject,
                    ASLFR_Window, win,
                    ASLFR_SleepWindow, TRUE,
                    ASLFR_InitialDrawer, "PROGDIR:modules/convolutions",
                    ASLFR_InitialPattern,"~(#?.info)",
                  EndObject;
            GetAttr( WINDOW_SigMask, Win, &sigmask );

            while(!quit) {
                sig = Wait(sigmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);

                /*
                 *  Break signal
                 */

                if( sig & SIGBREAKF_CTRL_C ) {
                    quit = TRUE;
                    res = PERR_BREAK;
                }

                if( sig & SIGBREAKF_CTRL_F ) {
                    WindowToFront( win );
                    ActivateWindow( win );
                }

                /*
                 *  Gadget message
                 */

                if( sig & sigmask ) {
                    while(( rc = HandleEvent( Win )) != WMHI_NOMORE ) {
                        struct TagItem clrtable[] = { STRINGA_LongVal, 0, TAG_END };
                        struct TagItem nonametable[] = { INFO_TextFormat, (ULONG)"unnamed", TAG_END };
                        struct TagItem settable[] = {STRINGA_LongVal, 0, TAG_END };
                        struct TagItem frqloadtable[] = {ASLFR_DoSaveMode, FALSE, TAG_END };
                        struct TagItem newnametable[] = {INFO_TextFormat, NULL, TAG_END };
                        struct TagItem frqsavetable[] = {ASLFR_DoSaveMode, TRUE, TAG_END };
                        struct TagItem nametable[] = {INFO_TextFormat, NULL, TAG_END };
                        int id;

                        switch(rc) {
                            case WMHI_CLOSEWINDOW:
                            case GID_CANCEL:
                                res = PERR_CANCELED;
                                quit = TRUE;
                                WindowClose(Win);
                                break;

                            case GID_CLEAR:
                                WindowBusy(Win);
                                clrtable[0].ti_Data = 0;
                                SetGadgetAttrsA( (struct Gadget *)Bias, win, NULL, clrtable);
                                for(i = 0; i < 7; i++) {
                                    for(j = 0; j < 7; j++) {
                                        SetGadgetAttrsA( (struct Gadget *)weights[i][j], win,
                                                         NULL, clrtable );
                                        cargs->weights[i][j] = 0;
                                    }
                                }
                                clrtable[0].ti_Data = 1; /* For Divisor & Middle value. */

                                SetGadgetAttrsA( (struct Gadget *)weights[3][3], win,
                                                 NULL, clrtable );
                                cargs->weights[3][3] = 1;

                                clrtable[0].ti_Data = 1; /* For Divisor */
                                SetGadgetAttrsA( (struct Gadget *)Div, win, NULL, clrtable);
                                SetGadgetAttrsA( (struct Gadget *) Name, win, NULL, nonametable);
                                WindowReady(Win);
                                break;

                            case GID_LOAD:
                                SetAttrsA( Frq, frqloadtable );
                                if(DoRequest( Frq ) == FRQ_OK) {
                                    GetAttr( FRQ_Path, Frq, (ULONG *)&name );
                                    WindowBusy(Win);
                                    if(LoadConvFilter( xd, name, cargs ) == PERR_OK ) {
                                        for(i = 0; i < 7; i++) {
                                            for(j = 0; j < 7; j++) {
                                                settable[0].ti_Data = cargs->weights[i][j];
                                                SetGadgetAttrsA( (struct Gadget *)weights[i][j], win,
                                                    NULL, settable );
                                            }
                                        }
                                        settable[0].ti_Data = cargs->bias;
                                        SetGadgetAttrsA( (struct Gadget *)Bias, win, NULL, settable);
                                        settable[0].ti_Data = cargs->div;
                                        SetGadgetAttrsA( (struct Gadget *)Div, win, NULL, settable);
                                        GetAttr( FRQ_File, Frq, (ULONG *) &newnametable[0].ti_Data );
                                        SetGadgetAttrsA( (struct Gadget *) Name, win, NULL, newnametable);
                                    }
                                    WindowReady(Win);
                                }
                                break;

                            case GID_SAVE:
                                SetAttrsA( Frq, frqsavetable );
                                if(DoRequest(Frq) == FRQ_OK) {
                                    WindowBusy(Win);
                                    GetAttr( FRQ_Path, Frq, (ULONG *)&name );
                                    for(i = 0; i < 7; i++) {
                                        for(j = 0; j < 7; j++) {
                                            GetAttr( STRINGA_LongVal, weights[i][j], (ULONG *) &(cargs->weights[i][j]) );
                                        }
                                    }
                                    GetAttr( STRINGA_LongVal, Bias, (ULONG *) &(cargs->bias));
                                    GetAttr( STRINGA_LongVal, Div, (ULONG *) &(cargs->div));
                                    SaveConvFilter( xd, name, cargs );
                                    GetAttr( FRQ_File, Frq, &nametable[0].ti_Data );
                                    SetGadgetAttrsA( (struct Gadget *) Name, win, NULL, nametable);
                                    WindowReady(Win);
                                }
                                break;

                            case GID_OK:
                                GetAttr( STRINGA_LongVal, Bias, (ULONG *) &(cargs->bias));
                                GetAttr( STRINGA_LongVal, Div, (ULONG *) &(cargs->div));
                                GetAttr( WINDOW_Bounds, Win, (ULONG *) &(cargs->winpos) );
                                if( cargs->div != 0 ) {
                                    WindowClose(Win);
                                    for(i = 0; i < 7; i++) {
                                        for(j = 0; j < 7; j++) {
                                            GetAttr( STRINGA_LongVal, weights[i][j], (ULONG *) &(cargs->weights[i][j]) );
                                        }
                                    }
                                    GetAttr( STRINGA_LongVal, Bias, (ULONG *) &(cargs->bias));
                                    GetAttr( STRINGA_LongVal, Div, (ULONG *) &(cargs->div));
                                    strncpy( cargs->name, name, 40 );
                                    res = PERR_OK;
                                    quit = TRUE;
                                }
                                break;

                            default:

                                if(rc >= OPR(0) && rc <= OPR(1000)) { /* It's a default opr */
                                    WindowBusy(Win);
                                    for(id = 0; presets[id].id != rc && presets[id].id != 0L; id++); /* Fetch correct area. */
                                    if(presets[id].id) {
                                        for(i = 0; i < 7; i++) {
                                            for(j = 0; j < 7; j++) {
                                                settable[0].ti_Data = cargs->weights[i][j] = presets[id].weights[i][j];
                                                SetGadgetAttrsA( (struct Gadget *)weights[i][j], win,
                                                    NULL, settable );
                                            }
                                        }
                                        settable[0].ti_Data = cargs->bias = presets[id].bias;
                                        SetGadgetAttrsA( (struct Gadget *)Bias, win, NULL, settable);
                                        settable[0].ti_Data = cargs->div = presets[id].div;
                                        SetGadgetAttrsA( (struct Gadget *)Div,  win, NULL, settable);
                                        newnametable[0].ti_Data = name = (ULONG)presets[id].name;
                                        SetGadgetAttrsA( (struct Gadget *)Name, win, NULL, newnametable);
                                    }
#ifdef DEBUG_MODE
                                    else {
                                        PDebug("Serious software error!!!\n");
                                    }
#endif
                                    WindowReady(Win);
                                }
                                break;

                        } /* switch */
                    } /* while */
                } /* sig & sigmask */

            } /* while(!quit) */

            DisposeObject(Win);
            DisposeObject(Frq);
        } else {
#ifdef DEBUG_MODE
            PDebug("\tFailed to get window\n");
#endif
            DisposeObject(Win);
            return PERR_WONTOPEN;
        }
    } else {
#ifdef DEBUG_MODE
        PDebug("\tFailed to get window object\n");
#endif
        return PERR_WONTOPEN;
    }

    return res;
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

VOID SetDefaults(struct convargs *args)
{
    args->weights[3][3] = 1;
    args->size = 3;
    args->bias = 0;
    args->div  = 1;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    struct convargs args = {0}, *saved;
    PERROR res;
    ULONG *rxargs;
    STRPTR buffer;

    SetDefaults(&args);

    if( saved = GetOptions(MYNAME) ) {
        args = *saved;
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    rxargs = (ULONG *)TagData( PPTX_RexxArgs, tags );
    if(rxargs) {
        if( (res = LoadConvFilter( PPTBase, (UBYTE *)rxargs[0], &args )) != PERR_OK ) {
            SetErrorMsg(frame,"Couldn't open the specified convolution file");
            return PERR_FILEREAD;
        }
        strncpy( args.name, FilePart( (UBYTE *)rxargs[0] ), 40 );
    }

    if( (res = GetConvArgs( frame, tags, PPTBase, &args )) == PERR_OK ) {
        SaveConvFilter( PPTBase, TEMPFILENAME, &args );
        SPrintF( buffer, "FILE %s", TEMPFILENAME );
    }

    return res;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    struct Library *UtilityBase = PPTBase->lb_Utility;
    struct DosLibrary *DOSBase  = PPTBase->lb_DOS;
    FRAME *newframe = NULL;
    struct convargs args = {0}, *saved;
    PERROR res;
    ULONG *rxargs;

    SetDefaults(&args);

    if( saved = GetOptions(MYNAME) ) {
        args = *saved;
    }

    rxargs = (ULONG *)TagData( PPTX_RexxArgs, tags );
    if(rxargs) {
        if( (res = LoadConvFilter( PPTBase, (UBYTE *)rxargs[0], &args )) != PERR_OK ) {
            SetErrorMsg(frame,"Couldn't open the specified convolution file");
            return NULL;
        }
        strncpy( args.name, FilePart( (UBYTE *)rxargs[0] ), 40 );
    } else {
        res = GetConvArgs( frame, tags, PPTBase, &args );
    }

    if( res == PERR_OK ) {
        /*
         *  Make the actual convolution
         */

        if(newframe = DupFrame( frame, DFF_COPYDATA )) {
            res = DoConvolute( frame, newframe, &args, PPTBase );
            if(res != PERR_OK) {
                RemFrame(newframe);
                newframe = NULL;
                SetErrorCode( frame, res );
            }
        } else {
            SetErrorMsg(frame,"Unable to duplicate frame");
        }

        PutOptions(MYNAME,&args,sizeof(struct convargs));
    }

    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

