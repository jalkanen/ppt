/*
    PROJECT: ppt

    $Id: colormap.c,v 1.7 1996/05/13 00:20:22 jj Exp $

    Contains routines to seek a given color in the current
    colorspace. Following colorspaces are implemented:

        * Colormapped (upto 256 colors)
        * HAM8
*/

/*--------------------------------------------------------------------*/
/* Includes */

#include <defs.h>
#include <render.h>

/*--------------------------------------------------------------------*/
/* Prototypes */

Prototype REG(d0) UWORD GetColor_Normal( REG(a0) struct RenderObject *, REG(d0) UBYTE , REG(d1) UBYTE , REG(d2) UBYTE );
Prototype REG(d0) UWORD GetColor_NormalGray( REG(a0) struct RenderObject *, REG(d0) UBYTE, REG(d1) UBYTE, REG(d2) UBYTE ) ;
Prototype REG(d0) UWORD GetColor_HAM( REG(a0) struct RenderObject *, REG(d0) UBYTE , REG(d1) UBYTE , REG(d2) UBYTE );
Prototype REG(d0) UWORD GetColor_HAM8( REG(a0) struct RenderObject *, REG(d0) UBYTE , REG(d1) UBYTE , REG(d2) UBYTE  );

/*--------------------------------------------------------------------*/
/* Defines */

#define RED 0
#define GREEN 1
#define BLUE 2
#define PEN 3
#define ham_color_type int

#undef HAM_DEBUG

/*--------------------------------------------------------------------*/
/* Code */

/*
    Uses HAM8 but in 16 colors to create an approximation, then
    converts into HAM6 form.
*/
REG(d0) UWORD GetColor_HAM( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
{
    UWORD pen;

    pen = GetColor_HAM8( rdo, r,g,b );

    if( pen <= 16 )
        return pen;

    return (pen >> 2);
}

/*
    Return the value to be loaded in the bitmap.

    00 = CMap entry # (0-63)
    01 = Change Blue
    10 = Change Red
    11 = Change Green

*/
REG(d0) UWORD GetColor_HAM8( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
{
    UWORD oldr,oldg,oldb;
    UWORD penr,peng,penb;
    UWORD pen;
    ham_color_type best;

    /*
     *  Get the old colormap entry
     */

    oldr = (UWORD)rdo->newr;
    oldg = (UWORD)rdo->newg;
    oldb = (UWORD)rdo->newb;

    // D(bug("Current pixel: R=%d, G=%d, B=%d\n",r,g,b));
    // D(bug("Last pixel: R=%d, G=%d, B=%d\n",oldr,oldg,oldb));

    /*
     *  Get the best choice from the colormap by
     *  utilizing our own routines.
     */

    pen  = GetColor_Normal( rdo, r, g, b );
    penr = (UWORD)rdo->newr;
    peng = (UWORD)rdo->newg;
    penb = (UWORD)rdo->newb;

    /*
     *  Now, let's calculate the best distances these systems give.
     *  We can assume we can always hit one correct color component
     *  with the HAM mode.
     */

    {
        LONG bestred, bestblue, bestgreen, bestpen;

        {
            /*
             *  Calculate the differences by colorchanging
             */

            {
                WORD gdiff, rdiff, bdiff;

                gdiff = oldg - (WORD)g;
                bdiff = oldb - (WORD)b;
                rdiff = oldr - (WORD)r;

                bestred   = MULS16(gdiff,gdiff) + MULS16(bdiff,bdiff);
                bestgreen = MULS16(rdiff,rdiff) + MULS16(bdiff,bdiff);
                bestblue  = MULS16(gdiff,gdiff) + MULS16(rdiff,rdiff);
            }

            /*
             *  Calculate differences by bestpenmapping
             */

            {
                WORD pengdiff, penrdiff, penbdiff;
                penrdiff = penr-(WORD)r;
                pengdiff = peng-(WORD)g;
                penbdiff = penb-(WORD)b;

                bestpen   = MULS16(penrdiff,penrdiff) +
                            MULS16(pengdiff,pengdiff) +
                            MULS16(penbdiff,penbdiff);
            }
        }

        // D(bug("\tCompare: R=%u, G=%u, B=%u, PEN=%u\n",bestred,bestgreen,bestblue,bestpen));

        /*
         *  Get the best possibility
         */

        {
            ham_color_type best1, best2;
            LONG best1val, best2val, bestval;

            if( bestred < bestgreen ) {
                best1 = RED;
                best1val = bestred;
            } else {
                best1 = GREEN;
                best1val = bestgreen;
            }

            if( bestblue < bestpen ) {
                best2 = BLUE;
                best2val = bestblue;
            } else {
                best2 = PEN;
                best2val = bestpen;
            }

            if( best1val < best2val ) {
                bestval = best1val;
                best = best1;
            } else {
                bestval = best2val;
                best = best2;
            }
        }
    }

    /*
     *  Then determine the real values to be loaded. If the current column
     *  is 0, then we will always choose the best PEN.
     */

    if( rdo->currentcolumn != 0 ) {

        switch( best ) {
            case PEN:
                // D(bug("\tPEN\n"));
                break;

            case RED:
                // D(bug("\tRED\n"));
                pen  = 0x80 | (r >> 2);
                penr = r;
                peng = oldg;
                penb = oldb;
                break;

            case GREEN:
                // D(bug("\tGREEN\n"));
                pen  = 0xC0 | (g >> 2);
                penr = oldr;
                peng = g;
                penb = oldb;
                break;

            case BLUE:
                // D(bug("\tBLUE\n"));
                pen  = 0x40 | (b >> 2);
                penr = oldr;
                peng = oldg;
                penb = b;
                break;

        }
    }

    /*
     *  Prepare for exit and load cache and old colormap values.
     */

    rdo->newr = (UBYTE)penr;
    rdo->newg = (UBYTE)peng;
    rdo->newb = (UBYTE)penb;

    // D(bug("\tNew effective value: R=%d, G=%d, B=%d, PEN=0x%02X\n",penr,peng,penb,pen));

    return pen;
}


/*
    Returns the new color index & the new color values.
*/

REG(d0) UWORD GetColor_Normal( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
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

REG(d0) UWORD GetColor_NormalGray( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
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

/*--------------------------------------------------------------------*/
/*                            END OF CODE                             */
/*--------------------------------------------------------------------*/
