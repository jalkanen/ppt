/*
    PROJECT: ppt
    MODULE : frame.c

    $Id: frame.c,v 6.3 1999/11/28 18:19:38 jj Exp $

    This contains frame handling routines

*/

/*-------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"
#include "gui.h"

#include <clib/alib_protos.h>
#include <pragmas/bgui_pragmas.h>

/*-------------------------------------------------------------------*/
/* Defines */

#define COPYBUFSIZE         32768

/*-------------------------------------------------------------------*/
/* Prototypes */

/// Prototypes
Prototype PERROR        AddAlpha( FRAME *, FRAME * );
Prototype VOID          RemoveAlpha( FRAME * );
Prototype void          DeleteFrame( FRAME * );
Prototype PERROR        ReplaceFrame( FRAME *old, FRAME *new );
Prototype BOOL          FrameFree( FRAME * );
Prototype struct Window *GetFrameWin( const FRAME *frame  );
Prototype PERROR        AddFrame( FRAME * );
Prototype PERROR        MakeUndoFrame( FRAME * );
Prototype FRAME         *UndoFrame( FRAME * );
Prototype BOOL          ChangeBusyStatus( FRAME *, ULONG );
Prototype VOID          UpdateFrameInfo( FRAME * );
Prototype VOID          RefreshFrameInfo( FRAME *, EXTBASE * );
Prototype BOOL          IsFrameBusy( FRAME * );

Local PERROR            SetBuffers( FRAME *frame, EXTBASE *xd );
Local VOID              FreeBuffers( FRAME *frame, EXTBASE *PPTBase );
Local VOID              ResetSharedFrameVars(FRAME *);
///

/*-------------------------------------------------------------------*/
/* Code */


/// ObtainFrame()
/****i* pptsupport/ObtainFrame ******************************************
*
*   NAME
*
*   SYNOPSIS
*       result = ObtainFrame( frame, method );
*       D0                    A0     D0
*
*       BOOL ObtainFrame( FRAME *, ULONG );
*
*   FUNCTION
*       TBA
*
*   INPUTS
*
*   RESULT
*       TRUE, if the frame could be obtained.
*
*   EXAMPLE
*
*   NOTES
*       DO NOT FORGET to call ReleaseFrame() after you are done with this
*       frame.
*
*   BUGS
*       This entry very incomplete.
*
*   SEE ALSO
*       ReleaseFrame()
*
*****************************************************************************
*
*
*    The new, improved frame locking routines. Returns TRUE
*    if it succeeds.
*    BUG: Uses goto...
*/

Prototype BOOL ASM ObtainFrame( REGDECL(a0,FRAME *), REGDECL(d0,ULONG) );

BOOL SAVEDS ASM
ObtainFrame( REGPARAM(a0,FRAME *,frame), REGPARAM(d0,ULONG,method) )
{
    APTR SysBase = SYSBASE();
    FRAME *cur;
    BOOL res = FALSE;
    D(int dbg_i)
    D(static int dbg_cnt = 0)

    D(for(dbg_i=0;dbg_i<dbg_cnt;dbg_i++) { bug("\t"); });
    D(bug("ObtainFrame(%08X [%s], method=%lu)\n",
           frame, frame->name, method));
    D(dbg_cnt++);

    if(!CheckPtr( frame, "ObtainFrame()" ))
        return FALSE;

    QLOCK(frame);

    /*
     *  If the lock is non-exclusive, then we can allow contact.
     *  But count has to be kept.
     */

    if( (frame->busy == BUSY_READONLY) && (method == BUSY_READONLY) ) {
        goto gotcha;
    }

    /*
     *  Is the frame already reserved?
     */

    if( frame->busy != BUSY_NOT ) {
        goto errorexit;
    }

    /*
     *  Reserve the frame and everyone attached to it.
     *  Does some nice recursion.
     *  BUG: What if the obtain attempt fails?
     */

gotcha:

    if( cur = FindFrame(frame->attached) ) {
        if(ObtainFrame( cur, method ) == FALSE)
            goto errorexit;
    }

#ifdef USE_OLD_ALPHA
    if( cur = FindFrame(frame->alpha) ) {
        if(ObtainFrame( cur, method ) == FALSE)
            goto errorexit;
    }
#endif

    frame->busy = method;
    frame->busycount++;
    res = TRUE;

errorexit:
    QUNLOCK(frame);

    D(dbg_cnt--);
    D(for(dbg_i=0;dbg_i<dbg_cnt;dbg_i++) { bug("\t"); })
    if( res ) {
        D(bug("done (count = %ld)\n",frame->busycount));
    } else {
        D(bug("failed\n"));
    }

    return res;
}
///

/// ReleaseFrame()
/****i* pptsupport/ReleaseFrame ******************************************
*
*   NAME
*
*   SYNOPSIS
*       ReleaseFrame( frame );
*                     A0
*
*       VOID ReleaseFrame( FRAME * );
*
*   FUNCTION
*       TBA
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       This entry very incomplete.
*
*   SEE ALSO
*       ObtainFrame()
*
*****************************************************************************
*
*
*    Release a lock on a frame. Returns FALSE, if unlocking
*    could not be made. This routine also clears the currext
*    fields.
*/

Prototype BOOL ASM ReleaseFrame( REGDECL(a0,FRAME *) );

BOOL SAVEDS ASM
ReleaseFrame( REGPARAM(a0,FRAME *,frame) )
{
    APTR SysBase = SYSBASE();
    FRAME *cur;
    D(int dbg_i)
    D(static int dbg_cnt = 0)

    D(for(dbg_i=0;dbg_i<dbg_cnt;dbg_i++) { bug("\t"); });
    D(bug("ReleaseFrame( %08X [%s] )\n",frame, frame->name ));
    D(dbg_cnt++);

    if(frame == NULL) return FALSE;

    if(!CheckPtr(frame,"ReleaseFrame()"))
        return FALSE;

    QLOCK(frame);

    if( frame->busy == BUSY_NOT || frame->busycount <= 0 ) {
        QUNLOCK(frame);
        InternalError( "Unmatched ReleaseFrame()" );
        return FALSE;
    }

    /*
     *  Release the frame. The relevant other fields are also cleared.
     */

    if( cur = FindFrame(frame->attached) ) {
        ReleaseFrame( cur );
    }

#ifdef USE_OLD_ALPHA
    if( cur = FindFrame(frame->alpha) ) {
        ReleaseFrame( cur );
    }
#endif

    /*
     *  Release the frame only if busycount is equal to zero
     */

    if( --frame->busycount == 0 ) {
        frame->busy = BUSY_NOT;
        frame->currext  = NULL;
        frame->currproc = NULL;
    }

    QUNLOCK(frame);

    D(dbg_cnt--);
    D(for(dbg_i=0;dbg_i<dbg_cnt;dbg_i++) { bug("\t"); });
    D(bug("done (count = %ld)\n", frame->busycount));

    return TRUE;
}
///

/// IsFrameBusy(), ChangeBusyStatus(), CopyBusyStatus()
/*
    Returns TRUE, if the frame is unusable at the moment.
*/
BOOL IsFrameBusy( FRAME *frame )
{
    BOOL result = FALSE;

    QSHLOCK(frame);

    if( frame->busy != BUSY_NOT || frame->currext != NULL || frame->currproc != NULL )
        result = TRUE;

    QUNLOCK(frame);
    return result;
}


/*
    Changes frame busy status. USE WITH EXTREME CAUTION!
    BUG: Maybe should check if busy != BUSY_NOT?
*/

BOOL ChangeBusyStatus( FRAME *frame, ULONG mode )
{
    APTR SysBase = SYSBASE();

    if(CheckPtr( frame, "ChangeBusyStatus():frame" )) {
        LOCK(frame);
        frame->busy = mode;
        UNLOCK(frame);
    }

    return TRUE;
}

VOID CopyBusyStatus( FRAME *source, FRAME *dest )
{
    SHLOCK(source);
    LOCK(dest);

    dest->busy      = source->busy;
    dest->busycount = source->busycount;

    UNLOCK(dest);
    UNLOCK(source);
}
///

/// ResetSharedFrameVars()
/*
    A stupid routine to reset any shared variables a frame may
    have before deleting it.
*/

Local
VOID ResetSharedFrameVars(FRAME *f)
{
    if( CheckPtr( f, "RSFV(): frame" )) {
        LOCK(f);
        f->disp         = NULL;
        f->renderobject = f->pw = NULL;
        f->dpw          = NULL;
        f->mywin        = NULL;
        f->parent       = NULL;
        f->editwin      = NULL;
        UNLOCK(f);
    }
}
///
/// AddFrame()
/*
    Add the frame to the system lists. Does not currently do much.
    Refreshes the listview.
*/

PERROR AddFrame( FRAME *frame )
{
    D(bug("AddFrame( %08X ) = %s\n",frame, frame->name));

    if(!CheckPtr( frame, "AddFrame: frame" ))
        return PERR_GENERAL;

    LOCKGLOB();
    AddTail( &globals->frames, (struct Node *)frame );
    UNLOCKGLOB();

    return PERR_OK;
}
///
/// DeleteFrame()
/*
    Gets rid of a frame. Completely. No traces. Safe to call with NULL
    args.

    What is the difference between DeleteFrame() & RemFrame()?

    Well, RemFrame is for the externals, and it de-allocates only
    stuff MakeFrame() allocated. DeleteFrame() knows about everything else
    (displays, etc) and knows what to do with them.
*/

void DeleteFrame( FRAME *f )
{
    APTR entry;
    PPTREXXARGS *ra;

    D(bug("DeleteFrame( frame = %08X )...",f));

    if(!CheckPtr( f, "Delete: Frame"))
        return;

    if(f) {

        LOCK(f);

        /*
         *  Remove REXX
         */

        if(ra = FindRexxWaitItemFrame(f)) {
            ra->rc = -20;
            ra->rc2 = (long)"Frame deleted";
            ReplyRexxWaitItem(ra);
        }

        entry = (APTR)FirstEntry(framew.Frames);
        while( entry && (strcmp(ParseListEntry(entry),f->nd.ln_Name) != 0) )
            entry = (APTR)NextEntry(framew.Frames, entry);

        if(entry) {
            if(framew.win)
                RemoveEntryVisible( framew.win, framew.Frames, entry );
            else
                DoMethod(framew.Frames, LVM_REMENTRY, NULL, entry );
            UpdateMainWindow( NULL ); /* Removes the data */
        }

        LOCKGLOB();

        /*
         *  Remove windows
         */


        if( f->pw ) {
            if(f->pw->Win) {
                DisposeObject( f->pw->Win );
            }
            sfree( f->pw );
            f->pw = NULL;
        }

        if(f->renderobject)
            CloseRender( f, globxd );

        if(f->disp) {

            if(f->disp->win) {
                D(bug("Display..."));
                RemoveDisplayWindow( f );
            }

            ReleaseColorTable( f );

        }

        if(f->dpw) {

            CheckPtr( f->dpw, "Delete: prefswin" );

            if(f->dpw->WO_Win) {
                D(bug("DispPrefs..."));
                DisposeObject(f->dpw->WO_Win);
            }
            pfree(f->dpw);
        }

        if(f->editwin) {
            DeleteEditWindow(f->editwin);
            FreeEditWindow(f->editwin);
        }

        /*
         *  Selection.
         */

        if( f->selection.vertices ) sfree( f->selection.vertices );
        if( f->selection.mask ) RemFrame( f->selection.mask, globxd );

        /*
         *  Remove the structure himself
         */

        D(bug("Remove..."));
        Remove( (struct Node *) f );
        RemFrame( f, globxd );
        UNLOCKGLOB();
//        SetGadgetAttrs( (struct Gadget *)GO_info, globals->maindisp->win, NULL,
//                        GA_Disabled, TRUE, TAG_END );
    }
    D(bug("\nReturning from DeleteFrame()\n"));
}
///

/// AddAlpha()
#ifdef USE_OLD_ALPHA

/*
    Adds an alpha frame to the frame given. Performs a multitude of checks, also.

    Returns PERR_OK, if the adding succeeded, otherwise something else.

    BUG: Insufficient error checking.  No locking.
*/

PERROR AddAlpha( FRAME *frame, FRAME *alpha )
{
    ULONG a;
    FRAME *oldalpha = NULL;

    D(bug("AddAlpha( %08X, %08X )\n",frame,alpha ));

    if( alpha->pix->colorspace != CS_GRAYLEVEL ) {
        Req(GetFrameWin(frame), NULL, "\nThe alpha channel MUST be a\n"
                                       "grayscale image\n" );
        return PERR_FAILED;
    }

    if( frame->pix->colorspace != CS_RGB ) {
        Req(GetFrameWin(frame), NULL, "\nAlpha channels on others than\n"
                                      "RGB images are not allowed (yet)\n" );
        return PERR_FAILED;
    }

    if( frame->alpha ) {
        oldalpha = FindFrame(frame->alpha);
    }

    /*
     *  The message comes from the main program, which has already
     *  asked the user about this. So we ask for confirmation only
     *  if there was a previous channel
     */

    if( oldalpha && DOCONFIRM ) {
        a = Req(GetFrameWin(frame), GetStr(MSG_YESNO_GAD),
                GetStr(MSG_FRAME_REPLACE_CURRENT_ALPHA_OLD),
                oldalpha->name, alpha->name );
    } else {
        a = 1;
    }

    if( a ) {
        if(AttachFrame( frame, alpha, ATTACH_ALPHA, globxd ))
            return PERR_OK;
        else
            return PERR_FAILED;
    }

    return PERR_CANCELED;
}

#else

/*
 *  This is the new and improved version.
 */

PERROR AddAlpha( FRAME *frame, FRAME *alpha )
{
    UBYTE cspace = frame->pix->colorspace;
    char buffer[100];

    if( cspace != CS_RGB && cspace != CS_ARGB ) {
        Req(GetFrameWin(frame), NULL, GetStr( MSG_FRAME_ALPHA_CHANNELS_NOT ) );
        return PERR_FAILED;
    }

    if( cspace == CS_ARGB && DOCONFIRM ) {
        if(Req(GetFrameWin(frame), GetStr(MSG_YESNO_GAD),
               GetStr( MSG_FRAME_REPLACE_CURRENT_ALPHA ),
               alpha->name ) )
        {
            return PERR_CANCELED;
        }
    }

    if( AttachFrame( frame, alpha, ATTACH_SIMPLE, globxd ) ) {
        sprintf(buffer, "ALPHA %ld", alpha->ID );
        if( RunFilterCommand( frame, "ADDALPHA", buffer ) != PERR_OK ) {
            Req( GetFrameWin(frame), NULL, GetStr(MSG_PERR_NO_NEW_PROCESS));
            return PERR_FAILED;
        }
    }

    return PERR_OK;
}

#endif
///
/// RemoveAlpha()
/*
    Removes the current alpha channel
*/

VOID RemoveAlpha( FRAME *f )
{
    D(bug("RemoveAlpha( %08X )\n",f ));

    LOCK(f);
#ifdef USE_OLD_ALPHA
    f->alpha = 0L;
#endif
    UNLOCK(f);
}
///
/// Local DoReplaceFrame()
/*
    Replaces old frame with new frame. List handling is done correctly,
    but the frames are not themselves changed. This routine is common
    for both Undo and Replace routines.
*/

Local
PERROR DoReplaceFrame( FRAME *old, FRAME *new )
{
    INFOWIN *iw;


    LOCK(old);
    LOCK(new);

    /*
     *  Put new into the master list and remove old.
     *  BUG: Should edit old.Node so that the list won't be traversed again.
     */

    LOCKGLOB();

    Insert( &globals->frames, (struct Node *)new, (struct Node *)old );
    Remove( (struct Node *) old );

    UNLOCKGLOB();

    /*
     *  Take care of associated shared objects
     *  First, make checks if the new frame had any objects of his own
     *  and remove them, if necessary.
     */

    if( new->mywin ) {
        DeleteInfoWindow(new->mywin,globxd);
    }

    if( new->disp ) {
        D(bug("Released new display struct for frame %08X\n",new));
        pfree(new->disp);
    }

    if( new->dpw ) {
        D(bug("Released new dispprefswin struct for frame %08X\n",new));
        pfree(new->dpw);
    }

    if( new->pw ) {
        D(bug("Released new palettewin struct for frame %08X\n",new));
        pfree(new->pw);
    }

    /*
     *  Then do the replace.  Some objects need a sanity check, for example
     *  due to size change.
     *
     *  BUG: The zoombox should really apply the difference between the two
     *       sizes so that it would look the same.
     */

    new->ID             = old->ID;
    new->disp           = old->disp;
    new->renderobject   = old->renderobject;
    new->dpw            = old->dpw;
    new->parent         = NULL;
    new->pw             = old->pw;
    new->zoombox        = old->zoombox;
    new->editwin        = old->editwin;
    new->attached       = old->attached;

    if( old->pix->height != new->pix->height || old->pix->width != new->pix->width ) {
        new->zoombox.Height = new->pix->height;
        new->zoombox.Width  = new->pix->width;
        new->zoombox.Top    = new->zoombox.Left = 0;
    }

    /*
     *  Busy status must be initialized, too, since it may have
     *  changed after DupFrame(), and we'll be using it anyway.
     */

    CopyBusyStatus( old, new );

    /*
     *  Windows
     */

    iw = new->mywin = old->mywin;
    if(iw) {
        if(CheckPtr(iw,"ReplaceFrame: Bad IW")) {
            iw->myframe = new;
            SetAttrs( iw->WO_win, WINDOW_Title, new->name, TAG_END );
        }
    }

    if(new->disp->win) {
        if(new->disp->Win) {
            /* No code */
//            SetAttrs( new->disp->Win, WINDOW_Title, new->disp->title, TAG_END );
        } else {
            /* The reallyrender window. BUG: Maybe not necessary? */
            SetWindowTitles( new->disp->win, new->disp->title, (UBYTE *)~0 );
        }
    }

    if( new->dpw && new->dpw->WO_Win )
        SetAttrs( new->dpw->WO_Win, WINDOW_Title, new->name, TAG_END );

    if( new->editwin )
        new->editwin->frame = new;

    UNLOCK(old);
    UNLOCK(new);

    return PERR_OK;
}
///
/// ReplaceFrame()
/*
    Replaces old frame with new frame.
*/

PERROR ReplaceFrame( FRAME *old, FRAME *new )
{
    UWORD i = 1;
    FRAME *cf;

    D(bug("ReplaceFrame( %08X with %08X )\n", old, new ));

    if(!CheckPtr( old, "ReplaceFrame(): old" ))
        return PERR_GENERAL;

    if(!CheckPtr( new, "ReplaceFrame(): new" ))
        return PERR_GENERAL;

    /*
     *  Copy data
     */

    DoReplaceFrame( old, new );

    /*
     *  Take care of old frame. First we clear all shared variables,
     *  that have already been passed to the new frame so that we can free
     *  the frame without deallocating resources by mistake.
     */

    ResetSharedFrameVars( old );

    MakeUndoFrame( old );
    new->lastframe = old;

    /*
     *  Go through the previous frames and see if we need to drop
     *  any of the old ones.
     */

    cf = new;

    while( cf->lastframe->lastframe ) {
        i++;
        cf = cf->lastframe;
    }

    D(bug("\tUndo count is %d\n",i));

    if( i > globals->userprefs->maxundo ) {
        D(bug("\tRemove due to undo! "));
        RemFrame( cf->lastframe, globxd );
        cf->lastframe = NULL;
    }

    /* The rest should already be copied by DupFrame() */

    return PERR_OK;
}
///
/// MakeUndoFrame()
/*
    Put a frame into undo state.
*/

PERROR MakeUndoFrame( FRAME *frame )
{
    D(bug("MakeUndoFrame( %08X )\n",frame));

    if(!CheckPtr( frame, "MakeUndoFrame(): frame" ))
        return PERR_GENERAL;

    LOCK( frame );

    /*
     *  Remove any buffers from hogging up the memory.
     */

    FlushVMData( frame->pix->vmh, globxd ); /* Need to flush first */

    if(!CheckPtr( frame->pix->vmh->data, "MakeUndoFrame(): vmh->data" ))
        return PERR_GENERAL;

    FreeBuffers( frame, globxd );

    /*
     *  Reset some variables. BUG: I am not sure if these belong here.
     */

    frame->currext   = NULL;
    frame->currproc  = NULL;
    frame->busy      = BUSY_NOT;
    frame->busycount = 0;
    //ClearFrameInput( frame ); // Just in case.
    //frame->selection.selstatus = 0;

    UNLOCK( frame );
    return PERR_OK;
}
///
/// UndoFrame()
/*
    Revert an undo buffer from a frame. Replaces the current frame
    with it's undo buffer and removes the current frame.
*/

FRAME *UndoFrame( FRAME *currentframe )
{
    FRAME *undoframe;

    D(bug("UndoFrame( curr = %08X )\n",currentframe ));

    if(!CheckPtr( currentframe, "UndoFrame(): frame\n"))
        return NULL;

    undoframe = currentframe->lastframe;

    if(!undoframe) {
        if( DOCONFIRM ) Req(GetFrameWin(currentframe), NULL, GetStr(MSG_NO_MORE_UNDO_LEVELS) );
        else DisplayBeep( NULL );
        return NULL;
    }

    if(!CheckPtr( undoframe, "UndoFrame() : undoframe"))
        return NULL;

    /*
     *  Re-setup buffers for the undo frame and refresh them.
     */

    D(bug("\tSet buffers\n"));
    if( SetBuffers( undoframe, globxd ) != PERR_OK )
        return NULL;

    LoadVMData( undoframe->pix->vmh, 0L, globxd );

//    D(bug("\tReplace it\n"));
    DoReplaceFrame( currentframe, undoframe );

    ResetSharedFrameVars( currentframe );

    currentframe->lastframe = NULL; /* So that RemFrame() won't remove it. */

//    D(bug("\tRemoving old\n"));

    RemFrame( currentframe, globxd );

    UpdateInfoWindow( undoframe->mywin, globxd );

    /*
     *  Since this is a new frame, we won't have a rectangle drawn for us.
     */

    undoframe->selection.selstatus &= ~(SELF_DRAWN|SELF_BUTTONDOWN);

    return undoframe;
}
///
/// FrameFree()
/*
    Returns true, if frame is free to be processed
    BUG: Should see what to do with it.
*/

BOOL FrameFree( FRAME *frame )
{

    if( !frame ) {
        Req(NEGNUL, NULL, GetStr( MSG_NO_FRAME_SELECTED ) );
        return FALSE;
    }

    if( !CheckPtr( frame, "FrameFree(): frame" )) return FALSE;

    /*
     *  Is the frame free?
     */

    if( IsFrameBusy(frame) == FALSE ) {
        return TRUE;
    }

    /*
     *  If not, display a message to the user, unless an external process
     *  can be located, in which case a CTRL_F (wakeup) signal is sent.
     */

    if( frame->currproc ) {
        Signal( frame->currproc, SIGBREAKF_CTRL_F );
        DisplayBeep(MAINSCR);
    } else {
        if( frame->currext == NEGNUL || frame->currext == NULL ) {
            Req(GetFrameWin(frame), NULL, GetStr(MSG_FRAME_IN_USE), frame->nd.ln_Name);
        } else {
            Req(GetFrameWin(frame), NULL, GetStr(MSG_FRAME_IN_USE_BY_XXX),
                frame->nd.ln_Name, frame->currext->nd.ln_Name );
        }
    }

    return FALSE;
}
///

/// GetFrameWin()
/*
    Returns a suitable window pointer for this frame. Windows are checked in
    this order:

        - Active window, if any
        - Display window
        - Main window
*/

struct Window *GetFrameWin( const FRAME *frame  )
{
    if(frame == NULL || frame == NEGNUL)
        return MAINWIN;

    if(!CheckPtr(frame,"frame")) return MAINWIN;

    /* Return active window if possible */

    if( frame->disp ) {
        if(frame->disp->win) {
            if(frame->disp->win->Flags & WFLG_WINDOWACTIVE)
                return frame->disp->win;
        }

        /* Prefer display window if on main screen */

        if(frame->disp->scr == MAINSCR && frame->disp->win)
            return frame->disp->win;
    }

    /* Otherwise, give main window */

    return MAINWIN;
}
///

/// UpdateFrameInfo()

/*
    This routine updates the following things:
        - window title
        - screen title
    The file name on the screen is cut to a maximum of MAXSCRTITLENAMELEN
    characters.  The title looks something like this:

    PPT: '<40 chars>' ?????x?????x?? <name> image (????????? bytes)

    So the total length of the message is (at maximum)
    6+40+17+40+8+9+7 = 127 bytes

    The max length reserved for the message itself is 50 bytes, out of
    which english default uses 42 bytes.  So the max length should be
    127 bytes + (50-42) = 135 bytes.
*/
VOID UpdateFrameInfo( FRAME *f )
{
    DISPLAY *d = f->disp;
    PIXINFO *p = f->pix;
    EDITWIN *e = f->editwin;
    UBYTE   name[SCRTITLELEN+1] = "..."; /* OK, it has some extra */
    long    len;

    LOCK(f);

    if( (len = strlen(f->path)+strlen(f->name)) > MAXSCRTITLENAMELEN)
        strcat( name, f->path + ( len - MAXSCRTITLENAMELEN ) );
    else
        strcpy( name, f->path );

    AddPart( name, f->name, SCRTITLELEN );

    if(d) {
        ULONG picArea, zoomArea;
        //struct  IBox *bounds;

        //GetAttr( AREA_AreaBox, d->RenderArea, (ULONG *)&bounds );
        picArea = p->width * p->height;

        /* Ensure we won't get a 0 area */

        if( f->zoombox.Width && f->zoombox.Height )
            zoomArea = f->zoombox.Width * f->zoombox.Height;
        else
            zoomArea = picArea;

        sprintf(d->title,"%s (%ux%u) %.3g%%",f->nd.ln_Name, p->width, p->height,100.0*(float)zoomArea/(float)picArea );
        sprintf(d->scrtitle, GetStr(MSG_FRAME_SCREEN_TITLE),
            name, p->width, p->height, p->origdepth,
            f->origtype ? (char const *)f->origtype->info.nd.ln_Name : GetStr(MSG_UNKNOWN),
            PICSIZE( f->pix ) );
    }

    if(e) {
        sprintf(e->title,"Edit: %s", f->nd.ln_Name );
    }

    UNLOCK(f);
}
///
/// RefreshFrameInfo()
/*
    Refresh the display after a frame change.

    This refreshes the window and screen title, as well as makes
    sure the propgadgets are correct.
*/
VOID RefreshFrameInfo( FRAME *f, EXTBASE *PPTBase )
{
    APTR IntuitionBase = PPTBase->lb_Intuition;
    DISPLAY *d = f->disp;
    EDITWIN *e = f->editwin;
    struct TagItem tags[] = {
        WINDOW_Title, NULL,
        WINDOW_ScreenTitle, NULL,
        TAG_END, 0L
    };

    if(!CheckPtr(f,"RefreshFrameInfo():frame")) return;

    LOCK(f);

    if(d) {
        if(d->Win) {
            tags[0].ti_Data = (ULONG)d->title;
            tags[1].ti_Data = (ULONG)d->scrtitle;

            SetAttrsA( d->Win, tags );
            XSetGadgetAttrs(PPTBase, GAD(d->GO_BottomProp), d->win,
                            NULL, PGA_Top, f->zoombox.Left,
                            PGA_Visible, f->zoombox.Width,
                            PGA_Total,   f->pix->width, TAG_END );
            XSetGadgetAttrs(PPTBase, GAD(d->GO_RightProp), d->win,
                            NULL, PGA_Top, f->zoombox.Top,
                            PGA_Visible, f->zoombox.Height,
                            PGA_Total, f->pix->height, TAG_END );
        }
    }

    if(e) {
        tags[0].ti_Data = (ULONG)e->title;
        tags[1].ti_Data = (ULONG)std_ppt_blurb;
        SetAttrsA( e->Win, tags );
    }

    UNLOCK(f);
}
///


/// IsAttached()
/*
    Returns TRUE if the srcid frame has been attached to it
*/
BOOL IsAttached( FRAME *frame, ID srcid )
{
    FRAME *cur, *next;

    cur = frame;
    while( (next = FindFrame(cur->attached)) ) {
        if( cur->ID == srcid )
            return TRUE;
        cur = next;
    }
    return FALSE;
}
///

/*---------------------------------------------------------------------------*/
/* Routines below this point are part of the support library. */

/// CopyFrameData()
/****u* pptsupport/CopyFrameData ******************************************
*
*   NAME
*       CopyFrameData - copy the data from one frame to an another (V4)
*
*   SYNOPSIS
*       error = CopyFrameData( source, dest, flags )
*       D0                     A0      A1    D0
*
*       PERROR CopyFrameData( FRAME *, FRAME *, ULONG );
*
*   FUNCTION
*       This function copies all the image data from the source
*       frame to the dest frame.  The frames must have the same
*       size and the same colorspace.
*
*   INPUTS
*       source - the source frame
*       dest - the destination frame
*       flags - controls the copying procedure. Possible flags are:
*
*           CFDF_SHOWPROGRESS - Specifying this flag causes a
*               progress bar to be displayed during the copying
*               operation.  You will want to enable this if you
*               are copying a very large image.
*
*   RESULT
*       PERR_OK if everything went ok, otherwise this can fail in
*           numerous ways...
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       No error checking.
*
*   SEE ALSO
*       MakeFrame(), InitFrame()
*
*****************************************************************************
*
*   BUG: Really needs error checking.
*/


Prototype PERROR ASM CopyFrameData( REGDECL(a0,FRAME *), REGDECL(a1,FRAME *), REGDECL(d0,ULONG), REGDECL(a6,EXTBASE *) );

PERROR SAVEDS ASM
CopyFrameData( REGPARAM(a0,FRAME *,frame), REGPARAM(a1,FRAME *,newframe),
               REGPARAM(d0,ULONG,flags), REGPARAM(a6,EXTBASE *,PPTBase) )
{
    UBYTE *buf;
    PERROR res = PERR_OK;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    LONG count, nread;
    VMHANDLE *svmh = frame->pix->vmh;

    if( svmh->vm_fh ) {
        if(flags & CFDF_SHOWPROGRESS)
            InitProgress(frame,XGetStr(MSG_BUILDING_NEW_FRAME),
                         0, PICSIZE(frame->pix)>>10, PPTBase);

        buf = pmalloc( COPYBUFSIZE );
        if(buf) {
            BPTR src, dst;

            src = svmh->vm_fh;
            dst = newframe->pix->vmh->vm_fh;

            FlushVMData( svmh, PPTBase ); /* Make sure disk data is valid */
            Seek(src,0,OFFSET_BEGINNING);
            Seek(dst,0,OFFSET_BEGINNING);
            count = 0;
            do {
                if(flags & CFDF_SHOWPROGRESS) Progress(frame,count>>10,PPTBase);
                nread = Read(src, buf, COPYBUFSIZE);
                Write(dst,buf,nread);
                count += nread;
            } while(nread == COPYBUFSIZE);
            Flush(dst);
            pfree(buf);
            D(bug("Wrote %lu bytes\n",count));

            /*
             *  Make sure the buffers are correct
             */

            SanitizeVMData( svmh, PPTBase );
            SanitizeVMData( newframe->pix->vmh, PPTBase );
            LoadVMData( newframe->pix->vmh, 0L, PPTBase );
        } else {
            D(bug("Dupframe(): Out of memory allocating copybuf\n"));
            SetErrorCode( frame, PERR_OUTOFMEMORY );
            res = PERR_OUTOFMEMORY;
        }

        if(flags & CFDF_SHOWPROGRESS) {
            FinishProgress( frame, PPTBase );
            CloseInfoWindow( frame->mywin, PPTBase );
            ClearProgress( frame, PPTBase );
        }
    } else {
        memcpy(newframe->pix->vmh->data, svmh->data, svmh->end - svmh->begin );
    }

    return res;
}
///

/// FindFrame()
/****u* pptsupport/FindFrame ******************************************
*
*   NAME
*       FindFrame -- Find a frame by its ID code
*
*   SYNOPSIS
*       frame = FindFrame( id )
*       D0                 D0
*
*       FRAME *MakeFrame( ID id );
*
*   FUNCTION
*       TBA
*
*   INPUTS
*       id - frame id code.
*
*   RESULT
*       frame - a frame handle. NULL if the id could not be found.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*    Find a frame by it's ID code. Returns NULL if not found.
*
*    This routine belongs to the support library. It
*    does receive the PPTBase in a6.  The reason it has not
*    been mentioned here is that I am lazy to write it all over
*    the source code, especially since all this needs is SysBase.
*/

Prototype FRAME * ASM  FindFrame( REGDECL(d0,ULONG) );

FRAME * SAVEDS ASM
FindFrame( REGPARAM(d0,ULONG,seekid) )
{
    struct Node *nd = globals->frames.lh_Head, *nn;

    SHLOCKGLOB();

    while( nn = nd->ln_Succ ) {
        if( ((FRAME *)nd)->ID == seekid ) {
            UNLOCKGLOB();
            return ((FRAME *)nd);
        }
        nd = nn;
    }

    if( clipframe && (clipframe->ID == seekid) ) {
        UNLOCKGLOB();
        return clipframe;
    }

    UNLOCKGLOB();
    return NULL;
}
///

/// MakeFrame()
/****u* pptsupport/MakeFrame ******************************************
*
*   NAME
*       MakeFrame -- Creates a new frame.
*
*   SYNOPSIS
*       frame = MakeFrame( oldframe )
*       D0                 A0
*
*       FRAME *MakeFrame( FRAME * );
*
*   FUNCTION
*       This will allocate space for a new FRAME structure. The FRAME
*       is essential in any PPT image handling. After obtaining a handle
*       to the frame, you should edit any values you feel necessary
*       and then call InitFrame(). Do not pass the frame to any other PPT
*       routines before calling InitFrame()!
*
*   INPUTS
*       oldframe - if you wish to duplicate a frame, you can either specify
*           the frame here or just call DupFrame(). If you specify the
*           oldframe, you should still call InitFrame(), though.
*
*   RESULT
*       frame - new frame handle
*
*   EXAMPLE
*
*   NOTES
*       Remove handles using RemFrame().
*
*   BUGS
*       This assumes the old frame will not coexist with a new one.
*
*   SEE ALSO
*       InitFrame(), DupFrame(), RemFrame()
*
*****************************************************************************
*
*   Allocate new frame structure. Initializes the values to something
*   intelligent. If old exists, the more useful values are copied from
*   it. Please note:
*
*       disp : shared
*       renderobject : shared
*       pix  : unique to each frame
*       vm_fh: unique to each frame
*       lastframe : unique
*       dpw  : shared
*       pw   : shared
*       mywin: shared
*       pix->data : unique
*       editwin : shared
*
*    Shared attributes are released by DeleteFrame() and unique by RemFrame()
*
*    Shared structures must be handled by the following routines:
*       MakeFrame() - when non-NULL, must still provide an alternative.
*
*    BUG: maybe the Node shouldn't be cleared after all?  The name-field...
*    BUG: Should lock!
*/

Prototype FRAME * ASM MakeFrame( REGDECL(a0,FRAME *), REGDECL(a6,EXTBASE *));

FRAME * SAVEDS ASM MakeFrame( REGPARAM(a0,FRAME *,old), REGPARAM(a6,EXTBASE *,PPTBase) )
{
    FRAME *f   = NULL;
    PIXINFO *p = NULL;
    DISPLAY *d = NULL;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    static ULONG id = 1;

    D(bug("MakeFrame( old = %08X )\n",old));

    /*
     *  Allocate structs.
     */

    if(!(f = pmalloc( sizeof(FRAME) )))
        return NULL;

    if(!(p = pmalloc( sizeof(PIXINFO) ))) {
        pfree(f); return NULL;
    }

    if(!(d = AllocDisplay( PPTBase ) ) ) {
        pfree(p); pfree(f); return NULL;
    }

    /*
     *  Initialize to sensible values.
     */

    bzero( p, sizeof(PIXINFO) );
    if(old == NULL) {
        p->width = p->height  = 0;
        p->colorspace         = CS_UNKNOWN;
        p->components         = 1;
        p->bits_per_component = 8;
        p->origdepth          = 1;
        p->DPIX = p->DPIY     = 72; /* A reasonable assumption */
        p->private1           = 0;
        p->bytes_per_row      = 0;  /* Will be filled in by InitFrame() */
        p->origmodeid         = INVALID_ID;
        p->vm_mode            = VMEM_ALWAYS;
    } else {
        bcopy( old->pix, p, sizeof(PIXINFO) );
        p->vmh = NULL;
    }

    if(old == NULL) {
        /* BUG: Should be in AllocDisplay() */
        d->renderq     = RENDER_NORMAL;
        d->dither      = DITHER_NONE;
        d->cmap_method = CMAP_MEDIANCUT;
        d->type        = DISPLAY_CUSTOMSCREEN;
        d->depth       = 1;
        d->ncolors     = 2;
        d->selpt       = 0xF0F0F0F0;
        d->saved_cmap  = CMAP_NONE;
    } else {
        /*
         *  We will initialize this as if this was a completely new frame,
         *  but we'll retain some information.  All objects which cannot
         *  be shared between two frames (ie all pointers) must be set
         *  to default values to mean that there are no objects attached.
         */
        bcopy( old->disp, d, sizeof(DISPLAY) );
        d->scr         = NULL;
        d->win         = NULL;
        d->colortable  = NULL;
        d->Win         = NULL;
        d->RenderArea  = NULL;
        d->GO_BottomProp = NULL;
        d->GO_RightProp = NULL;
        d->lock        = NULL;
        d->bitmaphandle= NULL;
    }

    bzero( f, sizeof(FRAME) );
    if(old == NULL) {
        UnselectImage( f );
        ChangeSelectMethod( f, GINP_LASSO_RECT );
    } else {
        bcopy(old,f,sizeof(FRAME) );
        bzero(&(f->nd), sizeof(struct Node) ); /* We are not in a list yet */
        f->parent       = old;
        f->renderobject = NULL;
        f->pw           = NULL;
        f->dpw          = NULL;
        f->mywin        = NULL;
        CopySelection( old, f );
        RescaleSelection( old, f );
        f->selection.selectport   = NULL;
        f->editwin      = NULL;
        f->attached     = 0L;
        ClearError( f );
        /* NOTE: Should alpha be copied? */
    }

    LOCKGLOB();
    f->ID = id++;
    UNLOCKGLOB();
    InitSemaphore( &(f->lock) );

    D(bug("Made a new frame @ %08X\n",f));

    f->pix  = p;
    f->disp = d;

    /*
     *  Set up variables that definitely should not be copied.
     */

    f->nd.ln_Name = f->name;
    f->lastframe = NULL;

    return f;
}
///

/// InitFrame()
/****u* pptsupport/InitFrame ******************************************
*
*   NAME
*       InitFrame -- Initialize a frame.
*
*   SYNOPSIS
*       success = InitFrame( frame )
*       D0                   A0
*
*       PERROR InitFrame( FRAME * );
*
*   FUNCTION
*       This function will allocate all the necessary system resources
*       for PPT to operate, ie. Virtual Memory, screen data, etc. Use
*       AFTER MakeFrame().
*
*   INPUTS
*       frame - Frame you wish to initialize.
*
*   RESULT
*       PERR_OK, if everything went nicely, otherwise an error code
*       telling what went wrong. See ppt.h for error codes.
*
*   EXAMPLE
*
*   NOTES
*       Do not change any values inside the FRAME structure after calling
*       this function. You may still remove the handle with RemFrame().
*
*   BUGS
*
*   SEE ALSO
*       MakeFrame(), DupFrame(), RemFrame()
*
*****************************************************************************
*
*    This initializes the frame buffers and sets up a completely working frame.
*    Some checking is made to see if the values are intelligent.
*
*/

Prototype PERROR ASM InitFrame( REGDECL(a0,FRAME *), REGDECL(a6,EXTBASE *) );

PERROR SAVEDS ASM InitFrame( REGPARAM(a0,FRAME *,f), REGPARAM(a6,EXTBASE *,PPTBase) )
{
    PIXINFO *p = f->pix;
    DISPLAY *d = f->disp;
    APTR SysBase = PPTBase->lb_Sys;
    BOOL sizechange = FALSE;
    D(APTR b);

    D(bug("InitFrame(%08X)\n",f));

    if(!CheckPtr(f, "InitFrame(): Illegal frame address" )) return PERR_FAILED;

    D(b = StartBench());

    LOCK(f);

    ClearError( f );

    /*
     *  Check up on some things first
     */

    if(p->width <= 0  || p->width > 16000 ||
       p->height <= 0 || p->height > 16000 ||
       p->components <= 0 || p->components > 10 ) {
        D(bug("Illegal values!\n"));
        SetErrorCode( f, PERR_INVALIDARGS );
        UNLOCK(f);
        return PERR_INVALIDARGS;
    }

    /*
     *  Initialize all sorts of miscallaneous variables
     */

    p->bytes_per_row = p->width * p->components;

    /*
     *  Initialize VM and Main Memory buffers, if need be.
     */

    if(p->vmh == NULL) {
        if( d && d->Win ) WindowBusy(d->Win);
        if(SetBuffers( f, PPTBase ) != PERR_OK) {
            if( d && d->Win ) WindowReady(d->Win);
            D(bug("SetBuffers() failed!\n"));
            UNLOCK(f);
            return PERR_INITFAILED;
        }
        if( d && d->Win ) WindowReady(d->Win);
        D(bug("\tVM page @ %08X...%08X\n",p->vmh->data,(ULONG)p->vmh->data+(p->vmh->end - p->vmh->begin) ));
    }

#ifdef TMPBUF_SUPPORTED
    if( NULL == (p->tmpbuf = pmalloc( ROWLEN(p) ))) {
        D(bug("\tCouldn't allocate tmpbuf\n"));
        return PERR_INITFAILED;
    }
#endif

    /*
     *  The select box must be updated, if the image size has changed. However,
     *  since the image size will change only if the entire image has been selected,
     *  we'll select the new image as well.
     */


    if( f->parent ) {
        CopySelection( f->parent, f );

        if( f->parent->pix->width != f->pix->width ||
            f->parent->pix->height != f->pix->height ) {
                //SelectWholeImage( f );
                RescaleSelection( f->parent, f );
                sizechange = TRUE;
        }
    }

    /*
     *  Set up the zoom box. Retains any old attributes, unless the
     *  size of the image has been changed.
     */

    if(sizechange || (f->parent == NULL) ) {
        D(bug("Size change detected, projecting changes...\n"));
        f->zoombox.Width  = p->width;
        f->zoombox.Height = p->height;
        f->zoombox.Top = f->zoombox.Left = 0;
    }

    UpdateFrameInfo( f );

    UNLOCK(f);

    D(StopBench(b));
    return PERR_OK;
}
///

/// RemFrame()
/****u* pptsupport/RemFrame ******************************************
*
*   NAME
*       RemFrame -- Remove a frame.
*
*   SYNOPSIS
*       RemFrame( frame )
*                 A0
*
*       VOID RemFrame( FRAME * );
*
*   FUNCTION
*       Use this function to remove the handle given to you by
*       MakeFrame() or DupFrame(). It does not matter if you call
*       this after InitFrame(). Of course, you should no longer use
*       the handle for anything after calling this function.
*
*   INPUTS
*       frame - Frame you wish to remove.
*
*   RESULT
*       A lot more free memory.
*
*   EXAMPLE
*
*   NOTES
*       Safe to call with NULL args.
*
*   BUGS
*
*   SEE ALSO
*       MakeFrame(), DupFrame(), InitFrame()
*
*****************************************************************************
*
*    Remove a frame. Safe to call even if NULL. If the frame is in a
*    list, you must take care of removing it from the list.
*    Deletes: frame, frame->pix, frame->disp, infowin and VM data files
*
*    This is re-entrant.
*
*    NB: Does not remove a frame from any lists!
*/

Prototype VOID ASM RemFrame( REGDECL(a0,FRAME *), REGDECL(a6,EXTBASE *) );

VOID SAVEDS ASM RemFrame( REGPARAM(a0,FRAME *,f), REGPARAM(a6,EXTBASE *,PPTBase) )
{
    D(bug("RemFrame( frame = %08X )\n",f));

    if(f) {

        if(!CheckPtr(f,"RemFrame(): Illegal FRAME pointer")) return;

        /*
         *  Remove possible undo buffers.
         */

        if( f->lastframe ) {
            RemFrame( f->lastframe, PPTBase );
            f->lastframe = NULL;
        }

        /*
         *  Remove associated structures.
         */

        if( f->mywin ) {
            DeleteInfoWindow( f->mywin, PPTBase );
        }

        if(f->pix) {

            if(CheckPtr(f->pix,"RemFrame(): Illegal PIXINFO pointer")) {

                if(f->pix->vmh) {
                    FreeBuffers( f, PPTBase );
                    DeleteVMData( f->pix->vmh, PPTBase );
                }

#ifdef TMPBUF_SUPPORTED
                if( f->pix->tmpbuf )
                    pfree( f->pix->tmpbuf );
#endif

                pfree(f->pix);
            }

        }

        if(f->disp) {

            if(CheckPtr(f->disp,"RemFrame(): Illegal DISPLAY pointer")) {
                FreeDisplay( f->disp, PPTBase );
            }

        }


        pfree(f);
    }
}
///

/// DupFrame()
/****u* pptsupport/DupFrame ******************************************
*
*   NAME
*       DupFrame -- Duplicate a frame.
*
*   SYNOPSIS
*       newframe = DupFrame( frame, flags )
*       D0                   A0,    D0
*
*       FRAME *DupFrame( FRAME *, ULONG );
*
*   FUNCTION
*       Makes a copy of the FRAME structure given - and if required -
*       also a copy of the image data.
*
*       You may get rid of the new frame by calling RemFrame().
*
*   INPUTS
*       frame - Frame you wish to duplicate.
*       flags - any of the following flags ORed together:
*           DFF_COPYDATA - Copies also the image data.
*           DFF_MAKENEWNAME - Generates a new name for the
*               image.
*
*   RESULT
*       newframe - A new frame that is an exact copy of the frame
*           you gave.
*
*   EXAMPLE
*
*   NOTES
*       If you don't want to copy the data, this routine just
*       calls MakeFrame() and InitFrame() to make a duplicate.
*
*       Most probably you will never require this routine, since the
*       frame passed to your external already is a copy. And if you
*       specify PPTX_NoNewFrame, you will probably want to modify the
*       sizes anyways, in which case this routine is not for you.
*
*   BUGS
*
*   SEE ALSO
*       MakeFrame(), RemFrame(), InitFrame()
*
*****************************************************************************
*
*    Clone a frame. This routine will also copy the shared variables.
*
*    Flags may have the following values:
*        DFF_COPYDATA = copy all old data to this new file.
*
*    This routine is part of support library and thus re-entrant.
*/

Prototype FRAME * ASM DupFrame( REGDECL(a0,FRAME *), REGDECL(d0,ULONG), REGDECL(a6,EXTBASE *) );

FRAME * SAVEDS ASM DupFrame( REGPARAM(a0,FRAME *,frame), REGPARAM(d0,ULONG,flags), REGPARAM(a6,EXTBASE *,PPTBase) )
{
    FRAME *newframe;
    ULONG copyflags = {0};

    D(bug("DupFrame( %08X, %lu )\n",frame,flags));

    if(!CheckPtr(frame,"DupFrame")) return NULL;

    if( newframe = MakeFrame( frame, PPTBase ) ) {

        /*
         *  Make up a new name unless instructed so
         */

        if(flags & DFF_MAKENEWNAME) {
            MakeFrameName( NULL, newframe->name, NAMELEN-1, PPTBase );
        }

        if(InitFrame( newframe, PPTBase ) != PERR_OK) {
            RemFrame( newframe, PPTBase );
            D(bug("\tInitFrame failed\n"));
            SetErrorCode( frame, PERR_INITFAILED );
            return NULL;
        }

        D(bug("Frame duplicated: %s (%s)\n",newframe->nd.ln_Name, newframe->path));

        /*
         *  Image data, but only if DFF_COPYDATA is specified.
         *  We'll use the progress indicator if copying would take
         *  a long time.
         */

        if( flags & DFF_COPYDATA ) {
            if( PICSIZE(frame->pix) > globals->userprefs->progress_filesize  ) {
                copyflags |= CFDF_SHOWPROGRESS;
            }

            if( CopyFrameData( frame, newframe, copyflags, PPTBase ) != PERR_OK ) {
                RemFrame(newframe,PPTBase);
                newframe = NULL;
            }

        }
    }
    return newframe;
}
///

/// NewFrame()
/****i* pptsupport/NewFrame ******************************************
*
*   NAME
*       NewFrame -- A quick way to build a frame.
*
*   SYNOPSIS
*       frame = NewFrame( width, height, components );
*       D0                D0     D1      D2
*
*       FRAME *NewFrame( ULONG, ULONG, UBYTE );
*
*   FUNCTION
*       A quick and easy way to create a frame if you don't bother
*       calling MakeFrame and InitFrame().
*
*   INPUTS
*       width, height - new frame dimensions.
*       components - number of components in this image.
*
*       If width OR height equal zero, then the VM buffers are not
*       allocated. In effect this is the same as just calling
*       MakeFrame(NULL).
*
*   RESULT
*       A new frame or NULL in case of failure.
*
*   EXAMPLE
*
*   NOTES
*       Obsolete, use MakeFrame() and InitFrame() instead.
*
*   BUGS
*
*   SEE ALSO
*       MakeFrame(), InitFrame(), RemFrame().
*
*****************************************************************************
*
*    Allocate & initialize a new frame. Won't add it to the list.
*    Returns NULL, if couldn't allocate memory. Also won't clear the memory
*    area. Enter with 0 for width and height if you do not wish to allocate
*    VM.
*
*    Support function. Re-entrant. Really a shorthand front-end for
*    MakeFrame()/InitFrame()
*
*    BUG: Some variables are not initialized properly.
*
*/

Prototype FRAME * ASM   NewFrame( REGDECL(d0,ULONG), REGDECL(d1,ULONG), REGDECL(d2,UBYTE), REGDECL(a6,EXTBASE *) );

FRAME * SAVEDS ASM NewFrame( REGPARAM(d0,ULONG,width), REGPARAM(d1,ULONG,height),
                             REGPARAM(d2,UBYTE,components), REGPARAM(a6,EXTBASE *,xd) )
{
    FRAME *f;

    D(bug("NewFrame( w=%lu, h=%lu, c=%lu );\n", width,height,components ));

    if( f = MakeFrame( NULL, xd ) ) {

        f->pix->width      = width;
        f->pix->height     = height;
        f->pix->components = components;

        if(components == 1)
            f->pix->colorspace = CS_GRAYLEVEL;
        else if(components == 3)
            f->pix->colorspace = CS_RGB;
        else if(components == 4)
            f->pix->colorspace = CS_ARGB;
        else
            f->pix->colorspace = CS_UNKNOWN;

        if(width != 0 && height != 0) {
            if(InitFrame(f,xd) != PERR_OK) {
                D(bug("\tInitFrame failed\n"));
                RemFrame(f, xd);
                return NULL;
            }
        } else {
            D(bug("\tSize == 0, I won't create memory buffers\n"));
        }
    }

    return f;
}
///
/// CopyFrame()
#if 0
/****i* pptsupport/CopyFrame ******************************************
*
*   NAME
*       CopyFrame -- Copy a frame fully
*
*   SYNOPSIS
*       success = CopyFrame( source );
*       D0                   A0
*
*       PERROR MakeFrame( FRAME *, FRAME *);
*
*   FUNCTION
*       This function will copy the information from one frame to
*       an another frame.
*
*       The difference between this function and CopyFrameData()
*       is that this one does have error checking as well as it will
*       create the exactly correct frame.
*
*       The difference between this function and DupFrame() is a bit
*       more subtle one.  While they basically use the same function,
*       CopyFrame() will do things a bit more automatically and
*       it will ensure that the new frame is a completely independent
*       system.
*
*   INPUTS
*       source - the original frame.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       Does not allocate info window, which means that any
*       progress display will not be done.
*
*   SEE ALSO
*       CopyFrameData()
*
*****************************************************************************
*  Make a full duplicate.
*/

Prototype FRAME * ASM CopyFrame( REGDECL(a0,FRAME *), REGDECL(a6,EXTBASE *) );

FRAME *SAVEDS ASM CopyFrame( REGPARAM(a0,FRAME *,source),
                             REGPARAM(a6,EXTBASE *,PPTBase) )
{
    FRAME *new = NULL;
    PIXINFO *sp = source->pix;
    ULONG copyflags = 0;

    D(bug("CopyFrame(%08X)\n",source));

    if( CheckPtr( source, "CopyFrame" ) ) {
        if( new = MakeFrame( source, PPTBase) ) {

            MakeFrameName( NULL, new->name, NAMELEN-1, PPTBase );

            if( InitFrame( new, PPTBase ) == PERR_OK ) {
                if( PICSIZE(sp) > globals->userprefs->progress_filesize  )
                    copyflags |= CFDF_SHOWPROGRESS;

                CopyFrameData( source, new, copyflags, PPTBase );
            } else {
                RemFrame( new, PPTBase );
                new = NULL;
            }
        }
    }

    return new;
}
#endif
///
/// AttachFrame()
/****i* pptsupport/AttachFrame ******************************************
*
*   NAME
*
*   SYNOPSIS
*       result = AttachFrame(  );
*       D0
*
*       PERROR AttachFrame();
*
*   FUNCTION
*       TBA
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       This entry very incomplete.
*
*   SEE ALSO
*
*
*****************************************************************************
*
*
*   This attaches a frame to an another frame, so that they can be
*   handled (locked, etc.) together.
*
*   This will in the future also handle animframes.
*
*/

Prototype BOOL ASM AttachFrame( REGDECL(a0,FRAME *),REGDECL(a1,FRAME *),REGDECL(d0,ULONG),REGDECL(a6,EXTBASE *) );

SAVEDS ASM
BOOL AttachFrame( REGPARAM(a0,FRAME *,dst),
                  REGPARAM(a1,FRAME *,src),
                  REGPARAM(d0,ULONG, how),
                  REGPARAM(a6,EXTBASE *,PPTBase) )
{
    FRAME *cur, *next;

    D(bug("AttachFrame( %08X to %08X )\n", src, dst ));

    if( (dst == src) || (dst->ID == src->ID ) ) {
        Req(NEGNUL,NULL,XGetStr(MSG_CANNOT_ATTACH_TO_ITSELF));
        return FALSE;
    }

    switch(how) {
        case ATTACH_ALPHA:
#ifdef USE_OLD_ALPHA
            if(IsAttached(dst, src->ID) || IsAttached(src, dst->ID) ) {
                Req(NEGNUL,NULL,"\nThe frame cannot be attached as an alpha channel\n"
                                "because it already has been attached to it somehow.\n");
                return FALSE;
            }
            dst->alpha = src->ID;
#endif
            break;

        case ATTACH_SIMPLE:

            /*
             *  Adds the srcframe to the end of simple attachment list
             *  BUG: Bloody slow.
             */
#ifdef USE_OLD_ALPHA
            if( (dst->alpha == src->ID) || (src->alpha == dst->ID) ) {
                Req(NEGNUL,NULL,"\nThis frame is already an alpha channel!\n"
                                "You cannot attach it!\n" );
                return FALSE;
            }
#endif

            cur = dst;
            while( (next = FindFrame(cur->attached)) ) cur = next;

            /*
             *  Attach it and set the busy status to be that of the parent,
             *  as they are supposed now to be a part of the same unit.
             */

            LOCK(cur);
            LOCK(src);
            cur->attached = src->ID;
            src->attached = 0L;
            ObtainFrame( src, cur->busy );
//            ChangeBusyStatus( src, cur->busy );
//            src->busycount = cur->busycount;

            UNLOCK(src);
            UNLOCK(cur);
            break;

        default:
            InternalError("Unknown AttachFrame id received");
            break;
    }

    return TRUE;
}
///

/// RemoveSimpleAttachments()
#if 0
/*
    BUG: Untested and unused.
 */
Prototype VOID RemoveAttachment( FRAME *frame, FRAME *attached );

VOID RemoveAttachment( FRAME *frame, FRAME *attached )
{
    FRAME *cur, *next;

    D(bug("\tRemoveAttachment( f=%08X, %08X )\n", frame, attached ));

    if( IsAttached( frame, attached->ID ) ) {
        cur = frame;
        while( (next = FindFrame( cur->attached )) != attached && next) cur = next;
        LOCK(cur);
        cur->attached = next->attached;
        UNLOCK(cur);
    }
}
#endif

/*
    Removes all simple attachments from a frame.
*/

Prototype VOID ASM  RemoveSimpleAttachments(REGDECL(a0,FRAME *) );

VOID SAVEDS ASM
RemoveSimpleAttachments(REGPARAM(a0,FRAME *,frame) )
{
    FRAME *next, *cur;

    D(bug("Removing attachments from frame %08X\n",frame));

    cur = frame;
    while( (next = FindFrame(cur->attached)) ) {

        LOCK(cur);
        ReleaseFrame(next);
        cur->attached = 0L;
        UNLOCK(cur);

        cur = next;
    }
}
///
/// SetBuffers()
/*
    This sets the VM and real memory buffers. If there are any
    buffers existing, will not allocate them.
    Re-entrant.
*/
Local
PERROR SetBuffers( FRAME *frame, EXTBASE *PPTBase )
{
    ULONG realsize, bufsize;
    PIXINFO *p = frame->pix;

    D(bug("SetBuffers(%08X)\n",frame));

    realsize = p->height * p->width * p->components;

    if( realsize > globals->userprefs->vmbufsiz * 1024L && (p->vm_mode == VMEM_ALWAYS))
        bufsize = globals->userprefs->vmbufsiz*1024L;
    else
        bufsize = realsize;

    /*
     *  Make sure a vm handle exists
     */

    if( !p->vmh ) {
        D(bug("\tAllocating VM handle\n"));
        if( !(p->vmh = AllocVMHandle( PPTBase ))) {
            SetErrorCode( frame, PERR_OUTOFMEMORY );
            return PERR_OUTOFMEMORY;
        }
    }

    /*
     *  Create memory page
     */

    if( p->vmh->data == NULL ) {
        D(bug("\tAllocating new VM page (%lu bytes)\n", bufsize));

        p->vmh->data = pmalloc( bufsize );

        if( NULL == p->vmh->data ) {
            SetErrorCode( frame, PERR_OUTOFMEMORY );
            FreeVMHandle( p->vmh, PPTBase );
            p->vmh = NULL;
            return PERR_OUTOFMEMORY;
        }
    }

    /*
     *  Create disk page, if needed
     */

    if( p->vm_mode == VMEM_ALWAYS ) {
        if( !p->vmh->vm_fh ) {
            if(CreateVMData( p->vmh, realsize, PPTBase ) != PERR_OK ) {
                D(bug("\tCreateVMData() failed\n"));
                SetErrorCode(frame,PERR_OUTOFMEMORY);
                FreeBuffers( frame, PPTBase );
                FreeVMHandle( p->vmh, PPTBase );
                p->vmh = NULL;
                return PERR_OUTOFMEMORY;
            }
        }
    } else {
        p->vmh->end = p->vmh->last = realsize;
    }

    return PERR_OK;
}
///
/// FreeBuffers()
Local
VOID FreeBuffers( FRAME *frame, EXTBASE *PPTBase )
{
    PIXINFO *p = frame->pix;

    if( p->vmh && p->vmh->data ) {
        pfree( p->vmh->data );
        p->vmh->data = NULL;
    }
}
///
/// SetVMemMode()
Prototype PERROR SetVMemMode( FRAME *frame, ULONG mode, EXTBASE *PPTBase );

PERROR SetVMemMode( FRAME *frame, ULONG mode, EXTBASE *PPTBase )
{
    VMHANDLE *vmh = frame->pix->vmh;
    PERROR res = PERR_OK;

    frame->pix->vm_mode = mode;

    if( mode == VMEM_NEVER && vmh && vmh->vm_fh ) {
        res = CloseVMFile( vmh, PPTBase );
    } else if( mode == VMEM_ALWAYS && vmh && !vmh->vm_fh ) {
        res = SetBuffers( frame, PPTBase );
    }
    return res;
}
///
/*-------------------------------------------------------------------*/
/*                           END OF CODE                             */
/*-------------------------------------------------------------------*/

