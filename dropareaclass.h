/*
    PROJECT: ppt
    MODULE : areaclass.h

    $Id: dropareaclass.h,v 1.2 1995/08/29 00:03:06 jj Exp $

    These are for AreaClass.c
*/

#ifndef AREACLASS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASS_H
#include <intuition/classes.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

//
//      AREA_MinWidth and AREA_MinHeight are required attributes.
//      Just pass the minimum area size you need here.
//
#define AREA_MinWidth           TAG_USER+0xCDEF // (I)
#define AREA_MinHeight          TAG_USER+0xCDF0 // (I)

//
//      When the ID of the area object is returned by the
//      event handler you should GetAttr() this attribute.
//      It will pass you a pointer (read only!) to the
//      current size of the area.
//
#define AREA_AreaBox            TAG_USER+0xCDF1 // (G)

Class *InitAreaClass( void );
BOOL FreeAreaClass( Class * );

#endif
