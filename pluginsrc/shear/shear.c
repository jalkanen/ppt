/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : shear

    PPT and this file are (C) Janne Jalkanen 1995-2000.

    $Id: shear.c,v 1.1 2001/10/25 16:23:03 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Shear"

#define HORIZONTAL  0
#define VERTICAL    1


/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Shear an image to horizontal or\n"
    "vertical direction.";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name,          (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,        (ULONG)"Janne Jalkanen, 1996-2000",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"HORIZONTAL/S,VERTICAL/S,ANGLE/N",

    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,

    PPTX_NoNewFrame,    TRUE,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

struct Values {
    LONG dir;
    LONG amount;
};

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

const char *labels[] = { "Horizontal", "Vertical", NULL };

struct TagItem cyc[] = {
    ARCYCLE_Active,     0,
    AROBJ_Value,        (ULONG)NULL,
    AROBJ_Label,        (ULONG)"Direction:",
    ARCYCLE_Labels,     (ULONG)labels,
    TAG_DONE
};

struct TagItem sli[] = {
    ARSLIDER_Default,   0,
    AROBJ_Value,        (ULONG)NULL,
    AROBJ_Label,        (ULONG)"Angle:",
    ARSLIDER_Min,       -89,
    ARSLIDER_Max,       89,
    TAG_DONE
};

struct TagItem win[] = {
    AR_CycleObject,     (ULONG)cyc,
    AR_SliderObject,    (ULONG)sli,
    AR_Text,            (ULONG)"Set the direction and angle you wish to shear to.\n"
                        "Negative angle means shear to counterclockwise",
    AR_HelpNode,        (ULONG)"effects.guide/Shear",
    TAG_DONE
};

FRAME *DoShearHoriz( FRAME *frame, LONG amount, struct PPTBase *PPTBase )
{
    FRAME *new;
    WORD comps = frame->pix->components;

    new = MakeFrame( frame );

    /*
     *  Calculate new frame size
     */

    new->pix->width = frame->pix->width + abs( amount );

    InitProgress( frame, "Shearing horizontally...", 0, new->pix->height );

    if( InitFrame( new ) == PERR_OK ) {
        WORD startcol, row;

        if( amount < 0 ) {
            startcol = 0;
        } else {
            startcol = amount;
        }

        for( row = 0; row < new->pix->height; row++ ) {
            ROWPTR cp, dcp;
            LONG offset;

            if( Progress( frame, row ) ) {
                SetErrorCode( frame, PERR_BREAK );
                RemFrame( new );
                return NULL;
            }

            cp = GetPixelRow( frame, row );
            dcp = GetPixelRow( new, row );

            offset = amount * row / new->pix->height;

            memset( dcp, 0, (startcol-offset)*comps );
            memcpy( dcp+(startcol-offset)*comps, cp, frame->pix->bytes_per_row );
            memset( dcp+(startcol-offset)*comps+frame->pix->bytes_per_row,
                    0,
                    (abs(amount)-(startcol-offset)) * comps );

            PutPixelRow( new, row, dcp );
        }

        FinishProgress( frame );
    } else {
        RemFrame( new );
        new = NULL;
    }

    return new;
}

FRAME *DoShearVert( FRAME *frame, LONG amount, struct PPTBase *PPTBase )
{
    FRAME *new;
    UBYTE bg[32] = {0};

    new = MakeFrame( frame );

    /*
     *  Calculate new frame size
     */

    new->pix->height = frame->pix->height + abs( amount );

    InitProgress( frame, "Shearing vertically...", 0, new->pix->height );

    if( InitFrame( new ) == PERR_OK ) {
        WORD width, comps;
        WORD row;

        width = new->pix->width;
        comps = new->pix->components * (new->pix->bits_per_component>>3);

        for( row = 0; row < new->pix->height; row++ ) {
            ROWPTR cp, dcp;
            WORD col;

            if( Progress( frame, row ) ) {
                SetErrorCode( frame, PERR_BREAK );
                RemFrame( new );
                return NULL;
            }

            dcp = GetPixelRow( new, row );

            for( col = 0; col < width; col++ ) {
                LONG y, x;

                if( amount < 0 ) {
                    y = row + amount * col / width;
                    x = width - col - 1;
                } else {
                    y = row - amount * col / width;
                    x = col;
                }

                if( y < 0 || y >= (LONG)frame->pix->height ) {
                    cp = bg;
                } else {
                    cp = GetPixelRow( frame, y );
                    cp += x*comps;
                }
                memcpy( dcp + (x*comps), cp, comps );
            }

            PutPixelRow( new, row, dcp );
        }

    } else {
        RemFrame( new );
        new = NULL;
    }

    return new;
}

PERROR
ParseRexxArgs( FRAME *frame, ULONG *args, struct Values *v, struct PPTBase *PPTBase )
{
    if( args[0] && args[1] ) {
        SetErrorMsg( frame, "Cannot shear in both directions. Use Rotate instead!" );
        return PERR_ERROR;
    }

    if( args[0] )
        v->dir = HORIZONTAL;
    else
        v->dir = VERTICAL;

    v->amount = (int) (* ((ULONG *)args[2]) );
    if( v->amount < -89 || v->amount > 89 ) {
        SetErrorCode( frame, PERR_INVALIDARGS );
        return PERR_ERROR;
    }

    return PERR_OK;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    PERROR res = PERR_OK;
    FRAME *newframe = NULL;
    ULONG *args;
    double angle;
    struct Values v = {0}, *opt;
    int numpixels;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    if( args = (ULONG *) TagData( PPTX_RexxArgs, tags ) ) {
        if( ParseRexxArgs( frame, args, &v, PPTBase ) != PERR_OK ) {
            return NULL; // They already took care of error reporting
        }
    } else {
        cyc[0].ti_Data = v.dir;
        sli[0].ti_Data = v.amount;

        cyc[1].ti_Data = (ULONG)&v.dir;
        sli[1].ti_Data = (ULONG)&v.amount;

        if( (res = AskReqA( frame, win )) != PERR_OK ) {
            SetErrorCode( frame, res );
            return NULL;
        }
    }

    /*
     *  Convert from degrees to pixels and do the shear.
     */

    angle = PI * ( ((double)v.amount)/180.0);

    if( v.dir == HORIZONTAL ) {
        numpixels = (LONG) (tan( angle ) * (double) frame->pix->height);
        newframe = DoShearHoriz( frame, numpixels, PPTBase );
    } else {
        numpixels = (LONG) (tan( angle ) * (double) frame->pix->width);
        newframe = DoShearVert( frame, numpixels, PPTBase );
    }

    if( newframe ) {
        PutOptions( MYNAME, &v, sizeof(v) );
    }

    return newframe;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    PERROR res = PERR_OK;
    ULONG *args;
    struct Values v = {0}, *opt;
    STRPTR buffer;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    if( args = (ULONG *) TagData( PPTX_RexxArgs, tags ) ) {
        if( ParseRexxArgs( frame, args, &v, PPTBase ) != PERR_OK ) {
            return PERR_INVALIDARGS; // They already took care of error reporting
        }
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    cyc[0].ti_Data = v.dir;
    sli[0].ti_Data = v.amount;

    cyc[1].ti_Data = (ULONG)&v.dir;
    sli[1].ti_Data = (ULONG)&v.amount;

    if( (res = AskReqA( frame, win )) != PERR_OK ) {
        SetErrorCode( frame, res );
        return res;
    }

    if( v.dir == HORIZONTAL ) {
        SPrintF( buffer, "HORIZONTAL %d", v.amount );
    } else {
        SPrintF( buffer, "VERTICAL %d", v.amount );
    }

    return PERR_OK;
}




/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

