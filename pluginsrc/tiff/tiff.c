/*
    PROJECT: ppt
    MODULE:  tiff.iomod

    This file is (C) Janne Jalkanen 1998.

    This file contains an interface to the TIFF library my Sam Leffler of SGI.
    TIFFlib is (C) 1996 Silicon Graphics, Inc.

    $Id: tiff.c,v 1.1 2001/10/25 16:23:03 jalkanen Exp $
*/

#undef DEBUG_MODE

#include <pptplugin.h>

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <libtiff/tiff.h>
#include <libtiff/tiffio.h>
#include <libtiff/version.h>

#include <exec/memory.h>

#include "libtiff/tiff.h"
#include "libtiff/tiffiop.h" /* I know, it's private, but then again,
                                we need to poke around with some of the
                                internal stuff in there. */

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "TIFF"

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "TIFF IO module for PPT.\n"
    VERSION;
/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {

    /*
     *  Tells the capabilities of this loader/saver unit.
     */

    PPTX_Load,          TRUE,
    // PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_LUT,

    /*
     *  Here are some pretty standard definitions. All iomodules should have
     *  these defined.
     */

    PPTX_Name,          (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,        (ULONG)"Janne Jalkanen 1998-1999",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)NULL,

    PPTX_PreferredPostFix,(ULONG)".tif",
    PPTX_PostFixPattern,(ULONG)"#?.(tif|tiff)",
#ifdef _M68030
# ifdef _M68881
    PPTX_CPU,           AFF_68030|AFF_68881,
# else
    PPTX_CPU,           AFF_68030,
# endif
#endif

    TAG_END, 0L
};

struct PPTTIFF {
    BPTR            fh;
    FRAME          *frame;
    struct PPTBase *PPTBase;
};

/*
 *  TIFF library always uses these two formats - unfortunately.
 */

struct BGRAPixel {
    UBYTE   r,g,b,a;
};

struct BGRPixel {
    UBYTE r,g,b;
};

jmp_buf error_buffer;
char error_string[256] = "";

/*
 *  TIFF defines alpha in premultiplied form.  We don't, so we have to
 *  define a conversion here.
 */

#define UNPREMULTIPLY( c, a ) (UBYTE)( (a) ? 255 * (int)(c) / (int)(a) : 0)
#define CVT(x)  (UBYTE)( ((x) * 255) / 65535 )

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

#if 0
LIBINIT
{
    return 0;
}


LIBCLEANUP
{
}
#endif

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

static tsize_t
_tiffReadProc( struct PPTTIFF *pt, tdata_t buf, tsize_t size )
{
    struct DosLibrary *DOSBase = pt->PPTBase->lb_DOS;

    D(bug("ReadProc( %d bytes )\n",size));
    return (tsize_t) Read( pt->fh, buf, size );
}

static tsize_t
_tiffWriteProc(struct PPTTIFF *pt, tdata_t buf, tsize_t size)
{
    struct DosLibrary *DOSBase = pt->PPTBase->lb_DOS;

    D(bug("WriteProc( %d bytes )\n",size));
    return Write( pt->fh, buf, size );
}

static toff_t
_tiffSeekProc(struct PPTTIFF *pt, toff_t off, int whence)
{
    struct DosLibrary *DOSBase = pt->PPTBase->lb_DOS;
    long res, filesize, currentpos;
    BPTR fd = pt->fh;

    currentpos = Seek( fd, 0, OFFSET_END );
    filesize = Seek( fd, currentpos, OFFSET_BEGINNING );

    switch(whence) {
        case SEEK_SET:
            D(bug("Seeking SET %d\n",off));
            res = Seek( fd, off, OFFSET_BEGINNING );
            if( off > filesize ) {
                D(bug("\tSetting file size to %d\n",off));
                SetFileSize( fd, off, OFFSET_BEGINNING );
                Seek( fd, off, OFFSET_BEGINNING );
            }
            break;
        case SEEK_CUR:
            D(bug("Seeking CURRENT %d\n",off));
            res = Seek( fd, off, OFFSET_CURRENT );
            if( off+currentpos > filesize ) {
                D(bug("\tSetting file size to %d\n",off+currentpos));
                SetFileSize( fd, off, OFFSET_CURRENT );
                Seek( fd, off+currentpos, OFFSET_BEGINNING );
            }
            break;
        case SEEK_END:
            D(bug("Seeking END %d\n",off));
            if( off > 0 ) {
                D(bug("\tSetting file size to %d\n",off+filesize));
                SetFileSize( fd, off, OFFSET_END );
                Seek( fd, currentpos, OFFSET_BEGINNING );
            }
            res = Seek( fd, off, OFFSET_END );
            break;
    }
    return Seek( fd, 0, OFFSET_CURRENT );
}

static int
_tiffCloseProc( struct PPTTIFF *pt )
{
    return 0; // No error
}

static toff_t
_tiffSizeProc(struct PPTTIFF *pt)
{
    toff_t size;
    struct DosLibrary *DOSBase = pt->PPTBase->lb_DOS;

    Seek( pt->fh, 0, OFFSET_END );
    size = Seek( pt->fh, 0, OFFSET_END );
    D(bug("Getting file size : %d bytes\n",size));
    return size;
}

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
        (void) fd; (void) pbase; (void) psize;
        return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
        (void) fd; (void) base; (void) size;
}

void*
_TIFFmalloc(tsize_t s)
{
        return (AllocVec(s, MEMF_CLEAR));
}

void
_TIFFfree(tdata_t p)
{
        FreeVec(p);
}

void*
_TIFFrealloc(tdata_t p, tsize_t s)
{
        tdata_t p2;

        if( p2 = AllocVec( s, MEMF_CLEAR) ) {
            CopyMem( p, p2, s );
            FreeVec( p );
        }

        return p2;
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
        memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
        memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
        return (memcmp(p1, p2, (size_t) c));
}

static void
pptWarningHandler(const char* module, const char* fmt, va_list ap)
{
    char errbuf[256];

    D(bug("TIFF Warning:\n"));
    if (module != NULL)
        D(bug("%s: ", module));

    vsprintf(errbuf, fmt, ap);

    D(bug( errbuf )) ;
}
TIFFErrorHandler _TIFFwarningHandler = pptWarningHandler;

static void
pptErrorHandler(const char* module, const char* fmt, va_list ap)
{
    char errbuf[256];

    D(bug("TIFF Error:\n"));

    if (module != NULL)
        sprintf(error_string, "%s: ", module);
    vsprintf(errbuf, fmt, ap);
    strcat( error_string, errbuf );

    longjmp( error_buffer, 1 );
}
TIFFErrorHandler _TIFFerrorHandler = pptErrorHandler;

void __assert(int condition, const char *module, int line)
{
    if(!condition) {
        D(bug("assert\n"));

        TIFFError( module, "Internal error (assert() failed: line %d)", line );
    }
}

/*
    BUG: Maybe should do more checkups, other than the magic number check.
*/

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    TIFFHeader *tif_header = (TIFFHeader *)buf;

    if( tif_header->tiff_magic != TIFF_BIGENDIAN && tif_header->tiff_magic != TIFF_LITTLEENDIAN )
        return FALSE;

    return TRUE;
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    PERROR res = PERR_OK;
    TIFF *tif = NULL;
    struct PPTTIFF pt;

    D(bug("TIFF_Load()\n"));

    /*
     *  Initialize our error and IO handlers and open TIFF library.
     */

    pt.fh      = fh;
    pt.frame   = frame;
    pt.PPTBase = PPTBase;

    if( setjmp( error_buffer ) != 0 ) {
        SetErrorMsg( frame, error_string );
        if( tif ) {
            TIFFClose( tif );
            tif = NULL;
        }
        return PERR_FAILED;
    }

    D(bug("Begin load\n"));

    if( tif = TIFFClientOpen( (char *)GetTagData( PPTX_FileName, (ULONG)"", tags), "r",
                              (thandle_t) &pt,
                              _tiffReadProc, _tiffWriteProc,
                              _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
                              _tiffMapProc, _tiffUnmapProc ) )
    {
        uint32 imagelength, imagewidth, rowsperstrip;
        uint16 bits_per_sample, config, photometric, samples;
        uint16 *rmap, *gmap, *bmap;
        UBYTE *strip_data;
        char *author = NULL, *description = NULL;

        D(bug("TIFF open succeeded\n"));

        tif->tif_fd = &pt;

        TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &imagelength );
        TIFFGetField( tif, TIFFTAG_PLANARCONFIG, &config );
        TIFFGetField( tif, TIFFTAG_IMAGEWIDTH, &imagewidth );
        TIFFGetField( tif, TIFFTAG_BITSPERSAMPLE, &bits_per_sample );
        if( bits_per_sample != 8 ) {
            TIFFError( NULL, "Can't handle a different number of bits than 8!" );
            // BUG: needs to handle more depths
        }

        TIFFGetField( tif, TIFFTAG_ARTIST, &author );  // BUG: Ignored
        TIFFGetField( tif, TIFFTAG_IMAGEDESCRIPTION, &description ); // BUG: Ignored
        TIFFGetField( tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip );
        TIFFGetField( tif, TIFFTAG_PHOTOMETRIC, &photometric );
        TIFFGetField( tif, TIFFTAG_SAMPLESPERPIXEL, &samples );

        /*
         *  BUG: no resolution units handled yet
         */

        D(bug("Image size = %ldx%ld\n",imagewidth,imagelength ));
        D(bug("Author: %s\nAnnotation: %s\n",author ? author : "NULL", description ? description : "NULL"));
        if( author ) AddExtension( frame, EXTNAME_AUTHOR, author, strlen(author), EXTF_CSTRING );
        if( description ) AddExtension( frame, EXTNAME_ANNO, description, strlen(description), EXTF_CSTRING );

        frame->pix->height = (WORD) imagelength;
        frame->pix->width  = (WORD) imagewidth;

        switch( photometric ) {
            case PHOTOMETRIC_MINISBLACK:
            case PHOTOMETRIC_MINISWHITE:
                if( samples != 1 ) TIFFError( NULL, "Too many samples (%d) in grayscale image", samples );
                frame->pix->colorspace = CS_GRAYLEVEL;
                frame->pix->components = 1;
                break;

            case PHOTOMETRIC_RGB:
                if( samples == 4 ) frame->pix->colorspace = CS_ARGB;
                else if( samples == 3 ) frame->pix->colorspace = CS_RGB;
                else TIFFError( NULL, "Not RGB or ARGB! (%d samples)", samples );
                frame->pix->components = samples;
                break;

            case PHOTOMETRIC_PALETTE:
                frame->pix->colorspace = CS_RGB;
                frame->pix->components = 3;
                TIFFGetField( tif, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap );
                break;

            default:
                TIFFError( NULL, "Unsupported colorspace %ld.  Sorry.", photometric );
                // BUG: should try to handle JPEG too.
                break;
        }

        if( config == PLANARCONFIG_CONTIG ) {
            D(bug("Got continuous configuration\n"));
        } else if( config == PLANARCONFIG_SEPARATE ) {
            D(bug("Got separate fields\n"));
            TIFFError( NULL, "Can't handle yet separate fields" );
        }

        if( ( res = InitFrame( frame ) ) == PERR_OK ) {

            InitProgress( frame, "Loading TIFF image...", 0, TIFFNumberOfStrips(tif ) );

            /*
             *  Initialization has been done, so we will start to
             *  read in the data.  The best idea is to use strip-based
             *  library, since it supports reading compressed data.
             *  BUG: We don't handle tiled data just yet.
             */

            if( strip_data = _TIFFmalloc( TIFFStripSize(tif) ) ) {
                tstrip_t strip;
                WORD totalrows = 0, row;
                int  bytes_per_row;

                bytes_per_row = TIFFStripSize(tif) / rowsperstrip;

                for( strip = 0; strip < TIFFNumberOfStrips(tif); strip++ ) {
                    if( Progress( frame, strip ) ) {
                        break;
                    }

                    D(bug("Reading strip %d of %d...\n", strip, TIFFNumberOfStrips(tif) ));
                    TIFFReadEncodedStrip( tif, strip, strip_data, (tsize_t) -1 );

                    for( row = 0; row < rowsperstrip; row++ ) {
                        ROWPTR cp;
                        struct BGRAPixel *abgr;
                        struct BGRPixel  *bgr;

                        bgr = abgr = strip_data+row*bytes_per_row;

                        D(bug("GetPixelRow( %d )\n",row+totalrows));
                        cp = GetPixelRow( frame, row+totalrows );
                        if( cp ) {
                            int col;
                            UBYTE r,g,b;

                            /*
                             *  Do conversion to the Amiga byte order from the
                             *  TIFF byte order.  Also, performs lookup table conversion.
                             */

                            for( col = 0; col < imagewidth; col++ ) {
                                switch( photometric ) {

                                    case PHOTOMETRIC_MINISBLACK:
                                        cp[col] = *( (UBYTE*) bgr );
                                        break;

                                    case PHOTOMETRIC_MINISWHITE:
                                        cp[col] = 255 - *( (UBYTE*) bgr );
                                        break;

                                    case PHOTOMETRIC_PALETTE:
                                        r = CVT(rmap[ *( strip_data+row*bytes_per_row+col) ]);
                                        g = CVT(gmap[ *( strip_data+row*bytes_per_row+col) ]);
                                        b = CVT(bmap[ *( strip_data+row*bytes_per_row+col) ]);
                                        cp[3*col]   = r;
                                        cp[3*col+1] = g;
                                        cp[3*col+2] = b;
                                        break;

                                    case PHOTOMETRIC_RGB:
                                        if( samples == 3 ) {
                                            cp[3*col]   = bgr[col].r;
                                            cp[3*col+1] = bgr[col].g;
                                            cp[3*col+2] = bgr[col].b;
                                        } else {
                                            cp[4*col]   = 255-abgr[col].a;
                                            cp[4*col+1] = UNPREMULTIPLY(abgr[col].r, abgr[col].a);
                                            cp[4*col+2] = UNPREMULTIPLY(abgr[col].g, abgr[col].a);
                                            cp[4*col+3] = UNPREMULTIPLY(abgr[col].b, abgr[col].a);
                                        }
                                        break;
                                }
                            }
                        }
                        PutPixelRow( frame, row+totalrows, cp );
                    }
                    totalrows += rowsperstrip;
                }

                FinishProgress( frame );
                _TIFFfree( strip_data );
            } else {
                SetErrorCode( frame, PERR_OUTOFMEMORY );
                res = PERR_ERROR;
            }
        }

        TIFFClose( tif );

    } else {
        SetErrorMsg( frame, "Not a TIFF file" );
        res = PERR_ERROR;
    }

    return res;
}

/*
    Format can be any of CSF_* - flags
*/

IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    PERROR res = PERR_OK;
    TIFF *tif = NULL;
    struct PPTTIFF pt;

    D(bug("IOSave(type=%08X)\n",format));


    D(bug("TIFF_Load()\n"));

    /*
     *  Initialize our error and IO handlers and open TIFF library.
     */

    pt.fh      = fh;
    pt.frame   = frame;
    pt.PPTBase = PPTBase;

    if( setjmp( error_buffer ) != 0 ) {
        SetErrorMsg( frame, error_string );
        if( tif ) {
            TIFFClose( tif );
            tif = NULL;
        }
        return PERR_ERROR;
    }


    if( tif = TIFFClientOpen( GetTagData( PPTX_FileName, (ULONG)"", tags), "w",
                              (thandle_t) &pt,
                              _tiffReadProc, _tiffWriteProc,
                              _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
                              _tiffMapProc, _tiffUnmapProc ) )
    {
        uint16 samples, photometric, compression = COMPRESSION_NONE;
        uint32 rowsperstrip = (uint32) -1;
        WORD row;

        InitProgress( frame, "Writing TIFF file...", 0, frame->pix->height );

        switch( format ) {
            case CSF_RGB:
                samples = 3;
                photometric = PHOTOMETRIC_RGB;
                break;

            case CSF_ARGB:
                samples = 4;
                photometric = PHOTOMETRIC_RGB;
                break;

            case CSF_GRAYLEVEL:
                samples = 1;
                photometric = PHOTOMETRIC_MINISBLACK;
                break;

            default:
                SetErrorCode( frame, PERR_UNKNOWNCOLORSPACE );
                TIFFClose( tif );
                return PERR_ERROR;
        }

        rowsperstrip = 5; // BUG: magic

        TIFFSetField( tif, TIFFTAG_IMAGELENGTH, (uint32) frame->pix->height );
        TIFFSetField( tif, TIFFTAG_IMAGEWIDTH,  (uint32) frame->pix->width );
        TIFFSetField( tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT );
        TIFFSetField( tif, TIFFTAG_SAMPLESPERPIXEL, samples );
        TIFFSetField( tif, TIFFTAG_BITSPERSAMPLE, 8 ); // BUG: should read from struct
        TIFFSetField( tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
        TIFFSetField( tif, TIFFTAG_PHOTOMETRIC, photometric );
        TIFFSetField( tif, TIFFTAG_COMPRESSION, compression );

        TIFFSetField( tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip );
        if( frame->pix->DPIX != 0 && frame->pix->DPIY != 0 ) {
            TIFFSetField( tif, TIFFTAG_XRESOLUTION, frame->pix->DPIX );
            TIFFSetField( tif, TIFFTAG_YRESOLUTION, frame->pix->DPIY );
            TIFFSetField( tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH );
        }

        for( row = 0; row < frame->pix->height; row++ ) {
            ROWPTR cp;

            if( Progress( frame, row ) ) {
                res = PERR_BREAK;
                break;
            }

            D(bug("Writing scanline %d\n",row));

            cp = GetPixelRow( frame, row );

            if(TIFFWriteScanline( tif, cp, row, 0 ) < 0) {
                res = PERR_ERROR;
                break;
            }
        }

        FinishProgress( frame );

        TIFFClose(tif);
    }
    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

