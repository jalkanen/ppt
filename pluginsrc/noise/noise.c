/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : noise

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    $Id: noise.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <string.h>
#include <stdlib.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Noise"


/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Types */

typedef enum {
    Additive,
    Multiplicative,
    SaltPepper
} Noise_T;

struct Values {
    Noise_T     type;
    long        amount;
    long        freq;
};

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Adds noise to the image.\n";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name,  (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,    (ULONG)"Janne Jalkanen 1996-2000",
    PPTX_InfoTxt,   (ULONG)infoblurb,

    PPTX_ColorSpaces, CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,

    PPTX_RexxTemplate, (ULONG)"AMOUNT/N/A,FREQUENCY=FREQ/N,TYPE/K",

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

const char *names[] = {
    "Additive",
    "Multiplicative",
    "Salt-n-Pepper",
    NULL
};

struct TagItem num[] = {
    ARSLIDER_Default,   0,
    AROBJ_Value,        (ULONG)NULL,
    ARSLIDER_Min,       0,
    ARSLIDER_Max,       100,
    AROBJ_Label,        (ULONG)"%",
    TAG_DONE
};

struct TagItem cyc[] = {
    ARCYCLE_Active,     0,
    AROBJ_Value,        (ULONG)NULL,
    ARCYCLE_Labels,     (ULONG)names,
    ARCYCLE_Popup,      TRUE,
    TAG_DONE
};

struct TagItem fr[] = {
    ARSLIDER_Default,   50,
    AROBJ_Value,        (ULONG)NULL,
    ARSLIDER_Min,       0,
    ARSLIDER_Max,       100,
    AROBJ_Label,        (ULONG)"Freq",
    TAG_DONE
};

struct TagItem win[] = {
    AR_Text, (ULONG)ISEQ_C"Please select the percentage,\n"
             ISEQ_C"frequency and the type of noise.",
    AR_SliderObject, (ULONG) num,
    AR_SliderObject, (ULONG) fr,
    AR_CycleObject,  (ULONG) cyc,
    AR_HelpNode,     (ULONG) "effects.guide/Noise",
    TAG_DONE
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

PERROR
ParseRexxArgs(FRAME *frame, ULONG *args, struct Values *v, struct PPTBase *PPTBase )
{
    int type;

    v->amount = *((LONG *)args[0]);

    if( args[1] )
        v->freq = *((LONG *) args[1]);

    if( args[2] ) {
        for( type = 0; names[type]; type++ )
            if( stricmp( (STRPTR)args[2], names[type] ) == 0 ) break;

        if( names[type] == NULL ) {
            SetErrorMsg(frame,"Invalid noise type!");
            return PERR_INVALIDARGS;
        }

        v->type = type;
    }

    return PERR_OK;
}


EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    ULONG *args, sec, ms;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    PERROR res = PERR_OK;
    struct Values v = {Additive,0,50}, *opt;

    /*
     *  Initialize random number generator and tables.
     */

    CurrentTime( &sec, &ms );
    srand( (unsigned int) ms );

    if( opt = GetOptions(MYNAME) ) {
        v = *opt;
    }

    cyc[0].ti_Data = v.type;
    num[0].ti_Data = v.amount;
    fr[0].ti_Data  = v.freq;

    cyc[1].ti_Data = (ULONG)&v.type;
    num[1].ti_Data = (ULONG)&v.amount;
    fr[1].ti_Data  = (ULONG)&v.freq;

    /*
     *  Check for REXX stuff.
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( (res = ParseRexxArgs( frame, args, &v, PPTBase )) != PERR_OK )
            return NULL;
    } else {
        res = AskReqA(frame, win);
    }


    /*
     *  Ask for values and then noisify.  A really strange word, eh?
     */

    if( res == PERR_OK ) {
        UWORD row, col;
        UWORD colors = frame->pix->components;
        long  freq;

        freq = v.freq * (RAND_MAX / 100);

        InitProgress( frame, "Making noise...", frame->selbox.MinY, frame->selbox.MaxY );

        for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
            ROWPTR cp;
            UBYTE  *xp;

            cp = GetPixelRow( frame, row );

            if( Progress( frame, row ) ) {
                SetErrorCode( frame, PERR_BREAK );
                return NULL;
            }

            for( col = frame->selbox.MinX, xp = cp+colors*frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
                int color;

                if( frame->pix->colorspace == CS_ARGB ) {
                    xp++;
                    color = 1;
                } else {
                    color = 0;
                }

                while( color < colors ) {
                    long r,f; // Make sure you're not compiling this with shortints...
                    int  a = 0;

                    f = rand();
                    if( f < freq ) {
                        r = rand();
                        switch(v.type) {
                            case Additive:
                                if( r & 1 ) { // Make decision based on the last bit.
                                    a = (LONG)*xp + 255 * v.amount * (r/100000) / (RAND_MAX/1000);
                                } else {
                                    a = (LONG)*xp - 255 * v.amount * (r/100000) / (RAND_MAX/1000);
                                }
                                break;

                            case Multiplicative:
                                if( r & 1 )
                                    a = (LONG)*xp - *xp * v.amount * (r/100000) / (RAND_MAX/1000);
                                else
                                    a = (LONG)*xp + *xp * v.amount * (r/100000) / (RAND_MAX/1000);
                                break;

                            case SaltPepper:
                                if( r & 1 )
                                    a = 255;
                                else
                                    a = 0;
                                break;
                        }

                        /*
                         *  Clamp and write back.
                         */

                        if( a > 255 ) a = 255; else if(a < 0) a = 0;

                        *xp++ = a;

                    } else {
                        xp++;
                    }
                    color++;
                } /* while() */
            }

            PutPixelRow( frame, row, cp );
        }

        FinishProgress( frame );
    }

    PutOptions( MYNAME, &v, sizeof(struct Values) );

    return frame;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    PERROR res = PERR_OK;
    struct Values v = {Additive,0,50}, *opt;
    STRPTR buffer;

    if( opt = GetOptions(MYNAME) ) {
        v = *opt;
    }

    cyc[0].ti_Data = v.type;
    num[0].ti_Data = v.amount;
    fr[0].ti_Data  = v.freq;

    cyc[1].ti_Data = (ULONG)&v.type;
    num[1].ti_Data = (ULONG)&v.amount;
    fr[1].ti_Data  = (ULONG)&v.freq;

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( (res = ParseRexxArgs( frame, args, &v, PPTBase )) != PERR_OK )
            return res;
    }

    buffer = (STRPTR)TagData( PPTX_ArgBuffer, tags );

    res = AskReqA(frame, win);

    if( res == PERR_OK ) {
    PPTX_RexxTemplate, (ULONG)"AMOUNT/N/A,FREQUENCY=FREQ/N,TYPE/K",
        SPrintF( buffer, "AMOUNT %d FREQUENCY %d TYPE %s",
                         v.amount, v.freq, names[v.type] );
    }

    PutOptions( MYNAME, &v, sizeof(struct Values) );

    return PERR_OK;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

