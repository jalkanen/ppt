/*
    PROJECT: ppt

    $Id: colormap.c,v 1.2 1995/09/14 22:40:46 jj Exp $

    Contains routines to seek a given color in the current
    colorspace. Following colorspaces are implemented:

        * Colormapped (upto 256 colors)

*/

#include <defs.h>
#include <render.h>

Prototype UWORD GetColor_Normal( struct RenderObject *, UBYTE , UBYTE , UBYTE );
Prototype UWORD GetColor_HAM( struct RenderObject *, UBYTE , UBYTE , UBYTE );
Prototype UWORD GetColor_HAM8( struct RenderObject *, UBYTE , UBYTE , UBYTE  );

UWORD GetColor_HAM( struct RenderObject *rdo, UBYTE r, UBYTE g, UBYTE b )
{ return 0; }

UWORD GetColor_HAM8( struct RenderObject *rdo, UBYTE r, UBYTE g, UBYTE b )
{ return 0; }


/*
    Returns the new color index & the new color values.
*/

UWORD GetColor_Normal( struct RenderObject *rdo, UBYTE r, UBYTE g, UBYTE b )
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

