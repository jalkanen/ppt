/*
    PROJECT: ppt
    MODULE:  yuvn.c

    YUVN loader module.

    $Id: yuvn.c,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
*/

#include <pptplugin.h>

#include <libraries/iffparse.h>
#include <exec/memory.h>

#include <clib/alib_protos.h>

#include <proto/iffparse.h>

#include "yuvn.h"

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "YUVN"

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "This loads VLab YUVN files.\n";


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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1997-1998",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)NULL,

    PPTX_PreferredPostFix,(ULONG)".yuv",
    PPTX_PostFixPattern,(ULONG)"#?.(yuv|yuvn|vlab)",

    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
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

/*
    This should be blatantly obvious.
*/

void PutErrMsg( FRAME *frame, ULONG code, struct PPTBase *PPTBase )
{
    char *msg;

    switch(code) {
        case SETUP_FAILED:
            msg = "Could not allocate Frame";
            break;
        case IFFPARSE_DID_NOT_OPEN:
            msg = "Couldn't open iffparse.library V36";
            break;
        case FORMAT_NOT_SUPPORTED:
            msg = "Unknown format encountered";
            break;
        case IFFERR_NOTIFF:
            msg = "This is not an IFF file";
            break;
        case IFFERR_MANGLED:
            msg = "Mangled IFF file";
            break;
        case IFFERR_SYNTAX:
            msg = "Not a correct IFF file";
            break;
        case IFFERR_EOF:
            msg = "Premature end of file";
            break;
        case NOT_YUVN:
            msg = "This is not an YUVN file";
            break;
        case PARSEIFF_FAILED:
            msg = "ParseIFF failed";
            break;
        case CANNOT_READ:
            msg = "Can't read IFF file";
            break;
        case MASKING_NOT_SUPPORTED:
            msg = "Masking is not supported";
            break;
        case IFFERR_NOMEM:
        case PERR_OUTOFMEMORY:
            msg = "Out of memory";
            break;
        case UNKNOWN_COMPRESSION:
            msg = "Unknown compression in YUVN";
            break;
        default:
            msg = NULL;
            break;
    }
    if(msg)
        SetErrorMsg(frame, msg);

}


/*
    This must always exist!
*/

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    struct Library *IFFParseBase;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    struct IFFHandle *iff;
    BOOL   res = FALSE;

    D(bug("IOCheck()\n"));

    if( (IFFParseBase = OpenLibrary("iffparse.library",36L)) == NULL) {
        return FALSE;
    }

    if( iff = AllocIFF() ) {
        iff->iff_Stream = fh;
        InitIFFasDOS( iff );

        D(bug("\tIFF handle allocated&initted\n"));

        if( OpenIFF(iff, IFFF_READ ) == 0 ) {
            StopChunk( iff, ID_YUVN, ID_YCHD );

            if( ParseIFF( iff, IFFPARSE_SCAN ) == 0) {
                D(bug("\tParsed OK\n"));
                res = TRUE;
            }

            CloseIFF( iff );
        }

        FreeIFF(iff);
    }

    CloseLibrary(IFFParseBase);

    return res;
}

/*
    BUG: Aspect ratios are not correctly decoded
    BUG: colorspace is not checked.
 */

PERROR DecodeYCHD( FRAME *frame, struct YCHD_Header *ychd, struct PPTBase *PPTBase )
{
    PIXINFO *p = frame->pix;
    PERROR res = PERR_OK;

    p->width  = ychd->ychd_Width;
    p->height = ychd->ychd_Height;
    p->DPIX   = ychd->ychd_AspectX;
    p->DPIY   = ychd->ychd_AspectY;

    D(bug("Image size: %d x %d\n",p->width,p->height));

    if( ychd->ychd_Norm == YCHD_NORM_PAL ) {
        D(bug("This is a PAL image\n"));
        p->origmodeid = PAL_MONITOR_ID;
    } else if( ychd->ychd_Norm == YCHD_NORM_NTSC ) {
        D(bug("This is an NTSC image\n"));
        p->origmodeid = NTSC_MONITOR_ID;
    }

    D(bug("Image mode: %ld",ychd->ychd_Mode ));

    switch( ychd->ychd_Mode ) {
        case YCHD_MODE_400:
        case YCHD_MODE_200:
            D(bug(" = graylevel\n"));
            p->components = 1;
            p->colorspace = CS_GRAYLEVEL;
            break;

        default:
            D(bug(" = color image\n"));
            p->components = 3;
            p->colorspace = CS_RGB;
            break;
    }

    if( ychd->ychd_Compress != 0 ) {
        PutErrMsg( frame, UNKNOWN_COMPRESSION, PPTBase );
        res = PERR_FAILED;
    }

    return res;
}

#define CLAMP(x) (((x) > 255) ? (x) = 255 : ((x) < 0) ? (x) = 0 : (x))

static
VOID YUVtoRGB( UBYTE y, BYTE u, BYTE v, UBYTE *r, UBYTE *g, UBYTE *b )
{
    LONG R, G, B;
#if 0
    *r = (v/0.877)+y;
    *b = (u/0.493)+y;
    *g = (y-0.299*r-0.114*b)/0.587;
#else
    R = 1000*y + 1140*v;
    G = 1000*y - 396*u - 581*v;
    B = 1000*y + 2029*u;

    R /= 1000;
    G /= 1000;
    B /= 1000;

    *r = CLAMP(R);
    *g = CLAMP(G);
    *b = CLAMP(B);
#endif
}

#if 0
static
VOID RGBtoYUV( ULONG r, ULONG g, ULONG b, ULONG *y, ULONG *u, ULONG *v )
{
    LONG Y, U, V;

    Y = 0.299 * r + 0.587 * g + 0.114 * b;
    U = -0.147 * r - 0.289 * g + 0.436 * b; // = 0.493*b-Y
    V = 0.615 * r - 0.515 * g + 0.100 * b;  // = 0.877*r-Y
}
#endif

VOID ConvertImage( FRAME *frame, struct PPTBase *PPTBase )
{
    WORD row, col;
    UBYTE dummy;

    InitProgress( frame, "Converting from YUV to RGB...", 0, frame->pix->height );

    for( row = 0; row < frame->pix->height; row++ ) {
        ROWPTR cp;

        if( Progress( frame, row ) )
            return;

        cp = GetPixelRow( frame, row );

        for( col = 0; col < frame->pix->width; col++ ) {
            if( frame->pix->colorspace == CS_RGB ) {
                YUVtoRGB( cp[3*col], cp[3*col+1], cp[3*col+2],
                          &cp[3*col], &cp[3*col+1], &cp[3*col+2] );
            } else {
                YUVtoRGB( cp[3*col], 0, 0,
                          &cp[3*col], &dummy, &dummy );
            }
        }

        PutPixelRow( frame, row, cp );
    }

    FinishProgress(frame);
}

ULONG DecodeDATY( struct IFFHandle *iff, FRAME *frame, struct YCHD_Header *ychd,
                  struct Library *IFFParseBase,
                  struct PPTBase *PPTBase )
{
    WORD row, col;
    UBYTE *buf;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    buf = AllocVec( ychd->ychd_Width, MEMF_CLEAR );

    InitProgress( frame, "Loading DATY hunk...", 0, frame->pix->height );

    for( row = 0; row < frame->pix->height; row++ ) {
        ROWPTR cp, dcp;

        if( Progress( frame, row ) )
            return 0L;

        dcp = cp = GetPixelRow( frame, row );
        ReadChunkBytes( iff, buf, ychd->ychd_Width );

        for( col = 0; col < frame->pix->width; col++ ) {
            *dcp = buf[col];
            dcp += frame->pix->components;
        }

        PutPixelRow( frame, row, cp );
    }

    FreeVec(buf);

    FinishProgress( frame );

    return PERR_OK;
}

ULONG DecodeDATV( struct IFFHandle *iff, FRAME *frame, struct YCHD_Header *ychd,
                  struct Library *IFFParseBase,
                  struct PPTBase *PPTBase )
{
    WORD row, col;
    UBYTE *buf;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    ULONG bytes, step;

    D(bug("DecodeDATV()\n"));

    switch( ychd->ychd_Mode ) {
        case YCHD_MODE_400:
        case YCHD_MODE_200:
            return 0; /* Fall silently */

        case YCHD_MODE_411:
            bytes = ychd->ychd_Width / 4;
            step  = 4;
            break;

        case YCHD_MODE_422:
        case YCHD_MODE_211:
            bytes = ychd->ychd_Width / 2;
            step  = 2;
            break;

        default:
            bytes = ychd->ychd_Width;
            step  = 1;
            break;
    }

    buf = AllocVec( bytes, MEMF_CLEAR );

    InitProgress( frame, "Loading DATV hunk...", 0, frame->pix->height );

    for( row = 0; row < frame->pix->height; row++ ) {
        ROWPTR cp, dcp;

        if( Progress( frame, row ) )
            return 0L;

        cp = GetPixelRow( frame, row );
        ReadChunkBytes( iff, buf, bytes );

        dcp = cp+2; // G is V

        for( col = 0; col < frame->pix->width; col++ ) {
            *dcp = buf[col/step] - 128;
            dcp += frame->pix->components;
        }

        PutPixelRow( frame, row, cp );
    }

    FreeVec(buf);

    FinishProgress( frame );

    return PERR_OK;
}

ULONG DecodeDATU( struct IFFHandle *iff, FRAME *frame, struct YCHD_Header *ychd,
                  struct Library *IFFParseBase,
                  struct PPTBase *PPTBase )
{
    WORD row, col;
    UBYTE *buf;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    ULONG bytes, step;

    D(bug("DecodeDATU()\n"));

    switch( ychd->ychd_Mode ) {
        case YCHD_MODE_400:
        case YCHD_MODE_200:
            return 0; /* Fall silently */

        case YCHD_MODE_411:
            bytes = ychd->ychd_Width / 4;
            step  = 4;
            break;

        case YCHD_MODE_422:
        case YCHD_MODE_211:
            bytes = ychd->ychd_Width / 2;
            step  = 2;
            break;

        default:
            bytes = ychd->ychd_Width;
            step  = 1;
            break;
    }

    buf = AllocVec( bytes, MEMF_CLEAR );

    InitProgress( frame, "Loading DATU hunk...", 0, frame->pix->height );

    for( row = 0; row < frame->pix->height; row++ ) {
        ROWPTR cp, dcp;

        if( Progress( frame, row ) )
            return 0L;

        cp = GetPixelRow( frame, row );
        ReadChunkBytes( iff, buf, bytes );

        dcp = cp+1; // B is U

        for( col = 0; col < frame->pix->width; col++ ) {
            *dcp = buf[col/step] - 128;
            dcp += frame->pix->components;
        }

        PutPixelRow( frame, row, cp );
    }

    FreeVec(buf);

    FinishProgress( frame );

    return PERR_OK;
}


IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    struct IFFHandle *iff;
    struct StoredProperty *sp;
    struct YCHD_Header *ychd;
    PERROR error = PERR_OK;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    struct Library  *IFFParseBase = NULL, *UtilityBase = PPTBase->lb_Utility;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    D(bug(MYNAME": Load()\n"));

    if( (IFFParseBase = OpenLibrary("iffparse.library",36L)) == NULL) {
        PutErrMsg(frame,IFFPARSE_DID_NOT_OPEN,PPTBase);
        return PERR_FAILED; /* Big time error :( */
    }

    InitProgress( frame, "Decoding YUVN image...", 0, 1 );

    if( iff = AllocIFF() ) {
        iff->iff_Stream = fh;
        InitIFFasDOS( iff );

        D(bug("\tIFF handle allocated&initted\n"));

        if( OpenIFF(iff, IFFF_READ ) == 0 ) {
            PropChunk( iff, ID_YUVN, ID_YCHD );
            PropChunk( iff, ID_YUVN, ID_ANNO );
            PropChunk( iff, ID_YUVN, ID_AUTH );

            StopChunk( iff, ID_YUVN, ID_DATY ); /* Luminance */
            StopChunk( iff, ID_YUVN, ID_DATU ); /* Optional, color only */
            StopChunk( iff, ID_YUVN, ID_DATV ); /* Optional, color only */
            StopChunk( iff, ID_YUVN, ID_DATA ); /* Alpha channel information */

            if( (error = ParseIFF( iff, IFFPARSE_SCAN )) == 0) {
                D(bug("\tParsed OK\n"));

                if( sp = FindProp(iff, ID_YUVN, ID_YCHD) ) {
                    D(bug("\tlocated YCHD\n"));

                    ychd = sp->sp_Data;

                    if( DecodeYCHD( frame, ychd, PPTBase ) == PERR_OK ) {

                        if( InitFrame( frame ) == PERR_OK) {

                            if( sp = FindProp(iff, ID_YUVN, ID_AUTH) ) {
                                AddExtension(frame,EXTNAME_AUTHOR, sp->sp_Data, sp->sp_Size, EXTF_CSTRING );
                            }

                            if( sp = FindProp(iff, ID_YUVN, ID_ANNO) ) {
                                AddExtension(frame,EXTNAME_ANNO, sp->sp_Data, sp->sp_Size, EXTF_CSTRING );
                            }


                            if( (error = DecodeDATY( iff, frame, ychd, IFFParseBase, PPTBase )) != PERR_OK ) {
                                PutErrMsg( frame, error,PPTBase );
                            }

                            if( (error = ParseIFF( iff, IFFPARSE_SCAN )) == 0 ) {
                                if( (error = DecodeDATU( iff, frame, ychd, IFFParseBase, PPTBase )) != PERR_OK ) {
                                    PutErrMsg( frame, error,PPTBase );
                                }

                                if( (error = ParseIFF( iff, IFFPARSE_SCAN )) == 0 ) {

                                    if( (error = DecodeDATV( iff, frame, ychd, IFFParseBase, PPTBase )) != PERR_OK ) {
                                        PutErrMsg( frame, error,PPTBase );
                                    }
                                }
                            }

                            ConvertImage( frame, PPTBase );

                        } else { /* InitFrame failed. */
                            PutErrMsg( frame, SETUP_FAILED, PPTBase );
                            error = PERR_FAILED;
                        }
                    } else {
                        error = PERR_FAILED;
                    }
                } else {
                    PutErrMsg( frame, NOT_YUVN, PPTBase );
                    error = PERR_FAILED;
                }
            } /* ParseIFF() */
            else {
                PutErrMsg( frame, error, PPTBase );
                error = PERR_FAILED;
            }

            CloseIFF(iff);

        } else { /* OpenIFF() */
            PutErrMsg( frame, IFFERR_NOTIFF, PPTBase );
            error = PERR_FAILED;
        }
        FreeIFF( iff );
    }

    CloseLibrary(IFFParseBase);
    return error;

}

/*
    Format can be any of CSF_* - flags
*/

IOSAVE(fh,format,frame,tags,PPTBase,IOMOduleBase)
{
    D(bug("IOSave(type=%08X)\n",format));
    SetErrorMsg(frame,"Saving disabled.");
    return PERR_UNKNOWNTYPE;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

