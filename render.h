/*
    $Id: render.h,v 4.1 1999/03/31 13:24:06 jj Exp $
*/

#ifndef RENDER_H
#define RENDER_H

typedef USHORT HGRAM;
#define MAXSAMPLE 255
#define QUICKREMAPSIZE       (256+64)

/* If you change the histogram size, then you must change these */

#define HGRAM_BITS_RED       5
#define HGRAM_BITS_GREEN     6
#define HGRAM_BITS_BLUE      5

#define HGRAM_RED           ( 1 << HGRAM_BITS_RED )
#define HGRAM_GREEN         ( 1 << HGRAM_BITS_GREEN )
#define HGRAM_BLUE          ( 1 << HGRAM_BITS_BLUE )

#define HGRAM_SIZE          ( HGRAM_RED * HGRAM_BLUE * HGRAM_GREEN * sizeof(HGRAM))

/* And these three */

#define MASK_RED            0xF8
#define MASK_GREEN          0xFC
#define MASK_BLUE           0xF8

#define OFFSET_RED          ( HGRAM_BITS_GREEN + HGRAM_BITS_BLUE  )
#define OFFSET_GREEN        ( HGRAM_BITS_BLUE )
#define OFFSET_BLUE         1

#define HADDR(r,g,b)        ( ((r) << OFFSET_RED) + ((g) << OFFSET_GREEN) + (b) )

/* And these three */

#define RED2RGB8(x)         (( (x) << (8-HGRAM_BITS_RED) ) | ((x) >> 2))
#define GREEN2RGB8(x)       (( (x) << (8-HGRAM_BITS_GREEN) ) | ((x) >> 4))
#define BLUE2RGB8(x)        (( (x) << (8-HGRAM_BITS_BLUE) ) | ((x) >> 2))

#define SWAPB(b1,b2)        { UBYTE a; a = (b1); (b1) = (b2); (b2) = a; }

/*
    These are for the median-cut algorithm.
    BUG: The color_type should be rewritten as defines.
*/
#define MAX_BOXES           300

typedef enum { RED, GREEN, BLUE } color_type;
#define GRAY RED


/*
    This structure has all necessary data for a render to succeed.
    NOTEZ-BIEN: Only some of the functions use registered arguments, chiefly
    those which are needed for fast usage.
*/

struct RenderObject {
    FRAME *     frame;
    EXTBASE *   ExtBase;
    ROWPTR      cp; /* Current row ptr */
    UBYTE *     buffer; /* Chunky buffer, which contains data in byte form */
    UWORD       currentrow, /* Check these to see where we're going... */
                currentcolumn;

    /*
     *  Dither object begins here.
     */

    ULONG       dither;
    PERROR      (*Dither)( struct RenderObject *);
    PERROR      (*DitherD)(struct RenderObject *);
    APTR        DitherObject;
    UBYTE       newr, newg, newb; /* These are READ ONLY, except for the colormapper */
    UBYTE       reserved0; /* Reserved for future usage */

    /*
     *  Palette selection
     */

    UWORD       ncolors;
    UWORD       reserved1; /* Reserved for future use. */
    HGRAM *     histograms;
    ULONG       hsize;          /* Size of the allocate histogram buffer */
    COLORMAP    *colortable;
    PERROR      (*Palette)(struct RenderObject *);
    PERROR      (*PaletteD)(struct RenderObject *);

    /*
     *  Color selection, when mapping. This one needs to be fast,
     *  thus we're using registered arguments.
     */

    UWORD       ASM (*GetColor)( REGDECL(a0,struct RenderObject *), REGDECL(d0,UBYTE), REGDECL(d1,UBYTE), REGDECL(d2,UBYTE) );
    ULONG       EHB_Data; /* EHB mode uses this.  Initialize to zero.  */

    /*
     *  Display device I/O
     */

    PERROR      (*DispDeviceOpen)(struct RenderObject *);
    PERROR      (*DispDeviceClose)(struct RenderObject *);
    PERROR      (*DispDeviceD)(struct RenderObject *);
    PERROR      (*WriteLine)( struct RenderObject *, WORD);
    PERROR      (*LoadCMap)(struct RenderObject *,COLORMAP *);
    PERROR      (*ActivateDisplay)(struct RenderObject *);
    PLANEPTR    (*GetRow)(struct RenderObject *, UWORD);
    LONG        (*HandleDispIDCMP)(struct RenderObject *);
    struct Screen * (*GetScreen)(struct RenderObject *);
    ULONG       SigMask; /* A copy of this window's sigmask */
    APTR        DispDeviceObject;
};


#endif /* RENDER_H */
