/*----------------------------------------------------------------------*/
/*
    PROJECT: PPT
    MODULE : gui.c

    $Id: gui.c,v 1.49 1998/01/04 16:38:07 jj Exp $

    This module contains most of the routines for GUI creation.

    BGUI and bgui.library are (C) Jan van den Baard, 1994-1996.
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "version.h"
#include "gui.h"
#include "misc.h"

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif


#include <pragmas/bgui_pragmas.h>

#ifndef PRAGMAS_INTUITION_PRAGMAS_H
#include <pragmas/intuition_pragmas.h>
#endif

#ifndef INTUITION_ICCLASS_H
#include <intuition/icclass.h>
#endif

#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <pragmas/utility_pragmas.h>

#ifndef PRAGMAS_DOS_PRAGMAS_H
#include <pragmas/dos_pragmas.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif

#ifndef LIBRARIES_ASL_H
#include <libraries/asl.h>
#endif

#include <cybergraphics/cybergraphics.h>

#include "libraries/ExecutiveAPI.h"

/*----------------------------------------------------------------------*/
/* Defines */

#define DEFAULT_TOOLBAR_WIDTH   200

/*----------------------------------------------------------------------*/
/* Prototypes */

Prototype PERROR        GimmeToolBarWindow( VOID );
Prototype PERROR        GimmePaletteWindow( FRAME *, ULONG );
Prototype Object *      GimmeDispPrefsWindow( DISPLAY *, struct DispPrefsWindow * );
Prototype struct Window *GimmeFilterWindow( EXTBASE *, FRAME *, ULONG *, struct EffectWindow * );
Prototype Object *      GimmePrefsWindow( VOID );
Prototype Object *      GimmeMainWindow( VOID );
Prototype Object *      GimmeDispPrefsWindow( DISPLAY *, struct DispPrefsWindow * );
Prototype Object *      GimmeSaveWindow( FRAME *, EXTBASE *, struct SaveWin * );
Prototype Object *      MyNewObject( EXTBASE *, ULONG, Tag, ... );

/*----------------------------------------------------------------------*/
/* Global variables */

/// PPTMenus[]

/* The Global Menu. This is duplicated for each of the windows to be
   created.

   The used letters for different shortcuts are:
   A - Select All                       T - ToolBar
   B - Break                            U -
   C - Copy                             V - Paste
   D - Close                            W - Save
   E - Effects Window                   X - Cut
   F - Frames Window                    Y -
   G -                                  Z - Undo
   H - Hide/Show                        + - Zoom in
   I - SelectWindow                     - - Zoom out
   J -                                  0 -
   K -                                  1 -
   L - Loaders Window                   2 -
   M -                                  3 -
   N -                                  4 -
   O - Open                             5 -
   P - Process                          6 -
   Q - Quit                             7 -
   R - Render                           8 -
   S - Scripts Window                   9 -

*/

struct NewMenu PPTMenus[] = {
    Title( "Project "),
        Item( "Open...",        "O", MID_LOADNEW ),
        Item("Open As...",      NULL, MID_LOADAS ),
        DItem("Save",           "W", MID_SAVE ),
        DItem( "Save As...",     NULL, MID_SAVEAS ),
        DItem( "Close",         "D", MID_DELETE ),
        ItemBar,
        DItem( "Rename...",     NULL, MID_RENAME ),
        DItem( "Hide/Show",     "H", MID_HIDE ),
        Item( "Preferences...", NULL, MID_PREFS ),
        ItemBar,
        Item( "About PPT",      NULL, MID_ABOUT ),
        ItemBar,
        Item( "Quit",           "Q",  MID_QUIT ),
    Title( "Edit" ),
        DItem( "Undo",          "Z", MID_UNDO ),
        DItem( "Cut",           "X", MID_CUT ),
        DItem( "Cut To New",    NULL, MID_CUTFRAME ),
        DItem( "Copy",          "C", MID_COPY ),
        DItem( "Paste",         "V", MID_PASTE ),
#ifdef DEBUG_MODE
        DItem( "Crop",          NULL, MID_CROP ),
#endif
        ItemBar,
        DItem("Zoom In",        "+", MID_ZOOMIN ),
        DItem("Zoom Out",       "-", MID_ZOOMOUT ),
        ItemBar,
        DItem( "Select All",    "A", MID_SELECTALL ),
        DItem( "Edit Info...",  NULL, MID_EDITINFO ),
    Title( "Display" ),
        DItem( "Settings...",   NULL, MID_SETRENDER ),
        DItem( "Render",        "R", MID_RENDER ),
        DItem( "Close Render",  NULL,MID_CLOSERENDER ),
        ItemBar,
        DItem( "Correct Aspect","J", MID_CORRECTASPECT ),
        DItem( "Palette",       NULL, MID_PALETTE ),
            SubItem( "Edit...", NULL, MID_EDITPALETTE ),
            SubItem( "Load...", NULL, MID_LOADPALETTE ),
            SubItem( "Save...", NULL, MID_SAVEPALETTE ),
    Title( "Process" ),
        DItem( "Process...",    "P", MID_PROCESS ),
        DItem( "Break",         "B", MID_BREAK ),
        DItem( "Remove Alpha",  NULL, MID_REMOVEALPHA ),
#ifdef DEBUG_MODE
    Title( "Debug" ),
        Item( "SavPalette", NULL, MID_SAVEMAINPALETTE ),
        Item( "Memory Check",   NULL, MID_MEMCHECK ),
        Item( "Test AskReq()",  NULL, MID_TESTAR ),
        Item( "Memory Fail Rate", NULL, MID_MEMFAIL ),
            { NM_SUB, " 0 %", NULL, CHECKIT|MENUTOGGLE|CHECKED, ~1, (APTR)MID_MEMFAIL0 },
            { NM_SUB, "10 %", NULL, CHECKIT|MENUTOGGLE, ~2, (APTR)MID_MEMFAIL10 },
            { NM_SUB, "25 %", NULL, CHECKIT|MENUTOGGLE, ~4, (APTR)MID_MEMFAIL25 },
            { NM_SUB, "50 %", NULL, CHECKIT|MENUTOGGLE, ~8, (APTR)MID_MEMFAIL50 },
#endif
    Title( "Window" ),
        { NM_ITEM, "Frames",    "F", CHECKIT|MENUTOGGLE, 0L, (APTR)MID_FRAMEWINDOW },
        { NM_ITEM, "Toolbar",   "T", CHECKIT|MENUTOGGLE, 0L, (APTR)MID_TOOLWINDOW },
        { NM_ITEM, "Select",    "I", CHECKIT|MENUTOGGLE, 0L, (APTR)MID_SELECTWINDOW },
        { NM_ITEM, "Effects",   "E", CHECKIT|MENUTOGGLE, 0L, (APTR)MID_EFFECTS },
        { NM_ITEM, "Loaders",   "L", CHECKIT|MENUTOGGLE, 0L, (APTR)MID_LOADERS },
        { NM_ITEM, "Scripts",   "S", CHECKIT|MENUTOGGLE, 0L, (APTR)MID_REXXWINDOW },
    End
};
///

/* Here are some mapping codes for different type objects */
const ULONG dpcol_sl2ind[] = { SLIDER_Level, INDIC_Level, TAG_END };
const ULONG dpcol_ind2sl[] = { INDIC_Level, SLIDER_Level, TAG_END };

const ULONG dpcol_sl2int[] = { SLIDER_Level, STRINGA_LongVal, TAG_END };
const ULONG dpcol_int2sl[] = { STRINGA_LongVal, SLIDER_Level, TAG_END };

const ULONG dpcol_sl2fl[] = { SLIDER_Level, FLOAT_LongValue, TAG_END };
const ULONG dpcol_fl2sl[] = { FLOAT_LongValue, SLIDER_Level, TAG_END };


/// Miscallaneous labels

/* Color cycle labels. */
const char *color_labels[] = {
    "Normal Color",
    "EHB",
    "HAM",
    "HAM8",
    NULL,
};

const char *palette_labels[] = {
    "Median Cut",
    "Popularity",
    "Force -->",
    NULL,
};

const char *dither_labels[] = {
    "Off",
    "Floyd-Steinberg",
    NULL,
};

const char *scr_labels[] = {
    "Amiga",
    NULL,
};

const char *save_mode_labels[] = {
    "Truecolor",
    "Colormapped",
    NULL,
};

/* This is the screentitle */
const char *std_ppt_blurb = "PPT "RELEASE" (v"VERSION").   "COPYRIGHT;

char workbuf[MAXPATHLEN+1],undobuf[MAXPATHLEN+1];
///

/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Code */

/*
    This is a replacement stub for BGUI_NewObject.
*/

/// "MyNewObject()"
Object *MyNewObject( EXTBASE *xd, ULONG classid, Tag tag1, ... )
{
    struct Library *BGUIBase = xd->lb_BGUI;

    return(BGUI_NewObjectA( classid, (struct TagItem *) &tag1 ));
}
///


/// GimmePaletteWindow()
/*
    Open a palette editor window on a separate custom screen.
    BUG: Shouldn't a display be enough?
*/

PERROR GimmePaletteWindow( FRAME *frame, ULONG depth )
{
    PERROR res = PERR_OK;
    struct PaletteWindow *pw = frame->pw;

    D(bug("GimmePaletteWindow( %08X, %08X )\n",frame,pw ));

#if 0
    pw->ColorWheel = BGUI_NewObject( BGUI_AREA_GADGET,
                                    AREA_MinWidth, 40, AREA_MinHeight, 40,
                                    GA_ID, GID_PAL_COLORWHEEL,
                                    ButtonFrame,
                                    TAG_END );


    if(!pw->ColorWheel) {
        D(bug("\tColorwheel alloc failed\n"));
        DisposeObject( pw->Palette );
        pw->Palette = NULL;
        return PERR_WINDOWOPEN;
    }
#endif

    pw->Win = WindowObject,
        WINDOW_Screen,       MAINSCR,
        WINDOW_SharedPort,   MainIDCMPPort,
        WINDOW_ScreenTitle,  std_ppt_blurb,
        WINDOW_Title,        frame->nd.ln_Name,
        WINDOW_ScaleWidth,   50,
        WINDOW_ScaleHeight,  20,
        WINDOW_Font,         globals->userprefs->mainfont,
        WINDOW_MasterGroup,
            VGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                StartMember,
                    HGroupObject, Spacing(4),
#if 0
                        StartMember,
                            /* The colorwheel */
                            pw->ColorWheel,
                        EndMember,
                        StartMember,
                            /* The gradient slider */
                            pw->GradientSlider = SliderObject, GA_ID, GID_PAL_GRADIENTSLIDER,
                                SLIDER_Min, 0,
                                SLIDER_Max, 255,
                                SLIDER_Level, 0,
                                PGA_Freedom, FREEVERT,
                            EndObject, FixMinWidth,
                        EndMember,
#endif

                        StartMember,
                            VGroupObject, NormalSpacing,
                                StartMember,
                                    pw->Palette = BGUI_NewObject( BGUI_PALETTE_GADGET,
                                            GA_ID, GID_PAL_PALETTE,
                                            FRM_Type, FRTYPE_BUTTON,
                                            FRM_Recessed, TRUE,
                                            PALETTE_Depth, depth,
                                            BT_DragObject, TRUE,
                                    EndObject, Weight(1000),
                                EndMember,
                                StartMember,
                                    HGroupObject, NormalSpacing,
                                        StartMember,
                                            pw->Slider1 = SliderObject, GA_ID, GID_PAL_SLIDER1,
                                                LAB_Label, "R",
                                                SLIDER_Min, 0,
                                                SLIDER_Max, 255,
                                                SLIDER_Level, 0,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            pw->Indic1 = IndicatorObject,
                                                INDIC_Min, 0, INDIC_Max, 255,
                                                INDIC_Justification, IDJ_RIGHT,
                                                INDIC_FormatString, "%3ld",
                                            EndObject, FixMinWidth,
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject, NormalSpacing,
                                        StartMember,
                                            pw->Slider2 = SliderObject, GA_ID, GID_PAL_SLIDER2,
                                                LAB_Label, "G",
                                                SLIDER_Min, 0,
                                                SLIDER_Max, 255,
                                                SLIDER_Level, 0,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            pw->Indic2 = IndicatorObject,
                                                INDIC_Min, 0, INDIC_Max, 255,
                                                INDIC_Justification, IDJ_RIGHT,
                                                INDIC_FormatString, "%3ld",
                                            EndObject, FixMinWidth,
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject, NormalSpacing,
                                        StartMember,
                                            pw->Slider3 = SliderObject, GA_ID, GID_PAL_SLIDER3,
                                                LAB_Label, "B",
                                                SLIDER_Min, 0,
                                                SLIDER_Max, 255,
                                                SLIDER_Level, 0,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            pw->Indic3 = IndicatorObject,
                                                INDIC_Min, 0, INDIC_Max, 255,
                                                INDIC_Justification, IDJ_RIGHT,
                                                INDIC_FormatString, "%3ld",
                                            EndObject, FixMinWidth,
                                        EndMember,
                                    EndObject,
                                EndMember,
                                StartMember,
                                    HGroupObject, NormalSpacing,
                                        StartMember,
                                            pw->Slider4 = SliderObject, GA_ID, GID_PAL_SLIDER4,
                                                LAB_Label, "A",
                                                SLIDER_Min, 0,
                                                SLIDER_Max, 255,
                                                SLIDER_Level, 0,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            pw->Indic4 = IndicatorObject,
                                                INDIC_Min, 0, INDIC_Max, 255,
                                                INDIC_Justification, IDJ_RIGHT,
                                                INDIC_FormatString, "%3ld",
                                            EndObject, FixMinWidth,
                                        EndMember,
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
                    HGroupObject, WideSpacing,
                        StartMember,
                            pw->Ok = GenericButton( GetStr(MSG_OK_GAD), GID_PAL_OK ),
                        EndMember,
                        StartMember,
                            pw->Remap = GenericButton( "Remap", GID_PAL_REMAP ),
                        EndMember,
                        StartMember,
                            pw->Cancel = GenericButton( GetStr(MSG_CANCEL_GAD), GID_PAL_OK ),
                        EndMember,
                    EndObject, Weight(1),
                EndMember,
            EndObject,
    EndObject;

    if(!pw->Win) {
        res = PERR_WINDOWOPEN;
    } else {
        AddMap( pw->Slider1, pw->Indic1, dpcol_sl2ind );
        AddMap( pw->Slider2, pw->Indic2, dpcol_sl2ind );
        AddMap( pw->Slider3, pw->Indic3, dpcol_sl2ind );
        AddMap( pw->Slider4, pw->Indic4, dpcol_sl2ind );
    }
    return res;
}
///

/// "GimmeSaveWindow()"
/*
    Saving window. This can be open for several frames at the same time.
*/

const struct NewMenu SaveMenus[] = {
    Title("Save"),
        Item("Save", NULL, GID_SW_SAVE),
        Item("Get Path...", NULL, GID_SW_GETFILE),
        Item("Cancel", NULL, GID_SW_CANCEL),
    End
};


SAVEDS Object *
GimmeSaveWindow( FRAME *frame, EXTBASE *ExtBase, struct SaveWin *gads )
{
    EXTBASE *xd = ExtBase; /* BUG */
    struct IntuitionBase *IntuitionBase = ExtBase->lb_Intuition;
    struct DosLibrary *DOSBase = ExtBase->lb_DOS;
    char buf[MAXPATHLEN+1], initialfile[NAMELEN+1];

    D(bug("GimmeSaveWindow()\n"));

    strncpy( initialfile, frame->name, NAMELEN );
    DeleteNameCount( initialfile );
    strcpy(buf, frame->path);
    AddPart(buf, initialfile, MAXPATHLEN);

    gads->Frq = MyFileReqObject,
        ASLFR_InitialDrawer, frame->path,
        ASLFR_InitialFile,   initialfile,
        ASLFR_DoSaveMode, TRUE,
        ASLFR_Screen, MAINSCR,
    EndObject;

    if( gads->Frq == NULL) return NULL;

    gads->Win = MyWindowObject,
        WINDOW_Screen,      MAINSCR,
        WINDOW_ScreenTitle, std_ppt_blurb,
        WINDOW_ScaleWidth,  10,
        WINDOW_ScaleHeight, 10,
        WINDOW_Title,       XGetStr(MSG_SAVEAS_WINDOW),
        WINDOW_Font,        globals->userprefs->mainfont,
        WINDOW_MenuStrip,   SaveMenus,
        WINDOW_MasterGroup,
            MyVGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                StartMember,
                    MyHGroupObject, NarrowSpacing,
                        StartMember,
                            gads->Name = MyStringObject, GA_ID, GID_SW_FILE,
                                StringFrame,
                                STRINGA_TextVal, buf,
                                STRINGA_MaxChars, MAXPATHLEN-1,
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/SaveAs",
                            EndObject,
                        EndMember,
                        StartMember,
                            MyButtonObject, GA_ID, GID_SW_GETFILE,
                                ButtonFrame, GetFile,
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/SaveAs",
                            EndObject, FixMinWidth,
                        EndMember,
                    EndObject, Weight(2),
                EndMember,
                StartMember,
                    MySeparatorObject,SEP_Horiz, TRUE, EndObject, FixMinHeight,
                EndMember,
                StartMember,
                    MyHGroupObject, NormalSpacing,
                        StartMember,
                            /* Format */
                            gads->Format = MyListviewObject,
                                LAB_Label, XGetStr(MSG_PICK_SAVE_FORMAT_LAB),
                                LAB_Place, PLACE_ABOVE,
                                GA_ID, GID_SW_SAVERS,
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/SaveAs",
                            EndObject,
                        EndMember,
                        StartMember,
                            MyVGroupObject,
                                VarSpace(50),
                                StartMember,
                                    gads->Mode = MyMxObject, GA_ID, GID_SW_MODE,
                                        GROUP_Style, GRSTYLE_VERTICAL,
                                        LAB_Label, XGetStr(MSG_PICK_SAVE_MODE_LAB),
                                        LAB_Place, PLACE_ABOVE,
                                        MX_Labels, save_mode_labels,
                                        BT_HelpHook, &HelpHook,
                                        BT_HelpNode, "PPT.guide/SaveAs",
                                        MX_Active, 0,
                                    EndObject,
                                EndMember,
                                VarSpace(50),
                            EndObject, Weight(25),
                        EndMember,
                    EndObject,
                EndMember,
                StartMember,
                    /* Save & cancel gadgets */
                    MyHGroupObject, NormalSpacing,
                        StartMember,
                            MyButtonObject, ULabel(XGetStr(MSG_SAVE_GAD) ),
                                GA_ID, GID_SW_SAVE,
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/SaveAs",
                            EndObject,
                        EndMember,
                        StartMember,
                            MyButtonObject, ULabel(XGetStr(MSG_CANCEL_GAD) ),
                                GA_ID, GID_SW_CANCEL,
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/SaveAs",
                            EndObject,
                        EndMember,
                    EndObject, Weight(10),
                EndMember,
            EndObject,
        EndObject;

    if(gads->Win) {

        if( NULL == (gads->win = WindowOpen( gads->Win ))) {
            DisposeObject(gads->Win);
            gads->Win = NULL;
            DisposeObject(gads->Frq);
        } else {
            XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, MX_DisableButton, 0, TAG_END );
            XSetGadgetAttrs( xd, (struct Gadget *)gads->Mode, gads->win, NULL, MX_DisableButton, 1, TAG_END );
            AddExtEntries( xd, gads->win, gads->Format, NT_LOADER, AEE_SAVE );

            /*
             *  Set the listview to point at the correct loader.
             */

            if(frame->origtype) {
                ULONG which = 0;
                APTR entry;
                LOADER *ld;

                entry = (APTR)FirstEntry( gads->Format );
                while( entry && (strcmp(entry, (ld = frame->origtype)->info.nd.ln_Name) != 0) ) {
                    entry = (APTR)NextEntry( gads->Format, entry ); which++;
                }
                if(entry) {
                    /* Set MX gadgets correctly */
                    extern void DoSaveMXGadgets( FRAME *, struct SaveWin *, LOADER *, EXTBASE * );

                    XSetGadgetAttrs( xd, (struct Gadget *)gads->Format, gads->win, NULL, LISTV_Select, which, TAG_END );
                    DoSaveMXGadgets( frame, gads, ld, xd );
                }
            }

            ScreenToFront(MAINSCR);

        }
    }
    return gads->Win;
}
///

/// GimmeDispPrefsWindow()

/*
    Display Preferences window
*/


Object *
GimmeDispPrefsWindow( DISPLAY *disp, struct DispPrefsWindow *dpw )
{
    FRAME *frame = (FRAME *)(dpw->frame);
    ULONG args[5];
    ULONG type, dither, ncolors;
    UBYTE buffer[NAMELEN+1];

    D(bug("GimmeDispPrefsWindow( %08X, %08X )\n",disp,dpw));
    D(bug("\tFrame = %08X\n",frame));

    GetNameForDisplayID( disp->dispid, buffer, NAMELEN );

    args[0] = disp->width;
    args[1] = disp->height;
    type    = disp->renderq;
    dither  = disp->dither;
    ncolors = disp->ncolors;

    if( dpw->WO_Win == NULL ) {
        dpw->WO_Win = WindowObject,
            WINDOW_Title, frame ? frame->nd.ln_Name : "Main Screen",
            WINDOW_ScreenTitle, std_ppt_blurb,
            WINDOW_Font,globals->userprefs->mainfont,
            WINDOW_Screen, MAINSCR,
            WINDOW_ScaleWidth, 25,
            WINDOW_SharedPort, MainIDCMPPort,
            WINDOW_MenuStrip, PPTMenus,
            WINDOW_MasterGroup,
                VGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                    StartMember,
                        InfoObject, INFO_TextFormat, GetStr(MSG_DISPLAY_PREFERENCES_LAB),
                        EndObject,
                    EndMember,
                    StartMember,
                        HGroupObject, NormalSpacing,
                            StartMember,
                                dpw->GO_ScrMode = InfoObject,
                                    LAB_Label, GetStr(MSG_SCREENMODE_LAB),
                                    GA_ID, GID_DP_SCRMODE,
                                    ButtonFrame, FRM_Flags, FRF_RECESSED,
                                    INFO_TextFormat, buffer,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject, Weight( 200 ),
                            EndMember,
                            StartMember,
                                dpw->GO_GetScrMode = ButtonObject,
                                    GA_ID, GID_DP_GETSCRMODE,
                                    PopUp,
                                    ButtonFrame,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject, FixMinWidth,
                            EndMember,
#ifdef DEBUG_MODE
                            StartMember,
                                dpw->GO_Screen = CycleObject, GA_ID, GID_DP_SCREEN,
                                    CYC_Labels, scr_labels,
                                    CYC_Active, 0,
                                    GA_Disabled, TRUE,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject,
                            EndMember,
#endif
                        EndObject,
                    EndMember,
#if 0
                    StartMember,
                        dpw->GO_Info = InfoObject, GA_ID, GID_DP_INFO,
                            INFO_TextFormat,ISEQ_C"%ld x %ld",
                            INFO_Args, &args[0],
                        EndObject,
                    EndMember,
#endif

                    StartMember,
                        HGroupObject, NarrowSpacing,
                            StartMember,
                                dpw->GO_Type = CycleObject, GA_ID, GID_DP_TYPE,
                                    ULabel("Color Mode:"),
                                    CYC_Labels, color_labels,
                                    CYC_Active, type,
                                    CYC_Popup,  TRUE,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject,
                            EndMember,
                        EndObject,
                    EndMember,

                    StartMember,
                        HGroupObject, NarrowSpacing,
                            StartMember,
                                dpw->GO_NColors = SliderObject,
                                    ULabel( GetStr(MSG_COLORS_LAB) ),
                                    GA_ID, GID_DP_NCOLORS,
                                    SLIDER_Min, 2, SLIDER_Max, 256,
                                    SLIDER_Level, ncolors,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject,
                            EndMember,
                            StartMember,
                                dpw->GO_NColorsI = IndicatorObject,
                                    INDIC_Min, 2, INDIC_Max, 256,
                                    INDIC_Level, ncolors,
                                    INDIC_FormatString, "%5ld",
                                    INDIC_Justification, IDJ_LEFT,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject, FixMinWidth,
                            EndMember,
                            StartMember,
                                dpw->GO_ForceBW = CheckBoxObject, GA_ID, GID_DP_FORCEBW,
                                    ULabel(GetStr(MSG_FORCE_BW_LAB)), Place( PLACE_LEFT ),
                                    ButtonFrame,
                                    GA_Selected, FALSE,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject, FixMinWidth,
                            EndMember,
                        EndObject,
                    EndMember,

                    StartMember,
                        HGroupObject, NarrowSpacing,
                            StartMember,
                                dpw->GO_Dither = CycleObject,
                                    ULabel( GetStr(MSG_DITHER_LAB) ),
                                    GA_ID, GID_DP_DITHER,
                                    CYC_Labels, dither_labels, CYC_Active, dither,
                                    CYC_Popup, TRUE,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject,
                            EndMember,
                        EndObject,
                    EndMember,

                    StartMember,
                        HGroupObject, NarrowSpacing,
                            StartMember,
                                dpw->GO_Palette = CycleObject, GA_ID, GID_DP_PALETTE,
                                    CYC_Labels, palette_labels, CYC_Active, 0,
                                    ULabel(GetStr(MSG_PALETTE_LAB)),
                                    CYC_Popup, TRUE,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject, FixMinWidth,
                            EndMember,
                            StartMember,
                                dpw->GO_PaletteName = StringObject, GA_ID, GID_DP_PALETTENAME,
                                    StringFrame,
                                    STRINGA_MaxChars, 256,
                                    GA_Disabled, TRUE,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject,
                            EndMember,
                            StartMember,
                                dpw->GO_GetPaletteName = ButtonObject, GA_ID, GID_DP_GETPALETTENAME,
                                    GetFile,
                                    ButtonFrame,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/RenderSettings",
                                    GA_Disabled, TRUE,
                                EndObject, FixMinWidth,
                            EndMember,
                        EndObject,
                    EndMember,

                    StartMember,
                        dpw->GO_DrawAlpha = CheckBoxObject, GA_ID, GID_DP_FORCEBW,
                             ULabel("Draw Alpha?"), Place( PLACE_LEFT ),
                             ButtonFrame,
                             GA_Selected, FALSE,
                             BT_HelpHook, &HelpHook,
                             BT_HelpNode, "PPT.guide/RenderSettings",
                        EndObject, FixMinWidth,
                    EndMember,

                    StartMember,HorizSeparator,EndMember,

                    StartMember,
                        HGroupObject,NormalSpacing,
                            StartMember,
                                dpw->GO_OK = GenericButton(GetStr(MSG_OK_GAD),GID_DP_OK),
                            EndMember,
                            StartMember,
                                dpw->GO_OKRender = GenericButton( "OK & Render", GID_DP_OKRENDER ),
                            EndMember,
                            StartMember,
                                dpw->GO_Cancel = GenericButton( GetStr(MSG_CANCEL_GAD),GID_DP_CANCEL),
                            EndMember,
                        EndObject, Weight(1),
                    EndMember,
                EndObject, /* Master VGroup */
            EndObject; /* Window */
    }

    D(bug("\tGot object\n"));

    if( dpw->WO_Win ) {
#if 0
        AddCondit( dpw->GO_Type, dpw->GO_NColors, CYC_Active, 0, GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( dpw->GO_Type, dpw->GO_NColorsI, CYC_Active, 0, GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( dpw->GO_Screen, dpw->GO_GetScrMode, CYC_Active, 1, GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( dpw->GO_Screen, dpw->GO_Type, CYC_Active, 1, GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( dpw->GO_Screen, dpw->GO_NColors, CYC_Active, 1, GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( dpw->GO_Screen, dpw->GO_Palette, CYC_Active, 1, GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( dpw->GO_Screen, dpw->GO_Dither, CYC_Active, 1, GA_Disabled, FALSE, GA_Disabled, TRUE );
#endif
        AddMap( dpw->GO_NColors, dpw->GO_NColorsI, dpcol_sl2ind );
        AddMap( dpw->GO_NColorsI, dpw->GO_NColors, dpcol_ind2sl );
        AddCondit( dpw->GO_Palette, dpw->GO_PaletteName, CYC_Active, 2, GA_Disabled, FALSE, GA_Disabled, TRUE );
        AddCondit( dpw->GO_Palette, dpw->GO_GetPaletteName, CYC_Active, 2, GA_Disabled, FALSE, GA_Disabled, TRUE );

        if( dpw->win = WindowOpen( dpw->WO_Win ) ) {
            ScreenToFront( MAINSCR );
        }
        else {
            DisposeObject( dpw->WO_Win );
            dpw->WO_Win = NULL;
        }
    }

    return dpw->WO_Win;
}
///

/// "GimmeExtInfoWindow()"
/*
    External info window
*/

Prototype Object *GimmeExtInfoWindow( const char *, const char *, struct ExtInfoWin * );

Object *
GimmeExtInfoWindow( const char *title, const char *ok, struct ExtInfoWin *ei )
{
    Object *obj;

    obj = WindowObject,
        WINDOW_Title, title,
        WINDOW_ScreenTitle, std_ppt_blurb,
        WINDOW_Font,globals->userprefs->mainfont,
        WINDOW_Screen, MAINSCR,
        WINDOW_ScaleWidth, 10,
        WINDOW_ScaleHeight,25,
        WINDOW_Position, POS_CENTERSCREEN,
        WINDOW_SharedPort, MainIDCMPPort,
        WINDOW_MenuStrip, PPTMenus,
        WINDOW_MasterGroup,
            VGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                StartMember,
                    ei->List = ListviewObject,
                        GA_ID, GID_XI_LIST,
                        LISTV_ListFont, globals->userprefs->listfont,
                        BT_HelpHook, &HelpHook,
                        BT_HelpNode, "PPT.guide/Windows",
                        BT_DragObject, FALSE,
                        BT_DropObject, FALSE,
                    EndObject, Weight( 1000 ),
                EndMember,
#if 0
                StartMember,
                    GenericDButton( "Configure", GID_XI_CONFIG ),
                EndMember,
#endif
                StartMember,
                    HGroupObject, NormalSpacing,
                        StartMember,
                            ei->Info = GenericButton( "Info", GID_XI_INFO ),
                        EndMember,
                        StartMember,
                            ei->Exec = GenericButton( ok, GID_XI_EXEC ),
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
            EndObject,
        EndObject;

    return obj;
}
///

/// GimmeFilterWindow()

/*
    Filter control window. Note that this may be opened several times.

    Please note that this is running in it's own task, so shared ports are out
    of the question.
*/

struct Window *
GimmeFilterWindow( EXTBASE *xd, FRAME *frame, ULONG *sigmask, struct EffectWindow *fw )
{
    EXTBASE *ExtBase = xd; /* BUG */
    struct Window *win = NULL, *twin = NULL;
    struct IBox pos;
    struct IntuitionBase *IntuitionBase = xd->lb_Intuition;

    /*
     *  Open up the window so that it is centered on the preferred window
     */

    twin = GetFrameWin( frame );
    pos = *( (struct IBox *) &(twin->LeftEdge) );

    fw->WO_Win = MyWindowObject,
        WINDOW_Title,       frame->nd.ln_Name,
        WINDOW_ScreenTitle, std_ppt_blurb,
        WINDOW_Font,        globals->userprefs->mainfont,
        WINDOW_Screen,      MAINSCR,
        WINDOW_ScaleWidth,  10,
        WINDOW_ScaleHeight, 25,
        WINDOW_PosRelBox,   &pos,
        WINDOW_MasterGroup,
            MyVGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                StartMember,
                    fw->GO_List = MyListviewObject, GA_ID, GID_FW_LIST,
                        ULabel(XGetStr(MSG_EFFECTS_LIST_LAB)), Place(PLACE_ABOVE),
                        LISTV_ListFont, globals->userprefs->listfont,
                        BT_HelpHook, &HelpHook,
                        BT_HelpNode, "PPT.guide/Process",
                    EndObject, Weight(1000),
                EndMember,
                StartMember,
                    fw->GO_Exec = MyButtonObject, GA_ID, GID_FW_EXEC,
                        ULabel(XGetStr(MSG_EXECUTE_GAD)),
                        GA_Disabled, TRUE,
                        BT_HelpHook, &HelpHook,
                        BT_HelpNode, "PPT.guide/Process",
                    EndObject, FixMinHeight,
                EndMember,
                StartMember,
                    MyHGroupObject, NarrowSpacing,
                        StartMember,
                            fw->GO_Info = MyButtonObject, GA_ID, GID_FW_INFO,
                                ULabel(XGetStr(MSG_INFO_GAD)),
                                GA_Disabled, TRUE,
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/Process",
                            EndObject, FixMinHeight,
                        EndMember,
                        StartMember,
                            fw->GO_Cancel = MyButtonObject, GA_ID, GID_FW_CANCEL,
                                ULabel(XGetStr(MSG_CANCEL_GAD)),
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/Process",
                            EndObject, FixMinHeight,
                        EndMember,
                    EndObject, FixMinHeight,
                EndMember,
            EndObject,
        EndObject;

    if( fw->WO_Win ) {
        if( win = WindowOpen( fw->WO_Win ) ) {
            GetAttr( WINDOW_SigMask, fw->WO_Win, sigmask );
            ScreenToFront( MAINSCR ); /* BUG: should use window screen */
        }
    }
    else
        DisposeObject(fw->WO_Win);

    fw->win = win;
    return win;
}
///

/// GimmePrefsWindow()

Local ULONG Cyc2Page[] = { MX_Active, PAGE_Active, TAG_END };
Local UBYTE *Prefspages[] = { "S_ystem","_GUI", "M_isc", NULL };
Local UBYTE *onopen_labels[] = { "do not show the image", "show smallest",
                                 "show as 25% of screen size",
                                 "show as 50% of screen size",
                                 "show largest possible", NULL };
Local UBYTE *preview_sizes[] = {"Off", "Small", "Medium", "Large", NULL };

/*
    Open the preferences window.

    Gadget shortcut keys reserved are:

    C - Color Preview
    E - External Stack
    F - Flush modules
    G - GUI
    I - Misc
    L - List Font
    M - Main Font
    P - Page Size
    S - Screen Mode
    U - Max Undo Levels
    V - VM Settings
    Y - System

 */

Prototype VOID CheckColorPreview( DISPLAY *disp );

VOID CheckColorPreview( DISPLAY *disp )
{
    ULONG disable, check, depth = disp->depth;

    if( CyberGfxBase ) {
        if( IsCyberModeID( disp->dispid ) ) {
            depth = GetCyberIDAttr( CYBRIDATTR_DEPTH, disp->dispid );
        }
    }

    if( depth < 5 ) {
        disable = TRUE;
        check   = FALSE;
    } else if( depth > 8 ) {
        disable = TRUE;
        check   = TRUE;
    } else {
        disable = FALSE;
        GetAttr( GA_Selected, prefsw.ColorPreview, &check );
    }

    SetGadgetAttrs( GAD(prefsw.ColorPreview), prefsw.win, NULL,
                    GA_Disabled, disable, GA_Selected, check, TAG_DONE );
}


Object *GimmePrefsWindow( VOID )
{
    ULONG args[7];
    Object *c, *p;
    UBYTE buffer[NAMELEN];

    args[4] = (ULONG) globals->userprefs->vmdir;
    args[5] = (ULONG) globals->userprefs->vmbufsiz;
    args[6] = (ULONG) globals->userprefs->maxundo;

    GetNameForDisplayID( globals->maindisp->dispid, buffer, NAMELEN-1 );

    if(!prefsw.Win) {
        prefsw.Win = WindowObject,
            WINDOW_Title,       "Main Preferences",
            WINDOW_ScreenTitle, std_ppt_blurb,
            WINDOW_ScaleWidth,  25,
            WINDOW_LockHeight,  TRUE,
            WINDOW_Font,        globals->userprefs->mainfont,
            WINDOW_Screen,      MAINSCR,
            WINDOW_SharedPort,  MainIDCMPPort,
            WINDOW_MenuStrip,   PPTMenus,
            WINDOW_AutoKeyLabel,TRUE,
            WINDOW_MasterGroup,
                VGroupObject, NormalHOffset, NormalVOffset, NarrowSpacing,
                    StartMember,
                        c = Tabs( NULL, Prefspages, 0, 0 ),
                    EndMember,
                    StartMember,
                        p = PageObject,
                            /*
                             *  VM page.  It contains
                             *  a) VM directory
                             *  b) VM buffer size
                             */
                                PageMember,
                                    VGroupObject, NarrowSpacing,
                                        VarSpace( 50 ),
                                        StartMember,
                                            VGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                                                FrameTitle("Virtual Memory"),
                                                NeXTFrame,
                                                StartMember,
                                                    HGroupObject, NarrowSpacing,
                                                        StartMember,
                                                            prefsw.VMDir = StringObject,
                                                                ULabel( "_VM directory" ),
                                                                GA_ID, GID_PW_VMDIR,
                                                                RidgeFrame,
                                                                STRINGA_TextVal, globals->userprefs->vmdir,
                                                                STRINGA_UndoBuffer, undobuf,
                                                                STRINGA_WorkBuffer, workbuf,
                                                                STRINGA_MaxChars, MAXPATHLEN-30,/* AmigaDOS file name length */
                                                                BT_HelpHook, &HelpHook,
                                                                BT_HelpNode, "PPT.guide/VMSettings",
                                                            EndObject,
                                                        EndMember,
                                                        StartMember,
                                                            ButtonObject,
                                                                GA_ID, GID_PW_GETVMDIR,
                                                                ButtonFrame,
                                                                GetPath,
                                                                BT_HelpHook, &HelpHook,
                                                                BT_HelpNode, "PPT.guide/VMSettings",
                                                            EndObject, FixMinWidth,
                                                        EndMember,
                                                    EndObject, FixMinHeight,
                                                EndMember,
                                                StartMember,
                                                    HGroupObject, NarrowSpacing,
                                                        StartMember,
                                                            prefsw.PageSize = StringObject,
                                                                ULabel("_Page size (kB):"), GA_ID, GID_PW_VMBUFSIZE,
                                                                STRINGA_LongVal, args[5],
                                                                STRINGA_MaxChars, 5,
                                                                STRINGA_IntegerMin, MIN_VMBUFSIZ,
                                                                STRINGA_MinCharsVisible, 5,
                                                                BT_HelpHook, &HelpHook,
                                                                BT_HelpNode, "PPT.guide/VMSettings",
                                                                RidgeFrame,
                                                            EndObject, FixMinWidth,
                                                        EndMember,
                                                    EndObject,
                                                EndMember,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            VGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                                                FrameTitle("Sub-tasks"),
                                                NeXTFrame,

                                                StartMember,
                                                    HGroupObject, NarrowSpacing,
                                                        StartMember,
                                                            prefsw.ExtStackSize = StringObject,
                                                                ULabel("_External Stack:"),
                                                                GA_ID, GID_PW_EXTSTACKSIZE,
                                                                STRINGA_LongVal, globals->userprefs->extstacksize,
                                                                STRINGA_MaxChars, 6,
                                                                STRINGA_IntegerMin, 5000,
                                                                STRINGA_MinCharsVisible, 6,
                                                                BT_HelpHook, &HelpHook,
                                                                BT_HelpNode, "PPT.guide/VMSettings",
                                                            EndObject, FixMinWidth,
                                                        EndMember,
                                                        StartMember,
                                                            InfoObject,
                                                                INFO_TextFormat, " bytes",
                                                            EndObject,
                                                        EndMember,
                                                    EndObject, FixMinHeight,
                                                EndMember,
                                                StartMember,
                                                    HGroupObject, NarrowSpacing,
                                                        StartMember,
                                                            prefsw.ExtPriority = SliderObject,
                                                                Label("Priority"),
                                                                GA_ID, GID_PW_EXTPRIORITY,
                                                                SLIDER_Level, globals->userprefs->extpriority,
                                                                SLIDER_Min,   -127,
                                                                SLIDER_Max,   1,
                                                                BT_HelpHook,  &HelpHook,
                                                                BT_HelpNode,  "PPT.guide/VMSettings",
                                                            EndObject,
                                                        EndMember,
                                                        StartMember,
                                                            prefsw.ExtPriorityI = StringObject,
                                                                STRINGA_LongVal, globals->userprefs->extpriority,
                                                                STRINGA_IntegerMin, -127,
                                                                STRINGA_IntegerMax, 1,
                                                                STRINGA_MinCharsVisible, 4,
                                                                STRINGA_MaxChars, 4,
                                                                BT_HelpHook,  &HelpHook,
                                                                BT_HelpNode,  "PPT.guide/VMSettings",
                                                            EndObject,  Weight(1),
                                                        EndMember,
                                                    EndObject,
                                                EndMember,
                                                StartMember,
                                                    HGroupObject, NarrowSpacing,
                                                        GA_Disabled, !FindPort(EXECUTIVEAPI_PORTNAME),
                                                        StartMember,
                                                            prefsw.ExtNiceVal = SliderObject,
                                                                ULabel("External _Nice:"),
                                                                GA_ID, GID_PW_EXTNICEVAL,
                                                                SLIDER_Level, globals->userprefs->extniceval,
                                                                SLIDER_Min,   -20,
                                                                SLIDER_Max,   20,
                                                                BT_HelpHook, &HelpHook,
                                                                BT_HelpNode, "PPT.guide/VMSettings",
                                                            EndObject,
                                                        EndMember,
                                                        StartMember,
                                                            prefsw.ExtNiceValI = IndicatorObject,
                                                                INDIC_Level, globals->userprefs->extniceval,
                                                                INDIC_Min,   -20,
                                                                INDIC_Max,   20,
                                                                INDIC_Justification, IDJ_RIGHT,
                                                                INDIC_FormatString,"%ld",
                                                                ButtonFrame, FRM_Flags, FRF_RECESSED,
                                                            EndObject, Weight(1),
                                                        EndMember,
                                                    EndObject,
                                                EndMember,
                                            EndObject,
                                        EndMember,
                                        VarSpace(50),
                                    EndObject,
                                /*
                                 *  GUI page.  Contains
                                 *  a)  Main font
                                 *  b)  List font
                                 *  c)  Preview mode
                                 *  d)  Main display mode
                                 */
                                PageMember,
                                    VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                                        /*
                                         *  Group: Fonts
                                         */
                                        StartMember,
                                            VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                                            FrameTitle("Fonts"),
                                            NeXTFrame,
                                            StartMember,
                                                HGroupObject, NarrowSpacing,
                                                    StartMember,
                                                        prefsw.MainFont = InfoObject, GA_ID, GID_PW_FONT,
                                                            ULabel( "_Main font:" ),
                                                            ButtonFrame, FRM_Flags, FRF_RECESSED,
                                                            BT_HelpHook, &HelpHook,
                                                            BT_HelpNode, "PPT.guide/Fonts",
                                                            INFO_VertOffset, 3,
                                                        EndObject,
                                                    EndMember,
                                                    StartMember,
                                                        ButtonObject,
                                                            GA_ID, GID_PW_GETFONT,
                                                            ButtonFrame,
                                                            GetFile,
                                                            BT_HelpHook, &HelpHook,
                                                            BT_HelpNode, "PPT.guide/Fonts",
                                                        EndObject, FixMinWidth,
                                                    EndMember,
                                                EndObject, FixMinHeight,
                                            EndMember,

                                            StartMember,
                                                HGroupObject, NarrowSpacing,
                                                    StartMember,
                                                        prefsw.ListFont = InfoObject, GA_ID, GID_PW_LISTFONT,
                                                            ULabel( "_List font:" ),
                                                            ButtonFrame, FRM_Flags, FRF_RECESSED,
                                                            BT_HelpHook, &HelpHook,
                                                            BT_HelpNode, "PPT.guide/Fonts",
                                                            INFO_VertOffset, 3,
                                                        EndObject,
                                                    EndMember,
                                                    StartMember,
                                                        ButtonObject,
                                                            GA_ID, GID_PW_GETLISTFONT,
                                                            ButtonFrame,
                                                            BT_HelpHook, &HelpHook,
                                                            BT_HelpNode, "PPT.guide/Fonts",
                                                            GetFile,
                                                        EndObject, FixMinWidth,
                                                    EndMember,
                                                EndObject, FixMinHeight,
                                            EndMember,
                                            EndObject,
                                        EndMember,

                                        /*
                                         *  Group: Screenmode
                                         */

                                        StartMember,
                                            VGroupObject, NarrowSpacing, NormalHOffset, NormalVOffset,
                                            FrameTitle("Main screen mode"),
                                            NeXTFrame,
                                            StartMember,
                                                HGroupObject, NarrowSpacing,
                                                    StartMember,
                                                        prefsw.DispName = InfoObject,
                                                            ULabel( GetStr(MSG_SCREENMODE_LAB) ),
                                                            GA_ID, GID_PW_DISPNAME,
                                                            ButtonFrame, FRM_Flags, FRF_RECESSED,
                                                            INFO_TextFormat, buffer,
                                                            BT_HelpHook, &HelpHook,
                                                            BT_HelpNode, "PPT.guide/MainScreen",
                                                            INFO_VertOffset, 3,
                                                        EndObject, Weight(200),
                                                    EndMember,
                                                    StartMember,
                                                        ButtonObject,
                                                            GA_ID, GID_PW_GETDISP,
                                                            PopUp,
                                                            ButtonFrame,
                                                            BT_HelpHook, &HelpHook,
                                                            BT_HelpNode, "PPT.guide/MainScreen",
                                                        EndObject, FixMinWidth,
                                                    EndMember,
                                                EndObject, FixMinHeight,
                                            EndMember,
                                            StartMember,
                                                HGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                                                    StartMember,
                                                        prefsw.ColorPreview = CheckBoxObject, GA_ID, GID_PW_COLORPREVIEW,
                                                            ULabel("_Color preview?"),
                                                            GA_Selected, globals->userprefs->colorpreview,
                                                            ButtonFrame,
                                                            BT_HelpHook, &HelpHook,
                                                            BT_HelpNode, "PPT.guide/MainScreen",
                                                        EndObject, FixMinSize,
                                                    EndMember,
                                                    StartMember,
                                                        prefsw.PreviewMode = CycleObject, GA_ID, GID_PW_PREVIEWMODE,
                                                            Label("Preview size?"),
                                                            CYC_Active, globals->userprefs->previewmode,
                                                            CYC_Labels, preview_sizes,
                                                            BT_HelpHook, &HelpHook,
                                                            BT_HelpNode, "PPT.guide/MainScreen",
                                                        EndObject,
                                                    EndMember,
                                                EndObject,
                                            EndMember,
                                            EndObject,
                                        EndMember,
                                    EndObject,
                                /*
                                 *  Miscallaneous.  Contains
                                 *  a) Undo Levels
                                 *  b) Flushmodules
                                 *  c) Load info
                                 */

                                PageMember,
                                    VGroupObject, NarrowSpacing, NormalVOffset, NormalHOffset,
                                        VarSpace(50),
                                        StartMember,
                                            prefsw.MaxUndo = StringObject, GA_ID, GID_PW_MAXUNDO,
                                                ULabel("Max _Undo levels:"),
                                                StringFrame,
                                                STRINGA_LongVal,  args[6],
                                                STRINGA_MaxChars, 3,
                                                STRINGA_MinCharsVisible, 4,
                                                STRINGA_IntegerMin, 0,
                                                BT_HelpHook, &HelpHook,
                                                BT_HelpNode, "PPT.guide/MiscSettings",
                                            EndObject, FixMinSize,
                                        EndMember,
                                        StartMember,
                                            prefsw.FlushLibs = CheckBoxObject, GA_ID, GID_PW_FLUSHLIBS,
                                                ULabel("_Flush modules after use?"),
                                                GA_Selected, globals->userprefs->expungelibs,
                                                ButtonFrame,
                                                BT_HelpHook, &HelpHook,
                                                BT_HelpNode, "PPT.guide/MiscSettings",
                                            EndObject, FixMinSize,
                                        EndMember,
#ifdef DEBUG_MODE
                                        StartMember,
                                            CycleObject, GA_ID, GID_PW_ONOPEN,
                                                ULabel("On open, do"),
                                                ButtonFrame,
                                                CYC_Active, 0,
                                                CYC_Labels, onopen_labels,
                                                CYC_Popup,  TRUE,
                                                BT_HelpHook, &HelpHook,
                                                BT_HelpNode, "PPT.guide/MiscSettings",
                                            EndObject, FixMinHeight,
                                        EndMember,
#endif
                                        VarSpace(50),
                                    EndObject,

                            EndObject, /* PAGE OBJECT*/
                    EndMember,
                    StartMember,
                        HorizSeparator,
                    EndMember,
                    StartMember,
                        HGroupObject, WideSpacing, NormalHOffset, NormalVOffset, BOffset(0),
                            StartMember,
                                prefsw.Save = GenericButton( GetStr(MSG_SAVE_GAD), GID_PW_SAVE ),
                            EndMember,
                            StartMember,
                                prefsw.Use = GenericButton( GetStr(MSG_USE_GAD), GID_PW_USE ),
                            EndMember,
                            StartMember,
                                prefsw.Cancel = GenericButton( GetStr(MSG_CANCEL_GAD), GID_PW_CANCEL ),
                            EndMember,
                        EndObject, FixMinHeight,
                    EndMember,
                EndObject,
            EndObject;

        /* If window opened OK, then set cycle gadgets.. */

        if(prefsw.Win) {
            AddMap(c,p,Cyc2Page);
            AddMap( prefsw.ExtNiceVal, prefsw.ExtNiceValI, dpcol_sl2ind );
            AddMap( prefsw.ExtPriority, prefsw.ExtPriorityI, dpcol_sl2int );
            AddMap( prefsw.ExtPriorityI, prefsw.ExtPriority, dpcol_int2sl );
        }
    }

    if( prefsw.Win ) {
        extern PREFS tmpprefs;
        extern DISPLAY tmpdisp;

        CopyPrefs( globals->userprefs, &tmpprefs );
        bcopy( globals->maindisp, &tmpdisp, sizeof(DISPLAY) );

        /*
         *  Make sure we open on the right screen.
         */

        SetAttrs( prefsw.Win, WINDOW_Screen, MAINSCR, TAG_DONE );

        CheckColorPreview( &tmpdisp );

        if( prefsw.win = WindowOpen( prefsw.Win ) ) {
            ScreenToFront(MAINSCR);
        } else {
            DisposeObject( prefsw.Win );
            prefsw.Win = NULL;
        }
    }
    return prefsw.Win;
}

///

/// GimmeMainWindow()

/*
    Open the main window. This routine will put the object pointers into
    the global structure, albeit it would be more beautiful to hide them
    somehow. There's no need to obtain main semaphore, since we are called
    only once in the beginning.

    NOTE: This window's IDCMP port is the master port for all main task
    windows.
 */

Object *GimmeMainWindow( VOID )
{
    struct IBox ibox;
    static ULONG colweights[] = { 20, 100 };
    extern ASM LONG CompareListEntryHook( REG(a0) struct Hook *, REG(a2) Object *, REG(a1) struct lvCompare * );
    static struct Hook frmCompareHook = { {0}, CompareListEntryHook, NULL, NULL };

    D(bug("GimmeMainWindow()\n"));

    if( framew.initialpos.Height == 0 ) {
        if(MAINSCR) {
            ibox.Left = MAINSCR->Width;
        } else {
            ibox.Left   = 640;
        }
        ibox.Top    = 10;
        ibox.Height = 200;
        ibox.Width  = 150;
    } else {
        ibox = framew.initialpos;
    }

    framew.Win = WindowObject,
            WINDOW_Title,           GetStr(MSG_MAIN_WINDOW),
            WINDOW_ScreenTitle,     std_ppt_blurb,
            WINDOW_MenuStrip,       PPTMenus,
            WINDOW_SharedPort,      MainIDCMPPort,
            WINDOW_AppWindow,       TRUE,
            WINDOW_ScaleWidth,      10,
            WINDOW_ScaleHeight,     20,
            WINDOW_Bounds,          &ibox,
            WINDOW_Font,            globals->userprefs->mainfont,
            WINDOW_Screen,          MAINSCR,
//            WINDOW_HelpHook,            &HelpHook,
//            WINDOW_HelpNode,            "PPT.guide/MainWindow",
            WINDOW_MasterGroup,
                VGroupObject, NarrowHOffset, NarrowVOffset, NarrowSpacing,
                    StartMember,
                        framew.Frames = ListviewObject,
                            ULabel(GetStr(MSG_PICK_FRAME)), Place(PLACE_ABOVE),
                            GA_ID, GID_MW_LIST,
                            LISTV_ListFont, globals->userprefs->listfont,
                            BT_DragObject, TRUE,
                            BT_DropObject, TRUE,
                            LISTV_ShowDropSpot, FALSE,
                            LISTV_Columns, 2,
                            LISTV_DragColumns, TRUE,
                            LISTV_ColumnWeights, colweights,
                            LISTV_CompareHook,   &frmCompareHook,
                            BT_HelpHook, &HelpHook,
                            BT_HelpNode, "PPT.guide/MainWindow",    
                        EndObject,
                    EndMember,
                    StartMember,
                        VGroupObject, Spacing(0),
                            StartMember,
                                framew.Info = InfoObject,
                                    INFO_TextFormat, GetStr(MSG_MAIN_NOFRAME),
                                    ButtonFrame, FRM_Flags, FRF_RECESSED,
                                    INFO_MinLines, 3,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, "PPT.guide/MainWindow",
                                EndObject,
                            EndMember,
                        EndObject, FixMinHeight,
                    EndMember,
                EndObject, /* Master group */
          EndObject;

    return framew.Win;
}
///

/// GimmeToolBarWindow()
/*
    This creates and opens the tool bar window.
*/
PERROR GimmeToolBarWindow( VOID )
{
    PERROR res = PERR_OK;
    ULONG  args[2];
    struct IBox ibox;
    long   minwidth, minheight;
    struct TextExtent te;

    args[0] = args[1] = 0L;

    if( toolw.initialpos.Height == 0 ) {
        ibox.Left   = 0;
        if( MAINSCR )
            ibox.Top    = MAINSCR->Height;
        else
            ibox.Top    = 320;
    } else {
        ibox = toolw.initialpos;
    }

    ibox.Height = 0;
    ibox.Width  = DEFAULT_TOOLBAR_WIDTH;

    D(bug("GimmeToolBarWindow()\n"));

    /*
     *  Create the area for rendering the co-ordinates in.
     *  Allow two pixels spare on all sides.
     */

    if(MAINWIN && MAINWIN->RPort) {
        TextExtent( MAINWIN->RPort, "(MMM,MMM)MM", 11, &te );
        minwidth = te.te_Width+4;
        minheight = te.te_Height+4;
        D(bug("\tCoordBox minimum size = %ld x %ld pixels\n",minwidth, minheight ));
    } else {
        minwidth = 100;
        minheight = 12;
        InternalError("Unable to determine Main Window RastPort???");
        return PERR_WINDOWOPEN;
    }

    toolw.GO_ToolInfo = BGUI_NewObject( BGUI_AREA_GADGET,
                                        AREA_MinWidth,     minwidth,
                                        AREA_MinHeight,    minheight,
                                        BT_DropObject,     FALSE,
                                        GA_ID,             GID_TOOL_COORDS,
                                        ICA_TARGET,        ICTARGET_IDCMP,
                                        ButtonFrame,
                                        FRM_Flags,         FRF_RECESSED,
                                        BT_HelpHook,       &HelpHook,
                                        BT_HelpNode,       "PPT.guide/Toolbar",
                                        TAG_END );
    if( !toolw.GO_ToolInfo ) {
        D(bug("Toolinfo didn't open\n"));
        return PERR_WINDOWOPEN;
    }

    /*
     *  The window itself. The area is attached onto it.
     */

    if(!toolw.Win) {
        toolw.Win = WindowObject,
            WINDOW_Title,           GetStr(MSG_TOOLBAR_WINDOW),
            WINDOW_ScreenTitle,     std_ppt_blurb,
            WINDOW_MenuStrip,       PPTMenus,
            WINDOW_SharedPort,      MainIDCMPPort,
            WINDOW_Font,            globals->userprefs->mainfont,
            WINDOW_Screen,          MAINSCR,
            WINDOW_Bounds,          &ibox,
            WINDOW_SmartRefresh,    TRUE,
            WINDOW_SizeGadget,      FALSE,
            WINDOW_NoBufferRP,      TRUE,
            // WINDOW_HelpHook,            &HelpHook,
            // WINDOW_HelpNode,            "Toolbar",
            WINDOW_MasterGroup,
                HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset, EqualWidth, EqualHeight,
                    StartMember,
                        HGroupObject, Spacing(0), HOffset(0), VOffset(0),
                            StartMember,
                                toolw.GO_ToolInfo,
                            EndMember,
                        EndObject, FixMinWidth,
                    EndMember,
                    StartMember,
                        GenericButton( GetStr(MSG_LOAD_GAD), GID_TOOL_LOAD),
                    EndMember,
                    StartMember,
                        GenericButton( GetStr(MSG_PROCESS_GAD),GID_TOOL_PROCESS),
                    EndMember,
                EndObject, FixMinHeight,
            EndObject;
    }

    if(!toolw.Win) {
        D(bug("Tool window couldn't be created\n"));
        DisposeObject( toolw.GO_ToolInfo );
        res = PERR_WINDOWOPEN;
    }

    return res;
}
///

/// GimmeSelectWindow()

Prototype PERROR GimmeSelectWindow(VOID);

PERROR GimmeSelectWindow(VOID)
{
    PERROR res = PERR_WINDOWOPEN;
    struct IBox ibox;

    if( selectw.initialpos.Height == 0 ) {
        ibox.Left   = DEFAULT_TOOLBAR_WIDTH+1; // Just to the left of the ToolBox
        if( MAINSCR )
            ibox.Top    = MAINSCR->Height;
        else
            ibox.Top    = 320;
    } else {
        ibox = selectw.initialpos;
    }

    ibox.Height = ibox.Width  = 0; // Use defaults

    selectw.Win = WindowObject,
        WINDOW_Screen,      MAINSCR,
        WINDOW_Title,       "Selected area",
        WINDOW_ScreenTitle, std_ppt_blurb,
        WINDOW_MenuStrip,   PPTMenus,
        WINDOW_SharedPort,  MainIDCMPPort,
        WINDOW_Font,        globals->userprefs->mainfont,
        WINDOW_SizeGadget,  FALSE,
        WINDOW_Bounds,      &ibox,
        WINDOW_MasterGroup,
            VGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                StartMember,
                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                        StartMember,
                            HGroupObject, NarrowSpacing, HOffset(0), VOffset(0),
                                StartMember,
                                    selectw.TopLeft = StringObject,
                                        GA_ID, GID_SELECT_TOPLEFT,
                                        STRINGA_LongVal, 0,
                                        STRINGA_IntegerMin, 0,
                                        STRINGA_MinCharsVisible, 5,
                                        STRINGA_Justification, STRINGCENTER,
                                        STRINGA_MaxChars, 5,
                                        BT_HelpHook, &HelpHook,
                                        BT_HelpNode, "PPT.guide/Select",
                                    EndObject,
                                EndMember,
                                StartMember,
                                    selectw.TopRight = StringObject,
                                        GA_ID, GID_SELECT_TOPRIGHT,
                                        STRINGA_IntegerMin, 0,
                                        STRINGA_MinCharsVisible, 5,
                                        STRINGA_Justification, STRINGCENTER,
                                        STRINGA_LongVal, 0,
                                        STRINGA_MaxChars, 5,
                                        BT_HelpHook, &HelpHook,
                                        BT_HelpNode, "PPT.guide/Select",
                                    EndObject,
                                EndMember,
                            EndObject,
                        EndMember,
                        StartMember,
                            InfoObject, INFO_TextFormat, "--",
                                        INFO_VertOffset, 3,
                            EndObject,
                        EndMember,
                        StartMember,
                            HGroupObject, NarrowSpacing, HOffset(0), VOffset(0),
                                StartMember,
                                    selectw.BottomLeft = StringObject,
                                        GA_ID, GID_SELECT_BOTTOMLEFT,
                                        STRINGA_LongVal, 0,
                                        STRINGA_IntegerMin, 0,
                                        STRINGA_MinCharsVisible, 5,
                                        STRINGA_Justification, STRINGCENTER,
                                        STRINGA_MaxChars, 5,
                                        BT_HelpHook, &HelpHook,
                                        BT_HelpNode, "PPT.guide/Select",
                                    EndObject,
                                EndMember,
                                StartMember,
                                    selectw.BottomRight = StringObject,
                                        GA_ID, GID_SELECT_BOTTOMRIGHT,
                                        STRINGA_LongVal, 0,
                                        STRINGA_IntegerMin, 0,
                                        STRINGA_MinCharsVisible, 5,
                                        STRINGA_Justification, STRINGCENTER,
                                        STRINGA_MaxChars, 5,
                                        BT_HelpHook, &HelpHook,
                                        BT_HelpNode, "PPT.guide/Select",
                                    EndObject,
                                EndMember,
                            EndObject,

                        EndMember,
                    EndObject,
                EndMember,

                StartMember,
                    HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                        StartMember,
                            InfoObject, INFO_TextFormat, "Area size:",
                                        INFO_VertOffset, 3,
                                        INFO_FixTextWidth, TRUE,
                            EndObject, FixMinWidth,
                        EndMember,
                        StartMember,
                            selectw.Width = StringObject,
                                GA_ID, GID_SELECT_WIDTH,
                                STRINGA_LongVal, 0,
                                STRINGA_MaxChars,5,
                                STRINGA_Justification, STRINGCENTER,
                                STRINGA_IntegerMin, 0,
                                STRINGA_MinCharsVisible, 5,
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/Select",
                            EndObject,
                        EndMember,
                        StartMember,
                            InfoObject, INFO_TextFormat, "x",
                                        INFO_VertOffset, 3,
                                        INFO_HorizOffset, 3,
                            EndObject, FixMinWidth,
                        EndMember,
                        StartMember,
                            selectw.Height = StringObject,
                                GA_ID, GID_SELECT_HEIGHT,
                                STRINGA_LongVal, 0,
                                STRINGA_MaxChars,5,
                                STRINGA_Justification, STRINGCENTER,
                                STRINGA_IntegerMin, 0,
                                STRINGA_MinCharsVisible, 5,
                                BT_HelpHook, &HelpHook,
                                BT_HelpNode, "PPT.guide/Select",
                            EndObject,
                        EndMember,
                    EndObject,
                EndMember,
            EndObject,
        EndObject;

    if( selectw.Win )
        res = PERR_OK;

    return res;
}

///

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

