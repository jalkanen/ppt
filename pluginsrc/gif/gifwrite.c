/*
    PROJECT: ppt module

    This is loaned from the netpbm distribution.  Original copyright
    notice appears below.
*/

/* ppmtogif.c - read a portable pixmap and produce a GIF file
**
** Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
** Lempel-Zim compression based on "compress".
**
** Modified by Marcel Wijkstra <wijkstra@fwi.uva.nl>
**
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** The Graphics Interchange Format(c) is the Copyright property of
** CompuServe Incorporated.  GIF(sm) is a Service Mark property of
** CompuServe Incorporated.
*/
/*---------------------------------------------------------------------------*/
/* Includes */

#include "GIF.h"
#include <ctype.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <strings.h>

/*
    The code will be compiled in only if the save mode is supported.
*/
#ifdef SUPPORT_SAVE

/*---------------------------------------------------------------------------*/
/* Type defines */

/*---------------------------------------------------------------------------*/
/* Prototypes */

static void Putword( int, BPTR, struct PPTBase * );
static PERROR compress( struct GIFSave *gs );
static PERROR output( code_int code, struct GIFSave *gs );
static void cl_block ( struct GIFSave *gs );
static void cl_hash(count_int hsize, struct GIFSave *gs);
static void char_init( struct GIFSave *gs );
static void char_out( int c, struct GIFSave *gs );
static void flush_char(struct GIFSave *gs );


/*---------------------------------------------------------------------------*/
/* Globals? */

/*---------------------------------------------------------------------------*/


/*
    BUG: Should really not copy the data...  Well, at least it's gonna
    be safe this way...
*/
int
GetPixelColor( int x, int y, struct GIFSave *gs )
{
    int color;
    UBYTE *bmp;
    struct PPTBase *PPTBase = gs->PPTBase;
    int Width = gs->frame->pix->width;

    /*
     *  If buffer was already filled, we won't bother reading it again.
     *  But if it wasn't, read and copy to the pixelbuffer.
     */

    if( gs->lasty != y ) {
        bmp = GetBitMapRow( gs->frame, y );
        bcopy( bmp, gs->pixelbuffer, Width );
        gs->lasty = y;
    }

    color = gs->pixelbuffer[x];

    return color;
}


/*****************************************************************************
 *
 * GIFENCODE.C    - GIF Image compression interface
 *
 * GIFEncode( FName, GHeight, GWidth, GInterlace, Background, Transparent,
 *            BitsPerPixel, Red, Green, Blue, GetPixel )
 *
 *****************************************************************************/


/*
 * Bump the 'curx' and 'cury' to point to the next pixel
 */
static PERROR
BumpPixel( struct GIFSave *gs )
{
    int Width = gs->frame->pix->width;
    int Height = gs->frame->pix->height;
    struct PPTBase *PPTBase = gs->PPTBase;

    /*
     * Bump the current X position
     */
    ++(gs->curx);

    /*
     * If we are at the end of a scan line, set curx back to the beginning
     * If we are interlaced, bump the cury to the appropriate spot,
     * otherwise, just increment it.
     */
    if( (gs->curx) == Width ) {
        gs->curx = 0;

        if( !gs->interlace ) {
            ++(gs->cury);
            if( Progress( gs->frame, gs->MaxCount - gs->CountDown ) ) {
                SetErrorCode( gs->frame, PERR_BREAK );
                return PERR_ERROR;
            }

        } else {
            switch( gs->Pass ) {

                case 0:
                    gs->cury += 8;
                    if( gs->cury >= Height ) {
                        ++(gs->Pass);
                        gs->cury = 4;
                    }
                    break;

                case 1:
                    gs->cury += 8;
                    if( gs->cury >= Height ) {
                        ++(gs->Pass);
                        gs->cury = 2;
                    }
                    break;

                case 2:
                    gs->cury += 4;
                    if( gs->cury >= Height ) {
                        ++(gs->Pass);
                        gs->cury = 1;
                    }
                    break;

                case 3:
                    gs->cury += 2;
                    break;
            }

            if( Progress( gs->frame, gs->MaxCount - gs->CountDown ) ) {
                SetErrorCode( gs->frame, PERR_BREAK );
                return PERR_ERROR;
            }
        }
    }

    return PERR_OK;
}

/*
 *  Return the next pixel from the image
 *  Returns EOF on error or file end.
 */
static int
GIFNextPixel( struct GIFSave *gs )
{
    int r;

    if( gs->CountDown == 0 )
        return EOF;

    --(gs->CountDown);

    r = ( * (gs->getpixel) )( gs->curx, gs->cury, gs );

    if( BumpPixel( gs ) != PERR_OK ) return EOF;

    return r;
}

/* public */

/*
    OK.
*/
PERROR GIFEncode( struct GIFSave *gs )
{
    int B;
    int RWidth, RHeight;
    int LeftOfs, TopOfs;
    int Resolution;
    int ColorMapSize;
    int InitCodeSize;
    PERROR res = PERR_OK;
    int i;
    struct PPTBase *PPTBase = gs->PPTBase;
    FRAME *frame = gs->frame;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    BPTR fh = gs->fh;

    ColorMapSize = 1 << gs->BitsPerPixel;

    RWidth = frame->pix->width;
    RHeight  = frame->pix->height;
    LeftOfs = TopOfs = 0;

    Resolution = gs->BitsPerPixel;

    D(bug("Writing out %s %s.  Transparency = %d\n",
           gs->interlace ? "interlaced" : "non-interlaced",
           gs->transparent >= 0 ? "GIF89a" : "GIF87a",
           gs->transparent ));

    /*
     * Calculate number of bits we are expecting
     */
    gs->MaxCount = gs->CountDown = (long)RWidth * (long)RHeight;

    InitProgress( frame, "Writing CompuServe GIF...", 0, gs->CountDown );

    /*
     * Indicate which pass we are on (if interlace)
     */
    gs->Pass = 0;

    /*
     * The initial code size
     */
    if( gs->BitsPerPixel <= 1 )
        InitCodeSize = 2;
    else
        InitCodeSize = gs->BitsPerPixel;

    /*
     * Set up the current x and y position
     */
    gs->curx = gs->cury = 0;

    /*
     * Write the Magic header
     */

    Write( fh, gs->transparent < 0 ? "GIF87a" : "GIF89a", 6 );

    /*
     * Write out the screen width and height
     */

    Putword( RWidth, fh, PPTBase );
    Putword( RHeight, fh, PPTBase );

    /*
     * Indicate that there is a global colour map
     */
    B = 0x80;       /* Yes, there is a color map */

    /*
     * OR in the resolution
     */
    B |= (Resolution - 1) << 5;

    /*
     * OR in the Bits per Pixel
     */
    B |= (gs->BitsPerPixel - 1);

    /*
     * Write it out
     */
    FPutC( fh, B );

    /*
     * Write out the Background colour
     */
    FPutC( fh, gs->background );

    /*
     * Byte of 0's (future expansion)
     */
    FPutC( fh, 0 );

    /*
     * Write out the Global Colour Map
     */
    for( i = 0; i < ColorMapSize; i++ ) {
        FPutC( fh, gs->colormap[i].r );
        FPutC( fh, gs->colormap[i].g );
        FPutC( fh, gs->colormap[i].b );
    }

    /*
     * Write out extension for transparent colour index, if necessary.
     */
    if ( gs->transparent >= 0 ) {
        FPutC( fh, '!' );
        FPutC( fh, 0xf9 );
        FPutC( fh, 4 );
        FPutC( fh, 1 );
        FPutC( fh, 0 );
        FPutC( fh, 0 );
        FPutC( fh, gs->transparent );
        FPutC( fh, 0 );
    }

    /*
     * Write an Image separator
     */
    FPutC( fh, ',' );

    /*
     * Write the Image header
     */

    Putword( LeftOfs, fh, PPTBase );
    Putword( TopOfs, fh, PPTBase );
    Putword( RWidth, fh, PPTBase );
    Putword( RHeight, fh, PPTBase );

    /*
     * Write out whether or not the image is interlaced
     */
    if( gs->interlace )
        FPutC( fh, 0x40 );
    else
        FPutC( fh, 0x00 );

    /*
     * Write out the initial code size
     */
    FPutC( fh, InitCodeSize );

    /*
     * Go and actually compress the data
     */

    gs->g_init_bits = InitCodeSize+1;

    if( (res = compress( gs )) == PERR_OK ) {

        /*
         * Write out a Zero-length packet (to end the series)
         */
        FPutC( fh, 0 );

        /*
         * Write the GIF file terminator
         */
        FPutC( fh, ';' );
        FinishProgress( frame );
    }

    return res;
}

/*
 *  Write out a word to the GIF file
 *  OK.
 */
static void
Putword( int w, BPTR fh, struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    FPutC( fh, w & 0xff );
    FPutC( fh, (w / 256) & 0xff );
}


/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/

/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */



static PERROR
compress( struct GIFSave *gs )
{
    register long fcode;
    register code_int i /* = 0 */;
    register int c;
    register code_int ent;
    register code_int disp;
    register code_int hsize_reg;
    register int hshift;
    PERROR   res = PERR_OK;

    /*
     * Set up the necessary values
     */
    gs->offset = 0;
    gs->out_count = 0;
    gs->clear_flg = 0;
    gs->in_count = 1;
    gs->maxcode = MAXCODE(gs->n_bits = gs->g_init_bits);

    gs->ClearCode = (1 << (gs->g_init_bits - 1));
    gs->EOFCode = gs->ClearCode + 1;
    gs->free_ent = gs->ClearCode + 2;

    char_init( gs );

    ent = GIFNextPixel( gs );

    hshift = 0;
    for ( fcode = (long) gs->hsize;  fcode < 65536L; fcode *= 2L )
        ++hshift;
    hshift = 8 - hshift;                /* set hash code range bound */

    hsize_reg = gs->hsize;
    cl_hash( (count_int) hsize_reg, gs );            /* clear hash table */

    res = output( (code_int)gs->ClearCode, gs );

    while ( (c = GIFNextPixel( gs )) != EOF && res == PERR_OK ) {
        gs->in_count++;

        fcode = (long) (((long) c << gs->maxbits) + ent);
        i = (((code_int)c << hshift) ^ ent);    /* xor hashing */

        if ( HashTabOf (i) == fcode ) {
            ent = CodeTabOf (i);
            continue;
        } else if ( (long)HashTabOf (i) < 0 )      /* empty slot */
            goto nomatch;
        disp = hsize_reg - i;           /* secondary hash (after G. Knott) */
        if ( i == 0 )
            disp = 1;
probe:
        if ( (i -= disp) < 0 )
            i += hsize_reg;

        if ( HashTabOf (i) == fcode ) {
            ent = CodeTabOf (i);
            continue;
        }
        if ( (long)HashTabOf (i) > 0 )
            goto probe;
nomatch:
        res = output ( (code_int) ent, gs );
        gs->out_count++;
        ent = c;

        if ( gs->free_ent < gs->maxmaxcode ) {
            CodeTabOf (i) = gs->free_ent++; /* code -> hashtable */
            HashTabOf (i) = fcode;
        } else {
            cl_block(gs);
        }
    }

    if( gs->CountDown == 0 ) {

        /*
         * All written, put out the final code.
         */

        res = output( (code_int)ent, gs );
        gs->out_count++;

        res = output( (code_int) gs->EOFCode, gs );
    } else {
        res = PERR_FAILED;
    }

    return res;
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

const unsigned long masks[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
                                0x001F, 0x003F, 0x007F, 0x00FF,
                                0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

static PERROR
output( code_int code, struct GIFSave *gs )
{
    struct PPTBase *PPTBase = gs->PPTBase;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    gs->cur_accum &= masks[ gs->cur_bits ];

    if( gs->cur_bits > 0 )
        gs->cur_accum |= ((long)code << gs->cur_bits);
    else
        gs->cur_accum = code;

    gs->cur_bits += gs->n_bits;

    while( gs->cur_bits >= 8 ) {
        char_out( (unsigned int)(gs->cur_accum & 0xff), gs );
        gs->cur_accum >>= 8;
        gs->cur_bits -= 8;
    }

    /*
     * If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
    if ( gs->free_ent > gs->maxcode || gs->clear_flg ) {

        if( gs->clear_flg ) {

            gs->maxcode = MAXCODE (gs->n_bits = gs->g_init_bits);
            gs->clear_flg = 0;

        } else {

            ++(gs->n_bits);
            if ( gs->n_bits == gs->maxbits ) {
                gs->maxcode = gs->maxmaxcode;
            } else {
                gs->maxcode = MAXCODE(gs->n_bits);
            }
        }
    }

    if( code == gs->EOFCode ) {
        /*
         * At EOF, write the rest of the buffer.
         */
        while( gs->cur_bits > 0 ) {
            char_out( (unsigned int)(gs->cur_accum & 0xff), gs );
            gs->cur_accum >>= 8;
            gs->cur_bits -= 8;
        }

        flush_char( gs );

        Flush( gs->fh );

        if( IoErr() ) {
            SetErrorCode( gs->frame, PERR_FILEWRITE );
            return PERR_ERROR;
        }
    }

    return PERR_OK;
}

/*
 * Clear out the hash table
 */
static void
cl_block ( struct GIFSave *gs )             /* table clear for block compress */
{
    cl_hash ( (count_int) gs->hsize, gs );
    gs->free_ent = gs->ClearCode + 2;
    gs->clear_flg = 1;

    output( (code_int)gs->ClearCode, gs );
}

/*
    BUG:  Shoud luse memset()
*/
static void
cl_hash(count_int hsize, struct GIFSave *gs)          /* reset code table */
{
    register count_int *htab_p = gs->htab+gs->hsize;
    register long i;
    register long m1 = -1;

    i = gs->hsize - 16;
    do {                            /* might use Sys V memset(3) here */
        *(htab_p-16) = m1;
        *(htab_p-15) = m1;
        *(htab_p-14) = m1;
        *(htab_p-13) = m1;
        *(htab_p-12) = m1;
        *(htab_p-11) = m1;
        *(htab_p-10) = m1;
        *(htab_p-9) = m1;
        *(htab_p-8) = m1;
        *(htab_p-7) = m1;
        *(htab_p-6) = m1;
        *(htab_p-5) = m1;
        *(htab_p-4) = m1;
        *(htab_p-3) = m1;
        *(htab_p-2) = m1;
        *(htab_p-1) = m1;
        htab_p -= 16;
    } while ((i -= 16) >= 0);

    for ( i += 16; i > 0; --i ) {
        *--htab_p = m1;
    }
}


/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/

/*
 * Set up the 'byte output' routine
 */
static void
char_init( struct GIFSave *gs )
{
    gs->a_count = 0;
}

/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void
char_out( int c, struct GIFSave *gs )
{
    gs->accum[ gs->a_count++ ] = c;
    if( gs->a_count >= 254 )
        flush_char( gs );
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static void
flush_char(struct GIFSave *gs )
{
    struct DosLibrary *DOSBase = gs->PPTBase->lb_DOS;

    if( gs->a_count > 0 ) {
        FPutC( gs->fh, gs->a_count );
        FWrite( gs->fh, gs->accum, 1, gs->a_count );
        gs->a_count = 0;
    }
}

#endif /* SUPPORT_SAVE */

/* The End */
