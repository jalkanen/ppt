/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt
    MODULE : display.c

    $Id: display.c,v 1.61 1999/02/21 20:49:46 jj Exp $

    Contains display routines.

*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/// Includes

#include "defs.h"
#include "misc.h"
#include "render.h"

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef INTUITION_ICCLASS_H
#include <intuition/icclass.h>
#endif

#ifndef GRAPHICS_GFXBASE_H
#include <graphics/gfxbase.h>
#endif

#include "gui.h"

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

#ifndef PROTO_GADTOOLS_H
#include <proto/gadtools.h>
#endif

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#include <proto/cybergraphics.h>
#include <cybergraphics/cybergraphics.h>

///

/*----------------------------------------------------------------------*/
/* Defines */

#define FLASHING_DISPRECT    /* Use IDCMP_INTUITICKS? */

#undef NO_WINDOW_BORDERS   /* Forget window border gadgets? */

/*
 *  Which way do you want the colormap to be built?
 *  0 = Use constant space division
 *  1 = Use frame
 *  2 = Read from file
 */

#define QUICK_COLORMAP      0

/*----------------------------------------------------------------------*/
/* Global variables */

UWORD textpen;

/*----------------------------------------------------------------------*/
/// Internal prototypes

Prototype VOID      CloseWindowSafely( struct Window * );
Prototype PERROR    DisplayFrame( FRAME * );
Prototype PERROR    RemoveDisplayWindow( FRAME *f );
Prototype PERROR    OpenDisplay( void );
Prototype PERROR    CloseDisplay( void );
Prototype UBYTE     SlowBestMatchPen8( COLORMAP *cm, UBYTE ncolors, UBYTE r, UBYTE g, UBYTE b );
Prototype VOID      GuessDisplay( FRAME * );
Prototype UWORD     GetMinimumDepth( ULONG );
Prototype VOID      ShowText( FRAME *, UBYTE *, EXTBASE * );

Local VOID BuildQuickColormap( struct Screen *, UBYTE, COLORMAP * );
///

/*----------------------------------------------------------------------*/
/* Code */

/// Alloc & FreeDisplay()
Prototype DISPLAY *AllocDisplay(EXTBASE *ExtBase);

DISPLAY *AllocDisplay(EXTBASE *ExtBase)
{
    DISPLAY *d;

    D(bug("AllocDisplay()\n"));

    if(d = pmalloc( sizeof(DISPLAY) ) ) {
        bzero( d, sizeof(DISPLAY) );
        D(bug("\tnew display = %08X\n",d ));
        d->DPIX = d->DPIY = 72;
    }
    return d;
}

Prototype VOID FreeDisplay( DISPLAY *d, EXTBASE *ExtBase );

VOID FreeDisplay( DISPLAY *d, EXTBASE *ExtBase )
{
    D(bug("FreeDisplay(%08X)\n",d));
    if(d) pfree(d);
}
///

/// CopyDisplay()
/*
    Copies everything necessary to the preferences.  Does not copy
    hooks or pointers.
*/

Prototype VOID CopyDisplay( DISPLAY *, DISPLAY * );

VOID CopyDisplay( DISPLAY *src, DISPLAY *dst )
{
    dst->height = src->height;
    dst->width  = src->width;
    dst->dispid = src->dispid;
    dst->ncolors = src->ncolors;
    dst->type   = src->type;
    dst->depth  = src->depth;
    dst->renderq = src->renderq;
    dst->dither = src->dither;
    dst->cmap_method = src->cmap_method;
    strcpy( dst->title, src->title );
    strcpy( dst->scrtitle, src->title );
    strcpy( dst->palettepath, src->palettepath );
    dst->forcebw = src->forcebw;
    dst->drawalpha = src->drawalpha;
    dst->DPIX = src->DPIX;
    dst->DPIY = src->DPIY;
    dst->keephidden = src->keephidden;
}
///

/// GuessDisplay() and associates

/*
    Gives a proper name for a given display mode id.
*/

Prototype VOID GetNameForDisplayID( ULONG, UBYTE *, LONG );

VOID GetNameForDisplayID( ULONG id, UBYTE *buffer, LONG length )
{
    struct NameInfo query;

    if( GetDisplayInfoData( NULL, (UBYTE*) &query, sizeof(query),
                            DTAG_NAME, id ) )
    {
        strncpy( buffer, query.Name, length );
    } else {
        strncpy( buffer, GetStr(MSG_UNKNOWN_DISPLAY_ID), length );
    }
}

/*
    Select the minimum amount of bitplanes that can display this many
    colors.
*/
UWORD GetMinimumDepth( ULONG ncolors )
{
    UWORD depth;

    for(depth = 1; depth < 24; depth++ ) {
        if( (1 << depth) >= ncolors )
            break;
    }

    D(bug("\t\tGot depth %u for %u colors\n",depth,ncolors));

    return depth;
}

/*
    Attempts to guess a display mode.
    BUG: Does not use correct aspect ratio.
    BUG: <V39 does not work correctly. Should check for ECS and attempt to
    open this thing.
*/

#define LACE_KEY 0x04

Local
void GuessModeAndDepth( FRAME *frame, ULONG *Modeid, UBYTE *Depth, EXTBASE *ExtBase )
{
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;
    ULONG id = 0L;
    UBYTE depth;
    struct TagItem ida[] = {
        { BIDTAG_NominalWidth, 0 },
        { BIDTAG_NominalHeight, 0 },
        { BIDTAG_Depth, 0L },
        { BIDTAG_SourceID, 0 },
        { TAG_END, 0L }
    };

    ida[0].ti_Data = frame->pix->width;
    ida[1].ti_Data = frame->pix->height;
    ida[3].ti_Data = frame->disp->dispid;


    /*
     *  Guess display depth and convert HAM and EHB to simple colormapped output
     */

    if(frame->disp->depth == 0) {
        frame->disp->depth = frame->pix->origdepth;
        frame->disp->ncolors = 1 << frame->disp->depth;
    }

    /*
     *  Select minimum depth for this amount of colors.
     */

    depth = GetMinimumDepth( frame->disp->ncolors );

    if( (frame->disp->dispid != INVALID_ID) && (frame->disp->dispid & EXTRAHALFBRITE_KEY) )
        depth++; /* EHB is not used with depths > 5 */

    if( GFXV39 ) {
        ida[2].ti_Data = (depth > 8) ? 8 : depth;
        id = BestModeIDA( ida );
        D(bug("\t\tBestModeIDA() suggests modeid %08X for this display\n",id));
    } else {

        /*
         *  Choose da old way.
         *  BUG: This is definately not very correct and does not allow
         *  for VGA modes.
         */

        if(frame->pix->width > 320)
            id &= HIRES_KEY;
        if(frame->pix->height > 256)
            id &= LACE_KEY;
    }

    /*
     *  Check for AGA, which allows all display modes to have depths upto 8.
     *  Do also a check for CyberGfx, which is our preferred mode.
     */

    if( CyberGfxBase ) {
        if( IsCyberModeID( id ) ) {
            LONG d;

            d = GetCyberIDAttr( CYBRIDATTR_DEPTH, id );
            if( d > 8 ) {
                depth = 8;
            }
        }
    } else {
        if( frame->pix->origdepth > 8 ) {
            id |= HAM_KEY;
        }

        if( IS_AGA ) {
            if(depth > 8)
                depth = 8;
        } else {
            if(depth > 4)
                depth = 4; /* BUG: Not true for lower screen modes. */
        }
    }

    if(ModeNotAvailable( id ) != NULL)
        id = DEFAULT_MONITOR_ID; /* Use PAL or NTSC, as required */

    id &= ~(EXTRAHALFBRITE_KEY);

    D(bug("\tReturning modeid=0x%08X, depth=%u\n",id,depth));

    *Modeid = id;
    *Depth = depth;
}

/*
    Make an educated guess on how to display the current image. This is
    either based on the preferences set in the file or guessed from
    the image type.

    1) Dispprefs are installed into the disp by loader
    2) if they're not, use this routine to make an educated guess
    3) open dispprefswin, which lets user make modifications
    4) check for correctness
    5) install back to disp
    6) render with current prefs.

    BUG: Locking is not OK.
    BUG: Does not understand about deep screens
    BUG: Goes wrong on an ECS machine
*/

VOID GuessDisplay( FRAME *frame )
{
    DISPLAY *disp = frame->disp;
    ULONG modeid = 0L;
    UBYTE depth = 0;

    LOCK(frame);
    frame->disp->depth = frame->pix->origdepth;
    if(frame->disp->depth > 8) frame->disp->depth = 8;
    frame->disp->ncolors = 1 << frame->disp->depth;
    UNLOCK(frame);

    if(disp->dispid == 0L) disp->dispid = frame->pix->origmodeid;

    GuessModeAndDepth( frame, &modeid, &depth, globxd );

    disp->dispid  = modeid;
    disp->depth   = depth;

    if( modeid & HAM_KEY ) {
        if( depth == 8 ) {
            disp->renderq = RENDER_HAM8;
            disp->ncolors = 64;
        } else {
            disp->renderq = RENDER_HAM6;
            disp->ncolors = 16;
        }
        D(bug("\tGuessDisplay proposes HAM mode!\n"));
    } else {
        if(modeid & EXTRAHALFBRITE_KEY)  {
            disp->renderq = RENDER_EHB;
            disp->ncolors = 32; /* BUG: 64? */
            D(bug("\tGuessDisplay proposes EHB mode!\n"));
        } else {
            disp->renderq = RENDER_NORMAL;
            D(bug("\tGuessDisplay proposes normal bitmapped mode\n"));
        }
    }

}
///

/// Color routines [SetRGB8(), LoadRGB8()]
/*
    Set a given color to the given ViewPort.  Uses the correct OS
    routines.
*/

Prototype VOID SetRGB8( struct ViewPort *, ULONG n, UBYTE r, UBYTE g, UBYTE b, EXTBASE * );

VOID SetRGB8( struct ViewPort *vp, ULONG n, UBYTE r, UBYTE g, UBYTE b, EXTBASE *ExtBase )
{
    struct GfxBase *GfxBase = ExtBase->lb_Gfx;

    if(vp == NULL) return;

    if( GFXV39 ) {
        SetRGB32(vp, n, r*0x01010101, g*0x01010101, b*0x01010101 );
    } else {
        SetRGB4(vp, n, (((UWORD)r)+9)/17, (((UWORD)g)+9)/17, (((UWORD)b)+9)/17 );
    }
}

/*
    Loads a given colormap to the given ViewPort. Colors must
    be in RGB-order, right after each other. This will use either
    LoadRGB32() or LoadRGB4(), whichever is supported by OS.

    Currently the 4bit version rounds off towards bright.
*/

Prototype void LoadRGB8( struct ViewPort *, COLORMAP *, long, EXTBASE * );

void LoadRGB8( struct ViewPort *vp, COLORMAP *ct, long ncolors, EXTBASE *xb )
{
    UBYTE r,g,b,*ct8;
    int i;
    struct GfxBase *GfxBase = xb->lb_Gfx;

    ct8 = (UBYTE *)ct;

    D(bug("LoadRGB8(%lu colors)\n",ncolors));

    if( GFXV39 ) {
        ULONG *ct32;

        if( NULL == (ct32 = (ULONG *)pmalloc( (ncolors*3+2) * 4 )))
            return;

        ct32[0] = ncolors << 16; /* ncolors colors, first at zero */
        for( i = 0; i < ncolors; i++ ) {
            ct32[3*i+1] = (ct[i].r << 24) + (ct[i].r << 16) + (ct[i].r << 8) + ct[i].r;
            ct32[3*i+2] = (ct[i].g << 24) + (ct[i].g << 16) + (ct[i].g << 8) + ct[i].g;
            ct32[3*i+3] = (ct[i].b << 24) + (ct[i].b << 16) + (ct[i].b << 8) + ct[i].b;
        }

        ct32[ ncolors * 3 + 1 ] = 0L;

        LoadRGB32( vp, ct32 );
        pfree(ct32);
    } else {
        UWORD *ct4;
        ct4 = (UWORD *) pmalloc( ncolors << 1 ); /* BUG: should check */
        for(i = 0; i < ncolors; i++ ) {
#if 0
            r = *ct8++ & 0xF0;
            g = *ct8++ & 0xF0;
            b = *ct8++;
            ct4[i] = (r << 4) | g | (b >> 4);
#else
            ct8++;  /* Skip alpha */
            r = (UBYTE)(((UWORD)*ct8++ + 9) / 17); /* This is the correct way to calculate */
            g = (UBYTE)(((UWORD)*ct8++ + 9) / 17); /* the color values. Think about it for */
            b = (UBYTE)(((UWORD)*ct8++ + 9) / 17); /* a moment and it'll be clear. */
            ct4[i] = (r << 8) | (g << 4) | b;
#endif
        }
        LoadRGB4( vp, ct4, ncolors );
        pfree(ct4);
    }
}

#ifdef DEBUG_MODE

#define SQR(x) ( (x) * (x) )

UBYTE SlowBestMatchPen8( COLORMAP *cm, UBYTE ncolors, UBYTE r, UBYTE g, UBYTE b )
{
    int i,idx = 0;
    ULONG min = 0xFFFFFFFF,dist;

    for(i = 0; i < ncolors; i++) {
        dist = SQR( (WORD)r - (WORD)cm[i].r )
             + SQR( (WORD)g - (WORD)cm[i].g )
             + SQR( (WORD)b - (WORD)cm[i].b );
        if(dist <= min) {
            min = dist;
            idx = i;
        }
    }
    return (UBYTE)idx;
}

#endif
///

/// SetupStandardColors()
Local
VOID SetUpStandardColors( UBYTE depth, COLORMAP *cm )
{
    /* And the first colors to begin with:
       BUG: These definately shouldn't be hardcoded */
    cm[0].r = cm[0].g = cm[0].b = 0xAA;
    cm[1].r = cm[1].g = cm[1].b = 0x00;
    cm[2].r = cm[2].g = cm[2].b = 0xFF;
    cm[3].r = 0x66; cm[3].g = 0x88; cm[3].b = 0xBB;

    /*
     *  Sprite colors.  Shouldn't be hardcoded as well.
     */

    if(depth > 4) {
        cm[17].r = 224; cm[17].g = cm[17].b = 64;
        cm[18].r = cm[18].g = cm[18].b = 0;
        cm[19].r = cm[19].g = 224; cm[19].b = 192;
    }
}
///

/// BuildQuick*Colormap()

#if (QUICK_COLORMAP == 0)

Local
VOID BuildQuickColorColormap( struct Screen *scr, UBYTE depth, COLORMAP *cm )
{
    int nR, nG, nB, r, g, b, i;
    int nColors;
    extern UBYTE QuickRemapTable_Color[];
    struct ColorMap *cmp = NULL;

    if( scr && GFXV39 )
        cmp = scr->ViewPort.ColorMap;

    switch( depth ) {
        case 5:
            nR = 3; nG = 4; nB = 2; /* Total of 24 colors */
            break;
        case 6:
            nR = 3; nG = 6; nB = 3; /* Total of 54 colors */
            break;
        case 7:
            nR = 5; nG = 6; nB = 4; /* Total of 120 colors */
            break;
        case 8:
            nR = 5; nG = 9; nB = 5; /* Total of 225 colors */
            break;
        default:
            InternalError( "Wrong depth specified" );
            return;
            break;
    }

    nColors = 8 + nR * nG * nB;

    SetUpStandardColors( depth, cm );

    i = 0;
    for( r = 0; r < nR; r++ ) {
        for( g = 0; g < nG; g++ ) {
            for( b = 0; b < nB; b++ ) {
                if( i >= 0 && i <= 3 ) i = 4;
                if( i >= 17 && i <= 20 ) i = 21;
                cm[i].r = 255 * r / nR;
                cm[i].g = 255 * g / nG;
                cm[i].b = 255 * b / nB;
                i++;
            }
        }
    }

    if( scr ) LoadRGB8( &scr->ViewPort, cm, nColors, globxd);

    for( i = 0; i < 16; i++ ) {
        int j;

        for( j = 0; j < 16; j++ ) {
            int k;

            for( k = 0; k < 16; k++ ) {
                int offset;
                /*
                 *  If this is V39+, we'll use colormap sharing
                 */

                offset = (i<<8)+(j<<4)+k;

                QuickRemapTable_Color[offset] =
                        BestMatchPen8( cm, nColors,
                                      (UBYTE)(i << 4),
                                      (UBYTE)(j << 4),
                                      (UBYTE)(k << 4));

                if( cmp )
                    ObtainPen( cmp, QuickRemapTable_Color[offset], 0,0,0, PEN_NO_SETCOLOR );

            }
        }
    }
}

#elif (QUICK_COLORMAP == 1 )

/*
 *  Create a color map for the main display if a colormapped screen is requested.
 *  Colors 0-3 are as standard Intuition
 *  Colors 4-15 are used to create a grayscape
 *  Colors 16-19 are used to preserve the Intuition sprite
 *  Colors 20->  are used to put a median cutted colormap.
 */

Local
VOID BuildQuickColorColormap( struct Screen *scr, UBYTE depth, COLORMAP *cm )
{
    int ncolors = 1<<depth;
    int i;
    FRAME *foo;
    struct RenderObject *rdo;
    extern UBYTE QuickRemapTable_Color[];
    struct ColorMap *cmp = NULL;

    if( scr && GFXV39 )
        cmp = scr->ViewPort.ColorMap;

    D(bug("BuildQuickColorColormap()\n"));

    SetUpStandardColors( depth, cm );

    /*
     *  Setup a gray scale
     */

    for( i = 4; i < 16; i++ ) {
        cm[i].r = cm[i].g = cm[i].b = (255*(i - 4))/12; /* BUG: Disregards b&W*/
    }

    /*
     *  Let us scam for a while and let the median cut
     *  algorithm to take care of calculating the color map.
     */

    foo = NewFrame( 1024, 32, 3, globxd );

    if( foo ) {

        sprintf(foo->disp->title, "%d colors...", ncolors-20);

        AllocInfoWindow( foo, globxd );

        InitProgress( foo, "Initializing screen...", 0, 256, globxd );
        DisableInfoWindow( foo->mywin, globxd );

        if( !(rdo = pzmalloc( sizeof(struct RenderObject) ) ) ) {
            Panic("Out of memory allocating initial renderobject!");
        }

        foo->renderobject = rdo;
        rdo->frame = foo;
        rdo->ExtBase = globxd;

        foo->disp->ncolors = ncolors - 20;

        D(bug("\tAttempting to allocate %d colors.\n",foo->disp->ncolors));

        if( Palette_MedianCutI( rdo ) == PERR_OK ) {

            rdo->colortable = (COLORMAP *) ((ULONG)cm + 20*sizeof(COLORMAP));
            rdo->ncolors = foo->disp->ncolors;

            /*
             *  Build a dummy frame pixmap
             */

            for( i = 0; i < 32; i++ ) {
                int j;
                ROWPTR cp;

                cp = GetPixelRow( foo, i, globxd );

                for( j = 0; j < 1024; j++ ) {
                    cp[3*j]   = i << 3;       // RED
                    cp[3*j+1] = (j/32) << 3;  // BLUE
                    cp[3*j+2] = (j%32) << 3;  // GREEN
                }

                PutPixelRow( foo, i, cp, globxd );
            }

            /*
             *  Build the colormap.
             */

            rdo->Palette( rdo );
            // DoShowColorTable( ncolors, cm );

            /*
             *  Build the lookup-tables and load the data, if
             *  a Screen has been defined
             */

            InitProgress( foo, "Setting up main display...", 0, 16, globxd );

            if( scr ) LoadRGB8( &scr->ViewPort, cm, ncolors, globxd);

            for( i = 0; i < 16; i++ ) {
                int j;

                Progress( foo, i, globxd );

                for( j = 0; j < 16; j++ ) {
                    int k;

                    for( k = 0; k < 16; k++ ) {
                        int offset;
                        /*
                         *  If this is V39+, we'll use colormap sharing
                         */

                        offset = (i<<8)+(j<<4)+k;

                        QuickRemapTable_Color[offset] =
                                BestMatchPen8( cm, ncolors,
                                              (UBYTE)(i << 4),
                                              (UBYTE)(j << 4),
                                              (UBYTE)(k << 4));

                        if( cmp )
                            ObtainPen( cmp, QuickRemapTable_Color[offset], 0,0,0, PEN_NO_SETCOLOR );

                    }
                }
            }

            FinishProgress( foo, globxd );

            /*
             *  Finished, kill away.
             */

            (rdo->PaletteD)( rdo );

        }

        pfree( rdo ); foo->renderobject = NULL;
        DeleteInfoWindow( foo->mywin, globxd );
        RemFrame(foo, globxd);
    } else {
        BuildQuickColormap( scr, depth, cm );
    }
}

#elif (QUICK_COLORMAP == 2)
Local
VOID BuildQuickColorColormap( struct Screen *scr, UBYTE depth, COLORMAP *cm )
{
    extern unsigned char *colormap_array[];
    extern UBYTE QuickRemapTable_Color[];
    int nColors = 1<<depth, i;
    struct ColorMap *cmp = NULL;
    D(APTR bench);

    D(bug("BuildQuickColorColormap()\n"));

    D(bench = StartBench());

    if( scr && GFXV39 )
        cmp = scr->ViewPort.ColorMap;

    for( i = 0; i < nColors; i++ ) {
        cm[i].r = colormap_array[depth][3*i];
        cm[i].g = colormap_array[depth][3*i+1];
        cm[i].b = colormap_array[depth][3*i+2];
    }

    SetUpStandardColors( depth, cm );

    if( scr ) LoadRGB8( &scr->ViewPort, cm, nColors, globxd);

    for( i = 0; i < 16; i++ ) {
        int j;

        for( j = 0; j < 16; j++ ) {
            int k;

            for( k = 0; k < 16; k++ ) {
                int offset;
                /*
                 *  If this is V39+, we'll use colormap sharing
                 */

                offset = (i<<8)+(j<<4)+k;

                QuickRemapTable_Color[offset] =
                        BestMatchPen8( cm, nColors,
                                      (UBYTE)(i << 4),
                                      (UBYTE)(j << 4),
                                      (UBYTE)(k << 4));

                if( cmp )
                    ObtainPen( cmp, QuickRemapTable_Color[offset], 0,0,0, PEN_NO_SETCOLOR );

            }
        }
    }

    D(StopBench(bench));
}

#endif

/*
    Creates a grayscale colormap for quick renderer.

    The grayscale colormap is generated as follows:
        Colors 0,1,2,3 are left alone, as they're used by the GUI.
        Colors 4-15 and 20-256 are used by the system to generate
            a greyscale.
        Colors 16-19 are left alone for the Intuition sprite.

    BUG: Should actually use the B/W colors found in the Intuition
         standard coloring scheme.
*/

Local
void BuildQuickColormap( struct Screen *scr, UBYTE depth, COLORMAP *cm )
{
    UWORD i,ncolors, mincolor;
    UBYTE val,*c;
    double step;
    extern UBYTE QuickRemapTable[];
    struct ColorMap *cmp = NULL;

    D(bug("BuildQuickColormap()\n"));

    if( scr && GFXV39 )
        cmp = scr->ViewPort.ColorMap;

    ncolors = (1<<depth);
    mincolor = (ncolors > 16) ? 8 : 4;

    c = (UBYTE *)&cm[4]; /* Skip first four values */

    step = 256.0 / (ncolors - (mincolor + 1) );

    SetUpStandardColors( depth, cm );

    for(i = mincolor; i < ncolors; i++) {
        if( i == 20 ) c += 4*sizeof(COLORMAP); /* Skip sprite colors to preserve pointer */
        val = (UBYTE)( (i-mincolor) * step);
        // DEBUG("i = %d, c = %08X, val = %d\n",i,c,val);
        c++;  /* skip alpha */
        *c++ = val;
        *c++ = val;
        *c++ = val;
    }

    if( scr )
        LoadRGB8( &scr->ViewPort, cm, ncolors, globxd);

    /* Create the quick remap table */
    for(i = 0; i < QUICKREMAPSIZE; i++) {
        UBYTE j;

        j = ( i > 255 ) ? 255 : i;

        QuickRemapTable[i] = BestMatchPen8( cm, ncolors, (UBYTE)j,(UBYTE)j,(UBYTE)j );

        if( cmp )
            ObtainPen( cmp, QuickRemapTable[i], 0,0,0, PEN_NO_SETCOLOR );

    }
    // DoShowColorTable( ncolors, cm );
}
///

/// ShowText()
/*
    Write a small busy-text into the frame window.
*/

VOID ShowText( FRAME *frame, UBYTE *txt, EXTBASE *xb )
{
    WORD clen;

    D(bug("ShowText()..."));
    if( frame->disp ) {
        if( frame->disp->win ) {
            if( frame->disp->RenderArea ) {
                struct IBox *ibox;
                WORD x,y;
                struct RastPort *rp = frame->disp->win->RPort;
                struct TextExtent te;

                clen = strlen( txt );

                /*
                 *  Get the display area and then render into it.
                 */

                TextExtent( rp, txt, clen, &te );
                GetAttr( AREA_AreaBox, frame->disp->RenderArea, (ULONG *)&ibox );

                x  = ibox->Left + ((ibox->Width  - te.te_Width ) >> 1);
                y  = ibox->Top  + ((ibox->Height - te.te_Height ) >> 1);

                SetAPen( rp, textpen );
                SetBPen( rp, 0 );
                SetDrMd( rp, JAM2 );
                Move( rp, x, y );
                Text( rp, txt, clen );
            } else {
                D(bug("No renderarea\n"));
            }
        } else {
            D(bug("No window\n"));
        }
    } else {
        D(bug("No display\n"));
    }
    D(bug("done\n"));
}
///

/// UndisplayFrame()
/*
    This routine closes down the display, but does not deallocate
    resources.
*/

Prototype PERROR UndisplayFrame( FRAME * );

PERROR UndisplayFrame( FRAME *frame )
{
    D(bug("UndisplayFrame()\n"));

    if( frame ) {
        LOCK(frame);
        if( frame->disp->RenderArea ) {
            WindowClose( frame->disp->Win );
            frame->disp->win = NULL;
        }
        UNLOCK(frame);
    }

    return PERR_OK;
}
///

/// RemoveDisplayWindow()
/*
    This routine kills off a quickrenderwindow.
*/

PERROR RemoveDisplayWindow( FRAME *f )
{
    D(bug("RemoveDisplayWindow()\n"));

    if(f->disp->RenderArea) {
        LOCK(f);
        DisposeObject(f->disp->Win);
        f->disp->RenderArea = f->disp->Win =
        f->disp->GO_BottomProp = f->disp->GO_RightProp = NULL;
        f->disp->win = NULL;
        UNLOCK(f);
        return PERR_OK;
    }

    InternalError("Freeing the frame display twice!");

    return PERR_OK;
}
///

/// Hooks

/*
    NOTE: Runs on input.device's task.

    BUG: should not be here.
    BUG: it is possible that the clicking of mousebutton and the movement
         happens out of sync for some reason.
*/

SAVEDS ASM
VOID AreaSelectHook( REG(a0) struct Hook *hook,
                     REG(a2) Object *object,
                     REG(a1) struct IntuiMessage *imsg )
{
    ULONG method = 0;

    switch( imsg->Class ) {
        case IDCMP_INTUITICKS:
            /*
             *  If the user is NOT holding the LMB down, then we'll
             *  report this event back to the program in order not to confuse
             *  the select box drawing routines.
             */
            if( !(imsg->Qualifier & IEQUALIFIER_LEFTBUTTON))
                method = GID_DW_INTUITICKS;
            break;

        case IDCMP_MOUSEMOVE:
#if 0
            /*
             *  Only if the user is holding the LMB down, shall
             *  we report this to the main program to keep the flood
             *  if mmove events down.
             */
            if(imsg->Qualifier & IEQUALIFIER_LEFTBUTTON)
                method = GID_DW_MOUSEMOVE;
#else
            /*
             *  Pass it down. Now we need the info on refreshing the
             *  co-ordinate window.
             *  BUG: Maybe should check for the existence of it and
             *       only then pass it down?
             */

            method = GID_DW_MOUSEMOVE;
#endif
            break;

        case IDCMP_MOUSEBUTTONS:
            switch(imsg->Code) {
                case SELECTUP:
                    method = GID_DW_SELECTUP;
                    break;
                case SELECTDOWN:
                    if( imsg->Qualifier & IEQUALIFIER_CONTROL ) {
                        method = GID_DW_CONTROLSELECTDOWN;
                    } else {
                        method = GID_DW_SELECTDOWN;
                    }
                    break;
            }
            break;

    }

    if(method)
        DoMethod( object, WM_REPORT_ID, method, 0L );

}

/*
    This hook is used to notify the main program that the window
    zoom area has actually changed.
*/

SAVEDS ASM
ULONG DPropHook( REG(a0) struct Hook *hook,
                 REG(a2) Object *object,
                 REG(a1) struct opUpdate *opu )
{
    if( (opu->MethodID == OM_UPDATE) &&
        ((opu->opu_Flags & OPUF_INTERIM) == 0) )
    {
        FRAME *f;

        f = FindFrame( (ID) (hook->h_Data) );

        /*
         *  Make a sanity check
         */

        if(f) {
            if( !f->zooming ) {
                DoMethod( f->disp->Win, WM_REPORT_ID, GID_DW_LOCATION,
                          WMRIF_TASK, globals->maintask, TAG_DONE );
            }
        }
    }

    return 0;
}
///

/// GimmeQuickDisplayWindow()
PERROR GimmeQuickDisplayWindow( FRAME *frame, EXTBASE *ExtBase )
{
    extern struct NewMenu PPTMenus[];
    struct IBox winbox;
    PERROR res = PERR_OK;
    PIXINFO *p = frame->pix;

    D(bug("GimmeQuickDisplayWindow()\n"));

    /*
     *  Does the area object exist? If not, we create it now.
     */

    if(!frame->disp->RenderArea) {
        Object *ra;

        D(bug("\trenderarea...\n"));

        ra = NewObject( DropAreaClass, NULL,
                        AREA_MinWidth, 40, AREA_MinHeight, 20,
                        BT_DropObject, TRUE,
                        GA_ID,         GID_DW_AREA,
                        ICA_TARGET,    ICTARGET_IDCMP,
                        FRM_EdgesOnly, TRUE,
                        ButtonFrame,
                        BT_HelpHook,   &HelpHook,
                        BT_HelpNode,   "PPT.guide/ImageWindow",
                        TAG_END );

        if(!ra) {
            D(bug("\tAREA wont open\n"));
            return 0;
        }

        LOCK(frame);
        frame->disp->RenderArea = ra;
        UNLOCK(frame);
    }

    /*
     *  Does the window object exist? If not, we create it now.
     */

    if(!frame->disp->Win) {
        /*
         *  First, create the Hook structure to be used in IDCMP hook.
         */

        D(bug("\twindow object...\n"));

        LOCK(frame);
#pragma msg 225 ignore
        frame->disp->qhook.h_Entry = AreaSelectHook;

        frame->disp->prophook.h_Entry = DPropHook;
#pragma msg 225 warn

        frame->disp->prophook.h_Data  = (APTR) (frame->ID);

        /*
         *  Guess a good window size and location
         *  BUG: Should read the window top border height from somewhere.
         *  BUG: Will overflow at some point.
         */

        winbox.Top    = (frame->ID % 10) * 8 + (frame->ID/10)*8;
        winbox.Left   = (frame->ID % 10) * 8;
        winbox.Width  = p->width;

        if( p->DPIX && p->DPIY )
            winbox.Height = p->height * p->DPIY / p->DPIX;
        else
            winbox.Height = p->height;

        winbox.Height = winbox.Height * globals->maindisp->DPIX / globals->maindisp->DPIY;

        /*
         *  Now, create window object.
         */

        frame->disp->Win = WindowObject,
            WINDOW_Title, &(frame->disp->title[0]),
            WINDOW_ScreenTitle, frame->disp->scrtitle,
            WINDOW_Screen, MAINSCR,
            WINDOW_MenuStrip, PPTMenus,
            WINDOW_SharedPort, MAINWIN->UserPort,
            WINDOW_SmartRefresh, TRUE,
#ifdef FLASHING_DISPRECT
            WINDOW_IDCMP, IDCMP_INTUITICKS|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,
#else
            WINDOW_IDCMP, IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,
#endif
            WINDOW_IDCMPHook, &(frame->disp->qhook),
            WINDOW_IDCMPHookBits, IDCMP_INTUITICKS|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,
            WINDOW_ReportMouse, TRUE,
            WINDOW_Bounds,    &winbox,
            WINDOW_SizeRight, TRUE,
            WINDOW_NoBufferRP, TRUE,
#ifndef NO_WINDOW_BORDERS
            WINDOW_TBorderGroup,
                HGroupObject, HOffset(0), VOffset(0), Spacing(0),
                    StartMember,
                        ButtonObject,
                            VIT_BuiltIn,    BUILTIN_ARROW_UP,
                            BUTTON_EncloseImage, TRUE,
                            GA_ID,          GID_DW_HIDE,
                            BorderFrame,
                        EndObject,
                    EndMember,
                EndObject,
            WINDOW_RBorderGroup,
                VGroupObject,
                    StartMember,
                        frame->disp->GO_RightProp = PropObject,
                            PGA_Top,        frame->zoombox.Top,
                            PGA_Total,      p->height,
                            PGA_Visible,    frame->zoombox.Height,
                            PGA_Arrows,     TRUE,
                            PGA_NewLook,    TRUE,
                            BorderFrame,
                            GA_ID,          GID_DW_RIGHTPROP,
                            GA_RelVerify,   TRUE,
                        EndObject,
                    EndMember,
                EndObject,

            WINDOW_BBorderGroup,
                HGroupObject,
                    StartMember,
                        frame->disp->GO_BottomProp = PropObject,
                            PGA_Top,        frame->zoombox.Left,
                            PGA_Total,      p->width,
                            PGA_Visible,    frame->zoombox.Width,
                            PGA_Arrows,     TRUE,
                            PGA_Freedom,    FREEHORIZ,
                            PGA_NewLook,    TRUE,
                            GA_ID,          GID_DW_BOTTOMPROP,
                            GA_RelVerify,   TRUE,
                            BorderFrame,
                        EndObject,
                    EndMember,
                EndObject,
#endif

            WINDOW_MasterGroup,
                VGroupObject,
                    StartMember, frame->disp->RenderArea, EndMember,
                EndObject, /* Mastervgroup */
            EndObject;

        if(!frame->disp->Win) {
            D(bug("\tWindow wont create\n"));
            DisposeObject( frame->disp->RenderArea );
            frame->disp->RenderArea = NULL;
            return PERR_WINDOWOPEN;
        }

#ifndef NO_WINDOW_BORDERS
        AddHook( frame->disp->GO_BottomProp, &(frame->disp->prophook) );
        AddHook( frame->disp->GO_RightProp,  &(frame->disp->prophook) );
#endif

        UNLOCK(frame);
    }

    return res;
}
///

/// HideDisplayWindow() & ShowDisplayWindow()

Prototype PERROR HideDisplayWindow( FRAME *frame );

PERROR HideDisplayWindow( FRAME *frame )
{
    PERROR res = PERR_FAILED;

    if( frame->disp ) {
        if( frame->disp->win ) {
            UndisplayFrame( frame );
            frame->selstatus = 0;
            frame->disp->keephidden = TRUE;
            res = PERR_OK;
        }
    }
    return res;
}

Prototype PERROR ShowDisplayWindow( FRAME *frame );

PERROR ShowDisplayWindow( FRAME *frame )
{
    PERROR res = PERR_FAILED;

    if( frame->disp ) {
        if( !frame->disp->win ) {
            frame->disp->keephidden = FALSE;
            if( MakeDisplayFrame( frame ) ) {
                DisplayFrame( frame );
                res = PERR_OK;
            }
        }
    }

    return res;
}

///

/// OpenQuickDisplayWindow()
/*
    Opens a preview window.
*/

PERROR OpenQuickDisplayWindow(FRAME *frame, EXTBASE *ExtBase)
{
    struct Window *win;

    D(bug("OpenQuickDisplayWindow(%08X)...\n",frame));

    SetAttrs( frame->disp->Win, WINDOW_Screen, MAINSCR, TAG_DONE );

    LOCK(frame);
    win = WindowOpen( frame->disp->Win );

    if(!win) {
        DisposeObject( frame->disp->Win );
        frame->disp->Win = frame->disp->RenderArea = NULL;
        D(bug("\t\twindow did not open\n"));
        UNLOCK(frame);
        return PERR_WINDOWOPEN;
    }

    SetFont( win->RPort, globals->userprefs->maintf );

    frame->disp->win = win;
    frame->disp->scr = MAINSCR;

    UpdateInfoWindow( frame->mywin, globxd );
    KludgeWindowMenus();

    UNLOCK(frame);

    return PERR_OK;
}
///

/// MakeDisplayFrame()
/*
    Just a shortcut.  Will do everything DisplayFrame() should,
    but will not draw the actual data.

    If the objects do not exist, will not allocate anything.
*/

Prototype PERROR MakeDisplayFrame( FRAME *frame );

PERROR MakeDisplayFrame( FRAME *frame )
{
    PERROR res = PERR_OK;

    /*
     *  Does the area object exist? If not, we create it now.
     */

    if( !frame->disp->Win ) {
        if( (res = GimmeQuickDisplayWindow( frame, globxd )) != PERR_OK )
            return res;
    } else {
        /*
         *  Just update the information.
         */
        D(bug("\twindow object ok, refresh display\n"));
        UpdateFrameInfo( frame );
        RefreshFrameInfo( frame, globxd );
    }

    /*
     *  If the window is not open, we open it now.
     */

    if(!frame->disp->win) {
        if( ( res = OpenQuickDisplayWindow(frame, globxd)) != PERR_OK )
            return res;
    }

    return res;
}
///
/// QuickDisplayFrame()
/*
    Quickdisplay a frame.

    Return code: ?

    BUG: Should really allocate the objects just once...
*/

Prototype ULONG QuickDisplayFrame( FRAME *frame );

ULONG QuickDisplayFrame( FRAME *frame )
{
    struct IBox *ibox;
    ULONG rc;

    D(bug("QuickDisplayFrame()\n"));

    if(!CheckPtr(frame->disp,"Display does not exist!"))
        return 0;

    if( MakeDisplayFrame( frame ) != PERR_OK ) {
        return 0;
    }

    /*
     *  Ask the area from the areaobject and then render to it.
     */

    D(bug("\trendering...\n"));

    GetAttr( AREA_AreaBox, frame->disp->RenderArea, (ULONG *)&ibox );

    BusyAllWindows(globxd);

    rc = QuickRender( frame, frame->disp->win->RPort,
                             ibox->Top, ibox->Left,
                             ibox->Height, ibox->Width, globxd );

    AwakenAllWindows(globxd);

    /*
     *  Make sure there is a select box visible
     */

    frame->selstatus &= ~SELF_RECTANGLE;
    DrawSelectBox( frame, 0L );

    return rc;
}
///
/// DisplayFrame()
/*
    This handles the preview window. The real render is handled
    elsewhere.

    In the future, this may be used to spawn a separate task. Maybe.
*/

PERROR DisplayFrame( FRAME *frame )
{
    PERROR res = PERR_OK;

    D(bug("DisplayFrame()\n"));

    if(!frame->disp) return PERR_OK;

    /*
     *  If this frame is supposed to be kept hidden, we do not
     *  wish to have it to pop up, unless the user says so.
     */

    if( frame->disp->keephidden ) {
        frame->reqrender = 0;
    } else {

        /*
         *  Check if we're in preview mode, ie. an external module
         *  has control over us.
         */

        if( IsPreview(frame) ) {
            res = DoPreview(frame);
        } else {

            /*
             *  Attempt to obtain a frame lock. If it fails, we will not render,
             *  but show a simple status message.
             */

            if( ObtainFrame( frame, BUSY_READONLY ) == FALSE ) {
                frame->reqrender = 1; /* Atomic */
                D(bug("\tFrame is busy...\n"));
                ShowText( frame, GetStr(MSG_BUSY), globxd );
            } else {
                res = QuickDisplayFrame( frame );

                frame->reqrender = 0;
                ReleaseFrame( frame );
            }
       } /* IsPreview */
    }

    return res;
}
///

/*-------------------------------------------------------------------------*/

/// OpenMainScreen()
const struct TagItem screenfailtext[] = {
    TAG_USER+OSERR_NOMONITOR, (ULONG)"Named monitor spec not available",
    TAG_USER+OSERR_NOCHIPS, (ULONG)"You need newer custom chips",
    TAG_USER+OSERR_NOMEM, (ULONG)"Couldn't get normal memory",
    TAG_USER+OSERR_NOCHIPMEM, (ULONG)"Couldn't get chip memory",
    TAG_USER+OSERR_PUBNOTUNIQUE, (ULONG)"Public screen name already used",
    TAG_USER+OSERR_UNKNOWNMODE, (ULONG)"Don't recognize mode asked for",
    TAG_USER+OSERR_TOODEEP, (ULONG)"Screen deeper than HW supports",
    TAG_USER+OSERR_ATTACHFAIL, (ULONG)"Failed to attach screens",
    TAG_USER+OSERR_NOTAVAILABLE, (ULONG)"Mode not available for unknown reason",
    TAG_DONE
};

/*
    Opens main screen.

    BUG: Should use some intelligent method in getting the pen values.
         (Maybe pass pen array from Initialize()?)
*/

struct Screen *OpenMainScreen( DISPLAY *d )
{
    ULONG errcode = 0;
    struct Screen *foo;
    UWORD mypens[] = {
        0, 1,
        1, 2,
        1, 3,
        1, 0,
        2, 1,
        2, 1,
        (UWORD)~0
    };
    struct DrawInfo *dri;
    struct DisplayInfo di;

    D(bug("OpenMainScreen()\n"));

    /*
     *  Open screen!
     */

    foo = OpenScreenTags( NULL,
        SA_Width,       d->width,
        SA_Height,      d->height,
        SA_Depth,       d->depth,
        SA_Pens,        mypens,
        SA_AutoScroll,  TRUE,
        SA_FullPalette, TRUE, /* Copy everything from WB */
        SA_Title,       std_ppt_blurb, /* To be added later. Show memory? */
        SA_DisplayID,   d->dispid,
        SA_Font,        globals->userprefs->mainfont,
        // SA_SysFont,     1, /* Use preferred font. BUG: should also be saved.*/
        SA_Type,        PUBLICSCREEN,
        SA_PubName,     PPTPUBSCREENNAME,
        SA_ErrorCode,   &errcode,
        // SA_Interleaved, TRUE,
        SA_SharePens,   TRUE, /* Allow externals to use our colormaps */
        TAG_END, 0L
        );

    if(!foo || errcode) {
        char *en;

        en = (char *)GetTagData(TAG_USER+errcode, (ULONG)GetStr(MSG_UNKNOWN_REASON), screenfailtext );

        Req(NULL,NULL,GetStr(MSG_OPENSCREEN_ERROR),
                      errcode, en);
        return NULL;
    }

    MAINSCR = foo;

    /*
     *  Check for deep screens
     */

    if( d->depth > 8 ) {
        D(bug("\tDeep screen detected, skipping colormaps...\n"));
        globals->maindisp->colortable = NULL;
    } else {
        /*
         *  Set colortable.  In color mode, we'll build a color map.
         */

        globals->maindisp->colortable = pmalloc(256*sizeof(COLORMAP));

        if( (globals->userprefs->colorpreview) && (d->depth >= 5) ) {
            BuildQuickColorColormap( MAINSCR, d->depth, globals->maindisp->colortable );
        } else {
            BuildQuickColormap( MAINSCR, d->depth, globals->maindisp->colortable );
            globals->userprefs->colorpreview = FALSE;
        }
        QuickRenderInit();
    }

    /* Read useful data for caching */

    dri = GetScreenDrawInfo(foo);
    if( dri )
        textpen = dri->dri_Pens[TEXTPEN];
    else
        textpen = 1;

    if(GetDisplayInfoData( NULL, (UBYTE *)&di, sizeof(di), DTAG_DISP, d->dispid )) {
        d->DPIX = di.Resolution.x; /* BUG: Should utilize monitor size */
        d->DPIY = di.Resolution.y;
        D(bug("\tScreen has aspect ratio (DPIX/DPIY): (%d/%d)\n",d->DPIX, d->DPIY ));
    }

    globals->WO_main = WindowObject,
        WINDOW_ScreenTitle, std_ppt_blurb,
        WINDOW_MenuStrip,   PPTMenus,
        WINDOW_Screen,      MAINSCR,
        WINDOW_SharedPort,  MainIDCMPPort,
        WINDOW_Backdrop,    TRUE,
        WINDOW_ShowTitle,   TRUE,
        WINDOW_Font,        globals->userprefs->mainfont,
        WINDOW_SmartRefresh,FALSE, /* No need */
        WINDOW_NoBufferRP,  TRUE,  /* Saves memory */
        WINDOW_Activate,    FALSE,
        WINDOW_ScaleHeight, 100,
        WINDOW_ScaleWidth,  100,
        WINDOW_MasterGroup,
            VGroupObject,
                BT_HelpHook, &HelpHook,
                BT_HelpNode, "PPT.guide/Main",
            EndObject,
    EndObject;

    if( globals->WO_main ) {
        MAINWIN = WindowOpen( globals->WO_main );
        if( !MAINWIN ) {
            DisposeObject( globals->WO_main );
            CloseScreen( foo );
            if( globals->maindisp->colortable) pfree( globals->maindisp->colortable );
            globals->maindisp->colortable = NULL;
            return NULL;
        }
    } else {
        CloseScreen( foo );
        if( globals->maindisp->colortable) pfree( globals->maindisp->colortable );
        globals->maindisp->colortable = NULL;
        return NULL;
    }

    /*
     *  Lastly, make the screen truly public.
     */

    PubScreenStatus( foo, 0 );

    return foo;
}
///
/// CloseMainScreen()
PERROR CloseMainScreen( void )
{
    PERROR res = PERR_OK;

    D(bug("CloseMainScreen()\n"));

    if(globals->maindisp->scr) {

        if( globals->WO_main ) {
            DisposeObject( globals->WO_main );
            MAINWIN = NULL;
            globals->WO_main = NULL;
        }

        while(MAINSCR) {
            if(CloseScreen(globals->maindisp->scr)) {
                if( globals->maindisp->colortable) pfree(globals->maindisp->colortable);
                globals->maindisp->colortable = NULL;
                MAINSCR = NULL;
            } else {
                D(bug("Could not close main screen!!!\n"));
                Req( NEGNUL, GetStr(MSG_RETRY), GetStr(MSG_CLOSE_ALL_WINDOWS) );
            }
        }

        QuickRenderExit();
    }
    return res;
}
///

/// OpenDisplay()
/*
    This routine will open all previously open windows and
    can well be used for startup, also.
*/

PERROR OpenDisplay( VOID )
{
    struct Node *cn, *nn;
    extern struct Window *win_prefs;

    D(bug("OpenDisplay()\n"));

    if( MAINSCR == NULL ) {
        if( NULL == (OpenMainScreen( globals->maindisp )))
            return PERR_FAILED;
    }

    /* Open frames window */

    if( framew.win && framew.Win ) {
        SetAttrs( framew.Win, WINDOW_Screen, MAINSCR, TAG_DONE );
        framew.win = WindowOpen( framew.Win );
    }

    /* The prefs window...BUG: may not have been open... */

    if( prefsw.win && prefsw.Win ) {
        SetAttrs( prefsw.Win, WINDOW_Screen, MAINSCR, TAG_DONE );
        prefsw.win = WindowOpen( prefsw.Win );
    }

    if( extf.win && extf.Win ) {
        SetAttrs( extf.List, LISTV_ListFont, globals->userprefs->listfont, TAG_DONE );
        SetAttrs( extf.Win, WINDOW_Screen, MAINSCR, TAG_DONE );
        extf.win = WindowOpen( extf.Win );
    }

    if( extl.win && extl.Win ) {
        SetAttrs( extl.List, LISTV_ListFont, globals->userprefs->listfont, TAG_DONE );
        SetAttrs( extl.Win, WINDOW_Screen, MAINSCR, TAG_DONE );
        extl.win = WindowOpen( extl.Win );
    }

    if( exts.win && exts.Win ) {
        SetAttrs( exts.List, LISTV_ListFont, globals->userprefs->listfont, TAG_DONE );
        SetAttrs( exts.Win, WINDOW_Screen, MAINSCR, TAG_DONE );
        exts.win = WindowOpen( exts.Win );
    }

    if( toolw.win && toolw.Win ) {
        RebuildToolbarWindow();
    }

    if( selectw.win && selectw.Win ) {
        SetAttrs( selectw.Win, WINDOW_Screen, MAINSCR, TAG_DONE );
        selectw.win = WindowOpen( selectw.Win );
    }

    /* Open all closed displays */

    cn = globals->frames.lh_Head;
    while( nn = cn->ln_Succ ) {
        FRAME *f;
        f = (FRAME*)cn;

        if( f->disp->win ) {
            SetAttrs( f->disp->Win, WINDOW_Screen, MAINSCR, TAG_DONE );
            f->disp->win = WindowOpen( f->disp->Win );
            DisplayFrame( (FRAME *)cn );
        }

        if( f->editwin ) {
            if( f->editwin->win ) {
                SetAttrs( f->editwin->Win, WINDOW_Screen, MAINSCR, TAG_DONE );
                f->editwin->win = WindowOpen( f->editwin->Win );
            }
        }

        if( f->dpw ) {
            if( f->dpw->win ) {
                SetAttrs( f->dpw->WO_Win, WINDOW_Screen, MAINSCR, TAG_DONE );
                f->dpw->win = WindowOpen( f->dpw->WO_Win );
            }
        }

        /* BUG: Palettewindow is also needed */

        cn = nn;
    }

    /* Set main screens for requesters */

    if( gvLoadFileReq.Req )    SetAttrs( gvLoadFileReq.Req, ASLFR_Screen, MAINSCR, TAG_DONE );
    if( gvPaletteOpenReq.Req ) SetAttrs( gvPaletteOpenReq.Req, ASLFR_Screen, MAINSCR, TAG_DONE );
    if( gvPaletteSaveReq.Req ) SetAttrs( gvPaletteSaveReq.Req, ASLFR_Screen, MAINSCR, TAG_DONE );

    return PERR_OK;
}
///
/// CloseDisplay()

/*
    Opposite of OpenDisplay(). Note! This just closes the windows; it does
    not clear the pointers!

    BUG: Does not work 100%
*/

PERROR CloseDisplay()
{
    struct Node *cn, *nn;

    D(bug("CloseDisplay()\n"));

    if( HowManyThreads() != 0 ) {
        Req(NEGNUL,NULL,GetStr(MSG_STILL_PROCESSES_LEFT) );
        return PERR_FAILED;
    }

    /* Close all open frame displays */

    cn = globals->frames.lh_Head;
    while( nn = cn->ln_Succ ) {
        FRAME *f;
        f = (FRAME*)cn;
        if( f->disp->win ) {
            WindowClose( f->disp->Win );
            // UndisplayFrame( (FRAME *)cn );
        }
        if( f->editwin ) {
            if( f->editwin->win ) {
                WindowClose( f->editwin->Win );
            }
        }
        if( f->dpw ) {
            if(f->dpw->win ) {
                WindowClose( f->dpw->WO_Win );
            }
        }

        cn = nn;
    }


    if( extf.Win )
        WindowClose( extf.Win );
    if( extl.Win )
        WindowClose( extl.Win );
    if( exts.Win )
        WindowClose( exts.Win );

    /* Prefs window BUG: what if not open? */

    if( prefsw.Win )
        WindowClose( prefsw.Win );

    if( toolw.Win )
        WindowClose( toolw.Win );

    if( framew.Win )
        WindowClose( framew.Win );

    if( selectw.Win )
        WindowClose( selectw.Win );

    if( CloseMainScreen() != PERR_OK ) {
        Req(NEGNUL,GetStr(MSG_RETRY),GetStr(MSG_CLOSE_ALL_WINDOWS) );
        OpenDisplay();
        return PERR_FAILED;
    }

    return PERR_OK;
}
///

/// StripIntuiMessages&CloseWindowSafely
/*
    These two have been robbed straight from the RKM's and
    are included here because we use a GadTools interface
    in the display windows. Probably should not be here.
*/

Local
void StripIntuiMessages( struct MsgPort *mp, struct Window *win )
{
    struct IntuiMessage *msg;
    struct Node *succ;
    APTR SysBase = SYSBASE();

    msg = (struct IntuiMessage *)mp->mp_MsgList.lh_Head;
    while(succ = msg->ExecMessage.mn_Node.ln_Succ) {
        if(msg->IDCMPWindow = win) {
            Remove( (struct Node *)msg );
            ReplyMsg( (struct Message *)msg );
        }
        msg = (struct IntuiMessage *)succ;
    }
}

VOID CloseWindowSafely( struct Window *win )
{
    APTR SysBase = SYSBASE(), IntuitionBase;

    IntuitionBase = OpenLibrary("intuition.library",36L);
    if(IntuitionBase) {

        Forbid();
        StripIntuiMessages( win->UserPort, win );
        win->UserPort = NULL;
        ModifyIDCMP( win, 0L );
        Permit();
        CloseWindow( win );

        CloseLibrary(IntuitionBase);
    }
}
///

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

