/*
    PROJECT: ppt
    MODULE : ppt.h

    $Revision: 2.5 $
        $Date: 1997/01/17 23:36:28 $
      $Author: jj $

    Main definitions for PPT.

    This file is (C) Janne Jalkanen 1994-1996.

    Please note that those fields marked PRIVATE in structures truly are
    so. So keep your hands off them, because they will probably change between releases.

    !!PRIVATE
    $Id: ppt_real.h,v 2.5 1997/01/17 23:36:28 jj Exp $

    This file contains also the PRIVATE fields in the structs.
    !!PUBLIC
 */

#ifndef PPT_H
#define PPT_H

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef LIBRARIES_BGUI_H
#include <libraries/bgui.h>
#endif

#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif

#ifndef ASKREQ_H
#include "askreq.h"
#endif

/*------------------------------------------------------------------*/
/* Types we will use everywhere */

typedef UBYTE *     ROWPTR;         /* Sample row pointer */
typedef void *      FPTR;           /* Function pointer */
typedef int         PERROR;         /* Error code */
typedef ULONG       ID;             /* Identification code */

typedef struct RGBPixel_T {
    UBYTE r,g,b;
} RGBPixel;

typedef struct ARGBPixel_T {
    UBYTE a,r,g,b;
} ARGBPixel;

typedef struct GrayPixel_T {
    UBYTE g;
} GrayPixel;

/*
 *  These macros can be used to extract an ULONG-cast
 *  ARGB pixels components.
 */

#define ARGB_A(x) (UBYTE)((x) >> 24)
#define ARGB_R(x) (UBYTE)((x) >> 16)
#define ARGB_G(x) (UBYTE)((x) >> 8)
#define ARGB_B(x) (UBYTE)(x)

/* Useful macros and definitions */

#define NEGNUL      ((void *) ~0)
#define POKE(s,v)   ( (*(s)) = (v) )
#define MIN(a,b)    ( ((a) < (b)) ? (a) : (b) )
#define MAX(a,b)    ( ((a) > (b)) ? (a) : (b) )
#define SYSBASE()   (struct Library *)(* ((ULONG *)4L))
#define MULU16(x,y) ( (UWORD)(x) * (UWORD)(y) ) /* To get DCC compile fast code */
#define MULS16(x,y) ( (WORD)(x) * (WORD)(y) )
#define MULUW       MULU16
#define MULSW       MULS16

/* This macro calculates picture size in bytes. Requires pointer to a  pixinfo
   structure.*/
#define PICSIZE(a) \
    (ULONG)( (a)->height * (a)->bytes_per_row )

/* This macro gives the row length. OBSOLETE */
#define ROWLEN(a) \
    ( ((PIXINFO *)(a))->bytes_per_row )

/*!!PRIVATE*/
/* Main window and main screen */

#define MAINWIN       globals->maindisp->win
#define MAINSCR       globals->maindisp->scr


/*------------------------------------------------------------------*/
/*!!PUBLIC*/
/* Definitions */

#define MAXPATHLEN          256     /* Std AmigaDOS path len */
#define NAMELEN             40      /* Maximum length of frame name */
#define MAXPATTERNLEN       80      /* The maximum length for PPTX_PostFixPattern */
/*!!PRIVATE*/
#define WINTITLELEN         80      /* Length of a window title buffer in DISPLAY */
#define SCRTITLELEN         80      /* Length of the screen title buffer in DISPLAY */
#define MAXSCRTITLENAMELEN  40      /* Max length of file name when shown on screen title */

/*!!PUBLIC*/

/*------------------------------------------------------------------*/

/*
    This must be in the beginning of each external module.

    NB:  Does not concern you!  Just make a shared library and be happy.
 */

struct ModuleInfo {
    ULONG           code[2];    /* moveq.l #-1,d0; rts */
    ULONG           id;         /* Identification string. See below.*/
    struct TagItem  *tagarray;
};

/* The ID's for the id-field. */

#define ID_LOADER       MAKE_ID('P','P','T','L')
#define ID_EFFECT       MAKE_ID('P','P','T','F')



/*------------------------------------------------------------------*/

/*!!PRIVATE*/
/*
    A common structure to ease handling of external modules.

    Note that in future this will go away, so you do not need to
    concern yourself with this.
*/

typedef struct {
    struct Node     nd;         /* ln_Type is type of module. See below */
    BPTR            seglist;    /* Actual code */
    struct TagItem  *tags;
    ULONG           usecount;
    BOOL            islibrary;  /* If != 0, this is a newstyle library */
    UBYTE           diskname[NAMELEN+1];/* The real name on disk. */
    UBYTE           realname[NAMELEN+1];/* The name by which this is known */
} EXTERNAL;

/* External types. Also double as Node types. Type is UBYTE  */

#define NT_LOADER           (NT_USER - 0)
#define NT_EFFECT           (NT_USER - 1)
#define NT_SCRIPT           (NT_USER - 2)

/* Values reserved from NT_USER - 2 downwards. */

/*------------------------------------------------------------------*/
/* Loader stuff */



typedef struct {
    EXTERNAL        info;
    ULONG           saveformats;    /* Result of PPTX_SaveFormats query
                                       cached here upon startup. */
    BOOL            canload;        /* Result of PPTX_Load query cached here */
    UBYTE           prefpostfix[NAMELEN+1];
    UBYTE           postfixpat[MAXPATTERNLEN+1];
} LOADER;


/*------------------------------------------------------------------*/
/*  Filters... */


typedef struct {
    EXTERNAL        info;
} EFFECT;



/*------------------------------------------------------------------*/
/*  Scripts...  */

typedef struct {
    EXTERNAL        info;
} SCRIPT;

/*------------------------------------------------------------------*/

/*
    A virtual memory handle. PRIVATE to the bones.
*/

typedef struct {
    BPTR            vm_fh;
    ULONG           vm_id;
    ULONG           begin,end;
    ULONG           last;
    APTR            data;
    BOOL            chflag;
} VMHANDLE;

/*!!PUBLIC*/

/*
    This structure contains information on the picture data itself.
    Your loader should fill out any necessary fields.
*/

typedef struct {
    UWORD           height,     /* Picture height and width */
                    width;

    UBYTE           colorspace; /* See below.  */
    UBYTE           components; /* Amount of components: 3 for fullcolor, 1 for grayscale */
    UBYTE           origdepth;  /* Original depth information */
    UBYTE           bits_per_component; /* Currently 8 */

    UWORD           DPIX, DPIY; /* Dots per inch. Not used currently. */
    UWORD           XAspect,    /* Pixel X and Y aspect ratio */
                    YAspect;

    ULONG           bytes_per_row; /* Amount of bytes / pixel row. */

    ULONG           origmodeid; /* Original display mode id */

    /* The fields beyond this point are PRIVATE! */

    /*!!PRIVATE*/

    VMHANDLE        *vmh;       /* For virtual memory */

    ROWPTR          tmpbuf;     /* This contains a temporary area into which the
                                   image data is copied when processing. */

    /*!!PUBLIC*/
} PIXINFO;

/* Possible color spaces. */

#define CS_UNKNOWN          0   /* Not known at this time */
#define CS_GRAYLEVEL        1   /* 8bit graylevel data */
#define CS_RGB              2   /* 24 bit RGB data */
#define CS_ARGB             4   /* 32 bit ARGB data */

/* Flag definitions for PPTX_ColorSpaces.  Please note that CSF_LUT
   is used ONLY on savers.  PPT processes only in real color, but a color-
   reduced image can be saved. */

#define CSF_NONE            0x00   /* Use this to signify nothing is understood*/
#define CSF_UNKNOWN         0x01   /* Dummy. Not used. */
#define CSF_GRAYLEVEL       0x02
#define CSF_RGB             0x04
#define CSF_LUT             0x08
#define CSF_ARGB            0x10

/*
    The display structure contains necessary info to open a given
    screen/display.

    !!PRIVATE
    NOTE: Do not keep here anything that is not shared between
          instances, as this structure is passed down to the child.

    NB: If you make any modifications, please check MakeFrame() to see
        what should be done with the new fields.
    !!PUBLIC
 */

typedef struct {
    struct Screen   *scr;       /* NULL, if not opened */
    struct Window   *win;       /* Render to this windows rastport. */
    UBYTE           *colortable;/* Colortable in LoadRGB8() format. */
    UWORD           height,width;  /* Sizes. */
    ULONG           dispid;     /* Standard display ID */
    UWORD           ncolors;    /* Number of colors. May be somewhere between 2...depth */
    UBYTE           type;       /* See below. */
    UBYTE           depth;      /* The depth the screen should use */

    /* All fields below this point are PRIVATE */
    /*!!PRIVATE*/

    UBYTE           renderq;    /* See below */
    UBYTE           dither;     /* Dithering method. */
    UBYTE           cmap_method;/* See below */
    Object          *Win, *RenderArea, /*  For quick display windows, */
                    *GO_BottomProp,    /*  ONLY */
                    *GO_RightProp;
    APTR            lock;       /* NULL, if window is not busy */
    char            title[WINTITLELEN];  /* Reserved for window title */
    char            scrtitle[SCRTITLELEN]; /* Reserved for screen title */
    struct Hook     qhook;      /* Quickrender hook for areashow */
    struct Hook     prophook;   /* Dispwindow border gadget hook */
    VMHANDLE        *bitmaphandle;  /* Rendering on disk utilizes this handle */
    UBYTE           palettepath[MAXPATHLEN]; /* Keeps the pointer at the palette */
    BOOL            forcebw;    /* TRUE, if Force Black/White is enabled */

    ULONG           selpt;      /* Draw selection pattern for SetDrPt() */
    /*!!PUBLIC*/
} DISPLAY;

/*!!PRIVATE*/
/* Dithering methods. FS only currently implemented. */

#define DITHER_NONE         0
#define DITHER_ORDERED      1
#define DITHER_FLOYD        2   /* Floyd-Steinberg */

/* Render quality */

#define RENDER_NORMAL       0
#define RENDER_EHB          1
#define RENDER_HAM6         2
#define RENDER_HAM8         3
#define RENDER_QUICK        0xFF

/* Types  */

#define DISPLAY_PPTSCREEN       0
#define DISPLAY_CUSTOMSCREEN    1

/* Palette selection methods */

#define CMAP_MEDIANCUT          0   /* Heckbert Median Cut. Slow but accurate. Default. */
#define CMAP_POPULARITY         1   /* Popularity algorithm. Fast but not accurate */
#define CMAP_FORCEPALETTE       2   /* Forces the colormap named in palettepath  */

/*!!PUBLIC*/

/*!!PRIVATE*/
/*
    This structure contains the necessary information on how to deal with
    information windows.

    Currently PRIVATE!
 */

typedef struct {
    APTR            myframe;
    struct Window   *win;
    Object          *WO_win,
                    *GO_progress,
                    *GO_status,
                    *GO_Break;
    ULONG           id;             /* PRIVATE */
    struct SignalSemaphore lock;
} INFOWIN;

/*!!PUBLIC*/

/*
    The main frame structure holds all necessary information to handle a
    picture.
 */


typedef struct Frame_t {
    struct Node     nd;             /* PRIVATE! */
    char            path[MAXPATHLEN];/* Just the path-part.*/
    char            name[NAMELEN];  /* The name of the frame, as seen in main display */
    struct Process  *currproc;      /* Points to current owning process */
    PIXINFO         *pix;           /* Picture data */
    DISPLAY         *disp;          /* This is the display window */
    struct Rectangle selbox;        /* This contains the area currently selected. */

    /* All data beyond this point is PRIVATE! */
    /*!!PRIVATE*/

    LOADER          *origtype;      /* Points to the loader that originally loaded this frame */
    EXTERNAL        *currext;       /* NULL, if not busy right now. May be ~0, if the external is not yet known */
    ULONG           busy;           /* See below for definitions */
    LONG            busycount;      /* Number of times this item is busy with non-exclusive locks */

    struct SignalSemaphore lock;    /* Used to arbitrate access to this structure */

    ID              ID;             /* Frame ID, an unique code that will last.*/
    struct DispPrefsWindow *dpw;    /* Display prefs window. May be NULL */
    struct PaletteWindow *pw;       /* Palette window. May be NULL */
    INFOWIN         *mywin;         /* This frame's infowindow. NULL, if not yet created */
    APTR            renderobject;   /* see render.h */

    UBYTE           selstatus;      /* see defs.h */
    UBYTE           reqrender;      /* != 0, if a render has been requested but not made */
    BOOL            doerror;        /* TRUE, if the error has not been displayed yet */

    PERROR          errorcode;
    UBYTE           errormsg[ERRBUFLEN];

    struct Frame_t  *lastframe;     /* The last before modifications. Undo frame. */
    struct Frame_t  *parent;        /* If the frame is a fresh copy, then you can find
                                       it's parent here. */
    ULONG           progress_min;   /* Store values for Progress() */
    ULONG           progress_diff;
    ULONG           progress_old;

    ID              alpha;          /* This frames alpha channel frame */

    ID              alphaparents[10]; /* All the frames this frames is an alpha channel to.
                                         End the list with 0L. */

    ULONG           selectmethod;
    APTR            selectdata;
    struct MsgPort  *selectport;    /* Where the messages should be sent */

    struct IBox     zoombox;
    BOOL            zooming;        /* TRUE, if the zoom gadgets were updated by the program,
                                       not the user. BUG: Is there really no other way? */

    BOOL            loading;        /* TRUE, if this frame is currently being loaded */

    ID              attached;       /* Simple attachment list. End with 0L */

    struct EClockVal eclock;

    struct IBox     fixrect;        /* Holds GINP_FIXED_RECT data */

    /*!!PUBLIC*/
} FRAME;

/*!!PRIVATE*/

/*
    Definitions for frame->busy field:
*/

#define BUSY_NOT                0L  /* Frame is not busy */
#define BUSY_READONLY           1L  /* Frame is busy, but can be read.
                                       Non-exclusive in nature. */
/* The following are reserved for the externals. */
#define BUSY_LOADING            2L
#define BUSY_SAVING             3L
#define BUSY_CHANGING           4L  /* An effect is being applied */
#define BUSY_RENDERING          5L
#define BUSY_EDITING            6L

/*!!PUBLIC*/

/*
    This structure contains info about user preferred values. sizeof(PREFS)
    should be divisible by four. This structure is definately read-only,
    but suggested reading if you open your own GUI. Please note especially
    user font preferences.
 */

typedef struct {
    struct TextAttr *mainfont;
    struct TextAttr *listfont;

    /* PRIVATE data only beyond this point */

    /*!!PRIVATE*/

    LONG            extstacksize;   /* Size of stack for externals. */

    char            vmdir[MAXPATHLEN]; /* Where the virtual memory should reside. */
    UWORD           vmbufsiz;       /* In kb. */

    UBYTE           *pubscrname;    /* NULL, if use default public screen or open own. Currently not used. */
    UWORD           maxundo;        /* Maximum amount of undo levels */
    struct TextFont *maintf;        /* These two are the same as the TextAttr - structs */
    struct TextFont *listtf;        /* above, but these contain valid pointers. */

    UWORD           progress_step;
    ULONG           progress_filesize;

    /*
     *  OK, this is not a pretty solution, but here are embedded the
     *  TextAttr - and font name fields.
     */

    struct TextAttr mfont;
    struct TextAttr lfont;

    char            mfontname[NAMELEN];
    char            lfontname[NAMELEN];

    BOOL            colorpreview;   /* TRUE, if the user wants to have a color prview */

    UBYTE           modulepath[MAXPATHLEN]; /* All the modules reside here. */

    BOOL            expungelibs;    /* TRUE, if all modules should be expunged
                                       after use. */
    UBYTE           rexxpath[MAXPATHLEN]; /* All scripts reside here */

    /*!!PUBLIC*/
} PREFS;



/*
    Global variables structure. If you wish to read this structure, you must
    ObtainSemaphoreShared(&phore) first. But don't hold it too long. Writing to
    this structure is reserved for the main program ONLY! Use the support
    functions, if you ever have to meddle with this structure.
 */

typedef struct {
    struct SignalSemaphore phore;   /* Must be locked if you read the struct! */
    DISPLAY         *maindisp;      /* Main display data. */
    PREFS           *userprefs;     /* Currently applicable preferences */
    struct MsgPort  *mport;         /* Main message port for programs to call in. */
    struct List     frames,
                    loaders,
                    effects;

    /* PRIVATE data beyond this point*/
    /*!!PRIVATE*/
    Object          *WO_main;       /* Main window object */
    struct List     tempframes;
    struct Process  *maintask;
    struct List     scripts;
    /*!!PUBLIC*/
} GLOBALS;


/*
    This structure is passed to externals in addition to a pointer to globals.
    It has valid library bases if you wish to use them. These are guaranteed to be
    unique to your task. A pointer to this structure is passed to your external
    in A6. All support functions also expect a pointer to this structure to be
    passed in A6 whenever you call them.

    Thou shalt not modify any data in it!

    lib is a standard library node. Currently only lib.Version field is supported.
    You may use this to check the PPT main version number.
*/

typedef struct ExtBase {
    struct Library  lib;            /* A standard library structure */

/* PPT public fields start here. Read, don't write. */

    GLOBALS         *g;             /* Pointer to PPT global variables */
    struct MsgPort  *mport;         /* Your own message port */
    struct ExecBase *lb_Sys;        /* Exec.library */
    struct IntuitionBase *lb_Intuition;
    struct Library  *lb_Utility;
    struct GfxBase  *lb_Gfx;        /* Graphics.library */
    struct DosLibrary *lb_DOS;
    struct Library  *lb_BGUI;       /* bgui.library */
    struct Library  *lb_GadTools;   /* gadtools.library */

    /*
     *  The following fields are for the AmigaOS localization system,
     *  available from OS 2.1 (V38) onwards. If your system does not
     *  support localization, locales could not be opened or your
     *  external module does not support localization at all, some or
     *  all of these fields may be NULL. Always check before using!
     *
     *  For more information on how the PPT submodule localization works,
     *  see the autodoc for pptsupp/GetStr().
     */

    struct Library  *lb_Locale;     /* locale.library */
    struct Locale   *locale;        /* The current locale. */
    struct Catalog  *extcatalog;    /* This external's own catalog, opened by PPT */

/* PPT private fields start here. Don't even read. */

    /*!!PRIVATE*/
    struct Catalog  *catalog;       /* The PPT catalog */
    BOOL            opened;         /* TRUE, if the libbases were opened. */

    /*!!PUBLIC*/

/* PPT hyper-private experimental fields start here. Don't even think about reading. */

    /*!!PRIVATE*/
    struct Device   *lb_Timer;
    struct timerequest *TimerIO;
    /*!!PUBLIC*/
} EXTBASE;

/*------------------------------------------------------------------------*/
/*
    The locale subsystem. See the autodoc on pptsupp/GetStr()
*/

struct LocaleString {
    LONG    ID;
    STRPTR  Str;
};


/*------------------------------------------------------------------*/
/* Generic constants that might be of use */

/*  The progress is measured in between these values. Give to
    UpdateProgress() a value between these values.
 */

#define MINPROGRESS         0     /* Nothing ready*/
#define MAXPROGRESS         1000  /* 100 % done */


/*------------------------------------------------------------------*/
/* Global tags for external programs. */


#define GTAGBASE                ( TAG_USER + 10000 )

/*
    External info tags. These tags can be found in the external's tagarray
    that describes the external. These are common for both loaders and
    filters.
*/

#define PPTX_Version            ( GTAGBASE + 1 ) /* UWORD */
#define PPTX_Revision           ( GTAGBASE + 2 ) /* UWORD */
#define PPTX_VerStr             ( GTAGBASE + 3 ) /* "$VER: blahblah" */
#define PPTX_Name               ( GTAGBASE + 4 ) /* UBYTE * */
#define PPTX_Init               ( GTAGBASE + 5 ) /* FPTR. OBSOLETE */
#define PPTX_Purge              ( GTAGBASE + 6 ) /* FPTR. OBSOLETE */
#define PPTX_InfoTxt            ( GTAGBASE + 7 ) /* STRPTR */
#define PPTX_Author             ( GTAGBASE + 8 ) /* STRPTR */
#define PPTX_ReqKickVersion     ( GTAGBASE + 9 ) /* UWORD */
#define PPTX_ReqPPTVersion      ( GTAGBASE + 10) /* UWORD */
#define PPTX_ColorSpaces        ( GTAGBASE + 11) /* ULONG */
#define PPTX_RexxTemplate       ( GTAGBASE + 12) /* STRPTR */
#define PPTX_Priority           ( GTAGBASE + 13) /* BYTE */


/*
    Loader specifig tags
 */

#define PPTX_Info               ( GTAGBASE + 100 ) /* FPTR. OBSOLETE */
#define PPTX_Load               ( GTAGBASE + 101 ) /* FPTR/BOOL */
#define PPTX_SaveTrueColor      ( GTAGBASE + 102 ) /* FPTR. OBSOLETE */
#define PPTX_SaveColorMapped    ( GTAGBASE + 103 ) /* FPTR. OBSOLETE */
#define PPTX_Check              ( GTAGBASE + 104 ) /* FPTR. OBSOLETE */
#define PPTX_NoFile             ( GTAGBASE + 105 ) /* BOOL */
#define PPTX_PreferredPostFix   ( GTAGBASE + 106 ) /* STRPTR */
#define PPTX_PostFixPattern     ( GTAGBASE + 107 ) /* STRPTR */

/*
    Effect specific tags
 */

// #define PPTX_Exec               ( GTAGBASE + 200 ) /* FPTR. OBSOLETE */
#define PPTX_EasyExec           ( GTAGBASE + 201 ) /* FPTR */
#define PPTX_EasyTitle          ( GTAGBASE + 202 ) /* FPTR */
#define PPTX_NoNewFrame         ( GTAGBASE + 203 ) /* BOOL */


/*
    These tags can be passed to externals upon execution.
 */

//#define PPTX_ErrMsg             ( GTAGBASE + 1000 ) /* OBSOLETE */
#define PPTX_BitPlanes          ( GTAGBASE + 1001 ) /* UWORD , # of bitplanes to be saved */
#define PPTX_ColorMap8          ( GTAGBASE + 1002 ) /* UBYTE * */
#define PPTX_PlanePtrs          ( GTAGBASE + 1003 ) /* ULONG **, pointer to bitplane data */
//#define PPTX_ErrCode            ( GTAGBASE + 1004 ) /* OBSOLETE! */
#define PPTX_RexxArgs           ( GTAGBASE + 1005 ) /* ULONG * */



/*------------------------------------------------------------------*/
/* Error codes */


#define PERR_OK             0
#define PERR_UNKNOWNTYPE    1 /* ie. Unknown filetype, etc. */
#define PERR_OUTOFMEMORY    2 /* Pretty clear, eh? */
#define PERR_WONTOPEN       3 /* if file refuses to open. OBSOLETE */
#define PERR_INUSE          4 /* Someone is using the object */
#define PERR_INITFAILED     5 /* Cannot initialize our values.
                                 Used (mainly) by L_Init() */
#define PERR_FAILED         6 /* A general failure */
#define PERR_BREAK          7 /* A break signal (CTRL-C) was received */
#define PERR_MISSINGCODE    8 /* If the loader/etc misses code. */
#define PERR_CANCELED       9 /* If the user canceled */
#define PERR_INVALIDARGS   10 /* If user gives invalid arguments to some
                                 lone piece of code. */
#define PERR_WINDOWOPEN    11
#define PERR_FILEOPEN      12
#define PERR_FILEREAD      13
#define PERR_FILEWRITE     14
#define PERR_FILECLOSE     15

#define PERR_WARNING       16 /* If something non-fatal happened */

#define PERR_GENERAL       PERR_FAILED  /* Obsolete */
#define PERR_ERROR         PERR_FAILED  /* Obsolete */

/*------------------------------------------------------------------*/
/* Flags */

#define DFF_COPYDATA        0x01

/*!!PRIVATE*/

/*------------------------------------------------------------------*/
/* Edit commands */

#define EDITCMD_CUT         1
#define EDITCMD_COPY        2
#define EDITCMD_PASTE       3
#define EDITCMD_CUTFRAME    4
#define EDITCMD_CROPFRAME   5

/*!!PUBLIC*/

#ifndef PPTMESSAGE_H
#include "pptmessage.h"
#endif


#endif /* PPT_H */

