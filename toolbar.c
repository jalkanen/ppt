/*
    PROJECT: ppt
    MODULE:  toolbar.c

    This module contains toolbar handling and other associated
    stuff.

    PPT is (C) Janne Jalkanen 1998.

    $Id: toolbar.c,v 1.5 1999/01/02 22:36:30 jj Exp $

 */

#include "defs.h"
#include "misc.h"
#include "gui.h"

/*------------------------------------------------------------------*/

#define TextItem(x)   { (x), TIT_TEXT, NULL, NULL, NULL }
#define PicItem(x,a)  { (x), TIT_IMAGE, a, NULL, NULL }
#define EndItem       { 0L,  TIT_END, NULL, NULL, NULL }

#define BUFLEN 80
#define DEFAULT_TOOLBAR_WIDTH   200
#define MAX_TOOLBARSIZE 30

/*------------------------------------------------------------------*/

Class *ToolbarClass;

struct ToolbarItem PPTToolbar[MAX_TOOLBARSIZE+1] = {
    TextItem( MID_LOADNEW ),
    TextItem( MID_PROCESS ),
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

const LONG AvailableButtons[] = {
    MID_NEW,
    MID_LOADNEW,
    MID_LOADAS,
    MID_SAVEAS,
    MID_DELETE,
    MID_RENAME,
    MID_HIDE,
    MID_QUIT,

    MID_UNDO,
    MID_CUT,
    MID_CUTFRAME,
    MID_COPY,
    MID_PASTE,

    MID_ZOOMIN,
    MID_ZOOMOUT,
    MID_SELECTALL,
    MID_EDITINFO,
    MID_SETRENDER,
    MID_RENDER,
    MID_CLOSERENDER,
    MID_CORRECTASPECT,
    MID_EDITPALETTE,
    MID_LOADPALETTE,
    MID_SAVEPALETTE,

    MID_PROCESS,
    MID_BREAK,
    -1L
};

typedef struct {
    struct ToolbarItem  **td_Toolbar;
    int                 td_NumItems;
} TD;

/*------------------------------------------------------------------*/

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
/// FindNewMenuItemName()

Prototype struct NewMenu *FindNewMenuItemName( struct NewMenu *menus, STRPTR name );

struct NewMenu *FindNewMenuItemName( struct NewMenu *menus, STRPTR name )
{
    struct NewMenu *ptr = menus;

    while( ptr->nm_Type != NM_END ) {
        if( strcmp(ptr->nm_Label,name) == 0) return ptr;
        ptr++;
    }
    return NULL;
}
///

Prototype struct ToolbarItem *FindInToolbar( struct ToolbarItem *head, ULONG gid );

struct ToolbarItem *FindInToolbar( struct ToolbarItem *head, ULONG gid )
{
    int i;

    for( i = 0; head[i].ti_Type != TIT_END; i++ ) {
        if( head[i].ti_GadgetID == gid ) return &head[i];
    }
    return NULL;
}

Prototype struct ToolbarItem *FindInToolbarName( struct ToolbarItem *head, STRPTR name );

struct ToolbarItem *FindInToolbarName( struct ToolbarItem *head, STRPTR name )
{
    int i;

    for( i = 0; head[i].ti_Type != TIT_END; i++ ) {
        if( strcmp( head[i].ti_Label,name) == 0 ) return &head[i];
    }
    return NULL;
}




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

/*
 *  Inserts a tool bar item onto the list.  Also updates visuals.
 *  BUG: Currently only addtail()s
 */

Prototype VOID InsertToolItem( struct ToolbarItem *, struct ToolbarItem *, struct NewMenu * );

VOID InsertToolItem( struct ToolbarItem *toolbar, struct ToolbarItem *prev, struct NewMenu *nm )
{
    int i;

    for( i = 0; toolbar[i].ti_Type != TIT_END; i++ );

    if( i < MAX_TOOLBARSIZE ) {
        toolbar[i].ti_Type = TIT_TEXT;
        toolbar[i].ti_GadgetID = (ULONG)nm->nm_UserData;
        toolbar[i].ti_FileName = NULL;
        toolbar[i].ti_Label = nm->nm_Label;
        toolbar[i].ti_Gadget = NULL;

        toolbar[i+1].ti_Type = TIT_END;

        AddEntrySelect( prefsw.win, prefsw.ToolbarList, nm->nm_Label, LVAP_TAIL );
    }
}

Prototype VOID RemoveToolItem( struct ToolbarItem *toolbar, struct ToolbarItem *tb );

VOID RemoveToolItem( struct ToolbarItem *toolbar, struct ToolbarItem *tb )
{
    int i,j;

    for( i = 0; toolbar[i].ti_Type != TIT_END; i++ ) {
        if( toolbar[i].ti_GadgetID == tb->ti_GadgetID ) {
            for( j = i; toolbar[j].ti_Type != TIT_END; j++ ) {
                toolbar[j] = toolbar[j+1];
            }
            return;
        }
    }

    RemoveEntryVisible( prefsw.win, prefsw.ToolbarList, tb->ti_Label );
}

/// SetupToolbarList()
/*
 *  Go through the toolbar settings and set up the lists.
 */
Prototype VOID SetupToolbarList(VOID);

VOID SetupToolbarList(VOID)
{
    struct ToolbarItem *tooldata = PPTToolbar;
    int item = 0;
    LONG barcode;
    struct NewMenu *nm;
    D(APTR b);

    SetGadgetAttrs( GAD(prefsw.ToolItemType), prefsw.win, NULL,
                    MX_DisableButton, 1, TAG_DONE );

    D(bug("SetupToolbarList()\n"));
    D(b = StartBench());

    /*
     *  First, go through the menu items and place them on the bigger listview.
     */

    while( (barcode = AvailableButtons[item]) > 0 ) {

        nm = (APTR)FindNewMenuItem( PPTMenus, barcode );
        if( nm ) {
            if( !FindInToolbar( tooldata, barcode ) ) {
                AddEntry( NULL, prefsw.AvailButtons, nm->nm_Label, LVAP_TAIL );
            }
        }

        item++;
    }

    SortList( NULL, prefsw.AvailButtons );

    /*
     *  Now, set up the original list.
     */
    item = 0;
    while( tooldata[item].ti_Type != TIT_END ) {
        D(bug("AddEntry(%08x=%s)\n",&tooldata[item],tooldata[item].ti_Label ));
        AddEntry( NULL, prefsw.ToolbarList, tooldata[item].ti_Label, LVAP_TAIL );
        item++;
    }
    D(StopBench(b));
    D(bug("...finished building toolbar list\n"));
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

/// TOOLM_ADDSINGLE
BOOL STATIC ASM
ToolbarAddSingle( REGPARAM(a0,Class *,cl),
                  REGPARAM(a2,Object *,obj),
                  REGPARAM(a1,struct toolAddSingle *,tas) )
{
    struct ToolbarItem *ti = tas->tas_Item;
    TD *td = (TD*)INST_DATA(cl,obj);
    int item = td->td_NumItems;

    if(td->td_Toolbar[item] = smalloc( sizeof( struct ToolbarItem ) )) {

        bcopy( ti, td->td_Toolbar[item], sizeof(struct ToolbarItem) );

        /*
         *  Allocate room for label
         *  BUG: filename is missing
         */

        td->td_Toolbar[item]->ti_Label = smalloc( strlen(ti->ti_Label) + 1);
        strcpy( td->td_Toolbar[item]->ti_Label, ti->ti_Label );

        if(td->td_Toolbar[item]->ti_Gadget = ButtonObject,
            UScoreLabel( td->td_Toolbar[item]->ti_Label, '_' ),
            GA_ID,       td->td_Toolbar[item]->ti_GadgetID,
            XenFrame,
            BT_ToolTip,  td->td_Toolbar[item]->ti_Label,
            EndObject)
        {
            DoMethod( obj, GRM_ADDMEMBER, td->td_Toolbar[item]->ti_Gadget, TAG_DONE );
        } else {
            return FALSE;
        }

        td->td_NumItems++;

    }
    return TRUE;
}
///

/// AddItems()
BOOL
AddItems( Class *cl, Object *obj, TD *td, struct ToolbarItem *tb )
{
    while( tb->ti_Type != TIT_END ) {
        if( DoMethod( obj, TOOLM_ADDSINGLE, NULL, tb, -1, 0L ) == FALSE ) {
            return FALSE;
        }
        tb++;
    }

    return TRUE;
}
///

/// ToolbarNew()

ULONG STATIC ASM
ToolbarNew( REGPARAM(a0,Class *,cl),
            REGPARAM(a2,Object *,obj),
            REGPARAM(a1,struct opSet *,ops) )
{
    TD      *td;
    struct TagItem *tag, *tags = ops->ops_AttrList, *tstate;
    int    c = 0, item = 0;
    BOOL   quit = FALSE;
    ULONG rc;

    D(bug("Toolbar: OM_NEW\n"));
    /*
     *  BUG: Should really do a FilterTagItems(),
     *       then add own tags.
     */

    if( tag = FindTagItem( TAG_DONE, tags ) ) {
        tag->ti_Tag  = TAG_MORE;
        tag->ti_Data = DefaultTags;
    }

    if( rc = DoSuperMethodA( cl, obj, (Msg) ops )) {
        td = (TD*)INST_DATA(cl,rc);
        bzero( td, sizeof(TD) );

        td->td_Toolbar = smalloc( sizeof(struct ToolbarItem *) * MAX_TOOLBARSIZE );

        tstate = tags;

        while( tag = NextTagItem( &tstate ) ) {
            switch( tag->ti_Tag ) {
                case TOOLBAR_Items:
                    if( AddItems( cl, rc, td, tag->ti_Data ) == FALSE ) {
                        CoerceMethod(cl, (Object *)rc, OM_DISPOSE);
                        rc = NULL;
                    }
                    break;
            }
        }
    }

    return rc;
}
///
/// ToolbarDispose()
ULONG STATIC ASM
ToolbarDispose( REGPARAM(a0,Class *,cl),
                REGPARAM(a2,Object *,obj),
                REGPARAM(a1,Msg,msg) )
{
    TD *td = (TD*)INST_DATA(cl,obj);
    int i;

    D(bug("Toolbar: OM_DISPOSE\n"));

    if( td->td_Toolbar ) {
        for( i = 0; i < td->td_NumItems; i++ ) {
            if( td->td_Toolbar[i]) {
                if( td->td_Toolbar[i]->ti_Label ) sfree( td->td_Toolbar[i]->ti_Label );
                sfree( td->td_Toolbar[i] );
            }
        }
        sfree( td->td_Toolbar );
    }

    return( DoSuperMethodA( cl, obj, msg ));
}
///

/// GimmeToolBarWindow()
/*
    This creates and opens the tool bar window.
*/
PERROR GimmeToolBarWindow( VOID )
{
    PERROR res = PERR_OK;
    ULONG  args[2];
    struct IBox ibox;
    long   minwidth, minheight;
    struct TextExtent te;

    args[0] = args[1] = 0L;

    if( toolw.prefs.initialpos.Height == 0 ) {
        ibox.Left   = 0;
        if( MAINSCR )
            ibox.Top    = MAINSCR->Height;
        else
            ibox.Top    = 320;
    } else {
        ibox = toolw.prefs.initialpos;
    }

    ibox.Height = 0;
    ibox.Width  = DEFAULT_TOOLBAR_WIDTH;

    D(bug("GimmeToolBarWindow()\n"));

    LocalizeToolbar();

    /*
     *  Create the area for rendering the co-ordinates in.
     *  Allow two pixels spare on all sides.
     */

    if(MAINWIN && MAINWIN->RPort) {
        TextExtent( MAINWIN->RPort, "(MMM,MMM)MM", 11, &te );
        minwidth = te.te_Width+4;
        minheight = te.te_Height+4;
        D(bug("\tCoordBox minimum size = %ld x %ld pixels\n",minwidth, minheight ));
    } else {
        minwidth = 100;
        minheight = 12;
        InternalError("Unable to determine Main Window RastPort???");
        return PERR_WINDOWOPEN;
    }

    toolw.GO_ToolInfo = BGUI_NewObject( BGUI_AREA_GADGET,
                                        AREA_MinWidth,     minwidth,
                                        AREA_MinHeight,    minheight,
                                        BT_DropObject,     FALSE,
                                        GA_ID,             GID_TOOL_COORDS,
                                        ICA_TARGET,        ICTARGET_IDCMP,
                                        ButtonFrame,
                                        FRM_Flags,         FRF_RECESSED,
                                        BT_HelpHook,       &HelpHook,
                                        BT_HelpNode,       "PPT.guide/Toolbar",
                                        TAG_END );
    if( !toolw.GO_ToolInfo ) {
        D(bug("Toolinfo didn't open\n"));
        return PERR_WINDOWOPEN;
    }

    toolw.GO_ToolBar = NewObject( ToolbarClass, NULL,
                                  TOOLBAR_Items, &PPTToolbar,
                                  TAG_DONE );

    /*
     *  The window itself. The area is attached onto it.
     */

    if(!toolw.Win) {
        toolw.Win = WindowObject,
            WINDOW_Title,           GetStr(MSG_TOOLBAR_WINDOW),
            WINDOW_ScreenTitle,     std_ppt_blurb,
            WINDOW_MenuStrip,       PPTMenus,
            WINDOW_SharedPort,      MainIDCMPPort,
            WINDOW_Font,            globals->userprefs->mainfont,
            WINDOW_Screen,          MAINSCR,
            WINDOW_Bounds,          &ibox,
            WINDOW_SmartRefresh,    TRUE,
            WINDOW_SizeGadget,      FALSE,
            WINDOW_NoBufferRP,      TRUE,
            // WINDOW_HelpHook,            &HelpHook,
            // WINDOW_HelpNode,            "Toolbar",
            WINDOW_MasterGroup,
                HGroupObject, NarrowSpacing, NarrowHOffset, NarrowVOffset,
                    StartMember,
                        HGroupObject, Spacing(0), HOffset(0), VOffset(0),
                            StartMember,
                                toolw.GO_ToolInfo,
                            EndMember,
                        EndObject, FixMinWidth,
                    EndMember,
                    StartMember,
                        toolw.GO_ToolBar,
                    EndMember,
                EndObject, FixMinHeight,
            EndObject;
    }

    if(!toolw.Win) {
        D(bug("Tool window couldn't be created\n"));
        res = PERR_WINDOWOPEN;
    }

    return res;
}
///

Prototype PERROR LocalizeToolbar(VOID);

PERROR LocalizeToolbar(VOID)
{
    int item = 0;
    struct NewMenu *nm;

    while( PPTToolbar[item].ti_Type != TIT_END ) {
        nm = (APTR)FindNewMenuItem( PPTMenus, PPTToolbar[item].ti_GadgetID );
        if( nm ) {
            PPTToolbar[item].ti_Label = nm->nm_Label;
        }

        item++;
    }

    return PERR_OK;
}

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

STATIC DPFUNC ClassFunc[] = {
    OM_NEW,             (FUNCPTR)ToolbarNew,
    OM_DISPOSE,         (FUNCPTR)ToolbarDispose,
    TOOLM_ADDSINGLE,    (FUNCPTR)ToolbarAddSingle,
    0xF00D,             (FUNCPTR)ToolbarNew,
    DF_END
};

Prototype Class *InitToolbarClass(VOID);

Class *InitToolbarClass(VOID)
{
    return BGUI_MakeClass( CLASS_SuperClassBGUI, BGUI_GROUP_GADGET,
                           CLASS_ObjectSize,     sizeof(TD),
                           CLASS_DFTable,        ClassFunc,
                           TAG_DONE );
}

Prototype BOOL FreeToolbarClass( Class *cl );

BOOL FreeToolbarClass( Class *cl )
{
    return BGUI_FreeClass( cl );
}
