/*
    This is a TARGA TrueVision file loader

    PPT is (C) Janne Jalkanen 1995-2000.

    $Id: targa.c,v 1.1 2001/10/25 16:23:03 jalkanen Exp $
*/

#undef DEBUG_MODE

#include <pptplugin.h>
#include "targa.h"

#include <exec/memory.h>

#include <string.h>

/*------------------------------------------------------------------*/
/* Defines */

#define MYNAME          "Targa"

/*------------------------------------------------------------------*/
/* Prototypes */

/*------------------------------------------------------------------*/
/* Global variables and constants */

const char infoblurb[] =
    "Loads and saves TrueVision Targa images.\n"
    "Loader supports 8-32 bit formats.\n"
    "Saver supports 8 and 24 bit formats.\n"
    "\n";

#pragma msg 186 ignore

const struct TagItem MyTagArray[] = {
    PPTX_Load,              TRUE,
    PPTX_Name,              (ULONG)MYNAME,
    PPTX_Author,            (ULONG)"Janne Jalkanen, 1996-2000",
    PPTX_InfoTxt,           (ULONG)infoblurb,
    PPTX_ColorSpaces,       CSF_RGB|CSF_GRAYLEVEL,
    PPTX_ReqPPTVersion,     3,
    PPTX_PreferredPostFix,  ".tga",
    PPTX_PostFixPattern,    "#?.(tga|targa)",
    PPTX_RexxTemplate,      "COMPRESS/S",
    PPTX_SupportsGetArgs,   TRUE,
    TAG_DONE, 0L
};

/*------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

PERROR ReadHeader( BPTR fh, FRAME *frame, struct Targa *tga, struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    unsigned int lo, hi, flags;
    LONG err;

    D(bug("ReadHeader()\n"));

    tga->IDLength   = FGetC(fh);
    tga->CoMapType  = FGetC(fh);
    tga->ImgType    = FGetC(fh);
    lo = FGetC(fh); hi = FGetC(fh);
    tga->Index      = 256*hi + lo;
    lo = FGetC(fh); hi = FGetC(fh);
    tga->Length     = 256*hi + lo;
    tga->CoSize     = FGetC(fh);

    lo = FGetC(fh); hi = FGetC(fh);
    tga->X_org      = 256*hi + lo;
    lo = FGetC(fh); hi = FGetC(fh);
    tga->Y_org      = 256*hi + lo;

    lo = FGetC(fh); hi = FGetC(fh);
    tga->Width      = 256*hi + lo;
    lo = FGetC(fh); hi = FGetC(fh);
    tga->Height     = 256*hi + lo;

    tga->PixelSize  = FGetC(fh);

    flags           = FGetC(fh);
    tga->AttBits    = flags & 0xf;
    tga->Rsrvd      = ( flags & 0x10 ) >> 4;
    tga->OrgBit     = ( flags & 0x20 ) >> 5;
    tga->IntrLve    = ( flags & 0xc0 ) >> 6;

    if ( tga->IDLength != 0 )
        FRead( fh, tga->IDString, 1, (int) tga->IDLength );

    if( (err = IoErr()) > 0 ) {
        char buf[40];
        D(bug("Error while reading header\n"));
        Fault( err, "Read error", buf, 39 );
        SetErrorMsg( frame, buf );
        return PERR_ERROR;
    }

    D(bug("Image size: %d x %d\n", tga->Width, tga->Height ));

    return PERR_OK;
}

PERROR FigureInfo( FRAME *frame, struct Targa *tga, struct PPTBase *PPTBase )
{
    D(bug("FigureInfo()\n"));

    switch( tga->ImgType ) {
        case TGA_Map:
        case TGA_RGB:
        case TGA_RLEMap:
        case TGA_RLERGB:
            if( tga->PixelSize == 3 ) {
                frame->pix->components = 3;
                frame->pix->origdepth = 24;
                frame->pix->colorspace = CS_RGB;
            } else {
                frame->pix->components = 4;
                frame->pix->origdepth = 32;
                frame->pix->colorspace = CS_ARGB;
            }
            break;

        case TGA_Mono:
        case TGA_RLEMono:
            frame->pix->components = 1;
            frame->pix->origdepth = 8;
            frame->pix->colorspace = CS_GRAYLEVEL;
            break;

        default:
            SetErrorMsg( frame, "Unknown Targa image type\n" );
            return PERR_ERROR;

    }

    if( tga->ImgType == TGA_Map || tga->ImgType == TGA_RLEMap ) {

        /*
         *  Contains colormapped information
         */

        if( tga->CoMapType != 1 ) {
            SetErrorMsg( frame, "Colormapped image with wrong color map type!" );
            return PERR_ERROR;
        }

        tga->mapped = 1;

        if( tga->CoSize == 8 || tga->CoSize == 24 || tga->CoSize == 32 ) {
            tga->maxval = 255;
        } else {
            if( tga->CoSize == 15 || tga->CoSize == 16 ) {
                tga->maxval = 31;
            } else {
                SetErrorMsg( frame, "Illegal colormap pixel size!\n") ;
                return PERR_ERROR;
            }
        }
    } else {
        /*
         *  Not colormapped
         */

         tga->mapped = 0;

         if( tga->PixelSize == 8 || tga->PixelSize == 24 || tga->PixelSize == 32 ) {
             tga->maxval = 255;
         } else {
             if( tga->PixelSize == 15 || tga->PixelSize == 16 ) {
                 tga->maxval = 31;
             } else {
                 SetErrorMsg( frame, "Illegal pixel size" );
                 return PERR_ERROR;
             }
         }
    }

    if( tga->ImgType == TGA_RLEMap || tga->ImgType == TGA_RLERGB || tga->ImgType == TGA_RLEMono )
        tga->rlenencoded = 1;
    else
        tga->rlenencoded = 0;

    D(bug("Info decoded: Imagetype=%d (colormapped=%d, rlenencoded=%d).\n"
          "              Pixelsize=%d (maxval=%d)\n",
          tga->ImgType, tga->mapped, tga->rlenencoded,
          tga->PixelSize, tga->maxval ));

    return PERR_OK;
}

PERROR GetMapEntry( BPTR fh, FRAME *frame, int cc, struct Targa *tga, struct PPTBase *PPTBase )
{
    unsigned char j, k, r = 0, g = 0, b = 0, a = 0;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    D(bug("GetMapEntry(%d)\n",cc));

    SetIoErr( 0 );

    /* Read appropriate number of bytes, break into rgb & put in map. */
    switch ( tga->CoSize ) {
        /* Grayscale data */
        case 8:
            r = g = b = FGetC(fh);
            break;

        case 16:
        case 15:
            j = FGetC(fh);
            k = FGetC(fh);
            r = ( k & 0x7C ) >> 2;
            g = ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 );
            b = j & 0x1F;
            break;

        case 32:
        case 24:
            b = FGetC(fh);
            g = FGetC(fh);
            r = FGetC(fh);
            if ( tga->PixelSize == 32 )
                a = FGetC(fh);      /* Read alpha byte & throw away. */
            break;
    }

    tga->colormap[cc].r = r;
    tga->colormap[cc].g = g;
    tga->colormap[cc].b = b;
    tga->colormap[cc].a = a;

    if( IoErr() > 0 ) {
        D(bug("Colormap reading failed\n"));
        SetErrorCode( frame, PERR_FILEREAD );
        return PERR_ERROR;
    }

    return PERR_OK;
}

PERROR ReadColorMap( BPTR fh, FRAME *frame, struct Targa *tga, struct PPTBase *PPTBase )
{
    /*
     *  If required, read the colormap info in now.
     */

    D(bug("ReadColorMap()\n"));

    if( tga->CoMapType ) {
        int i;

        if( (tga->Index + tga->Length + 1) > MAXCOLORS ) {
            SetErrorMsg( frame, "Too many colors in file!" );
            return PERR_ERROR;
        }

        for( i = tga->Index; i < tga->Index+tga->Length; i++ ) {
            if(GetMapEntry( fh, frame, i, tga, PPTBase ) != PERR_OK ) {
                D(bug("Couldn't get map entry!\n"));
                return PERR_ERROR;
            }
        }
    }

    D(bug("Colormap read OK\n"));

    return PERR_OK;
}


PERROR get_pixel( BPTR fh, FRAME *frame, UBYTE *dest, struct Targa *tga, struct PPTBase *PPTBase )
{
    static UBYTE Red, Grn, Blu, Alp;
    unsigned char j, k;
    static unsigned int l;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS; /* You're gonna need the read-stuff anyways */

    // D(bug("\tGetPixel(dest=%08X)\n",dest));

    /* Check if run length encoded. */
    if ( tga->rlenencoded ) {
        if ( tga->RLE_count == 0 ) { /* Have to restart run. */
            UBYTE i;

            i = FGetC(fh);
            tga->RLE_flag = ( i & 0x80 );
            if ( tga->RLE_flag == 0 ) {
                /* Stream of unencoded pixels. */
                tga->RLE_count = i + 1;
            } else {
                /* Single pixel replicated. */
                tga->RLE_count = i - 127;
            }
            /* Decrement count & get pixel. */
            tga->RLE_count--;
        } else { /* Have already read count & (at least) first pixel. */
            tga->RLE_count--;
            if ( tga->RLE_flag != 0 ) {
                /* Replicated pixels. */
                goto PixEncode;
            }
        }
    }


    /* Read appropriate number of bytes, break into RGB. */
    switch ( tga->PixelSize ) {
        case 8:
            Red = Grn = Blu = l = FGetC( fh );
            break;

        case 16:
        case 15:
            j = FGetC( fh );
            k = FGetC( fh );
            l = ( (unsigned int) k << 8 ) + j;
            Red = ( k & 0x7C ) >> 2;
            Grn = ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 );
            Blu = j & 0x1F;
            break;

        case 32:
        case 24:
            Blu = FGetC( fh );
            Grn = FGetC( fh );
            Red = FGetC( fh );
            if ( tga->PixelSize == 32 )
                Alp = FGetC( fh );      /* Alpha */
            l = 0;
            break;
    }

PixEncode:
    if ( tga->mapped ) {
        switch( frame->pix->colorspace ) {
            case CS_ARGB:
                ((ARGBPixel *)dest)->a = tga->colormap[l].a;
                ((ARGBPixel *)dest)->r = tga->colormap[l].r;
                ((ARGBPixel *)dest)->g = tga->colormap[l].g;
                ((ARGBPixel *)dest)->b = tga->colormap[l].b;
                break;
            case CS_RGB:
                ((RGBPixel *)dest)->r = tga->colormap[l].r;
                ((RGBPixel *)dest)->g = tga->colormap[l].g;
                ((RGBPixel *)dest)->b = tga->colormap[l].b;
                break;
            case CS_GRAYLEVEL:
                *dest = tga->colormap[l].r;
                break;

        }
    } else {
        switch( frame->pix->colorspace ) {
            case CS_RGB:
                ((RGBPixel *)dest)->r = Red;
                ((RGBPixel *)dest)->g = Grn;
                ((RGBPixel *)dest)->b = Blu;
                break;

            case CS_ARGB:
                ((ARGBPixel *)dest)->r = Red;
                ((ARGBPixel *)dest)->g = Grn;
                ((ARGBPixel *)dest)->b = Blu;
                ((ARGBPixel *)dest)->a = Alp;
                break;

            case CS_GRAYLEVEL:
                *((UBYTE*)dest) = Red;
                break;
        }
    }

    return PERR_OK;
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    UBYTE *cp;
    UWORD crow;
    struct Targa *tga;
    PERROR res = PERR_OK;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS; /* You're gonna need the read-stuff anyways */
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    D(bug(MYNAME" : Load()\n"));

    /*
     *  Allocate & Initialize load object
     */

    tga = AllocVec( sizeof(struct Targa), MEMF_CLEAR );
    if(!tga) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_ERROR;
    }

    if( !(tga->colormap = AllocVec( MAXCOLORS * sizeof( ARGBPixel ), MEMF_CLEAR )) ) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        FreeVec( tga );
        return PERR_ERROR;
    }

    /*
     *  Get picture size and initialize the Frame
     */

    if( (res = ReadHeader( fh, frame, tga, PPTBase )) == PERR_OK ) {

        frame->pix->height = tga->Height;
        frame->pix->width  = tga->Width;

        InitProgress( frame, "Loading "MYNAME" picture...", 0, tga->Height );

        if( (res = FigureInfo( frame, tga, PPTBase )) == PERR_OK ) {

            if( (res = InitFrame( frame )) == PERR_OK ) {
                WORD realrow, truerow, baserow, col;

                if( ReadColorMap( fh, frame, tga, PPTBase ) != PERR_OK ) {
                    res = PERR_ERROR;
                    goto errorexit;
                }

                D(bug("Read colormap, starting on big stuff\n"));

                truerow = baserow = crow = 0;

                while( crow < tga->Height ) {

                    D(bug("\tRow %d (truerow = %d, baserow = %d)\n",crow, truerow, baserow));

                    if(Progress(frame, crow)) {
                        SetErrorCode( frame, PERR_BREAK );
                        res = PERR_ERROR;
                        goto errorexit;
                    }

                    /*
                     *  Get the actual row of pixels, then decompress it into cp
                     */

                    realrow = truerow;
                    if ( tga->OrgBit == 0 )
                        realrow = tga->Height - realrow - 1;

                    cp = GetPixelRow( frame, realrow );

                    for ( col = 0; col < tga->Width; col++ ) {
                        // D(bug("\t\tCol %d\n",col));
                        get_pixel( fh, frame, (UBYTE *)&(cp[col*frame->pix->components]),
                                   tga, PPTBase );
                    }

                    if ( tga->IntrLve == TGA_IL_Four )
                        truerow += 4;
                    else if ( tga->IntrLve == TGA_IL_Two )
                        truerow += 2;
                    else
                        truerow++;

                    if ( truerow >= tga->Height )
                        truerow = ++baserow;

                    PutPixelRow( frame, realrow, cp );

                    crow++;
                } /* while */

                FinishProgress(frame);
            } /* BeginLoad() */

errorexit: ;

        } /* If FigureInfo() */

    } /* If ReadHeader() */


    /*
     *  Release allocated resources
     */

    FreeVec( tga->colormap );
    FreeVec( tga );

    return res;
}

PERROR writetga( BPTR fh, FRAME *frame, struct Targa *tga, struct PPTBase *PPTBase )
{
    unsigned char flags;
    LONG err;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    FPutC( fh, tga->IDLength );
    FPutC( fh, tga->CoMapType );
    FPutC( fh, tga->ImgType );
    FPutC( fh, tga->Index % 256 );
    FPutC( fh, tga->Index / 256 );
    FPutC( fh, tga->Length % 256 );
    FPutC( fh, tga->Length / 256 );
    FPutC( fh, tga->CoSize );
    FPutC( fh, tga->X_org % 256 );
    FPutC( fh, tga->X_org / 256 );
    FPutC( fh, tga->Y_org % 256 );
    FPutC( fh, tga->Y_org / 256 );
    FPutC( fh, tga->Width % 256 );
    FPutC( fh, tga->Width / 256 );
    FPutC( fh, tga->Height % 256 );
    FPutC( fh, tga->Height / 256 );
    FPutC( fh, tga->PixelSize );
    flags = ( tga->AttBits & 0xf ) | ( ( tga->Rsrvd & 0x1 ) << 4 ) |
            ( ( tga->OrgBit & 0x1 ) << 5 ) | ( ( tga->OrgBit & 0x3 ) << 6 );
    FPutC( fh, flags );
    if ( tga->IDLength )
        FWrite( fh, tga->IDString, 1, (int) tga->IDLength );

    if( (err = IoErr()) > 0 ) {
        char buf[40];
        D(bug("Error while writing header\n"));
        Fault( err, "Write error", buf, 39 );
        SetErrorMsg( frame, buf );
        return PERR_ERROR;
    }

    return PERR_OK;
}

VOID compute_runlengths( FRAME *frame, ROWPTR pixelrow, struct Targa *tga )
{
    int cols = tga->Width;
    int* runlength = tga->runlength;
    int col, start;
    int comps = frame->pix->components;

    /* Initialize all run lengths to 0.  (This is just an error check.) */
    for ( col = 0; col < cols; ++col )
        runlength[col] = 0;

    /* Find runs of identical pixels. */
    for ( col = 0; col < cols; ) {
        start = col;
        do {
            ++col;
        } while ( col < cols &&
                  col - start < 128 &&
                  !memcmp( &pixelrow[col*comps], &pixelrow[start*comps], comps ) );

        runlength[start] = col - start;
    }

    /* Now look for runs of length-1 runs, and turn them into negative runs. */
    for ( col = 0; col < cols; ) {
        if ( runlength[col] == 1 ) {
            start = col;
            while ( col < cols &&
                    col - start < 128 &&
                    runlength[col] == 1 )
            {
                runlength[col] = 0;
                ++col;
            }
            runlength[start] = - ( col - start );
        } else {
            col += runlength[col];
        }
    }
}


/* CS_GRAYLEVEL */
static void
put_mono( BPTR fh, struct Targa *tga, UBYTE *pixel, struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    int c;

    c = (*pixel * tga->maxval) / 255;

    FPutC( fh, c );
}

/* CS_LUT */
static void
put_map( BPTR fh, struct Targa *tga, UBYTE *pixel, struct PPTBase *PPTBase )
{
    // struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    /* No code */
}

/*
    CS_RGB

    Note that Targa format has its components in the BGR order!
*/

static void
put_rgb( BPTR fh, struct Targa *tga, UBYTE *pixel, struct PPTBase *PPTBase )
{
    int r,g,b;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    r = *pixel++ * tga->maxval / 255; /* R */
    g = *pixel++ * tga->maxval / 255; /* G */
    b = *pixel++ * tga->maxval / 255; /* B */
    FPutC( fh, b );
    FPutC( fh, g );
    FPutC( fh, r );
}

static void
put_pixel( BPTR fh, struct Targa *tga, UBYTE *pixel, struct PPTBase *PPTBase )
{
    switch ( tga->ImgType ) {
        case TGA_Mono:
        case TGA_RLEMono:
            put_mono( fh, tga, pixel, PPTBase );
            break;
        case TGA_Map:
        case TGA_RLEMap:
            put_map( fh, tga, pixel, PPTBase );
            break;
        case TGA_RGB:
        case TGA_RLERGB:
            put_rgb( fh, tga, pixel, PPTBase );
            break;
    }
}

PERROR AskThings( FRAME *frame, struct TgaOpts *t, struct PPTBase *PPTBase )
{
    struct TagItem win[] = {
        AR_CheckBoxObject, NULL,
        // AR_CheckBoxObject, NULL,
        AR_Text, (ULONG)ISEQ_C"Do you wish to compress\n"
                              "the file you're about to save?",
        TAG_DONE
    };
    struct TagItem com[] = {
        AROBJ_Value, NULL,
        ARCHECKBOX_Selected, FALSE,
        AROBJ_Label, (ULONG) "Compress?",
        TAG_DONE
    };
#if 0
    struct TagItem intr[] = {
        AROBJ_Value, NULL,
        AROBJ_Label, (ULONG) "Interleave?",
        TAG_DONE
    };
#endif

    win[0].ti_Data = (ULONG)com;
    // win[1].ti_Data = (ULONG)intr;
    com[0].ti_Data = (ULONG)&(t->compress);
    com[1].ti_Data = t->compress;
    // intr[0].ti_Data = (ULONG)&(t->interleave);

    return AskReqA( frame, win );
}

IOGETARGS(format,frame,tags,PPTBase,IOModuleBase)
{
    struct TgaOpts tgaopts = {0}, *t;
    ULONG *args;
    PERROR res;
    STRPTR buffer;

    if( format == CSF_LUT || format == CSF_ARGB ) {
        return PERR_MISSINGCODE;
    }

    if( t = GetOptions(MYNAME) )
        tgaopts = *t;

    buffer = (STRPTR)TagData(PPTX_ArgBuffer, tags);

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        tgaopts.compress = args[0];
    }

    if( (res=AskThings( frame, &tgaopts, PPTBase )) == PERR_OK ) {
        SPrintF(buffer,"%s", tgaopts.compress ? "COMPRESS" : "" );
    }

    return res;
}

IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    PERROR res = PERR_OK;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    struct Targa *tga;
    WORD comps = frame->pix->components;
    ULONG *args;
    UWORD row, col, realrow;
    ROWPTR cp;
    struct TgaOpts tgaopts = {0}, *t;

    if( format == CSF_LUT || format == CSF_ARGB ) {
        return PERR_MISSINGCODE;
    }

    if( t = GetOptions(MYNAME) )
        tgaopts = *t;

    /*
     *  Allocate & Initialize save object
     */

    tga = AllocVec( sizeof(struct Targa), MEMF_CLEAR );
    if(!tga) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_ERROR;
    }

    if( !( tga->runlength = AllocVec( frame->pix->width*sizeof(int)*frame->pix->components, 0L) ) ) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        FreeVec( tga );
        return PERR_ERROR;
    }

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        tgaopts.compress = args[0];
    } else {
        if( AskThings( frame, &tgaopts, PPTBase ) != PERR_OK ) {
            SetErrorCode( frame, PERR_CANCELED );
            res = PERR_ERROR;
            goto errorexit;
        }
    }

    D(bug("Writing%s compressed &%s interleaved image\n",compress ? "" : "not",
           interleave ? "" : "not" ));

    tga->rlenencoded = (BOOL)tgaopts.compress;
    tga->IntrLve = tgaopts.interleave ? TGA_IL_Four : TGA_IL_None;

    tga->Height = frame->pix->height;
    tga->Width  = frame->pix->width;

    tga->CoMapType = 0;
    if( tga->rlenencoded ) {
        tga->ImgType = (frame->pix->colorspace == CS_RGB ) ? TGA_RLERGB : TGA_RLEMono;
    } else {
        tga->ImgType = (frame->pix->colorspace == CS_RGB ) ? TGA_RGB : TGA_Mono;
    }
    tga->Index  = 0;
    tga->Length = 0;
    tga->CoSize = 0;
    tga->X_org  = tga->Y_org = 0;
    tga->AttBits = 0;
    tga->Rsrvd   = 0;
    tga->OrgBit  = 0;
    tga->PixelSize = (frame->pix->colorspace == CS_RGB) ? 24 : 8;
    tga->mapped = FALSE;
    tga->maxval = 255;

    tga->IDLength = 0;

    /*
     *  Do the saving dance.
     */

    InitProgress( frame, "Saving Targa image...", 0, tga->Height );

    if( writetga( fh, frame, tga, PPTBase ) == PERR_OK ) {

        for ( row = 0; row < tga->Height; ++row ) {
            UBYTE *pP;

            if( Progress( frame, row ) ) {
                SetErrorCode( frame, PERR_BREAK );
                res = PERR_ERROR;
                goto errorexit;
            }

            realrow = row;
            if ( tga->OrgBit == 0 )
                realrow = tga->Height - realrow - 1;

            cp = GetPixelRow( frame, realrow );

            if ( tga->rlenencoded ) {

                compute_runlengths( frame, cp, tga );

                for ( col = 0; col < tga->Width; ) {
                    if ( tga->runlength[col] > 0 ) {
                        FPutC( fh, 0x80 + tga->runlength[col] - 1 );
                        put_pixel( fh, tga, &(cp[col*comps]), PPTBase );
                        col += tga->runlength[col];
                    } else if ( tga->runlength[col] < 0 ) {
                        int i;

                        FPutC( fh, -(tga->runlength[col]) - 1 );
                        for ( i = 0; i < -(tga->runlength[col]); ++i )
                            put_pixel( fh, tga, &(cp[(col+i)*comps]), PPTBase );
                        col += -(tga->runlength[col]);
                    }
                }
            } else {
                for ( col = 0, pP = cp; col < tga->Width; ++col, pP+=comps )
                    put_pixel( fh, tga, pP, PPTBase );
            }
        }

        FinishProgress( frame );
    }

    PutOptions( MYNAME, &tgaopts, sizeof( struct TgaOpts ) );

errorexit:
    /*
     *  Clean up.
     */

    FreeVec( tga->runlength );
    FreeVec( tga );

    return res;
}


/*
    This does the compare based on the file name only.
    BUG: Should really do something else...
*/

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS; /* You're gonna need the read-stuff anyways */
    char name[41], *s;

    D(bug(MYNAME": Check()\n"));

    if( NameFromFH(fh, name, 40) ) {
        if( s = strrchr(name, '.') ) {
            if( strncmp(s,".tga",4) == 0 ) {
                return TRUE;
            }
        }
    }
    return FALSE; /* By default, reject the stuff. */
}

/*-----------------------------------------------------------------*/
/*                          END OF CODE                            */
/*-----------------------------------------------------------------*/
