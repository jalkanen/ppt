/*
    PROJECT: ppt
    MODULE : palette.c

    $Id: palette_edit.c,v 1.11 1999/10/02 16:33:07 jj Exp $

    Palette selector and editor.
*/

#include "defs.h"
#include "misc.h"

#include <clib/alib_protos.h>

#include "gui.h"
#include "render.h"

#include <libraries/iffparse.h>
#include <proto/iffparse.h>

/*----------------------------------------------------------------------*/

Prototype LONG   LoadPalette( FRAME *, UBYTE *, EXTBASE * );
Prototype PERROR SavePalette( FRAME *, UBYTE *, EXTBASE * );
Prototype PERROR OpenPaletteWindow( FRAME *frame );
Prototype VOID   ClosePaletteWindow( FRAME *frame );

/*----------------------------------------------------------------------*/

struct Library *BGUIPaletteBase;
Class          *PaletteClass;

/*----------------------------------------------------------------------*/


struct PaletteHeader {
    UBYTE   id[8];
    ULONG   nColors;
};

#define PALETTEHDR_ID "PPTCMAP0"

#define ID_ILBM         MAKE_ID( 'I','L','B','M' )
#define ID_CMAP         MAKE_ID( 'C','M','A','P' )
#define ID_ALFA         MAKE_ID( 'A','L','F','A' )

/*
    The palette window should open on

    1) the rendered image screen
    2) a custom screen
*/
PERROR OpenPaletteWindow( FRAME *frame )
{
    struct Screen *scr = NULL;
    COLORMAP *ct = NULL;
    ULONG depth = 1;

    D(bug("OpenPaletteWindow()\n"));

    if(frame) {
        struct RenderObject *rdo;

        /*
         *  Determine screen on which to open
         */

        if( rdo = frame->renderobject ) {
            if( rdo->GetScreen ) {
                scr = rdo->GetScreen( rdo );
                ct  = frame->disp->colortable;
                depth = frame->disp->depth;
            }
        }

        if( scr == NULL ) {
#if 0
            scr = MAINSCR; /* BUG: Should open a custom screen */
            ct  = globals->maindisp->colortable;
            depth = globals->maindisp->depth;
#else
            Req( NEGNUL, NULL, "\nThere is no rendered image.\n" );
            return PERR_FAILED;
#endif
        }

        if( frame->pw == NULL ) {
            frame->pw = smalloc( sizeof( struct PaletteWindow ));
            /* BUG: should check */
            bzero( frame->pw, sizeof(struct PaletteWindow));
        }

        if( frame->pw->Win == NULL ) {
            if( GimmePaletteWindow( frame, depth ) != PERR_OK ) {
                D(bug("Could not create palette window object!\n"));
                sfree( frame->pw );
                frame->pw = NULL;
                return PERR_WINDOWOPEN;
            } else {
                D(bug("\tAllocated window object\n"));
            }
        }

        if( frame->pw->win == NULL ) {

            SetAttrs( frame->pw->Win,
                      WINDOW_Screen, scr,
                      WINDOW_Bounds, &frame->pw->windowpos,
                      TAG_END );

            frame->pw->scr        = scr;
            frame->pw->colortable = pmalloc( sizeof(COLORMAP) * 256 );
            /* BUG: should check */

            memcpy( frame->pw->colortable, ct, sizeof(COLORMAP)*frame->disp->ncolors );

            if( NULL == (frame->pw->win = WindowOpen( frame->pw->Win ))) {
                D(bug("\tCould not open palette window\n"));
                return PERR_WINDOWOPEN;
            } else {
                D(bug("\tOpened window ok\n"));
            }

        }

        ScreenToFront( frame->pw->scr ); /* Make sure the user can find us. */

    }
    return PERR_OK;
}

VOID ClosePaletteWindow( FRAME *frame )
{
    if(frame) {
        if( frame->pw ) {
            if( frame->pw->Win ) {
                GetAttr( WINDOW_Bounds, frame->pw->Win, (ULONG *)&frame->pw->windowpos );
                DisposeObject( frame->pw->Win );
                frame->pw->win = NULL;
                frame->pw->Win = NULL;
            }

            if( frame->pw->colortable ) {
                pfree( frame->pw->colortable );
            }
        }
    }
}

Prototype VOID DoLoadPalette( FRAME *frame );

VOID DoLoadPalette( FRAME *frame )
{
    UBYTE *path;
    LONG nColors;

    if( DoRequest( gvPaletteOpenReq.Req ) == FRQ_OK ) {
        GetAttr( FRQ_Path, gvPaletteOpenReq.Req, (ULONG*)&path );

        nColors = LoadPalette( frame, path, globxd );

        CloseInfoWindow( frame->mywin, globxd );

        if( nColors > 0 ) {
            if( nColors != frame->disp->ncolors ) {
                if( Req( GetFrameWin(frame), GetStr(mKEEP_DISCARD_COLORS),
                          GetStr(mDIFFERENT_AMOUNT_OF_COLORS) ) != 0 )
                {
                    DoRender( frame->renderobject );
                } else {
                    frame->disp->ncolors     = nColors;
                    frame->disp->depth       = GetMinimumDepth(nColors);
                    frame->disp->saved_cmap  = frame->disp->cmap_method;
                    frame->disp->cmap_method = CMAP_FORCEOLDPALETTE;
                    if( frame->renderobject ) {
                        OpenRender( frame, globxd );
                        DoRender( frame->renderobject );
                    }
                }
            }
        }
    }
}

/*----------------------------------------------------------------*/
/*
 *  Disk handling routines
 */

/*
    Load an IFF conforming palette from the disk.

    Returns number of colors read (0 for error)
 */

LONG LoadPalette( FRAME *frame, UBYTE *name, EXTBASE *PPTBase )
{
    BPTR fh;
    APTR DOSBase = PPTBase->lb_DOS, SysBase = PPTBase->lb_Sys;
    struct Library *IFFParseBase;
    COLORMAP *cmap = frame->disp->colortable;
    struct IFFHandle *iff;
    LONG nColors = 0;

    D(bug("LoadPaletteIFF()\n"));

    IFFParseBase = OpenLibrary( "iffparse.library", 0L );
    if(!IFFParseBase) {
        D(bug("Couldn't open iffparse.library!\n"));
        return PERR_INITFAILED;
    }

    if( fh = Open( name, MODE_OLDFILE ) ) {

        if( iff = AllocIFF() ) {
            iff->iff_Stream = fh;
            InitIFFasDOS( iff );
            if( OpenIFF(iff, IFFF_READ) == 0 ) {
                PropChunk( iff, ID_ILBM, ID_ALFA );
                StopChunk( iff, ID_ILBM, ID_CMAP );
                if( ParseIFF( iff, IFFPARSE_SCAN ) == 0 ) {
                    struct ContextNode *cn;
                    int cc;
                    struct StoredProperty *sp;

                    /*
                     *  The colormap was found, and we'll just load it up.
                     *  Check if the colortable exists and initialize it if it didn't.
                     */

                    if( frame->disp->colortable == NULL ) {
                        if( AllocColorTable( frame ) != PERR_OK ) {
                            nColors = 0;
                            goto errorexit;
                        }
                    }

                    cn = CurrentChunk(iff);

                    nColors = cn->cn_Size / 3;

                    InitProgress( frame, XGetStr(mLOADING_PALETTE), 0, nColors, PPTBase );
                    D(bug("\tReading %d colors...\n",nColors));

                    /* BUG: SHould check if the color amount is consistent */

                    for( cc = 0; cc < nColors; cc++ ) {
                        if( ReadChunkBytes( iff, &(cmap[cc].r), 3 ) < 0 ) {
                            Req( NEGNUL,NULL, "File read error!" );
                            nColors = 0;
                            goto errorexit;
                        }
                    }

                    /*
                     *  Alpha channel
                     */

                    if(sp = FindProp(iff, ID_ILBM, ID_ALFA) ) {
                        for( cc = 0; cc < nColors; cc++ ) {
                            cmap[cc].a = ((UBYTE *)sp->sp_Data)[cc];
                        }
                    }

                    FinishProgress( frame, PPTBase );

errorexit:
                    ;
                } else {
                    D(bug("No CMAP chunk found!\n"));
                    Req( NEGNUL, NULL, XGetStr(mNO_COLORMAP) );
                    nColors = 0;
                }

                CloseIFF( iff );
            } else {
                D(bug("File is not an IFF file!\n"));
                Req(NEGNUL,NULL,XGetStr(mNO_ILBM_FILE) );
                nColors = 0;
            }

            FreeIFF( iff );
        } else {
            D(bug("Couldn't allocate IFF handle\n"));
            nColors = 0;
        }

        Close( fh );
    } else {
        D(bug("Couldn't open palette file!\n"));
        Req(NEGNUL,NULL,XGetStr(MSG_PERR_FILEOPEN) );
        nColors = 0;
    }

    CloseLibrary( IFFParseBase );

    return nColors;
}

/*
    Save an IFF palette onto the disk.
*/

PERROR SavePalette( FRAME *frame, UBYTE *name, EXTBASE *PPTBase )
{
    BPTR fh;
    APTR DOSBase = PPTBase->lb_DOS, SysBase = PPTBase->lb_Sys;
    struct Library *IFFParseBase;
    COLORMAP *cmap = frame->disp->colortable;
    PERROR res = PERR_OK;
    struct IFFHandle *iff;
    LONG nColors;

    D(bug("LoadPaletteIFF()\n"));

    if( !cmap ) return PERR_ERROR;

    IFFParseBase = OpenLibrary( "iffparse.library", 0L );
    if(!IFFParseBase) {
        D(bug("Couldn't open iffparse.library!\n"));
        return PERR_INITFAILED;
    }

    if( fh = Open( name, MODE_NEWFILE ) ) {

        if( iff = AllocIFF() ) {
            iff->iff_Stream = fh;
            InitIFFasDOS( iff );

            if( OpenIFF(iff, IFFF_WRITE) == 0 ) {
                LONG error;

                nColors = frame->disp->ncolors;

                InitProgress( frame, XGetStr(mSAVING_PALETTE), 0, nColors, PPTBase );

                if( (error = PushChunk( iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN )) == 0 ) {

                    if( (error = PushChunk( iff, ID_ILBM, ID_CMAP, IFFSIZE_UNKNOWN )) == 0 ) {
                        int cc;

                        D(bug("\tWriting %d colors...\n",nColors));

                        for(cc = 0; cc < nColors && error >= 0; cc++ ) {
                            error = WriteChunkRecords( iff, &cmap[cc].r, 3, 1 );
                        }

                        PopChunk( iff ); /* CMAP */

                        if( (error = PushChunk(iff, ID_ILBM, ID_ALFA, IFFSIZE_UNKNOWN )) == 0 ) {
                            for( cc = 0; cc < nColors && error >= 0; cc++ ) {
                                error = WriteChunkBytes(iff, &cmap[cc].a, 1 );
                            }

                            PopChunk( iff ); /* ALFA */
                        }

                        if( error < 0 ) {
                            Req( NEGNUL,NULL, "\nError while writing palette:\nIffparse error %d\n", error );
                            res = PERR_FILEWRITE;
                        }

                        FinishProgress( frame, PPTBase );
                    }
                    PopChunk( iff ); /* FORM */
                }

                CloseIFF( iff );
            } else {
                D(bug("Can't OpenIFF()!\n"));
                Req(NEGNUL,NULL,XGetStr(mCANNOT_OPEN_IFF_FILE) );
                res = PERR_FILEREAD;
            }

            FreeIFF( iff );
        } else {
            D(bug("Couldn't allocate IFF handle\n"));
            res = PERR_INITFAILED;
        }

        Close( fh );
        if( res != PERR_OK ) DeleteFile( name );
    } else {
        D(bug("Couldn't open palette file!\n"));
        Req(NEGNUL,NULL,XGetStr(MSG_PERR_FILEOPEN) );
        res = PERR_FILEOPEN;
    }

    CloseLibrary( IFFParseBase );

    return res;

}

