/*----------------------------------------------------------------------*/
/*
    PROJECT: ilbm.loader
    MODULE : ilbmload.c

    $Id: ilbmload.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "ilbm.h"
#include <ppt.h>
#include <pragmas/pptsupp_pragmas.h>
#include <libraries/iffparse.h>
#include <exec/memory.h>

#include <string.h>
#include <stdio.h>

#define HAM6   0
#define HAM8   1
#define EHB    2
#define ILBM24 3
#define BITMAP 4

/*----------------------------------------------------------------------*/
/* Global variables */


/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Code */


/*
    Initializes a frame.

    NOTE! Call first
*/

void SetUp( FRAME *f, struct PPTBase *PPTBase, struct BitMapHeader *bmhd )
{
    D(bug("\tSetUp()\n"));

    f->pix->height = bmhd->h;
    f->pix->width  = bmhd->w;
    f->pix->components = 3;
    f->pix->colorspace = CS_RGB;
    f->pix->origdepth = bmhd->nPlanes;
    f->pix->DPIX = bmhd->xAspect;
    f->pix->DPIY = bmhd->yAspect;
}

/*
    BUG: really slow.
    NOTE! Call second
*/

UBYTE *SetUpColors( struct PPTBase *PPTBase, struct StoredProperty *sp, ULONG modeid )
{
    UBYTE *rgb, *colors;
    int i, idx;
    ULONG ncolors;
    UBYTE r,g,b;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    ncolors = sp->sp_Size / 3;
    if( modeid & HAM_KEY ) ncolors -= 2; /* Currently true, might change later */
    if( modeid & EXTRAHALFBRITE_KEY ) ncolors = 5;
    colors = AllocVec( ncolors * 3, MEMF_CLEAR);
    if(!colors) return NULL;

    idx = 0;
    rgb = (UBYTE *) sp->sp_Data;
    for( i = 0; i < ncolors; i++ ) {
        r = *rgb++;
        g = *rgb++;
        b = *rgb++;
        colors[idx++] = r;
        colors[idx++] = g;
        colors[idx++] = b;
    }

    return colors;
}

/*
    NOTE! Call third
*/
void SetUpDisplayMode( struct StoredProperty *sp, FRAME *f )
{
    ULONG modeid;

    modeid = *((ULONG *)sp->sp_Data);

    /* Remove old-style 16bit stuff */
    if((!(modeid & MONITOR_ID_MASK))|| ((modeid & EXTENDED_MODE)&&(!(modeid & 0xFFFF0000))))
        modeid &= (~(EXTENDED_MODE|SPRITES|GENLOCK_AUDIO|GENLOCK_VIDEO|VP_HIDE));

    /* Bogus CAMGs */
    if((modeid & 0xFFFF0000)&&(!(modeid & 0x00001000))) modeid = 0L;

    f->pix->origmodeid = modeid;

    return;
}

/*
    BUG: slow. (should really change the get_color to assembly)
    SetUpDisplayMode() MUST be called before this!
*/

BOOL bitplane24bit( UBYTE *chunkybuffer, FRAME *f, UBYTE *colors, UBYTE nplanes, UBYTE *frameaddr )
{
    ULONG color, column;
    UBYTE or = 0,og = 0,ob = 0,sel;
    WORD  pictype;
    UBYTE jumpsize;

    if( nplanes == 24 ) {
        pictype  = ILBM24;
        jumpsize = 3;
    } else if( f->pix->origmodeid & HAM_KEY ) {
        if( nplanes == 6 )
            pictype = HAM6;
        else
            pictype = HAM8;
        jumpsize = 1;
    } else {
        pictype  = BITMAP;
        jumpsize = 1;
    }

    // D(bug("\t\tConvert24bit()\n"));

    // D(bug("Frameaddr is %lu...",frameaddr));

    for( column = 0; column < f->pix->width; column++) {
        color = 0L;

        color = (ULONG) *chunkybuffer++;

        switch(pictype) {
            case HAM6:
                // PDebug("HAM6");
                sel = ( ((UBYTE)color) & 0x30);
                color &= 0x0F; /* Mask out upper bits */
                switch( sel ) {
                    case 0x00: /* New value */
                        // kprintf("new value");
                        *frameaddr++ = or = colors[ color * 3 ];
                        *frameaddr++ = og = colors[ color * 3 + 1];
                        *frameaddr++ = ob = colors[ color * 3 + 2];
                        break;
                    case 0x10: /* Change blue */
                        // kprintf("change blue");
                        *frameaddr++ = or;
                        *frameaddr++ = og;
                        *frameaddr++ = ob = (UBYTE)( color | (color << 4));
                        break;
                    case 0x20: /* red */
                        // kprintf("change red");
                        *frameaddr++ = or = (UBYTE)( color | (color << 4));
                        *frameaddr++ = og;
                        *frameaddr++ = ob;
                        break;
                    case 0x30: /* green */
                        // kprintf("change green");
                        *frameaddr++ = or;
                        *frameaddr++ = og = (UBYTE)( color | (color << 4));
                        *frameaddr++ = ob;
                        break;
                }
                // PDebug(", sel = %lu, color = %lu (8bit: %lu) -> (%lu,%lu,%lu)\n",(ULONG)sel,color, (color | (color << 4)),(ULONG)or,(ULONG)og,(ULONG)ob);
                break;

            case HAM8:
                // PDebug("HAM8");
                sel = (color & 0xC0);
                color &= 0x3F; /* Mask out upper bits, color value 0...63 */
                switch( sel ) {
                    case 0: /* New value */
                        // kprintf("new");
                        *frameaddr++ = or = colors[ color * 3 ];
                        *frameaddr++ = og = colors[ color * 3 + 1];
                        *frameaddr++ = ob = colors[ color * 3 + 2];
                        break;
                    case 0x40: /* Change blue */
                        // kprintf("blue");
                        *frameaddr++ = or;
                        *frameaddr++ = og;
                        ob = (ob & 0x03) | (UBYTE)(color << 2);
                        *frameaddr++ = ob;
                        break;
                    case 0x80: /* red */
                        // kprintf("red");
                        or = (or & 0x03) | (UBYTE)(color << 2);
                        *frameaddr++ = or;
                        *frameaddr++ = og;
                        *frameaddr++ = ob;
                        break;
                    case 0xC0: /* green */
                        // kprintf("green");
                        *frameaddr++ = or;
                        og = (og & 0x03) | (UBYTE)(color << 2);
                        *frameaddr++ = og;
                        *frameaddr++ = ob;
                        break;
                }
                break;

            case EHB:
                // PDebug("EHB");
                if( color >= 32 ) {
                    *frameaddr++ = colors[ (color-32) * 3 ] >> 1;
                    *frameaddr++ = colors[ (color-32) * 3 +1 ] >> 1;
                    *frameaddr++ = colors[ (color-32) * 3 +2 ] >> 1;
                } else {
                    *frameaddr++ = colors[ color * 3 ];
                    *frameaddr++ = colors[ color * 3 +1 ];
                    *frameaddr++ = colors[ color * 3 +2];
                }
                break;

            case ILBM24:
                // PDebug("ILBM24");
                *frameaddr++ = (UBYTE)color;  /* RED */
                color = *chunkybuffer++;
                *frameaddr++ = (UBYTE)color;  /* GREEN */
                color = *chunkybuffer++;
                *frameaddr++ = (UBYTE)color;  /* BLUE */
                break;

            case BITMAP:
                // PDebug("BITMAP");
                *frameaddr++ = colors[ color * 3 ];
                *frameaddr++ = colors[ color * 3 + 1 ];
                *frameaddr++ = colors[ color * 3 + 2 ];
                // PDebug(", color = %lu\n",color);
                break;
        } /* switch*/
    } /* for */

    // PDebug("...END: Frameaddr = %lu\n",frameaddr);
    return PERR_OK;
}


/*
    Decodes the IFF BODY information. BUG: error checking is minimal.
*/

SAVEDS int DecodeBody( struct IFFHandle *iff,
                FRAME *f,
                UBYTE *cr,
                struct BitMapHeader *bmhd,
                struct PPTBase *PPTBase,
                struct Library *IFFParseBase )
{
    UBYTE *buffer, *ibuf, **destplaneptr, *t,*frameaddr;
    ULONG bufsize;
    struct ContextNode *cn;
    WORD compression = bmhd->compression;
    UBYTE *planes[ 25 ]; /* We can load this many bit planes at maximum. */
    int res = PERR_OK, nRows = bmhd->h, i;
    int crow, nempty, nfilled;
    UBYTE nplanes = bmhd->nPlanes, cplane;
    LONG bufrowbytes = MaxPackedSize( RowBytes(bmhd->w) );
    WORD srcrowbytes = RowBytes(bmhd->w);
    WORD destrowbytes = srcrowbytes;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    struct Library *UtilityBase = PPTBase->lb_Utility;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    /* Initialization */
    UBYTE *chunkybuffer = NULL;

    D(bug("DecodeBody()\n"));

    InitProgress( f, "Loading IFF/ILBM picture...", 0,bmhd->h );

    if( compression > cmpByteRun1)
        return UNKNOWN_COMPRESSION;

    /* Reserve input buffer */
    bufsize = MaxPackedSize(RowBytes(bmhd->w)) << 4;
    if( NULL == (buffer = AllocVec( bufsize, 0L )) )
        return PERR_OUTOFMEMORY;

    /* Reserve plane pointers */

    for(i = 0; i < 24; i++) {
        planes[i] = AllocVec(RowBytes( bmhd->w ), 0L );
        if(!planes[i])
            goto myexit;
    }

    chunkybuffer = AllocVec( bmhd->w * 3, 0L ); /* Allocate for 24bit data */
    if(!chunkybuffer) goto myexit;

    cn = CurrentChunk(iff); /* We must be stopped at BODY! */

    /* Check for masks. BUG: should really do something? */

    if(bmhd->masking == mskHasMask) {
        res = MASKING_NOT_SUPPORTED; goto myexit;
    }

    /* The biggie himself. */

    ibuf = buffer + bufsize;
    for( crow = 0; crow < nRows; crow++ ) {

        if(Progress(f,crow)) {
            res = PERR_BREAK;
            goto myexit;
        }

        for( cplane = 0; cplane < nplanes; cplane++ ) {
            destplaneptr = &planes[cplane];
            nempty  = ibuf - buffer;
            nfilled = bufsize - nempty;
            if( nfilled < bufrowbytes ) { /* We need to read more data */
                movmem(ibuf, buffer, nfilled );
                if( nempty > ChunkMoreBytes(cn) ) {
                    nempty = ChunkMoreBytes(cn);
                    bufsize = nfilled + nempty; /* What's so funny? */
                }

                /* Append new data to existing data */
                if(ReadChunkBytes(iff, &buffer[nfilled], nempty) < nempty) {
                    res = CANNOT_READ; goto myexit;
                }

                ibuf = buffer;
                nfilled = bufsize;
                nempty = 0;
            }
            if(compression == cmpNone) {
                if(nfilled < srcrowbytes) { res = IFFERR_MANGLED; }
                movmem(ibuf, *destplaneptr, srcrowbytes);
                ibuf += srcrowbytes;
                // *destplaneptr += destrowbytes;
            } else {
                t = *destplaneptr;
                if( unpackrow(&ibuf, &t, nfilled, srcrowbytes) ) {
                    res = IFFERR_MANGLED;
                } else {
                    // *destplaneptr += (destrowbytes - srcrowbytes);
                }
            }
        } /* for( cplane... )*/

        /* All the planes for this scan line have been gone through, so make
           it into chunky mode */

        PlanarToChunky( planes, chunkybuffer, bmhd->w, nplanes );

        /* Convert to 24 bit data */

        frameaddr = GetPixelRow( f, crow );
        if( bitplane24bit( chunkybuffer, f, cr, nplanes, frameaddr ) != PERR_OK ) {
            res = FORMAT_NOT_SUPPORTED; goto myexit;
        }
        PutPixelRow( f, crow, frameaddr );

    }

    FinishProgress( f );

    /* Release resources */

myexit:

    if( chunkybuffer ) FreeVec( chunkybuffer );

    for(i = 0; i<24; i++) {
        if(planes[i]) FreeVec(planes[i]);
    }

    FreeVec( buffer );

    return res;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

