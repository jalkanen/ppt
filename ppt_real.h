/*
    PROJECT: ppt
    MODULE : ppt.h

    $Revision: 6.2 $
        $Date: 1999/12/27 21:59:10 $
      $Author: jj $

    Main definitions for PPT.

    This file is (C) Janne Jalkanen 1994-1999.

    Please note that those fields marked PRIVATE in structures truly are
    so. So keep your hands off them, because they will probably change between releases.

    !!PRIVATE
    $Id: ppt_real.h,v 6.2 1999/12/27 21:59:10 jj Exp $

    This file contains also the PRIVATE fields in the structs.
    !!PUBLIC
 */

#ifndef PPT_H
#define PPT_H

#if defined(_POWERPC)
#pragma options align=m68k
#endif

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
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
typedef LONG        PERROR;         /* Error code */
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

typedef void Pixel;                 /* Use only as Pixel * */

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
#define PEEKL(x)    (*( (LONG *) (x) ))
#define MIN(a,b)    ( ((a) < (b)) ? (a) : (b) )
#define MAX(a,b)    ( ((a) > (b)) ? (a) : (b) )
#define SYSBASE()   (struct Library *)(* ((ULONG *)4L))
#define MULU16(x,y) ( (UWORD)(x) * (UWORD)(y) ) /* To get DCC compile fast code */
#define MULS16(x,y) ( (WORD)(x) * (WORD)(y) )
#define MULUW(x,y)  MULU16(x,y)
#define MULSW(x,y)  MULS16(x,y)

/* Clamps x between a and b (inclusive) */

#define CLAMP(x,min,max)        if( (x) < (min) ) { (x) = (min); } \
                                else if( (x) > (max) ) { (x) = (max); }

/* This macro calculates picture size in bytes. Requires pointer to a  pixinfo
   structure.*/
#define PICSIZE(a) \
    (ULONG)( (a)->height * (a)->bytes_per_row )

/* This macro gives the row length. OBSOLETE */
#define ROWLEN(a) \
    ( ((PIXINFO *)(a))->bytes_per_row )

/*------------------------------------------------------------------*/
/* Definitions */

#define MAXPATHLEN          256     /* Std AmigaDOS path len */
#define NAMELEN             40      /* Maximum length of frame name */
#define MAXPATTERNLEN       80      /* The maximum length for PPTX_PostFixPattern */

/*
 *  Maximum image dimensions - current internal limit.
 *  Do not rely on these.  They may grow bigger, but not
 *  smaller.
 */

#define MAX_WIDTH           16383
#define MAX_HEIGHT          16383

/*!!PRIVATE*/
#define WINTITLELEN         80      /* Length of a window title buffer in DISPLAY */
#define SCRTITLELEN        180      /* Length of the screen title buffer in DISPLAY */
#define MAXSCRTITLENAMELEN  40      /* Max length of file name when shown on screen title */
#define MAXPIXELSIZE         4      /* Maximum size of a pizel in bytes */

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

    The main point is to save information that does not
    have to be fetched each time.
*/

typedef struct {
    struct Node     nd;         /* ln_Type is type of module. See below */
    BPTR            seglist;    /* Actual code */
    void            *elfobject; /* ELF object for powerup */
    struct TagItem  *tags;
    ULONG           usecount;
    BOOL            islibrary;  /* If != 0, this is a newstyle library */
    UBYTE           diskname[MAXPATHLEN+NAMELEN+1];/* The real name on disk,
                                                      including path. */
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
    BOOL            chflag; /* TRUE, if the memory has been changed. */
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
    UBYTE           components; /* Amount of components: 4 for ARGB,
                                   3 for fullcolor, 1 for grayscale */
    UBYTE           origdepth;  /* Original depth information. */

    UBYTE           bits_per_component; /* Currently 8 */

    UWORD           DPIX, DPIY; /* Dots per inch.  Use these to signify the
                                   aspect ratio of the image.  If you do not
                                   know the DPI values, use 72 as the base value,
                                   as this is a common guess for an image to
                                   be shown on a computer screen.  It is safe
                                   to put zero in either of the locations (which
                                   signifies 1:1 aspect ratio.) */

    ULONG           private1;   /* Private data. */

    ULONG           bytes_per_row; /* Amount of bytes / pixel row. */

    ULONG           origmodeid; /* Original display mode id */

    /* The fields beyond this point are PRIVATE! */

    /*!!PRIVATE*/

    VMHANDLE        *vmh;       /* For virtual memory */
    ULONG           vm_mode;    /* Virtual memory mode.  See below */

    ROWPTR          tmpbuf;     /* This contains a temporary area into which the
                                   image data is copied when processing. */

    UBYTE           transparentpixel[MAXPIXELSIZE];
    /*!!PUBLIC*/
} PIXINFO;

/* Possible color spaces. */

#define CS_UNKNOWN          0   /* Not known at this time */
#define CS_GRAYLEVEL        1   /* 8bit graylevel data */
#define CS_RGB              2   /* 24 bit RGB data */
#define CS_LUT              3   /* Color lookup table.  DO NOT USE! */
#define CS_ARGB             4   /* 32 bit ARGB data */

/* Flag definitions for PPTX_ColorSpaces.  Please note that CSF_LUT
   is used ONLY on savers.  PPT processes only in real color, but a color-
   reduced image can be saved. */

#define CSF_NONE            0x00   /* Use this to signify nothing is understood*/
#define CSF_UNKNOWN         (1<<CS_UNKNOWN)   /* Dummy. Not used. */
#define CSF_GRAYLEVEL       (1<<CS_GRAYLEVEL)
#define CSF_RGB             (1<<CS_RGB)
#define CSF_LUT             (1<<CS_LUT)
#define CSF_ARGB            (1<<CS_ARGB)


/*
    The display structure contains the data about a rendered image or
    an open display.

    This structure is READ ONLY!  And most of the time, nothing very
    useful is in it, anyway...  Use only, if you really need the information,
    for example in a saver module.

    The colortable contains up to ncolors ARGBPixel_T structures that
    contain the RGB values as well as the Alpha channel value for an image.
    An alpha value of 255 means that this color is fully transparent, a value
    of zero means that it is not transparent at all.

    If the image format supports only one transparent color, then you should
    use the first color (starting from color zero) with a transparency value of 255.

    More than 8 bits-per-gun colortables are not supported, as I really do not
    see the value for using anything else.  In heavy-duty-image processing yes,
    but colormapped images were originally used because the display hardware
    could not cope with the bandwiths required for a full truecolor display and
    are nowadays used just to keep the image size down.

    If you really need 16-bits per gun color, then tell me.

    !!PRIVATE
    NOTE: Do not keep here anything that is not shared between
          instances, as this structure is passed down to the child.

    NB: If you make any modifications, please check MakeFrame() to see
        what should be done with the new fields.
    !!PUBLIC
 */

typedef struct ARGBPixel_T      COLORMAP;

typedef struct {
    struct Screen   *scr;       /* NULL, if not opened */
    struct Window   *win;       /* Render to this windows rastport. */
    COLORMAP        *colortable; /* The colormap. See above. */
    UWORD           height,width;  /* Sizes. */
    ULONG           dispid;     /* Standard display ID */
    ULONG           ncolors;    /* Number of colors. May be somewhere between 2...depth */
    UWORD           type;       /* PRIVATE */
    UWORD           depth;      /* The depth the screen should use */

    UWORD           DPIX, DPIY; /* The DPI values of the display. */

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
    UBYTE           saved_cmap; /* Saves cmap method for Remapping on-the fly */
    struct Rectangle handles;   /* The co-ordinates of the handles on the select
                                   box.  They're updated automatically by
                                   DrawSelectBox() */
    BOOL            drawalpha;  /* TRUE, if alpha channels should be taken into
                                   account when rendering */

    BOOL            keephidden; /* TRUE, if the display should be kept hidden */

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
#define CMAP_FORCEOLDPALETTE    3   /* Forces whatever palette was last loaded */

#define CMAP_NONE             0xFF  /* No cmap method defined.  Used as a marker */

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

extern struct EditWindow_T;

/*
    Contains anything defined for the previews.  This structure is
    embedded into the main frame structure.

*/

struct PreviewFrame {

    /*
     *  Preview stuff.  Previewframe tells if we're currently doing a preview
     *  and previewhook points to a hook structure that is used to render
     *  this preview.
     *
     *  ispreview is true only if this is actually a preview frame.
     *
     *  tempframe is reserved for the modifications: the previewframe
     *  is always copied into the tempframe before any modifications
     *  are made.
     */

    BOOL            pf_IsPreview;
    struct Frame_t  *pf_Frame;
    struct Frame_t  *pf_TempFrame;
    struct Hook     *pf_Hook;

    Object          *pf_RenderArea;
    struct Window   *pf_win;
};

struct MouseLocationMsg {
    WORD            mousex, mousey; /* Mouse coords */
    WORD            xloc, yloc;     /* Image coords */
};

/*
    Contains the data for a selection.

    Point is defined in graphics/gfx.h
 */

struct Selection {
    ULONG           selectmethod;
    UBYTE           selstatus;      /* see defs.h */

    struct MsgPort  *selectport;    /* Where the messages should be sent */

    /*
     *  Hook functions
     */

    VOID            (*ButtonDown)(struct Frame_t *,struct MouseLocationMsg *);
    VOID            (*ControlButtonDown)(struct Frame_t *,struct MouseLocationMsg *);
    VOID            (*ButtonUp)(struct Frame_t *,struct MouseLocationMsg *);
    VOID            (*MouseMove)(struct Frame_t *,struct MouseLocationMsg *);
    VOID            (*DrawSelection)(struct Frame_t *,ULONG);
    VOID            (*EraseSelection)(struct Frame_t *);
    BOOL            (*IsInArea)(struct Frame_t *,WORD,WORD);
    VOID            (*Rescale)(struct Frame_t *, struct Frame_t *);
    VOID            (*Copy)(struct Frame_t *, struct Frame_t *);

    /*
     *  GINP_FIXED_RECT
     */

    struct IBox     fixrect;        /* Holds GINP_FIXED_RECT data */
    WORD            fixoffsetx,     /* Hold the offset of the mouse relative */
                    fixoffsety;     /* to the corner */

    /*
     *  GINP_LASSO_CIRCLE
     */

    WORD            circlex,        /* These hold GINP_LASSO_CIRCLE data */
                    circley;        /* First, the center, then the radius */
    WORD            circleradius;

    ULONG           cachedmethod;   /* Used during Start/StopInput to return
                                       previous method back to life. */

    /*
     *  GINP_LASSO_FREE
     */

    struct Frame_t  *mask;
    Point           *vertices;
    LONG            nVertices;
    LONG            nMaxVertices;   /* Number of allocated vertices */
};

/*!!PUBLIC*/

/*
    The main frame structure holds all necessary information to handle a
    picture.
 */


typedef struct Frame_t {
    struct Node     nd;             /* PRIVATE! */
    char            path[MAXPATHLEN];/* Just the path-part.*/
    char            name[NAMELEN];  /* The name of the frame, as seen in main display */
    struct Process  *currproc;      /* Points to current owning process. */
    PIXINFO         *pix;           /* Picture data.  See above. */
    DISPLAY         *disp;          /* This is the display window. See above. */
    struct Rectangle selbox;        /* This contains the bounding box of the area
                                       currently selected. */

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

#ifdef USE_OLD_ALPHA
    ID              alpha;          /* This frames alpha channel frame */

    ID              alphaparents[10]; /* All the frames this frames is an alpha channel to.
                                         End the list with 0L. */
#endif


    struct IBox     zoombox;
    BOOL            zooming;        /* TRUE, if the zoom gadgets were updated by the program,
                                       not the user. BUG: Is there really no other way? */

    BOOL            loading;        /* TRUE, if this frame is currently being loaded */

    ID              attached;       /* Simple attachment list. End with 0L */

    struct EClockVal eclock;

    struct EditWindow_T *editwin;      /* Info editing window */

    struct PreviewFrame preview;

    struct Selection selection;     /* See above for definition */

    BOOL            istemporary;    /* Only used when the frame is just a temporary
                                       frame and may be released by GetArgs() */

#ifdef DEBUG_MODE
    BPTR            debug_handle;   /* The debug output of this task */
#endif
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

/*
 *  These define the fr_VMemMode field, above
 */

#define VMEM_ALWAYS     0L  /* Always use virtual memory. */
#define VMEM_NEVER      1L  /* Never use virtual memory. vm_fh will be NULL */


/*!!PUBLIC*/

/*
    This structure contains info about user preferred values. sizeof(PREFS)
    should be divisible by four. This structure is definately read-only,
    but suggested reading if you open your own GUI. Please note especially
    user font preferences.
 */

typedef struct {
    /*
     *  NB:  The TextAttr pointers below may very well be NULL, in which
     *       case the user has not set a preference and you should use the
     *       default screen font.
     */

    struct TextAttr *mainfont;
    struct TextAttr *listfont;

    /*
     *  The (recommended) preview height and preview width.  If you are
     *  providing your own previews, then you should observe these.  Try not
     *  to skew the aspect ratio of the image too much.
     *
     *  Do not cache these, as they may be set by the user (or PPT) at any time.
     */

    WORD            previewheight,
                    previewwidth;

    /*
     *  If this is set to TRUE, then the user wants to confirm most of
     *  his requesters.  If FALSE, he does not want to do that, so you
     *  should only show the most important requesters and take the default
     *  action for everything else.  This is really a power-user option.
     */

    BOOL            confirm;

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
    ULONG           previewmode;    /* The preview mode */

    UBYTE           modulepath[MAXPATHLEN]; /* All the modules reside here. */

    BOOL            expungelibs;    /* TRUE, if all modules should be expunged
                                       after use. */
    UBYTE           rexxpath[MAXPATHLEN]; /* All scripts reside here */

    UBYTE           startupdir[MAXPATHLEN];
    UBYTE           startupfile[NAMELEN+1];

    LONG            extniceval;     /* Nice for scheduler */

    LONG            extpriority;    /* Priority for any subprocess */

    BOOL            ditherpreview;  /* Should we apply dithering to the
                                       previews also? */
    BOOL            splash;         /* Show splash screen? */

    LONG            tipnumber;      /* -1 = tips are disabled */

    /*!!PUBLIC*/
} PREFS;

/*!!PRIVATE*/
#define PWMODE_OFF      0L
#define PWMODE_SMALL    1L
#define PWMODE_MEDIUM   2L
#define PWMODE_LARGE    3L
#define PWMODE_HUGE     4L
#define PWMODE_LAST     PWMODE_HUGE
/*!!PUBLIC*/

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
    struct MsgPort  *mport;         /* Main message port for externals to call in. */

    /* PRIVATE data beyond this point*/
    /*!!PRIVATE*/

    struct List     frames,
                    loaders,
                    effects;

    Object          *WO_main;       /* Main window object */
    struct Process  *maintask;
    struct List     scripts;

    /*!!PUBLIC*/
} GLOBALS;


/*
    This structure is the library base for the PPT support functions library.
    It has valid library bases if you wish to use them. These are guaranteed to be
    unique to your task. A pointer to this structure is passed to your external
    in A5. All support functions also expect a pointer to this structure to be
    passed in A6 whenever you call them.

    You don't need to explicitly open this library.

    Thou shalt not modify any data in it!

    lib is a standard library node. Currently only lib.Version field is supported.
    You may use this to check the PPT main version number.
*/

struct PPTBase {
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

    struct Device   *lb_Timer;
    struct timerequest *TimerIO;
    struct Library  *lb_CyberGfx;   /* cybergraphics.library */

    /*!!PUBLIC*/

/* PPT hyper-private experimental fields start here. Don't even think about reading.
   See?  Now the system is in a completely unstable state and may crash any time.
   Your fault! */

    /*!!PRIVATE*/
    Class           *FloatClass;
    struct Library  *lb_PPC;        /* ppc.library */
    /*!!PUBLIC*/
};

/*
 *  Obsolete stuff
 */

typedef struct PPTBase EXTBASE;

/*------------------------------------------------------------------------*/
/*
    The locale subsystem. See the autodoc on pptsupp/GetStr()
*/

struct LocaleString {
    LONG    ID;
    STRPTR  Str;
};

/*!!PRIVATE*/

/*------------------------------------------------------------------*/
/* Generic constants that might be of use */

/*  The progress is measured in between these values. Give to
    UpdateProgress() a value between these values.
 */

#define MINPROGRESS         0     /* Nothing ready*/
#define MAXPROGRESS         1000  /* 100 % done */

/*!!PUBLIC*/

/*------------------------------------------------------------------*/
/* Global tags for external programs. */


#define GTAGBASE                ( TAG_USER + 10000 )

/*
 *  External info tags. These tags can be found in the external's tagarray
 *  that describes the external. These are common for both loaders and
 *  filters.
 *
 *  More information about the different tags can be found in the
 *  file 'tags.doc'
 */

#define PPTX_Version            ( GTAGBASE + 1 ) /* UWORD. OBSOLETE */
#define PPTX_Revision           ( GTAGBASE + 2 ) /* UWORD. OBSOLETE */
#define PPTX_Name               ( GTAGBASE + 4 ) /* UBYTE * */
#define PPTX_CPU                ( GTAGBASE + 5 ) /* UWORD, see execbase.h */
#define PPTX_InfoTxt            ( GTAGBASE + 7 ) /* STRPTR */
#define PPTX_Author             ( GTAGBASE + 8 ) /* STRPTR */
#define PPTX_ReqKickVersion     ( GTAGBASE + 9 ) /* UWORD */
#define PPTX_ReqPPTVersion      ( GTAGBASE + 10) /* UWORD */
#define PPTX_ColorSpaces        ( GTAGBASE + 11) /* ULONG */
#define PPTX_RexxTemplate       ( GTAGBASE + 12) /* STRPTR */
#define PPTX_Priority           ( GTAGBASE + 13) /* BYTE */
#define PPTX_SupportsGetArgs    ( GTAGBASE + 14) /* BOOL */


/*
 *  Loader specifig tags
 */

#define PPTX_Load               ( GTAGBASE + 101 ) /* BOOL */
#define PPTX_NoFile             ( GTAGBASE + 105 ) /* BOOL */
#define PPTX_PreferredPostFix   ( GTAGBASE + 106 ) /* STRPTR */
#define PPTX_PostFixPattern     ( GTAGBASE + 107 ) /* STRPTR */

/*
 *  Effect specific tags
 */

#define PPTX_EasyExec           ( GTAGBASE + 201 ) /* FPTR. OBSOLETE */
#define PPTX_EasyTitle          ( GTAGBASE + 202 ) /* FPTR. OBSOLETE */
#define PPTX_NoNewFrame         ( GTAGBASE + 203 ) /* BOOL */
#define PPTX_NoChangeFrame      ( GTAGBASE + 204 ) /* BOOL (V5) */

/*
 *  These tags can be passed to externals upon execution.
 */

#define PPTX_RexxArgs           ( GTAGBASE + 1005 ) /* ULONG * */
#define PPTX_FileName           ( GTAGBASE + 1006 ) /* STRPTR */
#define PPTX_ArgBuffer          ( GTAGBASE + 1007 ) /* STRPTR (V6) */

/*
 *  The following tags have been reserved for the ObtainPreviewFrame()
 *  calls.
 */

#define PREV_PreviewHook        ( GTAGBASE + 10000 ) /* struct Hook * */

/*------------------------------------------------------------------*/
/* Error codes.  You should note that if you use these error
   codes in SetErrorCode() you'll get a text box describing the
   error in a vaguely general way.  For example, if you return
   PERR_OUTOFMEMORY, PPT will display a requester stating that
   your module just ran out of memory. */


#define PERR_OK             0
#define PERR_UNKNOWNTYPE    1 /* ie. Unknown filetype, etc. */
#define PERR_OUTOFMEMORY    2 /* Pretty clear, eh? */
#define PERR_WONTOPEN       3 /* if file refuses to open. OBSOLETE */
#define PERR_INUSE          4 /* Someone is using the object */
#define PERR_INITFAILED     5 /* Cannot initialize.  Could mean anything. */
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

#define PERR_NOTINIMAGE    17 /* An out-of-bounds error happened when
                                 indexing an image */
#define PERR_UNKNOWNCOLORSPACE 18 /* An unsupported colorspace was met */

/*!!PRIVATE*/
#define PERR_UNKNOWNCPU    1000 /* This CPU is not supported */

/*!!PUBLIC*/

/*------------------------------------------------------------------*/
/* Flags.

   First, the DFF_* flags are for the DupFrame() function.  See
   the autodoc for more information.

*/

#define DFF_COPYDATA        (1<<0)
#define DFF_MAKENEWNAME     (1<<1)

/*
 *  Flags for the CopyFrameData() function.
 */

#define CFDF_SHOWPROGRESS   (1<<0)

/* This is completely unofficial and should only be used within PPT */
#define AFF_PPC             (1<<8)

/*------------------------------------------------------------------*/
/* Extensions.  This also contains the pre-defined names for
   AddExtension().  For more explanation, see the
   AddExtension() autodoc. */

struct Extension {
    struct Node         en_Node;    /* PRIVATE */
    STRPTR              en_Name;
    ULONG               en_Length;
    ID                  en_FrameID;
    ULONG               en_Flags;
    APTR                en_Data;

    /* Nothing but private data down from here */
};

/*
   Pre-defined names.  Loaders may or may not understand these,
   but these will be supported.  The meanings are as explained
   in the AddExtension() autodoc.
*/

#define EXTNAME_AUTHOR      "Author"
#define EXTNAME_ANNO        "Annotation"
#define EXTNAME_DATE        "Date"

/* Flags for en_Flags.  As usual, all unused bits should be set
   to 0.  Note that some of the flags are mutually exclusive,
   as a datatype cannot be a text string or a value at the same
   time. */

#define EXTF_PRIVATE        0x00 /* Private type. PPT will not attempt
                                    to parse this. */
#define EXTF_CSTRING        0x01 /* This extension is a standard C
                                    string with ASCII codeset */
#define EXTF_LONG           0x02 /* The value is a signed long */
#define EXTF_FLOAT          0x03 /* The value is an IEEE floating point
                                    number */
/*!!PRIVATE*/

/*------------------------------------------------------------------*/
/* Edit commands */

#define EDITCMD_CUT         1
#define EDITCMD_COPY        2
#define EDITCMD_PASTE       3
#define EDITCMD_CUTFRAME    4
#define EDITCMD_CROPFRAME   5

/*------------------------------------------------------------------*/
/* Return values for GetOptID() */

#define GOID_COMMENT        -1
#define GOID_UNKNOWN        -2

/*!!PUBLIC*/

#ifndef PPTMESSAGE_H
#include "pptmessage.h"
#endif


#endif /* PPT_H */

