/*----------------------------------------------------------------------*/
/*
   PROJECT: ppt
   MODULE : render.c

   $Id: render.c,v 1.30 1999/03/14 20:59:08 jj Exp $

   Additional rendering routines and quickrendering stuff.

 */
/*----------------------------------------------------------------------*/

#undef FAST_QUICKMAP           /* Use assembler code in QuickRender()? */
#define FAST_QUICKMAP_DEEP     /* How about for deep screens? */

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"
#include "gui.h"
#include "render.h"

#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif

#ifndef GRAPHICS_GFX_H
#include <graphics/gfx.h>
#endif

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

#ifndef PROTO_INTUITION_H
#include <proto/intuition.h>
#endif

#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif

#ifndef GRAPHICS_GFXBASE_H
#include <graphics/gfxbase.h>
#endif

#include <clib/alib_protos.h>

#include <cybergraphics/cybergraphics.h>
#include <proto/cybergraphics.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define ALPHA_GRAY_LOW      0xA0
#define ALPHA_GRAY_HIGH     0xE0

#define DITHER_NS           256     /* Number of shades */
#define DITHER(p,d)         ((UBYTE) (( (((p)+(d))<<4) -(p))>>4) )

struct QuickRenderArgs {
    FRAME *source;
    struct RastPort *dest, *temprp;
    UWORD top, left, winheight, winwidth;       /* Renderarea location */
    UBYTE *pixelrow;
    UBYTE dither;                   /* Values: see ppt_real.h */
};


/*----------------------------------------------------------------------*/
/* Global variables */

UBYTE QuickRemapTable[QUICKREMAPSIZE];
UBYTE QuickRemapTable_Color[4096];

int dith_dim, dith_dm2;
int **dith_mat;

/*----------------------------------------------------------------------*/
/* Internal prototypes */

Prototype PERROR QuickRender(FRAME *, struct RastPort *, UWORD, UWORD, UWORD, UWORD, EXTBASE *);
Prototype PERROR AllocColorTable(FRAME *);
Prototype VOID ReleaseColorTable(FRAME *);

extern ASM void QuickMapRow(REG(a0) UBYTE *, REG(a1) UBYTE *, REG(d0) UWORD, REG(d1) UWORD, REG(d2) UBYTE);
extern ASM void QuickMapARGBDeepRow(REG(a0) ARGBPixel *, REG(a1) UBYTE *, REG(d0) UWORD, REG(d1) UWORD, REG(d2) UBYTE, REG(d3) UBYTE, REG(d4) WORD);


/*----------------------------------------------------------------------*/
/* Code */

/// AllocColorTable()
/*
   Won't allocate it if it already exists.
 */
PERROR AllocColorTable(FRAME * frame)
{
    COLORMAP *ct;
    PERROR res = PERR_OK;

    D(bug("AllocColorTable()\n"));

    if (frame->disp->colortable)
        return PERR_OK;

    ct = pzmalloc(256 * sizeof(COLORMAP));      /* BUG: Should not be hardcoded */
    if (!ct)
        res = PERR_OUTOFMEMORY;

    LOCK(frame);
    frame->disp->colortable = ct;
    UNLOCK(frame);

    return res;
}
///
/// ReleaseColorTable()
VOID ReleaseColorTable(FRAME * frame)
{
    if (frame) {
        LOCK(frame);
        if (frame->disp) {
            if (frame->disp->colortable) {
                pfree(frame->disp->colortable);
                frame->disp->colortable = NULL;
            }
        }
        UNLOCK(frame);
    }
}
///

/// Init & exit
Prototype VOID QuickRenderInit( VOID );

VOID QuickRenderInit( VOID )
{
    MakeDitherMatrix( &dith_mat, &dith_dim, &dith_dm2, 2 );
}

Prototype VOID QuickRenderExit(VOID);

VOID QuickRenderExit(VOID)
{
    if( dith_mat ) sfree( dith_mat );
}
///

/// QuickRender_RGB_Gray()
/*
   From 24bit RGB format to grayscale preview window.
 */

ULONG QuickRender_RGB_Gray(struct QuickRenderArgs *qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct IBox *zb = &(source->zoombox);
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;
    UBYTE *pixelrow = qra->pixelrow;
    int dm = dith_dim - 1;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tTRUECOLOR render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ROWPTR cp;
        int *m;
        WORD col;

        m = dith_mat[row & dm];
        cp = GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

#ifdef FAST_QUICKMAP

        QuickMapRow(cp + MULU16(zb->Left, 3), qra->pixelrow,
                    qra->winwidth, zb->Width, 3);
#else
        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset;
            UBYTE r, g, b;
            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;
            offset = offset + offset + offset;

            if( qra->dither == DITHER_NONE ) {
                r = cp[offset];
                g = cp[offset + 1];
                b = cp[offset + 2];
            } else {
                int d;

                d = m[col&dm];
                r = DITHER(cp[offset],d);
                g = DITHER(cp[offset + 1],d);
                b = DITHER(cp[offset + 2],d);
            }
            pixelrow[col] = QuickRemapTable[(UBYTE) ((UWORD) (r + g + b) / (UWORD) 3)];
        }
#endif
        /* Write to display */
        WritePixelLine8(qra->dest, qra->left, row + qra->top,
                        qra->winwidth, qra->pixelrow, qra->temprp);

#if 0
        if (row % 10 == 0) {
            ULONG rc;
            while ((rc = HandleEvent(source->disp->Win)) != WMHI_NOMORE) {
                if (rc < 65535 && rc > 0 && rc != GID_DW_MOUSEMOVE && rc != GID_DW_INTUITICKS) {
                    res = rc;
                    goto quit;
                }
            }
        }
#endif

    }                           /* for */

  quit:
    return res;
}
///
/// QuickRender_ARGB_Gray()
/*
   From 32bit ARGB format to grayscale preview window.
 */

ULONG QuickRender_ARGB_Gray(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct IBox *zb = &(source->zoombox);
    UBYTE *pixelrow = qra->pixelrow;
    UBYTE QuickAlphaTable[2];
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;
    int dm = dith_dim - 1;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    QuickAlphaTable[0] = ALPHA_GRAY_LOW;
    QuickAlphaTable[1] = ALPHA_GRAY_HIGH;

    D(bug("\tTRUECOLOR render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ROWPTR cp;
        WORD col;
        int *m;

        m = dith_mat[row&dm];
        cp = GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset, argb;
            UBYTE r, g, b, c, a, t;

            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;
            offset = offset << 2;
            argb = *((ULONG *) (cp + offset));

            if( qra->dither == DITHER_NONE ) {
                a = ARGB_A(argb);
                r = ARGB_R(argb);
                g = ARGB_G(argb);
                b = ARGB_B(argb);
            } else {
                int d;
                d = m[col&dm];
                a = ARGB_A(argb);
                r = DITHER(ARGB_R(argb),d);
                g = DITHER(ARGB_G(argb),d);
                b = DITHER(ARGB_B(argb),d);
            }

            t = QuickAlphaTable[((row >> 3) + (col >> 3)) & 0x01];
            c = (r+g+b)/3;
            c = ((255 - (UWORD) a) * c + (UWORD) a * t) / 255;
            c = QuickRemapTable[c];
            pixelrow[col] = c;
        }

        /* Write to display */
        WritePixelLine8(qra->dest, qra->left, row + qra->top,
                        qra->winwidth, qra->pixelrow, qra->temprp);

    }                           /* for */

  quit:
    return res;
}
///
/// QuickRender_Gray_Gray()
/*
   From 8bit Graylevel format to grayscale preview window
 */
ULONG QuickRender_Gray_Gray(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row;
    ULONG res = 0;
    WORD winwidth = qra->winwidth, winheight = qra->winheight;
    FRAME *source = qra->source;
    WORD srcwidth, srcheight;
    UBYTE *pixelrow = qra->pixelrow;
    struct IBox *zb = &(source->zoombox);
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;
    int dm = dith_dim-1;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tGray render...\n"));

    for (row = 0; row < winheight; row++) {
        ROWPTR cp;
        WORD col;
        int *m;

        cp = GetPixelRow(source, zb->Top + (LONG) MULU16(row, zb->Height) / (WORD) winheight, ExtBase);
        m = dith_mat[row&dm];

        for (col = 0; col < winwidth; col++) {
            ULONG offset;
            offset = (LONG) MULU16(col, zb->Width) / (WORD) winwidth + zb->Left;
            if( qra->dither == DITHER_NONE ) {
                pixelrow[col] = QuickRemapTable[cp[offset]];
            } else {
                int d;
                d = m[col&dm];
                pixelrow[col] = QuickRemapTable[DITHER(cp[offset],d)];
            }
        }

        WritePixelLine8(qra->dest, qra->left, row + qra->top,
                        winwidth, pixelrow, qra->temprp);

#if 0
        if (row % 10 == 0) {
            ULONG rc;
            while ((rc = HandleEvent(source->disp->Win)) != WMHI_NOMORE) {
                if (rc < 65535 && rc > 0 && rc != GID_DW_MOUSEMOVE && rc != GID_DW_INTUITICKS) {
                    res = rc;
                    goto quit;
                }
            }
        }
#endif

    }                           /* row */

  quit:
    return res;
}
///

#define COLOR(r,g,b) QuickRemapTable_Color[(((r) & 0xF0)<<4)|((g) & 0xF0)|((b)>>4)];
// #define DITHER(p,d)   ((UBYTE) ((15*p/16)+(d)))
#define LEVELS(s)     (dith_dm2 * ((s) - 1) + 1)

/// QuickRender_RGB_Color()
ULONG QuickRender_RGB_Color(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct IBox *zb = &(source->zoombox);
    UBYTE *pixelrow = qra->pixelrow;
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;
    int dm = dith_dim - 1;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tTRUECOLOR render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ROWPTR cp;
        WORD col;
        int *m;

        cp = GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

        m = dith_mat[row & dm];

        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset;
            UBYTE r, g, b;

            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;
            offset = offset + offset + offset;

            if( qra->dither == DITHER_NONE ) {
                r = cp[offset];
                g = cp[offset + 1];
                b = cp[offset + 2];
            } else {
                int d;

                d = m[col&dm];
                r = DITHER(cp[offset],d);
                g = DITHER(cp[offset + 1],d);
                b = DITHER(cp[offset + 2],d);
            }
            pixelrow[col] = COLOR(r,g,b);
        }

        /* Write to display */
        WritePixelLine8(qra->dest, qra->left, row + qra->top,
                        qra->winwidth, qra->pixelrow, qra->temprp);

    }                           /* for */

  quit:
    return res;
}
///
/// QuickRender_ARGB_Color()
ULONG QuickRender_ARGB_Color(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct IBox *zb = &(source->zoombox);
    UBYTE *pixelrow = qra->pixelrow;
    UBYTE QuickAlphaTable[2];
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;
    int dm = dith_dim - 1;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    QuickAlphaTable[0] = ALPHA_GRAY_LOW;
    QuickAlphaTable[1] = ALPHA_GRAY_HIGH;

    D(bug("\tARGB->Color render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ARGBPixel *cp;
        WORD col;
        int *m;

        cp = (ARGBPixel *) GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

        m = dith_mat[row & dm];

        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset, argb;
            UBYTE r, g, b, a, c, t;
            int d;

            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;

            // offset = offset<<2;
            argb = *((ULONG *) (cp + offset));

            if( qra->dither == DITHER_NONE ) {
                a = ARGB_A(argb);
                r = ARGB_R(argb);
                g = ARGB_G(argb);
                b = ARGB_B(argb);
            } else {
                d = m[col&dm];
                a = ARGB_A(argb);
                r = DITHER(ARGB_R(argb),d);
                g = DITHER(ARGB_G(argb),d);
                b = DITHER(ARGB_B(argb),d);
            }

            t = QuickAlphaTable[((row >> 3) + (col >> 3)) & 0x01];
            r = ((255 - (UWORD) a) * r + (UWORD) a * t) >> 8;
            g = ((255 - (UWORD) a) * g + (UWORD) a * t) >> 8;
            b = ((255 - (UWORD) a) * b + (UWORD) a * t) >> 8;

            c = COLOR(r,g,b);

            // c = ((255 - (UWORD) a) * c + (UWORD) a * t) >> 8;

            pixelrow[col] = c;
        }

        /* Write to display */
        WritePixelLine8(qra->dest, qra->left, row + qra->top,
                        qra->winwidth, qra->pixelrow, qra->temprp);

    }                           /* for */

  quit:
    return res;
}
///
/// QuickRender_Gray_Color()
/*
 *  BUG: This could be fastened somewhat by using the knowledge
 *       on which colours have been set by the init routines
 *       to provide the gray scale map.
 */

ULONG QuickRender_Gray_Color(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row;
    ULONG res = 0;
    WORD winwidth = qra->winwidth, winheight = qra->winheight;
    FRAME *source = qra->source;
    WORD srcwidth, srcheight;
    UBYTE *pixelrow = qra->pixelrow;
    struct IBox *zb = &(source->zoombox);
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;
    int dm = dith_dim - 1;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tGray render...\n"));

    for (row = 0; row < winheight; row++) {
        ROWPTR cp;
        WORD col;
        int *m;

        cp = GetPixelRow(source, zb->Top + (LONG) MULU16(row, zb->Height) / (WORD) winheight, ExtBase);
        m = dith_mat[row & dm];

        for (col = 0; col < winwidth; col++) {
            ULONG offset;
            UBYTE v;

            offset = (LONG) MULU16(col, zb->Width) / (WORD) winwidth + zb->Left;
            if( qra->dither == DITHER_NONE ) {
                v = cp[offset];
            } else {
                int d;
                d = m[col&dm];
                v = DITHER(cp[offset],d);
            }
            pixelrow[col] = COLOR(v,v,v);
        }

        WritePixelLine8(qra->dest, qra->left, row + qra->top,
                        winwidth, pixelrow, qra->temprp);

    }                           /* row */

  quit:
    return res;
}
///

/// QuickRender_RGB_Deep()

/*
 *  Deep screen system.
 */

ULONG QuickRender_RGB_Deep(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct Library *CyberGfxBase = ExtBase->lb_CyberGfx;
    struct IBox *zb = &(source->zoombox);

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tRGB->DEEP render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ROWPTR cp;

        cp = GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        ScalePixelArray(cp + zb->Left * 3, zb->Width, 1, zb->Width * 3, qra->dest, qra->left,
                        row + qra->top, qra->winwidth, 1, RECTFMT_RGB);
    }                           /* for */

  quit:
    return res;

}
///
/// QuickRender_ARGB_Deep()
ULONG QuickRender_ARGB_Deep(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct IBox *zb = &(source->zoombox);
    struct Library *CyberGfxBase = ExtBase->lb_CyberGfx;
    RGBPixel *pixelrow = (RGBPixel *)qra->pixelrow;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tARGB->DEEP render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ARGBPixel *cp;
        WORD col;

        cp = (ARGBPixel *)GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);

        /* Write to display */
#if 0
        ScalePixelArray(cp + zb->Left * 4, zb->Width, 1, zb->Width * 4, qra->dest, qra->left,
                        row + qra->top, qra->winwidth, 1, RECTFMT_ARGB);
#else

# ifdef FAST_QUICKMAP_DEEP
        QuickMapARGBDeepRow(cp + MULU16(zb->Left, 3), qra->pixelrow,
                            qra->winwidth, zb->Width, ALPHA_GRAY_HIGH, ALPHA_GRAY_LOW, row);
# else
        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset;
            UWORD a, t;

            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;

            a = cp[offset].a;
            t = (((row >> 3) + (col >> 3)) % 2) ? ALPHA_GRAY_HIGH : ALPHA_GRAY_LOW;

            pixelrow[col].r = ((255 - (UWORD) a) * cp[offset].r + (UWORD) a * t) >> 8;
            pixelrow[col].g = ((255 - (UWORD) a) * cp[offset].g + (UWORD) a * t) >> 8;
            pixelrow[col].b = ((255 - (UWORD) a) * cp[offset].b + (UWORD) a * t) >> 8;
        }
# endif

        /* Write to display */
        WritePixelArray(pixelrow, 0, 0, qra->winwidth * 3, qra->dest, qra->left,
                        row + qra->top, qra->winwidth, 1, RECTFMT_RGB);

#endif
    } /* for */

  quit:
    return res;

}
///
/// QuickRender_Gray_Deep()
ULONG QuickRender_Gray_Deep(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct IBox *zb = &(source->zoombox);
    UBYTE *pixelrow = qra->pixelrow;
    struct Library *CyberGfxBase = ExtBase->lb_CyberGfx;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tGRAY8->DEEP render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ROWPTR cp;
        WORD col;

        cp = GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset;
            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;
            pixelrow[col] = cp[offset];
        }

        /* Write to display */
        WritePixelArray(pixelrow, 0, 0, qra->winwidth * 3, qra->dest, qra->left,
                        row + qra->top, qra->winwidth, 1, RECTFMT_GREY8);

    }                           /* for */

  quit:
    return res;

}
///

/// QuickRender()
/*
   This routine is just for use with the main screen. It will make a quick
   render to the rastport given, using it's color map, which must be preset
   to an intelligent value.

   We'll use gray scales for now. First four colors are reserved for the program
   itself, thus we will not use them. Even if they would fit. Our color range
   will be (ncolors - 4) colors, with equal length steps. Thus, on an 8color screen,
   we'll have 4 system colors, then black, 33 % grey, 67% grey and white.
 */

PERROR QuickRender(FRAME * source, struct RastPort * dest,
                   UWORD top, UWORD left, UWORD winheight, UWORD winwidth,
                   EXTBASE * ExtBase)
{
    UBYTE *pixelrow = NULL, colorspace = source->pix->colorspace;
    int i;
    struct RastPort temprp;
    struct BitMap tempbm;
    PERROR res = PERR_OK;
    struct QuickRenderArgs qra;
    UWORD destdepth;
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;
    D(APTR bt);

    LOCK(source);

    if (colorspace != CS_RGB && colorspace != CS_GRAYLEVEL && colorspace != CS_ARGB) {
        ShowText(source, "Unknown colorspace", ExtBase);
        UNLOCK(source);
        return PERR_UNKNOWNTYPE;
    }
//    DEBUG("X ratio = %lf, Y ratio = %lf\n", (double)srcwidth/winwidth, (double)srcheight/winheight );

    bcopy(dest, &temprp, sizeof(struct RastPort));
    temprp.Layer = NULL;
    if (GFXV39) {

        /*
         *  Note that it safe to pass some checks here, because the deep
         *  screens can only be used on V39+ machines
         */

        destdepth = GetBitMapAttr(dest->BitMap, BMA_DEPTH);

        if (destdepth <= 8) {
            temprp.BitMap = AllocBitMap(winwidth, 1, destdepth, 0L, dest->BitMap);
            if (!temprp.BitMap) {
                D(bug("ERROR: Failed to allocate bitmaps!\n"));
                ShowText(source, "Memory error", ExtBase);
                goto quit;
            }
        } else {
            temprp.BitMap = NULL;
        }
    } else {
        temprp.BitMap = &tempbm;
        tempbm.Rows = 1;
        destdepth = tempbm.Depth = dest->BitMap->Depth;
        tempbm.BytesPerRow = (((winwidth + 15) >> 4) << 1);

        for (i = 0; i < dest->BitMap->Depth; i++) {
            tempbm.Planes[i] = AllocRaster(winwidth, 1);
            if (!tempbm.Planes[i]) {
                D(bug("ERROR: Failed to allocate bitmap plane %d!\n", i));
                ShowText(source, "Memory error", ExtBase);
                goto quit;
            }
        }
    }

    pixelrow = pmalloc(4 * (((winwidth + 15) >> 4) << 4));
    if (!pixelrow) {
        D(bug("ERROR: Failed to allocate pixelrow!\n"));
        ShowText(source, "Memory error", ExtBase);
        goto quit;
    }
    qra.dest = dest;
    qra.pixelrow = pixelrow;
    qra.temprp = &temprp;
    qra.source = source;
    qra.winwidth = winwidth;
    qra.winheight = winheight;
    qra.top = top;
    qra.left = left;

    if( globals->userprefs->ditherpreview )
        qra.dither = DITHER_ORDERED;
    else
        qra.dither = DITHER_NONE;

//    DEBUG("Allocated %lu bytes for %lu pixels / row\n", tempbm.BytesPerRow, winwidth );

    /*
     *  Dispatch the correct rendering engine
     */


    D(bt = StartBench());

    if (destdepth > 8) {
        switch (colorspace) {
        case CS_RGB:
            res = QuickRender_RGB_Deep(&qra, ExtBase);
            break;
        case CS_ARGB:
            res = QuickRender_ARGB_Deep(&qra, ExtBase);
            break;
        case CS_GRAYLEVEL:
            res = QuickRender_Gray_Deep(&qra, ExtBase);
            break;
        default:
            InternalError("Not yet supported");
        }
    } else {
        if (globals->userprefs->colorpreview) {
            switch (colorspace) {
            case CS_RGB:
                res = QuickRender_RGB_Color(&qra, ExtBase);
                break;
            case CS_GRAYLEVEL:
                res = QuickRender_Gray_Color(&qra, ExtBase);
                break;
            case CS_ARGB:
                res = QuickRender_ARGB_Color(&qra, ExtBase);
                break;
            }
        } else {
            switch (colorspace) {
            case CS_RGB:
                res = QuickRender_RGB_Gray(&qra, ExtBase);
                break;
            case CS_GRAYLEVEL:
                res = QuickRender_Gray_Gray(&qra, ExtBase);
                break;
            case CS_ARGB:
                res = QuickRender_ARGB_Gray(&qra, ExtBase);
                break;
            }
        }
    }

    D(StopBench(bt));

  quit:

    UNLOCK(source);

    WaitBlit();

    if (GFXV39) {
        FreeBitMap(temprp.BitMap);      // NULL is OK.

    } else {
        for (i = 0; i < dest->BitMap->Depth && tempbm.Planes[i]; i++)
            FreeRaster(tempbm.Planes[i], winwidth, 1);
    }
    if (pixelrow)
        pfree(pixelrow);

    return res;
}
///

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/
