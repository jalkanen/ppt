
#include <pptplugin.h>

#include <libraries/iffparse.h>
#include <proto/iffparse.h>

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef GRAPHICS_VIDEOCONTROL_H
#include <graphics/videocontrol.h>
#endif


#define MYNAME          "ILBM"
#define VERSION         1
#define REVISION        0
#define VERSTR          "1.0"

#define VERSIONSTRING   "$VER: "MYNAME".loader V. "VERSTR" ("__DATE__")"

#define ID_ILBM         MAKE_ID('I','L','B','M')

#define ID_BMHD         MAKE_ID('B','M','H','D')
#define ID_CMAP         MAKE_ID('C','M','A','P')
#define ID_CRNG         MAKE_ID('C','R','N','G')
#define ID_CCRT         MAKE_ID('C','C','R','T')
#define ID_GRAB         MAKE_ID('G','R','A','B')
#define ID_SPRT         MAKE_ID('S','P','R','T')
#define ID_DEST         MAKE_ID('D','E','S','T')
#define ID_CAMG         MAKE_ID('C','A','M','G')
#define ID_BODY         MAKE_ID('B','O','D','Y')
#define ID_ANNO         MAKE_ID('A','N','N','O')
#define ID_AUTH         MAKE_ID('A','U','T','H')



#define BODY_BUFSIZE    4096


/* Internal errors */
#define ERRBASE                 (100)

#define UNKNOWN_COMPRESSION     (ERRBASE + 1)
#define MASKING_NOT_SUPPORTED   (ERRBASE + 2)
#define CANNOT_READ             (ERRBASE + 3)
#define FORMAT_NOT_SUPPORTED    (ERRBASE + 4)
#define PARSEIFF_FAILED         (ERRBASE + 5)
#define SETUP_FAILED            (ERRBASE + 6)
#define IFFPARSE_DID_NOT_OPEN   (ERRBASE + 7)
#define NO_IFF_HANDLE           (ERRBASE + 8)
#define NOT_ILBM                (ERRBASE + 10)

/*
    From IFF specs.
*/

struct BitMapHeader {
    UWORD   w, h;           /* Width, height in pixels */
    WORD    x, y;           /* x, y position for this bitmap  */
    UBYTE   nPlanes;        /* # of planes (not including mask) */
    UBYTE   masking;        /* a masking technique listed above */
    UBYTE   compression;    /* cmpNone or cmpByteRun1 */
    UBYTE   reserved1;      /* must be zero for now */
    UWORD   transparentColor;
    UBYTE   xAspect, yAspect;
    WORD    pageWidth, pageHeight;
};

typedef struct {
    UBYTE red, green, blue;   /* MUST be UBYTEs so ">> 4" won't sign extend.*/
} ColorRegister;


/* Use this constant instead of sizeof(ColorRegister). */
#define sizeofColorRegister  3

typedef WORD Color4;   /* Amiga RAM version of a color-register,
          * with 4 bits each RGB in low 12 bits.*/

/* Maximum number of bitplanes storable in BitMap structure */
#define MAXAMDEPTH 8

/* Use ViewPort->ColorMap.Count instead
#define MAXAMCOLORREG 32
*/

/* Maximum planes we can save */
#define MAXSAVEDEPTH 24

/* Convert image width to even number of BytesPerRow for ILBM save.
 * Do NOT use this macro to determine the actual number of bytes per row
 * in an Amiga BitMap scan line.  For that, use BitMap->BytesPerRow.
 */
#define BytesPerRow(w)  ((w) + 15 >> 4 << 1)
#define BitsPerRow(w)   ((w) + 15 >> 4 << 4)

/* Flags that should be masked out of old 16-bit CAMG before save or use.
 * Note that 32-bit mode id (non-zero high word) bits should not be twiddled
 */
#define BADFLAGS  (SPRITES|VP_HIDE|GENLOCK_AUDIO|GENLOCK_VIDEO)
#define OLDCAMGMASK  (~BADFLAGS)


/*  Masking techniques  */
#define mskNone                 0
#define mskHasMask              1
#define mskHasTransparentColor  2
#define mskLasso                3

/*  Compression techniques  */
#define cmpNone                 0
#define cmpByteRun1             1

#define RowBytes(w)     ((((w) + 15) >> 4) << 1)


#define ChunkMoreBytes(cn)      (cn->cn_Size - cn->cn_Scan)
/* This macro computes the worst case packed size of a "row" of bytes. */
#define MaxPackedSize(rowSize)  ( (rowSize) + ( ((rowSize)+127) >> 7 ) )


/* Protos */

extern void SetUp( FRAME *, EXTBASE *, struct BitMapHeader * );
extern UBYTE *SetUpColors( EXTBASE *, struct StoredProperty *, ULONG );
extern void SetUpDisplayMode( struct StoredProperty *, FRAME * );
extern int  DecodeBody( struct IFFHandle *, FRAME *, UBYTE *,
                        struct BitMapHeader *, EXTBASE *, struct Library * );

extern BOOL unpackrow(BYTE **pSource, BYTE **pDest, WORD srcBytes0, WORD dstBytes0);
extern LONG packrow(BYTE **, BYTE **, LONG);
extern void InitBitMapHeader( struct BitMapHeader *bmhd, FRAME *frame, UBYTE, EXTBASE *ExtBase );
extern int WriteBody( struct Library *, struct IFFHandle *, FRAME *, EXTBASE * );
extern int WriteBMBody( struct Library *, struct IFFHandle *, FRAME *, EXTBASE * );


/* ---------- Point2D --------------------------------------------------*/
/* A Point2D is stored in a GRAB chunk. */
typedef struct {
    WORD x, y;      /* coordinates (pixels) */
    } Point2D;

/* ---------- DestMerge ------------------------------------------------*/
/* A DestMerge is stored in a DEST chunk. */
typedef struct {
    UBYTE depth;   /* # bitplanes in the original source */
    UBYTE pad1;      /* UNUSED; for consistency store 0 here */
    UWORD planePick;   /* how to scatter source bitplanes into destination */
    UWORD planeOnOff;   /* default bitplane data for planePick */
    UWORD planeMask;   /* selects which bitplanes to store into */
    } DestMerge;

/* ---------- SpritePrecedence -----------------------------------------*/
/* A SpritePrecedence is stored in a SPRT chunk. */
typedef UWORD SpritePrecedence;

/* ---------- Camg Amiga Viewport Mode Display ID ----------------------*/
/* The CAMG chunk is used to store the Amiga display mode in which
 * an ILBM is meant to be displayed.  This is very important, especially
 * for special display modes such as HAM and HALFBRITE where the
 * pixels are interpreted differently.
 * Under V37 and higher, store a 32-bit Amiga DisplayID (aka. ModeID)
 * in the ULONG ViewModes CAMG variable (from GetVPModeID(viewport)).
 * Pre-V37, instead store the 16-bit viewport->Modes.
 * See the current IFF manual for information on screening for bad CAMG
 * chunks when interpreting a CAMG as a 32-bit DisplayID or 16-bit ViewMode.
 * The chunk's content is declared as a ULONG.
 */
typedef struct {
   ULONG ViewModes;
   } CamgChunk;

/* ---------- CRange cycling chunk -------------------------------------*/
#define RNG_NORATE  36   /* Dpaint uses this rate to mean non-active */
/* A CRange is store in a CRNG chunk. */
typedef struct {
    WORD  pad1;              /* reserved for future use; store 0 here */
    WORD  rate;      /* 60/sec=16384, 30/sec=8192, 1/sec=16384/60=273 */
    WORD  active;     /* bit0 set = active, bit 1 set = reverse */
    UBYTE low, high;   /* lower and upper color registers selected */
    } CRange;

/* ---------- Ccrt (Graphicraft) cycling chunk -------------------------*/
/* A Ccrt is stored in a CCRT chunk. */
typedef struct {
   WORD  direction;  /* 0=don't cycle, 1=forward, -1=backwards */
   UBYTE start;      /* range lower */
   UBYTE end;        /* range upper */
   LONG  seconds;    /* seconds between cycling */
   LONG  microseconds; /* msecs between cycling */
   WORD  pad;        /* future exp - store 0 here */
   } CcrtChunk;

/* If you are writing all of your chunks by hand,
 * you can use these macros for these simple chunks.
 */
#define putbmhd(iff, bmHdr)  \
    PutCk(iff, ID_BMHD, sizeof(BitMapHeader), (BYTE *)bmHdr)
#define putgrab(iff, point2D)  \
    PutCk(iff, ID_GRAB, sizeof(Point2D), (BYTE *)point2D)
#define putdest(iff, destMerge)  \
    PutCk(iff, ID_DEST, sizeof(DestMerge), (BYTE *)destMerge)
#define putsprt(iff, spritePrec)  \
    PutCk(iff, ID_SPRT, sizeof(SpritePrecedence), (BYTE *)spritePrec)
#define putcamg(iff, camg)  \
    PutCk(iff, ID_CAMG, sizeof(CamgChunk),(BYTE *)camg)
#define putcrng(iff, crng)  \
    PutCk(iff, ID_CRNG, sizeof(CRange),(BYTE *)crng)
#define putccrt(iff, ccrt)  \
    PutCk(iff, ID_CCRT, sizeof(CcrtChunk),(BYTE *)ccrt)



struct ILBMInfo {
        /* general parse.c related */
//        struct  ParseInfo ParseInfo;

        /* The following variables are for
         * programs using the ILBM-related modules.
         * They may be removed or replaced for
         * programs parsing other forms.
         */
        /* ILBM */
        struct BitMapHeader bmhd;              /* filled in by load and save ops */
        ULONG   camg;                   /* filled in by load and save ops */
        Color4  *colortable;            /* allocated by getcolors */
        ULONG   ctabsize;               /* size of colortable in bytes */
        USHORT  ncolors;                /* number of color registers loaded */
        USHORT  Reserved1;

        /* for getbitmap.c */
        struct BitMap *brbitmap;        /* for loaded brushes only */

        /* for screen.c */
        struct Screen *scr;             /* screen of loaded display   */
        struct Window *win;             /* window of loaded display   */
        struct ViewPort *vp;            /* viewport of loaded display */
        struct RastPort *srp;           /* screen's rastport */
        struct RastPort *wrp;           /* window's rastport */
        BOOL TBState;                   /* state of titlebar hiddenness */

        /* caller preferences */
        struct NewWindow *windef;       /* definition for window */
        UBYTE *stitle;          /* screen title */
        LONG stype;             /* additional screen types */
        WORD ucliptype;         /* overscan display clip type */
        BOOL EHB;               /* default to EHB for 6-plane/NoCAMG */
        BOOL Video;             /* Max Video Display Clip (non-adjustable) */
        BOOL Autoscroll;        /* Enable Autoscroll of screens */
        BOOL Notransb;          /* Borders not transparent to genlock */
        ULONG *stags;           /* Additional screen tags for 2.0 screens  */
        ULONG IFFPFlags;        /* For CBM-designated use by IFFP modules  */
        APTR  *IFFPData;        /* For CBM-designated use by IFFP modules  */
        ULONG UserFlags;        /* For use by applications for any purpose */
        APTR  *UserData;        /* For use by applications for any purpose */
        ULONG Reserved[3];      /* must be 0 for now */

        /* Application-specific variables may go here */
        };

/* referenced by modules */


