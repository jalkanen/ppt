/*
**             File: paletteclass.c
**      Description: BOOPSI (BGUI) palette selector gadget class.
**        Copyright: (C) Copyright 1995 Jaba Development.
**                   (C) Copyright 1995 Jan van den Baard.
**                   All Rights Reserved.
**
**  Slight modifications for the PPT project:
**  $Id: paletteclass.c,v 1.1 1995/10/15 00:18:29 jj Exp $
**/

#include <defs.h>
#include <gui.h>

#include <graphics/gfxmacros.h>
#include <bgui.h>

#include <clib/alib_protos.h>

#include <pragma/intuition_pragmas.h>
#include <pragma/bgui_pragmas.h>
#include <pragma/utility_pragmas.h>
#include <pragma/graphics_pragmas.h>

#include <string.h>

#include "paletteclass.h"

Prototype Class *InitPaletteClass( VOID );
Prototype BOOL   FreePaletteClass( Class * );

Class *PaletteClass;

/*
**      Compiler stuff.
**/
#ifdef _DCC
#define SAVEDS __geta4
#define ASM
#define REG(x) __ ## x
#else
#define SAVEDS __saveds
#define ASM __asm
#define REG(x) register __ ## x
#endif

/*
**      Clamp a value in a range.
**/
#define CLAMP(v,i,a)    if ( v < i ) v = i; else if ( v > a ) v = a;

/*
**      Simple type-cast.
**/
#define GAD(x)          (( struct Gadget * )x)

/*
**      PaletteClass object instance data.
**/
typedef struct {
        UWORD           pd_ColorOffset;         /* Offset in palette/table.          */
        UWORD           pd_Depth;               /* Palette depth.                    */
        UWORD           pd_NumColors;           /* Number of colors.                 */
        UWORD          *pd_PenTable;            /* Table of pens.                    */
        UWORD           pd_CurrentColor;        /* Currently selected color.         */
        Object         *pd_Frame;               /* Frame for selected color.         */
        UWORD           pd_Columns;             /* Number of columns.                */
        struct IBox     pd_ColorBox;            /* Bounds of real color box.         */
        UWORD           pd_RectWidth;           /* Width of color rectangle.         */
        UWORD           pd_RectHeight;          /* Height of color rectangle.        */
        UWORD           pd_InitialColor;        /* Color selected when going active. */
        BOOL            pd_ResetInitial;        /* Reset initial color?              */
} PD;

/*
**      Make sure that "CurrentColor" is a valid
**      pen number in the colors which are displayed
**      in the palette object.
**/
STATIC ASM UWORD ValidateColor( REG(a0) PD *pd, REG(d0) ULONG pen )
{
        UWORD                   *tab = pd->pd_PenTable, i;

        /*
        **      Do we have a pen table?
        **/
        if ( tab ) {
                /*
                **      Check the pens in the table.
                **/
                for ( i = 0; i < pd->pd_NumColors; i++ ) {
                        /*
                        **      Does the pen exist in the
                        **      pen table?
                        **/
                        if ( pen == tab[ i + pd->pd_ColorOffset ] )
                                return( pen );
                }
                /*
                **      The pen does not exist in the table.
                **      Make it the first pen in the table.
                **/
                i = tab[ pd->pd_ColorOffset ];
        } else {
                /*
                **      Check if the pen number is
                **      in range.
                **/
                if ( pen >= pd->pd_ColorOffset && pen <= ( pd->pd_ColorOffset + pd->pd_NumColors - 1 ))
                        return( pen );
                /*
                **      No. Default to the first pen.
                **/
                i = pd->pd_ColorOffset;
        }

        return( i );
}

/*
**      Create a new palette object.
**/
STATIC ASM ULONG PaletteClassNew( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct opSet *ops )
{
        PD                      *pd;
        struct TagItem          *tstate = ops->ops_AttrList, *tag;
        Object                  *label;
        ULONG                    rc, place;

        /*
        **      Let the superclass make the object.
        **/
        if ( rc = DoSuperMethodA( cl, obj, ( Msg )ops )) {
                /*
                **      Get a pointer to the object
                **      it's instance data.
                **/
                pd = ( PD * )INST_DATA( cl, rc );

                /*
                **      Preset the data to 0. Don't
                **      know if this is necessary.
                **/
                bzero(( char * )pd, sizeof( PD ));

                /*
                **      Setup the default settings.
                **/
                pd->pd_Depth            = 1;
                pd->pd_NumColors        = 2;

                /*
                **      Setup the instance data.
                **/
                while ( tag = NextTagItem( &tstate )) {
                        switch ( tag->ti_Tag ) {

                                case    PALETTE_Depth:
                                        pd->pd_Depth     = tag->ti_Data;
                                        CLAMP( pd->pd_Depth, 1, 8 );
                                        pd->pd_NumColors = ( 1L << pd->pd_Depth );
                                        break;

                                case    PALETTE_ColorOffset:
                                        pd->pd_ColorOffset = tag->ti_Data;
                                        break;

                                case    PALETTE_PenTable:
                                        pd->pd_PenTable = ( UWORD * )tag->ti_Data;
                                        break;

                                case    PALETTE_CurrentColor:
                                        pd->pd_CurrentColor = tag->ti_Data;
                                        break;
                        }
                }

                /*
                **      Make sure the offset stays in range.
                **/
                CLAMP( pd->pd_ColorOffset,  0, 256 - pd->pd_NumColors );

                /*
                **      The color must remain OK.
                **/
                pd->pd_CurrentColor = ValidateColor( pd, pd->pd_CurrentColor );

                /*
                **      See if the object has a label
                **      attached to it.
                **/
                DoMethod(( Object * )rc, OM_GET, BT_LabelObject, &label );
                if ( label ) {
                        /*
                        **      Yes. Query the place because it may
                        **      not be PLACE_IN for obvious reasons.
                        **/
                        DoMethod( label, OM_GET, LAB_Place, &place );
                        if ( place == PLACE_IN )
                                SetAttrs( label, LAB_Place, PLACE_LEFT, TAG_END );
                }

                /*
                **      Create a frame for the
                **      currently selected pen.
                **/
                if ( pd->pd_Frame = BGUI_NewObject( BGUI_FRAME_IMAGE, FRM_Type, FRTYPE_BUTTON, FRM_Flags, FRF_RECESSED, TAG_END ))
                        return( rc );

                /*
                **      No frame means no object.
                **/
                CoerceMethod( cl, ( Object * )rc, OM_DISPOSE );
        }
        return( 0L);
}

/*
**      Dispose of the object.
**/
STATIC ASM ULONG PaletteClassDispose( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) Msg msg )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );

        /*
        **      Dispose of the frame.
        **/
        if ( pd->pd_Frame )     DoMethod( pd->pd_Frame, OM_DISPOSE );

        /*
        **      The superclass handles
        **      the rest.
        **/
        return( DoSuperMethodA( cl, obj, msg ));
}

/*
**      Get an attribute.
**/
STATIC ASM ULONG PaletteClassGet( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct opGet *opg )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );
        ULONG                    rc = 1L;

        /*
        **      We only know one attribute.
        **/
        if ( opg->opg_AttrID == PALETTE_CurrentColor )
                /*
                **      Pass on the currently
                **      selected color.
                **/
                *( opg->opg_Storage ) = pd->pd_CurrentColor;
        else
                /*
                **      Everything else goes
                **      to the superclass.
                **/
                rc = DoSuperMethodA( cl, obj, ( Msg )opg );

        return( rc );
}

/*
**      Render the color rectangles
**      inside the palette object it's
**      color box.
**/
STATIC ASM VOID RenderColorRects( REG(a0) PD *pd, REG(a1) struct RastPort *rp, REG(a2) struct IBox *area, REG(a3) struct DrawInfo *dri )
{
        UWORD           colorwidth, colorheight, columns = 1, rows = 1, depth = pd->pd_Depth;
        UWORD           hadjust = 0, vadjust = 0, left, top, colsize, rowsize, c, r, color;
        UWORD           offset = pd->pd_ColorOffset;

        /*
        **      Get the first color of the
        **      displayed palette.
        **/
        if ( pd->pd_PenTable ) color = pd->pd_PenTable[ offset ];
        else                   color = offset;

        /*
        **      Get the Width and height of the
        **      area in which we layout the
        **      color rectangles.
        **/
        colorwidth  = area->Width;
        colorheight = area->Height;

        /*
        **      Layout the color
        **      rectangles.
        **/
        while ( depth ) {
                if (( colorheight << 1 ) > colorwidth ) {
                        colorheight >>= 1;
                        rows        <<= 1;
                } else {
                        colorwidth  >>= 1;
                        columns     <<= 1;
                }
                depth--;
        }

        /*
        **      Compute the pixel width and
        **      height of the color rectangles.
        **/
        colsize = area->Width  / columns;
        rowsize = area->Height / rows;

        /*
        **      Since this is not precise
        **      we compute the space left
        **      over to adjust the left
        **      and top offset at which we
        **      begin.
        **
        **      Also we re-compute the color
        **      box dimensions.
        **/
        hadjust = ( area->Width  - ( area->Width  = colsize * columns )) >> 1;
        vadjust = ( area->Height - ( area->Height = rowsize * rows    )) >> 1;

        /*
        **      Adjust the colorbox position.
        **/
        area->Left += hadjust;
        area->Top  += vadjust;

        /*
        **      Get initial left and top offset.
        **/
        left = area->Left + 1;
        top  = area->Top  + 1;

        /*
        **      No patterns!
        **/
        SetAfPt( rp, NULL, 0 );

        /*
        **      Set these up. We need this information to
        **      compute the color the mouse is over.
        **/
        pd->pd_Columns    = columns;
        pd->pd_RectWidth  = colsize;
        pd->pd_RectHeight = rowsize;

        /*
        **      Now render the rectangles.
        **/
        for ( r = 0; r < rows; r++ ) {
                for ( c = 0; c < columns; c++ ) {
                        /*
                        **      The currently selected color
                        **      is done with a frameclass object.
                        **/
                        if ( color == pd->pd_CurrentColor ) {
                                /*
                                **      Setup the object.
                                **/
                                SetAttrs( pd->pd_Frame, IA_Left,        left,
                                                        IA_Top,         top,
                                                        IA_Width,       colsize - 1,
                                                        IA_Height,      rowsize - 1,
                                                        FRM_BackPen,    color,
                                                        TAG_END );
                                /*
                                **      Render it.
                                **/
                                DrawImageState( rp, ( struct Image * )pd->pd_Frame, 0, 0, IDS_NORMAL, dri );
                        } else {
                                /*
                                **      Other colors we do ourselves.
                                **/
                                SetAPen( rp, color );
                                RectFill( rp, left, top, left + colsize - 2, top + rowsize - 2 );
                        }
                        left += colsize;
                        /*
                        **      Get the next color from the
                        **      displayed palette.
                        **/
                        if ( pd->pd_PenTable ) color = pd->pd_PenTable[ ++offset ];
                        else                   color++;
                }
                left = area->Left + 1;
                top += rowsize;
        }
}

/*
**      Render the palette object.
**/
STATIC ASM ULONG PaletteClassRender( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct gpRender *gpr )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );
        struct RastPort          rp = *gpr->gpr_RPort;
        struct DrawInfo         *dri = gpr->gpr_GInfo->gi_DrInfo;
        struct IBox             *bounds;
        Object                  *frame;
        ULONG                    fw = 0, fh = 0;
        static UWORD             dispat[ 2 ] = { 0x2222, 0x8888 };
        ULONG                    rc;

        /*
        **      First we let the superclass
        **      render. If it returns 0 we
        **      do not render!
        **/
        if ( rc = DoSuperMethodA( cl, obj, ( Msg )gpr )) {
                /*
                **      Get the hitbox bounds of the object
                **      and copy it's contents. We need to
                **      copy the data because we must adjust
                **      it's contents.
                **/
                DoMethod( obj, OM_GET, BT_HitBox, &bounds );
                pd->pd_ColorBox = *bounds;

                /*
                **      Do we have a frame?
                **/
                DoMethod( obj, OM_GET, BT_FrameObject, &frame );
                if ( frame ) {
                        /*
                        **      Find out the frame thickness.
                        **/
                        DoMethod( frame, OM_GET, FRM_FrameWidth,  &fw );
                        DoMethod( frame, OM_GET, FRM_FrameHeight, &fh );
                        fw++;
                        fh++;

                        /*
                        **      Adjust bounds accoordingly.
                        **/
                        pd->pd_ColorBox.Left      += fw;
                        pd->pd_ColorBox.Top       += fh;
                        pd->pd_ColorBox.Width     -= fw << 1;
                        pd->pd_ColorBox.Height    -= fh << 1;
                }

                /*
                **      Render the color rectangles.
                **/
                RenderColorRects( pd, &rp, &pd->pd_ColorBox, dri );

                /*
                **      Disabled?
                **/
                if ( GAD( obj )->Flags & GFLG_DISABLED ) {
                        SetAPen( &rp, dri ? dri->dri_Pens[ SHADOWPEN ] : 2 );
                        SetDrMd( &rp, JAM1 );
                        SetAfPt( &rp, dispat, 1 );
                        RectFill( &rp, bounds->Left,
                                       bounds->Top,
                                       bounds->Left + bounds->Width  - 1,
                                       bounds->Top  + bounds->Height - 1 );
                }

        }
        return( rc );
}

/*
**      Get the pen number of
**      ordinal color number "num".
**/
STATIC ASM ULONG GetPenNumber( REG(a0) PD *pd, REG(d0) ULONG num )
{
        /*
        **      Return the pen number
        **      from the pen table or...
        **/
        if ( pd->pd_PenTable ) return( pd->pd_PenTable[ pd->pd_ColorOffset + num ] );

        /*
        **      From the screen palette.
        **/
        return( num + pd->pd_ColorOffset );
}

/*
**      Determine the ordinal number
**      of the pen in the object.
**/
STATIC ASM ULONG GetOrdinalNumber( REG(a0) PD *pd, REG(d0) ULONG pen )
{
        UWORD                   *tab = pd->pd_PenTable, i;

        /*
        **      Do we have a pen table?
        **/
        if ( tab ) {
                /*
                **      Look up the pen in the table.
                **/
                for ( i = 0; i < pd->pd_NumColors; i++ ) {
                        /*
                        **      Is this the one?
                        **/
                        if ( tab[ i + pd->pd_ColorOffset ] == pen )
                                return( i );
                }
        }

        /*
        **      Return the ordinal palette pen.
        **/
        return( pen - pd->pd_ColorOffset );
}

/*
**      Determine which color rectangle
**      the coordinates are in.
**/
STATIC ASM UWORD GetColor( REG(a0) PD *pd, REG(d0) ULONG x, REG(d1) ULONG y )
{
        UWORD           col, row;

        /*
        **      Compute the row and column
        **      we clicked on.
        **/
        row = y / pd->pd_RectHeight;
        col = x / pd->pd_RectWidth;

        /*
        **      With this information we can simple
        **      compute and return the color under
        **      these coordinates.
        **/
        return( GetPenNumber( pd, ( row * pd->pd_Columns ) + col ));
}

/*
**      Get the top-left position of
**      a color in the color box.
**/
STATIC ASM GetTopLeft( REG(a0) PD *pd, REG(d0) UWORD color, REG(a1) UWORD *x, REG(a2) UWORD *y )
{
        UWORD                   row, col;

        /*
        **      First compute the row and column
        **      of the color.
        **/
        row = color / pd->pd_Columns;
        col = color - ( row * pd->pd_Columns );

        /*
        **      Now we can simply get the
        **      x/y position of the color
        **      box.
        **/
        *x = pd->pd_ColorBox.Left + ( col * pd->pd_RectWidth  );
        *y = pd->pd_ColorBox.Top  + ( row * pd->pd_RectHeight );
}

/*
**      Notify about an attribute change.
**/
STATIC ULONG NotifyAttrChange( Object *obj, struct GadgetInfo *gi, ULONG flags, Tag tag1, ... )
{
        return( DoMethod( obj, OM_NOTIFY, &tag1, gi, flags ));
}

/*
**      Change the currently selected color.
**/
STATIC ASM VOID ChangeSelectedColor( REG(a0) PD *pd, REG(a1) struct GadgetInfo *gi, REG(d0) ULONG newcolor )
{
        struct RastPort         *rp;
        UWORD                    l, t;

        /*
        **      Allocate a rastport.
        **/
        if ( gi && ( rp = ObtainGIRPort( gi ))) {
                /*
                **      First pickup the coordinates
                **      of the currently selected
                **      color.
                **/
                GetTopLeft( pd, GetOrdinalNumber( pd, pd->pd_CurrentColor ), &l, &t );

                /*
                **      Render this color rectangle
                **      as not-selected.
                **/
                SetAPen( rp, pd->pd_CurrentColor );
                RectFill( rp, l + 1,
                              t + 1,
                              l + pd->pd_RectWidth  - 1,
                              t + pd->pd_RectHeight - 1 );

                /*
                **      Now pickup the coordinates
                **      of the new selected color.
                **/
                GetTopLeft( pd, GetOrdinalNumber( pd, newcolor ), &l, &t );

                /*
                **      Setup and render the frame to
                **      reflect the new data.
                **/
                SetAttrs( pd->pd_Frame, IA_Left,     l + 1,
                                        IA_Top,      t + 1,
                                        IA_Width,    pd->pd_RectWidth  - 1,
                                        IA_Height,   pd->pd_RectHeight - 1,
                                        FRM_BackPen, newcolor,
                                        TAG_END );

                DrawImageState( rp, ( struct Image * )pd->pd_Frame, 0, 0, IDS_NORMAL, gi->gi_DrInfo );

                /*
                **      Free up the rastport.
                **/
                ReleaseGIRPort( rp );
        }
        /*
        **      Setup the new color.
        **/
        pd->pd_CurrentColor = newcolor;
}

/*
**      Set attributes.
**/
STATIC ASM ULONG PaletteClassSet( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct opUpdate *opu )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );
        struct TagItem          *tag;
        ULONG                    rc, new;

        /*
        **      First the superclass.
        **/
        rc = DoSuperMethodA( cl, obj, ( Msg )opu );

        /*
        **      Frame thickness change? When the window in which
        **      we are located has WINDOW_AutoAspect set to TRUE
        **      the windowclass distributes the FRM_ThinFrame
        **      attribute to the objects in the window. Here we
        **      simply intercept it to set the selected color
        **      frame thickness.
        **/
        if ( tag = FindTagItem( FRM_ThinFrame, opu->opu_AttrList ))
                /*
                **      Set it to the frame.
                **/
                SetAttrs( pd->pd_Frame, FRM_ThinFrame, tag->ti_Data, TAG_END );

        /*
        **      Color change?
        **/
        if ( tag = FindTagItem( PALETTE_CurrentColor, opu->opu_AttrList )) {
                /*
                **      Make sure it's valid.
                **/
                new = ValidateColor( pd, tag->ti_Data );
                /*
                **      Did it really change?
                **/
                if ( new != pd->pd_CurrentColor ) {
                        /*
                        **      Yes. Show it and notify
                        **      the change.
                        **/
                        ChangeSelectedColor( pd, opu->opu_GInfo, new );
                        NotifyAttrChange( obj, opu->opu_GInfo, opu->MethodID == OM_UPDATE ? opu->opu_Flags : 0L, GA_ID, GAD( obj )->GadgetID, PALETTE_CurrentColor, pd->pd_CurrentColor, TAG_END );
                }
        }
        return( rc );
}

/*
**      Let's go active :)
**/
STATIC ASM ULONG PaletteClassGoActive( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct gpInput *gpi )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );
        WORD                     l, t;
        UWORD                    newcol;
        ULONG                    rc = GMR_NOREUSE;

        /*
        **      We do not go active when we are
        **      disabled or when we where activated
        **      by the ActivateGadget() call.
        **/
        if (( GAD( obj )->Flags & GFLG_DISABLED ) || ( ! gpi->gpi_IEvent ))
                return( rc );

        /*
        **      Save color selected when going
        **      active. This way we can reset
        **      the initial color when the
        **      gadget activity is aborted by
        **      the user or intuition.
        **/
        pd->pd_InitialColor = pd->pd_CurrentColor;

        /*
        **      Get the coordinates relative
        **      to the top-left of the colorbox.
        **/
        l = gpi->gpi_Mouse.X - ( pd->pd_ColorBox.Left - GAD( obj )->LeftEdge );
        t = gpi->gpi_Mouse.Y - ( pd->pd_ColorBox.Top  - GAD( obj )->TopEdge  );

        /*
        **      Are we really hit?
        **/
        if ( l >= 0 && t >= 0 && l < pd->pd_ColorBox.Width && t < pd->pd_ColorBox.Height ) {
                /*
                **      Did the color change?
                **/
                if (( newcol = GetColor( pd, l, t )) != pd->pd_CurrentColor ) {
                        /*
                        **      Yes. Setup the new color and send
                        **      a notification.
                        **/
                        ChangeSelectedColor( pd, gpi->gpi_GInfo, newcol );
                        NotifyAttrChange( obj, gpi->gpi_GInfo, 0L, GA_ID, GAD( obj )->GadgetID, PALETTE_CurrentColor, pd->pd_CurrentColor, TAG_END );
                }
                /*
                **      Go active.
                **/
                rc = GMR_MEACTIVE;
        }
        return( rc );
}

/*
**      Handle the user input.
**/
STATIC ASM ULONG PaletteClassHandleInput( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct gpInput *gpi )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );
        WORD                     l, t;
        UWORD                    newcol;
        ULONG                    rc = GMR_MEACTIVE;

        /*
        **      Get the coordinates relative
        **      to the top-left of the colorbox.
        **/
        l = gpi->gpi_Mouse.X - ( pd->pd_ColorBox.Left - GAD( obj )->LeftEdge );
        t = gpi->gpi_Mouse.Y - ( pd->pd_ColorBox.Top  - GAD( obj )->TopEdge  );

        /*
        **      Mouse pointer located over the object?
        **/
        if ( l >= 0 && t >= 0 && l < pd->pd_ColorBox.Width && t < pd->pd_ColorBox.Height ) {
                /*
                **      Mouse over a new color?
                **/
                if (( newcol = GetColor( pd, l, t )) != pd->pd_CurrentColor ) {
                        /*
                        **      Change the selected color.
                        **/
                        ChangeSelectedColor( pd, gpi->gpi_GInfo, newcol );
                        /*
                        **      Send notification
                        **      about the change.
                        **/
                        NotifyAttrChange( obj, gpi->gpi_GInfo, 0L, GA_ID, GAD( obj )->GadgetID, PALETTE_CurrentColor, pd->pd_CurrentColor, TAG_END );
                }
        }

        /*
        **      Check mouse input.
        **/
        if ( gpi->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE ) {
                switch ( gpi->gpi_IEvent->ie_Code ) {

                        case    SELECTUP:
                                /*
                                **      Left-mouse button up means we
                                **      return GMR_VERIFY.
                                **/
                                rc = GMR_NOREUSE | GMR_VERIFY;
                                break;

                        case    MENUDOWN:
                                /*
                                **      The menu button aborts the
                                **      selection.
                                **/
                                pd->pd_ResetInitial = TRUE;
                                rc = GMR_NOREUSE;
                                break;
                }
        }
        return( rc );
}

/*
**      Go inactive.
**/
STATIC ASM ULONG PaletteClassGoInactive( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct gpGoInactive *ggi )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );

        /*
        **      Reset initial color?
        **/
        if ( pd->pd_ResetInitial || ggi->gpgi_Abort == 1 ) {
                /*
                **      Reset color.
                **/
                ChangeSelectedColor( pd, ggi->gpgi_GInfo, pd->pd_InitialColor );
                /*
                **      Notification of the reset.
                **/
                NotifyAttrChange( obj, ggi->gpgi_GInfo, 0L, GA_ID, GAD( obj )->GadgetID, PALETTE_CurrentColor, pd->pd_CurrentColor, TAG_END );
                /*
                **      Clear reset flag.
                **/
                pd->pd_ResetInitial = FALSE;
        }

        return( DoSuperMethodA( cl, obj, ( Msg )ggi ));
}

/*
**      Tell'm our minimum dimensions.
**/
STATIC ASM ULONG PaletteClassDimensions( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct grmDimensions *dim )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );
        UWORD                    mx, my;
        ULONG                    rc;

        /*
        **      First the superclass.
        **/
        rc = DoSuperMethodA( cl, obj, ( Msg )dim );

        /*
        **      Now we pass our requirements. Note that
        **      it is a bit difficult for this type of
        **      gadgetclass to determine a suitable
        **      minimum size. It would probably be
        **      best to add tags for optional
        **      specification. I'll leave it at this
        **      for now.
        **/
        switch ( pd->pd_NumColors ) {

                case    2:
                case    4:
                case    8:
                        mx = 16;
                        my = 16;
                        break;

                case    16:
                case    32:
                        mx = 32;
                        my = 32;
                        break;

                default:
                        mx = 64;
                        my = 64;
                        break;
        }

        /*
        **      Store these values.
        **/
        *( dim->grmd_MinSize.Width  ) += mx;
        *( dim->grmd_MinSize.Height ) += my;

        return( rc );
}

/*
**      Key activation.
**/
STATIC ASM PaletteClassKeyActive( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) struct wmKeyInput *wmki )
{
        PD                      *pd = ( PD * )INST_DATA( cl, obj );
        UWORD                    qual = wmki->wmki_IEvent->ie_Qualifier, new;

        /*
        **      Get the ordinal number of
        **      the currently selected pen.
        **/
        new = GetOrdinalNumber( pd, pd->pd_CurrentColor );

        /*
        **      Shifted is backwards, normal forward.
        **/
        if ( qual & ( IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT )) {
                if ( new                           ) new--;
                else                                 new = pd->pd_NumColors - 1;
        } else {
                if ( new < ( pd->pd_NumColors - 1 )) new++;
                else                                 new = 0;
        }

        /*
        **      Change the color.
        **/
        ChangeSelectedColor( pd, wmki->wmki_GInfo, GetPenNumber( pd, new ));

        /*
        **      Notify the change.
        **/
        NotifyAttrChange( obj, wmki->wmki_GInfo, 0L, GA_ID, GAD( obj )->GadgetID, PALETTE_CurrentColor, pd->pd_CurrentColor, TAG_END );

        /*
        **      Setup ID to report to
        **      the event handler.
        **/
        *( wmki->wmki_ID ) = GAD( obj )->GadgetID;

        return( WMKF_VERIFY );
}

/*
**      The class dispatcher. Here's
**      where the fun starts.
**
**      SAS Users: You should either compile this module with
**                 stack checking turned off (NOSTACKCHECK) or
**                 you must use the "__interrupt" qualifier in
**                 this routine.
**/
STATIC SAVEDS ASM ULONG PaletteClassDispatch( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) Msg msg )
{
        ULONG                   rc;

        switch ( msg->MethodID ) {

                case    OM_NEW:
                        rc = PaletteClassNew( cl, obj, ( struct opSet * )msg );
                        break;

                case    OM_DISPOSE:
                        rc = PaletteClassDispose( cl, obj, msg );
                        break;

                case    OM_GET:
                        rc = PaletteClassGet( cl, obj, ( struct opGet * )msg );
                        break;

                case    OM_SET:
                case    OM_UPDATE:
                        rc = PaletteClassSet( cl, obj, ( struct opUpdate * )msg );
                        break;

                case    GM_RENDER:
                        rc = PaletteClassRender( cl, obj, ( struct gpRender * )msg );
                        break;

                case    GM_GOACTIVE:
                        rc = PaletteClassGoActive( cl, obj, ( struct gpInput * )msg );
                        break;

                case    GM_HANDLEINPUT:
                        rc = PaletteClassHandleInput( cl, obj, ( struct gpInput * )msg );
                        break;

                case    GM_GOINACTIVE:
                        rc = PaletteClassGoInactive( cl, obj, ( struct gpGoInactive * )msg );
                        break;

                case    GRM_DIMENSIONS:
                        rc = PaletteClassDimensions( cl, obj, ( struct grmDimensions * )msg );
                        break;

                case    WM_KEYACTIVE:
                        rc = PaletteClassKeyActive( cl, obj, ( struct wmKeyInput * )msg );
                        break;

                default:
                        rc = DoSuperMethodA( cl, obj, msg );
                        break;
        }
        return( rc );
}

/*
**      Initialize the class.
**/
Class *InitPaletteClass( void )
{
        Class                   *super, *cl = NULL;

        /*
        **      Obtain the BaseClass pointer which
        **      will be our superclass.
        **/
        if ( super = BGUI_GetClassPtr( BGUI_BASE_GADGET )) {
                /*
                **      Create the class.
                **/
                if ( cl = MakeClass( NULL, NULL, super, sizeof( PD ), 0L ))
                        /*
                        **      Setup dispatcher.
                        **/
                        cl->cl_Dispatcher.h_Entry = ( HOOKFUNC )PaletteClassDispatch;
        }
        return( cl );
}

/*
**      Kill the class.
**/
BOOL FreePaletteClass( Class *cl )
{
        return( FreeClass( cl ));
}
