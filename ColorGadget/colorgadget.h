#ifndef GADGETS_COLORGADGET_H
#define GADGETS_COLORGADGET_H

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>

#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>
#include <graphics/scale.h>
#include <graphics/gfxbase.h>
#include <gadgets/colorwheel.h>
#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>

#define CBASE               (TAG_USER+0xBADF00D)

#define COLOR_RGB           (CBASE + 1)     /* (struct ColorWheelRGB *) ISG   */
#define COLOR_HSV           (CBASE + 2)     /* (struct ColorWheelHSV *) ISG   */

/*
 *  COLOR_ColorWheel works only under V39.  If it's used, you MUST specify
 *  COLOR_Screen, otherwise this won't work.
 */

#define COLOR_ColorWheel    (CBASE + 3)     /* BOOL                     I     */
#define COLOR_Screen        (CBASE + 4)     /* struct Screen *          I     */

#ifndef __SLIB
Class *InitColorClass( VOID );
#endif
BOOL FreeColorClass( Class * );


#endif /* GADGETS_COLORGADGET_H */


