/*
    PROJECT: ppt
    MODULE : dither.c

    $Id: dither.c,v 5.0 1999/02/20 15:27:03 jj Exp $

    This contains the dither initialization, destruction and
    execution functions. The following dither modes are enabled:

        * No dither
        * Floyd-Steinberg

*/

#include "defs.h"
#include "misc.h"
#include "render.h"

/*-----------------------------------------------------------*/
/* Type definitions */

typedef WORD FSERROR;

#define GILES 1

struct FSObject {
    int *error_limit;
    FSERROR *fserrors;
    FSERROR *errorptr;
    BOOL odd_row;
};

struct OrderedObject {
    int **dith_mat;
    int dith_dim, dith_dm2;
};

/*-----------------------------------------------------------*/
/* Prototypes */

Prototype PERROR Dither_NoneI( struct RenderObject * );
Prototype PERROR Dither_FSI( struct RenderObject * );
Prototype PERROR Dither_OrderedI( struct RenderObject * );

/*-----------------------------------------------------------*/
/* No dithering */

/// Dither_NoneD()
/*
    Deconstructor
*/
PERROR Dither_NoneD( struct RenderObject *rdo )
{
    D(bug("\tDither_NoneD()\n"));
    return PERR_OK;
}
///
/// Dither_None()
/*
    Execution
*/
Local
PERROR Dither_None( struct RenderObject *rdo )
{
    WORD width = rdo->frame->pix->width,col;
    ROWPTR cp = rdo->cp;
    UBYTE *dcp = rdo->buffer;
    UBYTE colorspace = rdo->frame->pix->colorspace;
    DISPLAY *d = rdo->frame->disp;

    // D(bug("\tDither_None(width=%d)\n",width));

    for( col = 0; col < width; col++ ) {
        UWORD pixcode;
        UBYTE red,green,blue, ialpha = 255;

        rdo->currentcolumn = col;

        if( colorspace == CS_ARGB ) ialpha = 255 - *cp++; /* Skip Alpha */

        red   = *cp++;
        green = *cp++;
        blue  = *cp++;

        if( d->drawalpha )
            pixcode = (*rdo->GetColor)( rdo, (red*ialpha)>>8, (green*ialpha)>>8, (blue*ialpha)>>8 );
        else
            pixcode = (*rdo->GetColor)( rdo, red, green, blue );

        dcp[col] = (UBYTE)pixcode;
    }
    return PERR_OK;
}
///
/// Dither_NoneGray()
Local
PERROR Dither_NoneGray( struct RenderObject *rdo )
{
    WORD width = rdo->frame->pix->width,col;
    ROWPTR cp = rdo->cp;
    UBYTE *dcp = rdo->buffer;

    // D(bug("\tDither_None(width=%d)\n",width));

    for( col = 0; col < width; col++ ) {
        UWORD pixcode;
        UBYTE gray;

        rdo->currentcolumn = col;

        gray   = *cp++;

        pixcode = (*rdo->GetColor)( rdo, gray, gray, gray );
        dcp[col] = (UBYTE)pixcode;
    }
    return PERR_OK;
}
///
/// Dither_NoneI()
/*
    Initialization
*/
PERROR Dither_NoneI( struct RenderObject *rdo )
{
    D(bug("\tDither_NoneI()\n"));

    rdo->DitherD = Dither_NoneD;

    switch(rdo->frame->pix->colorspace) {
        case CS_RGB:
        case CS_ARGB:
            rdo->Dither = Dither_None;
            break;

        case CS_GRAYLEVEL:
            rdo->Dither = Dither_NoneGray;
            break;
    }

    return PERR_OK;
}
///

/*-----------------------------------------------------------*/
/* Ordered dithering */

#define DITHER(p,d)         ((UBYTE) (( (((p)+(d))<<4) -(p))>>4) )

/// MakeDitherMatrix()
/*
 *  Builds a global dithering matrix.
 *  Uses code from the netpbm package.
 *  ppmdither is (C) 1991 Christos Zoulas
 */

Local
int DithValue( int y, int x, int size )
{
    int d;

    for( d = 0; size-- > 0; x >>= 1, y >>= 1 ) {
        d = (d << 2) | (((x & 1) ^ (y & 1)) << 1) | (y & 1);
    }
    return d;
}

Prototype VOID MakeDitherMatrix(int ***matrix, int *ddim, int *dm2, int dim);

VOID MakeDitherMatrix(int ***matrix, int *ddim, int *dm2, int dim)
{
    int x, y, *dat;

    D(bug("MakeDitherMatrix(%d)\n",dim));

    (*ddim) = (1<<dim);
    (*dm2) = (*ddim) * (*ddim);

    *matrix = (int **)smalloc( ( (*ddim) * sizeof(int *) ) +
                               ( (*dm2) * sizeof(int)));

    if( *matrix == NULL ) return; // BUG: no error code

    dat = (int *) &(*matrix)[ (*ddim) ];
    for (y = 0; y < (*ddim); y++)
        (*matrix)[y] = &dat[y * (*ddim)];

    for (y = 0; y < (*ddim); y++) {
        for (x = 0; x < (*ddim); x++) {
             (*matrix)[y][x] = DithValue(y, x, dim);

            D(bug("%4d ",(*matrix)[y][x]));
        }
        D(bug("\n"));
    }
}
///

/// Dither_Ordered()
Local
PERROR Dither_Ordered( struct RenderObject *rdo )
{
    struct OrderedObject *oo = (struct OrderedObject *) rdo->DitherObject;
    WORD width = rdo->frame->pix->width, col;
    ROWPTR cp = rdo->cp;
    UBYTE *dcp = rdo->buffer;
    UBYTE colorspace = rdo->frame->pix->colorspace;
    DISPLAY *d = rdo->frame->disp;
    int dm = oo->dith_dim - 1, *m;

    m = oo->dith_mat[rdo->currentrow & dm];

    for( col = 0; col < width; col++ ) {
        UWORD pixcode;
        UBYTE red,green,blue, ialpha = 255;
        int dv;

        rdo->currentcolumn = col;
        dv = m[col & dm];

        if( colorspace == CS_ARGB ) ialpha = 255 - *cp++; /* Skip Alpha */

        red   = *cp++;
        green = *cp++;
        blue  = *cp++;

        red   = DITHER(red,dv);
        green = DITHER(green,dv);
        blue  = DITHER(blue,dv);

        if( d->drawalpha )
            pixcode = (*rdo->GetColor)( rdo, (red*ialpha)>>8, (green*ialpha)>>8, (blue*ialpha)>>8 );
        else
            pixcode = (*rdo->GetColor)( rdo, red, green, blue );

        dcp[col] = (UBYTE)pixcode;

    }

    return PERR_OK;
}
///
/// Dither_OrderedGray()
Local
PERROR Dither_OrderedGray( struct RenderObject *rdo )
{
    struct OrderedObject *oo = (struct OrderedObject *) rdo->DitherObject;
    WORD width = rdo->frame->pix->width, col;
    ROWPTR cp = rdo->cp;
    UBYTE *dcp = rdo->buffer;
    int dm = oo->dith_dim - 1, *m;

    m = oo->dith_mat[rdo->currentrow & dm];

    for( col = 0; col < width; col++ ) {
        UWORD pixcode;
        UBYTE grey;
        int dv;

        rdo->currentcolumn = col;
        dv = m[col & dm];

        grey  = *cp++;
        grey  = DITHER(grey,dv);

        pixcode = (*rdo->GetColor)( rdo, grey, grey, grey );

        dcp[col] = (UBYTE)pixcode;
    }

    return PERR_OK;
}
///
/// Dither_OrderedD()
PERROR Dither_OrderedD( struct RenderObject *rdo )
{
    struct OrderedObject *oo = (struct OrderedObject *) rdo->DitherObject;

    D(bug("\tDither_OrderedD()\n"));

    if( oo ) {
        if( oo->dith_mat ) sfree( oo->dith_mat );
        sfree( oo );
        rdo->DitherObject = NULL;
    }
    return PERR_OK;
}
///
/// Dither_OrderedI()
PERROR Dither_OrderedI( struct RenderObject *rdo )
{
    struct OrderedObject *oo;

    D(bug("\tDither_OrderedI()\n"));

    rdo->DitherD = Dither_OrderedD;

    oo = smalloc( sizeof( struct OrderedObject ) );
    if(!oo) return PERR_OUTOFMEMORY;

    bzero( oo, sizeof(oo) );
    rdo->DitherObject = oo;

    MakeDitherMatrix( &oo->dith_mat, &oo->dith_dim, &oo->dith_dm2, 2 );

    switch( rdo->frame->pix->colorspace ) {
        case CS_RGB:
        case CS_ARGB:
            rdo->Dither = Dither_Ordered;
            break;
        case CS_GRAYLEVEL:
            rdo->Dither = Dither_OrderedGray;
            break;
    }

    return PERR_OK;
}
///

/*-----------------------------------------------------------*/
/* FLOYD-STEINBERG */

/// Init_FS_ErrorLimits()
/*
    This routine has been stolen from the IJG V5 code. It initializes
    the FS error limits.
*/
Local
int *Init_FS_ErrorLimits( void )
{
    int * table;
    int in, out;

    table = (int *) pmalloc( (MAXSAMPLE*2+1) * sizeof(int) );
    if(!table) {
        D(bug("Outta memory allocating FS error tables!\n"));
        return NULL;
    }

    table += MAXSAMPLE;

#ifdef GILES
#define STEPSIZE ((MAXSAMPLE+1)/32)
    /* Map errors 1:1 up to +- MAXSAMPLE/32 */
    out = 0;
    for (in = 0; in < STEPSIZE; in++, out++) {
        table[in] = out; table[-in] = -out;
    }
    /* Map errors 1:2 up to +- 3*MAXSAMPLE/32 */
    for (; in < STEPSIZE*3; in++, out += (in&1) ? 0 : 1) {
        table[in] = out; table[-in] = -out;
    }
    /* Map errors 1:4 up to +- 7*MAXSAMPLE/32 */
    for (; in < STEPSIZE*7; in++, out += (in&3) ? 0 : 1) {
        table[in] = out; table[-in] = -out;
    }
    /* Map errors 1:8 up to +- 15*MAXSAMPLE/32 */
    for (; in < STEPSIZE*15; in++, out += (in&7) ? 0 : 1) {
        table[in] = out; table[-in] = -out;
    }
    /* Clamp the rest to final out value (which is (MAXSAMPLE+1)/8) */
    for (; in <= MAXSAMPLE; in++) {
        table[in] = out; table[-in] = -out;
    }
#else
#define STEPSIZE ((MAXSAMPLE+1)/16)
    /* Map errors 1:1 up to +- MAXSAMPLE/16 */
    out = 0;
    for (in = 0; in < STEPSIZE; in++, out++) {
        table[in] = out; table[-in] = -out;
    }
    /* Map errors 1:2 up to +- 3*MAXSAMPLE/16 */
    for (; in < STEPSIZE*3; in++, out += (in&1) ? 0 : 1) {
        table[in] = out; table[-in] = -out;
    }
    /* Clamp the rest to final out value (which is (MAXSAMPLE+1)/8) */
    for (; in <= MAXSAMPLE; in++) {
        table[in] = out; table[-in] = -out;
    }
#endif
#undef STEPSIZE

    return table;
}
///
/// Dither_FSD()
/*
    Deconstructor
*/
PERROR Dither_FSD( struct RenderObject *rdo )
{
    struct FSObject *fs;

    fs = (struct FSObject *) rdo->DitherObject;
    if( fs ) {
        if( fs->fserrors ) pfree( fs->fserrors );
        if( fs->error_limit ) pfree( fs->error_limit - MAXSAMPLE );
        pfree( fs );
        rdo->DitherObject = NULL;
    }
    return PERR_OK;
}
///
/// Dither_FS()
/*
    Execution.  This will handle both RGB and ARGB images.
*/
Local
PERROR Dither_FS( struct RenderObject *rdo )
{
    struct FSObject *fs;
    UBYTE pixelsize = rdo->frame->pix->components;
    int *error_limit;
    FSERROR *errorptr;
    int dir, dir3, dirc;
    int cur0,cur1, cur2;
    int belowerr0, belowerr1, belowerr2;
    int bpreverr0, bpreverr1, bpreverr2;
    ROWPTR cp = rdo->cp;
    WORD width = rdo->frame->pix->width;
    WORD col;
    UBYTE *dcp = rdo->buffer, ialpha = 255; // By default, all pixels are opaque
    DISPLAY *d = rdo->frame->disp;

    fs = (struct FSObject *) rdo->DitherObject;
    error_limit = fs->error_limit;
    errorptr = fs->errorptr;

    if(fs->odd_row) {
        cp += MULU16((width-1),pixelsize); /* Locate end of row */
        dcp += width-1; /* End of destination pointer */
        dir = -1; dir3 = -3; dirc = (pixelsize == 4 ) ? -5 : -3;
        errorptr = fs->fserrors + MULU16((width+1),3);
        fs->odd_row = FALSE;
    } else {
        UBYTE renderq = rdo->frame->disp->renderq;

        dir = 1; dir3 = 3; dirc = 3;
        errorptr = fs->fserrors;

        /*
         *  We'll change the direction only when the rendermode is not HAM, since
         *  in these modes the pixel value depends from the pixel left of the
         *  current position.
         */

        if( renderq != RENDER_HAM6 && renderq != RENDER_HAM8 )
            fs->odd_row = TRUE;
        else
            fs->odd_row = FALSE;
    }
    cur0 = cur1 = cur2 = 0; /* Zero error values */
    belowerr0 = belowerr1 = belowerr2 = 0;
    bpreverr0 = bpreverr1 = bpreverr2 = 0;

    for( col = width; col > 0; col-- ) {

        if( pixelsize == 4 ) {
            ialpha = 255 - *cp++;
        }

        rdo->currentcolumn = width-col;

        cur0 = (cur0 + errorptr[dir3+0] + 8) >> 4;
        cur1 = (cur1 + errorptr[dir3+1] + 8) >> 4;
        cur2 = (cur2 + errorptr[dir3+2] + 8) >> 4;
        cur0 = error_limit[cur0];
        cur1 = error_limit[cur1];
        cur2 = error_limit[cur2];
        cur0 += cp[0]; cur1 += cp[1]; cur2 += cp[2];
#if 1
        if(cur0 < 0) cur0 = 0; else if(cur0 > MAXSAMPLE) cur0 = MAXSAMPLE;
        if(cur1 < 0) cur1 = 0; else if(cur1 > MAXSAMPLE) cur1 = MAXSAMPLE;
        if(cur2 < 0) cur2 = 0; else if(cur2 > MAXSAMPLE) cur2 = MAXSAMPLE;
#else
        cur0 = range_limit[cur0];
        cur1 = range_limit[cur1];
        cur2 = range_limit[cur2];
#endif

        /*
         *  This does the actual color matching. Calculate the error we
         *  just made.
         *
         *  If alpha blending is set on, then we will blend it with
         *  the background color.
         *  BUG: Currently the background color is black, always.
         *  BUG: Should call the real routine.
         */

        {   register UWORD pixcode;

            if( d->drawalpha )
                pixcode = (*rdo->GetColor)( rdo, (cur0*ialpha)>>8, (cur1*ialpha)>>8, (cur2*ialpha)>>8 );
            else
                pixcode = (*rdo->GetColor)( rdo, cur0, cur1, cur2 );

            *dcp = (UBYTE)(pixcode);
            cur0 -= rdo->newr;
            cur1 -= rdo->newg;
            cur2 -= rdo->newb;
        }

        /*
         *  Distribute the error
         */

        { register int bnexterr, delta;

            bnexterr = cur0;        /* Process component 0 */
            delta = cur0 << 1;
            cur0 += delta;          /* form error * 3 */
            errorptr[0] = (FSERROR) (bpreverr0 + cur0);
            cur0 += delta;          /* form error * 5 */
            bpreverr0 = belowerr0 + cur0;
            belowerr0 = bnexterr;
            cur0 += delta;          /* form error * 7 */
            bnexterr = cur1;        /* Process component 1 */
            delta = cur1 << 1;
            cur1 += delta;          /* form error * 3 */
            errorptr[1] = (FSERROR) (bpreverr1 + cur1);
            cur1 += delta;          /* form error * 5 */
            bpreverr1 = belowerr1 + cur1;
            belowerr1 = bnexterr;
            cur1 += delta;          /* form error * 7 */
            bnexterr = cur2;        /* Process component 2 */
            delta = cur2 << 1;
            cur2 += delta;          /* form error * 3 */
            errorptr[2] = (FSERROR) (bpreverr2 + cur2);
            cur2 += delta;          /* form error * 5 */
            bpreverr2 = belowerr2 + cur2;
            belowerr2 = bnexterr;
            cur2 += delta;          /* form error * 7 */
        }

        cp += dirc;
        dcp += dir;
        errorptr += dir3;
    } /* for col */

    errorptr[0] = (FSERROR) bpreverr0; /* unload prev errs into array */
    errorptr[1] = (FSERROR) bpreverr1;
    errorptr[2] = (FSERROR) bpreverr2;

    return PERR_OK;
}
///
/// Dither_FSGray8()
/*
    Execution. This is for graylevel data.
*/
Local
PERROR Dither_FSGray8( struct RenderObject *rdo )
{
    struct FSObject *fs;
    int *error_limit;
    FSERROR *errorptr;
    int dir;
    int cur;
    int belowerr;
    int bpreverr;
    ROWPTR cp = rdo->cp;
    WORD width = rdo->frame->pix->width;
    WORD col;
    UBYTE *dcp = rdo->buffer;

    fs = (struct FSObject *) rdo->DitherObject;
    error_limit = fs->error_limit;
    errorptr = fs->errorptr;

    if(fs->odd_row) {
        cp += (width-1); /* Locate end of row */
        dcp += width-1; /* End of destination pointer */
        dir = -1;
        errorptr = fs->fserrors + width + 1;
        fs->odd_row = FALSE;
    } else {
        UBYTE renderq = rdo->frame->disp->renderq;

        dir = 1;
        errorptr = fs->fserrors;

        /*
         *  We'll change the direction only when the rendermode is not HAM, since
         *  in these modes the pixel value depends from the pixel left of the
         *  current position.
         */

        if( renderq != RENDER_HAM6 && renderq != RENDER_HAM8 )
            fs->odd_row = TRUE;
    }
    cur = 0; /* Zero error values */
    belowerr = 0;
    bpreverr = 0;

    for( col = width; col > 0; col-- ) {

        rdo->currentcolumn = width-col;

        cur = (cur + errorptr[dir] + 8) >> 4;
        cur = error_limit[cur];

        cur += cp[0];
        if(cur < 0) cur = 0; else if(cur > MAXSAMPLE) cur = MAXSAMPLE;

        /*
         *  This does the actual color matching. Calculate the error we
         *  just made.
         *  BUG: Should call the real routine.
         */

        {   register UWORD pixcode;

            pixcode = (*rdo->GetColor)( rdo, cur, cur, cur );
            *dcp = (UBYTE)(pixcode);
            cur -= rdo->newr;
        }

        /*
         *  Distribute the error
         */

        { register int bnexterr, delta;

            bnexterr = cur;        /* Process component 0 */
            delta = cur << 1;
            cur += delta;          /* form error * 3 */
            errorptr[0] = (FSERROR) (bpreverr + cur);
            cur += delta;          /* form error * 5 */
            bpreverr = belowerr + cur;
            belowerr = bnexterr;
            cur += delta;          /* form error * 7 */
        }

        cp += dir;
        dcp += dir;
        errorptr += dir;
    } /* for col */

    errorptr[0] = (FSERROR) bpreverr; /* unload prev errs into array */

    return PERR_OK;
}
///
/// Dither_FSI()
/*
    Initialization routine
*/
PERROR Dither_FSI( struct RenderObject *rdo )
{
    struct FSObject *fs;
    UWORD width = rdo->frame->pix->width;
    PERROR res = PERR_OK;

    fs = pzmalloc( sizeof( struct FSObject ) );
    if(!fs) return PERR_OUTOFMEMORY;

    fs->odd_row = FALSE;
    fs->error_limit = Init_FS_ErrorLimits();
    if(fs->error_limit) {
        fs->fserrors = pzmalloc( (width + 2) * rdo->frame->pix->components * sizeof(FSERROR) );
        if( fs->fserrors ) {
            res = PERR_OK;
        } else {
            res = PERR_INITFAILED;
        }
    } else {
        res = PERR_INITFAILED;
    }

    switch( rdo->frame->pix->colorspace ) {

        case CS_RGB:
        case CS_ARGB:
            rdo->DitherObject = (APTR) fs;
            rdo->Dither = Dither_FS;
            rdo->DitherD = Dither_FSD;
            break;

        case CS_GRAYLEVEL:
            rdo->DitherObject = (APTR) fs;
            rdo->Dither = Dither_FSGray8;
            rdo->DitherD = Dither_FSD;
            break;

        default:
            Req( NEGNUL,NULL, "You cannot make a Floyd-Steinberg dither\n"
                              "in this colorspace!" );
            res = PERR_INITFAILED;
            break;
    }

    if( res != PERR_OK )
        Dither_FSD( rdo );

    return res;
}
///

