/*
    PROJECT: ppt
    MODULE:  bmp i/o module

    Loader/saver for Windows BMP images.  PPT and this file are (C) Janne Jalkanen 1999.

    $Id: bmp.c,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
*/

#undef DEBUG_MODE

#include <pptplugin.h>

#include "bitio.h"

#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "BMP"

/*
 * Classes of BMP files
 */

#define C_WIN   0
#define C_OS2   1

typedef unsigned char pixval;

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Windows and OS/2 BMP saving and loading.\n"
    "Contains code (C) 1992 David W. Sanderson.\n"
    "(See documentation for license)";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {

    PPTX_ReqPPTVersion, 6,

    /*
     *  Tells the capabilities of this loader/saver unit.
     */

    PPTX_Load,          TRUE,
    PPTX_ColorSpaces,   CSF_LUT,

    /*
     *  Here are some pretty standard definitions. All iomodules should have
     *  these defined.
     */

    PPTX_Name,          (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,        (ULONG)"Janne Jalkanen 1999",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"TYPE/K",

    PPTX_PreferredPostFix,(ULONG)".bmp",
    PPTX_PostFixPattern,(ULONG)"#?.(bmp)",

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif


LIBINIT
{
    return 0;
}

LIBCLEANUP
{
}

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

static unsigned long
BMPlenfileheader(int class)
{
        switch (class)
        {
        case C_WIN:
                return 14;
        case C_OS2:
                return 14;
        }
}

static unsigned long
BMPleninfoheader(int class)
{
        switch (class)
        {
        case C_WIN:
                return 40;
        case C_OS2:
                return 12;
        }
}

static unsigned long
BMPlenrgbtable(int class, ULONG bitcount)
{
        unsigned long   lenrgb;

        if (bitcount < 1)
        {
                // ERROR!
                return 0;
        }
        switch (class)
        {
        case C_WIN:
                lenrgb = 4;
                break;
        case C_OS2:
                lenrgb = 3;
                break;
        }

        return (1 << bitcount) * lenrgb;
}

/*
 * length, in bytes, of a line of the image
 *
 * Evidently each row is padded on the right as needed to make it a
 * multiple of 4 bytes long.  This appears to be true of both
 * OS/2 and Windows BMP files.
 */
static unsigned long
BMPlenline(int class, ULONG bitcount, ULONG x)
{
        unsigned long   bitsperline;

        switch (class)
        {
        case C_WIN:
                break;
        case C_OS2:
                break;
        }

        bitsperline = x * bitcount;

        /*
         * if bitsperline is not a multiple of 32, then round
         * bitsperline up to the next multiple of 32.
         */
        if ((bitsperline % 32) != 0)
        {
                bitsperline += (32 - (bitsperline % 32));
        }

        if ((bitsperline % 32) != 0)
        {
                // pm_error(er_internal, "BMPlenline");
                return 0;
        }

        /* number of bytes per line == bitsperline/8 */
        return bitsperline >> 3;
}

/* return the number of bytes used to store the image bits */
static unsigned long
BMPlenbits(int class, ULONG bitcount, ULONG x, ULONG y)
{
        return y * BMPlenline(class, bitcount, x);
}

/* return the offset to the BMP image bits */
static unsigned long
BMPoffbits(int class, ULONG bitcount)
{
        return BMPlenfileheader(class)
                + BMPleninfoheader(class)
                + BMPlenrgbtable(class, bitcount);
}

/* return the size of the BMP file in bytes */
static unsigned long
BMPlenfile(int class, ULONG bitcount, ULONG x, ULONG y)
{
        return BMPoffbits(class, bitcount)
                + BMPlenbits(class, bitcount, x, y);
}

static LONG
GetByte(BPTR fh)
{
    LONG             v;

    if ((v = FGetC(fh)) == -1) {
        return -1;
    }

    return v & 0xFF;
}

static short
GetShort(BPTR fh)
{
    short           v;
    long hi, lo;

    lo = GetByte( fh );
    hi = GetByte( fh );

    if( hi < 0 || lo < 0 ) return -1;

    v = (hi << 8 ) | lo;

    return v;
}

static long
GetLong(BPTR fh)
{
    long            v;
    long a, b, c, d;

    a = GetByte( fh );
    b = GetByte( fh );
    c = GetByte( fh );
    d = GetByte( fh );

    if( a < 0 || b < 0 || c < 0 || d < 0 ) return -1;

    v = (d << 24) | (c << 16) | (b << 8) | a;

    return v;
}


/*
    This must always exist!
*/

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    if( buf[0] == 'B' && buf[1] == 'M' )
        return TRUE;

    return FALSE;
}

static PERROR
BMPreadfileheader(BPTR fh, ULONG *ppos, ULONG *poffBits)
{
        unsigned long   cbSize;
        unsigned short  xHotSpot;
        unsigned short  yHotSpot;
        unsigned long   offBits;

        if (GetByte(fh) != 'B')
        {
                D(bug("This is not a BMP file\n"));
                return PERR_UNKNOWNTYPE;
        }
        if (GetByte(fh) != 'M')
        {
                D(bug("This is not a BMP file"));
                return PERR_UNKNOWNTYPE;
        }

        cbSize = GetLong(fh);
        xHotSpot = GetShort(fh);
        yHotSpot = GetShort(fh);
        offBits = GetLong(fh);

        *poffBits = offBits;

        *ppos += 14;

        return PERR_OK;
}

static PERROR
BMPreadinfoheader(FRAME *frame, BPTR fh, ULONG *ppos, ULONG *pcx, ULONG *pcy, USHORT *pcBitCount, int *pclass, struct PPTBase *PPTBase)
{
        unsigned long   cbFix;
        unsigned short  cPlanes;

        unsigned long   cx;
        unsigned long   cy;
        unsigned short  cBitCount;
        int             class;

        cbFix = GetLong(fh);

        switch (cbFix)
        {
        case 12:
                class = C_OS2;

                cx = GetShort(fh);
                cy = GetShort(fh);
                cPlanes = GetShort(fh);
                cBitCount = GetShort(fh);

                break;
        case 40:
                class = C_WIN;

                cx = GetLong(fh);
                cy = GetLong(fh);
                cPlanes = GetShort(fh);
                cBitCount = GetShort(fh);

                /*
                 * We've read 16 bytes so far, need to read 24 more
                 * for the required total of 40.
                 */

                GetLong(fh);
                GetLong(fh);
                GetLong(fh);
                GetLong(fh);
                GetLong(fh);
                GetLong(fh);

                break;
        default:
                return PERR_ERROR;
                break;
        }

        if (cPlanes != 1)
        {
                D(bug("Help: don't know how to handle cPlanes = %d"
                         ,cPlanes));
                SetErrorMsg( frame, "cPlanes != 1" );
                return PERR_ERROR;
        }

        switch (class)
        {
        case C_WIN:
                D(bug("Windows BMP, %dx%dx%d"
                           ,cx
                           ,cy
                           ,cBitCount));
                break;
        case C_OS2:
                D(bug("OS/2 BMP, %dx%dx%d"
                           ,cx
                           ,cy
                           ,cBitCount));
                break;
        }

        *pcx = cx;
        *pcy = cy;
        *pcBitCount = cBitCount;
        *pclass = class;

        *ppos += cbFix;

        return PERR_OK;
}

static int
BMPreadrgbtable(FRAME *frame, BPTR fh, ULONG *ppos, USHORT cBitCount, int class, COLORMAP *ct)
{
    int             i;
    int             nbyte = 0;
    long            ncolors = (1 << cBitCount);

    for (i = 0; i < ncolors; i++) {

        ct[i].b = GetByte( fh );
        ct[i].g = GetByte( fh );
        ct[i].r = GetByte( fh );
        nbyte += 3;

        if (class == C_WIN) {
            (void) GetByte(fh);
            nbyte++;
        }
    }

    *ppos += nbyte;
    return nbyte;
}

static int
BMPreadrow(BPTR fh, ULONG *ppos, RGBPixel *row, ULONG cx, USHORT cBitCount, COLORMAP *ct)
{
        BITSTREAM       b;
        int             nbyte = 0;
        int             rc;
        unsigned        x;

        if ((b = pm_bitinit(fh, "r")) == (BITSTREAM) 0)
        {
                return -1;
        }

        for (x = 0; x < cx; x++, row++)
        {
                unsigned long   v;

                if ((rc = pm_bitread(b, cBitCount, &v)) == -1)
                {
                        return -1;
                }
                nbyte += rc;

                row->r = ct[v].r;
                row->g = ct[v].g;
                row->b = ct[v].b;

        }

        if ((rc = pm_bitfini(b)) != 0)
        {
                return -1;
        }

        /*
         * Make sure we read a multiple of 4 bytes.
         */
        while (nbyte % 4)
        {
                GetByte(fh);
                nbyte++;
        }

        *ppos += nbyte;
        return nbyte;
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    PERROR res = PERR_OK;
    int             rc;
    unsigned long   pos = 0;
    unsigned long   offBits;
    unsigned long   cx;
    unsigned long   cy;
    unsigned short  cBitCount;
    int             class;
    COLORMAP      *ct;

    D(bug("IOLoad()\n"));

    if( (res = BMPreadfileheader(fh, &pos, &offBits)) == PERR_OK ) {
        if( (res = BMPreadinfoheader(frame, fh, &pos, &cx, &cy, &cBitCount, &class, PPTBase)) == PERR_OK ) {
            InitProgress( frame, "Reading BMP file...", 0, cy );

            /* Initialize the frame */

            frame->pix->width      = cx;
            frame->pix->height     = cy;
            frame->pix->colorspace = CS_RGB;
            frame->pix->origdepth  = cBitCount;
            frame->pix->components = 3;

            if( ct = AllocVec( (1<<cBitCount) * sizeof( ARGBPixel ), 0L ) ) {
                if( ( res = InitFrame( frame ) ) == PERR_OK ) {
                    WORD row;

                    BMPreadrgbtable( frame, fh, &pos, cBitCount, class, ct );

                    for( row = cy-1; row >= 0; row-- ) {
                        RGBPixel *cp;

                        if( Progress( frame, cy-row ) )
                            return PERR_BREAK;

                        cp = (RGBPixel *)GetPixelRow( frame, row );

                        D(bug("   Reading in row %d\n",row ));

                        rc = BMPreadrow( fh, &pos, cp, cx, cBitCount, ct );

                        if( rc > 0 && rc%4 ) {
                            SetErrorMsg( frame, "Corrupted file: Bad number of bytes in a row." );
                            return PERR_ERROR;
                        } else if( rc < 0 ) {
                            SetErrorCode( frame, PERR_FILEREAD );
                            return PERR_ERROR;
                        }

                        PutPixelRow( frame, row, cp );
                    }

                }

                FreeVec( ct );
            } else {
                SetErrorCode( frame, PERR_OUTOFMEMORY );
            }

            FinishProgress( frame );
        }
    } else {
        SetErrorMsg( frame, "This is not a BMP file!" );
    }

    return res;
}

/*----------------------------------------------------------------------------*/

static void
_PutByte(BPTR fh, UBYTE v, struct PPTBase *PPTBase)
{
    // struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    FPutC( fh, v );
}

static void
_PutShort(BPTR fh, USHORT v, struct PPTBase *PPTBase)
{
    // struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    FPutC( fh, v & 0xFF );
    FPutC( fh, (v >> 8) & 0xFF );
}

static void
_PutLong(BPTR fh, ULONG v, struct PPTBase *PPTBase)
{
    // struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    FPutC( fh, v & 0xFF );
    FPutC( fh, (v >> 8) & 0xFF );
    FPutC( fh, (v >> 16) & 0xFF );
    FPutC( fh, (v >> 24) & 0xFF );
}

#define PutLong(a,b) _PutLong(a,b,PPTBase)
#define PutShort(a,b) _PutShort(a,b,PPTBase)
#define PutByte(a,b) _PutByte(a,b,PPTBase)

/*
 * returns the number of bytes written, or -1 on error.
 */
static int
BMPwritefileheader(BPTR fh, int class, ULONG bitcount, ULONG x, ULONG y, struct PPTBase *PPTBase)
{
    PutByte(fh, 'B');
    PutByte(fh, 'M');

    /* cbSize */
    PutLong(fh, BMPlenfile(class, bitcount, x, y));

    /* xHotSpot */
    PutShort(fh, 0);

    /* yHotSpot */
    PutShort(fh, 0);

    /* offBits */
    PutLong(fh, BMPoffbits(class, bitcount));

    return 14;
}

/*
 * returns the number of bytes written, or -1 on error.
 */
static int
BMPwriteinfoheader(BPTR fh, int class, ULONG bitcount, ULONG x, ULONG y, struct PPTBase *PPTBase)
{
    long    cbFix;

    /* cbFix */
    switch (class) {
      case C_WIN:
        cbFix = 40;
        PutLong(fh, cbFix);

        /* cx */
        PutLong(fh, x);
        /* cy */
        PutLong(fh, y);
        /* cPlanes */
        PutShort(fh, 1);
        /* cBitCount */
        PutShort(fh, bitcount);

        /*
         * We've written 16 bytes so far, need to write 24 more
         * for the required total of 40.
         */

        PutLong(fh, 0);
        PutLong(fh, 0);
        PutLong(fh, 0);
        PutLong(fh, 0);
        PutLong(fh, 0);
        PutLong(fh, 0);

        break;

      case C_OS2:
        cbFix = 12;
        PutLong(fh, cbFix);

        /* cx */
        PutShort(fh, x);
        /* cy */
        PutShort(fh, y);
        /* cPlanes */
        PutShort(fh, 1);
        /* cBitCount */
        PutShort(fh, bitcount);
        break;
    }

    return cbFix;
}

/*
 * returns the number of bytes written, or -1 on error.
 */
static int
BMPwritergb(BPTR fh, int class, ARGBPixel *argb,struct PPTBase *PPTBase)
{
    switch (class) {
      case C_WIN:
        PutByte(fh, argb->b);
        PutByte(fh, argb->g);
        PutByte(fh, argb->r);
        PutByte(fh, 0);
        return 4;

      case C_OS2:
        PutByte(fh, argb->b);
        PutByte(fh, argb->g);
        PutByte(fh, argb->r);
        return 3;
    }
    return -1;
}

/*
 * returns the number of bytes written, or -1 on error.
 */
static int
BMPwritergbtable(BPTR fh, int class, int bpp, int colors,
                 COLORMAP *cmap,struct PPTBase *PPTBase)
{
    int             nbyte = 0;
    int             i;
    long            ncolors;

    for (i = 0; i < colors; i++)
    {
        nbyte += BMPwritergb(fh,class, &cmap[i], PPTBase);
    }

    ncolors = (1 << bpp);

    for (; i < ncolors; i++) {
        ARGBPixel black = {0};

        nbyte += BMPwritergb(fh,class,&black,PPTBase);
    }

    return nbyte;
}

/*
 * returns the number of bytes written, or -1 on error.
 */
static int
BMPwriterow(BPTR fh, ROWPTR row, ULONG width, USHORT bpp, struct PPTBase *PPTBase)
{
    BITSTREAM       b;
    int             nbyte = 0;
    int             rc;
    unsigned        x;

    if ((b = pm_bitinit(fh, "w")) == (BITSTREAM) 0)
    {
        D(bug("  bitinit() failed\n"));
        return -1;
    }

    for (x = 0; x < width; x++, row++) {

        if ((rc = pm_bitwrite(b, bpp, (ULONG) *row )) == -1) {
            D(bug("  bitwrite() failed\n"));
            return -1;
        }
        nbyte += rc;
    }

    if ((rc = pm_bitfini(b)) == -1) {
        D(bug("  bitfini() failed\n"));
        return -1;
    }
    nbyte += rc;

    /*
     * Make sure we write a multiple of 4 bytes.
     */
    while (nbyte % 4) {
        PutByte(fh, 0);
        nbyte++;
    }

    return nbyte;
}

/*
 * returns the number of bytes written, or -1 on error.
 */
static int
BMPwritebits(FRAME *frame, ULONG format, BPTR fh, int cBitCount, struct PPTBase *PPTBase)
{
    int             nbyte = 0;
    long            y;


    if(cBitCount > 24) {
        SetErrorMsg( frame, "Internal error: cannot handle this cBitCount");
        return -1;
    }

    /*
     * The picture is stored bottom line first, top line last
     */

    for (y = frame->pix->height - 1; y >= 0; y--) {
        ROWPTR cp;
        int rc;

        if( Progress( frame, frame->pix->height-y ) ) {
            return -1;
        }

        if( format == CSF_LUT ) {
            cp = GetBitMapRow( frame, y );
        } else {
            cp = GetPixelRow( frame, y );
        }

        rc = BMPwriterow(fh, cp, frame->pix->width, cBitCount, PPTBase );

        if(rc == -1) {
            D(bug("couldn't write row %d\n" ,y));
        }

        if(rc%4) {
            D(bug("row had bad number of bytes: %d\n",rc));
        }

        nbyte += rc;
    }

    return nbyte;
}

/*
 * Return the number of bits per pixel required to represent the
 * given number of colors.
 */

static int
colorstobpp(int colors)
{
    int bpp;

    if (colors < 1) {
        D(bug("can't have less than one color\n"));
    }

    for(bpp = 1; bpp < 24; bpp++ ) {
        if( (1 << bpp) >= colors )
            break;
    }

    return bpp;
}

/*
 * Write a BMP file of the given class.
 *
 * Note that we must have 'colors' in order to know exactly how many
 * colors are in the R, G, B, arrays.  Entries beyond those in the
 * arrays are undefined.
 */
static PERROR
BMPEncode(FRAME *frame, ULONG format, BPTR fh, int class, struct PPTBase *PPTBase )
{
    int             bpp;    /* bits per pixel */
    unsigned long   nbyte = 0;
    int colors = frame->disp->ncolors;
    int height = frame->pix->height;
    int width  = frame->pix->width;
    int rc;

    InitProgress( frame, "Writing BMP...", 0, frame->pix->height );

    bpp = colorstobpp(colors);

    /*
     * I have found empirically at least one BMP-displaying program
     * that can't deal with (for instance) using 3 bits per pixel.
     * I have seen no programs that can deal with using 3 bits per
     * pixel.  I have seen programs which can deal with 1, 4, and
     * 8 bits per pixel.
     *
     * Based on this, I adjust actual the number of bits per pixel
     * as follows.  If anyone knows better, PLEASE tell me!
     */
    switch(bpp) {
      case 2:
      case 3:
        bpp = 4;
        break;

      case 5:
      case 6:
      case 7:
        bpp = 8;
        break;
    }

    D(bug("Using %d bits per pixel\n", bpp));

    nbyte += BMPwritefileheader(fh, class, bpp, width, height, PPTBase);
    nbyte += BMPwriteinfoheader(fh, class, bpp, width, height, PPTBase);
    nbyte += BMPwritergbtable(fh, class, bpp, colors, frame->disp->colortable, PPTBase);

    if(nbyte !=     ( BMPlenfileheader(class)
                    + BMPleninfoheader(class)
                    + BMPlenrgbtable(class, bpp)))
    {
        SetErrorMsg( frame, "Internal error.  Header count wrong." );
        return PERR_ERROR;
    }

    rc = BMPwritebits(frame, format, fh, bpp, PPTBase );
    if( rc < 0 ) return PERR_BREAK;

    nbyte += rc;
    if(nbyte != BMPlenfile(class, bpp, width, height))
    {
        SetErrorMsg( frame, "Internal error.  File length wrong." );
        return PERR_ERROR;
    }

    FinishProgress( frame );

    return PERR_OK;
}

const char *mode_labels[] = {
    "Windows",
    "OS/2",
    NULL
};


PERROR DoGUI( FRAME *frame, LONG *mode, struct PPTBase *PPTBase )
{
    struct TagItem modemx[] = { AROBJ_Value, NULL,
                                ARMX_Labels, (ULONG)mode_labels,
                                TAG_DONE };
    struct TagItem window[] = { AR_MxObject, NULL,
                                AR_Text,     (ULONG)"\nPick save format, please.\n",
                                AR_HelpNode, (ULONG)"loaders.guide/BMP",
                                TAG_DONE };

    window[0].ti_Data = (ULONG)modemx;
    modemx[0].ti_Data = (ULONG)mode;

    return AskReqA( frame, window );
}

PERROR ParseRexxArgs( LONG *mode, ULONG *args, struct PPTBase *PPTBase )
{
    if( args[0] ) {
        if( strncmp( (char *)args[0], mode_labels[0], strlen( (char *)args[0])) == 0 ) {
            *mode = C_WIN;
        } else {
            *mode = C_OS2;
        }
    }
    return PERR_OK;
}

/*
    Format can be any of CSF_* - flags
*/
IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    LONG mode = C_WIN;
    ULONG *args;

    D(bug("IOSave(type=%08X)\n",format));

    if( format != CSF_LUT ) {
        return PERR_UNKNOWNTYPE;
    }

    if( args = (ULONG *) TagData( PPTX_RexxArgs, tags ) ) {
        ParseRexxArgs( &mode, args, PPTBase );
    } else {
        PERROR res;
        if( (res = DoGUI( frame, &mode, PPTBase ) != PERR_OK )) {
            return res;
        }
    }

    D(bug("Proceeding to write a %s BMP file...\n", mode_labels[mode] ));

    return BMPEncode( frame, format, fh, mode, PPTBase );
}

IOGETARGS(format,frame,tags,PPTBase,IOModuleBase)
{
    PERROR res;
    ULONG *args;
    STRPTR buffer;
    LONG mode = C_WIN;

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *) TagData( PPTX_RexxArgs, tags ) )
        ParseRexxArgs( &mode, args, PPTBase );

    if( (res = DoGUI( frame, &mode, PPTBase )) == PERR_OK ) {
        SPrintF( buffer, "TYPE %s", mode_labels[mode] );
    }

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

