/*------------------------------------------------------------------------*/
/*
    PROJECT: PPT
    MODULE : TGA loader

    DESCRIPTION:

 */
/*------------------------------------------------------------------------*/

#ifndef SAMPLE_LOADER_H
#define SAMPLE_LOADER_H

/*------------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

/*------------------------------------------------------------------------*/
/* Type defines & structures */

struct TgaOpts {
    ULONG compress;
    ULONG interleave;
};

struct Targa {
    unsigned char IDLength;             /* length of Identifier String */
    unsigned char CoMapType;            /* 0 = no map */
    unsigned char ImgType;              /* image type (see below for values) */
    unsigned short Index;               /* index of first color map entry */
    unsigned short Length;              /* number of entries in color map */
    unsigned char CoSize;               /* size of color map entry (15,16,24,32) */
    unsigned short X_org;               /* x origin of image */
    unsigned short Y_org;               /* y origin of image */
    unsigned short Width;               /* width of image */
    unsigned short Height;              /* height of image */
    unsigned char PixelSize;            /* pixel size (8,16,24,32) */
    unsigned char AttBits;              /* 4 bits, number of attribute bits per pixel */
    unsigned char Rsrvd;                /* 1 bit, reserved */
    unsigned char OrgBit;               /* 1 bit, origin: 0=lower left, 1=upper left */
    unsigned char IntrLve;              /* 2 bits, interleaving flag */
    unsigned char IDString[256];        /* Commercial messages */
    ARGBPixel     *colormap;            /* Contains the extra alpha byte */
    BOOL          rlenencoded;
    BOOL          mapped;
    int           maxval;

    /*
     *  Variables used only while loading TGA image
     */

    int           RLE_count, RLE_flag;

    /*
     *  Variables used only while saving TGA image
     */

    int           *runlength;
};

/* Image types */
#define TGA_Null        0
#define TGA_Map         1
#define TGA_RGB         2
#define TGA_Mono        3
#define TGA_RLEMap      9
#define TGA_RLERGB      10
#define TGA_RLEMono     11
#define TGA_CompMap     32
#define TGA_CompMap4    33

/* Definitions for interleave flag. */
#define TGA_IL_None     0
#define TGA_IL_Two      1
#define TGA_IL_Four     2

#define MAXCOLORS       16384

/*------------------------------------------------------------------------*/
/* Prototypes */

/*------------------------------------------------------------------------*/
/* Globals */



#endif /* TESTLOADER_H */

/*------------------------------------------------------------------------*/
/*                          END OF HEADER FILE                            */
/*------------------------------------------------------------------------*/

