/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : gamma

    PPT is (C) Janne Jalkanen 1995-1999.

    $Id: gamma.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $

*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#undef  DEBUG_MODE

#include <pptplugin.h>

#include <math.h>
#include <stdlib.h>
#include <dos.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "Gamma"

#define DIVISOR 1000
#define DEFAULT_GAMMA 1.0

/*----------------------------------------------------------------------*/
/* Internal prototypes */

struct Values {
    float gamma;
};

/*----------------------------------------------------------------------*/
/* Global variables */

#pragma msg 186 ignore

const struct TagItem MyTagArray[] = {
    PPTX_Name,              MYNAME,
    /* Other tags go here */
    PPTX_Author,            "Janne Jalkanen, 1996-1999",
    PPTX_InfoTxt,           "Provides gamma correction for the image",
    PPTX_ColorSpaces,       CSF_RGB|CSF_ARGB|CSF_GRAYLEVEL,
    PPTX_RexxTemplate,      "AMOUNT/A",
    PPTX_ReqPPTVersion,     4,
    PPTX_SupportsGetArgs,   TRUE,

    TAG_END, 0L
};

struct Library *MathIeeeDoubBasBase, *MathIeeeDoubTransBase;

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
void Gamma( FRAME *frame, float correction, UBYTE *dest, struct PPTBase *PPTBase )
{
    int i;
    double g;

    g = 1.0 / correction;

    for(i = 0; i < 256; i++) {
        int val;

        val = (int) (pow( ( (double)i / 255.0 ), g ) * 255.0 + 0.5);

        if(val > 255) val = 255; else if( val < 0 ) val = 0;
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
int DoModify( FRAME *frame, ULONG which, float gamma, struct PPTBase *PPTBase )
{
    UWORD row;
    UBYTE T[256];
    int res = PERR_OK;
    WORD pixelsize = frame->pix->components;
    BOOL alpha = 0;

    /*
     *  Select the transform function.
     */

    D(bug("Gamma correction is %lf\n", gamma ));

    Gamma( frame, gamma, T, PPTBase );

    DumpTable( T );

    InitProgress( frame, "Gamma correcting...", frame->selbox.MinY, frame->selbox.MaxY );

    if( frame->pix->colorspace == CS_ARGB ) alpha = 1;

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

ASM ULONG MyHookFunc( REG(a0) struct Hook *hook,
                      REG(a2) Object *obj,
                      REG(a1) struct ARUpdateMsg *msg )
{
    /*
     *  Set up A4 so that globals still work
     */

    PUTREG(REG_A4, (long) hook->h_Data);

    D(bug("MODIFY!  Frame %08X, value=%ld\n",msg->aum_Frame,msg->aum_Values[0]));

    DoModify( msg->aum_Frame, 0, (float)(msg->aum_Values[0]/(float)DIVISOR), msg->aum_PPTBase );

    return ARR_REDRAW;
}

LIBINIT
{
    if(MathIeeeDoubBasBase = OpenLibrary( "mathieeedoubbas.library", 0L ) ) {
        if(MathIeeeDoubTransBase = OpenLibrary( "mathieeedoubtrans.library", 0L ) ) {
            return 0;
        }
        CloseLibrary(MathIeeeDoubBasBase);
    }
    return 1;
}


LIBCLEANUP
{
    CloseLibrary( MathIeeeDoubTransBase );
    CloseLibrary( MathIeeeDoubBasBase );
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

struct Hook pwhook = { {0}, MyHookFunc, 0L, NULL };

LONG lgamma;

struct TagItem gama[] = { ARFLOAT_Min,      1,
                          ARFLOAT_Default,  DEFAULT_GAMMA*DIVISOR,
                          ARFLOAT_Max,      10*DIVISOR,
                          ARFLOAT_FormatString, "%.3f",
                          ARFLOAT_Divisor,  DIVISOR,
                          AROBJ_Value,      &lgamma,
                          AROBJ_PreviewHook, &pwhook,
                          TAG_END };

struct TagItem win[] = { AR_Text,           "\nSet the new gamma\n",
                         AR_FloatObject,    gama,
                         AR_HelpNode,       "effects.guide/Brightness",
                         TAG_END };

PERROR
ParseRexxArgs(ULONG *args, struct Values *v, struct PPTBase *PPTBase )
{
    if( args[0] )
        v->gamma = atof( (STRPTR)args[0] );
    else
        v->gamma = 0.0;

    return PERR_OK;
}


EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    int res;
    ULONG *args;
    struct Values v = {DEFAULT_GAMMA}, *opt;

    D(bug(MYNAME": Exec()\n"));

    pwhook.h_Data = (void *)GETREG(REG_A4); // Save register

    if( opt = GetOptions(MYNAME) ) {
        v = *opt;
    }

    args = (ULONG *) TagData( PPTX_RexxArgs, tags );

    if( args ) {
        res = ParseRexxArgs( args, &v, PPTBase );
    } else {
        FRAME *pwframe;

        pwframe = ObtainPreviewFrame( frame, NULL );

        gama[1].ti_Data = (ULONG) (v.gamma * DIVISOR);

        if( ( res = AskReqA( frame, win )) != PERR_OK) {
            SetErrorCode( frame, res );
            if( pwframe ) ReleasePreviewFrame( pwframe );
            return NULL;
        }
        v.gamma = (float)lgamma / (float)DIVISOR;
        if( pwframe ) ReleasePreviewFrame( pwframe );
    }

    if( v.gamma < 1.0/DIVISOR || v.gamma > 10.0 ) {
        SetErrorCode( frame, PERR_INVALIDARGS );
        return NULL;
    } else {
        DoModify( frame, 0, v.gamma, PPTBase );
    }

    PutOptions( MYNAME, &v, sizeof(long) );

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

    gama[1].ti_Data = (ULONG) (v.gamma * DIVISOR);

    res = AskReqA( frame, win );

    if( res == PERR_OK ) {
        SPrintF( buffer, "AMOUNT %f", (float)lgamma/ (float)DIVISOR );
    }

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

