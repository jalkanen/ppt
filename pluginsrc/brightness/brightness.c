/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : brightness

    PPT is (C) Janne Jalkanen 1995.

    $Id: brightness.c,v 1.1 2001/10/25 16:22:59 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

#include <pptplugin.h>

#include "brightness.h"
#include <stdlib.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "Brightness"

#define PREVIEW_MODE

/*----------------------------------------------------------------------*/
/* Internal prototypes */

/*----------------------------------------------------------------------*/
/* Global variables */

const struct TagItem MyTagArray[] = {
    PPTX_Name,      (ULONG)MYNAME,
    /* Other tags go here */
    PPTX_Author,    (ULONG)"Janne Jalkanen, 1996-1999",
    PPTX_InfoTxt,   (ULONG)"Provides brightness correction for the image",
    PPTX_ColorSpaces, CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,
    PPTX_RexxTemplate, (ULONG)"AMOUNT",
    PPTX_ReqPPTVersion, 4,
    PPTX_SupportsGetArgs, TRUE,
    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

/*
    These build the histogram transformation tables required.
*/
void Brightness( FRAME *frame, int correction, UBYTE *dest, struct PPTBase *PPTBase )
{
    int i;

    for(i = 0; i < 256; i++) {
        int val;
        val = i * (correction+1000) / 1000;
        if(val > 255) val = 255;
        dest[i] = (UBYTE)val;
    }
}

#ifdef DEBUG_MODE
void DumpTable( UBYTE *table )
{
    int i;

    for(i = 0; i < 256; i++) {
        PDebug("%3d -> %3d     ",i,table[i]);
        if(i % 4 == 3) PDebug("\n");
    }
}
#else
#define DumpTable(x)
#endif

/*
    Main dispatcher
*/
PERROR DoModify( FRAME *frame, ULONG which, int howmuch, struct PPTBase *PPTBase )
{
    UWORD row;
    UBYTE T[256];
    int res = PERR_OK;
    WORD pixelsize = frame->pix->components;
    BOOL alpha = 0;

    /*
     *  Select the transform function.
     */

    Brightness( frame, howmuch, T, PPTBase );

    DumpTable( T );

    InitProgress( frame, "Modifying histograms...", frame->selbox.MinY, frame->selbox.MaxY );

    if( frame->pix->colorspace == CS_ARGB )
        alpha = 1;

    for(row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        ROWPTR cp, dcp;
        WORD col;

        cp = GetPixelRow( frame, row );
        dcp = cp + (frame->selbox.MinX) * pixelsize;
        if(Progress(frame,row)) {
            res = PERR_BREAK;
            goto quit;
        }

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++) {
            UBYTE comp;

            if( alpha ) dcp++;

            for(comp = 0; comp < pixelsize-alpha; comp++) {
                *dcp = T[*dcp];
                dcp++;
            }
        }

        PutPixelRow( frame, row, cp );
    }
quit:
    return res;
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

#ifdef PREVIEW_MODE
ASM ULONG MyHookFunc( REG(a0) struct Hook *hook,
                      REG(a2) Object *obj,
                      REG(a1) struct ARUpdateMsg *msg )
{
    /*
     *  Set up A4 so that globals still work
     */

    PUTREG(REG_A4, (long) hook->h_Data);

    D(bug("MODIFY!  Frame %08X, value=%ld\n",msg->aum_Frame,msg->aum_Values[0]));

    DoModify( msg->aum_Frame, 0, msg->aum_Values[0], msg->aum_PPTBase );

    return ARR_REDRAW;
}
#endif

LONG brightness = 0;

#ifdef PREVIEW_MODE
struct Hook pwhook = { {0}, MyHookFunc, 0L, NULL };
#endif

struct TagItem bright[] = { ARFLOAT_Min, -1000, ARFLOAT_Default, 0,
#ifdef PREVIEW_MODE
                            AROBJ_PreviewHook, (ULONG)&pwhook,
#endif
                            ARFLOAT_Max, 1000,
                            AROBJ_Value, (ULONG)&brightness,
                            ARFLOAT_Divisor, 1000,
                            ARFLOAT_FormatString, (ULONG)"%.4f",
                            TAG_END };

struct TagItem win[] = { AR_Text, (ULONG)"\nSet the new brightness\n",
                         AR_FloatObject, (ULONG)bright,
                         AR_HelpNode, (ULONG)"effects.guide/Brightness",
                         TAG_END };

PERROR
ParseRexxArgs(ULONG *args, struct Values *v, struct PPTBase *PPTBase )
{
    if( args[0] )
        v->brightness = atof( (STRPTR)args[0] ) * 1000;
    else
        v->brightness = 0;

    return PERR_OK;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    PERROR res;
    ULONG *args;
    struct Values v = {0}, *opt;

    D(bug(MYNAME": Exec()\n"));

#ifdef PREVIEW_MODE
    pwhook.h_Data = (void *)GETREG(REG_A4); // Save register
#endif

    if(opt = GetOptions(MYNAME) )
        v = *opt;

    args = (ULONG *) TagData( PPTX_RexxArgs, tags );

    if( args ) {
        res = ParseRexxArgs( args, &v, PPTBase );
    } else {
        FRAME *pwframe;
        bright[1].ti_Data = v.brightness;

#ifdef PREVIEW_MODE
        pwframe = ObtainPreviewFrame( frame, NULL );
#endif

        if( ( res = AskReqA( frame, win )) != PERR_OK) {
            SetErrorCode( frame, res );
#ifdef PREVIEW_MODE
            if(pwframe) ReleasePreviewFrame( pwframe );
#endif
            return NULL;
        }
        v.brightness = brightness;

#ifdef PREVIEW_MODE
        if(pwframe) ReleasePreviewFrame( pwframe );
#endif
    }

    if( v.brightness < -1000 || v.brightness > 1000 ) {
        SetErrorCode( frame, PERR_INVALIDARGS );
        return NULL;
    } else {
        DoModify( frame, 0, v.brightness, PPTBase );
    }

    PutOptions(MYNAME, &v, sizeof(struct Values));

    return frame;
}


EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    PERROR res;
    ULONG *args;
    STRPTR buffer;

    struct Values v = {0}, *opt;

    if(opt = GetOptions(MYNAME) )
        v = *opt;

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *) TagData( PPTX_RexxArgs, tags ) ) {
        res = ParseRexxArgs( args, &v, PPTBase );
    }

    bright[1].ti_Data = v.brightness;

    res = AskReqA( frame, win );

    if( res == PERR_OK ) {
        SPrintF( buffer, "AMOUNT %f", brightness/1000.0f );
    }

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

