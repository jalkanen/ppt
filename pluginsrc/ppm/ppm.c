/*
    PROJECT: ppt
    MODULE : Loaders/ppm

    PPT and this file are (C) Janne Jalkanen 1998.

    $Id: ppm.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/


#include <exec/memory.h>

#include "ppm.h"

#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------*/

#define MYNAME          "PPM"

/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/

const char infoblurb[] = "Loads PPM/PGM images (P2,P3,P5 and P6)\n"
                         "Saves PPM/PGM images of type P5 and P6\n";

#pragma msg 186 ignore

const struct TagItem MyTagArray[] = {
    PPTX_Load,              TRUE,
    PPTX_ColorSpaces,       CSF_RGB|CSF_GRAYLEVEL,

    PPTX_Name,              MYNAME,
    PPTX_Author,            "Janne Jalkanen, 1995-1998",
    PPTX_InfoTxt,           infoblurb,
    PPTX_ReqPPTVersion,     3,

    PPTX_RexxTemplate,      "",

    PPTX_PreferredPostFix,  ".ppm",
    PPTX_PostFixPattern,    "#?.(ppm|pgm|pnm)",

    PPTX_SupportsGetArgs,   TRUE,

    TAG_DONE
};

#if defined( __GNUC__ )
const BYTE LibName[]="ppm.iomod";
const BYTE LibIdString[]="ppm.iomod 1.0";
const UWORD LibVersion=1;
const UWORD LibRevision=0;
ADD2LIST(LIBIOInquire,__FuncTable__,22);
ADD2LIST(LIBIOCheck,__FuncTable__,22);
ADD2LIST(LIBIOLoad,__FuncTable__,22);
ADD2LIST(LIBIOSave,__FuncTable__,22);
/* The following two definitions are only required
   if you link with libinitr */
ADD2LIST(LIBIOInquire,__LibTable__,22);
ADD2LIST(LIBIOCheck,__LibTable__,22);
ADD2LIST(LIBIOLoad,__LibTable__,22);
ADD2LIST(LIBIOSave,__LibTable__,22);

/* Make GNU C specific declarations. __UtilityBase is
   required for the libnix integer multiplication */
struct ExecBase *SysBase = NULL;
struct Library *__UtilityBase = NULL;
#endif

/*------------------------------------------------------------------*/

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

LIBINIT
{
#if defined(__GNUC__)
    SysBase = SYSBASE();

    if( NULL == (__UtilityBase = OpenLibrary("utility.library",37L))) {
        return 1L;
    }

#endif
    return 0;
}


LIBCLEANUP
{
#if defined(__GNUC__)
    if( __UtilityBase ) CloseLibrary(__UtilityBase);
#endif
}


/*
    Eats away all whitespaces, returning the beginning of the next string
*/
UBYTE *NextChar( UBYTE *string )
{
    UBYTE *s = string;

    while(*s == ' ' || *s == '\t') s++;

    return s;
}

UBYTE *NextEmpty( UBYTE *string )
{
    UBYTE *s = string;

    while( *s != ' ' && *s != '\t') s++;

    return s;
}

UBYTE *NextItem( UBYTE *string )
{
    UBYTE *s = string;

    s = NextEmpty( s );
    s = NextChar( s );

    return s;
}



/*
    Read one number, ignoring comments and whitespaces.

    Returns -1 on EOF, -2 on error.
*/
LONG ReadNextNumber( struct PPTBase *PPTBase, BPTR fh )
{
    LONG ch, val;
    APTR DOSBase = PPTBase->lb_DOS;

    /*
     *  Skip whitespaces & newlines
     */
    do {
        ch = FGetC( fh );
    } while( ch == ' ' || ch == '\t' || ch == '\n' );

    /*
     *  If comment, read until the end of line.
     */

    if( ch == '#' ) {
        do {
            ch = FGetC( fh );
        } while (ch != '\n' && ch != -1);
    }

    /*
     *  Is this EOF?
     */

    if( ch == EOF )
        return -1;

    /*
     *  Read the number.
     */

    if( ch < '0' || ch > '9' ) return -2; /* File read error */

    val = ch - '0';

    while( (ch = FGetC( fh )) >= '0' && ch <= '9' ) {
        val *= 10;
        val += ch - '0';
    }

    return val;
}

/* Read ASCII RGB data */
PERROR Read_P3_Row( struct PPTBase *PPTBase, ROWPTR dest, UBYTE *scale, UWORD width, BPTR fh )
{
    ROWPTR ds = dest;
    UWORD ccol = 0;

    width *= 3;

    while( ccol < width ) {
        LONG val;

        val = ReadNextNumber( PPTBase, fh );
        if( val < 0 ) return PERR_FILEREAD;
        *ds++ = (UBYTE) val;
        ccol++;
    }

    return PERR_OK;
}

/* Read ASCII GRAY data */
PERROR Read_P2_Row( struct PPTBase *PPTBase, ROWPTR dest, UBYTE *scale, UWORD width, BPTR fh )
{
    ROWPTR ds = dest;
    UWORD ccol = 0;

    while( ccol < width ) {
        LONG val;

        val = ReadNextNumber( PPTBase, fh );
        if( val < 0 ) return PERR_FILEREAD;
        *ds++ = (UBYTE) val;
        ccol++;
    }

    return PERR_OK;
}


/*
    RAW RGB data
*/
PERROR Read_P6_Row( struct PPTBase *PPTBase, ROWPTR dest, UBYTE *scale, UWORD width, BPTR fh )
{
    APTR DOSBase = PPTBase->lb_DOS;
    UWORD ccol = 0;
    ROWPTR ds = dest;

    width *= 3; /* Make it in bytes */

    while( ccol < width ) {
        LONG val;

        val = FGetC( fh );
        if( val == -1 )
            return PERR_FILEREAD;
        *ds++ = (UBYTE) val;
        ccol++;
    }
    return PERR_OK;
}

/*
    RAW GRAY data
*/
PERROR Read_P5_Row( struct PPTBase *PPTBase, ROWPTR dest, UBYTE *scale, UWORD width, BPTR fh )
{
    APTR DOSBase = PPTBase->lb_DOS;
    UWORD ccol = 0;
    ROWPTR ds = dest;

    while( ccol < width ) {
        LONG val;

        val = FGetC( fh );
        if( val == -1 )
            return PERR_FILEREAD;
        *ds++ = (UBYTE) val;
        ccol++;
    }
    return PERR_OK;
}

UBYTE *MakeScaleTable( LONG maxval, struct PPTBase *PPTBase )
{
    UBYTE *buffer;
    APTR SysBase = PPTBase->lb_Sys;
    LONG i, mv2 = maxval >> 1;

    buffer = AllocVec( maxval+1, MEMF_CLEAR );
    if(!buffer) return NULL;

    for( i = 0; i <= maxval; i++ ) {
        buffer[i] = (i * 255 + mv2) / maxval;
    }

    return buffer;
}

VOID FreeScaleTable( UBYTE *st, struct PPTBase *PPTBase )
{
    APTR SysBase = PPTBase->lb_Sys;

    FreeVec( st );
}

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    struct Library *DOSBase = PPTBase->lb_DOS;
    UWORD height, width, crow = 0;
    UBYTE msg[80], type, buf[BUFFERSIZE+1], *s;
    ULONG maxval;
    PERROR res = PERR_OK;
    UBYTE *typestr, id, *scaletable = NULL;
    PERROR (*ReadRow)( struct PPTBase *, ROWPTR, UBYTE *, UWORD, BPTR );


    // PDebug(MYNAME" : Load()\n");

    /*
     *  Recognize file type
     */

    FGets( fh, buf, BUFFERSIZE );

    // PDebug(" ID: %s\n",buf );

    if( buf[0] != 'P' ) {
        SetErrorMsg( frame, "This is not a PPM file!" );
        return PERR_FAILED;
    }

    id = buf[1];

    /*
     *  Get picture size
     */

    FGets( fh, buf, BUFFERSIZE );

    s = NextChar( buf );
    width = atol(s);
    s = NextItem( s );
    height = atol(s);

    /*
     *  Maximum value
     */

    FGets( fh, buf, BUFFERSIZE );
    s = NextChar( buf );
    maxval = atol(s);

    /*
     *  Initialize frame
     */

    frame->pix->height = height;
    frame->pix->width = width;

    switch( id ) {
        case '6': /* P6 = RAW RGB data */
            frame->pix->components = 3;
            frame->pix->origdepth = 24;
            frame->pix->colorspace = CS_RGB;
            type = PPM_P6;
            typestr = "P6";
            ReadRow = Read_P6_Row;
            break;

        case '3': /* P3 = ASCII RGB data */
            frame->pix->components = 3;
            frame->pix->origdepth = 24;
            frame->pix->colorspace = CS_RGB;
            type = PPM_P3;
            typestr = "P3";
            ReadRow = Read_P3_Row;
            scaletable = MakeScaleTable( maxval, PPTBase );
            if(!scaletable) {
                SetErrorCode( frame, PERR_OUTOFMEMORY );
                return PERR_ERROR;
            }
            break;

        case '5': /* P5 = RAW GRAY data */
            frame->pix->components = 1;
            frame->pix->origdepth = 8;
            frame->pix->colorspace = CS_GRAYLEVEL;
            type = PPM_P5;
            typestr = "P5";
            ReadRow = Read_P5_Row;
            break;

        case '2': /* P2 = ASCII GRAY data */
            frame->pix->components = 1;
            frame->pix->origdepth = 8;
            frame->pix->colorspace = CS_GRAYLEVEL;
            type = PPM_P2;
            typestr = "P2";
            ReadRow = Read_P2_Row;
            scaletable = MakeScaleTable( maxval, PPTBase );
            if(!scaletable) {
                SetErrorCode( frame, PERR_OUTOFMEMORY );
                return PERR_ERROR;
            }
            break;

        default:
            SetErrorCode( frame, PERR_UNKNOWNTYPE );
            return PERR_FAILED;
    }

    if( maxval > 255  && (type == PPM_P6 || type == PPM_P5)  ) {
        SetErrorMsg( frame, "Illegal P5/P6 maximum value" );
        return PERR_FAILED;
    }

    // PDebug(" Height=%d, Width=%d, Maxval = %d\n",height,width,maxval );

    sprintf(msg,"Loading PPM (%s) picture...", typestr );

    InitProgress( frame, msg, 0, height );

    if(InitFrame( frame ) != PERR_OK) {
        /* Do clean up */
        return PERR_FAILED;
    }

    while( crow < height ) {
        ROWPTR cp;

        if(Progress(frame, (ULONG)crow)) {
            SetErrorCode( frame, PERR_BREAK );
            res = PERR_FAILED;
            break;
        }

        cp = GetPixelRow( frame, crow );

        /*
         *  Get the actual row of pixels, then decompress it into cp
         */

        res = (*ReadRow)( PPTBase, cp, NULL, width, fh );

        if(res != PERR_OK) {
            SetErrorCode( frame, res );
            res = PERR_FAILED;
            goto errorexit;
        }

        PutPixelRow( frame, crow, cp );

        crow++;

    } /* while */

    FinishProgress( frame );

errorexit:
    /*
     *  Release allocated resources
     */

    if( scaletable ) FreeScaleTable( scaletable, PPTBase );

    return res;
}

/*
    The save routine is actually quite easy. All we currently do is decide whether
    to write GRAY or RGB data, since we don't know how to deal with ASCII files yet.
*/
IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    UWORD height = frame->pix->height, width = frame->pix->width;
    UBYTE buf[80];
    APTR DOSBase = PPTBase->lb_DOS;
    UWORD crow = 0;
    LONG bpr = frame->pix->bytes_per_row;
    PERROR res = PERR_OK;

    if( format & (CSF_LUT|CSF_ARGB) )
        return PERR_UNKNOWNTYPE;

    InitProgress( frame, "Saving PPM/PGM image...", 0, height );

    /*
     *  Write header info.
     */

    sprintf(buf, "%s\n%lu %lu\n%lu\n",
                 (frame->pix->colorspace == CS_RGB) ? "P6" : "P5",
                 width, height,
                 255 ); /* Default maxval. BUG: Might fail on future versions of ppt */

    FPuts( fh, buf );

    /*
     *  Write the actual data.
     */

    while( crow < height ) {
        ROWPTR cp;
        UWORD ccol;

        if(Progress( frame, (ULONG)crow )) {
            SetErrorCode( frame, PERR_BREAK );
            res = PERR_FAILED;
            goto errorexit;
        }

        cp = GetPixelRow( frame, crow );

        for( ccol = 0; ccol < bpr; ccol++ ) {
            if(FPutC( fh, (LONG) *cp++ ) == EOF) {
                SetErrorCode( frame, PERR_FILEWRITE );
                res = PERR_FAILED;
                goto errorexit;
            }
        }
        crow++;
    }
    FinishProgress( frame );

errorexit:

    return res;
}

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    // PDebug(MYNAME": Check()\n");

    /*
     *  Read bytes from file, then return TRUE if we can handle this
     *  type of file. If two first bytes from the file are P3, P2, P5 or P6
     *  this is a PPM/PGM file.
     */

    if( buf[0] == 'P' && ( buf[1] == '5' || buf[1] == '6' || buf[1] == '3' || buf[1] == '2' ) )
        return TRUE;

    return FALSE;
}

IOGETARGS(format,frame,tags,PPTBase,IOModuleBase)
{
    return PERR_OK;
}



