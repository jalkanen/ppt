/*
    PROJECT: ppt
    MODULE:  toolbar.h

    $Id: toolbar.h,v 1.2 1999/01/02 22:37:40 jj Exp $
*/
#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <exec/types.h>

/*
    Tool bar items
 */

typedef enum {
    TIT_TEXT,
    TIT_IMAGE,
    TIT_END
} ToolbarItemType;

struct ToolbarItem {
    ULONG               ti_GadgetID;
    ToolbarItemType     ti_Type;
    STRPTR              ti_FileName;
    STRPTR              ti_Label;
    Object              *ti_Gadget;
};

/*
 *  Tags and methods
 */

#define TBTBASE                 (TAG_USER+0x1900000)
#define TOOLBAR_Items           (TBTBASE + 0)    /* struct ToolbarItem * */

#define TBMBASE                 (0x1900000)
#define TOOLM_ADDSINGLE         (TBMBASE + 0)
#define TOOLM_ADDMULTI          (TBMBASE + 1)

struct toolAddSingle {
    ULONG               MethodID;
    struct GadgetInfo   *tas_GInfo;
    struct ToolbarItem  *tas_Item;
    LONG                tas_Number;
    ULONG               flags;
};

#define TASNUM_FIRST    0       /* Why, of course? */
#define TASNUM_LAST     -1

/*
 *  Methods
 */


#endif
