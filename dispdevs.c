/*
    $Id: dispdevs.c,v 1.20 1999/10/02 16:33:07 jj Exp $

    Has code for different display devices.

    Currently just a standard Amiga Bitmapped screen is supported.
*/

#include "defs.h"
#include "misc.h"
#include "render.h"

#ifndef GRAPHICS_GFXBASE_H
#include <graphics/gfxbase.h>
#endif

#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

Prototype PERROR Device_BitMappedScreenI( struct RenderObject * );

/*------------------------------------------------------------------*/
/*
    Internal definitions
*/



/*
 *  The BitMapObject for the display data.  This object is shared also
 *  with the CGX display system, as most of the routines are quite
 *  common.
 */

struct BitMapObject {

    /*
     *  These two contain private copies of the screen structures.
     *  If they have not been set, it means that the current display
     *  must be a quickrender one.
     */

    struct Screen   *scr;
    struct Window   *win;

    /*
     *  These contain pointers to various RastPorts, like the RP to be
     *  renderer into and the temporary RP for WritePixelLine8()
     */

    struct RastPort *temprp;
    struct RastPort *destrp;

    /*
     *  This one tells if it's safe to commence in IDCMP handling
     */

    BOOL            idcmpok;

};


/*
    Deallocate TempRP space.
    This routine can be called from different tasks.
*/
VOID BMS_DestroyTempRp( struct RenderObject *rdo )
{
    struct GfxBase *GfxBase = rdo->PPTBase->lb_Gfx;
    int i;
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;
    struct RastPort *temprp = bmo->temprp;
    struct BitMap *tempbm;
    FRAME *frame = rdo->frame;

    D(bug("DestroyTempRP()\n"));

    if( temprp ) {

        tempbm = temprp->BitMap;

        if( tempbm ) {
            if( GFXV39 ) {
                FreeBitMap( tempbm );
            } else {
                for(i = 0; i < tempbm->Depth && tempbm->Planes[i]; i++)
                    FreeRaster(tempbm->Planes[i], frame->pix->width, 1 );
                sfree( tempbm );
            }
        }
        sfree( temprp );
    }
}

/*
    Allocate a temporary RastPort. RenderObject MUST be correctly
    initialized before calling this function.

    The RastPort contains enough space to contain one line.

    This routine may be called from different tasks.
*/

PERROR BMS_GetTempRp( struct RenderObject *rdo )
{
    struct GfxBase *GfxBase = rdo->PPTBase->lb_Gfx;
    PERROR res = PERR_OK;
    struct RastPort *temprp, *dest;
    struct BitMap *tempbm;
    ULONG width = (ULONG) rdo->frame->pix->width;
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;

    D(bug("\tAllocating temporary RastPort space (dest=%08X,width = %d)...\n",
             bmo->destrp,width));

    dest = bmo->destrp;

    temprp = smalloc( sizeof( struct RastPort ) );
    if( temprp ) {

        if( GFXV39 ) {
            bcopy( dest, temprp, sizeof(struct RastPort) );
            temprp->Layer = NULL;
            temprp->BitMap = AllocBitMap( width, 1, dest->BitMap->Depth, 0L, dest->BitMap );
            if( !temprp->BitMap ) {
                res = PERR_OUTOFMEMORY;
                D(bug("\tError allocating TempBitMap\n"));
            }
        } else {
            if( tempbm = smalloc( sizeof( struct BitMap ) ) ) {
                int i;

                bcopy( dest, temprp, sizeof(struct RastPort) );
                temprp->Layer = NULL;
                temprp->BitMap = tempbm;
                tempbm->Rows = 1;
                tempbm->Depth = dest->BitMap->Depth;
                tempbm->BytesPerRow = (((width+15)>>4)<<1);
                for(i = 0; i < dest->BitMap->Depth; i++) {
                    tempbm->Planes[i] = AllocRaster(width,1);
                    if(!tempbm->Planes[i]) {
                        res = PERR_OUTOFMEMORY;
                        D(bug("\tError allocating bitplanes\n"));
                        break;
                    }
                }
            } else {
                res = PERR_OUTOFMEMORY;
                D(bug("\tError allocating TempBitMap\n"));
            }
        }
    } else  {
        res = PERR_OUTOFMEMORY;
        D(bug("\tError allocating TempRastPort\n"));
    }

    bmo->temprp = temprp;

    /*
     *  Make the cleanup
     */

    if(res != PERR_OK) {
        BMS_DestroyTempRp( rdo );
    }

    D(bug("\tAllocated new temporary rastport area @ %08X\n",temprp ));

    return res;
}


/*
    May be called from different tasks.
*/
PERROR Device_BitMappedScreenClose( struct RenderObject *rdo )
{
    PERROR res = PERR_OK;
    APTR IntuitionBase = rdo->PPTBase->lb_Intuition;
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;

    D(bug("BitMapScreenClose(%08X)\n",rdo));

    /*
     *  Remove the temprp space and clean up bmo
     */

    if(bmo) {
        BMS_DestroyTempRp( rdo );

        if( bmo->win ) {

            /*
             *  Close the window. We share an IDCMP port with the main
             *  window, so we'd better be careful.
             */

            bmo->idcmpok = FALSE; /* Disable */

            D(bug("\tClosing window\n"));

            CloseWindowSafely( bmo->win );
            bmo->win = NULL;
            rdo->SigMask = 0L;
        }

        /*
         *  Attempt to close screen.
         */

        if( bmo->scr != NULL ) {
            CloseScreen( bmo->scr );
            bmo->scr = NULL;
            D(bug("\tClosed screen\n"));
        }

    } /* if(bmo) */

    return res;
}




/*
    BUG: Error messages are too terse
*/

PERROR Device_BitMappedScreenOpen( struct RenderObject *rdo )
{
    struct IntuitionBase *IntuitionBase = rdo->PPTBase->lb_Intuition;
    struct Screen *scr;
    struct Window *win;
    FRAME *     frame = rdo->frame;
    PERROR      res = PERR_OK;
    ULONG       flags = WFLG_BORDERLESS|WFLG_RMBTRAP|WFLG_NOCAREREFRESH;
    ULONG       idcmpflags = IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE;
    ULONG       modeid = frame->disp->dispid;
    UBYTE       depth = frame->disp->depth;
    UWORD       openheight, openwidth;
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;
    struct TagItem scrtaglist[] = {
            SA_Width, 0,
            SA_Height, 0,
            SA_Depth, 0,
            SA_DisplayID, 0,
            SA_FullPalette, TRUE,
            SA_Type, CUSTOMSCREEN,
            SA_Quiet, TRUE,
            SA_AutoScroll, TRUE,
            SA_Behind, TRUE,
            SA_Overscan, OSCAN_TEXT,
            SA_Title, (ULONG)"PPT Render",
            SA_ShowTitle, FALSE,
            TAG_END };

    D(bug("BitMapScreenOpen()\n"));

    /*
     *  First, check whether we are already open or not.
     */

    scr = bmo->scr;
    if(scr) {
        D(bug("\tScreen already exists\n"));
        return PERR_OK;
    }

    /*
     *  OK, so we were not open. Let's start by opening up a screen.
     *  We need to have a nice guess on the modeid and depth first.
     */

    D(bug("\tScreen is not open, attempting to do something about it...\n"));
    D(bug("\t\tHeight = %lu, Width = %lu, Depth = %u, ModeId = 0x%08X\n",
          MAX(frame->pix->height,200), MAX(frame->pix->width, 320),
          depth, modeid ));


    scrtaglist[0].ti_Data = MAX(frame->pix->width,320);
    scrtaglist[1].ti_Data = MAX(frame->pix->height,200);
    scrtaglist[2].ti_Data = depth;
    scrtaglist[3].ti_Data = modeid;

    scr = OpenScreenTagList( NULL, scrtaglist );

    /*
     *  The open the window, if the screen open succeeded.
     */

    if(scr) {
        struct TagItem windowtags[] = {
            WA_CustomScreen, 0,
            WA_Flags, 0,
            WA_Height,0,
            WA_Width, 0,
            WA_Left, 0, WA_Top,0,
            WA_IDCMP, NULL, /* Set IDCMP later */
            WA_ScreenTitle, NULL,
            // WA_Title, NULL,
            TAG_END, 0L };

        D(bug("\tScreen open successful\n"));

        openheight = frame->pix->height;
        openwidth  = frame->pix->width;

        windowtags[0].ti_Data = (ULONG)scr;
        windowtags[1].ti_Data = flags;
        windowtags[2].ti_Data = openheight;
        windowtags[3].ti_Data = openwidth;

        win = OpenWindowTagList( NULL, windowtags );

        if( win ) {

            D(bug("Window open successfull\n"));

            /*
             *  Set the IDCMP port to be common with the main window
             */

            win->UserPort = MAINWIN->UserPort;
            ModifyIDCMP( win, idcmpflags );

            /*
             *  Allocate a temporary rastport and set bmo variables
             */

            bmo->destrp = win->RPort;
            rdo->SigMask = 1 << win->UserPort->mp_SigBit;
            frame->disp->dispid = modeid;
            frame->disp->width  = openwidth;
            frame->disp->height = openheight;

            if( (res = BMS_GetTempRp( rdo )) != PERR_OK) {
                D(bug("\tTempRP allocation failed!\n"));
            }

        } else { /* if(win) */
            D(bug("\tCould not open render window!\n"));
            res = PERR_WINDOWOPEN;
        }

    } else { /* if(scr) */
        D(bug("\tCould not open render screen!\n"));
        res = PERR_WINDOWOPEN;
    }

    /*
     *  On failure, do the cleanup, otherwise start IDCMP handling.
     */

    if( res != PERR_OK ) {
        Device_BitMappedScreenClose( rdo );
    } else {
        bmo->idcmpok = TRUE;
    }

    /*
     *  Set variables and return
     */

    bmo->scr = scr;
    bmo->win = win;

    return res;
}

/*
    This writes one line onto a bitmapped display, using
    V36+ routines.
*/

PERROR Device_BitMappedScreenWrite( struct RenderObject *rdo, WORD row )
{
    FRAME *frame = rdo->frame;
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;
    APTR GfxBase = rdo->PPTBase->lb_Gfx;

    // D(bug("BitMapScreenWrite()\n"));

    /*
     *  Do the the chunky->planar conversion by using the OS V36 routines
     */

    // D(bug("\tDest:%08X, row=%d, width=%d, buffer:%08X,temprp:%08X\n",
    //        bmo->destrp, row, frame->pix->width, rdo->buffer, bmo->temprp ));

    WritePixelLine8( bmo->destrp, 0, row, frame->pix->width, rdo->buffer, bmo->temprp );

    return PERR_OK;
}

/*
    Load up a colortable into the display.
    Never fails.
    BUG: ncolors should be a parameter, too.

    Safe to call from separate task.
*/

#define SWAP(type,a,b) { type tmp; tmp = a; a = b; b = tmp; }

PERROR Device_BitMappedScreenLoadCMap( struct RenderObject *rdo, COLORMAP *colortable )
{
    struct ViewPort *vPort;
    long nColors = rdo->ncolors;
    UBYTE ocolor;
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;

    D(bug("BitMapLoadCMap()\n"));

    /*
     *  Make sure the first pen is close to black and the second one
     *  is close to white. (This is to ensure the menu colors look somewhat
     *  correct.)
     */

    ocolor = BestMatchPen8(colortable,nColors,255,255,255); /* White */
    SWAP( COLORMAP, colortable[1], colortable[ocolor] );

    D(bug("\tColor %d is white\n",ocolor));

    ocolor = BestMatchPen8(colortable,nColors,0,0,0); /* Black */
    SWAP( COLORMAP, colortable[0], colortable[ocolor] );

    D(bug("\tColor %d is black\n",ocolor));

    if(rdo->frame->disp->forcebw) {
        colortable[0].r = colortable[0].g = colortable[0].b = 0;
        colortable[1].r = colortable[1].g = colortable[1].b = 255;
    }

    // DoShowColorTable(4,colortable);

    if(bmo->scr) {
        vPort   = &(bmo->scr->ViewPort);
        nColors = rdo->ncolors;

        LoadRGB8( vPort, colortable, nColors, rdo->PPTBase );
    }

    return PERR_OK;
}

/*
    Just brings the display window to front and activates it.
*/
Local
PERROR Device_BitMappedScreenActivate( struct RenderObject *rdo )
{
    APTR IntuitionBase = rdo->PPTBase->lb_Intuition;
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;

    if(bmo->scr) ScreenToFront( bmo->scr );
    if(bmo->win) ActivateWindow( bmo->win );

    return PERR_OK;
}

/*
    BUG: Should really return error code if fails.
*/

PERROR Device_BitMappedScreenD( struct RenderObject *rdo )
{
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;

    D(bug("BitMapScreenD()\n"));

    if(bmo) {

        /*
         *  If the screen still exists, kill it.
         */

        if(bmo->scr)
            Device_BitMappedScreenClose( rdo );

        rdo->DispDeviceObject = NULL;
        sfree( bmo );
    }
    return PERR_OK;
}


/*
    Handles IDCMP messages from the display window.

    NOT SAFE TO CALL FROM SEPARATE TASKS
*/

Local
LONG Device_BitMappedScreenIDCMP( struct RenderObject *rdo )
{
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;
    struct IntuiMessage *imsg;
    struct Node         *succ;
    struct Window       *win;
    FRAME               *frame = rdo->frame;

    /*
     *  First, it may be possible that we do not exist yet.
     */

    if( !bmo ) return 0L;

    if( bmo->idcmpok == FALSE) return 0L;

    if( !(win = bmo->win)) {
        InternalError( "Control has run haywire in IDCMP handler!" );
        return 0L;
    }

    if( !win->UserPort ) {
        InternalError( "No IDCMP port for this window?!?" );
        return 0L;
    }

    /*
     *  Start the actual IDCMP handling loop. Looks kludgy, I know.
     */

    imsg = (struct IntuiMessage *) (win->UserPort->mp_MsgList.lh_Head);

    while( succ = imsg->ExecMessage.mn_Node.ln_Succ ) {
        if( imsg->IDCMPWindow == win ) {

            switch(imsg->Class) {
                case IDCMP_CLOSEWINDOW:
                    /*
                     *  BUG: Shouldn't this return something else?
                     */
                    CloseRender( frame,globxd );
                    // frame->selstatus &= ~(0x01);
                    break;

                case IDCMP_INACTIVEWINDOW:
                    break;

                case IDCMP_ACTIVEWINDOW:
                    DoMainList( frame );
                    break;

                case IDCMP_MOUSEBUTTONS:

                    switch(imsg->Code) {
                        case SELECTDOWN: /* Mark the start of select area */
                            ScreenToFront( MAINSCR );
                            break;
                        case SELECTUP:
                            break;
                        case MENUUP:
                            break;
                        case MENUDOWN:
                            ScreenToFront( MAINSCR );
                            break;
                    }
                    break;

                case IDCMP_MOUSEMOVE:
                    break;

                default:
                    break;
            }
            Remove( (struct Node *)imsg );
            ReplyMsg( (struct Message *)imsg ); /* BUG: should be earlier */
        } /* If message is for this window */
        imsg = (struct IntuiMessage *)succ;
    }
    return 0;
}

/*
    Safe to call from separate tasks.  Works only on original Amiga
    screens that are accessible this way.
 */
PLANEPTR Device_BitMappedScreenGetRow( struct RenderObject *rdo, UWORD row, UWORD plane )
{
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;
    struct BitMap *bmap;
    ULONG  offset;

    if(!bmo) return NULL;

    bmap = bmo->win->RPort->BitMap;
    offset = bmap->BytesPerRow * row;

    return( bmap->Planes[plane] + offset );
}

/*
    Warning: destroys rdo->buffer contents, overwriting them with
    the pixel data
 */

UBYTE *Device_BitMappedScreenRead( struct RenderObject *rdo, UWORD row )
{
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;
    struct GfxBase *GfxBase = rdo->PPTBase->lb_Gfx;

    D(bug("Device_BitmappedScreenRead( %08X, row=%d )\n",rdo, row));

    if(!bmo) return NULL;

    ReadPixelLine8( bmo->destrp, 0, row, rdo->frame->pix->width, rdo->buffer, bmo->temprp );

    return rdo->buffer;
}

/*
    Return the screen pointer if we wish to change the colormap or something.
*/
struct Screen *Device_BitMappedScreenGetScreen( struct RenderObject *rdo )
{
    struct BitMapObject *bmo = (struct BitMapObject *)rdo->DispDeviceObject;

    if(bmo)
        return bmo->scr;
    else
        return NULL;
}


/*
    May be called from a separate task.
*/
PERROR Device_BitMappedScreenI( struct RenderObject *rdo )
{
    struct BitMapObject *bmo;
    PERROR res = PERR_OK;

    D(bug("BitMapScreenI()\n"));

    bmo = (struct BitMapObject *)smalloc( sizeof( struct BitMapObject ) );
    if(!bmo) {
        res = PERR_OUTOFMEMORY;
    } else {
        bzero( bmo, sizeof( struct BitMapObject ));

        bmo->idcmpok = FALSE;

        /*
         *  Set up function table
         */

        rdo->WriteLine   = Device_BitMappedScreenWrite;
        rdo->DispDeviceD = Device_BitMappedScreenD;
        rdo->LoadCMap    = Device_BitMappedScreenLoadCMap;
        rdo->DispDeviceOpen = Device_BitMappedScreenOpen;
        rdo->DispDeviceClose = Device_BitMappedScreenClose;
        rdo->ActivateDisplay = Device_BitMappedScreenActivate;
        rdo->HandleDispIDCMP = Device_BitMappedScreenIDCMP;
        rdo->GetRow = Device_BitMappedScreenRead;
        rdo->GetScreen = Device_BitMappedScreenGetScreen;

        rdo->DispDeviceObject = (APTR) bmo;
    }

    return res;
}

