/*------------------------------------------------------------------------*/
/*
    PROJECT:
    MODULE : gif.h

    DESCRIPTION:

    Common definitions for the gif reader/writer

 */
/*------------------------------------------------------------------------*/

#ifndef GIF_H
#define GIF_H

/*------------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

#ifndef PPT_H
#include <ppt.h>
#endif

#include <pragmas/pptsupp_pragmas.h>

#include <exec/types.h>
#include <exec/memory.h>

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#undef DEBUG_MODE

/*------------------------------------------------------------------------*/
/* Type defines & structures */

#define MAXCOLORMAPSIZE         256
#define MAXCOLORS               MAXCOLORMAPSIZE

#define CM_RED                    0
#define CM_GREEN                  1
#define CM_BLUE                   2

#define MAX_LWZ_BITS             12

#define INTERLACE              0x40
#define LOCALCOLORMAP          0x80

#ifndef EOF
#define EOF                      -1
#endif

/* Compression details */

#define BITS                     12

#define HSIZE                  5003

typedef        unsigned char   char_type;

struct GIF89 {
       int     transparent;
       int     delayTime;
       int     inputFlag;
       int     disposal;
}; /* = { -1, -1, -1, 0 }; */

struct GIF {
    unsigned int    Width;
    unsigned int    Height;
    unsigned char   ColorMap[3][MAXCOLORMAPSIZE];
    unsigned int    BitPixel;
    unsigned int    ColorResolution;
    unsigned int    Background;
    unsigned int    AspectRatio;
    int             GrayScale;
    int             ZeroDataBlock;
    int             Interlace;

    /*
     *  LZW data fields
     */

    int             fresh;
    int             code_size, set_code_size;
    int             max_code, max_code_size;
    int             firstcode, oldcode;
    int             clear_code, end_code;
    int             *table;
    int             *stack, *sp;

    unsigned char   gcbuf[280];
    int             curbit, lastbit, done, last_byte;

    /*
     *  GIF 89 extensions
     */

    struct GIF89    gif89;

    /*
     *  My own data fields
     */

    FRAME           *Frame;
};

/*
 * Pointer to function returning an int
 */
typedef int (*ifunptr)(int, int, struct GIFSave *);

typedef int             code_int;
typedef long int        count_int;

struct GIFSave {
    /*
     *  PPT specifics
     */
    FRAME   *frame;
    struct PPTBase *PPTBase;
    BPTR    fh;
    ROWPTR  pixelbuffer;

    /*
     *  Variables controlling saving.
     */

    int     interlace;
    int     transparent;
    int     background;

    COLORMAP *colormap;

    /*
     *  Coding info
     */

    int     curx, cury, lasty;
    long    CountDown, MaxCount;
    int     Pass;
    int     n_bits;
    int     maxbits;
    code_int maxcode;
    code_int maxmaxcode;

    count_int htab[HSIZE];
    UWORD   codetab[HSIZE];

    code_int hsize;

    int     BitsPerPixel;

    ifunptr getpixel;

    code_int free_ent;

    int     clear_flg;
    int     offset;
    int     in_count;
    int     out_count;
    int     g_init_bits;

    int     ClearCode;
    int     EOFCode;

    /* output() */

    ULONG   cur_accum;
    int     cur_bits;

    /* GIF specific stuff */

    int     a_count;
    char    accum[256];
};


/*------------------------------------------------------------------------*/
/* Macros */

#define BitSet(byte, bit)       (((byte) & (bit)) == (bit))
#define ReadOK(file,buffer,len) (FRead(file, buffer, len, 1) != 0)
#define LM_to_uint(a,b)         (((b)<<8)|(a))

#define MAXCODE(n_bits)         (((code_int) 1 << (n_bits)) - 1)
#define HashTabOf(i)            gs->htab[i]
#define CodeTabOf(i)            gs->codetab[i]
#define tab_prefixof(i)         CodeTabOf(i)
#define tab_suffixof(i)         ((char_type*)(gs->htab))[i]
#define de_stack                ((char_type*)&tab_suffixof((code_int)1<<BITS))


/*------------------------------------------------------------------------*/
/* Prototypes */

extern PERROR ReadColorMap( BPTR, struct GIF *, struct PPTBase * );
extern PERROR ReadImage( BPTR, FRAME *, struct GIF *, struct PPTBase * );
extern int DoExtension( BPTR, int, struct GIF *, struct PPTBase * );
extern PERROR InitLWZData( struct GIF *, struct PPTBase * );
extern VOID FreeLWZData( struct GIF *, struct PPTBase * );

extern int GetPixelColor( int, int, struct GIFSave * );

extern PERROR GIFEncode( struct GIFSave * );

/*------------------------------------------------------------------------*/
/* Globals */



#endif /* GIF_H */

/*------------------------------------------------------------------------*/
/*                          END OF HEADER FILE                            */
/*------------------------------------------------------------------------*/

