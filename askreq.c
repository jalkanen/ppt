/*----------------------------------------------------------------------*/
/*
    PROJECT: PPT
    MODULE : askreq.c

    $Id: askreq.c,v 1.26 1998/02/21 15:50:21 jj Exp $

    This module contains the GUI management code for external modules.

    BGUI and bgui.library are (C) Jan van den Baard, 1994-1995.
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "gui.h"
#include "misc.h"

#include "askreq.h"

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#ifndef PRAGMAS_BGUI_PRAGMAS_H
#include <pragmas/bgui_pragmas.h>
#endif

#ifndef PRAGMAS_INTUITION_PRAGMAS_H
#include <pragmas/intuition_pragmas.h>
#endif

#ifndef PRAGMAS_GRAPHICS_PRAGMAS_H
#include <pragmas/graphics_pragmas.h>
#endif

#ifndef PRAGMAS_UTILITY_PRAGMAS_H
#include <pragmas/utility_pragmas.h>
#endif

#ifndef PRAGMAS_DOS_PRAGMAS_H
#include <pragmas/dos_pragmas.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif

#ifndef LIBRARIES_ASL_H
#include <libraries/asl.h>
#endif

#ifndef RENDERAREACLASS_H
#include "renderareaclass.h"
#endif

#include "proto/bguifloat.h"

#include <sprof.h>
#include <math.h>

#define GID_AR_RENDERAREA (GID_START+MAX_AROBJECTS+0xB5)

/*----------------------------------------------------------------------*/
/* Types */

struct AskReqWinData {
    struct IBox     winpos;
};

struct RealObject {
    ULONG           type;
    Object          *obj;
    struct Hook     *hook;
};

/*----------------------------------------------------------------------*/
/* Prototypes */

Prototype ASM int      AskReqA( REG(a0) FRAME *, REG(a1) struct TagItem *, REG(a6) EXTBASE * );

/*----------------------------------------------------------------------*/
/* Code */

/****u* pptsupport/AskReqA ******************************************
*
*   NAME
*       AskReqA -- Query the user for some values
*       AskReq -- Varargs version
*
*   SYNOPSIS
*       error = AskReqA( frame, objectlist );
*       D0               A0     A1
*
*       PERROR AskReqA( FRAME *, struct TagItem * );
*
*       error = AskReq( ExtBase, frame, tag1, ... )
*
*       PERROR AskReq( EXTBASE *, FRAME *, Tag, ... )
*
*   FUNCTION
*       Show a configurable requester to ask the user for some values.
*
*   INPUTS
*       frame - as usual. May be NULL.
*       objs - a pointer to an array of TagItems, which contain
*           the object data. The following tags are allowed:
*
*           AR_Text - A piece of text to be shown to the user. English
*               default is "Change values:". You may use any BGUI control
*               sequences here.
*
*           AR_Positive - Text for the gadget for a positive answer.
*               English default is "Ok".
*
*           AR_Negative - Text for the gadget for a negative answer.
*               English default is "Cancel".  If the negative tag
*               is specified, but set to NULL or to an empty string,
*               the window will have only the positive gadget.
*
*           AR_Title - Title for the requester window. English default is
*               "PPT Request".
*
*           AR_HelpText (STRPTR) - A help text that is shown in a separate
*               requester when the user presses HELP.  This requester is
*               synchronous.  Default is NULL.
*
*           AR_HelpNode (STRPTR) - The Amigaguide node that is to be shown
*               in the help window when the user presses HELP.  This help
*               is asynchronous.  Default is NULL.
*
*               Note that both AR_HelpText and AR_HelpNode are shown globally
*               for the window, if there are no object-specific help
*               texts defined.  See below for AROBJ_HelpText and
*               AROBJ_HelpNode.
*
*       You may also specify an object type for creation. These are handled
*       in the order they appear in. The ti_Data field should point to an
*       array of TagItems, which should contain more information on the
*       object. If you do not state any objects, you will get a simple
*       boolean Yes/No -type requester.
*
*       You may choose from these objects:
*
*       AR_SliderObject - Creates a slider object that has also an integer
*           gadget associated. Labels are placed on the left side. Available
*           attributes are:
*
*           ARSLIDER_Min
*           ARSLIDER_Max - Minimum and maximum value for the slider. Defaults
*               are 0 and 100, respectively.
*
*           ARSLIDER_Default - Starting value. Default is 50.  If this tag
*               does not exist, the default is taken from wherever
*               AROBJ_Value happens to be pointing at.
*
*           The AROBJ_Value field will contain the slider value on return.
*
*       AR_StringObject - Create a string gadget. Available attributes
*           are:
*
*           ARSTRING_MaxChars (ULONG) - Maximum length of the string
*           ARSTRING_InitialString (STRPTR) - A pointer to the initial string
*               to be displayed in the gadget.
*
*           The AROBJ_Value - field should point to a buffer containing
*           space for at least ARSTRING_MaxChars characters.
*
*       AR_CheckBoxObject - Create a checkbox.  Available attributes
*           are:
*
*           ARCHECKBOX_Selected (BOOL) - TRUE, if the original state is
*               selected; FALSE otherwise.
*
*           The AROBJ_Value - field is set upon return to != 0, if the
*           gadget was set, 0 otherwise.
*
*       AR_CycleObject - Create a cycle (popmenu) object.  Available
*           attributes are:
*
*           ARCYCLE_Labels (STRPTR *) - pointer to a NULL-terminated array
*               of strings, containing the labels of different choices.
*
*           ARCYCLE_Active (LONG) - which one of the choices should be active
*               upon startup.  Default 0.
*
*           ARCYCLE_Popup (BOOL) - TRUE, if the cycle object should be
*               a popup menu variant.  Default is FALSE.
*
*           The AROBJ_Value is set upon return to the currently active
*           selection.
*
*       AR_FloatObject - Creates a string gadget which accepts only
*           floating point values.  It also attaches a slider next to it.
*           Available attributes are:
*
*           ARFLOAT_Min (LONG) - The minimum allowed value for the gadget.
*               Default is -100 (-1.0).
*
*           ARFLOAT_Max (LONG) - The maximum allowed value for the gadget.
*               Default is 100 (1.0).
*
*           ARFLOAT_Default (LONG) - The startup value.  Default is 0.
*
*           ARFLOAT_FormatString (STRPTR) - How the string gadget's contents
*               should be formatted.  See printf(1) for more information.
*               Default is ".3f".
*
*           ARFLOAT_Divisor (LONG) - The divisor by which all values will
*               be divided.  Default is 100.
*
*           The float gadget uses long integers instead of floats because
*           the values are passed in a tag array.  To simplify casting
*           problems, I adopted this methodology.
*
*           The result is a long, which should be divided by the divisor
*           before acting on it.  Note that the divisor also defines
*           the minimum value by which the knob of the slider can be moved
*           and also the minimum resolution of the gadget.
*
*           NB: If you use too large values, you'll get into the roundoff
*           error hell.  Try to keep your numbers below 65535.
*
*       You may also specify these common attributes for any objects:
*
*           AROBJ_Label (STRPTR) - A label for this object.
*
*           AROBJ_Value (ULONG *) - This points to a location in which
*               the value from the gadget is written if the user
*               selected the positive answer.
*
*           AROBJ_HelpText (STRPTR) - Pointer to a nul-terminated string,
*               which will be shown in a simple requester when the user
*               presses the HELP key over the object.  Use this
*               for short help-texts.
*
*           AROBJ_HelpNode (STRPTR) - Pointer to a standard C string,
*               which contains the file and node of an AmigaGuide help
*               text.  For example, you could say something like:
*
*               AROBJ_HelpNode, "HELP:mymodule.guide/Twiddle",
*
*               and PPT would try to locate a guide called "mymodule.guide"
*               in directory "HELP:" and display the node called "Twiddle"
*               from it.
*
*               The node is displayed when the user presses the HELP key over
*               this gadget.
*
*   RESULT
*       error - Standard PPT error code. If the user chose Cancel,
*           this will be PERR_CANCELED. If everything went OK, then
*           this will be PERR_OK.
*
*           Ctrl-C signals are recognised correctly and a PERR_BREAK
*           is returned. If this happens, exit as soon as you can
*           from your module.
*
*   EXAMPLE
*
*       This opens up a simple window with two sliders.
*
*       void TestAR() {
*           LONG foo2, foo3;
*           struct TagItem myslider[] = { \* Use defaults for this slider *\
*               AROBJ_Label, "%",
*               AROBJ_Value, &foo2,
*               TAG_END
*           };
*
*           struct TagItem myslider2[] = {
*               ARSLIDER_Min, -100,
*               ARSLIDER_Max, 100,
*               ARSLIDER_Default, 0,
*               AROBJ_Value, &foo3,
*               TAG_END
*           };
*
*           struct TagItem mywindow[] = {
*               AR_Title, "AR Test Window",
*               AR_Text, ISEQ_C"\nPlease do tamper around with\n"
*                              "the sliders...\n",
*               AR_Positive, "Cool",
*               AR_Negative, "Not Cool",
*               AR_SliderObject, myslider,
*               AR_SliderObject, myslider2,
*               TAG_END
*           };
*
*           if(AskReqA( NULL, mywindow ) == PERR_OK) {
*               \* User chose "Cool" *\
*               PDebug("Slider 1 = %ld\n", foo2);
*               PDebug("Slider 2 = &ld\n", foo3);
*           }
*       }
*
*   NOTES
*       The varargs version AskReq() can be found in pptsupp.lib. Please
*       note that it really does require the ExtBase in stack, since the
*       pointer cannot be declared global.  Unless you're running SAS/C
*       of course, in which case the compiler uses the #pragma tagcall
*       and you can stop worrying.
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
* Support returns std ppt error code
* BUG: Should heed frame window as parent.
* BUG: A lot more objects needed.
* BUG: Should be able to give 1...N gadgets.
*   maybe add AR_Gadgets with "OK|Really|Cancel" - style?
* BUG: Should heed also the text height.
*/

Local
Object *GetARObject( struct TagItem *tag, ULONG id,
                     struct RealObject *realobject,
                     STRPTR defaulthelptext, STRPTR defaulthelpnode,
                     EXTBASE *xd )
{
    Object *obj = NULL;
    struct TagItem *list = (struct TagItem *)(tag->ti_Data), *t;
    APTR UtilityBase = xd->lb_Utility;
    EXTBASE *ExtBase = xd; /* BUG! */
    struct Hook *hook;
    STRPTR helptext, helpnode;

    if(list == NULL)
        return NULL;

    D(bug("\tAdding a new object...\n"));

    helptext = (STRPTR) GetTagData(AROBJ_HelpText, (ULONG)defaulthelptext, list );
    helpnode = (STRPTR) GetTagData(AROBJ_HelpNode, (ULONG)defaulthelpnode, list );

    switch( tag->ti_Tag ) {
        case AR_SliderObject: {
            Object *Slider, *Integer;
            ULONG min,max,level;

            /*
             *  Fetch limits and current value. Default value is preferred
             *  over whatever is found in the Value field at the time.
             */

            min = GetTagData( ARSLIDER_Min, 0, list );
            max = GetTagData( ARSLIDER_Max, 100, list );
            if( t = FindTagItem( ARSLIDER_Default, list ) ) {
                level = t->ti_Data;
            } else if (t = FindTagItem( AROBJ_Value, list ) ) {
                level = *((LONG *)(t->ti_Data));
            } else {
                level = 50; // Default
            }

            obj = MyHGroupObject, Spacing(4),
                GROUP_EqualHeight, TRUE,
                StartMember,
                    Slider = MySliderObject, GA_ID, id,
                        Label( (STRPTR)GetTagData( AROBJ_Label, NULL, list ) ), Place( PLACE_LEFT ),
                        SLIDER_Min, min,
                        SLIDER_Max, max,
                        SLIDER_Level,level,
                        helptext ? TAG_IGNORE : TAG_SKIP,1,
                            BT_HelpText, helptext,
                        helpnode ? TAG_IGNORE : TAG_SKIP,2,
                            BT_HelpHook, &HelpHook,
                            BT_HelpNode, helpnode,
                    EndObject,
                EndMember,
                StartMember,
                    Integer = MyStringObject, GA_ID, 1000+id, /* BUG */
                        RidgeFrame,
                        STRINGA_MinCharsVisible,4,
                        STRINGA_MaxChars,       8, /* BUG: Should adjust according to the max/min values. */
                        STRINGA_LongVal,        level,
                        STRINGA_IntegerMin,     min,
                        STRINGA_IntegerMax,     max,
                        STRINGA_Justification,  STRINGRIGHT,
                        helptext ? TAG_IGNORE : TAG_SKIP,1,
                            BT_HelpText, helptext,
                        helpnode ? TAG_IGNORE : TAG_SKIP,2,
                            BT_HelpHook, &HelpHook,
                            BT_HelpNode, helpnode,
                    EndObject, Weight(1),
                EndMember,
            EndObject;

            if(obj) { /* If either of the gagdets did not open, the group didn't either. */

                AddMap( Slider, Integer, dpcol_sl2int );
                AddMap( Integer,Slider,  dpcol_int2sl );
                realobject->obj = Slider; /* The real gadget */
                D(bug("\tAdded slider object\n"));
            }

            break;
        }

        case AR_FloatObject: {
            Object *Slider, *Float;
            LONG min,max,level,div,mcv;
            STRPTR format;
            char tmpbuf[20];

            if(!xd->FloatClass) {
                D(bug("No float class available\n"));
                return NULL;
            }

            min     = (LONG)GetTagData( ARFLOAT_Min, -100, list );
            max     = (LONG)GetTagData( ARFLOAT_Max, 100, list );
            level   = (LONG)GetTagData( ARFLOAT_Default, 0, list );
            div     = (LONG)GetTagData( ARFLOAT_Divisor, 100, list );
            format  = (STRPTR)GetTagData( ARFLOAT_FormatString, (ULONG)"%.3f", list );

            D(bug("\tFloat: min=%ld, max=%ld, def=%ld, div=%ld, format=%s\n",
                    min,max,level,div,format));

            /*
             *  Calculate the width that the given format string requires.
             *  Add two bytes for sign and the cursor.
             */

            sprintf(tmpbuf,format,PI);
            mcv = strlen(tmpbuf)+2;

            obj = MyHGroupObject, Spacing(4),
                GROUP_EqualHeight, TRUE,
                StartMember,
                    Slider = MySliderObject, GA_ID, id,
                        Label( (STRPTR)GetTagData( AROBJ_Label, NULL, list ) ), Place( PLACE_LEFT ),
                        SLIDER_Min, min,
                        SLIDER_Max, max,
                        SLIDER_Level,level,
                        helptext ? TAG_IGNORE : TAG_SKIP,1,
                            BT_HelpText, helptext,
                        helpnode ? TAG_IGNORE : TAG_SKIP,2,
                            BT_HelpHook, &HelpHook,
                            BT_HelpNode, helpnode,
                    EndObject,
                EndMember,
                StartMember,
                    Float = NewObject( xd->FloatClass, NULL,
                        GA_ID, 1000+id, /* BUG */
                        RidgeFrame,
                        STRINGA_MinCharsVisible,mcv,
                        STRINGA_MaxChars,       12, /* BUG: Should adjust according to the max/min values. */
                        FLOAT_LongValue,        level,
                        FLOAT_LongMin,          min,
                        FLOAT_LongMax,          max,
                        FLOAT_Divisor,          div,
                        FLOAT_Format,           format,
                        STRINGA_Justification,  STRINGRIGHT,
                        helptext ? TAG_IGNORE : TAG_SKIP,1,
                            BT_HelpText, helptext,
                        helpnode ? TAG_IGNORE : TAG_SKIP,2,
                            BT_HelpHook, &HelpHook,
                            BT_HelpNode, helpnode,
                    EndObject, Weight(1),
                EndMember,
            EndObject;

            if(obj) { /* If either of the gagdets did not open, the group didn't either. */

                AddMap( Slider, Float,  dpcol_sl2fl );
                AddMap( Float,  Slider, dpcol_fl2sl );
                realobject->obj = Float; /* The real gadget */
                D(bug("\tAdded float object\n"));
            }

            break;
        }

        case AR_StringObject:
            obj = MyStringObject, GA_ID, id,
                Label( (STRPTR) GetTagData( AROBJ_Label, NULL, list )), Place( PLACE_LEFT ),
                RidgeFrame,
                STRINGA_TextVal, (STRPTR) GetTagData( ARSTRING_InitialString, (ULONG) "", list ),
                STRINGA_MaxChars, GetTagData( ARSTRING_MaxChars, 40, list ),
                helptext ? TAG_IGNORE : TAG_SKIP,1,
                    BT_HelpText, helptext,
                helpnode ? TAG_IGNORE : TAG_SKIP,2,
                    BT_HelpHook, &HelpHook,
                    BT_HelpNode, helpnode,
            EndObject;

            if( obj ) {
                realobject->obj = obj;
                D(bug("\tAdded string object\n"));
            }
            break;

        case AR_CheckBoxObject: {
            Object *cbox;

            obj = MyHGroupObject,
                VarSpace(DEFAULT_WEIGHT),
                StartMember,
                    cbox = MyCheckBoxObject, GA_ID, id,
                        Label( (STRPTR) GetTagData( AROBJ_Label, NULL, list )), Place( PLACE_LEFT ),
                        ButtonFrame,
                        GA_Selected, GetTagData( ARCHECKBOX_Selected, FALSE, list ),
                        helptext ? TAG_IGNORE : TAG_SKIP,1,
                            BT_HelpText, helptext,
                        helpnode ? TAG_IGNORE : TAG_SKIP,2,
                            BT_HelpHook, &HelpHook,
                            BT_HelpNode, helpnode,
                    EndObject, FixMinSize,
                    LGO_NoAlign, TRUE,
                EndMember,
                VarSpace(DEFAULT_WEIGHT),
            EndObject;

            if(obj) {
                realobject->obj = cbox;
                D(bug("\tAdded checkbox object\n"));
            }

            break;
        }

        case AR_CycleObject:
            obj = MyCycleObject, GA_ID, id,
                Label( (STRPTR) GetTagData( AROBJ_Label, NULL, list )), Place(PLACE_LEFT),
                ButtonFrame,
                CYC_Active, GetTagData( ARCYCLE_Active, 0, list ),
                CYC_Labels, GetTagData( ARCYCLE_Labels, NULL, list ),
                CYC_Popup,  GetTagData( ARCYCLE_Popup,  FALSE, list ),
                helptext ? TAG_IGNORE : TAG_SKIP,1,
                    BT_HelpText, helptext,
                helpnode ? TAG_IGNORE : TAG_SKIP,2,
                    BT_HelpHook, &HelpHook,
                    BT_HelpNode, helpnode,
            EndObject;

            if( obj ) {
                realobject->obj = obj;
                D(bug("\tAdded cycle object\n"));
            }

            break;

        default:
            Req(NEGNUL, NULL, XGetStr(MSG_AR_UNKNOWNTYPE),tag->ti_Tag );
            break;
    }

    realobject->type = tag->ti_Tag;
    if( hook = (struct Hook *)GetTagData(AROBJ_PreviewHook, NULL, list ) ) {
        D(bug("\t\tThis object has a preview hook @%08x\n",hook));
        realobject->hook = hook;
    }

    D(bug("\t\tNew object type = %08X, created @ %08X (real = %08X)\n",tag->ti_Tag, obj, *realobject ));

    return obj;
}

Local
void FetchARGadgetValue( struct RealObject *who, ULONG *where, EXTBASE *ExtBase )
{
    UBYTE *tmp;
    struct IntuitionBase *IntuitionBase = ExtBase->lb_Intuition;

    if( where ) {
        switch( who->type ) {

            case AR_SliderObject:
                GetAttr( SLIDER_Level, (APTR)who->obj, where );
                break;

            case AR_StringObject:
                GetAttr( STRINGA_TextVal, (APTR)who->obj, (ULONG*)&tmp );
                strcpy( (char *)where, tmp );
                break;

            case AR_CheckBoxObject:
                GetAttr( GA_Selected, (APTR)who->obj, where );
                break;

            case AR_CycleObject:
                GetAttr( CYC_Active, (APTR)who->obj, where );
                break;

            case AR_FloatObject:
                GetAttr( FLOAT_LongValue, (APTR)who->obj, where );
                break;

            default:
                XReq(NEGNUL,NULL, XGetStr(MSG_AR_WEIRDOBJECT), who->type );
                break;
        }
    }
}

Local
void ReadARGadgets( Object *Win, struct RealObject *Gadgets, struct TagItem *gadgets, EXTBASE *xd )
{
    struct TagItem *tstate = gadgets, *tag;
    APTR UtilityBase = xd->lb_Utility;
    ULONG objnum = 0;
    EXTBASE *ExtBase = xd; /* BUG */

    D(bug("ReadARGadgets()\n"));

    while( (tag = NextTagItem( &tstate )) ) {
        if(tag->ti_Tag >= AR_ObjectMin && tag->ti_Tag <= AR_ObjectMax) {
            volatile APTR where;

            where = (APTR)GetTagData( AROBJ_Value, NULL, (struct TagItem *)(tag->ti_Data) );
            D(bug("Object %d:\n\tWhere = %08X\n",objnum,where));

            FetchARGadgetValue( &Gadgets[objnum], (ULONG *)where, ExtBase );

            objnum++; /* Next Object */

        } else {
            D(bug("\tSkipping tag %08X\n",tag->ti_Tag));
        }
    }

}

Local
void ReadARGadgetsToArray( Object *Win, struct RealObject *Gadgets, LONG *table, EXTBASE *ExtBase )
{
    ULONG objnum = 0;

    D(bug("ReadARGadgetsToArray()\n"));

    for( objnum = 0; Gadgets[objnum].obj; objnum++ ) {
        FetchARGadgetValue( &Gadgets[objnum], (ULONG *)&table[objnum], ExtBase );
    }
}

struct ARArgs {
    FRAME               *frame, *pwframe;
    Object              *Win, *Ok, *Cancel,
                        *renderArea, *userGadgets;
    struct TagItem      *artags;
    struct Hook         *renderHook;
    LONG                hookobjectid;
    char                awdname[80];
    char                deftitle[NAMELEN*2+1];
    struct AskReqWinData awd;
};

Local
PERROR GetARWindow( struct ARArgs *ar, struct RealObject *RealObjs, EXTBASE *ExtBase )
{
    STRPTR text, helpnode, helptext;
    FRAME *frame = ar->frame;
    UWORD textlen, lines;
    char *negative;
    Object  WindowObjs[MAX_AROBJECTS*5] = {0};
    struct TagItem *list = ar->artags;
    PERROR res = PERR_OK;
    struct AskReqWinData *awd;
    struct TagItem *tstate, *tag;
    ULONG objs = 0, wobjcount = 0;
    int i;

    D(bug("\tOpenARWindow()\n"));
    tstate = list;

    /*
     *  Help system and default values
     */

    text = (STRPTR)GetTagData( AR_Text, (ULONG)XGetStr(MSG_SELECT_VALUES), list );
    helpnode = (STRPTR) GetTagData( AR_HelpNode, NULL, list );
    helptext = (STRPTR) GetTagData( AR_HelpText, NULL, list );

    BGUI_InfoTextSize( MAINWIN->RPort, text, &textlen, &lines );
    D(bug("\tWe got the text length of %d pixels\n",textlen));
    if(textlen)
        textlen = 100 * (textlen + 20) / (MAINSCR->Width - 80 );
    else
        textlen = 30;

    D(bug("\tLength after justification is %d\n",textlen));

    lines /= MAINWIN->RPort->Font->tf_YSize;

    /* Build default title */

    if( frame ) {
        if( frame->currext ) {
            sprintf( ar->deftitle, "%s: %s", frame->currext->realname, frame->name );
        } else {
            strncpy( ar->deftitle, frame->name, NAMELEN );
        }
    } else {
        strcpy( ar->deftitle, GetStr(MSG_PPT_REQUEST) );
    }

    /*
     *  Figure out a name for this system.
     *  Fetch possible pre-saved data
     */

    sprintf(ar->awdname,"%s_AR", ar->deftitle);
    if( awd = (struct AskReqWinData *)GetOptions( ar->awdname, ExtBase ) ) {
        ar->awd = *awd;
    }

    WindowObjs[0] = GROUP_Style;
    WindowObjs[1] = GRSTYLE_VERTICAL;
    WindowObjs[2] = GROUP_Spacing;
    WindowObjs[3] = GRSPACE_NORMAL;
    WindowObjs[4] = GROUP_SpaceObject;
    WindowObjs[5] = DEFAULT_WEIGHT;
    wobjcount = 6;

    while( (tag = NextTagItem( &tstate )) ) {
        if(tag->ti_Tag >= AR_ObjectMin && tag->ti_Tag <= AR_ObjectMax) {
            D(bug("Object %d:\n",objs));
            WindowObjs[wobjcount++] = GROUP_Member;
            WindowObjs[wobjcount++] = (ULONG)GetARObject( tag, GID_START+objs, &RealObjs[objs], helptext, helpnode, ExtBase );
            WindowObjs[wobjcount++] = LGO_FixMinHeight;
            WindowObjs[wobjcount++] = TRUE;
            WindowObjs[wobjcount++] = TAG_END; WindowObjs[wobjcount++] = 0L;
            objs++;
        } else {
            D(bug("\tSkipping tag %08X\n",tag->ti_Tag));
        }
    }

    WindowObjs[wobjcount++] = GROUP_SpaceObject;
    WindowObjs[wobjcount++] = DEFAULT_WEIGHT;
    WindowObjs[wobjcount++] = TAG_DONE;
    WindowObjs[wobjcount++] = 0L;

    RealObjs[objs].obj = 0; // Sentinel

    /*
     *  Check up on the negative text.
     */

    tag = FindTagItem( AR_Negative, list );
    if(tag) {
        if( tag->ti_Data ) {
            if( ((char *)(tag->ti_Data))[0] == '\0' )
                negative = NULL;
            else
                negative = (char *) tag->ti_Data;
        } else {
            negative = NULL;
        }
    } else {
        negative = XGetStr(MSG_CANCEL_GAD);
    }

    /*
     *  Create render class object, if needed
     */

    if( RenderAreaClass && HasPreview( frame ) ) {
        for( i = 0; i < objs; i++ ) {
            if( RealObjs[i].hook ) {
                D(bug("Allocating rendering area\n"));
#if 0
                renderArea = NewObject( RenderAreaClass, NULL,
                                        GA_ID, GID_AR_RENDERAREA,
                                        RAC_Frame, frame,
                                        RAC_ExtBase, ExtBase,
#else
                ar->renderArea = BGUI_NewObject( BGUI_AREA_GADGET,
                                        GA_ID, GID_AR_RENDERAREA,
#endif
                                        AREA_MinHeight,ar->pwframe->pix->height,
                                        AREA_MinWidth,ar->pwframe->pix->width,
                                        ButtonFrame, FRM_Flags, FRF_RECESSED,
                                        helptext ? TAG_IGNORE : TAG_SKIP,1,
                                            BT_HelpText, helptext,
                                        helpnode ? TAG_IGNORE : TAG_SKIP,2,
                                            BT_HelpHook, &HelpHook,
                                            BT_HelpNode, helpnode,
                                        TAG_DONE );
                if(!ar->renderArea) { D(bug("Couldn't alloc renderArea\n")); }
                ar->hookobjectid = i;

                /*
                 *  Check if it also has a rendering hook enabled
                 */

                ar->renderHook = (struct Hook *)GetTagData( AR_RenderHook, NULL, list );
                break; /* We can quit the for() - loop, since the area needs to be done */
            }
        }
    }

    /*
     *  Create the user gadgets
     */

    if( objs ) {
        ar->userGadgets = BGUI_NewObjectA( BGUI_GROUP_GADGET, (struct TagItem *)WindowObjs );
    }

    /*
     *  Then, build window object and open it.
     */

    ar->Win = MyWindowObject,
        WINDOW_Title,       GetTagData( AR_Title, (ULONG) ar->deftitle, list ),
        WINDOW_Screen,      MAINSCR,
        WINDOW_ScaleWidth,  (ULONG)textlen,
        WINDOW_Font,        globals->userprefs->mainfont,
        TAG_SKIP,           awd ? 0 : 1,
        WINDOW_Bounds,      &(awd->winpos),
        WINDOW_UniqueID,    GetTagData( AR_WindowID, 0L, list ),
        WINDOW_SmartRefresh,FALSE,
        WINDOW_MasterGroup,
            MyVGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                /* Infoblurb */
                StartMember,
                    MyInfoObject,
                        ButtonFrame,
                        FRM_Flags, FRF_RECESSED,
                        INFO_TextFormat, (ULONG)text,
                        INFO_MinLines, (ULONG)lines,
                        helptext ? TAG_IGNORE : TAG_SKIP,1,
                            BT_HelpText, helptext,
                        helpnode ? TAG_IGNORE : TAG_SKIP,2,
                            BT_HelpHook, &HelpHook,
                            BT_HelpNode, helpnode,
                    EndObject, Weight(2000),
                EndMember,

                StartMember,
                    MyHGroupObject, NormalSpacing,
                        /*
                         *  Rendering area, if requested.
                         */

                        (ar->renderArea) ? TAG_IGNORE : TAG_SKIP, 4,
                        GROUP_Member, ar->renderArea,
                            LGO_FixMinWidth, TRUE,
                            LGO_FixMinHeight, TRUE,
                        TAG_END, 0L,

                        /*
                         *  User supplied Gadgets. If there are none, skip this phase
                         */

                        (ar->userGadgets) ? TAG_IGNORE : TAG_SKIP, 4,
                        GROUP_Member, ar->userGadgets,
                            LGO_Align, TRUE,
                            LGO_FixMinHeight, FALSE,
                        TAG_END, 0L,
                    EndObject,
                EndMember,

                /*
                 *  Separator and the standard gadgets.
                 */

                StartMember,
                    MySeparatorObject, SEP_Horiz, TRUE, EndObject,
                EndMember,
                /* OK & Cancel */
                StartMember,
                    MyHGroupObject, Spacing(4),
                        negative ? TAG_SKIP : TAG_IGNORE, 1,
                        VarSpace(50),
                        StartMember,
                            ar->Ok = MyButtonObject, GA_ID, GID_AR_OK,
                                ULabel( GetTagData( AR_Positive,
                                                  (ULONG)XGetStr(MSG_OK_GAD),
                                                  list) ),
                                helptext ? TAG_IGNORE : TAG_SKIP,1,
                                    BT_HelpText, helptext,
                                helpnode ? TAG_IGNORE : TAG_SKIP,2,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, helpnode,
                            EndObject,
                        EndMember,
                        VarSpace(50),
                        negative ? TAG_IGNORE : TAG_SKIP, 1,
                        StartMember,
                            ar->Cancel = MyButtonObject, GA_ID, GID_AR_CANCEL,
                                ULabel( negative ),
                                helptext ? TAG_IGNORE : TAG_SKIP,1,
                                    BT_HelpText, helptext,
                                helpnode ? TAG_IGNORE : TAG_SKIP,2,
                                    BT_HelpHook, &HelpHook,
                                    BT_HelpNode, helpnode,
                            EndObject,
                        EndMember,
                    EndObject, FixMinHeight, Weight(10),
                EndMember,
            EndObject,
    EndObject;

    return res;
}

SAVEDS ASM PERROR AskReqA( REG(a0) FRAME *frame, REG(a1) struct TagItem *list, REG(a6) EXTBASE *ExtBase )
{
    PERROR res = PERR_OK;
    APTR UtilityBase = ExtBase->lb_Utility, SysBase = ExtBase->lb_Sys, IntuitionBase = ExtBase->lb_Intuition;
    FRAME *pwframe, *tempframe;
    LONG armValues[MAX_AROBJECTS];
    struct ARUpdateMsg aum = {0};
    struct ARRenderMsg arm = {0};
    struct ARArgs ar = {0};
    struct RealObject RealObjs[MAX_AROBJECTS] = {0};
    struct Library *BGUIFloatBase;

    D(bug("AskReqA()\n"));

    /*
     *  Attempt to open libs
     */

    if(BGUIFloatBase = OpenLibrary( "Gadgets/bgui_float.gadget", 0L ) ) {
        D(bug("\tOpened BGUI float gadget\n"));
        ExtBase->FloatClass = GetFloatClassPtr();
    }

    /*
     *  Setting up auto variables
     */

    aum.aum_Values = armValues;
    arm.arm_Values = armValues;

    if( frame ) {
        if( frame->parent ) {
            pwframe   = frame->parent->preview.pf_Frame;
            tempframe = frame->parent->preview.pf_TempFrame;
        } else {
            pwframe   = frame->preview.pf_Frame;
            tempframe = frame->preview.pf_TempFrame;
        }
    }

    /*
     *  Initialize the ar window and try to create it
     */

    ar.frame    = frame;
    ar.pwframe  = pwframe;
    ar.artags   = list;

    GetARWindow( &ar, RealObjs, ExtBase );

    /*
     *  Open the window and enter IDMCP handler loop
     */

    if(ar.Win) {
        struct Window *win;

        win = WindowOpen( ar.Win );
        if(win) {
            ULONG sigmask;
            BOOL quit = FALSE;
            struct IBox *ibox;

            GetAttr( WINDOW_SigMask, ar.Win, &sigmask );

            /*
             *  Set up the previewer.
             */

            if( HasPreview( frame ) ) {
                D(bug("\tSetting up preview\n"));
                frame->preview.pf_win = win;
                frame->preview.pf_RenderArea = ar.renderArea;

                /*
                 *  Set up initial values
                 */

                aum.MethodID    = ARM_UPDATE;
                arm.MethodID    = ARM_RENDER;
                aum.aum_Frame   = arm.arm_Frame     = tempframe;
                aum.aum_ExtBase = arm.arm_ExtBase   = ExtBase;
                aum.aum_RPort   = arm.arm_RPort     = win->RPort;

                /*
                 *  If this is a preview, we will now redraw this once.  This also serves
                 *  as reading in the initial values.
                 */

                CopyFrameData( pwframe, tempframe, 0L, ExtBase ); // BUG: Redundant?
                ReadARGadgetsToArray( ar.Win, RealObjs, armValues, ExtBase );

                if( RealObjs[ar.hookobjectid].hook)
                    res = CallHookPkt(RealObjs[ar.hookobjectid].hook, RealObjs[ar.hookobjectid].obj, &aum );
            }

            while(!quit) {
                ULONG sig;
                PROFILE_OFF();
                sig = Wait( sigmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F );
                PROFILE_ON();

                if(sig & SIGBREAKF_CTRL_C) {
                    res = PERR_BREAK;
                    quit = TRUE;
                }

                if( sig & SIGBREAKF_CTRL_F ) {
                    WindowToFront( win );
                    ActivateWindow( win );
                }

                if( sig & sigmask ) {
                    ULONG rc2;

                    while(( rc2 = HandleEvent( ar.Win )) != WMHI_NOMORE ) {

                        switch(rc2) {
                            case WMHI_CLOSEWINDOW:
                            case GID_AR_CANCEL:
                                quit = TRUE;
                                res = PERR_CANCELED;
                                break;

                            case GID_AR_OK:
                                quit = TRUE;
                                res  = PERR_OK;
                                ReadARGadgets( ar.Win, RealObjs, list, ExtBase );
                                break;

                            case GID_AR_RENDERAREA:
                                GetAttr(AREA_AreaBox, ar.renderArea, (ULONG *)&ibox );
                                D(bug("\tComplete refresh of RenderArea: (h=%d,w=%d)\n",ibox->Height,ibox->Width));

                                if( ar.renderHook ) {
                                    arm.arm_Area = *ibox;
                                    CallHookPkt( ar.renderHook, NULL, &arm );
                                } else {
                                    RenderFrame( tempframe, win->RPort,
                                                 ibox, 0, ExtBase );
                                }
                                break;

                            default:
                                if( rc2 >= GID_START && rc2 < GID_START+MAX_AROBJECTS && HasPreview( frame ) ) {
                                    ULONG objid;
                                    objid = rc2 - GID_START;

                                    if( RealObjs[objid].type != AR_StringObject && RealObjs[objid].hook ) {
                                        ULONG res;

                                        CopyFrameData( pwframe, tempframe, 0L, ExtBase );

                                        FetchARGadgetValue( &RealObjs[objid], (ULONG *)&aum.aum_Values[objid], ExtBase );
                                        aum.aum_ObjectID = objid;
                                        GetAttr(AREA_AreaBox, ar.renderArea, (ULONG *)&ibox );
                                        aum.aum_Area = *ibox;

                                        res = CallHookPkt(RealObjs[objid].hook,
                                                          RealObjs[objid].obj,
                                                          &aum );

                                        if( res == ARR_REDRAW ) {
                                            GetAttr(AREA_AreaBox, ar.renderArea, (ULONG *)&ibox );
                                            D(bug("\tRenderArea: (h=%d,w=%d)\n",ibox->Height, ibox->Width));
                                            RenderFrame( tempframe, win->RPort,
                                                         ibox, 0, ExtBase );
                                        }

                                    }
                                }
                                break;
                        }
                    }
                }
            }
        } else {
            D(bug("\tUnable to open window\n"));
            res = PERR_WONTOPEN;
        }
        if( res == PERR_OK ) {

            /*
             *  Fetch and save the window position for the next time this routine
             *  is called with the same window title
             */

            GetAttr(WINDOW_Bounds, ar.Win, (ULONG *) &(ar.awd.winpos) );
            PutOptions( ar.awdname, &ar.awd, sizeof(struct AskReqWinData), ExtBase );
        }

        DisposeObject( ar.Win );

    } else {
        D(bug("\tUnable to create window object\n"));
        res = PERR_WONTOPEN;
    }

    /*
     *  Finish up
     */

    if( BGUIFloatBase ) CloseLibrary( BGUIFloatBase );

    return res;
}

#ifdef DEBUG_MODE
/*-----test taglist-----*/

#pragma msg 186 ignore

LONG foo2, foo1, foo3, foo4;
char foostring[81];
struct Hook testhook = { 0 };

struct TagItem myslider[] = {
    ARSLIDER_Min, 0,
    ARSLIDER_Max, 100,
    ARSLIDER_Default, 50,
    AROBJ_Value, &foo2,
//    AROBJ_PreviewHook, &testhook,
    AROBJ_HelpNode, "PPT.guide/Main",
    TAG_END
};

struct TagItem myfloat[] = {
    ARFLOAT_Min, -100,
    ARFLOAT_Max, 100,
    ARFLOAT_Divisor, 100,
    ARFLOAT_Default, 20,
    ARFLOAT_FormatString, "%.2f",
    AROBJ_Value, &foo4,
    TAG_DONE
};

struct TagItem mystring[] = {
    ARSTRING_InitialString, "A test string.",
    ARSTRING_MaxChars, 80,
    AROBJ_Value, foostring,
    AROBJ_HelpText,"This is a simple help text\n"
                   "which should be shown in a separate\n"
                   "requester window.",
    TAG_END
};


struct TagItem mycheckbox[] = {
    ARCHECKBOX_Selected, TRUE,
    AROBJ_Value, &foo1,
    AROBJ_Label, "Select this!",
//    AROBJ_PreviewHook, &testhook,
    TAG_END
};

const char *names[] = { "Selection 1", "Selection 2", "Selection 3", NULL };

struct TagItem mycycle[] = {
    ARCYCLE_Labels,     (ULONG)names,
    AROBJ_Value,        (ULONG)&foo3,
    ARCYCLE_Active,     1,
    ARCYCLE_Popup,      TRUE,
//    AROBJ_PreviewHook, &testhook,
    TAG_DONE
};

struct TagItem mywindow[] = {
    AR_Title, "Test window",
    AR_Text, "Do you wish to?",
    AR_Positive, "Yes",
    // AR_Negative, NULL,
    AR_SliderObject, myslider,
    AR_StringObject, mystring,
    AR_CheckBoxObject, mycheckbox,
    AR_CycleObject, mycycle,
    AR_FloatObject, myfloat,
    TAG_END
};


SAVEDS ASM
ULONG TestARHook( REG(a0) struct Hook *hook,
                  REG(a2) APTR         object,
                  REG(a1) struct ARUpdateMsg *msg )
{
    D(bug("TESTARHOOK: Got message:\n\tObjid = %ld, frame = %08X, value = %ld\n",
           msg->aum_ObjectID, msg->aum_Frame, msg->aum_Values[0] ));
    return 0;
}

void TestAR(void)
{
    testhook.h_Entry = (HOOKFUNC *)TestARHook;

    if(AskReqA( NULL, mywindow, globxd ) == PERR_OK) {
        D(bug("Slider value @ %08X is %ld\n",&foo2,foo2));
        D(bug("Checkbox value @ %08X is %ld\n",&foo1,foo1));
        D(bug("String value @ %08X is '%s'\n", foostring, foostring ));
        D(bug("Cycle value @ %08X is %ld\n",&foo3, foo3 ));
        D(bug("Float value @ %08X is %ld\n",&foo4, foo4 ));
    } else {
        D(bug("User cancelled or some other mistake\n"));
    }
}

#pragma msg 186 warn

#endif


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

