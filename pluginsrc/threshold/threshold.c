/*----------------------------------------------------------------------*/
/*
   PROJECT: ppt effects
   MODULE : a thresholding effect.

   PPT and this file are (C) Janne Jalkanen 1995-1997.

   $Id: threshold.c,v 1.1 2001/10/25 16:23:03 jalkanen Exp $
 */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
   You should define this to your module name. Try to use something
   short, as this is the name that is visible in the PPT filter listing.
 */

#define MYNAME      "Threshold"


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
"Provides simple thresholding";

/*
   This is the global array describing your effect. For a more detailed
   description on how to interpret and use the tags, see docs/tags.doc.
 */

const struct TagItem MyTagArray[] =
{

    PPTX_Name, (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author, (ULONG) "Janne Jalkanen",
    PPTX_InfoTxt, (ULONG) infoblurb,

    PPTX_ColorSpaces, CSF_RGB | CSF_GRAYLEVEL | CSF_ARGB,

    PPTX_RexxTemplate, (ULONG) "LIMIT/N/A,INTENSITY/S",

    PPTX_ReqPPTVersion, 4,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

struct Values {
    LONG threshold;
    ULONG intensity;
};

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void)
{
}
void __regargs _CXBRK(void)
{
}

#endif

static
int DoModify(FRAME * frame, ULONG which, int thr, BOOL intensity, struct PPTBase * PPTBase)
{
    UWORD row;
    int res = PERR_OK;
    WORD pixelsize = frame->pix->components;
    UBYTE cspace = frame->pix->colorspace;

    InitProgress(frame, "Thresholding...", frame->selbox.MinY, frame->selbox.MaxY);

    for (row = frame->selbox.MinY; row < frame->selbox.MaxY; row++) {
        ROWPTR cp, dcp;
        WORD col;

        cp = GetPixelRow(frame, row);
        dcp = cp + (frame->selbox.MinX) * pixelsize;
        if (Progress(frame, row)) {
            res = PERR_BREAK;
            goto quit;
        }
        for (col = frame->selbox.MinX; col < frame->selbox.MaxX; col++) {
            UBYTE comp;

            if (intensity) {
                ULONG val;

                switch (cspace) {
                case CS_RGB:
                    val = (dcp[0]+dcp[1]+dcp[2])/3;
                    if( val < thr )
                        dcp[0] = dcp[1] = dcp[2] = 0;
                    else
                        dcp[0] = dcp[1] = dcp[2] = 255;
                    break;
                case CS_ARGB:
                    val = (dcp[1]+dcp[2]+dcp[3])/3;
                    if( val < thr )
                        dcp[1] = dcp[2] = dcp[3] = 0;
                    else
                        dcp[1] = dcp[2] = dcp[3] = 255;
                    break;
                case CS_GRAYLEVEL:
                    if( *dcp < thr )
                        *dcp = 0;
                    else
                        *dcp = 255;
                    break;
                }
                dcp += pixelsize;
            } else {
                for (comp = 0; comp < pixelsize; comp++) {
                    if (*dcp < thr)
                        *dcp = 0;
                    else
                        *dcp = 255;
                    dcp++;
                }
            }
        }
        PutPixelRow(frame, row, cp);
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

    DoModify( msg->aum_Frame, 0, msg->aum_Values[0], msg->aum_Values[1], msg->aum_PPTBase );

    return ARR_REDRAW;
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData(attr, MyTagArray);
}

PERROR
ParseRexxArgs( FRAME *frame, ULONG *args, struct Values *v, struct PPTBase *PPTBase )
{
    if (args[0])
        v->threshold = *((LONG *) args[0]);

    v->intensity = (BOOL)args[1];

    return PERR_OK;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    struct Hook pwhook = {{0}, MyHookFunc, 0L, NULL};
    struct TagItem level[] = {
        AROBJ_Value, NULL,
        ARSLIDER_Min, 0,
        ARSLIDER_Default, 128,
        ARSLIDER_Max, 255,
        AROBJ_PreviewHook, NULL,
        TAG_END};
    struct TagItem inten[] = {
        AROBJ_Value, NULL,
        ARCHECKBOX_Selected, NULL,
        AROBJ_PreviewHook, NULL,
        AROBJ_Label, (ULONG)"Use intensity?",
        TAG_END};

    struct TagItem win[] = {
     AR_SliderObject, NULL,
     AR_CheckBoxObject, NULL,
     AR_Text, (ULONG) "\nSet the thresholding level\n",
     AR_HelpNode, (ULONG) "effects.guide/Threshold",
     TAG_END};

    ULONG *args;
    PERROR res = PERR_OK;
    struct Values v = {128,TRUE}, *opt;

    if( opt = GetOptions( MYNAME ) ) {
        v = *opt;
    }

    level[0].ti_Data = (ULONG) &v.threshold;
    level[2].ti_Data = (ULONG) v.threshold;
    level[4].ti_Data = (ULONG) &pwhook;
    win[0].ti_Data = (ULONG) level;
    win[1].ti_Data = (ULONG) inten;
    inten[0].ti_Data = (ULONG) &v.intensity;
    inten[1].ti_Data = v.intensity;
    inten[2].ti_Data = (ULONG) &pwhook;

    pwhook.h_Data = (void *)GETREG(REG_A4); // Save register

    args = (ULONG *) TagData(PPTX_RexxArgs, tags);

    if (args) {
        ParseRexxArgs( frame, args, &v, PPTBase );
    } else {
        FRAME *pwframe;

        pwframe = ObtainPreviewFrame( frame, NULL );
        if ((res = AskReqA(frame, win)) != PERR_OK) {
            SetErrorCode(frame, res);
            if( pwframe ) ReleasePreviewFrame( pwframe );
            return NULL;
        }
        if( pwframe ) ReleasePreviewFrame( pwframe );
    }
    if (v.threshold < 0 || v.threshold > 255) {
        SetErrorCode(frame, PERR_INVALIDARGS);
        return NULL;
    } else {
        DoModify(frame, 0, v.threshold, (BOOL)v.intensity,PPTBase);
    }

    PutOptions( MYNAME, &v, sizeof(v) );

    return frame;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    struct TagItem level[] = {
        AROBJ_Value, NULL,
        ARSLIDER_Min, 0,
        ARSLIDER_Default, 128,
        ARSLIDER_Max, 255,
        TAG_END};
    struct TagItem inten[] = {
        AROBJ_Value, NULL,
        ARCHECKBOX_Selected, NULL,
        AROBJ_Label, (ULONG)"Use intensity?",
        TAG_END};

    struct TagItem win[] = {
     AR_SliderObject, NULL,
     AR_CheckBoxObject, NULL,
     AR_Text, (ULONG) "\nSet the thresholding level\n",
     AR_HelpNode, (ULONG) "effects.guide/Threshold",
     TAG_END};

    ULONG *args;
    PERROR res = PERR_OK;
    struct Values v = {128,TRUE}, *opt;
    STRPTR buffer;

    if( opt = GetOptions( MYNAME ) ) {
        v = *opt;
    }

    level[0].ti_Data = (ULONG) &v.threshold;
    level[2].ti_Data = (ULONG) v.threshold;
    win[0].ti_Data = (ULONG) level;
    win[1].ti_Data = (ULONG) inten;
    inten[0].ti_Data = (ULONG) &v.intensity;
    inten[1].ti_Data = v.intensity;

    args = (ULONG *) TagData(PPTX_RexxArgs, tags);
    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if (args) {
        ParseRexxArgs( frame, args, &v, PPTBase );
    }

    if ((res = AskReqA(frame, win)) != PERR_OK) {
        SetErrorCode(frame, res);
        return res;
    }

    if (v.threshold < 0 || v.threshold > 255) {
        SetErrorCode(frame, PERR_INVALIDARGS);
        return PERR_INVALIDARGS;
    } else {
        SPrintF( buffer, "LIMIT %d %s", v.threshold, v.intensity ? "INTENSITY" : "" );
    }

    PutOptions( MYNAME, &v, sizeof(v) );

    return PERR_OK;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/
