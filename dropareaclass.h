/*
    PROJECT: ppt
    MODULE : areaclass.h

    $Id: dropareaclass.h,v 1.4 1997/08/30 21:29:41 jj Exp $

    These are for AreaClass.c
*/

#ifndef DROPAREACLASS_H
#define DROPAREACLASS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASS_H
#include <intuition/classes.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define AREA_DropEntry          TAG_USER+0xCDF2 // (SG)

Class *InitDropAreaClass( void );
BOOL FreeDropAreaClass( Class * );

#endif
