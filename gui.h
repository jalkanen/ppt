
/*
    GUI definitions.

    $Id: gui.h,v 1.35 1999/02/14 19:37:38 jj Exp $
*/

#ifndef GUI_H
#define GUI_H

#ifndef LIBRARIES_BGUI_H
#include <libraries/bgui.h>
#endif

#ifndef LIBRARIES_BGUI_MACROS_H
#include <libraries/bgui_macros.h>
#endif

#ifndef PRAGMAS_BGUI_PRAGMAS_H
#include <pragmas/bgui_pragmas.h>
#endif

#ifndef CLIB_INTUITION_PROTOS_H
#include <clib/intuition_protos.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif

#ifndef DROPAREACLASS_H
#include "dropareaclass.h"
#endif

#include "toolbar.h"
#include "bitmapclass.h"

#ifndef FLOATGADGET_H
#include "floatgadget/floatgadget.h"
#endif

#ifndef INTUITION_ICCLASS_H
#include <intuition/icclass.h>
#endif

/*------------------------------------------------------------------*/
/* Effect window object structure */

struct EffectWindow {
    Object *WO_Win, *GO_Info, *GO_Exec, *GO_Cancel, *GO_List;
    struct Window *win;
    ULONG  lcs, lcm; /* Last Clicked Seconds/Milliseconds */
    ULONG  lastclicked;
};

/*------------------------------------------------------------------*/
/* Generic window object structure - contains opening information
   and stuff.  Must be first always in a window structure, otherwise
   autoinitialization will fail. */

struct WindowPrefs {
    struct IBox     initialpos;     /* DO NOT MOVE! */
    BOOL            initialopen;    /* DO NOT MOVE! */
};

/*------------------------------------------------------------------*/
/* Saver window */

struct SaveWin {
    struct Window *win;
    Object *Win, *Frq, *Mode, *Format, *Name;
};


/*
    Display prefs window structure.
    The relevant data pointers are kept here.
*/

struct DispPrefsWindow {
    Object          *WO_Win, *GO_ScrMode, *GO_Info, *GO_NColors, *GO_Type,
                    *GO_NColorsI, *GO_Screen, *GO_GetScrMode, *GO_Palette,
                    *GO_PaletteName, *GO_GetPaletteName, *GO_OKRender,
                    *GO_Dither, *GO_OK, *GO_Cancel, *GO_ForceBW,
                    *GO_DrawAlpha;
    struct Window   *win;
    APTR            frame;
    DISPLAY         *tempdisp;
};

/*
    Palette window. This is usually created once for each frame.

    Note that since the Palette object can only be set during
    creation, we will save the window position during openings.
*/
struct PaletteWindow {
    struct Screen   *scr;
    struct Window   *win;
    COLORMAP        *colortable;
    Object          *Win;
    Object          *GradientSlider, *ColorWheel; /* NA */
    Object          *Ok, *Cancel, *Remap;
    Object          *Slider1, *Slider2, *Slider3, *Slider4;
    Object          *Indic1, *Indic2, *Indic3, *Indic4;
    Object          *ColorSpace;
    Object          *Palette;
    struct IBox     windowpos;
};

/*
    External info window
*/
struct ExtInfoWin {
    struct WindowPrefs prefs;
    Object *Win,    *List, *Exec, *Info;
    struct Window   *win;
    ULONG           secs, ms;
    LONG            last;
    ULONG           menuid;
    UBYTE           type;
    struct List     *mylist;
};

/*
    Tool window

    Fields marked not to be moved are due to the initialization scheme
    in main.c
*/

struct ToolWindow {
    struct WindowPrefs prefs;
    Object          *Win;
    Object          *GO_ToolInfo, *GO_ToolBar;
    struct Window   *win;
};

/*
    Select area window
 */

struct SelectWindow {
    struct WindowPrefs prefs;

    Object          *Win;
    struct  Window  *win;

    Object          *Page;

    Object          *TopLeft, *TopRight, *BottomLeft, *BottomRight,
                    *Height, *Width;

    Object          *CircleX, *CircleY, *CircleRadius;
};

/*
    Frames window

    Fields marked not to be moved are due to the initialization scheme
    in main.c
*/

struct FramesWindow {
    struct WindowPrefs prefs;

    Object          *Win;
    struct Window   *win;
    Object          *Frames, *Info;
};

struct PrefsWindow {
    Object          *Win;
    struct Window   *win;
    Object          *VMDir, *PageSize, *ExtStackSize,
                    *MainFont, *ListFont, *DispName, *ColorPreview,
                    *MaxUndo, *FlushLibs, *ExtNiceVal, *ExtNiceValI,
                    *ExtPriority, *ExtPriorityI, *PreviewMode, *Confirm;

    Object          *AvailButtons, *ToToolbar, *FromToolbar,
                    *ToolbarList, *ToolItemType, *ToolItemFile,
                    *ToolItemFileGroup, *DitherPreview;

    Object          *Save, *Use, *Cancel;
};

typedef struct EditWindow_T {
    FRAME               *frame;
    struct Window       *win;
    Object              *Win;
    Object              *ExtList, *ExtName, *ExtValue, *ExtNew, *ExtRemove,
                        *ExtOk;
    UBYTE               title[WINTITLELEN+1];
} EDITWIN;

/*
    File requesters.
 */

struct PPTFileRequester {
    struct WindowPrefs  prefs;
    Object              *Req;
};

/*------------------------------------------------------------------*/
/* Useful macros */

/* A generic button for most of the windows. */

#define GenericButton(l,id)\
    ButtonObject, UScoreLabel(l,'_'),\
        GA_ID, id,\
        XenFrame,\
    EndObject, FixMinHeight

#define GenericDButton(l,id)\
    ButtonObject, UScoreLabel(l,'_'),\
        GA_ID, id,\
        XenFrame,\
        GA_Disabled, TRUE,\
    EndObject, FixMinHeight

#define ULabel(l) LAB_Label,l, LAB_Underscore,'_'

/* Just like an Item(), but disabled */

#define DItem(t,s,i) \
    { NM_ITEM, t, s, NM_ITEMDISABLED, 0, (APTR) i }

#define DSubItem(t,s,i) \
    { NM_SUB, t, s, NM_ITEMDISABLED, 0, (APTR) i }

/* Using these defines causes the MyNewObject() to be called, which
   results in using the local ExtBase variable base. ExtBase MUST be defined!  */

#define MyHGroupObject            MyNewObject( ExtBase,BGUI_GROUP_GADGET
#define MyVGroupObject            MyNewObject( ExtBase,BGUI_GROUP_GADGET,\
                                               GROUP_Style, GRSTYLE_VERTICAL
#define MyButtonObject            MyNewObject( ExtBase,BGUI_BUTTON_GADGET
#define MyToggleObject            MyNewObject( ExtBase,BGUI_BUTTON_GADGET,\
                                                  GA_ToggleSelect, TRUE
#define MyCycleObject             MyNewObject( ExtBase,BGUI_CYCLE_GADGET
#define MyCheckBoxObject          MyNewObject( ExtBase,BGUI_CHECKBOX_GADGET
#define MyInfoObject              MyNewObject( ExtBase,BGUI_INFO_GADGET
#define MyStringObject            MyNewObject( ExtBase,BGUI_STRING_GADGET
#define MyPropObject              MyNewObject( ExtBase,BGUI_PROP_GADGET
#define MyIndicatorObject         MyNewObject( ExtBase,BGUI_INDICATOR_GADGET
#define MyProgressObject          MyNewObject( ExtBase,BGUI_PROGRESS_GADGET
#define MySliderObject            MyNewObject( ExtBase,BGUI_SLIDER_GADGET
#define MyPageObject              MyNewObject( ExtBase,BGUI_PAGE_GADGET
#define MyMxObject                MyNewObject( ExtBase,BGUI_MX_GADGET
#define MyListviewObject          MyNewObject( ExtBase,BGUI_LISTVIEW_GADGET
#define MyExternalObject          MyNewObject( ExtBase,BGUI_EXTERNAL_GADGET, GA_Left, 0, GA_Top, 0, GA_Width, 0, GA_Height, 0
#define MySeparatorObject         MyNewObject( ExtBase,BGUI_SEPARATOR_GADGET

#define MyWindowObject            MyNewObject( ExtBase,BGUI_WINDOW_OBJECT
#define MyFileReqObject           MyNewObject( ExtBase,BGUI_FILEREQ_OBJECT
#define MyCommodityObject         MyNewObject( ExtBase,BGUI_COMMODITY_OBJECT

#define MyGenericButton(l,id)\
    MyButtonObject, UScoreLabel(l,'_'),\
        GA_ID, id,\
        XenFrame,\
    EndObject, FixMinHeight

#define MyGenericDButton(l,id)\
    MyButtonObject, UScoreLabel(l,'_'),\
        GA_ID, id,\
        XenFrame,\
        GA_Disabled, TRUE,\
    EndObject, FixMinHeight

#define GAD(x)                    ( (struct Gadget *) (x) )

/*------------------------------------------------------------------*/
/* Sorta globals. */

extern struct NewMenu PPTMenus[];
extern const char *std_ppt_blurb;
extern const ULONG dpcol_sl2int[];
extern const ULONG dpcol_int2sl[];
extern const ULONG dpcol_sl2ind[];
extern const ULONG dpcol_ind2sl[];
extern const ULONG dpcol_sl2fl[];
extern const ULONG dpcol_fl2sl[];

/*------------------------------------------------------------------*/
/* IDCMP handlers */

#define FROM_MAINWINDOW     0
#define FROM_EFFECTINFO     1
#define FROM_LOADERINFO     2
#define FROM_INFOWINDOW     3
#define FROM_DISPLAYWINDOW  4
#define FROM_PREFSWINDOW    5
#define FROM_DISPPREFSWINDOW 6
#define FROM_PALETTEWINDOW  8
#define FROM_TOOLWINDOW     9
#define FROM_REXXWINDOW    10
#define FROM_FRAMESWINDOW  11
#define FROM_EDITWINDOW    12

/*
 *  IDCMP handler return codes
 */

#define HANDLER_OK          0
#define HANDLER_QUIT        1
#define HANDLER_RESTART     -2
#define HANDLER_DELETED     -1


/*------------------------------------------------------------------*/
/* Gadget ID's */

/* Main Window */

#define MGIDB               (1000)

#define GID_MW_LIST         (MGIDB + 1)
#define GID_MW_LOAD         (MGIDB + 2)
#define GID_MW_SAVE         (MGIDB + 3)
#define GID_MW_QUIT         (MGIDB + 4)
#define GID_MW_INFO         (MGIDB + 5)
#define GID_MW_BREAK        (MGIDB + 6)
#define GID_MW_PROCESS      (MGIDB + 7)
#define GID_MW_ABOUT        (MGIDB + 8)
#define GID_MW_DELETE       (MGIDB + 9)
#define GID_MW_PREFS        (MGIDB + 10)
#define GID_MW_ABORT        (MGIDB + 11)
#define GID_MW_PROGRESS     (MGIDB + 12)
#define GID_MW_STATUS       (MGIDB + 13)
#define GID_MW_INFOL        (MGIDB + 14)
#define GID_MW_INFOF        (MGIDB + 15)



/* Info windows. BUG: Check which ones are obsolete */

#define IGIDB               ( 2000 )


#define GID_IW_CLOSE        (IGIDB + 1)
#define GID_IW_SAVE         (IGIDB + 2)
#define GID_IW_BREAK        (IGIDB + 3)
#define GID_IW_LOAD         (IGIDB + 4)
#define GID_IW_PROCESS      (IGIDB + 5)
#define GID_IW_DISPLAY      (IGIDB + 6)
#define GID_IW_DISPMODE     (IGIDB + 7)
#define GID_IW_DITHER       (IGIDB + 8)
#define GID_IW_DITHER_OFF   (IGIDB + 9)
#define GID_IW_DITHER_ORDERED (IGIDB + 10)
#define GID_IW_DITHER_FS    (IGIDB + 11)
#define GID_IW_DISP_MAIN    (IGIDB + 12)
#define GID_IW_DISP_CUSTOM  (IGIDB + 13)
#define GID_IW_SAVEAS       (IGIDB + 14)
#define GID_IW_DELETE       (IGIDB + 15)
#define GID_IW_UNDO         (IGIDB + 16)
#define GID_IW_CUT          (IGIDB + 17)
#define GID_IW_COPY         (IGIDB + 18)
#define GID_IW_PASTE        (IGIDB + 19)
#define GID_IW_ABOUT        (IGIDB + 20)
#define GID_IW_SELECTALL    (IGIDB + 21)



/* Prefs window */
#define PGIDB               (3000)

#define GID_PW_FONT         (PGIDB + 1)
#define GID_PW_LISTFONT     (PGIDB + 2)
#define GID_PW_VMDIR        (PGIDB + 3)
#define GID_PW_GETVMDIR     (PGIDB + 4)
#define GID_PW_VMBUFSIZE    (PGIDB + 5)
#define GID_PW_GETFONT      (PGIDB + 6)
#define GID_PW_GETLISTFONT  (PGIDB + 7)
#define GID_PW_CANCEL       (PGIDB + 8)
#define GID_PW_SAVE         (PGIDB + 9)
#define GID_PW_USE          (PGIDB + 10)
#define GID_PW_GETDISP      (PGIDB + 11)
#define GID_PW_DISP         (PGIDB + 12)
#define GID_PW_DISPTYPE     (PGIDB + 13)
#define GID_PW_MAXUNDO      (PGIDB + 14)
#define GID_PW_COLORPREVIEW (PGIDB + 15)
#define GID_PW_DISPNAME     (PGIDB + 16)
#define GID_PW_FLUSHLIBS    (PGIDB + 17)
#define GID_PW_ONOPEN       (PGIDB + 18)
#define GID_PW_EXTSTACKSIZE (PGIDB + 19)
#define GID_PW_EXTNICEVAL   (PGIDB + 20)
#define GID_PW_EXTPRIORITY  (PGIDB + 21)
#define GID_PW_PREVIEWMODE  (PGIDB + 22)
#define GID_PW_CONFIRM      (PGIDB + 23)
#define GID_PW_AVAILBUTTONS (PGIDB + 24)
#define GID_PW_TOTOOLBAR    (PGIDB + 25)
#define GID_PW_FROMTOOLBAR  (PGIDB + 26)
#define GID_PW_TOOLBARLIST  (PGIDB + 27)
#define GID_PW_TOOLITEMTYPE (PGIDB + 28)
#define GID_PW_TOOLITEMFILE (PGIDB + 29)
#define GID_PW_GETTOOLITEMFILE (PGIDB + 30)
#define GID_PW_DITHERPREVIEW (PGIDB + 31)

/* Filter window */
#define FGIDB               (4000)

#define GID_FW_EXEC         (FGIDB + 1)
#define GID_FW_CANCEL       (FGIDB + 2)
#define GID_FW_INFO         (FGIDB + 3)
#define GID_FW_LIST         (FGIDB + 4)


/* External info window */

#define XIGIDB              (5000)

#define GID_XI_LIST         (XIGIDB + 1)
#define GID_XI_CONFIG       (XIGIDB + 2)
#define GID_XI_INFO         (XIGIDB + 3)
#define GID_XI_CANCEL       (XIGIDB + 4)
#define GID_XI_CLOSE        GID_XI_CANCEL
#define GID_XI_EXEC         (XIGIDB + 5)

/* Display preferences window */
#define DPGIDB              (6000)

#define GID_DP_CANCEL       (DPGIDB + 1)
#define GID_DP_OK           (DPGIDB + 2)
#define GID_DP_NCOLORS      (DPGIDB + 3)
#define GID_DP_SCRMODE      (DPGIDB + 4)
#define GID_DP_GETSCRMODE   (DPGIDB + 5)
#define GID_DP_INFO         (DPGIDB + 6)
#define GID_DP_TYPE         (DPGIDB + 7)
#define GID_DP_DITHER       (DPGIDB + 8)
#define GID_DP_PALETTE      (DPGIDB + 9)
#define GID_DP_NCOLORSI     (DPGIDB + 10)
#define GID_DP_SCREEN       (DPGIDB + 11)
#define GID_DP_OKRENDER     (DPGIDB + 12)
#define GID_DP_PALETTENAME  (DPGIDB + 13)
#define GID_DP_GETPALETTENAME (DPGIDB + 14)
#define GID_DP_FORCEBW      (DPGIDB + 15)

/* Save window */

#define SWGIDB              (7000)
#define GID_SW_CANCEL       (SWGIDB + 0)
#define GID_SW_SAVE         (SWGIDB + 1)
#define GID_SW_GETFILE      (SWGIDB + 2)
#define GID_SW_SAVERS       (SWGIDB + 3)
#define GID_SW_MODE         (SWGIDB + 4)
#define GID_SW_FILE         (SWGIDB + 5)

/* Display window. Note that some of these are not
   gadget ID's, but method ID's. */

#define DWGIDB              (8000)

#define GID_DW_AREA         (DWGIDB + 0)
#define GID_DW_SELECTDOWN   (DWGIDB + 1)
#define GID_DW_SELECTUP     (DWGIDB + 2)
#define GID_DW_MOUSEMOVE    (DWGIDB + 3)
#define GID_DW_INTUITICKS   (DWGIDB + 4)
#define GID_DW_RIGHTPROP    (DWGIDB + 5)
#define GID_DW_BOTTOMPROP   (DWGIDB + 6)
#define GID_DW_LOCATION     (DWGIDB + 7)  /* Actually a method */
#define GID_DW_HIDE         (DWGIDB + 8)
#define GID_DW_CONTROLSELECTDOWN (DWGIDB+9)

/* The palette window */

#define PALGIDB             (10000)
#define GID_PAL_OK          (PALGIDB + 0)
#define GID_PAL_CANCEL      (PALGIDB + 1)
#define GID_PAL_SLIDER1     (PALGIDB + 2)
#define GID_PAL_SLIDER2     (PALGIDB + 3)
#define GID_PAL_SLIDER3     (PALGIDB + 4)
#define GID_PAL_COLORWHEEL  (PALGIDB + 5)
#define GID_PAL_GRADIENTSLIDER (PALGIDB + 6)
#define GID_PAL_PALETTE     (PALGIDB + 7)
#define GID_PAL_SLIDER4     (PALGIDB + 8)
#define GID_PAL_REMAP       (PALGIDB + 9)

/* The toolbar */

#define TOOLGIDB            (11000)
#define GID_TOOL_PROCESS    (TOOLGIDB + 0)
#define GID_TOOL_LOAD       (TOOLGIDB + 1)
#define GID_TOOL_COORDS     (TOOLGIDB + 2)

/* Info & Edit window */

#define EDITGIDB            (11100)
#define GID_EDIT_OK         (EDITGIDB + 0)
#define GID_EDIT_CANCEL     (EDITGIDB + 1)
#define GID_EDIT_EXTNEW     (EDITGIDB + 2)
#define GID_EDIT_EXTREMOVE  (EDITGIDB + 3)
#define GID_EDIT_EXTVALUE   (EDITGIDB + 4)
#define GID_EDIT_EXTNAME    (EDITGIDB + 5)
#define GID_EDIT_EXTLIST    (EDITGIDB + 6)
#define GID_EDIT_EXTOK      (EDITGIDB + 7)

/* Selection window */

#define SELGIDB             (11200)
#define GID_SELECT_PAGE     (SELGIDB + 0)
#define GID_SELECT_TOPRIGHT (SELGIDB + 1)
#define GID_SELECT_TOPLEFT  (SELGIDB + 2)
#define GID_SELECT_BOTTOMLEFT (SELGIDB + 3)
#define GID_SELECT_BOTTOMRIGHT (SELGIDB + 4)
#define GID_SELECT_WIDTH    (SELGIDB + 5)
#define GID_SELECT_HEIGHT   (SELGIDB + 6)
#define GID_SELECT_CIRCLEX  (SELGIDB + 7)
#define GID_SELECT_CIRCLEY  (SELGIDB + 8)
#define GID_SELECT_CIRCLERADIUS  (SELGIDB + 9)


/*
 *  Global Menus
 */

#define MENUB               (20000)

/* Project Menu */
#define MID_LOADAS          (MENUB + 1)
#define MID_LOADNEW         (MENUB + 2)
#define MID_SAVE            (MENUB + 3)
#define MID_SAVEAS          (MENUB + 4)
#define MID_DELETE          (MENUB + 5)
#define MID_CLOSE           MID_DELETE      /* Since v2.2 */
#define MID_PREFS           (MENUB + 6)
#define MID_MODULEINFO      (MENUB + 7)
#define MID_LOADERS         (MENUB + 8)
#define MID_EFFECTS         (MENUB + 9)
#define MID_ABOUT           (MENUB + 10)
#define MID_QUIT            (MENUB + 11)
#define MID_RENAME          (MENUB + 12)
#define MID_NEW             (MENUB + 13)

/* Edit Menu */
#define MID_UNDO            (MENUB + 101)
#define MID_CUT             (MENUB + 102)
#define MID_COPY            (MENUB + 103)
#define MID_PASTE           (MENUB + 104)
#define MID_SELECT          (MENUB + 105)
#define MID_SELECTALL       (MENUB + 106)
#define MID_PROCESS         (MENUB + 107)
#define MID_BREAK           (MENUB + 108)
#define MID_RENDER          (MENUB + 109)
#define MID_SETRENDER       (MENUB + 110)
#define MID_CUTFRAME        (MENUB + 111)
#define MID_CLOSERENDER     (MENUB + 112)
#define MID_PALETTE         (MENUB + 113)
#define MID_EDITPALETTE     (MENUB + 114)
#define MID_LOADPALETTE     (MENUB + 115)
#define MID_SAVEPALETTE     (MENUB + 116)
#define MID_CROP            (MENUB + 117)
#define MID_ZOOMIN          (MENUB + 118)
#define MID_ZOOMOUT         (MENUB + 119)
#define MID_REMOVEALPHA     (MENUB + 120)
#define MID_CORRECTASPECT   (MENUB + 121)
#define MID_HIDE            (MENUB + 122)
#define MID_EDITINFO        (MENUB + 123)

/* Process Menu */

/* Debug Menu */
#define MID_TESTAR          (MENUB + 900)
#define MID_MEMCHECK        (MENUB + 901)
#define MID_MEMFAIL         (MENUB + 902)
#define MID_MEMFAIL0        (MENUB + 903)
#define MID_MEMFAIL10       (MENUB + 904)
#define MID_MEMFAIL25       (MENUB + 905)
#define MID_MEMFAIL50       (MENUB + 906)
#define MID_SAVEMAINPALETTE (MENUB + 907)


/* Window Menu */
#define MID_TOOLWINDOW      (MENUB + 140)
#define MID_REXXWINDOW      (MENUB + 141)
#define MID_FRAMEWINDOW     (MENUB + 142)
#define MID_SELECTWINDOW    (MENUB + 143)

/*---------------------------------------------------------------------*/
/* AskReq() */

#define GID_AR_OK       1
#define GID_AR_CANCEL   2
#define GID_START       10 /* 0...9 reserved for system gadgets */


#endif /* GUI_H */

