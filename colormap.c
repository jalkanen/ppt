/*
    PROJECT: ppt

    $Id: colormap.c,v 1.4 1995/09/23 21:59:51 jj Exp $

    Contains routines to seek a given color in the current
    colorspace. Following colorspaces are implemented:

        * Colormapped (upto 256 colors)

*/

#include <defs.h>
#include <render.h>

Prototype __D0 UWORD GetColor_Normal( __A0 struct RenderObject *, __D0 UBYTE , __D1 UBYTE , __D2 UBYTE );
Prototype __D0 UWORD GetColor_NormalGray( __A0 struct RenderObject *, __D0 UBYTE, __D1 UBYTE, __D2 UBYTE ) ;
Prototype __D0 UWORD GetColor_HAM( __A0 struct RenderObject *, __D0 UBYTE , __D1 UBYTE , __D2 UBYTE );
Prototype __D0 UWORD GetColor_HAM8( __A0 struct RenderObject *, __D0 UBYTE , __D1 UBYTE , __D2 UBYTE  );

__D0 UWORD GetColor_HAM( __A0 struct RenderObject *rdo, __D0 UBYTE r, __D1 UBYTE g, __D2 UBYTE b )
{ return 0; }

__D0 UWORD GetColor_HAM8( __A0 struct RenderObject *rdo, __D0 UBYTE r, __D1 UBYTE g, __D2 UBYTE b )
{ return 0; }


/*
    Returns the new color index & the new color values.
*/

__D0 UWORD GetColor_Normal( __A0 struct RenderObject *rdo, __D0 UBYTE r, __D1 UBYTE g, __D2 UBYTE b )
{
    HGRAM *hgrams = rdo->histograms;
    ULONG haddr;
    UWORD color;
    UBYTE *colormap = rdo->colortable;

    haddr = HADDR( r >> (8-HGRAM_BITS_RED),
                   g >> (8-HGRAM_BITS_GREEN),
                   b >> (8-HGRAM_BITS_BLUE) ) ;

    color = hgrams[ haddr ];

    if(color == 0) {
        color = 1 + BestMatchPen8( colormap, rdo->ncolors, r, g, b );
        hgrams[ haddr ] = color;
    }

    --color;

    { register ULONG color3;
        color3 = color * 3;

        rdo->newr = colormap[color3++];
        rdo->newg = colormap[color3++];
        rdo->newb = colormap[color3];
    }
    return color;
}

__D0 UWORD GetColor_NormalGray( __A0 struct RenderObject *rdo, __D0 UBYTE r, __D1 UBYTE g, __D2 UBYTE b )
{
    UBYTE *colormap   = rdo->colortable;
    HGRAM *histograms = rdo->histograms;
    UWORD color;

    color = histograms[r];

    if( color == 0 ) {
        color = 1 + BestMatchPen8( colormap, rdo->ncolors, r,r,r );
        histograms[r] = color;
    }

    --color;

    { register ULONG color3;
        color3 = color * 3;

        rdo->newr = colormap[color3++];
        rdo->newg = colormap[color3++];
        rdo->newb = colormap[color3];
    }

    return color;
}
