/*
    $Id: dither.c,v 1.2 1995/09/07 23:10:16 jj Exp $
*/

#include <render.h>

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


/*-----------------------------------------------------------*/
/* Prototypes */

Prototype PERROR Dither_NoneI( struct RenderObject * );
Prototype PERROR Dither_NoneD( struct RenderObject * );
Prototype VOID   Dither_None( struct RenderObject * );

Prototype PERROR Dither_FSI( struct RenderObject * );
Prototype PERROR Dither_FSD( struct RenderObject * );
Prototype VOID   Dither_FS( struct RenderObject * );

/*-----------------------------------------------------------*/

/*
    Initialization routine
*/
PERROR Dither_NoneI( struct RenderObject *rdo )
{
    return PERR_OK;
}

/*
    Deconstructor
*/
PERROR Dither_NoneD( struct RenderObject *rdo )
{
    return PERR_OK;
}

/*
    Execution
*/
VOID Dither_None( struct RenderObject *rdo )
{
    WORD width = rdo->frame->pix->width,col;
    ROWPTR cp = rdo->cp;
    UBYTE *dcp = rdo->dest;

    for( col = 0; col < width; col++ ) {
        ULONG pixcode;
        UBYTE red,green,blue;

        red   = *cp++;
        green = *cp++;
        blue  = *cp++;
        pixcode = ((*rdo->GetColor))( rdo, &red, &green, &blue );
        *dcp++  = pixcode;
    }
}

/*-----------------------------------------------------------*/
/* FLOYD-STEINBERG */

/*
    Initialization routine
*/
PERROR Dither_FSI( struct RenderObject *rdo )
{
    struct FSObject *fs;
    UWORD width = rdo->frame->pix->width;
    PERROR res;

    fs = pzmalloc( sizeof( struct FSObject ) );
    if(!fs) return PERR_OUTOFMEMORY;

    fs->odd_row = FALSE;
    fs->error_limit = Init_FS_ErrorLimit();
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

    rdo->DitherObject = (APTR) fs;

    return res;
}

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
    }
    return PERR_OK;
}

/*
    Execution
*/
VOID Dither_FS( struct RenderObject *rdo )
{
    struct FSObject *fs;
    UBYTE pixelsize = rdo->frame->pix->components;
    int *error_limit;
    FSERROR *errorptr;
    int dir, dir3;
    int cur0,cur1, cur2;
    int belowerr0, belowerr1, belowerr2;
    int bpreverr0, bpreverr1, bpreverr2;
    ROWPTR cp = rdo->cp;
    WORD width = rdo->frame->pix->width;
    WORD col;
    HGRAM *hgrams = rdo->histograms;
    UBYTE *dcp = rdo->dest;

    fs = (struct FSObject *) rdo->DitherObject;
    error_limit = fs->error_limit;
    errorptr = fs->errorptr;

    if(fs->odd_row) {
        cp += MULU16((width-1),pixelsize); /* Locate end of row */
        dcp += width-1; /* End of destination pointer */
        dir = -1; dir3 = -3;
        errorptr = fs->fserrors + MULU16((width+1),pixelsize);
        fs->odd_row = FALSE;
    } else {
        dir = 1; dir3 = 3;
        errorptr = fs->fserrors;
        fs->odd_row = TRUE;
    }
    cur0 = cur1 = cur2 = 0; /* Zero error values */
    belowerr0 = belowerr1 = belowerr2 = 0;
    bpreverr0 = bpreverr1 = bpreverr2 = 0;

    for( col = width; col > 0; col-- ) {
        UBYTE color;

        cur0 = (cur0 + errorptr[dir3+0] + 8) >> 4;
        cur1 = (cur1 + errorptr[dir3+1] + 8) >> 4;
        cur2 = (cur2 + errorptr[dir3+2] + 8) >> 4;
        cur0 = error_limit[cur0];
        cur1 = error_limit[cur1];
        cur2 = error_limit[cur2];
        cur0 += cp[0]; cur1 += cp[1]; cur2 += cp[2];
        if(cur0 < 0) cur0 = 0; else if(cur0 > MAXSAMPLE) cur0 = MAXSAMPLE;
        if(cur1 < 0) cur1 = 0; else if(cur1 > MAXSAMPLE) cur1 = MAXSAMPLE;
        if(cur2 < 0) cur2 = 0; else if(cur2 > MAXSAMPLE) cur2 = MAXSAMPLE;
#if 0
        cur0 = range_limit[cur0];
        cur1 = range_limit[cur1];
        cur2 = range_limit[cur2];
#endif

        /*
         *  This does the actual color matching. Calculate the error we
         *  just made.
         *  BUG: Should call the real routine.
         */

        {   register UWORD pixcode;
            register UBYTE oldcur0, oldcur1, oldcur2;

            oldcur0 = cur0; oldcur1 = cur1; oldcur2 = cur2;

            pixcode = (*(rdo->GetColor))( rdo, &cur0, &cur1, &cur2 );
            *dcp = (UBYTE)(pixcode);
            cur0 = oldcur0 - cur0;
            cur1 = oldcur1 - cur1;
            cur2 = oldcur2 - cur2;
        }

        /*
         *  Distribute the error
         */

        { register int bnexterr, delta;

            bnexterr = cur0;        /* Process component 0 */
            delta = cur0 * 2;
            cur0 += delta;          /* form error * 3 */
            errorptr[0] = (FSERROR) (bpreverr0 + cur0);
            cur0 += delta;          /* form error * 5 */
            bpreverr0 = belowerr0 + cur0;
            belowerr0 = bnexterr;
            cur0 += delta;          /* form error * 7 */
            bnexterr = cur1;        /* Process component 1 */
            delta = cur1 * 2;
            cur1 += delta;          /* form error * 3 */
            errorptr[1] = (FSERROR) (bpreverr1 + cur1);
            cur1 += delta;          /* form error * 5 */
            bpreverr1 = belowerr1 + cur1;
            belowerr1 = bnexterr;
            cur1 += delta;          /* form error * 7 */
            bnexterr = cur2;        /* Process component 2 */
            delta = cur2 * 2;
            cur2 += delta;          /* form error * 3 */
            errorptr[2] = (FSERROR) (bpreverr2 + cur2);
            cur2 += delta;          /* form error * 5 */
            bpreverr2 = belowerr2 + cur2;
            belowerr2 = bnexterr;
            cur2 += delta;          /* form error * 7 */
        }

        cp += dir3;
        dcp += dir;
        errorptr += dir3;
    } /* for col */

    errorptr[0] = (FSERROR) bpreverr0; /* unload prev errs into array */
    errorptr[1] = (FSERROR) bpreverr1;
    errorptr[2] = (FSERROR) bpreverr2;

    return;
}

