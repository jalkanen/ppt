/*----------------------------------------------------------------------*/
/*
    PROJECT: PPT
    MODULE : gui.c

    $Id: gui.c,v 1.65 1999/03/13 17:33:22 jj Exp $

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
   B -                                  U - Save As
   C - Copy                             V - Paste
   D - Close                            W - Save
   E - Effects Window                   X - Cut
   F - Frames Window                    Y - Edit Info
   G - Edit Palette                     Z - Undo
   H - Hide/Show                        + - Zoom in
   I - SelectWindow                     - - Zoom out
   J - Correct Aspect                   0 -
   K - Close Render                     1 -
   L - Loaders Window                   2 -
   M -                                  3 -
   N - New                              4 -
   O - Open                             5 -
   P - Process                          6 -
   Q - Quit                             7 -
   R - Render                           8 -
   S - Scripts Window                   9 -
   . - Break                            , -

*/

/*
 *  We don't want to localize all of the menu codes
 */

#define MF_NO_LOCALIZE                  0x80000000
#define TitleTxt(t)                     { NM_TITLE, t, NULL, 0, 0, (APTR)MF_NO_LOCALIZE }
#define ItemTxt(t,s,i)                  { NM_ITEM, t, s, 0, 0, (APTR)(i | MF_NO_LOCALIZE) }
#define DItemTxt(t,s,i)                 { NM_ITEM, t, s, NM_ITEMDISABLED, 0, (APTR)(i | MF_NO_LOCALIZE) }
#define SubItemTxt(t,s,i)               { NM_SUB, t, s, 0, 0, (APTR)(i |MF_NO_LOCALIZE) }

/* Turns off the SAS/C warning about incompatible pointer types. */
#pragma msg 225 ignore
struct NewMenu PPTMenus[] = {
    Title( mMENU_PROJECT ),
        Item( mMENU_PROJECT_NEW,        mMENU_PROJECT_NEW_KEY,          MID_NEW ),
        Item( mMENU_PROJECT_OPEN,       mMENU_PROJECT_OPEN_KEY,         MID_LOADNEW ),
        Item( mMENU_PROJECT_OPENAS,     mMENU_PROJECT_OPENAS_KEY,       MID_LOADAS ),
        // DItem("Save",            "W",                        MID_SAVE ),
        DItem(mMENU_PROJECT_SAVEAS,     mMENU_PROJECT_SAVEAS_KEY,       MID_SAVEAS ),
        DItem(mMENU_PROJECT_CLOSE,      mMENU_PROJECT_CLOSE_KEY,        MID_DELETE ),
        ItemBar,
        DItem(mMENU_PROJECT_RENAME,     mMENU_PROJECT_RENAME_KEY,       MID_RENAME ),
        DItem(mMENU_PROJECT_HIDESHOW,   mMENU_PROJECT_HIDESHOW_KEY,     MID_HIDE ),
        Item( mMENU_PROJECT_PREFERENCES,mMENU_PROJECT_PREFERENCES_KEY,  MID_PREFS ),
        ItemBar,
        Item( mMENU_PROJECT_ABOUT,      mMENU_PROJECT_ABOUT_KEY,        MID_ABOUT ),
        ItemBar,
        Item( mMENU_PROJECT_QUIT,       mMENU_PROJECT_QUIT_KEY,         MID_QUIT ),
    Title( mMENU_EDIT ),
        DItem(mMENU_EDIT_UNDO,          mMENU_EDIT_UNDO_KEY,            MID_UNDO ),
        DItem(mMENU_EDIT_CUT,           mMENU_EDIT_CUT_KEY,             MID_CUT ),
        DItem(mMENU_EDIT_CUTTONEW,      mMENU_EDIT_CUTTONEW_KEY,        MID_CUTFRAME ),
        DItem(mMENU_EDIT_COPY,          mMENU_EDIT_COPY_KEY,            MID_COPY ),
        DItem(mMENU_EDIT_PASTE,         mMENU_EDIT_PASTE_KEY,           MID_PASTE ),
#ifdef DEBUG_MODE
        DItemTxt( "Crop",          NULL,   MID_CROP ),
#endif
        ItemBar,
        DItem(mMENU_EDIT_ZOOM_IN,       mMENU_EDIT_ZOOM_IN_KEY,         MID_ZOOMIN ),
        DItem(mMENU_EDIT_ZOOM_OUT,      mMENU_EDIT_ZOOM_OUT_KEY,        MID_ZOOMOUT ),
        ItemBar,
        DItem(mMENU_EDIT_SELECT_ALL,    mMENU_EDIT_SELECT_ALL_KEY,      MID_SELECTALL ),
        DItem(mMENU_EDIT_EDIT_INFO,     mMENU_EDIT_EDIT_INFO_KEY,       MID_EDITINFO ),
    Title( mMENU_DISPLAY ),
        DItem( mMENU_DISP_SETTINGS,     mMENU_DISP_SETTINGS_KEY,        MID_SETRENDER ),
        DItem( mMENU_DISP_RENDER,       mMENU_DISP_RENDER_KEY,          MID_RENDER ),
        DItem( mMENU_DISP_CLOSERENDER,  mMENU_DISP_CLOSERENDER_KEY,     MID_CLOSERENDER ),
        ItemBar,
        DItem( mMENU_DISP_CORRECTASP,   mMENU_DISP_CORRECTASP_KEY,      MID_CORRECTASPECT ),
        DItem( mMENU_DISP_PALETTE,      mMENU_DISP_PALETTE_KEY,         MID_PALETTE ),
            SubItem( mMENU_DISP_PAL_EDIT, mMENU_DISP_PAL_EDIT_KEY,      MID_EDITPALETTE ),
            SubItem( mMENU_DISP_PAL_LOAD, mMENU_DISP_PAL_LOAD_KEY,      MID_LOADPALETTE ),
            SubItem( mMENU_DISP_PAL_SAVE, mMENU_DISP_PAL_SAVE_KEY,      MID_SAVEPALETTE ),
    Title( mMENU_PROCESS ),
        DItem( mMENU_PROC_PROCESS,      mMENU_PROC_PROCESS_KEY,         MID_PROCESS ),
        DItem( mMENU_PROC_BREAK,        mMENU_PROC_BREAK_KEY,           MID_BREAK ),
#ifdef USE_OLD_ALPHA
        DItemTxt( "Remove Alpha",  NULL,   MID_REMOVEALPHA ),
#endif
#ifdef DEBUG_MODE
    TitleTxt( "Debug" ),
        ItemTxt( "SavPalette",     NULL,   MID_SAVEMAINPALETTE ),
        ItemTxt( "Memory Check",   NULL,   MID_MEMCHECK ),
        ItemTxt( "Test AskReq()",  NULL,   MID_TESTAR ),
        ItemTxt( "Memory Fail Rate", NULL, MID_MEMFAIL ),
            { NM_SUB, " 0 %", NULL, CHECKIT|MENUTOGGLE|CHECKED, ~1, (APTR)(MID_MEMFAIL0|MF_NO_LOCALIZE) },
            { NM_SUB, "10 %", NULL, CHECKIT|MENUTOGGLE, ~2, (APTR)(MID_MEMFAIL10|MF_NO_LOCALIZE) },
            { NM_SUB, "25 %", NULL, CHECKIT|MENUTOGGLE, ~4, (APTR)(MID_MEMFAIL25|MF_NO_LOCALIZE) },
            { NM_SUB, "50 %", NULL, CHECKIT|MENUTOGGLE, ~8, (APTR)(MID_MEMFAIL50|MF_NO_LOCALIZE) },
#endif
    Title( mMENU_WINDOW ),
        { NM_ITEM, mMENU_WIN_FRAMES,    mMENU_WIN_FRAMES_KEY,     CHECKIT|MENUTOGGLE, 0L, (APTR)MID_FRAMEWINDOW },
        { NM_ITEM, mMENU_WIN_TOOLBAR,   mMENU_WIN_TOOLBAR_KEY,    CHECKIT|MENUTOGGLE, 0L, (APTR)MID_TOOLWINDOW },
        { NM_ITEM, mMENU_WIN_SELECT,    mMENU_WIN_SELECT_KEY,     CHECKIT|MENUTOGGLE, 0L, (APTR)MID_SELECTWINDOW },
        { NM_ITEM, mMENU_WIN_EFFECTS,   mMENU_WIN_EFFECTS_KEY,    CHECKIT|MENUTOGGLE, 0L, (APTR)MID_EFFECTS },
        { NM_ITEM, mMENU_WIN_LOADERS,   mMENU_WIN_LOADERS_KEY,    CHECKIT|MENUTOGGLE, 0L, (APTR)MID_LOADERS },
        { NM_ITEM, mMENU_WIN_SCRIPTS,   mMENU_WIN_SCRIPTS_KEY,    CHECKIT|MENUTOGGLE, 0L, (APTR)MID_REXXWINDOW },
    End
};
#pragma msg 225 warn


const struct NewMenu SaveMenus[] = {
    Title("Save"),
        Item("Save", NULL, GID_SW_SAVE),
        Item("Get Path...", NULL, GID_SW_GETFILE),
        Item("Cancel", NULL, GID_SW_CANCEL),
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
const char *color_labels[5] = { 0 };
const char *palette_labels[4] = { 0 };
const char *save_mode_labels[3] = { 0 };

const char *dither_labels[] = {
    NULL, // Off
    NULL, // Ordered
    "Floyd-Steinberg",
    NULL,
};

const char *scr_labels[] = {
    "Amiga",
    NULL,
};

Local UBYTE *Prefspages[5] = { NULL };
Local UBYTE *preview_sizes[6] = { NULL };
Local UBYTE *toolbar_type_labels[3] = { NULL };

/* This is the screentitle */
#ifdef DEBUG_MODE
const char *std_ppt_blurb = "PPT "RELEASE" (v"VERSION").   "COPYRIGHT" - DO NOT DISTRIBUTE!";
#else
const char *std_ppt_blurb = "PPT "RELEASE" (v"VERSION").   "COPYRIGHT;
#endif

char workbuf[MAXPATHLEN+1],undobuf[MAXPATHLEN+1];
///



/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Code */

/// ColorSpaceName()
/*
    This returns the colorspace name.  It is a separate function instead
    of an array for localization purposes.  Note that if you need
    an english name, you can access ColorSpaceNamesE[] array.
 */
Prototype const char *ColorSpaceName( int id );

const char *ColorSpaceName( int id )
{
    switch(id) {
        case CS_GRAYLEVEL:
            return GetStr(mCSPACE_GREY);
        case CS_RGB:
            return GetStr(mCSPACE_RGB);
        case CS_LUT:
            return GetStr(mCSPACE_COLORMAPPED);
        case CS_ARGB:
            return GetStr(mCSPACE_ARGB);
    }
    return "Unknown";
}
///

/// InitGUILocale()
/*
 *  Initializes the menu array for localization.
 */

Prototype PERROR InitGUILocale(VOID);

PERROR InitGUILocale(VOID)
{
    int i;
    char *s;

    D(bug("\tInitGUILocale()\n"));

    save_mode_labels[0] = GetStr( mSAVE_MODE_LABEL_TRUECOLOR );
    save_mode_labels[1] = GetStr( mSAVE_MODE_LABEL_COLORMAP );

    dither_labels[0] = GetStr(mRENDER_DITHER_OFF);
    dither_labels[1] = GetStr(mRENDER_DITHER_ORDERED);

    palette_labels[0] = GetStr(mRENDER_PALETTE_MEDIAN_CUT);
    palette_labels[1] = GetStr(mRENDER_PALETTE_POPULARITY);
    palette_labels[2] = GetStr(mRENDER_PALETTE_FORCE);

    color_labels[0] = GetStr(mRENDER_COLOR_NORMAL);
    color_labels[1] = GetStr(mRENDER_COLOR_EHB);
    color_labels[2] = GetStr(mRENDER_COLOR_HAM);
    color_labels[3] = GetStr(mRENDER_COLOR_HAM8);

    Prefspages[0] = GetStr(mPREFS_PAGE_LABEL_SYSTEM);
    Prefspages[1] = GetStr(mPREFS_PAGE_LABEL_GUI);
    Prefspages[2] = GetStr(mPREFS_PAGE_LABEL_MISC);
    Prefspages[3] = GetStr(mPREFS_PAGE_LABEL_TOOLBAR);

    preview_sizes[0] = GetStr(mPREFS_PREVIEW_SIZE_OFF);
    preview_sizes[1] = GetStr(mPREFS_PREVIEW_SIZE_SMALL);
    preview_sizes[2] = GetStr(mPREFS_PREVIEW_SIZE_MEDIUM);
    preview_sizes[3] = GetStr(mPREFS_PREVIEW_SIZE_LARGE);
    preview_sizes[4] = GetStr(mPREFS_PREVIEW_SIZE_HUGE);

    toolbar_type_labels[0] = GetStr(mPREFS_TOOLBAR_TYPE_TEXT);
    toolbar_type_labels[1] = GetStr(mPREFS_TOOLBAR_TYPE_IMAGE);

    for( i = 0; PPTMenus[i].nm_Type != NM_END; i++ ) {
        /*
         *  Skip, if we don't want to localize.  Otherwise, start
         *  going through the localization thingies.
         */
        if( (ULONG)PPTMenus[i].nm_UserData & MF_NO_LOCALIZE ) {
            PPTMenus[i].nm_UserData = (APTR)((ULONG)PPTMenus[i].nm_UserData & ~MF_NO_LOCALIZE); /* Remove flag */
            continue;
        }
        if( (PPTMenus[i].nm_Type == NM_TITLE || PPTMenus[i].nm_Type == NM_ITEM ||
             PPTMenus[i].nm_Type == NM_SUB )
             && !(PPTMenus[i].nm_Type & NM_IGNORE)
             && PPTMenus[i].nm_Label != NM_BARLABEL )
        {
            PPTMenus[i].nm_Label = GetStr( (struct LocaleString *)PPTMenus[i].nm_Label );
        }

        s = GetStr( (struct LocaleString *)PPTMenus[i].nm_CommKey);
        if( s[0] == '\0' || s[1] != '\0' || s == NULL ) {
            PPTMenus[i].nm_CommKey = NULL;
        } else {
            PPTMenus[i].nm_CommKey = s;
        }
    }

    LocalizeToolbar();

    return PERR_OK;
}

///

/// "MyNewObject()"
/*
    This is a replacement stub for BGUI_NewObject.
*/

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
                            pw->Remap = GenericButton( GetStr(mPALETTE_REMAP_GAD), GID_PAL_REMAP ),
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
        ASLFR_InitialPattern, "#?",
        ASLFR_DoPatterns,    TRUE,
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
                        NoFrame,
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
                                    NoFrame,
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
                        HGroupObject,
                            StartMember,
                                dpw->GO_DrawAlpha = CheckBoxObject, GA_ID, GID_DP_FORCEBW,
                                     ULabel("Draw Alpha?"), Place( PLACE_LEFT ),
                                     ButtonFrame,
                                     GA_Selected, FALSE,
                                     BT_HelpHook, &HelpHook,
                                     BT_HelpNode, "PPT.guide/RenderSettings",
                                EndObject, FixMinWidth,
                            EndMember,
                            VarSpace(50),
                        EndObject,
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

#if 0
ULONG SAVEDS ASM
PrefsToolbarDisplayHook( REGPARAM(a0,struct Hook *,hook),
                         REGPARAM(a2,Object *,obj),
                         REGPARAM(a1,struct lvRender *,lvr) )
{
    return (ULONG)( ((struct ToolbarItem *)lvr->lvr_Entry)->ti_Label );
}

/*
 *  We do not need to copy the data, because there is only
 *  one instance, and the data is already in a static location
 *  in memory.
 */

ULONG SAVEDS ASM
PrefsToolbarResourceHook( REGPARAM(a0,struct Hook *,hook),
                          REGPARAM(a2,Object *,obj),
                          REGPARAM(a1,struct lvResource *,lvr) )
{
    // D(bug("AddHook(%08X=%s)\n",lvr->lvr_Entry, ((struct ToolbarItem*)lvr->lvr_Entry)->ti_Label));
    return lvr->lvr_Entry;
}
struct Hook hook_PrefsToolbarDisplay = {
    {0},
    PrefsToolbarDisplayHook,
    NULL,
    NULL
};

struct Hook hook_PrefsToolbarResource = {
    {0},
    PrefsToolbarResourceHook,
    NULL,
    NULL
};
#endif

Local ULONG Cyc2Page[] = { MX_Active, PAGE_Active, TAG_END };
Local UBYTE *onopen_labels[] = { "do not show the image", "show smallest",
                                 "show as 25% of screen size",
                                 "show as 50% of screen size",
                                 "show largest possible", NULL };


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

#if 0
    if( CyberGfxBase ) {
        if( IsCyberModeID( disp->dispid ) ) {
            depth = GetCyberIDAttr( CYBRIDATTR_DEPTH, disp->dispid );
        }
    }
#endif

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
                                                                ULabel("_Page size (kB)"), GA_ID, GID_PW_VMBUFSIZE,
                                                                STRINGA_LongVal, args[5],
                                                                STRINGA_MaxChars, 5,
                                                                STRINGA_IntegerMin, MIN_VMBUFSIZ,
                                                                STRINGA_UndoBuffer, undobuf,
                                                                STRINGA_WorkBuffer, workbuf,
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
                                                                ULabel("_External Stack"),
                                                                GA_ID, GID_PW_EXTSTACKSIZE,
                                                                STRINGA_LongVal, globals->userprefs->extstacksize,
                                                                STRINGA_MaxChars, 6,
                                                                STRINGA_IntegerMin, 5000,
                                                                STRINGA_MinCharsVisible, 6,
                                                                BT_HelpHook, &HelpHook,
                                                                STRINGA_UndoBuffer, undobuf,
                                                                STRINGA_WorkBuffer, workbuf,
                                                                BT_HelpNode, "PPT.guide/VMSettings",
                                                            EndObject, FixMinWidth,
                                                        EndMember,
                                                        StartMember,
                                                            InfoObject,
                                                                INFO_TextFormat, " bytes",
                                                                NoFrame,
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
                                                                STRINGA_UndoBuffer, undobuf,
                                                                STRINGA_WorkBuffer, workbuf,
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
                                                                ULabel("External _Nice"),
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
                                                                NoFrame,
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
                                                            ULabel( "_Main font" ),
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
                                                            ULabel( "_List font" ),
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
                                                HGroupObject, WideSpacing, NormalHOffset, NormalVOffset,
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
                                                        prefsw.DitherPreview = CheckBoxObject, GA_ID, GID_PW_DITHERPREVIEW,
                                                            ULabel("Dither preview?"),
                                                            GA_Selected, globals->userprefs->ditherpreview,
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
                                                ULabel("Max _Undo levels"),
                                                StringFrame,
                                                STRINGA_LongVal,  args[6],
                                                STRINGA_UndoBuffer, undobuf,
                                                STRINGA_WorkBuffer, workbuf,
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
                                        StartMember,
                                            prefsw.Confirm = CheckBoxObject, GA_ID, GID_PW_CONFIRM,
                                                ULabel("Confirm requesters?"),
                                                GA_Selected, globals->userprefs->confirm,
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

                                /*
                                 *  Toolbar configuration window.
                                 */
                                PageMember,
                                    HGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                                        StartMember,
                                            prefsw.AvailButtons = ListviewObject,
                                                Label("Available buttons"), Place(PLACE_ABOVE),
                                                GA_ID, GID_PW_AVAILBUTTONS,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            VGroupObject,
                                                VarSpace(50),
                                                StartMember,
                                                    prefsw.ToToolbar = ButtonObject,
                                                        Label("->"),
                                                        GA_ID, GID_PW_TOTOOLBAR,
                                                    EndObject, Weight(25),
                                                EndMember,
                                                VarSpace(10),
                                                StartMember,
                                                    prefsw.FromToolbar = ButtonObject,
                                                        Label("<-"),
                                                        GA_ID, GID_PW_FROMTOOLBAR,
                                                    EndObject, Weight(25),
                                                EndMember,
                                                VarSpace(50),
                                            EndObject, Weight(5),
                                        EndMember,
                                        StartMember,
                                            prefsw.ToolbarList = ListviewObject,
                                                Label("Toolbar"),
                                                Place(PLACE_ABOVE),
                                                GA_ID, GID_PW_TOOLBARLIST,
                                                BT_DragObject, TRUE,
                                                BT_DropObject, TRUE,
                                                LISTV_ShowDropSpot, TRUE,
                                                // LISTV_DisplayHook, &hook_PrefsToolbarDisplay,
                                                // LISTV_ResourceHook, &hook_PrefsToolbarResource,
                                            EndObject,
                                        EndMember,
                                        StartMember,
                                            VGroupObject, NormalSpacing, NormalOffset,
                                                NeXTFrame,
                                                FrameTitle("Item type"),
                                                VarSpace(50),
                                                StartMember,
                                                    prefsw.ToolItemType = MxObject,
                                                        GROUP_Style,GRSTYLE_VERTICAL,
                                                        MX_Labels,  toolbar_type_labels,
                                                        MX_Active,  0,
                                                        GA_ID,      GID_PW_TOOLITEMTYPE,
                                                    EndObject,
                                                EndMember,
                                                StartMember,
                                                    prefsw.ToolItemFileGroup = HGroupObject, NarrowSpacing, NormalOffset,
                                                        GA_Disabled, TRUE,
                                                        StartMember,
                                                            prefsw.ToolItemFile = StringObject,
                                                                STRINGA_MaxChars, MAXPATHLEN,
                                                                STRINGA_UndoBuffer, undobuf,
                                                                STRINGA_WorkBuffer, workbuf,
                                                                STRINGA_Justification, STRINGRIGHT,
                                                                Label("File"),
                                                                Place(PLACE_LEFT),
                                                                GA_ID, GID_PW_TOOLITEMFILE,
                                                            EndObject,
                                                        EndMember,
                                                        StartMember,
                                                            ButtonObject,
                                                                GetFile,
                                                                GA_ID, GID_PW_GETTOOLITEMFILE,
                                                            EndObject, FixMinWidth,
                                                        EndMember,
                                                    EndObject, FixMinHeight,
                                                EndMember,
                                                VarSpace(50),
                                            EndObject,
                                        EndMember,
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
            AddCondit( prefsw.ToolItemType, prefsw.ToolItemFileGroup, MX_Active, 0, GA_Disabled, TRUE, GA_Disabled, FALSE);

            SetupToolbarList();
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

    if( framew.prefs.initialpos.Height == 0 ) {
        if(MAINSCR) {
            ibox.Left = MAINSCR->Width;
        } else {
            ibox.Left   = 640;
        }
        ibox.Top    = 10;
        ibox.Height = 200;
        ibox.Width  = 150;
    } else {
        ibox = framew.prefs.initialpos;
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

/// GimmeSelectWindow()

Local UBYTE *Selectpages[] = { "Rectangle", "Circle", NULL };

Prototype PERROR GimmeSelectWindow(VOID);

PERROR GimmeSelectWindow(VOID)
{
    PERROR res = PERR_WINDOWOPEN;
    struct IBox ibox;
    Object *c;

    if( selectw.prefs.initialpos.Height == 0 ) {
        ibox.Left   = 201; // BUG: Must adjust to default toolbar place.
        if( MAINSCR )
            ibox.Top    = MAINSCR->Height;
        else
            ibox.Top    = 320;
    } else {
        ibox = selectw.prefs.initialpos;
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
            VGroupObject,
#ifdef DEBUG_MODE
                StartMember,
                    c = Tabs( NULL, Selectpages, 0, GID_SELECT_PAGE ),
                EndMember,
#endif
                StartMember,
                    selectw.Page = PageObject,
                        // GA_ID, GID_SELECT_PAGE,
                        PageMember,
                            /*
                             *  Rectangle page
                             */

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
                        PageMember,
                            VGroupObject, NormalSpacing, NormalOffset,
                                StartMember,
                                    HGroupObject, NormalSpacing, Offset(0),
                                        StartMember,
                                            selectw.CircleX = StringObject,
                                                GA_ID, GID_SELECT_CIRCLEX,
                                                Label("X"),
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
                                            selectw.CircleY = StringObject,
                                                GA_ID, GID_SELECT_CIRCLEY,
                                                Label("Y"),
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
                                StartMember,
                                    selectw.CircleRadius = StringObject,
                                        GA_ID, GID_SELECT_CIRCLERADIUS,
                                        Label("Radius:"),
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
                    EndObject, /* Page object */
                EndMember,
            EndObject, /* Master vgroup */
        EndObject;

    if( selectw.Win ) {
#ifdef DEBUG_MODE
        AddMap(c,selectw.Page,Cyc2Page);
#endif
        res = PERR_OK;
    }

    return res;
}

///

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

