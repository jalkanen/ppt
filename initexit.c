/*----------------------------------------------------------------------*/
/*
    PROJECT: PPT
    MODULE : initexit.c

    $Id: initexit.c,v 1.36 1998/11/08 00:45:25 jj Exp $

    Initialization and exit code.
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"
#include "rexx.h"

#include "version.h"

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef LIBRARIES_ASL_H
#include <libraries/asl.h>
#endif

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

#ifndef PROTO_DISKFONT_H
#include <proto/diskfont.h>
#endif

#ifndef PROTO_ASL_H
#include <proto/asl.h>
#endif

#ifndef PROTO_LOCALE_H
#include <proto/locale.h>
#endif

#include <clib/alib_protos.h>

#include <stdlib.h>



/*----------------------------------------------------------------------*/
/* Defines */

#define BREAK_HOW_MANY_TIMES    10
#define BREAK_DELAY_PERIOD      15

#undef USE_LOGOIMAGE

/*----------------------------------------------------------------------*/
/* Internal prototypes */

extern int CloseMainScreen( void ); /* From display.c */

Prototype int Initialize( void );
Prototype int FreeResources (GLOBALS *);
Prototype void Panic( const char * );


/*----------------------------------------------------------------------*/
/*  Globals. Also, some of these are here so that the system could allocate
    space for us and we wouldn't need to do it. We just point at it from a
    different structure. */

struct Library *BGUIBase = NULL, *GadToolsBase = NULL, *LocaleBase = NULL,
               *CyberGfxBase = NULL, *PPCLibBase = NULL;
struct Device  *TimerBase = NULL;
struct FontRequester *fontreq = NULL;
struct FileRequester *filereq = NULL;

struct Locale  *MyLocale = NULL;
struct Catalog *MyCatalog = NULL;

BOOL MasterQuit = FALSE;

/*
    The default fonts.
*/
const struct TextAttr defaultmainfont = {
    "topaz.font",
    8,
    FS_NORMAL,
    0
};

const struct TextAttr defaultlistfont = {
    "topaz.font",
    8,
    FS_NORMAL,
    0
};


/*----------------------------------------------------------------------*/
/* Locals */

const struct TextAttr startupwinfont1 =
{ "helvetica.font", 24, 0, FPF_DISKFONT };

struct TextFont *sfont1 = NULL;

const struct TextAttr startupwinfont2 =
{ "topaz.font", 8, 0, FPF_ROMFONT };

struct TextFont *sfont2 = NULL;

Object *Win_Startup = NULL, *GO_StartupText = NULL;
struct Window *win_Startup;
struct Screen *scr_Startup;
#ifdef USE_LOGOIMAGE
FRAME  *startupframe;
#endif

const char initblurb1[] =
    ISEQ_C ISEQ_B ISEQ_HIGHLIGHT "Welcome to PPT!";

const char initblurb2[] =
    ISEQ_C ISEQ_TEXT "Release " RELEASE " (v"VERSION", "VERSDATE")\n"
    ISEQ_C COPYRIGHT"\n\n"
    ISEQ_C ISEQ_I "PPT is starting... Please wait!";

const char initblurb3[] =
    "Initializing...";

/*----------------------------------------------------------------------*/
/* Code */
/// OpenStartupFrame
#ifdef USE_LOGOIMAGE
FRAME *OpenStartupFrame(VOID)
{
    FRAME *f;
    extern UWORD logoimagec_height, logoimagec_width;
    extern UBYTE __far logoimagec_data[];

    D(bug("Allocating startup frame with %lu bytes\n",
          logoimagec_height*logoimagec_width*3));

    if(f = MakeFrame( NULL, globxd )) {
        f->pix->height = logoimagec_height;
        f->pix->width  = logoimagec_width;
        f->pix->colorspace = CS_RGB;
        f->pix->vm_mode = VMEM_NEVER;

        if( InitFrame( f, globxd  ) == PERR_OK ) {
            memcpy( f->pix->vmh->data, logoimagec_data, logoimagec_height*logoimagec_width*3);
            return f;
        }

        RemFrame( f, globxd );
    }

    return NULL;
}
#endif
///
/// OpenStartupWindow
/*
    I'm really bored.  So this is the new startup window:
    Requires:  Libraries open
*/

VOID OpenStartupWindow(VOID)
{
    char *blurb2args[] = { RELEASE, VERSION, VERSDATE, COPYRIGHT };
#ifdef USE_LOGOIMAGE
    Object *Area;
#endif

    if( MAINSCR )
        scr_Startup = MAINSCR;
    else
        scr_Startup = NULL;

    sfont1 = OpenDiskFont( &startupwinfont1 ); // BGUI will fall back.
    sfont2 = OpenDiskFont( &startupwinfont2 );

    Win_Startup = WindowObject,
        WINDOW_Screen,      scr_Startup,
        WINDOW_SizeGadget,  FALSE,
        WINDOW_Borderless,  TRUE,
        WINDOW_ScreenTitle, std_ppt_blurb,
        WINDOW_ScaleWidth,  55,
        WINDOW_ScaleHeight, 25,
        WINDOW_LockWidth,   TRUE,
        WINDOW_LockHeight,  TRUE,
        WINDOW_DragBar,     FALSE,
        WINDOW_CloseGadget, FALSE,
        WINDOW_DepthGadget, FALSE,
        WINDOW_MasterGroup,
            HGroupObject,
#ifdef USE_LOGOIMAGE
                StartMember,
                    Area = AreaObject,
                        AREA_MinWidth, logoimagec_width,
                        AREA_MinHeight, logoimagec_height,
                        ButtonFrame,
                    EndObject, FixMinSize,
                EndMember,
#endif
                StartMember,
                    VGroupObject, StringFrame,
                        // GROUP_BackFill,SHINE_RASTER,
                        VarSpace(50),
                        StartMember,
                            InfoObject,
                                INFO_TextFormat, GetStr(mINITBLURB1),
                                BT_TextAttr,     &startupwinfont1,
                                // FRM_BackFill,    SHINE_RASTER,
                                FRM_Type,        FRTYPE_NONE,
                            EndObject,
                        EndMember,
                        VarSpace(40),
                        StartMember,
                            InfoObject,
                                INFO_TextFormat, GetStr(mINITBLURB2),
                                INFO_Args,       blurb2args,
                                INFO_MinLines,   4,
                                BT_TextAttr,     &startupwinfont2,
                                // FRM_BackFill,    SHINE_RASTER,
                                FRM_Type,        FRTYPE_NONE,
                            EndObject,
                        EndMember,
                        VarSpace(50),
                        StartMember,
                            GO_StartupText = InfoObject,
                                INFO_TextFormat, GetStr(mINITBLURB3),
                                BT_TextAttr,     &startupwinfont2,
                                // FRM_BackFill,    SHINE_RASTER,
                                FRM_Type,        FRTYPE_NONE,
                            EndObject,
                        EndMember,
                    EndObject,
                EndMember,
           EndObject,
    EndObject;

    if(Win_Startup) {
        struct IBox *area;

        win_Startup = WindowOpen( Win_Startup );
        WindowBusy( Win_Startup );
#ifdef USE_LOGOIMAGE
        GetAttr( AREA_AreaBox, Area, (ULONG *)&area );
        if(!startupframe)
            startupframe = OpenStartupFrame();

        if( startupframe && MAINSCR && scr_Startup == MAINSCR ) {
            RenderFrame( startupframe, win_Startup->RPort, area, 0L, globxd );
        }
#endif
    } else {
        D(bug("\tWARNING! Couldn't open startup window!\n"));
    }
}
///
/// UpdateStartupWindow
/*
 *  Will also check if the main screen is open and switch to that.
 */

Prototype VOID UpdateStartupWindow( const char *text );

VOID UpdateStartupWindow( const char *text )
{
    if( MAINSCR && scr_Startup != MAINSCR ) {
        CloseStartupWindow();
        OpenStartupWindow();
    }

    if( GO_StartupText ) {
        SetGadgetAttrs( (struct Gadget *)GO_StartupText, win_Startup, NULL,
                        INFO_TextFormat, text, TAG_DONE );
        // Delay(100);
    }
}
///
/// CloseStartupWindow
Prototype VOID CloseStartupWindow(VOID);

VOID CloseStartupWindow(VOID)
{
    if(Win_Startup) DisposeObject(Win_Startup);
    Win_Startup = NULL;
    if(sfont1) CloseFont( sfont1 );
    if(sfont2) CloseFont( sfont2 );
    sfont1 = sfont2 = NULL;
#ifdef USE_LOGOIMAGE
    if( startupframe ) RemFrame( startupframe, globxd );
#endif
}
///
/*
    Main initialization subroutine.

    Note:  we have these message ports set up:

    globals->mport = globxd->mport : inter-PPT process communication port
    MainIDCMPPort : shared port for all windows to use for IDCMP traffic
    rxhost->port : REXX communications port
    Amigaguide port : For AmigaGuide communications
*/

int Initialize( void )
{
    PREFS *p;
    struct Screen *pubscr = NULL;
    struct DrawInfo *drinfo = NULL;
    struct EasyStruct es = {
        sizeof(struct EasyStruct),0L,"PPT library request",
        "Sorry, but I need %s.library V%ld+","Gotcha"
    };

    D(bug("Initialize()\n"));

    if( SysBase->LibNode.lib_Version < 37 ) { /* OS 2.0 only, sorry. */
        Write(Output(),"Needs OS V37+",14); /* BUG: requires CR */
        DisplayBeep(NULL);
        exit(RETURN_FAIL);
    }

    /*
     *  Global variables structure
     *  Sets the real pointers too.
     */

//    DEBUG("\tAllocating globals\n");
    globals = pzmalloc(sizeof(GLOBALS) );
    if(!globals) {
        exit(20);
    }

    if(!(globals->maindisp = pzmalloc( sizeof( DISPLAY ) ))) {
        pfree( globals );
        exit(20);
    }

    if(!(globals->userprefs = pzmalloc(sizeof(PREFS)) )) {
        pfree(globals); pfree(globals->maindisp);
        exit(20);
    }

    p = globals->userprefs; /* For short. */

    D(bug("\tOpening BGUI\n"));

    if( (BGUIBase = OpenLibrary( BGUINAME, BGUI_VERSION_REQUIRED )) == NULL) {
        D(bug("\t\tFAILED!\n"));
        EasyRequest(NULL, &es, NULL, "bgui", BGUI_VERSION_REQUIRED );
        pfree(globals->maindisp);
        pfree(globals->userprefs);
        pfree(globals);
        exit(RETURN_FAIL);
    }

    if( (GadToolsBase = OpenLibrary( "gadtools.library", 37L )) == NULL ) {
        D(bug("Could not open gadtools\n"));
        EasyRequest(NULL, &es, NULL, "gadtools", 37L );
        pfree(globals->maindisp);
        pfree(globals->userprefs);
        pfree(globals);
        CloseLibrary(BGUIBase);
        exit(RETURN_FAIL);
    }

    if( (LocaleBase = OpenLibrary( "locale.library", 0L )) == NULL ) {
        D(bug("WARNING: Couldn't open locale.library!\n"));
    } else {
        MyLocale   = OpenLocale( NULL );
        if(!MyLocale) D(bug("WARNING: Couldn't open the default locale!\n"));
    }

    CyberGfxBase = OpenLibrary("cybergraphics.library",40L);

    /*
     *  Set up library base vectors for this task.
     */

//    DEBUG("\tSetting up globxd\n");
    globxd = NewExtBase( FALSE ); /* Don't open libraries nor message ports */
    globxd->g = globals;
    globxd->lb_BGUI      = BGUIBase;
    globxd->lb_DOS       = (struct DosLibrary *)DOSBase;
    globxd->lb_Intuition = IntuitionBase;
    globxd->lb_Gfx       = GfxBase;
    globxd->lb_Utility   = UtilityBase;
    globxd->lb_Sys       = SysBase;
    globxd->lb_GadTools  = GadToolsBase;
    globxd->lb_Locale    = LocaleBase;
    globxd->locale       = MyLocale;
    globxd->lb_CyberGfx  = CyberGfxBase;

    globxd->lib.lib_Version  = VERNUM;
    globxd->lib.lib_Revision = 0;

    /*
     *  Miscallaneous
     */

    globals->maintask = (struct Process *)FindTask(NULL);

    /*
     *  Initialize list structures
     */

    NewList( &globals->loaders );
    NewList( &globals->effects );
    NewList( &globals->frames );
    NewList( &globals->scripts );

    /*
     *  Semaphores
     */

    InitSemaphore( &globals->phore );

    /*
     *  Make main message ports
     */

    globals->mport = CreateMsgPort();
    if(!globals->mport)
        Panic( "Couldn't alloc a message port!" );

    globxd->mport  = globals->mport;

    MainIDCMPPort = CreateMsgPort();
    if(!MainIDCMPPort) Panic("Couldn't allocate main IDCMP port!");

    /*
     *  Timer device
     */

    if( globxd->TimerIO = CreateIORequest( globxd->mport, sizeof( struct timerequest ) ) ) {
        OpenDevice("timer.device", UNIT_ECLOCK, globxd->TimerIO, 0L );
        TimerBase = globxd->lb_Timer = globxd->TimerIO->tr_node.io_Device;
    }

    /*
     *  Memory pools & catalogs.  After this point it is legal to use
     *  GetStr().
     */

    if( OpenPool() != PERR_OK )
        D(bug("Could not use memory pools, defaulting to standard system\n"));

    OpenpptCatalog( NULL, NULL, globxd );
    if(!globxd->catalog) D(bug("WARNING: Couldn't open ppt.catalog!\n"));

    /*
     *  Initialize all the GUI locale settings
     */

    InitGUILocale();

    /*
     *  Check if a copy of PPT is already running!
     */

    if(pubscr = LockPubScreen( PPTPUBSCREENNAME )) {
        UnlockPubScreen(NULL, pubscr);
        Req( NEGNUL, NULL, GetStr( mINIT_PPT_ALREADY_RUNNING ) );
        return PERR_FAILED;
    }

    /*
     *  Open the startup window
     */

    OpenStartupWindow();

    /*
     *  Clone WB screen attributes for our usage.
     *  It would be actually quite a lot neater to use SA_LikeWorkBench...
     */

    D(bug("\tCloning WB screen\n"));
    pubscr = LockPubScreen(NULL); /* Get default public screen */
    if(pubscr) {
        drinfo = GetScreenDrawInfo(pubscr);
        if(drinfo) {
            globals->maindisp->dispid = GetVPModeID(&(pubscr->ViewPort));
            globals->maindisp->height = pubscr->Height;
            globals->maindisp->width  = pubscr->Width;
            /* globals->maindisp->depth  = drinfo->dri_Depth; */
            globals->maindisp->depth  = 5; /* DEFAULT_SCRDEPTH; */
            FreeScreenDrawInfo(pubscr,drinfo);
        }
        UnlockPubScreen(NULL,pubscr);
    }
    /* If the values didn't work up nicely, then use defaults */
    if( !pubscr || !drinfo || globals->maindisp->dispid == INVALID_ID ) {
        globals->maindisp->dispid = DEFAULT_DISPID;
        globals->maindisp->height = DEFAULT_SCRHEIGHT;
        globals->maindisp->width  = DEFAULT_SCRWIDTH;
        globals->maindisp->depth  = DEFAULT_SCRDEPTH;
        globals->userprefs->colorpreview = FALSE; /* Just in case */
    }

    /*
     *  Set up some things and
     *  load user preferences from the disk.
     */

    UpdateStartupWindow( GetStr(mINIT_LOADING_PREFERENCES) );

    p->mfont.ta_Name = p->mfontname;
    p->lfont.ta_Name = p->lfontname;

    D(bug("\tLoading Prefs\n"));
    LoadPrefs( globals, NULL );

    /*
     *  Initialize classes, etc.
     */

    DropAreaClass = InitDropAreaClass();
    if(!DropAreaClass) {
        Req(NEGNUL,NULL,"\nUnable to allocate BGUI area class!\n");
        return PERR_INITFAILED;
    }

    if(NULL == (RenderAreaClass = InitRenderAreaClass())) {
        Req(NEGNUL,NULL,"\nUnable to allocate BGUI area class!\n");
        return PERR_INITFAILED;
    }

    /*
     *  Act on user preferences: fonts
     *  If they do are not set, fall into default topaz 8.
     */

    if(p->mainfont) {
        if( (p->maintf = OpenDiskFont( p->mainfont )) == NULL)
            Req(NEGNUL,NULL,GetStr(mINIT_CANNOT_OPEN_FONT),
                p->mainfont->ta_Name, p->mainfont->ta_YSize);
    } else {
        p->maintf = OpenFont( &defaultmainfont );
    }

    if(p->listfont) {
        if( (p->listtf = OpenDiskFont( p->listfont )) == NULL)
            Req(NEGNUL,NULL,GetStr(mINIT_CANNOT_OPEN_FONT),
                p->listfont->ta_Name, p->listfont->ta_YSize);
    } else {
        p->listtf = OpenFont( &defaultlistfont );
    }


    /**********************************************************************
     *
     *  This is the end of any preferences or GUI related stuff.  We now
     *  start the main display and allocate any resources that are not
     *  graphics dependant.
     *
     *  Open main PPT display
     */

    UpdateStartupWindow(GetStr(mINIT_OPENING_DISPLAY));

    D(bug("\tOpening Display\n"));

    if(OpenDisplay() != PERR_OK) {
        globals->maindisp->dispid = DEFAULT_DISPID;
        globals->maindisp->height = DEFAULT_SCRHEIGHT;
        globals->maindisp->width  = DEFAULT_SCRWIDTH;
        globals->maindisp->depth  = DEFAULT_SCRDEPTH;
        globals->userprefs->colorpreview = FALSE; /* Just in case */
        if( OpenDisplay() != PERR_OK )
            Panic("Couldn't get screen!");
        else
            Req(NEGNUL,NULL,GetStr(mINIT_NO_PREFERRED_SCREEN) );
    }

    WindowBusy(globals->WO_main);

    /*
     *  Delete old virtual memory files
     */

    UpdateStartupWindow( GetStr(mINIT_CLEANING_UP) );
    CleanVMDirectory( globxd );

    UpdateStartupWindow( GetStr(mINIT_ALLOCING_RESOURCES) );

    /*
     *  Start up REXX
     */

    rxhost = InitRexx("PPT");

    /*
     *  Alloc global ASL stuff.
     *  REQ: Screen open, locale open.
     */

    fontreq = AllocAslRequestTags( ASL_FontRequest,
                                   ASLFO_Screen, globals->maindisp->scr,
                                   ASLFO_MaxHeight, FONT_MAXHEIGHT,
                                   TAG_END );
    if(fontreq == NULL) {
        Req(NEGNUL,NULL,GetStr(mINIT_NO_ASL_REQUESTER));
        return PERR_INITFAILED;
    }

    if( NULL == (filereq = AllocAslRequestTags( ASL_FileRequest,
                                                ASLFR_Screen, MAINSCR,
                                                ASLFR_InitialDrawer, "PROGDIR:",
                                                TAG_DONE ) ))
    {
        Req(NEGNUL,NULL,GetStr(mINIT_NO_ASL_REQUESTER));
        return PERR_INITFAILED;
    }

    gvLoadFileReq.Req = FileReqObject,
        ASLFR_Screen,       MAINSCR,
        ASLFR_Locale,       globxd->locale,
        ASLFR_InitialDrawer,globals->userprefs->startupdir,
        ASLFR_InitialFile,  globals->userprefs->startupfile,
        ASLFR_InitialTopEdge,   gvLoadFileReq.prefs.initialpos.Top,
        ASLFR_InitialLeftEdge,  gvLoadFileReq.prefs.initialpos.Left,
        ASLFR_InitialWidth, gvLoadFileReq.prefs.initialpos.Width,
        ASLFR_InitialHeight,gvLoadFileReq.prefs.initialpos.Height,
    EndObject;

    gvPaletteOpenReq.Req = FileReqObject,
        ASLFR_Screen,       MAINSCR,
        ASLFR_Locale,       globxd->locale,
        ASLFR_InitialDrawer,globals->userprefs->startupdir,
        ASLFR_InitialTopEdge,   gvPaletteOpenReq.prefs.initialpos.Top,
        ASLFR_InitialLeftEdge,  gvPaletteOpenReq.prefs.initialpos.Left,
        ASLFR_InitialWidth, gvPaletteOpenReq.prefs.initialpos.Width,
        ASLFR_InitialHeight,gvPaletteOpenReq.prefs.initialpos.Height,
    EndObject;

    gvPaletteSaveReq.Req = FileReqObject,
        ASLFR_Screen,       MAINSCR,
        ASLFR_Locale,       globxd->locale,
        ASLFR_DoSaveMode,   TRUE,
        ASLFR_InitialDrawer,globals->userprefs->startupdir,
        ASLFR_InitialTopEdge,   gvPaletteSaveReq.prefs.initialpos.Top,
        ASLFR_InitialLeftEdge,  gvPaletteSaveReq.prefs.initialpos.Left,
        ASLFR_InitialWidth, gvPaletteSaveReq.prefs.initialpos.Width,
        ASLFR_InitialHeight,gvPaletteSaveReq.prefs.initialpos.Height,
    EndObject;

    if( !gvLoadFileReq.Req || !gvPaletteOpenReq.Req || !gvPaletteSaveReq.Req ) {
        Req(NEGNUL,NULL,GetStr(mINIT_NO_ASL_REQUESTER));
        return PERR_INITFAILED;
    }

    /*
     *  Initialize help system.
     */

    if( InitHelp() != PERR_OK ) {
        D(bug("Could not open Amigaguide help\n"));
    } else {
        D(bug("Amigaguide help opened\n"));
    }

    /*
     *  Misc stuff
     */

    InitOptions(); /* BUG: Should check */

    /*
     *  The End
     */

    D(bug("Initialize() OK\n"));

    return PERR_OK;
}

/*
    This will attempt to break a frame in progress. It will not return
    until it is reasonably certain the process won't die or it has
    killed it succesfully.
*/
PERROR BreakFrame( FRAME *f )
{
    struct Task *ct;
    int i;

    if( ct = (struct Task *) f->currproc ) {
        while(1) {
            Signal( ct, SIGBREAKF_CTRL_C );
            for( i = 0; i < BREAK_HOW_MANY_TIMES; i++ ) {
                struct PPTMessage *msg;

                D(bug("\tWaiting...\n"));
                Delay(5L); /* First, a short delay */

                /*
                 *  If we got a message, then quit.
                 */

                if(msg = (struct PPTMessage *) GetMsg(globals->mport)) {
                    if( msg->msg.mn_Node.ln_Type == NT_REPLYMSG ) {
                        FreePPTMsg( msg, globxd );
                    } else {
                        if( (msg->frame == f) && (msg->code & PPTMSGF_DONE) ) {
                            D(bug("\tframe ok\n"));
                            f->currproc = NULL;
                            f->currext  = NULL;
                            ReplyMsg( (struct Message *) msg );
                            break;
                        }
                        ReplyMsg( (struct Message *) msg );
                    }
                }
                /* wait a bit more before trying a next time */

                Delay(BREAK_DELAY_PERIOD*i);
            } // for

            if(i == BREAK_HOW_MANY_TIMES) {
                if( 0 == Req(NEGNUL,GetStr(mRETRY_IGNORE),
                             GetStr(mEXIT_EXTERNAL_DIDNT_DIE),
                             f->currext->nd.ln_Name ) )
                {
                    D(bug("WARNING: External %s didn't die.\n", f->currext->nd.ln_Name ));
                    D(Close(f->debug_handle));
                    f->currproc = NULL;
                    f->currext  = NULL;
                    return PERR_GENERAL;
                }
            } else {
                // Breaking was successfull
                return PERR_OK;
            }
        }
    }
    return PERR_OK;
}

/*
    Removes allocated resources
*/
int FreeResources (GLOBALS *g)
{
    struct Node *fn, *nn;
    int i;
    PREFS *p = g->userprefs;

    /*
     *   First, make sure that nobody is using the variables any more and set
     *   up the master quit flag
     */

    BusyAllWindows(globxd);

    MasterQuit = TRUE;

    CloseStartupWindow(); /* Just in case we die during the startup */

    /* Send break to all active processes.
       BUG: Should really wait until they've told they've quit all */

    ExitHelp();

    fn = g->frames.lh_Head;
    while( nn = fn->ln_Succ ) {
        BreakFrame( (FRAME *)fn );
        fn = nn;
    }

    ObtainSemaphore( &g->phore );

    /*
     *  Close AREXX host
     */

    ExitRexx( rxhost );

    D(bug("Emptying message port\n"));
    EmptyMsgPort( g->mport, globxd );
    D(bug("Messages done\n"));

    /*
     * Free lists, nodes and allocated memory
     */

    /* Frames and possible associated displays */
    fn = g->frames.lh_Head; i=0;
    while( nn = fn->ln_Succ ) {
        DeleteFrame( (FRAME *)fn );
        fn = nn;
        i++;
    }
    D(bug("\nReleased %d frames\n",i));

    if( clipframe )  {
        D(bug("Releasing clipframe\n"));
        RemFrame( clipframe, globxd );
    }

    /*
     *  Make sure we expunge all modules on exit.
     */

    globals->userprefs->expungelibs = TRUE;

    /* Loaders... */

    fn = g->loaders.lh_Head; i = 0;
    while( nn = fn->ln_Succ ) {
        PurgeExternal( (EXTERNAL *)fn, TRUE );
        fn = nn; i++;
    }
    D(bug("\nReleased %d loaders\n",i));

    /* Filters... */
    fn = g->effects.lh_Head; i = 0;
    while( nn = fn->ln_Succ ) {
        PurgeExternal( (EXTERNAL *)fn, TRUE );
        fn = nn; i++;
    }
    D(bug("\nReleased %d effects\n",i));

    /* Scripts... */
    fn = g->scripts.lh_Head; i = 0;
    while( nn = fn->ln_Succ ) {
        PurgeExternal( (EXTERNAL *)fn, TRUE );
        fn = nn; i++;
    }
    D(bug("\nReleased %d scripts\n",i));

    /*
     *  Kill windows, screens
     */

    if( extf.Win ) DisposeObject( extf.Win );
    if( extl.Win ) DisposeObject( extl.Win );
    if( exts.Win ) DisposeObject( exts.Win );
    if( framew.Win ) DisposeObject( framew.Win );
    if( prefsw.Win ) DisposeObject( prefsw.Win );
    if( toolw.Win ) DisposeObject( toolw.Win );
    if( selectw.Win ) DisposeObject( selectw.Win );

    CloseMainScreen(); /* Cannot use CloseDisplay(), since the objects are already disposed of. */

    /*
     *  Get rid of allocated system resources
     */

    if(p->maintf) CloseFont(p->maintf);
    if(p->maintf) CloseFont(p->maintf);

    if(filereq) FreeAslRequest( filereq );
    if(fontreq) FreeAslRequest( fontreq );
    if(gvLoadFileReq.Req)    DisposeObject( gvLoadFileReq.Req );
    if(gvPaletteOpenReq.Req) DisposeObject( gvPaletteOpenReq.Req );
    if(gvPaletteSaveReq.Req) DisposeObject( gvPaletteSaveReq.Req );

    ExitOptions();

    /*
     *  BGUI classes
     */

    if(DropAreaClass) FreeDropAreaClass( DropAreaClass );
    if(RenderAreaClass) FreeRenderAreaClass( RenderAreaClass );

    /*
     *  Timer device
     */

    if( TimerBase ) CloseDevice( globxd->TimerIO );

    /*
     *  IDCMP port
     */

    if(MainIDCMPPort) {
        EmptyMsgPort(MainIDCMPPort,globxd);
        DeleteMsgPort(MainIDCMPPort);
    }

    DeleteMsgPort(g->mport); /* Has been already emptied */

    /*
     *  Free libraries (if any). Currently SAS/C autoexit code takes care of most of them.
     */

    if( CyberGfxBase ) CloseLibrary(CyberGfxBase);

    ClosepptCatalog( globxd );

    ClosePool();
    if( MyLocale ) CloseLocale( MyLocale );

    if( BGUIBase ) CloseLibrary(BGUIBase);
    if( GadToolsBase ) CloseLibrary(GadToolsBase);
    if( LocaleBase ) CloseLibrary( LocaleBase );

    if(globxd) RelExtBase(globxd);

    pfree(globals->maindisp);
    pfree(globals->userprefs);
    pfree(globals);

    D(bug("FreeResources() OK\n"));

    return PERR_OK;
} /* FreeResources */


/*
    This is a general panic-out for those situations. Mainly debug
    purposes.
*/

void Panic( const char *msg )
{
    extern BPTR debug_fh, old_fh;
    struct EasyStruct es = {
        sizeof(struct EasyStruct),0L,"PPT Panic!",
        "%s\n","Exit program"
    };

    EasyRequest(NULL, &es, NULL, msg );

    PutStr("PANIC: ");
    PutStr(msg); /* Put it into stdout as well */
    Delay(100); /* Make sure it gets out. */
    FreeResources(globals);

#ifdef DEBUG_MODE
    Flush(Output());
    SelectOutput( old_fh );
    Close(debug_fh);
#endif

    exit(20);
}

#ifdef __SASC
/*
    Provide a stack overflow checking function of our own.
    BUG: Untested.
*/
void __stdargs _CXOVF(void)
{
    struct Task *mytask;

    mytask = FindTask(NULL);

    if( mytask == (struct Task *)globals->maintask ) {
        Panic("Stack overflow detected, attempting to exit...");
    } else {
        /*
         *  This is one of dem spawned subtasks.  Currently I just
         *  hang, which means it is theoretically safe to RemTask()
         *  us.  However, none of the resources allocated will be freed.
         */

        InternalError("Stack overflow detected.  Hanging this task...");
        Wait(0L);
    }
}
#endif

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

