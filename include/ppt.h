/*
    PROJECT: ppt
    MODULE : ppt.h

    $Revision: 6.0 $
        $Date: 1999/09/05 02:19:28 $
      $Author: jj $

    Main definitions for PPT.

    This file is (C) Janne Jalkanen 1994-1999.

    Please note that those fields marked PRIVATE in structures truly are
    so. So keep your hands off them, because they will probably change between releases.

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
} DISPLAY;



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
} FRAME;


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
    struct MsgPort  *mport;         /* Main message port for externals to call in. */

    /* PRIVATE data beyond this point*/
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


/* PPT hyper-private experimental fields start here. Don't even think about reading.
   See?  Now the system is in a completely unstable state and may crash any time.
   Your fault! */

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

#ifndef PPTMESSAGE_H
#include "pptmessage.h"
#endif


#endif /* PPT_H */

