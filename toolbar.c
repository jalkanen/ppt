/*
    PROJECT: ppt
    MODULE:  toolbar.c

    This module contains toolbar handling and other associated
    stuff.

    PPT is (C) Janne Jalkanen 1998.

    $Id: toolbar.c,v 1.1 1998/10/25 22:42:54 jj Exp $

 */

#include "defs.h"
#include "misc.h"

/// UpdateMouseLocation()
/*
    Updates the mouse location in the tool window.

    BUG: Is now quite slow due to the TextExtent().
*/

Prototype VOID UpdateMouseLocation( WORD xloc, WORD yloc );

VOID UpdateMouseLocation( WORD xloc, WORD yloc )
{
    struct IBox *ibox;
    struct RastPort *rport;
    char buffer[40];
    struct TextExtent te;
    extern UWORD textpen;

    if( !toolw.win ) return;

    rport = toolw.win->RPort;

    GetAttr(AREA_AreaBox, toolw.GO_ToolInfo, (ULONG *)&ibox );

    if( xloc != -1 )
        sprintf(buffer,"(%3d,%3d)",xloc,yloc);
    else
        strcpy(buffer,"(---,---)");

    TextExtent(rport, buffer, strlen(buffer), &te);

    /* Clear the areabox and center the text. */

    EraseRect( rport, ibox->Left, ibox->Top,
                      ibox->Left+ibox->Width-1, ibox->Top+ibox->Height-1 );

    Move( rport, ibox->Left + ((ibox->Width-te.te_Width)>>1),
                 ibox->Top - te.te_Extent.MinY + 3 );
    SetAPen( rport, textpen );
    Text( rport, buffer, strlen(buffer) );
    WaitBlit(); /* Has to be done to ensure the buffer
                   is valid through the entire blit */
}
///
/// ClearMouseLocation()
Prototype VOID ClearMouseLocation(VOID);

VOID ClearMouseLocation(VOID)
{
    UpdateMouseLocation(-1,-1);
}
///

/// HandleToolIDCMP()
/*
    Handle the tool window IDCMP messages.
*/

Prototype int HandleToolIDCMP( ULONG );

int HandleToolIDCMP( ULONG rc )
{
    switch(rc) {
        case WMHI_CLOSEWINDOW:
            WindowClose( toolw.Win );
            toolw.win = NULL;
            CheckMenuItem( MID_TOOLWINDOW, FALSE );
            break;

        case GID_TOOL_LOAD:
            HandleMenuIDCMP( MID_LOADNEW, NULL, FROM_TOOLWINDOW );
            break;

        case GID_TOOL_PROCESS:
            HandleMenuIDCMP( MID_PROCESS, NULL, FROM_TOOLWINDOW );
            break;

        case GID_TOOL_COORDS:
            /*
             *  BUG: Should refresh with the current co-ordinates.
             */
            ClearMouseLocation();
            break;

        default:
            if(rc > MENUB && rc < 65536)
                return( HandleMenuIDCMP( rc, NULL, FROM_TOOLWINDOW ));
            break;
    }

    return HANDLER_OK;
}
///

