/*
    PROJECT: ppt
    MODULE : palette.c

    $Id: palette.c,v 1.13 1999/10/02 16:33:07 jj Exp $

    Palette selection routines.
*/

#include "defs.h"
#include "misc.h"
#include "render.h"

#include <math.h>

Prototype PERROR Palette_PopularityI( struct RenderObject * );
Prototype PERROR Palette_MedianCutI( struct RenderObject * );
Prototype PERROR Palette_ForceI( struct RenderObject * );
Prototype PERROR Palette_HAMI( struct RenderObject * );
Prototype PERROR AllocHistograms( struct RenderObject * );
Prototype VOID   ReleaseHistograms( struct RenderObject * );

/*
    COMMON ROUTINES
*/


PERROR AllocHistograms( struct RenderObject *rdo )
{
    HGRAM *hg;
    PERROR res = PERR_OK;

    D(bug("AllocHistograms(size=%lu)\n",HGRAM_SIZE));

    hg = pmalloc( HGRAM_SIZE );
    if(!hg) {
        D(bug("\tUnable to allocate histogram space!\n"));
        res = PERR_OUTOFMEMORY;
    } else
        bzero( hg, HGRAM_SIZE );

    rdo->histograms = hg;
    rdo->hsize = HGRAM_SIZE;

    return res;
}


VOID ReleaseHistograms( struct RenderObject *rdo )
{
    D(bug("ReleaseHistograms()\n"));
    if( rdo->histograms ) {
        pfree(rdo->histograms);
        rdo->histograms = NULL;
    }
}

/*
    This will use a 'diminished distance' algorithm to generate
    a HAM image colormap.
*/

#define SQR(a) ((a)*(a))

Local
PERROR HAM_Histograms( struct RenderObject *rdo )
{
    PERROR res;
    WORD row, col;
    FRAME *frame = rdo->frame;
    int cspace = frame->pix->colorspace;
    EXTBASE *PPTBase = rdo->PPTBase;

    D(bug("HAM_Histograms()\n"));

    if( PERR_OK == (res = AllocHistograms(rdo))) {
        HGRAM *buffer;

        buffer = rdo->histograms;

        InitProgress( frame, XGetStr(MSG_BUILDING_HISTOGRAMS), 0, frame->pix->height, PPTBase );

        for( row = 0; row < frame->pix->height; row++ ) {
            int r,g,b;
            ROWPTR cp, dcp;

            if( Progress( frame, row, PPTBase ) ) {
                res = PERR_BREAK;
                break;
            }

            dcp = cp = GetPixelRow( frame, row, PPTBase );

            r = 0; g = 0; b = 0; /* BUG: Not so, should use background? */

            for( col = 0; col < frame->pix->width; col++ ) {
                long dist;
                int newr, newg, newb;
                color_type shortest;

                switch(cspace) {
                  case CS_RGB:
                  case CS_ARGB:

                    if( cspace == CS_ARGB ) dcp++; /* Skip alpha */
                    newr = *dcp++;
                    newg = *dcp++;
                    newb = *dcp++;

                    if( r-newr < g-newg && r-newr < b-newb ) {
                        dist = SQR(g-newg) + SQR(b-newb);
                        shortest = RED;
                    } else {
                        if( g-newg < b-newb ) {
                            dist = SQR(r-newr) + SQR(b-newb);
                            shortest = GREEN;
                        } else {
                            dist = SQR(r-newr) + SQR(g-newg);
                            shortest = BLUE;
                        }
                    }

                    dist = (long) sqrt( (double)dist );
                    break;

                  case CS_GRAYLEVEL:
                    newr = newg = newb = *dcp++;
                    dist = abs( r - newr );
                }

                r = newr; g = newg; b = newb;

                newr = (newr) >> (8-HGRAM_BITS_RED);
                newg = (newg) >> (8-HGRAM_BITS_GREEN);
                newb = (newb) >> (8-HGRAM_BITS_BLUE);
                buffer[HADDR(newr,newg,newb)] += dist;
            }
        }

        if( res == PERR_OK ) FinishProgress( frame, PPTBase );

    }

    return res;
}

/*
    Extra-HalfBrite is a curious mode, in which every pixel
    entry is duplicated but at half intensity at
    color_index + 32, thus resulting into 64 colors.

    (Does AGA support EHB in more than 6 bpls?)
*/

Local
PERROR EHB_Histograms( struct RenderObject *rdo )
{
    PERROR res;
    WORD row, col;
    FRAME *frame = rdo->frame;
    int cspace = frame->pix->colorspace;
    EXTBASE *PPTBase = rdo->PPTBase;

    D(bug("EHB_Histograms()\n"));

    if( PERR_OK == (res = AllocHistograms(rdo))) {
        HGRAM *buffer;

        buffer = rdo->histograms;

        InitProgress( frame, XGetStr(MSG_BUILDING_HISTOGRAMS), 0, frame->pix->height, PPTBase );

        for( row = 0; row < frame->pix->height; row++ ) {
            int r,g,b;
            ROWPTR cp, dcp;

            if( Progress( frame, row, PPTBase ) ) {
                res = PERR_BREAK;
                break;
            }

            dcp = cp = GetPixelRow( frame, row, PPTBase );

            for( col = 0; col < frame->pix->width; col++ ) {
                long dist;

                if( cspace != CS_GRAYLEVEL ) {

                    if( cspace == CS_ARGB ) dcp++; /* Skip Alpha */
                    r = *dcp++;
                    g = *dcp++;
                    b = *dcp++;

                    if( r >= 0x80 || g >= 0x80 || b >= 0x80 )
                        dist = 1;
                    else
                        dist = 1;

                } else {
                    r = g = b = *dcp++;
                    if (r >= 0x80)
                        dist = 1;
                    else
                        dist = 1;
                }

                r = (r) >> (8-HGRAM_BITS_RED);
                g = (g) >> (8-HGRAM_BITS_GREEN);
                b = (b) >> (8-HGRAM_BITS_BLUE);
                buffer[HADDR(r,g,b)] += dist;
            }
        }

        if( res == PERR_OK ) FinishProgress( frame, PPTBase );

    }

    return res;
}

Local
PERROR BuildSimpleHistograms( struct RenderObject *rdo, color_type which )
{
    FRAME *source = rdo->frame;
    HGRAM *buffer;
    WORD row,comps = source->pix->components;
    EXTBASE *PPTBase = rdo->PPTBase;
    PERROR res = PERR_OK;

    D(bug("Build simple histograms for color %d",which));

    InitProgress( source, XGetStr(MSG_BUILDING_HISTOGRAMS),0,source->pix->height, PPTBase );
    buffer = pzmalloc( 256 * sizeof(HGRAM) );
    if(!buffer) {
        D(bug("\tUnable to allocate histogram space!\n"));
        return PERR_OUTOFMEMORY;
    }

    for( row = 0; row < source->pix->height; row++) {
        ROWPTR buf;
        WORD col;
        if(Progress(source,row,PPTBase)) {
            res = PERR_BREAK;
            goto errorexit;
        }

        buf = GetPixelRow( source, row, PPTBase );
        for(col = 0; col < source->pix->width; col++) {
            UBYTE val;
            val = buf[MULU16(col,comps) + which];
            if(++buffer[val] == 0)
                buffer[val] = (HGRAM)~0;
        }
    }
    FinishProgress(source,PPTBase);

errorexit:

    if( res != PERR_OK ) {
        pfree( buffer );
        rdo->histograms = NULL;
        rdo->hsize = 0;
    } else {
        rdo->histograms = buffer;
        rdo->hsize      = 256 * sizeof(HGRAM);
    }

    return res;
}


PERROR MakeHistograms( struct RenderObject *rdo )
{
    HGRAM *buffer;
    HGRAM cc;
    UBYTE *buf;
    UWORD row, col;
    FRAME *frame = rdo->frame;
    PERROR res = PERR_OK;
    EXTBASE *PPTBase = rdo->PPTBase;

    InitProgress( frame, XGetStr(MSG_BUILDING_HISTOGRAMS),0,frame->pix->height, PPTBase );

    if( frame->disp->renderq == RENDER_HAM6 ||
        frame->disp->renderq == RENDER_HAM8 )
    {
        return HAM_Histograms( rdo );
    }

    if( frame->disp->renderq == RENDER_EHB ) return EHB_Histograms( rdo );

    if(frame->pix->colorspace != CS_GRAYLEVEL) {

        D(bug("Build histograms...\n"));

        if( (res = AllocHistograms(rdo)) != PERR_OK) {
            Req( GetFrameWin(frame), NULL, "\nUnable to allocate %lu bytes for histograms!\n",HGRAM_SIZE);
            return res;
        }

        buffer = rdo->histograms;

        for(row = 0; row < frame->pix->height; row++) {

            if(Progress(frame,row,PPTBase)) {
                res = PERR_BREAK;
                goto errorexit;
            }

            buf = GetPixelRow( frame, row, PPTBase );
            cc = 0;
            for( col = 0; col < frame->pix->width; col++) {
                UBYTE r,g,b;

                if( frame->pix->colorspace == CS_ARGB ) cc++; /* Skip alpha */

                r = (buf[cc++]) >> (8-HGRAM_BITS_RED);
                g = (buf[cc++]) >> (8-HGRAM_BITS_GREEN);
                b = (buf[cc++]) >> (8-HGRAM_BITS_BLUE);
                if( ++buffer[ HADDR(r,g,b) ] == 0) /* Check for overflow */
                    buffer[HADDR(r,g,b)] = (HGRAM)~0;
            }
        }
        FinishProgress( frame, PPTBase );

    } else { /* GRAY */
        res = BuildSimpleHistograms(rdo, GRAY);
    }

errorexit:

    if( res != PERR_OK ) {
        ReleaseHistograms( rdo );
    }

    return res;
}



/*
    POPULARITY ALGORITHM
*/


PERROR Palette_PopularityD( struct RenderObject *rdo )
{
    ReleaseHistograms(rdo);

    return PERR_OK;
}

/*
    BUG: This could actually be fastened somewhat, I think. It should
         not be necessary to go through the histograms so many times.
*/
PERROR Palette_Popularity( struct RenderObject *rdo )
{
    int cc;
    FRAME *f = rdo->frame;
    ULONG nColors = rdo->ncolors;
    EXTBASE *PPTBase = rdo->PPTBase;
    COLORMAP *colortable = rdo->colortable;
    HGRAM *histograms;
    PERROR res = PERR_OK;

    D(bug("BuildColorTable(%lu colors, ct=%x )\n",nColors, colortable ));

    /*
     *  Make histograms and return on error
     */

    res = MakeHistograms( rdo );

    if(res != PERR_OK)
        return res;

    InitProgress(f, XGetStr( MSG_SELECTING_PALETTE ), 0, nColors, PPTBase );

    histograms = rdo->histograms;

    for( cc = 0; cc < nColors; cc++) {
        UBYTE r,br,bg,bb;
        HGRAM max;

        if(Progress(f,cc,PPTBase)) {
            res = PERR_BREAK;
            goto errorexit;
        }

        max = 0;
        for( r = 0; r < HGRAM_RED; r++ ) {
            UBYTE g;

            for(g = 0; g < HGRAM_GREEN; g++ ) {
                UBYTE b;

                for(b = 0; b < HGRAM_BLUE; b++ ) {
                    HGRAM val;

                    val = histograms[ HADDR(r,g,b) ];
                    if( val > max ) { /* Should check if a close entry already exists. */
                        max = val;
                        br = r; bg = g; bb = b;
                    }
                }
            }
        }
        histograms[ HADDR(br,bg,bb) ] = 0; /* To make sure it won't pop up again. */
        /* Make a LoadRGB8() compatible table. */
        colortable[cc].r = RED2RGB8(br);
        colortable[cc].g = GREEN2RGB8(bg);
        colortable[cc].b = BLUE2RGB8(bb);
    }

    FinishProgress(f,PPTBase);

    // DoShowColorTable( nColors, colortable );

errorexit:
    return res;
}

Local PERROR
Palette_GrayPopularity( struct RenderObject *rdo )
{
    FRAME *frame = rdo->frame;
    ULONG nColors = rdo->ncolors;
    EXTBASE *PPTBase = rdo->PPTBase;
    HGRAM *histograms;
    WORD cc,g,bg;
    PERROR res = PERR_OK;
    COLORMAP *colortable = rdo->colortable;

    D(bug("Selecting gray palette\n"));

    res = MakeHistograms( rdo );

    if(res != PERR_OK)
        return res;

    histograms = rdo->histograms;

    if(!CheckPtr(histograms,"histograms"))
        return PERR_FAILED;

    if(!CheckPtr(rdo->colortable,"colortable"))
        return PERR_FAILED;


    InitProgress(frame, XGetStr(MSG_SELECTING_PALETTE), 0, nColors, PPTBase );

    for(cc = 0; cc < nColors; cc++) {
        HGRAM max;

        if(Progress( frame,cc,PPTBase )) {
            res = PERR_BREAK;
            break;
        }

        max = 0;
        for( g = 0; g < 256; g++ ) {
            if(histograms[g] > max) {
                max = histograms[g];
                bg = g;
            }
        }
        histograms[bg] = 0;
        colortable[cc].r = colortable[cc].g = colortable[cc].b = (UBYTE)bg;
    }
    return res;
}


PERROR Palette_PopularityI( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;

    switch( rdo->frame->pix->colorspace ) {

        case CS_RGB:
        case CS_ARGB:
            rdo->ncolors = rdo->frame->disp->ncolors;
            rdo->Palette  = Palette_Popularity;
            rdo->PaletteD = Palette_PopularityD;
            break;

        case CS_GRAYLEVEL:
            rdo->ncolors = rdo->frame->disp->ncolors;
            rdo->Palette  = Palette_GrayPopularity;
            rdo->PaletteD = Palette_PopularityD;
            break;

        default:
            InternalError( "Unknown colorspace detected!" );
            res = PERR_INITFAILED;
            break;
    }

    return res;
}

/*-------------------------------------------------------------------------*/
/*

    MEDIAN CUT BEGINS HERE

*/


/* Keep divisible by four */
struct median_box {
    UWORD s[3]; /* Box start co-ordinate near zero */
    UWORD d[3]; /* Box size */
    ULONG hits;
};


PERROR Palette_MedianCutD( struct RenderObject *rdo )
{
    D(bug("Palette_MedianCutD()\n"));

    ReleaseHistograms(rdo);

    return PERR_OK;
}
/*
    Select new current box by searching for the one with most
    values. However, we won't select boxes that are just one pixel wide or those
    that are empty
*/

Local
WORD MC_GetNewBox( int nboxes, HGRAM *histograms, struct median_box *boxes )
{
    ULONG tmax;
    WORD tcb,i;
    struct median_box *ptr = boxes;

    tmax = 0;
    for(i = 0; i <= nboxes; i++) {
        if( ptr->hits >= tmax ) {
            if( ptr->d[RED] > 1 || ptr->d[GREEN] > 1 || ptr->d[BLUE] > 1) {
                tcb = i;
                tmax = ptr->hits;
            }
        }
        ptr++;
    }

    if(tmax <= 1)
        tcb = -1;

    return tcb;
}

/*
    This uses the median cut algorithm to select color values.
*/

PERROR Palette_MedianCut( struct RenderObject *rdo )
{
    struct median_box *boxptr, *boxes;
    UWORD colors = 0, nboxes = 0;
    WORD  cb = 0;
    UWORD  i,j,k,n;
    FRAME *f = rdo->frame;
    ULONG nColors = f->disp->ncolors;
    HGRAM *histograms;
    COLORMAP *colortable = rdo->colortable;
    PERROR res = PERR_OK;
    EXTBASE *PPTBase = rdo->PPTBase;

    D(bug("Palette_MedianCut()\n"));

    boxes = pzmalloc( (MAX_BOXES + 1) * sizeof(struct median_box) );
    if(!boxes) return PERR_OUTOFMEMORY;

    boxptr = boxes;

    boxptr->s[RED]   = boxptr->s[GREEN] = boxptr->s[BLUE] = 0;
    boxptr->d[RED]   = HGRAM_RED;
    boxptr->d[GREEN] = HGRAM_GREEN;
    boxptr->d[BLUE]  = HGRAM_BLUE;

    res = MakeHistograms( rdo );

    if(res != PERR_OK) goto errorexit;

    histograms = rdo->histograms;

    InitProgress(f, XGetStr(MSG_SELECTING_PALETTE), 0,nColors, PPTBase );

    while( colors < nColors ) {
        color_type axis;
        ULONG freq[65]; /* MAX 6 bitplanes */
        UWORD *bdx;

        if(Progress( f, colors, PPTBase )) {
            res = PERR_BREAK;
            goto errorexit;
        }

#ifdef NBCT_DEBUG
        DEBUG("\nColor # %d\n",colors);
        DEBUG("Box %d: %d-%d-%d  (%d-%d-%d)\n",cb,boxptr->s[0],boxptr->s[1],boxptr->s[2],
                                                  boxptr->d[0],boxptr->d[1],boxptr->d[2]);
#endif

        /* Find longest axis */
        if(boxptr->d[RED] >= boxptr->d[GREEN] && boxptr->d[RED] >= boxptr->d[BLUE])
                axis = RED;
        else {
            if( boxptr->d[GREEN] >= boxptr->d[BLUE] )
                axis = GREEN;
            else
                axis = BLUE;
        }

        /* Calculate # of colors on each cross-section plane */

        for( i = 0; i < 65; i++) { freq[i] = 0L; }

        if(axis == RED) {
#ifdef NBCT_DEBUG
            DEBUG("Longest is RED\n");
#endif
            for(i = 0; i < boxptr->d[RED]; i++ ) {
                ULONG fz;
                fz = 0;
                for(j = 0; j < boxptr->d[GREEN]; j++ ) {
                    for( k = 0; k < boxptr->d[BLUE]; k++ ) {
                        if(histograms[ HADDR(boxptr->s[RED]+i,boxptr->s[GREEN]+j, boxptr->s[BLUE]+k) ])
                            fz++;
                    }
                }
                freq[i] = fz;
            }
        }
        if(axis == GREEN) {
#ifdef NBCT_DEBUG
            DEBUG("Longest is GREEN\n");
#endif
            for(i = 0; i < boxptr->d[GREEN]; i++ ) {
                ULONG fz;
                fz = 0;
                for(j = 0; j < boxptr->d[RED]; j++ ) {
                    for( k = 0; k < boxptr->d[BLUE]; k++ ) {
                        if(histograms[ HADDR(boxptr->s[RED]+j, boxptr->s[GREEN]+i, boxptr->s[BLUE]+k) ])
                            fz++;
                    }
                }
                freq[i] = fz;
            }
        }
        if(axis == BLUE) {
#ifdef NBCT_DEBUG
            DEBUG("Longest is BLUE\n");
#endif
            for(i = 0; i < boxptr->d[BLUE]; i++ ) {
                ULONG fz;
                fz = 0;
                for(j = 0; j < boxptr->d[GREEN]; j++ ) {
                    for( k = 0; k < boxptr->d[RED]; k++ ) {
                        if(histograms[ HADDR(boxptr->s[RED]+k, boxptr->s[GREEN]+j, boxptr->s[BLUE]+i) ])
                            fz++;
                    }
                }
                freq[i] = fz;
            }
        }

        bdx = &(boxptr->d[axis]);

#ifdef NBCT_DEBUG
        DEBUG("Searching median\n");
#endif

        /* Search median */

        for(i = 0; i < *bdx; i++ ) {
#ifdef NBCT_DEBUG
            DEBUG("freq[%d] = %lu\n",i,freq[i]);
#endif
            freq[i+1] += freq[i];
        }
#ifdef NBCT_DEBUG
        DEBUG("Search target %d (/2 = %d)\n",freq[boxptr->d[axis]], freq[boxptr->d[axis]] >> 1);
#endif

        for(i = 0; freq[i] < (freq[*bdx] >> 1); i++);
        /* i now points at the area, do the split */

        if(i == *bdx - 1) i--;

#ifdef NBCT_DEBUG
        DEBUG("Split at %d\n",i);
#endif

        /* First, make room after this area */

        for(j = nboxes+1; j > cb; j--) {
            boxes[j] = boxes[j-1];
        }

        /* Now split. We won't accept empty boxes or those at the
           border area. */

        colors++;

        (boxptr+1)->s[axis] = boxptr->s[axis] + i + 1;
        (boxptr+1)->d[axis] = *bdx - i - 1;
        boxptr->hits = freq[i];
        (boxptr+1)->hits = freq[*bdx] - freq[i];

        if( freq[i] == freq[*bdx] || freq[i] == 0L) {
            colors--;
        }

        *bdx = i + 1;

#ifdef NBCT_DEBUG
        DEBUG("Box split %d: %d-%d-%d  (%d-%d-%d)%s (%lu)\n",cb,boxptr->s[0],boxptr->s[1],boxptr->s[2],
                                                    boxptr->d[0],boxptr->d[1],boxptr->d[2], boxptr->hits ? "" : " EMPTY",
                                                    boxptr->hits);
        DEBUG("Box split %d: %d-%d-%d  (%d-%d-%d)%s (%lu)\n",cb+1,boxes[cb+1].s[0],boxes[cb+1].s[1],boxes[cb+1].s[2],
                                                    boxes[cb+1].d[0],boxes[cb+1].d[1],boxes[cb+1].d[2], boxes[cb+1].hits ? "" : " EMPTY",
                                                    boxes[cb+1].hits);
#endif

        nboxes++;

        cb = MC_GetNewBox( nboxes, histograms, boxes );
        boxptr = &(boxes[cb]); //  + MULS16(cb,sizeof(struct median_box));

#ifdef NBCT_DEBUG
        DEBUG("New box is %d \n",cb);
#endif

        if(cb < 0 || nboxes >= MAX_BOXES)  /* Run out of colors */
            break;
        /* if(# of colors == nColors) exit split loop */

    } /* while(colors) */

    FinishProgress( f, PPTBase );

#ifdef NBCT_DEBUG
    DEBUG("Median cut ready, now selecting colors\n");
#endif
    /* Find largest value in each box for color */
    cb = 0; /* Current color */
    boxptr = boxes;

    D(bug("Used %d colors out of %d (%d boxes)\n",colors,nColors,nboxes));

    for(i = 0; i <= nboxes && cb < nColors; i++) {
        HGRAM maxval,val;
        maxval = 0;

        if(boxptr->hits) {
#ifdef NBCT_DEBUG
            DEBUG("Box %d: %d-%d-%d  (%d-%d-%d)\n",i,boxes[i].s[0],boxes[i].s[1],boxes[i].s[2],
                boxes[i].d[0],boxes[i].d[1],boxes[i].d[2]);
#endif

            for( j = boxptr->s[RED]; j < boxptr->s[RED]+boxptr->d[RED]; j++ ) {
                for( k = boxptr->s[GREEN]; k < boxptr->s[GREEN]+boxptr->d[GREEN]; k++ ) {
                    for( n = boxptr->s[BLUE]; n < boxptr->s[BLUE]+boxptr->d[BLUE]; n++ ) {
                        val = histograms[ HADDR(j,k,n) ];
                        if( val > maxval ) {
                            colortable[ cb ].r = RED2RGB8(j);
                            colortable[ cb ].g = GREEN2RGB8(k);
                            colortable[ cb ].b = BLUE2RGB8(n);
#ifdef NBCT_DEBUG
                            DEBUG("\tColor %d: set val (%X,%X,%X)\n",cb,RED2RGB8(j),GREEN2RGB8(k),BLUE2RGB8(n));
#endif
                        }
                    }
                }
            }
            cb++; /* Next color */
        }
        boxptr++;
    }
#ifdef NBCT_DEBUG
    DoShowColorTable( nColors, colortable );
#endif

errorexit:

    pfree(boxes);

    return res;
}

Local
WORD MC_GrayGetNewBox( int nboxes, HGRAM *histograms, struct median_box *boxes )
{
    ULONG tmax;
    WORD tcb,i;
    struct median_box *ptr = boxes;

    tmax = 0;
    for(i = 0; i <= nboxes; i++) {
        if( ptr->hits >= tmax ) {
            if( ptr->d[GRAY] > 1 ) {
                tcb = i;
                tmax = ptr->hits;
            }
        }
        ptr++;
    }

    if(tmax <= 1)
        tcb = -1;

    return tcb;
}

/*
    The graylevel median cut.
    BUG: Should use something else than the median_box
    BUG: This could be optimized more.
*/
Local
PERROR Palette_GrayMedianCut( struct RenderObject *rdo )
{
    struct median_box *boxptr, *boxes;
    UWORD colors = 0, nboxes = 0;
    WORD  cb = 0;
    UWORD  i,j;
    FRAME *f = rdo->frame;
    ULONG nColors = f->disp->ncolors;
    HGRAM *histograms;
    COLORMAP *colortable = rdo->colortable;
    PERROR res = PERR_OK;
    EXTBASE *PPTBase = rdo->PPTBase;

    D(bug("Palette_MedianCut()\n"));

    boxes = pzmalloc( (MAX_BOXES + 1) * sizeof(struct median_box) );
    if(!boxes) return PERR_OUTOFMEMORY;

    boxptr = boxes;

    boxptr->s[GRAY]  = 0;
    boxptr->d[GRAY]  = 256;

    res = MakeHistograms( rdo );

    if(res != PERR_OK) goto errorexit;

    histograms = rdo->histograms;

    InitProgress(f, XGetStr(MSG_SELECTING_PALETTE), 0,nColors, PPTBase );

    while( colors < nColors ) {
        ULONG freq[258];
        UWORD *bdx;

        for(i = 0; i < 258; i++) freq[i] = 0L;

        if(Progress( f, colors, PPTBase )) {
            res = PERR_BREAK;
            goto errorexit;
        }

        /* Count together all values in a slice */

        for(i = 0; i < boxptr->d[GRAY]; i++ ) {
            freq[i] = (ULONG) histograms[boxptr->s[GRAY]+i];
        }

        /* Search for median */

        bdx = &(boxptr->d[GRAY]);

        for(i = 0; i < *bdx; i++ ) {
            // DEBUG("freq[%d] = %lu\n",i,freq[i]);
            freq[i+1] += freq[i];
        }

        // DEBUG("Search target @%d = %ld (/2 = %ld)\n",boxptr->d[GRAY],
        //                                              freq[boxptr->d[GRAY]],
        //                                              freq[boxptr->d[GRAY]] >> 1);

        for(i = 0; freq[i] < (freq[*bdx] >> 1); i++);
        /* i now points at the area, do the split */

        if(i == *bdx - 1) i--;

        // DEBUG("Split at %d\n",i);

        /* First, make room after this area */

        for(j = nboxes+1; j > cb; j--) {
            boxes[j] = boxes[j-1];
        }

        /* Now split. We won't accept empty boxes or those at the
           border area. */

        colors++;

        (boxptr+1)->s[GRAY] = boxptr->s[GRAY] + i + 1;
        (boxptr+1)->d[GRAY] = *bdx - i - 1;
        boxptr->hits = freq[i];
        (boxptr+1)->hits = freq[*bdx] - freq[i];

        if( freq[i] == freq[*bdx] || freq[i] == 0L) {
            colors--;
        }

        *bdx = i + 1;

        nboxes++;

        cb = MC_GrayGetNewBox( nboxes, histograms, boxes );
        boxptr = &(boxes[cb]);

        // DEBUG("New box is %d \n",cb);

        if(cb < 0 || nboxes >= MAX_BOXES)  /* Run out of colors */
            break;
        /* if(# of colors == nColors) exit split loop */

    } /* while(colors) */

    FinishProgress( f, PPTBase );

    // DEBUG("Median cut ready, now selecting colors\n");

    /* Calculate a weighted average for each box */
    cb = 0; /* Current color */
    boxptr = boxes;

    D(bug("Used %d colors out of %d (%d boxes)\n",colors,nColors,nboxes));

    for(i = 0; i <= nboxes && cb < nColors; i++) {
        ULONG total, hits;
        UBYTE avg;

        hits = 0; total = 0;

        for(j = boxptr->s[GRAY]; j < boxptr->s[GRAY]+boxptr->d[GRAY]; j++ ) {
            hits  += histograms[j];
            total += j * histograms[j];
        }

        if(hits) {
            avg = (UBYTE) (total/hits);
        } else {
            avg = 0;
        }

        // DEBUG("Color=%d:  hits=%lu, total=%lu  => avg = %d\n",
        //       cb, hits, total, avg );

        colortable[cb].r = avg;
        colortable[cb].g = avg;
        colortable[cb].b = avg;

        cb++;
        boxptr++;
    }

    // DoShowColorTable( nColors, colortable );

errorexit:

    pfree(boxes);

    return res;

}

PERROR Palette_MedianCutI( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;

    D(bug("Palette_MedianCutI()\n"));

    switch(rdo->frame->pix->colorspace) {

        case CS_RGB:
        case CS_ARGB:
            rdo->ncolors = rdo->frame->disp->ncolors;
            rdo->Palette  = Palette_MedianCut;
            rdo->PaletteD = Palette_MedianCutD;
            break;

        case CS_GRAYLEVEL:
            rdo->ncolors = rdo->frame->disp->ncolors;
            rdo->Palette  = Palette_GrayMedianCut;
            rdo->PaletteD = Palette_MedianCutD;
            break;

        default:
            Req( NEGNUL,NULL,"This colorspace is not supported\n"
                             "with median cut!");
            res = PERR_INITFAILED;
            break;
    }

    if( res != PERR_OK ) Palette_MedianCutD( rdo );

    return res;
}

/*--------------------------------------------------------------------------*/
/*
    The force-method.
*/

/*
    BUG: The ncolors get out of sync here.
*/
PERROR Palette_Force( struct RenderObject *rdo )
{
    PERROR res;

    if( LoadPalette( rdo->frame, rdo->frame->disp->palettepath, rdo->PPTBase ) <= 0 )
        res = PERR_FILEREAD;

    return res;
}

PERROR Palette_ForceD( struct RenderObject *rdo )
{
    return PERR_OK;
}

PERROR Palette_ForceI( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;

    if( rdo->frame->pix->colorspace != CS_RGB && rdo->frame->pix->colorspace != CS_ARGB) {
        Req( NEGNUL, NULL, GetStr(MSG_PALETTE_FORCE_ON_RGB) );
        res = PERR_INITFAILED;
    } else {
        rdo->ncolors = rdo->frame->disp->ncolors;
        rdo->Palette = Palette_Force;
        rdo->PaletteD = Palette_ForceD;
    }

    return res;
}

/*--------------------------------------------------------------------------*/
/*
    The force old palette method.
*/

/*
    Does not have to do anything.
*/
PERROR Palette_OldForce( struct RenderObject *rdo )
{
    return PERR_OK;
}

PERROR Palette_OldForceD( struct RenderObject *rdo )
{
    return PERR_OK;
}


Prototype PERROR Palette_OldForceI( struct RenderObject *rdo );

PERROR Palette_OldForceI( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;

    rdo->ncolors = rdo->frame->disp->ncolors;
    rdo->Palette = Palette_OldForce;
    rdo->PaletteD = Palette_OldForceD;

    return res;
}

/*--------------------------------------------------------------------------*/
/*                               END OF CODE                                */
/*--------------------------------------------------------------------------*/


