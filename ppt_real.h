/*
    PROJECT: ppt
    MODULE : ppt.h

    $Id: ppt_real.h,v 1.3 1995/08/20 18:35:43 jj Exp $

    Main definitions for PPT.

    This file is (C) Janne Jalkanen 1994.

    Please note that those fields marked PRIVATE in structures truly are
    so. So keep your hands off them, because they will probably change between releases.

    This file contains also the PRIVATE fields in the structs.
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
#include <bgui.h>
#endif


/*------------------------------------------------------------------*/
/* Types we will use everywhere */

typedef UBYTE *     ROWPTR;         /* Sample row pointer */
typedef void *      FPTR;           /* Function pointer */
typedef int         PERROR;         /* Error code */


/* Useful macros and definitions */

#define NEGNUL      ((void *) ~0)
#define POKE(s,v)   ( (*(s)) = (v) )
#define MIN(a,b)    ( ((a) < (b)) ? (a) : (b) )
#define MAX(a,b)    ( ((a) > (b)) ? (a) : (b) )
#define SYSBASE()   (APTR)(* ((ULONG *)4L))
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

/* These three macros exist for simplicity. They are used
   for locking the global base. BUG: should really not be here */

#define LOCKGLOB()    ObtainSemaphore( &globals->phore )
#define SHLOCKGLOB()  ObtainSemaphoreShared( &globals->phore )
#define UNLOCKGLOB()  ReleaseSemaphore( &globals->phore )

/* Main window and main screen */

#define MAINWIN       globals->maindisp->win
#define MAINSCR       globals->maindisp->scr


/*------------------------------------------------------------------*/

#define MAXPATHLEN          256     /* Std AmigaDOS path len */
#define NAMELEN             40      /* Maximum length of frame name */
#define WINTITLELEN         80      /* Length of a window title buffer in DISPLAY */
#define SCRTITLELEN         80      /* Length of the screen title buffer in DISPLAY */
#define MAXSCRTITLENAMELEN  40      /* Max length of file name when shown on screen title */

/*------------------------------------------------------------------*/

/*
    This must be in the beginning of each external module
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


/*
    A common structure to ease handling of external modules.
*/

typedef struct {
    struct Node     nd;         /* ln_Type is type of module. See below */
    BPTR            seglist;    /* Actual code */
    struct TagItem  *tags;
    ULONG           usecount;
} EXTERNAL;


/* External types. Also double as Node types. Type is UBYTE  */

#define NT_LOADER           (NT_USER - 0)
#define NT_EFFECT           (NT_USER - 1)

/* Values reserved from NT_USER - 2 downwards. */


/*------------------------------------------------------------------*/
/* Loader stuff */



typedef struct {
    EXTERNAL        info;
} LOADER;



/*------------------------------------------------------------------*/
/*  Filters... */


typedef struct {
    EXTERNAL        info;
} EFFECT;


#define FTAGBASE         ( TAG_USER + 0x20000 )


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
    UBYTE           reserved0;

    UWORD           DPIX, DPIY; /* Dots per inch. Not used currently. */
    UWORD           XAspect,    /* Pixel X and Y aspect ratio */
                    YAspect;

    ULONG           bytes_per_row; /* Amount of bytes / pixel row. */

    /* The fields beyond this point are PRIVATE! */

    VMHANDLE        *vmh;

} PIXINFO;

/* Possible color spaces. */

#define CS_UNKNOWN          0   /* Not known at this time */
#define CS_GRAYLEVEL        1   /* 8bit graylevel data */
#define CS_RGB              2   /* 24bit RGB data */

/* Flag definitions for PPTX_ColorSpaces */

#define CSF_UNKNOWN         0x01   /* Dummy. Not used. */
#define CSF_GRAYLEVEL       0x02
#define CSF_RGB             0x04


/*
    The display structure contains necessary info to open a given
    screen/display.
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

    UBYTE           renderq;    /* See below */
    UBYTE           dither;     /* Dithering method. */
    UBYTE           cmap_method;/* See below */
    Object          *Win, *RenderArea; /* For quick display windows, ONLY! */
    APTR            *visualinfo;/* Visualinfo pointer for this screen */
    struct Menu     *menustrip; /* GadTools menustrip for the real window */
    APTR            lock;       /* NULL, if window is not busy */
    char            title[WINTITLELEN];  /* Reserved for window title */
    char            scrtitle[SCRTITLELEN]; /* Reserved for screen title */
    struct Hook     qhook;      /* Quickrender hook for areashow */
    VMHANDLE        *bitmaphandle;  /* Rendering on disk utilizes this handle */
} DISPLAY;

/* Dithering methods. FS only currently implemented. */

#define DITHER_NONE         0
#define DITHER_ORDERED      1
#define DITHER_FLOYD        2   /* Floyd-Steinberg */

/* Render quality */

#define RENDER_QUICK        0
#define RENDER_NORMAL       1

/* Types  */

#define DISPLAY_PPTSCREEN       0
#define DISPLAY_CUSTOMSCREEN    1

/* Palette selection methods */

#define CMAP_MEDIANCUT          0   /* Heckbert Median Cut. Slow but accurate. Default. */
#define CMAP_POPULARITY         1   /* Popularity algorithm. Fast but not accurate */
#define CMAP_FORCEPALETTE       2   /* Forces the colormap currently in disp->colortable */



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
} INFOWIN;



/*
    The main frame structure holds all necessary information to handle a
    picture.
 */


typedef struct Frame_t {
    struct Node     nd;             /* PRIVATE! */
    char            fullname[MAXPATHLEN]; /* Complete name, including path etc.*/
    char            name[NAMELEN];  /* The name of the frame, as seen in main display */
    LOADER          *origtype;      /* Points to the loader that originally loaded this frame */
    EXTERNAL        *currext;       /* NULL, if not busy right now. May be ~0, if the external is not yet known */
    struct Process  *currproc;      /* Points to current owning process */
    PIXINFO         *pix;           /* Picture data */
    DISPLAY         *disp;          /* This is the display window */
    struct Rectangle selbox;        /* This contains the area currently selected. */

    /* All data beyond this point is PRIVATE! */

    ULONG           busy;           /* See below for definitions */
    ULONG           ID;             /* Frame ID, an unique code that will last.*/
    struct DispPrefsWindow *dpw;    /* Display prefs window. May be NULL */
    INFOWIN         *mywin;         /* This frame's infowindow. NULL, if not yet created */
    UBYTE           selstatus;      /* 0 if not visible, 1 if visible */
    UBYTE           reserved[3];    /* Unused */
    struct Frame_t  *lastframe;     /* The last before modifications. Undo frame. */
    struct Frame_t  *parent;        /* If the frame is a fresh copy, then you can find
                                       it's parent here. */
    ULONG           progress_min;   /* Store values for Progress() */
    ULONG           progress_diff;
    ULONG           progress_old;
    PERROR          lasterror;      /* internal functions will set this. */
} FRAME;

/*
    Definitions for frame->busy field:
*/

#define BUSY_NOT                0L  /* Frame is not busy */
#define BUSY_READONLY           1L  /* Frame is busy, but can be read. */
/* The following are reserved for the externals. */
#define BUSY_LOADING            2L
#define BUSY_SAVING             3L
#define BUSY_CHANGING           4L  /* An effect is being applied */
#define BUSY_RENDERING          5L
#define BUSY_EDITING            6L


/*
    This structure contains info about user preferred values. sizeof(PREFS)
    should be divisible by four. This structure is definately read-only,
    but suggested reading if you open your own GUI. Please note especially
    user font preferences.
 */

typedef struct {
    struct TextAttr *mainfont;
    struct TextAttr *listfont;
    char            vmdir[MAXPATHLEN]; /* Where the virtual memory should reside. */
    UWORD           vmbufsiz;       /* In kb. */

    /* PRIVATE data only beyond this point */

    UBYTE           *pubscrname;    /* NULL, if use default public screen or open own. Currently not used. */
    BOOL            iwonload;
    BOOL            disponload;
    UWORD           maxundo;        /* Maximum amount of undo levels */
} PREFS;



/*
    Global variables structure. If you wish to read this structure, you must
    ObtainSemaphoreShared(&phore) first. But don't hold it too long. Writing to
    this structure is reserved for the main program ONLY! Use the support
    functions, if you ever have to meddle with this structure.
 */

typedef struct {
    struct SignalSemaphore phore;   /* Must be locked if you read the struct! */
    DISPLAY         *maindisp;
    PREFS           *userprefs;
    struct MsgPort  *mport;         /* Main message port for programs to call in. */
    struct List     frames,
                    loaders,
                    effects;

    /* PRIVATE data beyond this point*/

    Object          *WO_main;       /* Main window object */
    Object          *LV_frames;
    struct List     tempframes;
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

typedef struct {
    struct Library  lib;            /* A standard library structure */

/* PPT public fields start here. Read, don't write. */

    GLOBALS         *g;             /* Pointer to PPT global variables */
    struct MsgPort  *mport;         /* Your own message port */
    struct Library  *lb_Sys;        /* Exec.library */
    struct Library  *lb_Intuition;
    struct Library  *lb_Utility;
    struct Library  *lb_Gfx;        /* Graphics.library */
    struct Library  *lb_DOS;
    struct Library  *lb_BGUI;       /* bgui.library */
    struct Library  *lb_GadTools;   /* gadtools.library */

/* PPT private fields start here. Don't even read. */

    BOOL            opened;

/* PPT hyper-private experimental fields start here. Don't even think about reading. */

} EXTBASE;

/* For old source code compatability (V0) */

#define EXTDATA     EXTBASE

/*------------------------------------------------------------------------*/
/*
    Here are the declarations for PPT GUI builder.
*/


#define AR_Dummy            (TAG_USER+0x8000000)
#define AR_Title            (AR_Dummy + 1)  /* STRPTR */
#define AR_Text             (AR_Dummy + 2)  /* STRPTR */
#define AR_Positive         (AR_Dummy + 3)  /* STRPTR */
#define AR_Negative         (AR_Dummy + 4)  /* STRPTR */
#define AR_Dimensions       (AR_Dummy + 5)  /* struct IBox * */

/* Values from AR_Dummy + 6 to AR_Dummy + 99 reserved */

#define AR_ObjectMin        (AR_Dummy + 100)    /* PRIVATE */
#define AR_SliderObject     (AR_Dummy + 100)    /* struct TagItem * */
#define AR_StringObject     (AR_Dummy + 101)    /* struct TagItem * */
#define AR_ToggleObject     (AR_Dummy + 102)    /* struct TagItem * */
#define AR_ObjectMax        (AR_Dummy + 499)

/*
 *  Generic tags for objects
 */

#define AROBJ_Value         (AR_Dummy + 500)    /* APTR */
#define AROBJ_Label         (AR_Dummy + 501)    /* STRPTR */

/* Slider value objects tag values. */

#define ARSLIDER_Min        (AR_Dummy + 1000)   /* LONG */
#define ARSLIDER_Max        (AR_Dummy + 1001)   /* LONG */
#define ARSLIDER_Default    (AR_Dummy + 1002)   /* LONG */


#define MAX_AROBJECTS 30 /* Maximum amount of objects you may create */


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
#define PPTX_Init               ( GTAGBASE + 5 ) /* FPTR */
#define PPTX_Purge              ( GTAGBASE + 6 ) /* FPTR */
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

#define PPTX_Info               ( GTAGBASE + 100 ) /* FPTR */
#define PPTX_Load               ( GTAGBASE + 101 ) /* FPTR */
#define PPTX_SaveTrueColor      ( GTAGBASE + 102 ) /* FPTR */
#define PPTX_SaveColorMapped    ( GTAGBASE + 103 ) /* FPTR */
#define PPTX_Check              ( GTAGBASE + 104 ) /* FPTR */



/*
    Effect specific tags
 */

#define PPTX_Exec               ( GTAGBASE + 200 ) /* FPTR */
#define PPTX_EasyExec           ( GTAGBASE + 201 ) /* FPTR */
#define PPTX_EasyTitle          ( GTAGBASE + 202 ) /* FPTR */
#define PPTX_NoNewFrame         ( GTAGBASE + 203 ) /* BOOL */


/*
    These tags can be passed to externals upon execution.
 */

#define PPTX_ErrMsg             ( GTAGBASE + 1000 ) /* STRPTR */
#define PPTX_BitPlanes          ( GTAGBASE + 1001 ) /* UWORD , # of bitplanes to be saved */
#define PPTX_ColorMap8          ( GTAGBASE + 1002 ) /* UBYTE * */
#define PPTX_PlanePtrs          ( GTAGBASE + 1003 ) /* ULONG **, pointer to bitplane data */
#define PPTX_ErrCode            ( GTAGBASE + 1004 ) /* ULONG, alternate to ErrMsg */
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
#define PERR_GENERAL        6 /* A pretty general error message */
#define PERR_ERROR          PERR_GENERAL
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

/*------------------------------------------------------------------*/
/* Flags */

#define DFF_COPYDATA        0x01


/*------------------------------------------------------------------*/
/* Edit commands */

#define EDITCMD_CUT         1
#define EDITCMD_COPY        2
#define EDITCMD_PASTE       3
#define EDITCMD_CUTFRAME    4

/*------------------------------------------------------------------*/

#endif /* PPT_H */

