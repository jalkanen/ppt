/*
    Converted from YUVN.I by JJ.

    $Id: yuvn.h,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
 */

#ifndef YUVN_H
#define YUVN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#include <libraries/iffparse.h>

/*
 *  ID's used in FORM YUVN
 */

#define ID_YUVN     MAKE_ID('Y','U','V','N') /* The FORM-ID */
#define ID_VLAB     MAKE_ID('V','L','A','B') /* private, do NOT use */
#define ID_YCHD     MAKE_ID('Y','C','H','D') /* the header-chunk-ID */
#define ID_DATY     MAKE_ID('D','A','T','Y') /* The Y-data-chunk-ID */
#define ID_DATU     MAKE_ID('D','A','T','U') /* The U-data-chunk-ID */
#define ID_DATV     MAKE_ID('D','A','T','V') /* The V-data-chunk-ID */
#define ID_DATA     MAKE_ID('D','A','T','A') /* The A-data-chunk-ID */

/*
 *  Values for ychd_AspectX and ychd_AspectY
 */

/*
 *  Values for ychd_Compress
 */

#define COMPRESS_NONE       0           /* no compression */

/*
 *  values for ychd_Flags
 */

#define YCHDB_LACE          0           /* if set the data-chunks contain */
#define YCHDF_LACE          1           /* a full-frame (interlaced) picture */

/*
 *  values for ychd_Mode
 */

#define YCHD_MODE_BW        0           /* OBSOLETE, use YCHD_MODE_400 instead */
#define YCHD_MODE_400       0           /* a black-and-white picture (no DATU and DATV) */
#define YCHD_MODE_411       1           /* a YUV-411 picture */
#define YCHD_MODE_422       2           /* a YUV-422 picture */
#define YCHD_MODE_444       3           /* a YUV-444 picture */

#define YCHD_MODE_200       8           /* a lores black-and-white picture */
#define YCHD_MODE_211       9           /* a lores color picture (422, but lores) */
#define YCHD_MODE_222      10           /* a lores color picture (444, but lores) */

#define YCHD_MODEB_LORES    3           /* test this bit to check for lores/hires */
#define YCHD_MODEF_LORES    8

/*
 * values for ychd_Norm
 */

#define YCHD_NORM_UNKNOWN   0           /* unknown, try to avoid this */
#define YCHD_NORM_PAL       1           /* PAL 4.433 MHz */
#define YCHD_NORM_NTSC      2           /* NTSC 3.579 MHz */

/*
 *  the FORM-YUVN DataHeader: 'YCHD'
 */

struct YCHD_Header {
    UWORD     ychd_Width;               /* picture width in Y-pixels */
    UWORD     ychd_Height;              /* picture height (rows) */

    UWORD     ychd_PageWidth;           /* source page width & height, */
    UWORD     ychd_PageHeight;          /* currently same as Width and Height */

    UWORD     ychd_LeftEdge;            /* position within the source page, */
    UWORD     ychd_TopEdge;             /* currently 0,0 */

    UBYTE     ychd_AspectX;             /* pixel aspect (width : height) */
    UBYTE     ychd_AspectY;
    UBYTE     ychd_Compress;            /* (see above) */
    UBYTE     ychd_Flags;               /* (see above) */

    UBYTE     ychd_Mode;                /* (see above) */
    UBYTE     ychd_Norm;                /* (see above) */

    WORD      ychd_reserved2;           /* must be 0 */

    LONG      ychd_reserved3;           /* must be 0 */
};

/*
 *---------------------------------------------------------------------------
 * Warning, the UBYTE fields are byte-packed, C-compilers should not add pad
 * bytes!
 *---------------------------------------------------------------------------
 */

/*
 *  My own definitions
 */

#define ERRBASE         100

#define UNKNOWN_COMPRESSION     (ERRBASE + 1)
#define MASKING_NOT_SUPPORTED   (ERRBASE + 2)
#define CANNOT_READ             (ERRBASE + 3)
#define FORMAT_NOT_SUPPORTED    (ERRBASE + 4)
#define PARSEIFF_FAILED         (ERRBASE + 5)
#define SETUP_FAILED            (ERRBASE + 6)
#define IFFPARSE_DID_NOT_OPEN   (ERRBASE + 7)
#define NO_IFF_HANDLE           (ERRBASE + 8)
#define NOT_YUVN                (ERRBASE + 10)

#define ID_ANNO         MAKE_ID('A','N','N','O')
#define ID_AUTH         MAKE_ID('A','U','T','H')


#endif

