/*----------------------------------------------------------------------*/
/*
   PROJECT: ppt
   MODULE : render.c

   $Id: render.c,v 1.24 1998/06/28 23:23:43 jj Exp $

   Additional rendering routines and quickrendering stuff.

 */
/*----------------------------------------------------------------------*/

#define FAST_QUICKMAP           /* Use assembler code in QuickRender()? */


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

#ifndef GRAPHICS_GFXMACROS_H
#include <graphics/gfxmacros.h>
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

struct QuickRenderArgs {
    FRAME *source;
    struct RastPort *dest, *temprp;
    UWORD top, left, winheight, winwidth;       /* Renderarea location */
    UBYTE *pixelrow;
};


/*----------------------------------------------------------------------*/
/* Global variables */

UBYTE QuickRemapTable[QUICKREMAPSIZE];
UBYTE QuickRemapTable_Color[4096];

/*----------------------------------------------------------------------*/
/* Internal prototypes */

Prototype PERROR QuickRender(FRAME *, struct RastPort *, UWORD, UWORD, UWORD, UWORD, EXTBASE *);
Prototype VOID RemoveSelectBox(FRAME *);
Prototype PERROR AllocColorTable(FRAME *);
Prototype VOID ReleaseColorTable(FRAME *);

extern ASM void QuickMapRow(REG(a0) UBYTE *, REG(a1) UBYTE *, REG(d0) UWORD, REG(d1) UWORD, REG(d2) UBYTE);
extern ASM void QuickMapARGBDeepRow(REG(a0) ARGBPixel *, REG(a1) UBYTE *, REG(d0) UWORD, REG(d1) UWORD, REG(d2) UBYTE, REG(d3) UBYTE, REG(d4) WORD);

Local VOID DrawSelectRectangle(FRAME * frame, WORD x0, WORD y0, WORD x1, WORD y1, ULONG flags);
Local VOID DrawSelectCircle( FRAME *, WORD, WORD, WORD );

/*----------------------------------------------------------------------*/
/* Code */

/*
   Removes the select box, if needed.
 */

VOID RemoveSelectBox(FRAME * frame)
{
    switch (frame->selectmethod) {
    case GINP_LASSO_RECT:
        /*
         *  Does the rectangle exist?
         */
        if (frame->selstatus & SELF_RECTANGLE) {
            struct Rectangle *sb = &frame->selbox;

            /*
             *  Yes, it exists.  If we are in a situation where the user
             *  is holding the LMB down and is moving around in the image,
             *  we won't delete the resize handles, because they don't exist.
             */

            DrawSelectRectangle(frame, sb->MinX, sb->MinY, sb->MaxX, sb->MaxY,
                                (frame->selstatus & SELF_BUTTONDOWN) ? DSBF_INTERIM : 0L);
            frame->selstatus &= ~SELF_RECTANGLE;
        }
        break;

    case GINP_FIXED_RECT:
        if( frame->selstatus & SELF_RECTANGLE ) {
            struct IBox *ib = &frame->fixrect;

            DrawSelectRectangle( frame, ib->Left, ib->Top, ib->Left+ib->Width,
                                 ib->Top+ib->Height, DSBF_FIXEDRECT );
            frame->selstatus &= ~SELF_RECTANGLE;
        }
        break;

    case GINP_LASSO_CIRCLE:
        if( frame->selstatus & SELF_RECTANGLE ) {
            DrawSelectCircle( frame, frame->circlex, frame->circley, frame->circleradius );
            frame->selstatus &= ~SELF_RECTANGLE;
        }
        break;

    default:
        break;
    }
}

/*
    This function draws a select box onto frame display window.  It knows
    automatically what to do, according to the current status.
 */

Prototype VOID DrawSelectBox( FRAME *frame, ULONG flags );

VOID DrawSelectBox( FRAME *frame, ULONG flags )
{
    switch( frame->selectmethod ) {
      case GINP_LASSO_RECT:
        if( frame->selbox.MinX != ~0 && !(frame->selstatus & SELF_RECTANGLE) ) {
            struct Rectangle *sb = &frame->selbox;

            DrawSelectRectangle(frame, sb->MinX, sb->MinY, sb->MaxX, sb->MaxY, flags);
            frame->selstatus |= SELF_RECTANGLE;
        }
        break;

      case GINP_FIXED_RECT:
        if( !(frame->selstatus & SELF_RECTANGLE) ) {
            struct IBox *ib = &frame->fixrect;

            DrawSelectRectangle( frame, ib->Left, ib->Top, ib->Left+ib->Width,
                                 ib->Top+ib->Height, DSBF_FIXEDRECT );
            frame->selstatus |= SELF_RECTANGLE;
        }

        break;

      case GINP_LASSO_CIRCLE:
        if( !(frame->selstatus & SELF_RECTANGLE) ) {
            DrawSelectCircle( frame, frame->circlex, frame->circley, frame->circleradius );
            frame->selstatus |= SELF_RECTANGLE;
        }
        break;
    }
}

Local
VOID DrawSelectCircle( FRAME *frame, WORD x, WORD y, WORD r )
{
    DISPLAY *d = frame->disp;
    struct Window *win = d->win;
    WORD winwidth, winheight, xoffset, yoffset;
    struct IBox *abox;
    WORD rx,ry;

    D(bug("DrawSelectCircle( x=%d, y=%d, r=%d)\n",x,y,r));

    GetAttr(AREA_AreaBox, frame->disp->RenderArea, (ULONG *) & abox);
    winwidth = abox->Width;
    winheight = abox->Height;
    xoffset = abox->Left;
    yoffset = abox->Top;

    /*
     *  Scale and limit the values to be inside the window
     */

    x -= frame->zoombox.Left;
    y -= frame->zoombox.Top;

    x = (LONG) (MULS16(x, winwidth)) / (WORD) (frame->zoombox.Width);
    y = (LONG) (MULS16(y, winheight)) / (WORD) frame->zoombox.Height;
    rx = (LONG) (MULS16(r, winwidth)) / (WORD) frame->zoombox.Width;
    ry = (LONG) (MULS16(r, winheight)) / (WORD) frame->zoombox.Height;

    /*
     *  Draw!
     */

    x += xoffset;
    y += yoffset;

    SetDrMd(win->RPort, COMPLEMENT);
    SetDrPt(win->RPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

    WaitTOF();

    DrawEllipse( win->RPort, x, y, rx, ry );
}

/*
   Draws a select box in the given frame displaywindow. Corners
   are at (x0,y0) and (x1,y1), relative to inner window top left
   corner.
   BUG: Won't work if pixsize > 16K
   BUG: Uses __divu

   flags can contain any of the following flags:

   DSBF_INTERIM : This is an interim draw, i.e. the image is still being
   drawn.  In this case, the resizing handles are not drawn.

   DSBF_FIXEDRECT : Draws a fixed-size rectangle. This means that
   the image contains also connecting beams between corners.
 */

Local
VOID DrawSelectRectangle(FRAME * frame, WORD x0, WORD y0, WORD x1, WORD y1, ULONG flags)
{
    DISPLAY *d = frame->disp;
    struct Window *win = d->win;
    WORD winwidth, winheight, xoffset, yoffset;
    BOOL lv = FALSE, rv = FALSE, th = FALSE, bh = FALSE;

    if (x0 == ~0 || !d)
        return;

    if (x0 == x1 || y0 == y1)
        return;

    /*
     *  Get the window size. If this is the bgui window on the main screen,
     *  then read it from the areabox, else assume we have a gzzwindow.
     */

    if (frame->disp->RenderArea) {
        struct IBox *abox;
        GetAttr(AREA_AreaBox, frame->disp->RenderArea, (ULONG *) & abox);
        winwidth = abox->Width;
        winheight = abox->Height;
        xoffset = abox->Left;
        yoffset = abox->Top;
    } else {
        winwidth = win->GZZWidth;
        winheight = win->GZZHeight;
        xoffset = 0;
        yoffset = 0;
    }

    /*
     *  Scale and limit the values to be inside the window
     */

    x0 -= frame->zoombox.Left;
    x1 -= frame->zoombox.Left;
    y0 -= frame->zoombox.Top;
    y1 -= frame->zoombox.Top;


    /*
     *  Do clipping
     */

    if (x1 < 0 || x0 > frame->zoombox.Width)
        return;                 // not visible

    if (y1 < 0 || y0 > frame->zoombox.Height)
        return;                 // not visible

    if (x0 >= 0)
        lv = TRUE;
    else
        x0 = 0;
    if (x1 <= frame->zoombox.Width)
        rv = TRUE;
    else
        x1 = frame->zoombox.Width;
    if (y0 >= 0)
        th = TRUE;
    else
        y0 = 0;
    if (y1 <= frame->zoombox.Height)
        bh = TRUE;
    else
        y1 = frame->zoombox.Height;


#if 1
    /*
     *  Dice generates better code with this.
     */
    x0 = (LONG) (MULS16(x0, winwidth)) / (WORD) (frame->zoombox.Width);
    x1 = (LONG) (MULS16(x1, winwidth)) / (WORD) (frame->zoombox.Width);

    y0 = (LONG) (MULS16(y0, winheight)) / (WORD) frame->zoombox.Height;
    y1 = (LONG) (MULS16(y1, winheight)) / (WORD) frame->zoombox.Height;
#else
    {
        x0 = ((LONG) x0 * winwidth) / frame->zoombox.Width;
        x1 = ((LONG) x1 * winwidth) / frame->zoombox.Width;
        y0 = ((LONG) y0 * winheight) / frame->zoombox.Height;
        y1 = ((LONG) y1 * winheight) / frame->zoombox.Height;
    }
#endif

    /*
     *  Draw!
     */

    {
        WORD boxsize = 5;       // BUG: Should calculate according to window size.

        x0 += xoffset;
        x1 += xoffset;
        y0 += yoffset;
        y1 += yoffset;

        SetDrMd(win->RPort, COMPLEMENT);

        if (!(flags & DSBF_FIXEDRECT))
            SetDrPt(win->RPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

        WaitTOF();

        // Left vertical beam

        if (lv) {
            Move(win->RPort, x0, y0);
            Draw(win->RPort, x0, y1);
        }
        // Right vertical beam

        if (rv) {
            Move(win->RPort, x1, y0);
            Draw(win->RPort, x1, y1);

        }
        // Top horizontal beam

        if (th) {
            Move(win->RPort, x0, y0);
            Draw(win->RPort, x1, y0);
        }
        // Bottom horizontal beam

        if (bh) {
            Move(win->RPort, x0, y1);
            Draw(win->RPort, x1, y1);
        }
        SetDrPt(win->RPort, 0xFFFF);

        if (flags & DSBF_FIXEDRECT) {

            /*
             *  Fixed rectangle: draw all areas
             */

            if( lv && rv && th && bh ) {
                Move( win->RPort, x0, y0 );
                Draw( win->RPort, x1, y1 );
                Move( win->RPort, x0, y1 );
                Draw( win->RPort, x1, y0 );
            }

        } else {
            /*
             *  A lasso rectangle : draw the resizing handles as well
             */

            if (lv && !(flags & DSBF_INTERIM)) {
                if (th) {
                    RectFill(win->RPort, x0, y0, x0 + boxsize, y0 + boxsize);
                }
                if (bh) {
                    RectFill(win->RPort, x0, y1 - boxsize, x0 + boxsize, y1);
                }
            }
            if (rv && !(flags & DSBF_INTERIM)) {
                if (th) {
                    RectFill(win->RPort, x1 - boxsize, y0, x1, y0 + boxsize);
                }
                if (bh) {
                    RectFill(win->RPort, x1 - boxsize, y1 - boxsize, x1, y1);
                }
            }
        }

        /*
         *  Update the handles of the image.  This information is
         *  needed when we need to check if the user has hit the
         *  buttons or not.
         */

        d->handles.MinX = x0;
        d->handles.MaxX = x1;
        d->handles.MinY = y0;
        d->handles.MaxY = y1;
    }
}

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

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tTRUECOLOR render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ROWPTR cp;

        cp = GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

#ifdef FAST_QUICKMAP

        QuickMapRow(cp + MULU16(zb->Left, 3), qra->pixelrow,
                    qra->winwidth, zb->Width, 3);
#else
        WORD col;
        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset;
            UBYTE r, g, b;
            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;
            offset = offset + offset + offset;
            r = cp[offset];
            g = cp[offset + 1];
            b = cp[offset + 2];
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

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    QuickAlphaTable[0] = ALPHA_GRAY_LOW;
    QuickAlphaTable[1] = ALPHA_GRAY_HIGH;

    D(bug("\tTRUECOLOR render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ROWPTR cp;
        WORD col;

        cp = GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset, argb;
            UBYTE r, g, b, c, a, t;

            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;
            offset = offset << 2;
            argb = *((ULONG *) (cp + offset));
            a = ARGB_A(argb);
            r = ARGB_R(argb);
            g = ARGB_G(argb);
            b = ARGB_B(argb);

            t = QuickAlphaTable[((row >> 3) + (col >> 3)) % 2];
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

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tGray render...\n"));

    for (row = 0; row < winheight; row++) {
        ROWPTR cp;
        WORD col;

        cp = GetPixelRow(source, zb->Top + (LONG) MULU16(row, zb->Height) / (WORD) winheight, ExtBase);

        for (col = 0; col < winwidth; col++) {
            ULONG offset;
            offset = (LONG) MULU16(col, zb->Width) / (WORD) winwidth + zb->Left;
            pixelrow[col] = QuickRemapTable[cp[offset]];
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

ULONG QuickRender_RGB_Color(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct IBox *zb = &(source->zoombox);
    UBYTE *pixelrow = qra->pixelrow;
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tTRUECOLOR render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ROWPTR cp;
        WORD col;

        cp = GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset;
            UBYTE r, g, b;
            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;
            offset = offset + offset + offset;
            r = cp[offset] & 0xF0;
            g = cp[offset + 1] & 0xF0;
            b = cp[offset + 2];
            pixelrow[col] = QuickRemapTable_Color[(r << 4) | g | (b >> 4)];
        }

        /* Write to display */
        WritePixelLine8(qra->dest, qra->left, row + qra->top,
                        qra->winwidth, qra->pixelrow, qra->temprp);

    }                           /* for */

  quit:
    return res;
}

ULONG QuickRender_ARGB_Color(struct QuickRenderArgs * qra, EXTBASE * ExtBase)
{
    WORD row, srcwidth, srcheight;
    FRAME *source = qra->source;
    ULONG res = 0;
    struct IBox *zb = &(source->zoombox);
    UBYTE *pixelrow = qra->pixelrow;
    UBYTE QuickAlphaTable[2];
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    QuickAlphaTable[0] = ALPHA_GRAY_LOW;
    QuickAlphaTable[1] = ALPHA_GRAY_HIGH;

    D(bug("\tTRUECOLOR render...\n"));
    for (row = 0; row < qra->winheight; row++) {
        ARGBPixel *cp;
        WORD col;

        cp = (ARGBPixel *) GetPixelRow(source, zb->Top + MULU16(row, zb->Height) / qra->winheight, ExtBase);
        // DEBUG("Read pixel line %lu\n",row * srcheight / qra->winheight );

        for (col = 0; col < qra->winwidth; col++) {
            ULONG offset, argb;
            UBYTE r, g, b, a, c, t;
            offset = MULU16(col, zb->Width) / qra->winwidth + zb->Left;

            // offset = offset<<2;
            argb = *((ULONG *) (cp + offset));
            a = ARGB_A(argb);
            r = ARGB_R(argb);
            g = ARGB_G(argb);
            b = ARGB_B(argb);

            t = QuickAlphaTable[((row >> 3) + (col >> 3)) % 2];
            r = ((255 - (UWORD) a) * r + (UWORD) a * t) >> 8;
            g = ((255 - (UWORD) a) * g + (UWORD) a * t) >> 8;
            b = ((255 - (UWORD) a) * b + (UWORD) a * t) >> 8;

            c = QuickRemapTable_Color[( (r&0xF0) << 4) | (g&0xF0) | (b >> 4)];

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

    srcwidth = source->pix->width;
    srcheight = source->pix->height;

    D(bug("\tGray render...\n"));

    for (row = 0; row < winheight; row++) {
        ROWPTR cp;
        WORD col;

        cp = GetPixelRow(source, zb->Top + (LONG) MULU16(row, zb->Height) / (WORD) winheight, ExtBase);

        for (col = 0; col < winwidth; col++) {
            ULONG offset;
            UBYTE v;

            offset = (LONG) MULU16(col, zb->Width) / (WORD) winwidth + zb->Left;
            v = cp[offset] & 0xF0;
            pixelrow[col] = QuickRemapTable_Color[(v << 4) | v | (v >> 4)];
        }

        WritePixelLine8(qra->dest, qra->left, row + qra->top,
                        winwidth, pixelrow, qra->temprp);

    }                           /* row */

  quit:
    return res;
}

/// Deep screens

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

# ifdef FAST_QUICKMAP
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
    if (GfxBase->LibNode.lib_Version >= 39) {

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

    if (GfxBase->LibNode.lib_Version >= 39) {
        FreeBitMap(temprp.BitMap);      // NULL is OK.

    } else {
        for (i = 0; i < dest->BitMap->Depth && tempbm.Planes[i]; i++)
            FreeRaster(tempbm.Planes[i], winwidth, 1);
    }
    if (pixelrow)
        pfree(pixelrow);

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/
