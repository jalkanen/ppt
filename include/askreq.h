/*
    PROJECT: ppt
    MODULE:  askreq.h

    $Id: askreq.h 6.0 1999/09/04 19:49:28 jj Exp jj $

    This file contains the definitions for the PPT requester
    system.
*/

#ifndef ASKREQ_H
#define ASKREQ_H

#ifndef PPT_H
# include "ppt.h"
#endif

#define MAX_AROBJECTS 30 /* Maximum amount of objects you may create.  This
                            applies to external modules ONLY, REXX ASKREQ may
                            have a different limit. */

#define AR_Dummy            (TAG_USER+0x8000000)
#define AR_Title            (AR_Dummy + 1)  /* STRPTR */
#define AR_Text             (AR_Dummy + 2)  /* STRPTR */
#define AR_Positive         (AR_Dummy + 3)  /* STRPTR */
#define AR_Negative         (AR_Dummy + 4)  /* STRPTR */
#define AR_Dimensions       (AR_Dummy + 5)  /* struct IBox * */
#define AR_WindowID         (AR_Dummy + 6)  /* PRIVATE */
#define AR_HelpText         (AR_Dummy + 7)  /* STRPTR */
#define AR_HelpNode         (AR_Dummy + 8)  /* STRPTR */
#define AR_RenderHook       (AR_Dummy + 9)  /* struct Hook * (V4) */

/* Values from AR_Dummy + 9 to AR_Dummy + 99 reserved */

#define AR_ObjectMin        (AR_Dummy + 100)    /* PRIVATE */
#define AR_SliderObject     (AR_Dummy + 100)    /* struct TagItem * */
#define AR_StringObject     (AR_Dummy + 101)    /* struct TagItem * */
#define AR_ToggleObject     (AR_Dummy + 102)    /* struct TagItem * */
#define AR_CheckBoxObject   (AR_Dummy + 103)    /* struct TagItem * */
#define AR_CycleObject      (AR_Dummy + 104)    /* struct TagItem * */
#define AR_FloatObject      (AR_Dummy + 105)    /* struct TagItem * */
#define AR_MxObject         (AR_Dummy + 106)    /* struct TagItem * */
#define AR_ObjectMax        (AR_Dummy + 499)

/**********************************************************************
 *
 *  Generic tags for objects
 */

#define AROBJ_Value         (AR_Dummy + 500)    /* APTR */
#define AROBJ_Label         (AR_Dummy + 501)    /* STRPTR */
#define AROBJ_PreviewHook   (AR_Dummy + 502)    /* struct Hook * (V4) */
#define AROBJ_HelpText      (AR_Dummy + 503)    /* STRPTR (V4) */
#define AROBJ_HelpNode      (AR_Dummy + 504)    /* STRPTR (V4) */

/*
 *  The function that is in the hook for AROBJ_PreviewHook
 *  will receive the following arguments:
 *
 *  A0: pointer to the hook
 *  A2: Pointer to the actual BGUI object that triggered the message
 *      (be EXTRA careful with this!)
 *  A1: Pointer to a ARHookMsg struct, defined below.  The objectid
 *      is the number of the object that the user selected, starting
 *      from zero.  For example: If you define a slider object, a
 *      checkbox object and a string object (in this order) the
 *      slider object would be number zero, the checkbox one and
 *      the string number two.
 *
 *      Note that the RPort and Area are quite valid, in case you'd
 *      like to render your own preview immediately.
 *
 *  The hook should return
 *      ARR_REDRAW, if the data was changed and PPT should do a redraw
 *      ARR_NOCHANGE, if the data was not changed and there is no need for a redraw
 *      ARR_DONE, if the data was changed, but you don't want PPT to touch the
 *          rendering area, like when you just rendered the preview yourself.
 *
 *  Take into account that if you want to render your own preview, you
 *  will need to prepare yourself for the ARM_RENDER messages as well.  They're
 *  sent usually when the window is resized or uncovered.
 */

#define ARM_UPDATE          1L      /* A gadget was updated */
#define ARM_RENDER          2L      /* The preview area requires updating */

/* ARM_UPDATE */
struct ARUpdateMsg {
    ULONG           MethodID;
    ULONG           aum_Flags;
    struct Frame_t  *aum_Frame;
    struct PPTBase  *aum_PPTBase; /* Current context */
    LONG            *aum_Values;

    struct RastPort *aum_RPort;
    struct IBox     aum_Area;

    ULONG           aum_ObjectID; /* Changed object id */
};

/* ARM_RENDER */
struct ARRenderMsg {
    ULONG           MethodID;
    ULONG           arm_Flags;
    struct Frame_t  *arm_Frame;
    struct PPTBase  *arm_PPTBase; /* Current context */
    LONG            *arm_Values;

    struct RastPort *arm_RPort;
    struct IBox     arm_Area;
};

/* arm_Flags */

#define ARF_INTERIM         (1<<0)  /* This was an intermediate update,
                                       not the final.  Check struct opUpdate
                                       and the OPUF_INTERIM field. */

/* Return values for the hook */

#define ARR_NOCHANGE        0L
#define ARR_DONE            1L
#define ARR_REDRAW          2L

/**********************************************************************
 *
 *  Slider value objects tag values.
 */

#define ARSLIDER_Min        (AR_Dummy + 1000)   /* LONG */
#define ARSLIDER_Max        (AR_Dummy + 1001)   /* LONG */
#define ARSLIDER_Default    (AR_Dummy + 1002)   /* LONG */

/**********************************************************************
 *
 *  String object tag values
 */

#define ARSTRING_InitialString (AR_Dummy + 1100) /* STRPTR */
#define ARSTRING_MaxChars   (AR_Dummy + 1101)    /* LONG */

/**********************************************************************
 *
 *  Checkbox object tag values
 */

#define ARCHECKBOX_Selected (AR_Dummy + 1200)   /* BOOL */

/**********************************************************************
 *
 *  Cycle object tag values
 */

#define ARCYCLE_Active      (AR_Dummy + 1300)   /* LONG */
#define ARCYCLE_Labels      (AR_Dummy + 1301)   /* STRPTR * */
#define ARCYCLE_Popup       (AR_Dummy + 1302)   /* BOOL */

/**********************************************************************
 *
 *  Floating point class tag values
 */

#define ARFLOAT_Min         (AR_Dummy + 1400)   /* LONG */
#define ARFLOAT_Max         (AR_Dummy + 1401)   /* LONG */
#define ARFLOAT_Default     (AR_Dummy + 1402)   /* LONG */
#define ARFLOAT_Divisor     (AR_Dummy + 1403)   /* LONG */
#define ARFLOAT_FormatString (AR_Dummy + 1404)   /* STRPTR */

/**********************************************************************
 *
 *  Mx object tag values
 */

#define ARMX_Active         (AR_Dummy + 1500)   /* LONG */
#define ARMX_Labels         (AR_Dummy + 1501)   /* STRPTR * */

/**********************************************************************/



#endif /* ASKREQ_H */
