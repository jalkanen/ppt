/*
    PROJECT: ppt

    $Id: colormap.c,v 1.12 1999/03/17 23:05:29 jj Exp $

    Contains routines to seek a given color in the current
    colorspace. Following colorspaces are implemented:

        * Colormapped (upto 256 colors)
        * HAM
        * HAM8
*/

/*--------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "render.h"

/*--------------------------------------------------------------------*/
/* Prototypes */

Prototype UWORD ASM GetColor_Normal( REGDECL(a0,struct RenderObject *), REGDECL(d0,UBYTE), REGDECL(d1,UBYTE) , REGDECL(d2,UBYTE) );
Prototype UWORD ASM GetColor_NormalGray( REGDECL(a0,struct RenderObject *), REGDECL(d0,UBYTE), REGDECL(d1,UBYTE), REGDECL(d2,UBYTE) ) ;
Prototype UWORD ASM GetColor_HAM( REGDECL(a0,struct RenderObject *), REGDECL(d0,UBYTE), REGDECL(d1,UBYTE), REGDECL(d2,UBYTE) );
Prototype UWORD ASM GetColor_HAM8( REGDECL(a0, struct RenderObject *), REGDECL(d0,UBYTE), REGDECL(d1,UBYTE), REGDECL(d2,UBYTE) );
Prototype UWORD ASM GetColor_EHB( REGDECL(a0,struct RenderObject *), REGDECL(d0,UBYTE), REGDECL(d1,UBYTE), REGDECL(d2,UBYTE) );

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
    Return an EHB color.  Assume the first 32 colors of the colormap
    are OK and the rest are just halved.
*/

ASM UWORD GetColor_EHB( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
{
    UWORD pen;
    int i;

    if( rdo->EHB_Data == 0 ) {
        COLORMAP *colortable = rdo->colortable;

        for( i = 0; i < rdo->ncolors; i++ ) {
            colortable[i+rdo->ncolors].r = colortable[i].r >> 1;
            colortable[i+rdo->ncolors].g = colortable[i].g >> 1;
            colortable[i+rdo->ncolors].b = colortable[i].b >> 1;
        }
        rdo->ncolors *= 2;
        rdo->EHB_Data = 1;
        D(DoShowColorTable( rdo->ncolors, colortable ));
    }

    pen = GetColor_Normal( rdo, r, g, b );

    return pen;
}

/*
    Uses HAM8 but in 16 colors to create an approximation, then
    converts into HAM6 form.
*/
ASM UWORD GetColor_HAM( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
{
    UWORD pen;

    pen = GetColor_HAM8( rdo, r,g,b );

    if( pen <= 16 )
        return pen;

    return (UWORD)(pen >> 2);
}

/*
    Return the value to be loaded in the bitmap.

    00 = CMap entry # (0-63)
    01 = Change Blue
    10 = Change Red
    11 = Change Green

*/
ASM UWORD GetColor_HAM8( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
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

    Uses rdo->histograms to cache pen values.
*/

ASM UWORD GetColor_Normal( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
{
    HGRAM *hgrams = rdo->histograms;
    ULONG haddr;
    UWORD color;
    COLORMAP *colormap = rdo->colortable;

    haddr = HADDR( r >> (8-HGRAM_BITS_RED),
                   g >> (8-HGRAM_BITS_GREEN),
                   b >> (8-HGRAM_BITS_BLUE) ) ;

    color = hgrams[ haddr ];

    if(color == 0) {
        color = 1 + BestMatchPen8( colormap, rdo->ncolors, r, g, b );
        hgrams[ haddr ] = color;
    }

    --color;

    rdo->newr = colormap[color].r;
    rdo->newg = colormap[color].g;
    rdo->newb = colormap[color].b;

    return color;
}

ASM UWORD GetColor_NormalGray( REG(a0) struct RenderObject *rdo, REG(d0) UBYTE r, REG(d1) UBYTE g, REG(d2) UBYTE b )
{
    COLORMAP *colormap = rdo->colortable;
    HGRAM *histograms = rdo->histograms;
    UWORD color;

    color = histograms[r];

    if( color == 0 ) {
        color = 1 + BestMatchPen8( colormap, rdo->ncolors, r,r,r );
        histograms[r] = color;
    }

    --color;

    rdo->newr = colormap[color].r;
    rdo->newg = colormap[color].g;
    rdo->newb = colormap[color].b;

    return color;
}

/*--------------------------------------------------------------------*/
/*                            END OF CODE                             */
/*--------------------------------------------------------------------*/
