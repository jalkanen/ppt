/*
    PROJECT: ppt
    MODULE:  toolbar.h

    $Id: toolbar.h,v 1.1 1998/12/20 19:11:07 jj Exp $
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

#define TBBASE                  (4000)
#define TOOLBAR_Items           (TBBASE + 0)    /* struct ToolbarItem * */

/*
 *  Methods
 */


#endif
