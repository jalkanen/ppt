/*
    PROJECT: ppt
    MODULE : renderareaclass.h

    $Id: renderareaclass.h,v 1.1 1997/08/30 21:31:11 jj Exp $

    These are for renderareaclass.c
*/

#ifndef RENDERAREACLASS_H
#define RENDERAREACLASS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASS_H
#include <intuition/classes.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define RAC_Dummy           (TAG_USER+0x1111000)

#define RAC_Frame           (RAC_Dummy + 0)  // IG
#define RAC_ExtBase         (RAC_Dummy + 1)  // I     MUST ALWAYS EXIST!!!

#endif
