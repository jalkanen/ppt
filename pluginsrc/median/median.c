/*----------------------------------------------------------------------*/
/*
 * PROJECT: ppt effects
 * MODULE : median filtering
 * 
 * PPT and this file are (C) Janne Jalkanen 1995-1998.
 *
 * $Id: median.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
 * 
 */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#undef  DEBUG_MODE

#include <pptplugin.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */


#define MAX_RADIUS  5
#define TABLE_SIZE  ((MAX_RADIUS*2+1) * (MAX_RADIUS*2+1))

/*
 * You should define this to your module name. Try to use something
 * short, as this is the name that is visible in the PPT filter listing.
 */

#define MYNAME      "Median"

struct Values {
    ULONG radius;
};

/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
 * as it may well be that two copies of this same code is run at
 * the same time. */

/*
 * Just a simple string describing this effect.
 */

const char infoblurb[] =
"Performs median filtering of\n"
"a given area with a settable radius\n";

/*
 * This is the global array describing your effect. For a more detailed
 * description on how to interpret and use the tags, see docs/tags.doc.
 */

const struct TagItem MyTagArray[] =
{
     /*
      *  Here are some pretty standard definitions. All filters should have
      *  these defined.
      */

    PPTX_Name,          (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,        (ULONG) "Janne Jalkanen 1996-2000",
    PPTX_InfoTxt,       (ULONG) infoblurb,

    PPTX_RexxTemplate, (ULONG) "RADIUS/N/A",

    PPTX_ColorSpaces,   CSF_RGB | CSF_GRAYLEVEL | CSF_ARGB,

    PPTX_ReqPPTVersion, 3,

    PPTX_SupportsGetArgs,TRUE,

    PPTX_NoNewFrame,    TRUE,
#ifdef _M68020
    PPTX_CPU,           AFF_68020,
#endif
#ifdef __PPC__
    PPTX_CPU,           AFF_PPC,
#endif

    TAG_END, 0L
};

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
#ifndef __PPC__
void __regargs __chkabort(void)
{
}
#endif
void __regargs _CXBRK(void)
{
}

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

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData(attr, MyTagArray);
}
struct TagItem rad[] =
{
    AROBJ_Value, NULL,
    ARSLIDER_Min, 1,
    ARSLIDER_Max, MAX_RADIUS,
    ARSLIDER_Default, 2,
    TAG_DONE
};

struct TagItem win[] =
{
    AR_SliderObject, (ULONG) rad,
    AR_Text, (ULONG) "Please select the radius\nfor median filtering",
    AR_HelpNode, (ULONG) "effects.guide/Median",
    TAG_DONE
};

struct PPTBase *PPTBase;

/*
 * Do gray level filtering.
 * BUG: Quite slow, could use some speedups.
 */
int Do_Gray(FRAME * src, ROWPTR dcp, int nels, LONG radius, WORD row, WORD col, ROWPTR * sscp)
{
    UBYTE gvals[256];
    int i;
    PIXINFO *pix = src->pix;

    memset(gvals, 0, 256);

    D(bug("\nRow:%d, Col:%d\n", row, col));

    for (i = -radius; i <= radius; i++) {
        WORD srow;
        ROWPTR scp;
        int j;

        srow = row + i;
        if (srow < 0) {
            srow = 0;
        } else {
            if (srow >= pix->height)
                srow = pix->height - 1;
        }

        scp = sscp[srow - row + radius];

        for (j = -radius; j <= radius; j++) {
            WORD scol;

            scol = col + j;

            if (scol < 0) {
                scol = 0;
            } else {
                if (scol >= pix->width)
                    scol = pix->width - 1;
            }

            gvals[scp[scol]]++;
        }
    }

    /*
     *  Reorganise pixels.
     */

    D(for (i = 0; i < 256; i++) PDebug("%d,", gvals[i]));
    D(bug("\n"));

    for (i = 0; i < 255; i++) {
        gvals[i + 1] += gvals[i];
    }

    for (i = 0; gvals[i] <= (gvals[255] >> 1); i++);

    *dcp = (UBYTE) i;

    D(bug("Median is %d\n", i));
    D(for (i = 0; i < 256; i++) PDebug("%d,", gvals[i]));
    D(bug("\n"));

    return 1;
}

int Do_ARGB(FRAME * src, ROWPTR dcp, int nels, LONG radius, WORD row, WORD col, ROWPTR * sscp)
{
    UBYTE rvals[256];
    UBYTE gvals[256];
    UBYTE bvals[256];
    int i;
    PIXINFO *pix = src->pix;

    /*
     *  Apply brute force method.  Really should do better...
     */

    memset(rvals, 0, 256);
    memset(gvals, 0, 256);
    memset(bvals, 0, 256);

    for (i = -radius; i <= radius; i++) {
        WORD srow;
        ARGBPixel *scp;
        int j;

        srow = row + i;
        if (srow < 0) {
            srow = 0;
        } else {
            if (srow >= pix->height)
                srow = pix->height - 1;
        }

        scp = (ARGBPixel *) sscp[srow - row + radius];

        for (j = -radius; j <= radius; j++) {
            WORD scol;

            scol = col + j;

            if (scol < 0) {
                scol = 0;
            } else {
                if (scol >= pix->width)
                    scol = pix->width - 1;
            }

            rvals[scp[scol].r]++;
            gvals[scp[scol].g]++;
            bvals[scp[scol].b]++;
        }
    }

    /*
     *  Reorganise pixels.
     */

    for (i = 0; i < 255; i++) {
        rvals[i + 1] += rvals[i];
        gvals[i + 1] += gvals[i];
        bvals[i + 1] += bvals[i];
    }

    /*
     *  Alpha channel is retained
     */

    *dcp++ = ((ARGBPixel *) (sscp[radius]))[col].a;
    for (i = 0; rvals[i] < (rvals[255] >> 1); i++);
    *dcp++ = (UBYTE) i;
    for (i = 0; gvals[i] < (gvals[255] >> 1); i++);
    *dcp++ = (UBYTE) i;
    for (i = 0; bvals[i] < (bvals[255] >> 1); i++);
    *dcp = (UBYTE) i;

    return 4;
}

int Do_RGB(FRAME * src, ROWPTR dcp, int nels, LONG radius, WORD row, WORD col, ROWPTR * sscp)
{
    UBYTE rvals[256];
    UBYTE gvals[256];
    UBYTE bvals[256];
    int i;
    PIXINFO *pix = src->pix;

    /*
     *  Apply brute force method.  Really should do better...
     */

    memset(rvals, 0, 256);
    memset(gvals, 0, 256);
    memset(bvals, 0, 256);

    for (i = -radius; i <= radius; i++) {
        WORD srow;
        ROWPTR scp;
        int j;

        srow = row + i;
        if (srow < 0) {
            srow = 0;
        } else {
            if (srow >= pix->height)
                srow = pix->height - 1;
        }

        scp = sscp[srow - row + radius];

        for (j = -radius; j <= radius; j++) {
            WORD scol;

            scol = col + j;

            if (scol < 0) {
                scol = 0;
            } else {
                if (scol >= pix->width)
                    scol = pix->width - 1;
            }

            scol *= 3;

            rvals[scp[scol++]]++;
            gvals[scp[scol++]]++;
            bvals[scp[scol]]++;
        }
    }

    /*
     *  Reorganise pixels.
     */

    for (i = 0; i < 255; i++) {
        rvals[i + 1] += rvals[i];
        gvals[i + 1] += gvals[i];
        bvals[i + 1] += bvals[i];
    }

    for (i = 0; rvals[i] < (rvals[255] >> 1); i++);
    *dcp++ = (UBYTE) i;
    for (i = 0; gvals[i] < (gvals[255] >> 1); i++);
    *dcp++ = (UBYTE) i;
    for (i = 0; bvals[i] < (bvals[255] >> 1); i++);
    *dcp = (UBYTE) i;

    return 3;
}

FRAME *Median(FRAME * src, LONG radius)
{
    UWORD row, col;
    FRAME *dest;
    int nels;
    UBYTE cspace = src->pix->colorspace;

    nels = radius * 2 + 1;
    nels *= nels;

    D(bug("Median() : radius = %d\n", radius));

    if (dest = DupFrame(src, DFF_COPYDATA)) {

        InitProgress(src, "Median filtering...", src->selbox.MinY, src->selbox.MaxY);

        for (row = src->selbox.MinY; row < src->selbox.MaxY; row++) {
            ROWPTR dcp, odcp, scp[MAX_RADIUS * 2 + 1];

            odcp = GetPixelRow(dest, row);
            GetNPixelRows(src, scp, row - radius, radius * 2 + 1);

            if (Progress(src, row)) {
                RemFrame(dest);
                return NULL;
            }
            dcp = odcp + src->pix->components * src->selbox.MinX;

            for (col = src->selbox.MinX; col < src->selbox.MaxX; col++) {
                switch (cspace) {
                case CS_RGB:
                    dcp += Do_RGB(src, dcp, nels, radius, row, col, scp);
                    break;
                case CS_GRAYLEVEL:
                    dcp += Do_Gray(src, dcp, nels, radius, row, col, scp);
                    break;
                case CS_ARGB:
                    dcp += Do_ARGB(src, dcp, nels, radius, row, col, scp);
                    break;
                }
            }

            PutPixelRow(dest, row, odcp);
        }

        FinishProgress(src);
    }
    return dest;
}

PERROR
ParseRexxArgs( FRAME *frame, ULONG *args, struct Values *v, struct PPTBase *PPTBase )
{
    v->radius = *((LONG *)args[0]);

    return PERR_OK;
}

EFFECTEXEC(frame,tags,lb_PPTBase,EffectBase)
{
    FRAME *newframe;
    ULONG *args;
    struct Values v = {0}, *opt;

    PPTBase = lb_PPTBase;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    if (args = (ULONG *) TagData(PPTX_RexxArgs, tags)) {
        ParseRexxArgs( frame, args, &v, PPTBase );
    } else {
        ULONG res;

        rad[0].ti_Data = (ULONG) & v.radius;
        if ((res = AskReqA(frame, win)) != PERR_OK) {
            SetErrorCode(frame, res);
            return NULL;
        }
    }

    PutOptions(MYNAME, &v, sizeof(struct Values) );

    newframe = Median(frame, v.radius);

    return newframe;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    STRPTR buffer;
    struct Values v = {0}, *opt;
    PERROR res;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    if (args = (ULONG *) TagData(PPTX_RexxArgs, tags))
        ParseRexxArgs( frame, args, &v, PPTBase );

    buffer = (STRPTR)TagData(PPTX_ArgBuffer, tags);

    rad[0].ti_Data = (ULONG) &v.radius;
    if((res=AskReqA(frame,win)) == PERR_OK) {
        SPrintF( buffer, "RADIUS %ld", v.radius );
    }

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/
