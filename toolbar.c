/*
    PROJECT: ppt
    MODULE:  toolbar.c

    This module contains toolbar handling and other associated
    stuff.

    PPT is (C) Janne Jalkanen 1998.

    $Id: toolbar.c,v 1.2 1998/12/07 13:47:56 jj Exp $

 */

#include "defs.h"
#include "misc.h"
#include "gui.h"

/*------------------------------------------------------------------*/

typedef enum {
    TIT_TEXT,
    TIT_IMAGE,
    TIT_END
} ToolbarItemType;

struct ToolbarItem {
    ULONG               ti_GadgetID;
    ToolbarItemType     ti_Type;
    STRPTR              ti_FileName;
};

#define TextItem(x)   { (x), TIT_TEXT, NULL }
#define PicItem(x,a)  { (x), TIT_IMAGE, a }
#define EndItem       { 0L,  TIT_END, NULL }

#define BUFLEN 80
#define MAX_TOOLBARSIZE 30

/*------------------------------------------------------------------*/

struct ToolbarItem PPTToolbar[MAX_TOOLBARSIZE] = {
    TextItem( MID_LOADNEW ),
    TextItem( MID_PROCESS ),
    TextItem( MID_RENDER ),
    EndItem
};

const struct TagItem DefaultTags[] = {
    GROUP_Style, GRSTYLE_HORIZONTAL,
    Spacing(0),
    HOffset(0),
    VOffset(0),
    EqualWidth,
    TAG_DONE, 0L
};

const char *ToolbarOptions[] = {
    "END",
    "IMAGE",
    "TEXT",
    NULL
};

enum {
    TO_END,
    TO_IMAGE,
    TO_TEXT
} ToolbarOption;

/*------------------------------------------------------------------*/

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

/// FindNewMenuItem()
Local
struct NewMenu *FindNewMenuItem( struct NewMenu *menus, ULONG gadgetid )
{
    struct NewMenu *ptr = menus;

    while( ptr->nm_Type != NM_END ) {
        if( (ULONG)(ptr->nm_UserData) == gadgetid ) return ptr;
        ptr++;
    }
    return NULL;
}
///
/// CreateToolbar()
Prototype Object *CreateToolbar(VOID);

Object *CreateToolbar(VOID)
{
    Object *toolbar;
    struct TagItem *tags;
    struct ToolbarItem *tooldata = PPTToolbar;
    int    c = 0, item = 0;
    BOOL   quit = FALSE;

    if(!(tags = AllocateTagItems( 100 ))) // BUG: MAGIC
        return NULL;

    while( !quit ) {
        switch( tooldata[item].ti_Type ) {
            struct NewMenu *nm;

            case TIT_END:
                quit = TRUE;
                break;

            case TIT_TEXT:
                /*
                 *  Create a text item on the toolbar.  Note that the GUI localization
                 *  has already been done on the menu bar.
                 */

                if( nm = FindNewMenuItem( PPTMenus, tooldata[item].ti_GadgetID ) ) {
                    tags[c].ti_Tag   = GROUP_Member;
                    tags[c].ti_Data  = (ULONG)ButtonObject,
                                          UScoreLabel( nm->nm_Label, '_' ),
                                          GA_ID, tooldata[item].ti_GadgetID,
                                          XenFrame,
                                       EndObject;
                    c++;
                    tags[c].ti_Tag   = TAG_END;
                    tags[c].ti_Tag   = 0L;
                    c++;
                }
                break;
        }
        item++;
    }

    tags[c].ti_Tag  = TAG_MORE;
    tags[c].ti_Data = (ULONG)DefaultTags;

    toolbar = BGUI_NewObjectA( BGUI_GROUP_GADGET, tags );

    FreeTagItems( tags );

    return toolbar;
}
///

/// ReadToolbarConfig()
/*
 *  Reads in a configuration.  END finishes.  buffer is meant
 *  for reading.
 *  BUG: If goes over MAX_TOOLBARSIZE, will crash.
 */

Prototype PERROR ReadToolbarConfig( BPTR fh );

PERROR ReadToolbarConfig( BPTR fh )
{
    UBYTE buf[ BUFLEN+1 ], *s;
    struct ToolbarItem *tbi = PPTToolbar;
    char *tail;

    D(bug("BEGIN_TOOLBAR\n"));

    while( FGets( fh, buf, BUFLEN ) ) {
        if( buf[strlen(buf)-1] = '\n' ) buf[strlen(buf)-1] = '\0'; /* Remove the final CR */

        s = strtok( buf, " \t" );
        if( s ) {
            switch( GetOptID( ToolbarOptions, s ) ) {
                case TO_END:
                    tbi->ti_Type = TIT_END;
                    return PERR_OK;

                case TO_IMAGE:
                    break;

                case TO_TEXT:
                    s = strtok( NULL, " \t\n" ); /* s = 2nd arg */
                    tbi->ti_Type = TIT_TEXT;
                    tbi->ti_GadgetID = strtol( s, &tail, 0 );
                    D(bug("\tRead in gadget %lu\n",tbi->ti_GadgetID));
                    tbi++;
                    break;

                case GOID_COMMENT:
                    D(bug("\tSkipped comment\n"));
                    break;

                case GOID_UNKNOWN:
                    D(bug("\tUnknown thingy '%s'\n",buf));
            }
        }

    }
}
///
/// WriteToolbarConfig()
Prototype PERROR WriteToolbarConfig( BPTR fh );

PERROR WriteToolbarConfig( BPTR fh )
{
    struct ToolbarItem *tbi = PPTToolbar;
    char buf[BUFLEN];

    FPuts( fh, "BEGINTOOLBAR\n" );

    while( tbi->ti_Type != TIT_END ) {
        switch( tbi->ti_Type ) {
            case TIT_TEXT:
                sprintf( buf, "    TEXT %lu\n", tbi->ti_GadgetID );
                FPuts( fh, buf );
                break;

        }
        tbi++;
    }
    FPuts( fh, "END\n" );

    return PERR_OK;
}
///
