/*
    PROJECT: PPT
    MODULE:  edit.c

    $Id: edit.c,v 1.11 1999/10/02 15:39:47 jj Exp $

    This module contains code for editing facilities. Basically
    they are all external modules, but they are fast enough to
    be internal and they are not spawned.
*/

/*------------------------------------------------------------------------*/

#include "defs.h"
#include "misc.h"

/*------------------------------------------------------------------------*/
/* Prototypes */

Prototype VOID  Composite( FRAME *, FRAME * );
Prototype FRAME *Edit( FRAME *f, ULONG type );

/*------------------------------------------------------------------------*/
/*  Global variables  */

FRAME *clipframe = NULL;

/*------------------------------------------------------------------------*/
/* Code */

/*
    This is a compositing system. Basically, it should just
    boot up an external named Composite. Or whatever the user has
    set it up to do.

    src = image to be composited
    dst = image onto which src is composited

    BUG: Should not be here.
    BUG: Should use user-defined name
*/

VOID Composite( FRAME *dst, FRAME *src )
{
    char buffer[100];

    if(AttachFrame( dst, src, ATTACH_SIMPLE, globxd )) {

        sprintf(buffer, "WITH %ld", src->ID );

        if( RunFilterCommand( dst, "COMPOSITE", buffer ) != PERR_OK ) {
            RemoveSimpleAttachments( dst );
            Req( GetFrameWin(dst),NULL,GetStr(MSG_PERR_NO_NEW_PROCESS) );
        }

    }
}

/*
    This cuts the selected area away, putting zeros in it's way.
*/

PERROR
ASM CutPixel( REGPARAM(a0,UBYTE *,r), REGPARAM(a1,UBYTE *,g),
              REGPARAM(a2,UBYTE *,b), REGPARAM(a6,struct PPTBase *,PPTBase) )
{
    *r = *g = *b = (UBYTE) 0;
    return PERR_OK;
}


/*
    Cut out the marked area from the frame. Returns the new frame.

    If keep == TRUE, then won't rename the old one or anything.

    BUG: Does not take non-rectangular areas into account.
*/

FRAME *CutToFrame( FRAME *oldframe, BOOL keep  )
{
    FRAME *newframe = NULL;
    WORD height, width;
    WORD srow,drow = 0;
    ULONG offset, amount;

    D(bug("CutToFrame()\n"));

    height = oldframe->selbox.MaxY - oldframe->selbox.MinY;
    width  = oldframe->selbox.MaxX - oldframe->selbox.MinX;
    D(bug("\tHeight = %d, Width = %d\n",height,width));

    if(height < 0 || width < 0) {
        Req( GetFrameWin(oldframe), "What!?","Soft error: CutToFrame() had digesting problems!" );
        return NULL;
    }

    newframe = MakeFrame( oldframe, globxd );
    if(newframe) {
        char namebuf[16];

        newframe->parent = NULL; /* This is needed because our frame may not have one */

        newframe->pix->height = height;
        newframe->pix->width  = width;

        if( keep == FALSE ) {
            MakeFrameName( NULL, namebuf, 15, globxd );
            strcpy(newframe->name,namebuf);
            strcpy(newframe->path,"");
        }

        UnselectImage( newframe );

        if(InitFrame( newframe, globxd) != PERR_OK) {
            Req(GetFrameWin( oldframe ), NULL, "Cannot initialize frame!\n");
            D(bug("InitFrame failed\n"));
            RemFrame( newframe, globxd );
            return NULL;
        }

        offset = oldframe->pix->components * oldframe->selbox.MinX;
        amount = oldframe->pix->components * width;

        for(srow = oldframe->selbox.MinY; srow < oldframe->selbox.MaxY; srow++) {
            UBYTE *scp, *dcp;
            scp = GetPixelRow( oldframe, srow, globxd );
            dcp = GetPixelRow( newframe, drow, globxd );
            CopyMem( scp + offset, dcp, amount );
            PutPixelRow( newframe, drow, dcp, globxd );
            drow++;
        }
    }

    return newframe;
}


FRAME *CopyToClipboard( FRAME *src )
{
    FRAME *t;

    if( t = CutToFrame( src, TRUE ) ) {
        LOCKGLOB();
        if( clipframe ) RemFrame(clipframe,globxd);
        clipframe = t;
        UNLOCKGLOB();
        return NEGNUL;
    }

    return NULL;
}

FRAME *PasteFromClipboard( FRAME *dst )
{
    if( clipframe ) {
        Composite( dst, clipframe );
    }
    return NULL;
}

/*
    This is the master routine, that diverts the control to different
    subroutines. Code is the command to be executed.
    BUG: does not respond well to failed commands
*/

FRAME *Edit( FRAME *f, ULONG type )
{
    FRAME *newframe;
    char  *title;
    FPTR  code;

    switch( type ) {
        case EDITCMD_CUT:
            if(!CopyToClipboard( f )) return NULL;
            code = (FPTR)CutPixel;
            title = "Cutting";
            break;

        case EDITCMD_COPY:
            return( CopyToClipboard( f ) );

        case EDITCMD_PASTE:
            return( PasteFromClipboard( f ) );

        case EDITCMD_CUTFRAME:
            return( CutToFrame( f, FALSE ) );

        case EDITCMD_CROPFRAME:
            return( CutToFrame( f, TRUE ) );

        default:
            InternalError("Command not yet implemented");
            return NULL;
            break;
    }

    if( ObtainFrame( f, BUSY_EDITING ) == FALSE )
        return NULL;

    newframe = DupFrame( f, DFF_COPYDATA, globxd );
    if(!newframe) { ReleaseFrame( f ); return NULL; }
    newframe->parent = f;

    UpdateProgress( newframe, title, 0, globxd );
    ExecEasyFilter( newframe, code, globxd );
    ClearProgress( newframe, globxd);

    newframe->parent = NULL;
    ReleaseFrame( f );
    ReleaseFrame( newframe );

    return newframe;
}

