/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : histogram equalization

    (C) Janne Jalkanen 1995-2000

    $Id: histeq.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

#include "histeq.h"

/*----------------------------------------------------------------------*/
/* Defines */

/* Remove comment markings from the following line if you want
   a debug-version. */

/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables */

const char blurb[] =
    "This filter equalizes histograms of\n"
    "all components in a picture separately.\n"
    "It is possible to do either local or global\n"
    "histogram equalization.";

const struct TagItem MyTagArray[] = {
    PPTX_Name,              (ULONG)MYNAME,
    /* Other tags go here */
    PPTX_Author,            (ULONG)"Janne Jalkanen 1996-2000",
    PPTX_InfoTxt,           (ULONG)blurb,
    PPTX_ColorSpaces,       CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,
    PPTX_NoNewFrame,        TRUE,
    PPTX_RexxTemplate,      (ULONG)"LOCALRADIUS/N",
    PPTX_ReqPPTVersion,     3,

    PPTX_SupportsGetArgs,   TRUE,
#ifdef _M68020
    PPTX_CPU,               AFF_68020,
#endif
    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
/* Code */

#ifndef __SASC
void __regargs _CXBRK(void) {}
#ifndef __PPC__
void __regargs _chkabort(void) {}
#endif
#endif

#ifdef __PPC__
#include <powerup/ppclib/interface.h>
// #include <powerup/pragmas/ppc_pragmas.h>

struct TagItem *__MyTagArray = MyTagArray;
void *__LIBEffectExec = EffectExec;
void *__LIBEffectInquire = EffectExec;

extern _m68kDoMethodA();

__inline ULONG DoMethodA( Object *obj, Msg msg )
{
    struct Caos c;

    c.caos_Un.Function = (APTR)_m68kDoMethodA;
    c.M68kCacheMode = IF_CACHEFLUSHALL;
    c.PPCCacheMode  = IF_CACHEFLUSHALL;
    c.a0 = (ULONG)obj;
    c.a1 = (ULONG)msg;

    return PPCCallM68k( &c );
}

ULONG DoMethod(Object *obj, ULONG MethodID, ... )
{
    return DoMethodA( obj, (Msg)&MethodID );
}

#endif


#ifdef DEBUG_MODE
void ShowHistograms( ULONG *n )
{
    int i;
    PDebug("*****\n");
    for(i = 0; i<256;i++) {
        PDebug("%3d:%6lu | ",i,n[i]);
        if(i % 5 == 4)
            PDebug("\n");
    }
    PDebug("\n");
}
#endif

/*
    Calculates equalized histograms.

    BUG: Requires a lot of optimizing.
*/
void EqualizeThem( FRAME *frame, ULONG *n, ULONG N, struct PPTBase *PPTBase )
{
    int i;
    ULONG cum = 0; /* Cumulative effect. */

#ifdef DEBUG_MODE
    PDebug("Equalizethem()\n");

    ShowHistograms(n);
#endif

    for(i = 0; i < 256; i++) {
        cum += n[i];
        n[i] = cum;
    }

#ifdef DEBUG_MODE
    if(cum != N)
        PDebug("Warning: cum = %ld\n",cum);
#endif

    /* n[i] on suhteellisen histogrammin kertymäfunktio (s_k). Nyt mäpätään
       se käytettävissä oleville harmaatasoille r_k */

    for(i = 0; i < 256; i++) {
        n[i] = ((n[i] << 12)/N)>>4;
        if(n[i] > 255) n[i] = 255;
    }
#ifdef DEBUG_MODE
    ShowHistograms(n);
#endif
}

FRAME *EqualizeLocal( FRAME *frame, ULONG **hg, WORD radius, struct PPTBase *PPTBase )
{
    WORD row,comps = frame->pix->components;
    WORD color, height, width;
    FRAME *newframe;
    ULONG N = (radius*2+1)*(radius*2+1);
    BOOL alpha = 0;

    height = frame->pix->height;
    width  = frame->pix->width;

    /*
     *  Data is copied so unused pixels/channels are not affected
     */

    newframe = DupFrame( frame, DFF_COPYDATA );
    if(!newframe) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return NULL;
    }

    InitProgress( frame, "Equalizing local histograms...", frame->selbox.MinY, frame->selbox.MaxY );

    if( frame->pix->colorspace == CS_ARGB ) alpha = 1;

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        ROWPTR dcp, qcp;
        WORD col;

        /* Clear hg array */

        for( color = 0; color < comps-alpha; color++ )
            bzero( hg[color], 256 * sizeof(ULONG) );

        if(Progress( frame, row) ) {
            RemFrame( newframe );
            return NULL;
        }

        dcp = GetPixelRow( newframe, row );

        qcp = dcp + (comps*frame->selbox.MinX);

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
            WORD x, y;

            for( y = -radius; y < radius; y++ ) {
                ROWPTR cp;
                WORD ry;

                ry = row + y;
                if( ry < 0 ) ry = 0; else if( ry >= height ) ry = height-1;

                cp = GetPixelRow( frame, ry );

                for( x = -radius; x < radius; x++ ) {
                    LONG rx;
                    ROWPTR scp;

                    rx = col + x;
                    if( rx < 0 ) rx = 0; else if( rx > width ) rx = width-1;

                    scp = cp + rx*comps;

                    if(alpha) scp++;

                    for( color = 0; color < comps-alpha; color++ ) {
                        hg[color][ *scp++ ]++;
                    }
                }

            }

            /*
             *  Equalization
             */

            for( color = 0; color < comps-alpha; color++ ) {
                D(bug("Equalizing color %d\n",color));
                EqualizeThem( frame, hg[color], N, PPTBase );
            }

            /*
             *  Write them back.
             */

            if(alpha) qcp++;

            for( color = 0; color < comps-alpha; color++, qcp++ ) {
                *qcp = (UBYTE)hg[color][*qcp];
            } /* color */

        }

        PutPixelRow( newframe, row, dcp );
    }

    return newframe;
}

FRAME *EqualizeGlobal( FRAME *frame, ULONG **hg, struct PPTBase *PPTBase )
{
    WORD row,comps = frame->pix->components;
    ULONG xbegin = (ULONG)(frame->pix->components * frame->selbox.MinX);
    WORD color;
    FRAME *newframe;
    ULONG N = frame->pix->width * frame->pix->height;
    BOOL alpha = 0;

    newframe = DupFrame(frame, DFF_COPYDATA);
    if(!newframe) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return NULL;
    }

    InitProgress( frame, "Building histograms...", frame->selbox.MinY, frame->selbox.MaxY );

    if( frame->pix->colorspace == CS_ARGB ) alpha = 1;

    /*
     *  Builds histograms.
     */

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        UBYTE *cp, *scp;
        UWORD col;

        cp = GetPixelRow( frame, row );
        scp = cp + xbegin;

        if(Progress( frame, row) ) {
            RemFrame( newframe );
            return NULL;
        }

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {

            if( alpha ) scp++; /* Skip alpha channel */

            for( color = 0; color < comps-alpha; color++ ) {
                hg[color][ *scp++ ]++;
            } /* color */
        } /* col */
    } /* row */

    /*
     *  Equalize
     */

    for( color = 0; color < comps-alpha; color++ ) {
#ifdef DEBUG_MODE
        PDebug("Equalizing color %d\n",color);
#endif
        EqualizeThem( frame, hg[color], N, PPTBase );
    }

    /*
     *  Write them back
     */

    InitProgress( frame, "Remapping picture...", frame->selbox.MinY, frame->selbox.MaxY );

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        ROWPTR cp,scp;
        UWORD col;

        cp = GetPixelRow( newframe, row );
        scp = cp + xbegin;

        if(Progress( frame, row ) ) {
            RemFrame( newframe );
            return NULL;
        }

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
            if( alpha ) scp++;
            for( color = 0; color < comps-alpha; color++,scp++ ) {
                *scp = (UBYTE)hg[color][*scp];
            } /* color */
        } /* col */

        PutPixelRow( newframe, row, cp );
    } /* row */

    return newframe;
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

VOID SetDefaults(struct Values *v, struct PPTBase *PPTBase)
{
    struct Values *saved;

    v->method = 0;
    v->radius = 5;

    if( saved = GetOptions(MYNAME) )
        *v = *saved;

}

PERROR GetRexxArgs(struct Values *v, ULONG *args, struct PPTBase *PPTBase)
{
    if( args[0] ) {
        v->method = 1;
        v->radius = *( (ULONG *)args[0]);
    } else {
        v->method = 0;
        v->radius = 0;
    }

    if( v->method == 1 && (v->radius < 0 || v->radius > MAX_RADIUS) ) {
        return PERR_INVALIDARGS;
    }

    return PERR_OK;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    struct Values v = {0};
    STRPTR buffer;
    PERROR res;

    SetDefaults(&v, PPTBase);

    buffer = (STRPTR)TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( GetRexxArgs(&v,args,PPTBase) != PERR_OK ) {
            SetErrorCode(frame,PERR_INVALIDARGS);
            return PERR_ERROR;
        }
    }

    if( (res = GetValues( frame, &v, PPTBase )) == PERR_OK ) {
        if( v.method == 1 ) {
            SPrintF(buffer,"LOCALRADIUS %d", v.radius);
        }
    }

    return res;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe = NULL;
    UBYTE color;
    ULONG *hg[6] = {0L}; /* BUG: Magic */
    WORD comps = frame->pix->components;
    ULONG *args;
    struct Values v = {0};

#ifdef DEBUG_MODE
    PDebug(MYNAME": Exec()\n");
#endif

    SetDefaults(&v, PPTBase);

    /*
     *  Check REXX
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( GetRexxArgs(&v,args,PPTBase) != PERR_OK ) {
            SetErrorCode(frame,PERR_INVALIDARGS);
            return NULL;
        }
    } else {
        if( GetValues( frame, &v, PPTBase ) != PERR_OK )
            return NULL;
    }

    /*
     *  Allocate histograms.
     *  BUG: Does not release them.
     */

    for( color = 0; color < comps; color++ ) {
        hg[color] = AllocVec( 256 * sizeof(ULONG), MEMF_CLEAR );
        if(hg[color] == NULL) {
            SetErrorCode( frame, PERR_OUTOFMEMORY );
            return NULL;
        }
    }

    if( v.method == 0 )
        newframe = EqualizeGlobal( frame, hg, PPTBase );
    else
        newframe = EqualizeLocal( frame, hg, (WORD)v.radius, PPTBase );

    PutOptions(MYNAME, &v, sizeof(struct Values) );

    /*
    *  Free resources
    */

    for( color = 0; color < frame->pix->components; color++ ) {
        FreeVec( hg[color] );
    }

    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

