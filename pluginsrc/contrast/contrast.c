/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : contrast

    PPT is (C) Janne Jalkanen 1995-1997.

    $Id: contrast.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $

*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

#include <stdlib.h>
#include "contrast.h"

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "Contrast"



/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables */

const struct TagItem MyTagArray[] = {
    PPTX_Name,          (ULONG)MYNAME,
    /* Other tags go here */
    PPTX_Author,        (ULONG)"Janne Jalkanen, 1996-1999",
    PPTX_InfoTxt,       (ULONG)"Provides contrast correction for the image",
    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,
    PPTX_RexxTemplate,  (ULONG)"AMOUNT",
    PPTX_ReqPPTVersion, 4,
    PPTX_SupportsGetArgs, TRUE,
    TAG_END, 0L
};

struct Library *MathIeeeDoubBasBase = NULL;

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#include <dos.h>
#endif

#if defined( __GNUC__ )
const BYTE LibName[]="contrast.effect";
const BYTE LibIdString[]="contrast.effect 1.6";
const UWORD LibVersion=1;
const UWORD LibRevision=6;
ADD2LIST(LIBEffectInquire,__FuncTable__,22);
ADD2LIST(LIBEffectExec,__FuncTable__,22);
/* The following two definitions are only required
   if you link with libinitr */
ADD2LIST(LIBEffectInquire,__LibTable__,22);
ADD2LIST(LIBEffectExec,__LibTable__,22);

/* Make GNU C specific declarations. __UtilityBase is
   required for the libnix integer multiplication */
struct ExecBase *SysBase = NULL;
struct Library *__UtilityBase = NULL;
#endif

/*
    These build the histogram transformation tables required.
*/
void Contrast( FRAME *frame, int correction, UBYTE *dest, struct PPTBase *PPTBase )
{
    int i;
    float c, dx, dy, b;

    c = correction * 128 / 1000.0;

    if( correction <= 0 ) {
        c  = -c;
        dx = 256.0;
        dy = 256.0 - 2.0 * c;
        b  = c;
    } else {
        dx = 256.0 - 2.0 * c;
        if( dx == 0.0 ) dx = 1.0;
        dy = 256.0;
        b  = 128.0 - (256.0*128.0)/dx;
    }

    for(i = 0; i < 256; i++) {
        float val;
        val = (dy * i) / dx + b;
        if(val > 255.0) val = 255.0; else if(val < 0.0) val = 0.0;
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
int DoModify( FRAME *frame, ULONG which, int howmuch, struct PPTBase *PPTBase )
{
    UWORD row;
    UBYTE T[256];
    int res = PERR_OK;
    WORD pixelsize = frame->pix->components;
    BOOL alpha = 0;

    /*
     *  Select the transform function.
     */

    Contrast( frame, howmuch, T, PPTBase );

    DumpTable( T );

    InitProgress( frame, "Correcting contrast...", frame->selbox.MinY, frame->selbox.MaxY );

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

    PUTDS( (long) hook->h_Data );

    DoModify( msg->aum_Frame, 0, msg->aum_Values[0], msg->aum_PPTBase );

    return ARR_REDRAW;
}


LIBINIT
{
#if defined(__GNUC__)
    SysBase = SYSBASE();

    if( NULL == (__UtilityBase = OpenLibrary("utility.library",37L))) {
        return 1L;
    }
    __initlibraries();

#endif
    if(MathIeeeDoubBasBase = OpenLibrary( "mathieeedoubbas.library", 0L ) ) {
        return 0;
    }
    return 1;
}

LIBCLEANUP
{
    if(MathIeeeDoubBasBase) CloseLibrary( MathIeeeDoubBasBase );
#if defined(__GNUC__)
    __exitlibraries();
    if( __UtilityBase ) CloseLibrary(__UtilityBase);
#endif
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

PERROR
ParseRexxArgs(ULONG *args, int *contrast, struct PPTBase *PPTBase )
{
    if( args[0] )
        *contrast = atof( (STRPTR)args[0] ) * 1000;
    else
        *contrast = 0;

    return PERR_OK;
}

PERROR DoGUI( FRAME *frame, int *contrast, struct PPTBase *PPTBase )
{
    struct Hook pwhook = { {0}, MyHookFunc, 0L, NULL };
    struct TagItem bright[] = { AROBJ_Value, NULL,
                                ARFLOAT_Min, -1000,
                                ARFLOAT_Default, 0,
                                ARFLOAT_Max, 1000,
                                AROBJ_PreviewHook, NULL,
                                ARFLOAT_Divisor, 1000,
                                ARFLOAT_FormatString, (ULONG) "%.4f",
                                TAG_END };

    struct TagItem win[] = { AR_FloatObject, NULL,
                             AR_Text, (ULONG)"Choose contrast change",
                             AR_HelpNode, (ULONG)"effects.guide/Contrast",
                             TAG_END };
    FRAME *pwframe;
    PERROR res;

    pwhook.h_Data = (void *)GETDS(); // Save register

    bright[0].ti_Data = (ULONG) contrast;
    bright[2].ti_Data = (ULONG) *contrast;
    bright[4].ti_Data = (ULONG) &pwhook;
    win[0].ti_Data = (ULONG) bright;

    pwframe = ObtainPreviewFrame( frame, NULL );

    if( ( res = AskReqA( frame, win )) != PERR_OK) {
        SetErrorCode( frame, res );
        if( pwframe ) ReleasePreviewFrame( pwframe );
        return NULL;
    }
    if( pwframe ) ReleasePreviewFrame( pwframe );

    return res;
}


EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    int res;
    int contrast = 0;
    ULONG *args;
    int *dta;

    D(bug(MYNAME": Exec()\n"));

    if( dta = (int *)GetOptions(MYNAME) ) {
        contrast = *dta;
    }

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );

    /*
     *  Check if we got the brightness?
     */

    if( args ) {
        ParseRexxArgs( args, &contrast, PPTBase );
    } else {
        res = DoGUI( frame, &contrast, PPTBase );
    }

    /*
     *  Check that the argument is correct and call the modify routine.
     */

    if( contrast < -1000 || contrast > 1000 ) {
        SetErrorCode( frame, PERR_INVALIDARGS );
        return NULL;
    } else {
        DoModify( frame, 0, contrast, PPTBase );
    }

    PutOptions(MYNAME,&contrast,sizeof(int));

    return frame;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    PERROR res;
    ULONG *args;
    STRPTR buffer;
    int contrast = 0, *dta;

    if( dta = (int *)GetOptions(MYNAME) ) {
        contrast = *dta;
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *) TagData( PPTX_RexxArgs, tags ) ) {
        res = ParseRexxArgs( args, &contrast, PPTBase );
    }

    res = DoGUI( frame, &contrast, PPTBase );

    if( res == PERR_OK ) {
        SPrintF( buffer, "AMOUNT %f", contrast/1000.0f );
    }

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

