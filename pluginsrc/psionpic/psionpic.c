/*
    PROJECT: ppt
    MODULE:  psionpic

    Psion .PIC file loader.  PPT and this file are (C) Janne Jalkanen 1998-1999.

    $Id: psionpic.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/

#undef DEBUG_MODE

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "PsionPIC"

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "This loads Psion PIC files,\n"
    "as used by the Series 3x.";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {

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

    PPTX_RexxTemplate,  (ULONG)"",

    PPTX_PreferredPostFix,(ULONG)".pic",
    PPTX_PostFixPattern,(ULONG)"#?.(pic)",

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

BOOL CheckPIC( UBYTE *buf )
{
    if( buf[0] == 'P' && buf[1] == 'I' && buf[2] == 'C' && buf[3] == 0xDC ) {
        return TRUE;
    }

    return FALSE;
}

/*
    This must always exist!
*/

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    D(bug("IOCheck()\n"));
    return( CheckPIC(buf) );
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    UBYTE buf[12];
    PERROR res = PERR_OK;
    UWORD numBitmaps, i, row, rowlen;
    UBYTE *bitmaps[8] = {NULL}, *tmp[8];

    D(bug("IOLoad()\n"));

    SetIoErr( 0L );
    FRead( fh, buf, 8, 1 );
    if( !CheckPIC( buf ) ) {
        SetErrorMsg( frame, "This is not a PIC file!" );
        return PERR_FAILED;
    }

    if( buf[4] != 0x30 || buf[5] != 0x30 ) {
        SetErrorMsg( frame, "This PIC file format not supported!" );
        return PERR_FAILED;
    }

    numBitmaps = buf[6] + buf[7]*256; /* Yap, it's little-endian */

    InitProgress( frame, "Loading Psion PIC...", 0, numBitmaps*2 );

    if( numBitmaps > 8 ) {
        SetErrorMsg( frame, "Only bitmaps that have less than 256 colors are supported." );
        return PERR_FAILED;
    }

    /*
     *  Read in header
     */

    for( i = 0; i < numBitmaps; i++ ) {
        WORD width, height, size;
        LONG offset, oldoffset;

        if( Progress( frame, i ) ) goto errorexit;

        FRead( fh, buf, 12, 1 ); /* Header */

        width  = buf[2] + buf[3]*256;
        height = buf[4] + buf[5]*256;
        size   = buf[6] + buf[7]*256;
        offset = buf[8] + buf[9]*256 + buf[10]*256*256 + buf[11]*256*256*256;

        /*
         *  Initialize the frame, if this is the first bitmap
         */

        if( i == 0 ) {
            frame->pix->height     = height;
            frame->pix->width      = width;
            frame->pix->components = 1;
            frame->pix->colorspace = CS_GRAYLEVEL;
            frame->pix->origdepth  = numBitmaps;
            if( InitFrame( frame ) != PERR_OK ) {
                return PERR_FAILED;
            }
        } else if( frame->pix->height != height || frame->pix->width != width ) {
            SetErrorMsg(frame,"Bitmaps must be the same size.");
            res = PERR_FAILED;
            goto errorexit;
        }

        rowlen = ((width+15) / 16) * 2; /* in bytes */

        D(bug("\tBitmap %d: %dx%d (%d bytes, offset=%ld, rowlen=%d)\n",i,width,height,size,offset,rowlen));

        /*
         *  Allocate space for the bitmap.
         */

        if( NULL == (bitmaps[i] = AllocVec( height * rowlen, 0L ))) {
            SetErrorCode( frame, PERR_OUTOFMEMORY );
            res = PERR_FAILED;
            goto errorexit;
        }

        tmp[i] = bitmaps[i]; /* make a copy */

        /*
         *  Read in the bitmap
         */

        oldoffset = Seek( fh, offset, OFFSET_CURRENT );
        if( oldoffset > 0 ) {
            for( row = 0; row < height; row++ ) {
                FRead( fh, bitmaps[i]+rowlen*row, rowlen, 1 );
                D(DumpMem( bitmaps[i]+rowlen*row, rowlen, 'b' ));
            }
            Seek( fh, oldoffset, OFFSET_BEGINNING );
        } else {
            SetErrorCode( frame, PERR_FILEREAD );
            res = PERR_FAILED;
            goto errorexit;
        }
    }

    /*
     *  Bitmaps are in memory, so put them into the frame
     */


    for( row = 0; row < frame->pix->height; row++ ) {
        ROWPTR cp;
        WORD col;
        UWORD mask;

        cp = GetPixelRow( frame, row );

        /*
         *  Makes the actual conversion to grayscales.
         */

        mask = 0x1;
        for( col = 0; col < frame->pix->width; col++ ) {
            LONG offset;

            offset = row*rowlen + col/8;
            switch(numBitmaps) {
                WORD val;

                case 1:
                    cp[col] = (bitmaps[0][offset] & mask) ? 0 : 255;
                    break;

                case 2:
                    val = ((bitmaps[0][offset] & mask) ? 0 : 255)
                           - ((bitmaps[1][offset] & mask) ? 128 : 0);
                    if( val < 0 ) val = 0; /* Clamp */
                    cp[col] = val;
                    break;

                default:
                    break;
            }
            mask = mask << 1;
            if( mask > 0x80 ) mask = 1;
        }

        for( i = 0; i < numBitmaps; i++ ) tmp[i] += rowlen;

        PutPixelRow( frame, row, cp );
    }

    FinishProgress( frame );

errorexit:
    for( i = 0; i < numBitmaps; i++ ) {
        if( bitmaps[i] ) FreeVec( bitmaps[i] );
    }

    return res;
}

STATIC INLINE
VOID WriteUWord( BPTR fh, UWORD w, struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    FPutC( fh, (UBYTE) w );
    FPutC( fh, (UBYTE) (w >> 8) );
}

STATIC INLINE
VOID WriteULong( BPTR fh, ULONG l, struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    FPutC( fh, (UBYTE) l );
    FPutC( fh, (UBYTE) (l >> 8) );
    FPutC( fh, (UBYTE) (l >> 16) );
    FPutC( fh, (UBYTE) (l >> 24) );
}

/*
    Format can be any of CSF_* - flags
*/
IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    PERROR res = PERR_OK;
    UWORD depth = frame->disp->depth, bp, height = frame->pix->height;
    ULONG size, rowlen;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    D(bug("IOSave(type=%08X)\n",format));

    if( depth > 2 ) {
        SetErrorMsg(frame, "Only depths <= 2 supported");
        return PERR_ERROR;
    }

    InitProgress( frame, "Saving Psion PIC file...", 0, height*depth );

    FPutC( fh, 'P' );
    FPutC( fh, 'I' );
    FPutC( fh, 'C' );
    FPutC( fh, 0xDC );
    FPutC( fh, 0x30 );
    FPutC( fh, 0x30 );

    WriteUWord( fh, depth, PPTBase );

    rowlen = ((frame->pix->width+15) / 16) * 2; /* in bytes */
    size   = rowlen * height;

    for( bp = 0; bp < depth; bp++ ) {
        ULONG offset;

        WriteUWord( fh, 0, PPTBase );
        WriteUWord( fh, frame->pix->width, PPTBase );
        WriteUWord( fh, height, PPTBase );
        WriteUWord( fh, size, PPTBase );

        offset = size * bp + (depth-bp-1)*12;

        WriteULong( fh, offset, PPTBase );
    }

    /*
     *  Then, write bitmap data
     */

    for( bp = 0; bp < depth; bp++ ) {
        WORD row;

        for( row = 0; row < height; row++ ) {
            UBYTE *buffer, c;
            UWORD mask;
            LONG bytes;
            WORD col;

            if( Progress( frame, bp*height+row ) ) {
                return PERR_BREAK;
            }

            buffer = GetBitMapRow( frame, row );
            c = 0;
            mask = 0x01;
            bytes = 0;

            D(DumpMem( buffer, frame->pix->width, 'x' ));

            for( col = 0; col < frame->pix->width; col++ ) {
                UBYTE bit;
                bit = buffer[col] & (1<<bp);

                switch(bp) {
                    case 0:
                        c |= bit ? 0 : mask; /* Black is on */
                        break;
                    default:
                        c |= bit ? mask : 0; /* Grey is on */
                        break;
                }

                if( (mask <<= 1) > 0x80 ) {
                    D(bug("%d,%d: %x\n",row, col, c));
                    FPutC( fh, c );
                    c = 0;
                    mask = 0x01;
                    bytes++;
                }
            }
            // FPutC( fh, c );
            while( bytes++ < rowlen ) FPutC( fh, 0 );
        }
    }

    FinishProgress( frame );

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

