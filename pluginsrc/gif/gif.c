/*
    GIF I/O module

    $Id: gif.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/

#include "pptplugin.h"

#include <exec/types.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>

#include <pragmas/exec_pragmas.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/utility_pragmas.h>


#include <utility/tagitem.h>

#include <pragmas/pptsupp_pragmas.h>
#include <dos/stdio.h>

#include <stdio.h>
#include <string.h>

#include "ppt.h"
#include "GIF.h"


/*------------------------------------------------------------------*/

#define MYNAME          "GIF"

/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/

const char infoblurb[] =
#ifdef SUPPORT_SAVE
    "Loads and saves Compuserve GIF files.\n"
#else
    "Loads CompuServe GIF files.\n"
#endif
    "Both the GIF87a and 89a formats are supported,\n"
    "as well as interlaced files.\n"
    "\n"
    "This is based on the NetPBM distribution\n"
    "and the original code is (C) David Koblas\n"
    "Thanks.\n"
    "\n"
#ifdef SUPPORT_SAVE
    "";
#else
    "Saving is not allowed in this version\n";
#endif

const struct TagItem MyTagArray[] = {
    PPTX_Load,              TRUE,
#ifdef SUPPORT_SAVE
    PPTX_ColorSpaces,       CSF_LUT,
#else
    PPTX_ColorSpaces,       0L,
#endif
    PPTX_Name,              (ULONG)MYNAME,
    PPTX_Author,            (ULONG)"Janne Jalkanen 1996-1998",
    PPTX_InfoTxt,           (ULONG)infoblurb,
    PPTX_RexxTemplate,      (ULONG)"INTERLACED/S,TRANSPARENT/S",

    PPTX_PreferredPostFix,  (ULONG)".gif",
    PPTX_PostFixPattern,    (ULONG)"#?.(gif|gif89)",
    PPTX_ReqPPTVersion,     4,

    TAG_DONE
};

/*------------------------------------------------------------------*/

/*
    Does a simple check on the file to see if it is a proper
    GIF file we can read.
*/
BOOL CheckGIF( FRAME *frame, BPTR fh, BOOL verbose, struct PPTBase *PPTBase )
{
    UBYTE buf[16], version[8];
    APTR DOSBase = PPTBase->lb_DOS;

    if (! ReadOK(fh,buf,6)) {
        if(verbose) SetErrorMsg( frame, "Error reading magic number" );
        return FALSE;
    }

    if (strncmp((char *)buf,"GIF",3) != 0) {
        if(verbose) SetErrorMsg(frame, "Not a GIF file!" );
        return FALSE;
    }

    strncpy(version, (char *)buf + 3, 3);
    version[3] = '\0';

    if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0)) {
        if(verbose) SetErrorMsg(frame,"Bad version number, not '87a' or '89a'" );
        return FALSE;
    }

    return TRUE;
}

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    UBYTE buf[16], c;
    struct GIF GifScreen = {0};
    PERROR res = PERR_OK;
    int useGlobalColormap, imageCount = 0;

    D(bug(MYNAME" : Load()\n"));

    if(CheckGIF( frame, fh, TRUE, PPTBase ) == FALSE)
        return PERR_FAILED;

    /*
     *  Defaults
     */

    GifScreen.gif89.transparent = -1;

    /*
     *  Get picture size and allocate the Frame
     */

    // SetVBuf( fh, NULL, BUF_FULL, 32768L );

    if (! ReadOK(fh,buf,7)) {
        SetErrorMsg(frame,"Failed to read image info!" );
        return PERR_FAILED;
    }

    GifScreen.Width           = LM_to_uint(buf[0],buf[1]);
    GifScreen.Height          = LM_to_uint(buf[2],buf[3]);
    GifScreen.BitPixel        = 2<<(buf[4]&0x07);
    GifScreen.ColorResolution = (((buf[4]&0x70)>>3)+1);
    GifScreen.Background      = buf[5];
    GifScreen.AspectRatio     = buf[6];

    GifScreen.Frame           = frame;

    /*
     *  Read in the GIF colormap. If the colormap proves to be
     *  a grayscale one, then we'll make a grayscale image.
     */

    if (BitSet(buf[4], LOCALCOLORMAP)) {    /* Global Colormap */
        if (ReadColorMap( fh, &GifScreen, PPTBase )) {
            SetErrorMsg(frame, "Error while reading global colormap" );
            return PERR_FAILED;
        }
    }

    while( res == PERR_OK ) {

        if (! ReadOK(fh,&c,1)) {
            SetErrorMsg(frame, "EOF / read error on image data" );
            return PERR_FAILED;
        }

        if (c == ';') {         /* GIF terminator */
            if (imageCount > 1) {
                SetErrorMsg(frame, "Only 1 image/file supported" );
                return PERR_WARNING;
            }
            return res;
        }

        if (c == '!') {         /* Extension */
            if (! ReadOK(fh,&c,1)) {
                SetErrorMsg(frame,"EOF while attempting to read extension block");
                return PERR_WARNING;
            }
            DoExtension(fh, c, &GifScreen, PPTBase);
            continue;
        }

        if (c != ',') {         /* Not a valid start character */
            /* BUG: Should do something about this */
            // pm_message("bogus character 0x%02x, ignoring", (int) c );
            continue;
        }

        ++imageCount;

        if (! ReadOK(fh,buf,9)) {
            SetErrorMsg(frame,"Couldn't read image attributes");
            return PERR_FAILED;
        }

        useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

        GifScreen.Interlace = BitSet( buf[8], INTERLACE );

        if (! useGlobalColormap) {
            GifScreen.BitPixel = 1<<((buf[8]&0x07)+1);

            if (ReadColorMap(fh, &GifScreen, PPTBase)) {
                SetErrorMsg(frame, "Error while reading local colormap" );
                return PERR_FAILED;
            }

        }

        frame->pix->height        = GifScreen.Height;
        frame->pix->width         = GifScreen.Width;
        frame->pix->origdepth     = 8; /* BUG: What? */

        if( GifScreen.gif89.transparent > 0 ) {
            frame->pix->colorspace = CS_ARGB;
            frame->pix->components = 4;
        } else {
            if( GifScreen.GrayScale ) {
                frame->pix->colorspace = CS_GRAYLEVEL;
                frame->pix->components = 1;
            } else {
                frame->pix->colorspace = CS_RGB;
                frame->pix->components = 3;
            }
        }

        res = InitFrame( frame );

        /*
         *  Read in the actual image
         */

        if( res == PERR_OK ) {
            InitProgress( frame, "Reading GIF image...", 0, GifScreen.Height );

            if( InitLWZData( &GifScreen, PPTBase ) ) {
                SetErrorCode( frame, PERR_OUTOFMEMORY );
                res = PERR_FAILED;
            } else {
                res = ReadImage(fh, frame, &GifScreen, PPTBase );
                FreeLWZData( &GifScreen, PPTBase );
                break; /* Exit while() */
            }

        }

    }

    /*
     *  Release allocated resources
     */

    return res;
}

#ifdef SUPPORT_SAVE

struct GIFSaveArgs {
    ULONG ilace, transp;
};

IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    struct GIFSave *gs;
    PERROR res;
    ULONG *args;
    struct GIFSaveArgs *save, defaults = {0};

    if( format != CSF_LUT )
        return PERR_MISSINGCODE;

    gs = (struct GIFSave *)AllocVec( sizeof( struct GIFSave ), 0L );
    if(!gs) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_FAILED;
    }

    /*
     *  Set up the image info structure
     */

    gs->frame = frame;
    gs->PPTBase = PPTBase;
    gs->fh = fh;
    gs->interlace = FALSE;
    gs->transparent = -1;
    gs->background = 0;
    gs->colormap = frame->disp->colortable;
    gs->curx = gs->cury = 0;
    gs->lasty = -1; /* No last row loaded, ie pixelbuffer is empty. */
    gs->CountDown = 0;
    gs->Pass = 0;
    gs->BitsPerPixel = frame->disp->depth;
    gs->getpixel = GetPixelColor;

    gs->maxbits = BITS;
    gs->maxmaxcode = (code_int) (1 << BITS);

    gs->hsize = HSIZE;
    gs->free_ent = 0;
    gs->clear_flg = 0;
    gs->in_count = 1;
    gs->out_count = 0;

    gs->cur_accum = 0;
    gs->cur_bits  = 0;

    /*
     *  Initiate queries on the GIF type.
     *      - transparency?
     *      - interlaced?
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( args[0] )
            gs->interlace = TRUE;

        if( args[1] )
            gs->transparent = 0;

    } else {
        struct TagItem cb1[] = {
            AROBJ_Label, (ULONG)"Interlaced?",
            AROBJ_Value, NULL,
            TAG_DONE
        };
        struct TagItem cb2[] = {
            AROBJ_Label, (ULONG)"Transparent?",
            AROBJ_Value, NULL,
            TAG_DONE
        };

        struct TagItem win[] = {
            AR_CheckBoxObject, NULL,
            AR_CheckBoxObject, NULL,
            AR_Text, (ULONG)"\033cSet the GIF file attributes, please!\n"
                            "(To write a GIF89a file, check the\n"
                            "'Transparent' - box)",
            TAG_DONE
        };

        if( save = GetOptions(MYNAME) )
            defaults = *save;

        win[0].ti_Data = (ULONG) &cb1;
        win[1].ti_Data = (ULONG) &cb2;
        cb1[1].ti_Data = (ULONG) &defaults.ilace;
        cb2[1].ti_Data = (ULONG) &defaults.transp;


        if( (res = AskReqA(frame, win)) != PERR_OK ) {
            SetErrorCode( frame, res );
            FreeVec( gs );
            return PERR_WARNING;
        }

        gs->interlace = defaults.ilace;
        gs->transparent = defaults.transp ? 0 : -1;
    }

    if( !(gs->pixelbuffer = AllocVec( frame->pix->width, MEMF_CLEAR )) ) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_ERROR;
    }

    /*
     *  Determine the transparent color by searching for the highest
     *  transparency value.  If there are multiple 255 alphas, the
     *  first one will be used.  If no alpha has been set, the color
     *  zero is used as a transparent background.
     */

    if( gs->transparent == 0 ) {
        int tp = 0,i,maxalpha = 0;

        for( i = 0; i < frame->disp->ncolors; i++ ) {
            if( frame->disp->colortable[i].a == 255 ) {
                tp = i;
                break;
            } else {
                if( frame->disp->colortable[i].a > maxalpha ) {
                    maxalpha = frame->disp->colortable[i].a;
                    tp = i;
                }
            }
        }

        gs->transparent = tp;
    }

    /*
     *  Save the body
     */

    res = GIFEncode( gs );

    if( gs->pixelbuffer ) FreeVec( gs->pixelbuffer );

    FreeVec( gs );

    PutOptions( MYNAME, &defaults, sizeof(defaults) );

    return res;
}
#else
IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    return PERR_MISSINGCODE;
}

#endif /* NO_SAVE */

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    D(bug(MYNAME": Check()\n"));

    return CheckGIF( NULL, fh, FALSE, PPTBase );
}


