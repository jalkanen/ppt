/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : an example effect.

    PPT and this file are (C) Janne Jalkanen 1995.

*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

/*
    First, some compiler stuff to make this compile on SAS/C too.
*/

#ifdef _DCC
#define SAVEDS __geta4
#define ASM
#define REG(x) __ ## x
#else
#define SAVEDS __saveds
#define ASM    __asm
#define REG(x) register __ ## x
#endif

/*
    Here are some includes you might find useful. Actually, not all
    of them are required, but I find it easier to delete extra files
    than add up forgotten ones.
*/

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>

#include <clib/alib_protos.h>

#define __USE_SYSBASE
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/bgui.h>

/*
    These are required, however. Make sure that these are in your include path!
*/

#include <ppt.h>
#include <pragmas/pptsupp_pragmas.h>

/*
    Just some extra, again.
*/

#include <stdio.h>
#include <stdarg.h>
#include <math.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    Define, if the temp2 array should be transposed.  It is a lot
    faster, but it might not work always.
*/

#define FAST_SHEAR

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Rotate"


#ifdef DEBUG_MODE
#define D(x)    x
#define bug     PDebug
#else
#define D(x)
#endif

/*----------------------------------------------------------------------*/
/* Internal prototypes */

static FRAME *DoRotate( FRAME *, ULONG, LONG, BOOL, EXTBASE * );
extern ASM ULONG LIBEffectInquire( REG(d0) ULONG , REG(a5) EXTBASE *);

extern ASM FRAME *LIBEffectExec( REG(a0) FRAME *frame,
                                 REG(a1) struct TagItem *tags,
                                 REG(a5) EXTBASE *ExtBase );


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Rotates an image between -90 and 90 degrees.\n\n";


/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name, (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author, (ULONG) "Janne Jalkanen 1996",
    PPTX_InfoTxt, (ULONG) infoblurb,

    PPTX_ColorSpaces, CSF_RGB|CSF_GRAYLEVEL,

    PPTX_RexxTemplate, (ULONG)"ANGLE/A/N, ANTIALIAS/S",

    PPTX_NoNewFrame, TRUE,
#ifdef _M68881
    PPTX_CPU,       AFF_68030|AFF_68881,
#endif

    TAG_END, 0L
};

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /*M_PI*/

#define SCALE 4096
#define HALFSCALE 2048

typedef struct {
    UBYTE r,g,b;
} xel;

/*----------------------------------------------------------------------*/
/* Code */

#ifndef _DCC
void __regargs __chkabort(void) {} /* Disable SAS/C Ctrl-C detection */
void __regargs _CXBRK(void) {}     /* Disable the handler as well */
#endif

static
FRAME *DoRotate( FRAME *src, ULONG which, LONG angle, BOOL antialias, EXTBASE *ExtBase )
{
    xel *xelrow, *newxelrow;
    xel *temp1xels;
    xel *temp2xels;
    register xel* xP;
    register xel* nxP;
    xel bgxel, prevxel, x;
    int rows = src->pix->height, cols = src->pix->width, newrows;
    int tempcols, newcols, yshearjunk, x2shearjunk, row, col, new;
    float fangle, xshearfac, yshearfac, new0;
    int intnew0;
    register long fracnew0, omfracnew0;
    FRAME *dest, *temp1, *temp2;
    ULONG cspace = src->pix->colorspace;
    WORD comps = src->pix->components;

#ifdef _DCC
    OpenFloats();
#endif

    D(bug("DoRotate()\n"));

    InitProgress( src, "Rotating (1)...", 0, rows );

    fangle = (float)angle * M_PI / 180.0;     /* convert to radians */
    xshearfac = tan( fangle / 2.0 );

    if ( xshearfac < 0.0 )
        xshearfac = -xshearfac;
    yshearfac = sin( fangle );
    if ( yshearfac < 0.0 )
        yshearfac = -yshearfac;

    tempcols = rows * xshearfac + cols + 0.999999;
    yshearjunk = ( tempcols - cols ) * yshearfac;
    newrows = (WORD) (tempcols * yshearfac + rows + 0.999999);
    x2shearjunk = ( newrows - rows - yshearjunk ) * xshearfac;
    newrows -= 2 * yshearjunk;
    newcols = (WORD) (newrows * xshearfac + tempcols + 0.999999 - 2 * x2shearjunk);

    dest = MakeFrame(src);
    dest->pix->width = newcols;
    dest->pix->height = newrows;

    D(bug("\tNew height = %ld, new width = %ld\n",newrows, newcols ));

    if( InitFrame( dest ) != PERR_OK ) {
        RemFrame( dest );
        return NULL;
    }

    bgxel.r = 0; bgxel.g = 0; bgxel.b = 0;

    /* First shear X into temp1xels. */

    temp1 = NewFrame( tempcols, rows, comps );

    for ( row = 0; row < rows; ++row ) {
        xelrow = (xel *)GetPixelRow( src, row );

        if( Progress( src, row ) ) {
            RemFrame( dest );
            RemFrame( temp1 );
            return NULL;
        }

        if ( fangle > 0 )
            new0 = row * xshearfac;
        else
            new0 = ( rows - row ) * xshearfac;
        intnew0 = (int) new0;

        if ( antialias ) {
            fracnew0 = ( new0 - intnew0 ) * SCALE;
            omfracnew0 = SCALE - fracnew0;

            for ( col = 0, nxP = (xel *)GetPixelRow(temp1,row); col < tempcols; ++col, ++nxP )
                *nxP = bgxel;

            prevxel = bgxel;

            temp1xels = (xel *)GetPixelRow( temp1, row );
            nxP = &(temp1xels[intnew0]);

            switch ( cspace ) {
                UBYTE *xp;

                case CS_RGB:
                    for ( col = 0, xP = xelrow; col < cols; ++col, ++nxP, ++xP ) {
                        nxP->r = (fracnew0 * prevxel.r + omfracnew0 * xP->r + HALFSCALE) / SCALE;
                        nxP->g = (fracnew0 * prevxel.g + omfracnew0 * xP->g + HALFSCALE) / SCALE;
                        nxP->b = (fracnew0 * prevxel.b + omfracnew0 * xP->b + HALFSCALE) / SCALE;
                        prevxel = *xP; // BUG: Possible alignment error?
                    }
                    break;

                default:
                    for ( col = 0, xp = (UBYTE *)xelrow; col < cols; ++col, ++nxP, ++xp ) {
                        nxP->r = (fracnew0 * prevxel.r + omfracnew0 * *xp + HALFSCALE)/SCALE;
                        prevxel.r = *xp;
                    }
                    break;
            }

            if ( fracnew0 > 0 && intnew0 + cols < tempcols ) {
                switch ( cspace ) {
                    case CS_RGB:
                        nxP->r = (fracnew0 * prevxel.r + omfracnew0 * bgxel.r + HALFSCALE ) / SCALE;
                        nxP->g = (fracnew0 * prevxel.g + omfracnew0 * bgxel.g + HALFSCALE ) / SCALE;
                        nxP->b = (fracnew0 * prevxel.b + omfracnew0 * bgxel.b + HALFSCALE ) / SCALE;
                        break;

                    default:
                        nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * bgxel.r + HALFSCALE ) / SCALE;
                        break;
                }
            }

            PutPixelRow( temp1, row, temp1xels );

        } else {
            temp1xels = (xel *)GetPixelRow( temp1, row );
            for ( col = 0, nxP = temp1xels; col < intnew0; ++col, ++nxP )
                *nxP = bgxel;
            switch( cspace ) {
                UBYTE *xp;
                case CS_RGB:
                    for ( col = 0, xP = xelrow; col < cols; ++col, ++nxP, ++xP )
                        *nxP = *xP;
                    break;

                default:
                    for ( col = 0, xp = (UBYTE *)xelrow; col < cols; ++col, ++nxP, ++xp )
                        nxP->r = *xp;
                    break;
            }
            for ( col = intnew0 + cols; col < tempcols; ++col, ++nxP )
                *nxP = bgxel;
            PutPixelRow( temp1, row, temp1xels );
        }
    }

    /*
     *  Now inverse shear Y from temp1 into temp2.
     *  This is *real* slow.
     */

    InitProgress( src, "Rotating (2)...", 0, tempcols );

#ifdef FAST_SHEAR
    temp2 = NewFrame( newrows, tempcols, 3 );
#else
    temp2 = NewFrame( tempcols, newrows, 3 );
#endif

    for ( col = 0; col < tempcols; ++col ) {

        if( Progress( src, col ) ) {
            RemFrame( dest );
            RemFrame( temp2 );
            RemFrame( temp1 );
            return NULL;
        }

        if ( fangle > 0 )
            new0 = ( tempcols - col ) * yshearfac;
        else
            new0 = col * yshearfac;

        intnew0 = (int) new0;
        fracnew0 = ( new0 - intnew0 ) * SCALE;
        omfracnew0 = SCALE - fracnew0;
        intnew0 -= yshearjunk;

#ifdef FAST_SHEAR
        temp2xels = (xel *) GetPixelRow( temp2, col );
        for( row = 0; row < newrows; ++row ) {
            temp2xels[row] = bgxel;
        }
#else
        for ( row = 0; row < newrows; ++row ) {
            temp2xels = (xel *)GetPixelRow( temp2, row );
            temp2xels[col] = bgxel;
            PutPixelRow( temp2, row, temp2xels );
        }
#endif
        if ( antialias ) {
            prevxel = bgxel;

#ifdef FAST_SHEAR
            temp2xels = (xel *)GetPixelRow( temp2, col );
#endif

            for ( row = 0; row < rows; ++row ) {
                new = row + intnew0;
                if ( new >= 0 && new < newrows ) {
#ifdef FAST_SHEAR
                    nxP = &(temp2xels[new]);
#else
                    temp2xels = (xel *)GetPixelRow( temp2, new );
                    nxP = &(temp2xels[col]);
#endif
                    temp1xels = (xel *)GetPixelRow( temp1, row );
                    x = temp1xels[col];
                    switch ( cspace ) {
                        case CS_RGB:
                            nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * x.r + HALFSCALE ) / SCALE;
                            nxP->g = ( fracnew0 * prevxel.g + omfracnew0 * x.g + HALFSCALE ) / SCALE;
                            nxP->b = ( fracnew0 * prevxel.b + omfracnew0 * x.b + HALFSCALE ) / SCALE;
                            break;

                        default:
                            nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * x.r + HALFSCALE ) / SCALE;
                            break;
                    }
                    prevxel = x;
#ifndef FAST_SHEAR
                    PutPixelRow( temp2, new, temp2xels );
#endif
                }
            }
            if ( fracnew0 > 0 && intnew0 + rows < newrows ) {
#ifdef FAST_SHEAR
                nxP = &(temp2xels[intnew0 + rows]);
#else
                temp2xels = (xel *)GetPixelRow( temp2, intnew0 + rows );
                nxP = &(temp2xels[col]);
#endif
                switch ( cspace ) {
                    case CS_RGB:
                        nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * bgxel.r + HALFSCALE ) / SCALE;
                        nxP->g = ( fracnew0 * prevxel.g + omfracnew0 * bgxel.g + HALFSCALE ) / SCALE;
                        nxP->b = ( fracnew0 * prevxel.b + omfracnew0 * bgxel.b + HALFSCALE ) / SCALE;
                        break;

                    default:
                        nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * bgxel.r + HALFSCALE ) / SCALE;
                        break;
                }

#ifndef FAST_SHEAR
                PutPixelRow( temp2, intnew0+rows, temp2xels );
#endif
            }
        } else {
            for ( row = 0; row < rows; ++row ) {
                new = row + intnew0;
                if ( new >= 0 && new < newrows ) {
#ifdef FAST_SHEAR
                    temp1xels = (xel *)GetPixelRow( temp1, row );
                    temp2xels[new] = temp1xels[col];
#else
                    temp2xels = (xel *)GetPixelRow( temp2, new );
                    temp1xels = (xel *)GetPixelRow( temp1, row );
                    temp2xels[col] = temp1xels[col];
                    PutPixelRow( temp2, new, temp2xels );
#endif
                }
            }
        }
#ifdef FAST_SHEAR
        PutPixelRow( temp2, col, temp2xels );
#endif
    }
    RemFrame( temp1 );

    /* Finally, shear X from temp2 into newxelrow. */

    InitProgress( src, "Rotating (3)...", 0, newrows );

    for ( row = 0; row < newrows; ++row ) {

        newxelrow = (xel *)GetPixelRow( dest, row );

        if( Progress( src, row ) ) {
            RemFrame( temp2 );
            RemFrame( dest );
            return NULL;
        }

        if ( fangle > 0 )
            new0 = row * xshearfac;
        else
            new0 = ( newrows - row ) * xshearfac;

        intnew0 = (int) new0;
        fracnew0 = ( new0 - intnew0 ) * SCALE;
        omfracnew0 = SCALE - fracnew0;
        intnew0 -= x2shearjunk;

        switch( cspace ) {
            UBYTE *nxp;

            case CS_RGB:
                for ( col = 0, nxP = newxelrow; col < newcols; ++col, ++nxP )
                    *nxP = bgxel;
                break;

            default:
                for( col = 0, nxp = (UBYTE *)newxelrow; col < newcols; ++col, ++nxp )
                    *nxp = bgxel.r;
                break;
        }

        if ( antialias ) {
            prevxel = bgxel;

#ifndef FAST_SHEAR
            temp2xels = (xel *)GetPixelRow( temp2, row );
            for ( col = 0, xP = temp2xels; col < tempcols; ++col, ++xP ) {
#else
            for( col = 0; col < tempcols; ++col ) {
                temp2xels = (xel *)GetPixelRow( temp2, col );
                xP = &(temp2xels[row]);
#endif
                new = intnew0 + col;
                if ( new >= 0 && new < newcols ) {
                    switch ( cspace ) {
                        case CS_RGB:
                            nxP = &(newxelrow[new]);
                            nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * xP->r + HALFSCALE ) / SCALE;
                            nxP->g = ( fracnew0 * prevxel.g + omfracnew0 * xP->g + HALFSCALE ) / SCALE;
                            nxP->b = ( fracnew0 * prevxel.b + omfracnew0 * xP->b + HALFSCALE ) / SCALE;
                            break;

                        default:
                            nxP = (xel *)((UBYTE *)newxelrow + new);
                            nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * xP->r + HALFSCALE ) / SCALE;
                            break;
                    }
                    prevxel = *xP;
                }
#ifdef FAST_SHEAR

#endif
            }
            if ( fracnew0 > 0 && intnew0 + tempcols < newcols ) {
                switch ( cspace ) {
                    case CS_RGB:
                        nxP = &(newxelrow[intnew0 + tempcols]);
                        nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * bgxel.r + HALFSCALE ) / SCALE;
                        nxP->g = ( fracnew0 * prevxel.g + omfracnew0 * bgxel.g + HALFSCALE ) / SCALE;
                        nxP->b = ( fracnew0 * prevxel.b + omfracnew0 * bgxel.b + HALFSCALE ) / SCALE;
                        break;

                    default:
                        nxP = (xel *)((UBYTE *)newxelrow + intnew0 + tempcols);
                        nxP->r = ( fracnew0 * prevxel.r + omfracnew0 * bgxel.r + HALFSCALE ) / SCALE;
                        break;
                }
            }
        } else {
#ifndef FAST_SHEAR
            temp2xels = (xel *)GetPixelRow( temp2, row );
            for ( col = 0, xP = temp2xels; col < tempcols; ++col, ++xP ) {
#else
            for ( col = 0; col < tempcols; ++col ) {
                temp2xels = (xel *)GetPixelRow( temp2, col );
                xP = &(temp2xels[row]);
#endif
                new = intnew0 + col;
                if ( new >= 0 && new < newcols ) {
                    switch( cspace ) {
                        case CS_RGB:
                            newxelrow[new] = *xP;
                            break;

                        default:
                            * ((UBYTE *)newxelrow + new) = xP->r;
                            break;
                    }
                }
            }
        }

        PutPixelRow( dest, row, newxelrow );
    }

    FinishProgress( src );

    if(temp2) RemFrame( temp2 );

#ifdef _DCC
    CloseFloats();
#endif

    return dest;
}



struct Library *MathIeeeDoubBasBase = NULL, *MathIeeeDoubTransBase = NULL;
struct DosLibrary *DOSBase;

SAVEDS ASM int __UserLibInit( REG(a6) struct Library *EffectBase )
{
    if( MathIeeeDoubBasBase = OpenLibrary("mathieeedoubbas.library",0L) ) {
        if( MathIeeeDoubTransBase = OpenLibrary("mathieeedoubtrans.library", 0L )) {
            return 0;
        }
        CloseLibrary(MathIeeeDoubBasBase);
    }
    return 1;
}

SAVEDS ASM void __UserLibCleanup( REG(a6) struct Library *EffectBase )
{
    if( MathIeeeDoubBasBase ) CloseLibrary(MathIeeeDoubBasBase);
    if( MathIeeeDoubTransBase ) CloseLibrary(MathIeeeDoubTransBase);
}

SAVEDS ASM ULONG LIBEffectInquire( REG(d0) ULONG attr, REG(a5) EXTBASE *ExtBase )
{
    return TagData( attr, MyTagArray );
}

SAVEDS ASM FRAME *LIBEffectExec( REG(a0) FRAME *frame,
                                 REG(a1) struct TagItem *tags,
                                 REG(a5) EXTBASE *ExtBase )
{
    ULONG *args;
    FRAME *newframe = NULL;
    LONG angle = -9999;
    ULONG antialias = FALSE;
    PERROR res;

    struct TagItem rot[] = { ARSLIDER_Min, -90,
                             ARSLIDER_Default, 0,
                             ARSLIDER_Max, 90,
                             AROBJ_Value, NULL, TAG_END };

    struct TagItem ant[] = { ARCHECKBOX_Selected, TRUE,
                             AROBJ_Value, NULL,
                             AROBJ_Label, (ULONG)"Antialias?",TAG_END };

    struct TagItem win[] = { AR_Text, (ULONG)"\nSelect rotation angle!\n",
                             AR_SliderObject, 0,
                             AR_CheckBoxObject, 0, TAG_END };

    rot[3].ti_Data = (ULONG) &angle;
    ant[1].ti_Data = (ULONG) &antialias;
    win[1].ti_Data = (ULONG) rot; win[2].ti_Data = (ULONG) ant;

    DOSBase = (struct DosLibrary *)ExtBase->lb_DOS;

    D(bug(MYNAME": Exec()\n"));

    args = (ULONG *) TagData( PPTX_RexxArgs, tags );

    if( args ) {
        if( args[0] )
            angle = *( (LONG *)args[0] );
        if( args[1] )
            antialias = TRUE;
    }

    /*
     *  Open window
     */

    if( angle == -9999 ) {
        if( ( res = AskReqA( frame, win )) != PERR_OK) {
            SetErrorCode( frame, res );
            return NULL;
        }
    }

    D(bug("Got angle %ld\n", angle ));

    if( angle < -90 || angle > 90 ) {
        SetErrorCode( frame, PERR_INVALIDARGS );
        return NULL;
    } else {
        newframe = DoRotate( frame, 0, angle, antialias, ExtBase );
    }

    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

