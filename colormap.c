/*
    Routines to map the color to whatever is required

    $Id: colormap.c,v 1.1 1995/09/06 23:35:32 jj Exp $
*/

#include <render.h>

Prototype UWORD GetColor_Normal( struct RenderObject *, UBYTE *, UBYTE *, UBYTE * );
Prototype UWORD GetColor_HAM( struct RenderObject *, UBYTE *, UBYTE *, UBYTE * );
Prototype UWORD GetColor_HAM8( struct RenderObject *, UBYTE *, UBYTE *, UBYTE * );

UWORD GetColor_HAM( struct RenderObject *rdo, UBYTE *r, UBYTE *g, UBYTE *b )
{ return 0; }

UWORD GetColor_HAM8( struct RenderObject *rdo, UBYTE *r, UBYTE *g, UBYTE *b )
{ return 0; }


/*
    Returns the new color index & the new color values.
*/

UWORD GetColor_Normal( struct RenderObject *rdo, UBYTE *r, UBYTE *g, UBYTE *b )
{
    HGRAM *hgrams = rdo->histograms;
    ULONG haddr;
    UWORD color;
    UBYTE *colormap = rdo->colortable;

    haddr = HADDR( *r >> (8-HGRAM_BITS_RED),
                   *g >> (8-HGRAM_BITS_GREEN),
                   *b >> (8-HGRAM_BITS_BLUE) ) ;

    color = (UWORD)hgrams[ haddr ];

    if(color == 0) {
        color = 1 + BestMatchPen8( colormap, rdo->ncolors, *r, *g, *b );
        hgrams[ haddr ] = (UBYTE)color;
    }

    { register ULONG color3;
        color3 = (--color) * 3;

        *r = colormap[color3++];
        *g = colormap[color3++];
        *b = colormap[color3];
    }
    return color;
}
