#ifndef PALETTECLASS_H
#define PALETTECLASS_H
/*
**	$VER: paletteclass.h 1.0 (27.5.95)
**	C Header for the BOOPSI palette gadget class.
**
**	(C) Copyright 1995 Jaba Development.
**	(C) Copyright 1995 Jan van den Baard.
**	    All Rights Reserved.
**/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/* Tags */
#define PALETTE_Depth		TAG_USER+0x70000	/* I---- */
#define PALETTE_ColorOffset	TAG_USER+0x70001	/* I---- */
#define PALETTE_PenTable	TAG_USER+0x70002	/* I---- */
#define PALETTE_CurrentColor	TAG_USER+0x70003	/* ISGNU */

/* TAG_USER+0x70004 through TAG_USER+0x700020 reserved. */

/* Prototypes */
Class *InitPaletteClass( void );
BOOL FreePaletteClass( Class * );

#endif
