/*
    PROJECT: ppt
    MODULE:  toolbar.c

    This module contains toolbar handling and other associated
    stuff.

    PPT is (C) Janne Jalkanen 1998.

    $Id: toolbar.c,v 1.7 1999/02/21 21:00:17 jj Exp $

    Grumble...

    Let's see.  The following code should open up a toolbar...

    list = AllocVec( sizeof(Object *) * numItems )

    for( i = 0; i < numItems; i++ ) {
        list[i] = NewObject( ToolItemClass,
                             TOOLITEM_Label, "foo",
                             TOOLITEM_Image, image,
                             GA_ID, gid
                             TAG_DONE );
    }

    NewObject( ToolbarClass,
               TOOLBAR_Items, list,
               TAG_DONE );

    FreeVec(list);
 */

/// Includes
#include "defs.h"
#include "misc.h"
#include "gui.h"

#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <proto/datatypes.h>

#include "pragmas/bguibitmap_pragmas.h"
///

/*------------------------------------------------------------------*/

#define TextItem(x)   { (x), TIT_TEXT,  NULL, NULL, NULL, NULL }
#define PicItem(x,a)  { (x), TIT_IMAGE, a,    NULL, NULL, NULL }
#define EndItem       { 0L,  TIT_END,   NULL, NULL, NULL, NULL }

#define BUFLEN 80
#define DEFAULT_TOOLBAR_WIDTH   200
#define MAX_TOOLBARSIZE 30

/*------------------------------------------------------------------*/

/// Globals & type definitions

Class *ToolbarClass, *ToolItemClass = NULL, *BGUIBitmapClass = NULL;
struct Library *BGUIBitmapBase = NULL;

/*
 *  This list contains the actual toolbar.
 */

LONG PPTToolbarConfig[MAX_TOOLBARSIZE+1] = {
    MID_LOADNEW,
    MID_PROCESS,
    -1
};

/*
 *  This list contains the available toolbar items.  It also
 *  contains their localized labels, file names (for images)
 *  etc.  It does not, however, contain the gadgets, since
 *  they're compiled on the PPTToolbar.
 */

struct ToolbarItem ToolbarList[] = {
    PicItem(MID_NEW,            "New" ),
    PicItem(MID_LOADNEW,        "Load" ),
    PicItem(MID_LOADAS,         "Load" ),
    PicItem(MID_SAVEAS,         "Save" ),
    PicItem(MID_DELETE,         "Close" ),
    PicItem(MID_RENAME,         "Rename" ),
    PicItem(MID_HIDE,           "HideShow" ),
    TextItem(MID_QUIT),

    PicItem(MID_UNDO,           "Undo"),
    PicItem(MID_CUT,            "Cut" ),
    TextItem(MID_CUTFRAME),
    PicItem(MID_COPY,           "Copy" ),
    PicItem(MID_PASTE,          "Paste" ),

    PicItem(MID_ZOOMIN,         "ZoomIn" ),
    PicItem(MID_ZOOMOUT,        "ZoomOut" ),
    PicItem(MID_SELECTALL,      "SelectAll" ),
    PicItem(MID_EDITINFO,       "Info" ),
    PicItem(MID_SETRENDER,      "RenderPrefs" ),
    PicItem(MID_RENDER,         "Render" ),
    PicItem(MID_CLOSERENDER,    "RenderClose" ),
    PicItem(MID_CORRECTASPECT,  "CorrectAspect" ),
    PicItem(MID_EDITPALETTE,    "Palette" ),
    TextItem(MID_LOADPALETTE),
    TextItem(MID_SAVEPALETTE),

    PicItem(MID_PROCESS,        "Process" ),
    PicItem(MID_BREAK,          "Break" ),
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

/*
 *  Toolbar class data.
 */

typedef struct {
    int                 td_NumItems;
} TD;

/*
 *  ToolItem class data.
 */

typedef struct {
    Object              *tid_Image;
} TID;
///

/*------------------------------------------------------------------*/
/* ToolItem class */

/// ToolItemNew()
ULONG STATIC ASM
ToolItemNew( REGPARAM(a0,Class *,cl),
             REGPARAM(a2,Object *,obj),
             REGPARAM(a1,struct opSet *,ops) )
{
    TID      *tid;
    struct TagItem *tag, *tags = ops->ops_AttrList, *tstate;
    ULONG rc;
    STRPTR filename = NULL;
    struct Screen *scr = NULL;

    D(bug("ToolItem: OM_NEW\n"));

    if( rc = DoSuperMethodA( cl, obj, (Msg) ops )) {
        tid = (TID*)INST_DATA(cl,rc);
        bzero( tid, sizeof(TID) );

        tstate = tags;

        while( tag = NextTagItem( &tstate ) ) {
            switch( tag->ti_Tag ) {
                case TOOLITEM_FileName:
                    filename = (STRPTR) tag->ti_Data;
                    break;
                case TOOLITEM_Screen:
                    scr = (struct Screen *)tag->ti_Data;
                    break;
            }
        }

        if( filename && BGUIBitmapClass ) {
            Object *dto;

            D(bug("\tOpening image file '%s'\n",filename));

            if( dto = NewDTObject( filename, DTA_SourceType, DTST_FILE,
                                             DTA_GroupID, GID_PICTURE,
                                             PDTA_Remap, FALSE, TAG_DONE ) )
            {
                struct BitMapHeader *bmh;
                struct BitMap *bm;
                UBYTE *cr;
                ULONG nColors;

                if( DoDTMethod( dto, NULL, NULL, DTM_PROCLAYOUT, NULL, TRUE ) ) {
                    if( GetDTAttrs( dto, PDTA_BitMapHeader, &bmh,
                                         PDTA_BitMap, &bm,
                                         PDTA_ColorRegisters, &cr,
                                         PDTA_NumColors, &nColors,
                                         TAG_DONE ) == 4 )
                    {
                        tid->tid_Image = NewObject( BGUIBitmapClass, NULL,
                                                    BITMAP_BitMap,   bm,
                                                    BITMAP_NumColors, nColors,
                                                    BITMAP_Colors,   cr,
                                                    BITMAP_Width,    bmh->bmh_Width,
                                                    BITMAP_Height,   bmh->bmh_Height,
                                                    BITMAP_Remap,    TRUE,
                                                    BITMAP_Screen,   scr,
                                                    TAG_DONE );

                        if( tid->tid_Image ) {
                            SetSuperAttrs( cl, (Object *)rc,
                                           BUTTON_Image,    tid->tid_Image,
                                           FRM_EdgesOnly,   TRUE,
                                           LAB_Label,       NULL,
                                           TAG_DONE );
                        }

                    }

                }

                DisposeObject( dto );
            }

            /*
             *  For some reason, opening failed.
             */

            if( !tid->tid_Image ) {
                D(bug("Couldn't open image file %s\n",filename));
            }
        }

    }

    return rc;

}
///
/// ToolItemDispose()
ULONG STATIC ASM
ToolItemDispose( REGPARAM(a0,Class *,cl),
                 REGPARAM(a2,Object *,obj),
                 REGPARAM(a1,Msg,msg) )
{
    TID *tid = (TID*)INST_DATA(cl,obj);

    D(bug("ToolItem: OM_DISPOSE\n"));

    if( tid->tid_Image ) {
        DisposeObject( tid->tid_Image );
    }

    return( DoSuperMethodA( cl, obj, msg ));
}
///

/// Class Initialization
STATIC DPFUNC TIClassFunc[] = {
    OM_NEW,             (FUNCPTR)ToolItemNew,
    OM_DISPOSE,         (FUNCPTR)ToolItemDispose,
    DF_END
};

Prototype Class *InitToolItemClass(VOID);

Class *InitToolItemClass(VOID)
{
    struct Library *BGUIBitmapBase;

    if( BGUIBitmapBase = OpenLibrary("gadgets/bgui_bitmap.image",0L) ) {
        BGUIBitmapClass = GetBitmapClassPtr();
    } else {
        D(bug( __FUNC__ ":: No Bitmap class available\n"));
    }

    return BGUI_MakeClass( CLASS_SuperClassBGUI, BGUI_BUTTON_GADGET,
                           CLASS_ObjectSize,     sizeof(TID),
                           CLASS_DFTable,        TIClassFunc,
                           TAG_DONE );
}

Prototype BOOL FreeToolItemClass( Class *cl );

BOOL FreeToolItemClass( Class *cl )
{
    if( BGUIBitmapBase ) CloseLibrary( BGUIBitmapBase );
    return BGUI_FreeClass( cl );
}
///

/*------------------------------------------------------------------*/
/* Toolbar class */

/// TOOLM_ADDSINGLE
BOOL STATIC ASM
ToolbarAddSingle( REGPARAM(a0,Class *,cl),
                  REGPARAM(a2,Object *,obj),
                  REGPARAM(a1,struct toolAddSingle *,tas) )
{
    TD *td = (TD*)INST_DATA(cl,obj);
    int item = td->td_NumItems;

    DoMethod( obj, GRM_ADDMEMBER, tas->tas_Item, TAG_DONE );

    td->td_NumItems++;

    return TRUE;
}
///

/// AddItems()
BOOL
AddItems( Class *cl, Object *obj, TD *td, Object **tb )
{
    while( tb ) {
        if( DoMethod( obj, TOOLM_ADDSINGLE, NULL, tb, -1, 0L ) == FALSE ) {
            return FALSE;
        }
        tb++;
    }

    return TRUE;
}
///

/// ToolbarSet()
ULONG STATIC ASM
ToolbarSet( REGPARAM(a0,Class *,cl),
            REGPARAM(a2,Object *,obj),
            REGPARAM(a1,struct opSet *,ops) )
{
    TD      *td = (TD *)INST_DATA(cl,obj);
    ULONG   rc;

    rc = DoSuperMethodA( cl, obj, (Msg) ops );

    return rc;
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
    // TD *td = (TD*)INST_DATA(cl,obj);

    D(bug("Toolbar: OM_DISPOSE\n"));

    return( DoSuperMethodA( cl, obj, msg ));
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
    struct ToolbarItem *tbi;
    char *tail;
    int  item = 0;
    ULONG gid;

    D(bug("BEGIN_TOOLBAR\n"));

    while( FGets( fh, buf, BUFLEN ) ) {
        if( buf[strlen(buf)-1] = '\n' ) buf[strlen(buf)-1] = '\0'; /* Remove the final CR */

        s = strtok( buf, " \t" );
        if( s ) {
            switch( GetOptID( ToolbarOptions, s ) ) {
                case TO_END:
                    PPTToolbarConfig[item] = -1;
                    return PERR_OK;

                case TO_IMAGE:
                    s = strtok( NULL, " \t\n" ); /* s = 2nd arg */
                    gid = strtol( s, &tail, 0 );

                    if( tbi = FindToolbarGID( ToolbarList, gid ) ) {
                        tbi->ti_Type = TIT_IMAGE;
                        s = strtok( NULL, "\n" ); /* s = 3rd arg */
                        PutFileNameToolItem( tbi, NULL, s );
                        D(bug("\tRead in image gadget %lu (file=%s)\n",tbi->ti_GadgetID,tbi->ti_FileName));
                        PPTToolbarConfig[item++] = gid;
                    } else {
                        D(bug("\tIllegal toolbar gid %lu\n",gid));
                    }
                    break;

                case TO_TEXT:
                    s = strtok( NULL, " \t\n" ); /* s = 2nd arg */
                    gid = strtol( s, &tail, 0 );

                    if( tbi = FindToolbarGID( ToolbarList, gid ) ) {
                        tbi->ti_Type = TIT_TEXT;
                        D(bug("\tRead in gadget %lu\n",tbi->ti_GadgetID));
                        PPTToolbarConfig[item++] = gid;
                    } else {
                        D(bug("\tIllegal toolbar gid %lu\n",gid));
                    }

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
/*
 *  Writes the tool bar configuration to the given filehandle.
 *  BUG: image file names should be quote-enclosed
 */
PERROR WriteToolbarConfig( BPTR fh )
{
    char buf[BUFLEN];
    struct ToolbarItem *tbi;
    int  i = 0;

    FPuts( fh, "BEGINTOOLBAR\n" );

    while( PPTToolbarConfig[i] != -1 ) {

        if( tbi = FindToolbarGID( ToolbarList, PPTToolbarConfig[i] ) ) {

            switch( tbi->ti_Type ) {
                case TIT_TEXT:
                    sprintf( buf, "    TEXT %lu\n", tbi->ti_GadgetID );
                    FPuts( fh, buf );
                    break;

                case TIT_IMAGE:
                    sprintf( buf, "    IMAGE %lu %s\n", tbi->ti_GadgetID, tbi->ti_FileName );
                    FPuts( fh, buf );
                    break;

            }
        }

        i++;
    }
    FPuts( fh, "END\n" );

    return PERR_OK;
}
///

/// Class Initialization
STATIC DPFUNC ClassFunc[] = {
    OM_SET,             (FUNCPTR)ToolbarSet,
    OM_NEW,             (FUNCPTR)ToolbarNew,
    OM_DISPOSE,         (FUNCPTR)ToolbarDispose,
    TOOLM_ADDSINGLE,    (FUNCPTR)ToolbarAddSingle,
    DF_END
};

Prototype Class *InitToolbarClass(VOID);

Class *InitToolbarClass(VOID)
{
    if( ToolItemClass = InitToolItemClass() ) {
        return BGUI_MakeClass( CLASS_SuperClassBGUI, BGUI_GROUP_GADGET,
                               CLASS_ObjectSize,     sizeof(TD),
                               CLASS_DFTable,        ClassFunc,
                               TAG_DONE );
    }
    return NULL;
}

Prototype BOOL FreeToolbarClass( Class *cl );

BOOL FreeToolbarClass( Class *cl )
{
    if( ToolItemClass ) FreeToolItemClass(ToolItemClass);

    return BGUI_FreeClass( cl );
}
///

/*--------------------------------------------------------------------*/
/* PPT functions */

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

/// FindInToolbar()
Prototype struct ToolbarItem *FindToolbarGID( struct ToolbarItem *head, ULONG gid );

struct ToolbarItem *FindToolbarGID( struct ToolbarItem *head, ULONG gid )
{
    int i;

    for( i = 0; head[i].ti_Type != TIT_END; i++ ) {
        if( head[i].ti_GadgetID == gid ) return &head[i];
    }
    return NULL;
}

Prototype struct ToolbarItem *FindToolbarName( struct ToolbarItem *head, STRPTR name );

struct ToolbarItem *FindToolbarName( struct ToolbarItem *head, STRPTR name )
{
    int i;

    for( i = 0; head[i].ti_Type != TIT_END; i++ ) {
        if( strcmp( head[i].ti_Label,name) == 0 ) return &head[i];
    }
    return NULL;
}
///

#if 0
/// InsertToolItem()
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
///
/// RemoveToolItem()
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
///
#endif

/// RebuildToolbarFromListview( Object *list )
Prototype VOID RebuildToolbarFromListview( Object *list );

VOID RebuildToolbarFromListview( Object *list )
{
    APTR entry;

    if( entry = (APTR)DoMethod( list, LVM_FIRSTENTRY, NULL, 0L ) ) {
        int item = 0;
        struct ToolbarItem *ti;

        do {
            if( entry ) {
                ti = FindToolbarName( ToolbarList, entry );
                PPTToolbarConfig[item++] = ti->ti_GadgetID;
            }
            entry = (APTR)DoMethod( list, LVM_NEXTENTRY, entry, 0L );
        } while( entry );
        PPTToolbarConfig[item] = -1;
    }
}
///

/// VOID SetupToolbarList(VOID)
/*
 *  Go through the toolbar settings and set up the lists.
 */
Prototype VOID SetupToolbarList(VOID);

VOID SetupToolbarList(VOID)
{
    struct ToolbarItem *tooldata = ToolbarList, *ti;
    int item = 0;
    LONG barcode;
    struct NewMenu *nm;
    D(APTR b);

    D(bug("SetupToolbarList()\n"));
    D(b = StartBench());

    /*
     *  First, go through the menu items and place them on the bigger listview.
     */

    item = 0;
    while( tooldata[item].ti_Type != TIT_END ) {
        int i;

        /*
         *  Check if it's on the list of things to be added to the listview
         */

        for( i = 0; PPTToolbarConfig[i] > 0 && PPTToolbarConfig[i] != tooldata[item].ti_GadgetID; i++ );
        if( PPTToolbarConfig[i] < 0 ) {
            D(bug("AddEntry(%08x=%s)\n",&tooldata[item],tooldata[item].ti_Label ));
            AddEntry( NULL, prefsw.AvailButtons, tooldata[item].ti_Label, LVAP_TAIL );
        }
        item++;
    }

    /*
     *  Now, set up the current list
     */

    item = 0;
    while( (barcode = PPTToolbarConfig[item]) > 0 ) {

        if( ti = FindToolbarGID( ToolbarList, barcode ) ) {
            D(bug("AddEntry(%08x=%s)\n",ti,ti->ti_Label ));
            AddEntry( NULL, prefsw.ToolbarList, ti->ti_Label, LVAP_TAIL );
        }

        item++;
    }

    SortList( NULL, prefsw.AvailButtons );

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

/// PERROR PutFileNameToolItem( struct ToolbarItem *ti, STRPTR path, STRPTR filename );

Prototype PERROR PutFileNameToolItem( struct ToolbarItem *ti, STRPTR path, STRPTR filename );

PERROR PutFileNameToolItem( struct ToolbarItem *ti, STRPTR path, STRPTR filename )
{
    int len = 0;
    char *fn = NULL;

    if( path ) {
        len = strlen(path) + strlen(filename) + 16;
        if(fn = smalloc( len )) {
            strcpy( fn, path );
            AddPart( fn, filename, len );
        }
    } else {
        len = strlen(filename) + 1;
        if( fn = smalloc(len) ) {
            strcpy( fn, filename );
        }
    }

    if( fn ) {
        if( ti->ti_FileName ) sfree( ti->ti_FileName );
        ti->ti_FileName = fn;
        D(bug("\tAllocated %lu bytes from %08X for filename\n", len, ti->ti_FileName));
        return PERR_OK;
    } else {
        return PERR_OUTOFMEMORY;
    }
}
///

/// GimmeToolbar()
Object *GimmeToolBar( LONG *toolbar )
{
    Object *tb;
    struct ToolbarItem *ti;

    D(bug("GimmeToolbar( %08X )\n",toolbar));

    if( tb = NewObject( ToolbarClass, NULL, TAG_DONE ) ) {
        int i;

        for(i = 0; toolbar[i] != -1; i++ ) {

            if( ti = FindToolbarGID( ToolbarList, toolbar[i] ) ) {

                /*
                 *  Is there a default filename?  How about a user-specified one?
                 */

                if( !ti->ti_FileName && ti->ti_DefaultFileName ) {
                    PutFileNameToolItem( ti, DEFAULT_TOOLBARDIR, ti->ti_DefaultFileName );
                }

                /*
                 *  Allocate object
                 */

                if( ti->ti_Gadget = NewObject( ToolItemClass, NULL,
                                               Label( ti->ti_Label ),
                                               BT_ToolTip, ti->ti_Label,
                                               (ti->ti_Type == TIT_IMAGE) ? TAG_IGNORE : TAG_SKIP, 1,
                                               TOOLITEM_FileName, ti->ti_FileName,
                                               TOOLITEM_Screen, MAINSCR,
                                               GA_ID, ti->ti_GadgetID,
                                               TAG_DONE ) )
                {
                    DoMethod( tb, TOOLM_ADDSINGLE, NULL, ti->ti_Gadget, TASNUM_LAST, 0L );
                }
            } else {
                InternalError("Toolbar list out of sync!");
            }
        }
    }

    return tb;
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

    toolw.GO_ToolBar = GimmeToolBar(PPTToolbarConfig);

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

/*
 *  This is a kludge to reopen the toolbar window
 *  when screen is changed to make sure the BITMAP_Screen
 *  is set correctly for icons
 */

Prototype PERROR RebuildToolbarWindow(VOID);

PERROR RebuildToolbarWindow(VOID)
{
    int i;

    DisposeObject( toolw.Win );
    toolw.Win = NULL;

    for( i = 0; ToolbarList[i].ti_Type != TIT_END; i++ )
        ToolbarList[i].ti_Gadget = NULL;

    if( GimmeToolBarWindow() == PERR_OK ) {
        toolw.win = WindowOpen( toolw.Win );
        return PERR_OK;
    }
    return PERR_ERROR;
}

/// LocalizeToolbar()
Prototype PERROR LocalizeToolbar(VOID);

PERROR LocalizeToolbar(VOID)
{
    int item = 0;
    struct NewMenu *nm;

    while( ToolbarList[item].ti_Type != TIT_END ) {
        nm = (APTR)FindNewMenuItem( PPTMenus, ToolbarList[item].ti_GadgetID );
        if( nm ) {
            ToolbarList[item].ti_Label = nm->nm_Label;
        }

        item++;
    }

    return PERR_OK;
}
///
/// FreeToolbar()
Prototype VOID FreeToolbar(VOID);

VOID FreeToolbar(VOID)
{
    int item = 0;
    struct NewMenu *nm;

    while( ToolbarList[item].ti_Type != TIT_END ) {
        if( ToolbarList[item].ti_FileName ) sfree( ToolbarList[item].ti_FileName );
        ToolbarList[item].ti_Gadget = NULL;
        item++;
    }
}
///

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

