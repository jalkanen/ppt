/*
    PROJECT: ilbm.loader
    MODULE : ilbmsave.c

    $Id: ilbmsave.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/

// #define DEBUG_MODE

#include "ilbm.h"

#include <exec/memory.h>

#include <string.h>

/*------------------------------------------------------------------------------*/

void InitBitMapHeader( struct BitMapHeader *bmhd, FRAME *frame, UBYTE nplanes, struct PPTBase *PPTBase )
{
    bmhd->w = frame->pix->width;
    bmhd->h = frame->pix->height;
    bmhd->x = bmhd->y = 0;
    bmhd->nPlanes = nplanes;
    bmhd->masking = mskNone;
    bmhd->compression = cmpByteRun1; /* BUG! */
    bmhd->xAspect = frame->pix->DPIX;
    bmhd->yAspect = frame->pix->DPIY;
    bmhd->pageWidth = bmhd->w;
    bmhd->pageHeight = bmhd->h;
}

/*
    Write a 24bit ilbm BODY to iffhandle
*/

int WriteBody( struct Library *IFFParseBase, struct IFFHandle *iff, FRAME *frame, struct PPTBase *PPTBase )
{
    WORD row, col;
    ROWPTR planes[24], buffer = NULL;
    ULONG rowlen = BytesPerRow( frame->pix->width );
    ULONG filebytes = RowBytes( frame->pix->width );
    ULONG buflen = MaxPackedSize( RowBytes(frame->pix->width) );
    int error = PERR_OK;
    BYTE compression = cmpByteRun1;
    // BYTE compression = cmpNone;
    APTR SysBase = PPTBase->lb_Sys, DOSBase = PPTBase->lb_DOS;

    /* Initialize */

    D(bug("Initializing...\n"));
    for(row = 0; row < 24; row++) {
        planes[row] = AllocVec( rowlen, MEMF_CLEAR );
        if(!planes[row]) {
            D(bug("memory error\n"));
            error = PERR_OUTOFMEMORY;
            goto errexit;
        }
    }
    buffer = AllocVec(buflen, MEMF_CLEAR);
    if(!buffer) {
        error = PERR_OUTOFMEMORY;
        goto errexit;
    }

    D(bug("Past initialization\n"));

    /* Convert */
    for(row = 0; row < frame->pix->height; row++) {
        ROWPTR cp;
        WORD ccol;
        UBYTE bit;

        cp = GetPixelRow( frame, row );

        D(bug("Converting pixel row %d\n",row));

        /*
         *  Data is now in 24bit chunky. We need to transform it into 24 bit
         *  deep bitplanes.
         */

        ccol = 0; bit = 0x80;
        for( col = 0; col < frame->pix->width; col++) {
            signed short plane;
            UBYTE val, mask;

            val = *cp++; /* BLUE */
            mask = 0x80;
            for( plane = 7; plane >= 0; plane--) {
                planes[plane][ccol] |= ((val & mask) ? bit : 0);
                mask = (mask >> 1);
            }

            val = *cp++; /* GREEN */
            mask = 0x80;
            for( plane = 15; plane >= 8; plane--) {
                planes[plane][ccol] |= ((val & mask) ? bit : 0);
                mask = (mask >> 1);
            }

            val = *cp++; /* RED */
            mask = 0x80;
            for( plane = 23; plane >= 16; plane--) {
                planes[plane][ccol] |= ((val & mask) ? bit : 0);
//                D(bug("plane = %d, ccol = %d, bit = %02X: Write bit %02X w/ mask %02X\n",
//                        plane,ccol,bit, ((val & mask) ? bit : 0), mask ));
//                Delay(50);
                mask = (mask >> 1);
            }

            bit >>= 1;
            if( bit == 0 ) {
                bit = 0x80;
                ccol++;
            }
        }

        /*
         *  Write to disk and clear the blocks while we're at it.
         */

        for(col = 0; col < 24; col++) {

            if(compression == cmpNone) {
                // PDebug("\tWriting plane %d\n",col);
                if(WriteChunkBytes( iff, planes[col], rowlen ) != rowlen) {
                    error = IFFERR_WRITE;
                    goto errexit;
                }
                // wplanes[col] += rowlen;
            } else {
                ROWPTR buf, wplane;
                ULONG packrowlen;

                buf = buffer; wplane = planes[col];
                packrowlen = packrow( &wplane, &buf, filebytes );
                if(WriteChunkBytes( iff, buffer, packrowlen ) != packrowlen) {
                    error = IFFERR_WRITE;
                    goto errexit;
                }
            }
            bzero( planes[col], rowlen );
        }

        if(Progress(frame,row))
            break;
    }

    /* Free resources */
errexit:
    for(row = 0; row < 24 && planes[row]; row++) {
        FreeVec(planes[row]);
    }
    if(buffer) FreeVec(buffer);

    return error;
}

/*
    Write a standard ILBM body for bitmapped code.
*/

int WriteBMBody( struct Library *IFFParseBase, struct IFFHandle *iff, FRAME *frame, struct PPTBase *PPTBase )
{
    DISPLAY *disp = frame->disp;
    WORD row, col;
    ROWPTR buffer = NULL;
    ULONG rowlen = BytesPerRow(frame->pix->width);
    ULONG buflen = MaxPackedSize( RowBytes(frame->pix->width) );
    ULONG filerowlen = RowBytes( frame->pix->width );
    int error = PERR_OK;
    BYTE compression = cmpByteRun1;
    APTR SysBase = PPTBase->lb_Sys, DOSBase = PPTBase->lb_DOS;
    UBYTE depth = disp->depth;
    PLANEPTR planes[8];

    /*
     *  Allocate buffers and initialize plane pointers
     */

    D(bug("WriteBMBody( frame = %08X )\n",frame));

    for(row = 0; row < depth; row++) {
        planes[row] = AllocVec( rowlen, MEMF_CLEAR );
        if(!planes[row]) {
            D(bug("memory error\n"));
            error = PERR_OUTOFMEMORY;
            goto errexit;
        }
    }

    buffer = AllocVec(buflen, MEMF_CLEAR);
    if(!buffer) {
        error = PERR_OUTOFMEMORY;
        goto errexit;
    }

    D(bug("\tPast initialization\n"));

    for(row = 0; row < frame->pix->height; row++) {
        UBYTE *pp, bit;
        WORD ccol;
        int i;

        D(bug("\tWriting row %d\n",row));

        pp = GetBitMapRow( frame, row );

        /*
         *  Make the C2P conversion
         *  BUG: Should use internal Chunky2Planar
         *  BUG: Slow and inefficient
         *  BUG: Only 8-bit depth
         */

        for( i = 0; i < depth; i++ )
            bzero( planes[i], rowlen);

        bit = 0x80; ccol = 0;
        for( col = 0; col < frame->pix->width; col++) {
            signed short plane;
            UBYTE val, mask;

            val = *pp++;
            mask = 0x80 >> (8-depth);
            for( plane = depth-1; plane >= 0; plane--) {
                planes[plane][ccol] |= ((val & mask) ? bit : 0);
                mask = (mask >> 1);
            }

            bit >>= 1;
            if( bit == 0 ) {
                bit = 0x80;
                ccol++;
            }
        }

        /*
         *  Write each row to disk.
         */

        for(col = 0; col < depth; col++) {
            if(compression == cmpNone) {
                D(bug("\t\tWriting plane %d\n",col));
                if(WriteChunkBytes( iff, planes[col], filerowlen ) != filerowlen) {
                    error = IFFERR_WRITE;
                    goto errexit;
                }
                // planes[col] += rowlen;
            } else {
                ROWPTR buf;
                ULONG packrowlen;
                PLANEPTR wplane;

                D(bug("\t\tPacking plane %d...\n",col));
                buf = buffer; wplane = planes[col];
                packrowlen = packrow( &wplane, &buf, filerowlen );
                // planes[col] += rowlen - filerowlen;
                if(WriteChunkBytes( iff, buffer, packrowlen ) != packrowlen) {
                    error = IFFERR_WRITE;
                    goto errexit;
                }
            }
        }

        if(Progress(frame,row))
            break;
    }

    /* Free resources */
errexit:
    for(row = 0; row < depth && planes[row]; row++) {
        FreeVec(planes[row]);
    }

    if(buffer) FreeVec(buffer);

    return error;
}

