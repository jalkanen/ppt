/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt
    MODULE : support.c

    Support functions.

    $Id: support.c,v 6.2 2000/02/06 19:24:15 jj Exp $
*/
/*----------------------------------------------------------------------*/

/****u* pptsupport/--background-- ******************************************
*
*                 PPT support library documentation
*
*                 $VER: pptsupport.doc 6.0 (02-Oct-99)
*
*   Please note that all functions expect to have a valid PPTBase * in
*   A6 upon entering the function. Otherwise, these are just like
*   any other library functions, except that there are no stack-based
*   versions. If your compiler does not support registerized parameters,
*   tough luck.
*
*
****************************************************************************
*
*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"
#include "render.h"

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#include <proto/locale.h>
#include <proto/timer.h>

/*----------------------------------------------------------------------*/
/* Defines */

#undef  L
#define L(x)            /* Remove lock-messages */

#define USE_TIMER_PROCESS

/*----------------------------------------------------------------------*/
/* Internal prototypes */


Prototype VOID OpenExtCatalog( EXTERNAL *, STRPTR, LONG, EXTBASE * );
Prototype VOID CloseExtCatalog( EXTERNAL *, EXTBASE * );

/*----------------------------------------------------------------------*/
/* Global variables */

/// ExtLibData[]
APTR ExtLibData[] = {
    NULL, /* LIB_OPEN */
    NULL, /* LIB_CLOSE */
    NULL, /* LIB_EXPUNGE */
    NULL, /* LIB_RESERVED */
    NewFrame,
    MakeFrame,
    InitFrame,
    RemFrame,
    DupFrame,

    FindFrame, /* 10 */

    GetPixel,
    PutPixel,
    GetPixelRow,
    PutPixelRow,
    GetNPixelRows,
    PutNPixelRows,
    GetBitMapRow,

    UpdateProgress,
    InitProgress,
    Progress,          /* 20 */
    FinishProgress,
    ClearProgress,

    SetErrorCode,
    SetErrorMsg,

    AskReqA,

    PlanarToChunky,
    NULL, /* BUG: ChunkyToPlanar */

    GetStr_External,
    TagData,

    StartInput,        /* 30 */
    StopInput,

    GetBackgroundColor,

    GetOptions,
    PutOptions,

    AddExtension,
    FindExtension,
    RemoveExtension,

    /* Start of V4 additions */

    ObtainPreviewFrameA,
    ReleasePreviewFrame,
    RenderFrame,       /* 40 */
    CopyFrameData,

    /* Start of V5 additions */

    CloseProgress,
    SetRexxVariable,

    /* Start of V6 additions */

    SPrintFA,

    (APTR) ~0 /* Marks the end of the table for MakeFunctions() */
};
///

/*----------------------------------------------------------------------*/
/* Code */

/// GetStr() and support funcs

/****i* pptsupport/GetStr *******************************************
*
*   NAME
*       GetStr - Get a localized message string
*
*   SYNOPSIS
*       string = GetStr( code )
*
*       STRPTR GetStr( struct LocaleString * )
*       D0             A0
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*       string - Pointer to the localized string or the default string
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       This entry is incomplete
*
*   SEE ALSO
*       locale.library/GetLocaleStr()
*
***********************************************************************
*
*   This is just like the local XGetStr(), but uses a different catalog
*
*/

Prototype STRPTR ASM GetStr_External( REGDECL(a0,struct LocaleString *), REGDECL(a6,EXTBASE *) );

SAVEDS STRPTR ASM GetStr_External(REGPARAM(a0,struct LocaleString *,ls),
                                  REGPARAM(a6,EXTBASE *,PPTBase) )
{
    STRPTR defaultstr;
    LONG strnum;
    struct Library *LocaleBase = PPTBase->lb_Locale;

    strnum = ls->ID;
    defaultstr = ls->Str;

    if( LocaleBase ) {
        return( (PPTBase->extcatalog) ?
               GetCatalogStr(PPTBase->extcatalog, strnum, defaultstr) :
               defaultstr);
    } else {
        return defaultstr;
    }
}

/*
    Opens up the catalog file for the external.
    BUG: Should this protect agains several openings?
*/

VOID OpenExtCatalog( EXTERNAL *ext, STRPTR builtin,
                     LONG version, EXTBASE *PPTBase )
{
    struct Library *LocaleBase = PPTBase->lb_Locale;
    struct TagItem tags[] = {
        OC_BuiltInLanguage, 0,
        OC_Version,         0,
        TAG_DONE
    };
    UBYTE buf[80];

    tags[0].ti_Data = (ULONG)builtin;
    tags[1].ti_Data = version;

    CloseExtCatalog( ext, PPTBase ); /* Does no harm */

    if( LocaleBase && PPTBase->extcatalog == NULL ) {
        sprintf( buf, "modules/%s.catalog", ext->nd.ln_Name );
        PPTBase->extcatalog = OpenCatalogA( NULL, (STRPTR) buf, tags );
    }

}

/*
    Close the external catalog.
*/

VOID CloseExtCatalog( EXTERNAL *ext, EXTBASE *PPTBase )
{
    struct Library *LocaleBase = PPTBase->lb_Locale;

    if( LocaleBase && PPTBase->extcatalog ) {
        CloseCatalog( PPTBase->extcatalog );
        PPTBase->extcatalog = NULL;
    }
}
///

/// PlanarToChunky()
/****u* pptsupport/PlanarToChunky ******************************************
*
*   NAME
*       PlanarToChunky -- Convert planar mode graphics to chunky
*
*   SYNOPSIS
*       PlanarToChunky( source, dest, width, depth )
*                       A0      A1    D0     D1
*
*       VOID PlanarToChunky( UBYTE **, ROWPTR, ULONG, UWORD );
*
*   FUNCTION
*       This is an optimized function for converting planar mode
*       graphics into chunky mode graphics. Useful if the data in
*       a file is in bitmapped format because you need to feed the
*       data into PPT in chunky pixel format.
*
*   INPUTS
*       source - A pointer to an array containing pointers to wherever
*           your planar data is.
*       dest - The chunky buffer where you wish the result to be put
*       width - Width of the row.
*       depth - Number of bitplanes and thus the number of entries in
*           the source array.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       Supports only 1-8, 16, 24 and 32 planes at the moment. Do not
*       attempt to try to convert other types.
*       Could really be faster
*
*   SEE ALSO
*       GetPixelRow()
*
******************************************************************************
*
*   Otherwise this is relatively straightforward, except only
*   if we have more than 8 planes. Then we make several passes over
*   the data, with the start location moved each time.
*/

Prototype VOID ASM PlanarToChunky( REGDECL(a0,UBYTE **), REGDECL(a1,ROWPTR), REGDECL(d0,ULONG),REGDECL(d1,UWORD),REGDECL(a6,EXTBASE *) );

VOID SAVEDS ASM PlanarToChunky( REGPARAM(a0,UBYTE **,source), REGPARAM(a1,ROWPTR,dest),
                                REGPARAM(d0,ULONG,width),REGPARAM(d1,UWORD,depth),
                                REGPARAM(a6,EXTBASE *,PPTBase) )
{
    UWORD numpasses = ((depth-1)>>3)+1;
    UWORD i;
    UBYTE *temp[32], **cursrc = temp;
    ROWPTR curdest = dest;

    //D(bug("PlanarToChunky( source=%08X,dest=%08X,width=%lu,depth=%u\n",source,dest,
    //       width,depth ));

    // Delay(25);

    memcpy( temp, source, depth * sizeof(UBYTE *) );

    for(i = 0; i < numpasses; i++ ) {
        // D(bug("\tPass %d\n",i));
        Plane2Chunk( cursrc, curdest, width, depth );
        cursrc  += 8;
        curdest += 1;
    }
    // D(bug("...done!\n"));
}
///

/// TagData()
/****u* pptsupport/TagData ******************************************
*
*   NAME
*       TagData -- Get Tag Data.
*
*   SYNOPSIS
*       data = TagData( value, list )
*       D0              D0     A0
*
*       ULONG TagData( Tag tag, struct TagList *list );
*
*   FUNCTION
*       This is a shortened version of utility.library's GetTagData().
*       Use this to read tags from the taglists passed to your external.
*
*   INPUTS
*       value - Tag value you seek.
*       list - A tag list terminated with TAG_DONE.
*
*   RESULT
*       data - Data associated with the tag. 0, if not found
*           for some reason.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       utility.library/GetTagData().
*
*****************************************************************************
*
*/

Prototype ULONG ASM TagData( REGDECL(d0,Tag), REGDECL(a0,struct TagItem *), REGDECL(a6,EXTBASE *) );

ULONG SAVEDS ASM TagData( REGPARAM(d0,Tag,value),
                          REGPARAM(a0,struct TagItem *,list),
                          REGPARAM(a6,EXTBASE *,PPTBase) )
{
    APTR UtilityBase = PPTBase->lb_Utility;

    return(GetTagData(value,0L,list));
}
///
/// SPrintF()
/****u* pptsupport/SPrintF ******************************************
*
*   NAME
*       SPrintF -- Print characters into a buffer. (V6)
*
*   SYNOPSIS
*       n = SPrintF( buffer, format, ... )
*       D0           A0      A1
*
*       n = SPrintFA( buffer, format, args )
*       D0            A0      A1      A2
*
*       ULONG SPrintF( STRPTR, STRPTR, ... );
*
*       ULONG SPrintFA( STRPTR, STRPTR, APTR );
*
*   FUNCTION
*       Works like sprintf().  This uses ANSI functions, NOT
*       exec.library/RawDoFmt().  Floating point is also supported.
*
*   INPUTS
*       buffer - buffer to which characters are written.  Make sure
*           it is big enough.
*
*       format - string of formatting characters.
*
*       args - arguments for the format string.
*
*   RESULT
*       n - number of characters written.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       sprintf()
*
*****************************************************************************
*
*/

Prototype ULONG ASM SPrintFA( REGDECL(a0,STRPTR), REGDECL(a1,STRPTR), REGDECL(a2,APTR), REGDECL(a6,EXTBASE *) );

ULONG SAVEDS ASM SPrintFA( REGPARAM(a0,STRPTR,buffer),
                           REGPARAM(a1,STRPTR,format),
                           REGPARAM(a2,APTR,args),
                           REGPARAM(a6,EXTBASE *,PPTBase) )
{
    ULONG res;

    D(bug("SprintFA( fmt='%s', args=0x%X )\n",format, args));
    res = vsprintf( buffer, format, args );
    D(bug("  buffer = '%s'\n",buffer));

    return res;
}
///

/// GetBackgroundColor()
/****u* pptsupport/GetBackgroundColor ******************************************
*
*   NAME
*       GetBackgroundColor
*
*   SYNOPSIS
*       success = GetBackgroundColor( frame, pixel )
*       D0                            A0     A1
*
*       PERROR GetBackgroundColor( FRAME *, ROWPTR );
*
*   FUNCTION
*       Returns the background color of the given frame.  If no background
*       color has been designated, then will calculate a good guess
*       using an average of each of the corners.
*
*   INPUTS
*       frame - the frame handle
*       pixel - pointer to a location where the pixel should be
*           written.  Make sure you have enough space for one pixel!
*
*   RESULT
*       success - PPT error code.  PERR_OK, if everything went OK.
*
*   EXAMPLE
*
*   NOTES
*       The reason this is a function instead of a field in the PIXINFO
*       structure is that if needed, the background color can be
*       calculated on the fly.
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*   BUG: Does not have any background selector.
*/

Prototype PERROR ASM GetBackgroundColor( REGDECL(a0,FRAME *), REGDECL(a1,ROWPTR), REGDECL(a6,EXTBASE *) );

PERROR SAVEDS ASM
GetBackgroundColor( REGPARAM(a0,FRAME *,frame), REGPARAM(a1,ROWPTR,pixel),
                    REGPARAM(a6,EXTBASE *,PPTBase) )
{
    PERROR res = PERR_OK;
    RGBPixel *rgb, prgb[4];
    ARGBPixel *argb, pargb[4];
    GrayPixel *gray, pgray[4];
    WORD width = frame->pix->width;

    /*
     *  A temporary frame gets the background color as black, otherwise
     *  calculates one from the image.
     */

    if( frame->istemporary ) {
        switch( frame->pix->colorspace ) {
            case CS_RGB:
                ((RGBPixel *)pixel)->r = ((RGBPixel *)pixel)->g = ((RGBPixel *)pixel)->b = 0;
                break;

            case CS_ARGB:
                ((ARGBPixel *)pixel)->r = ((ARGBPixel *)pixel)->g = ((ARGBPixel *)pixel)->b = 0;
                ((ARGBPixel *)pixel)->a = 0;
                break;

            case CS_GRAYLEVEL:
                ((GrayPixel *)pixel)->g = 0;
                break;
        }
    } else {
        switch( frame->pix->colorspace ) {
            case CS_RGB:
                rgb = (RGBPixel *)GetPixelRow( frame, 0, PPTBase );
                prgb[0] = rgb[0]; prgb[1] = rgb[width-1];
                rgb = (RGBPixel *)GetPixelRow( frame, frame->pix->height-1, PPTBase );
                prgb[2] = rgb[0]; prgb[3] = rgb[width-1];
                ((RGBPixel *)pixel)->r = (((ULONG)prgb[0].r + prgb[1].r + prgb[2].r + prgb[3].r)/4);
                ((RGBPixel *)pixel)->g = (((ULONG)prgb[0].g + prgb[1].g + prgb[2].g + prgb[3].g)/4);
                ((RGBPixel *)pixel)->b = (((ULONG)prgb[0].b + prgb[1].b + prgb[2].b + prgb[3].b)/4);
                break;

            case CS_ARGB:
                argb = (ARGBPixel *)GetPixelRow( frame, 0, PPTBase );
                pargb[0] = argb[0]; pargb[1] = argb[width-1];
                argb = (ARGBPixel *)GetPixelRow( frame, frame->pix->height-1, PPTBase );
                pargb[2] = argb[0]; pargb[3] = argb[width-1];
                ((ARGBPixel *)pixel)->r = (((ULONG)pargb[0].r + pargb[1].r + pargb[2].r + pargb[3].r)/4);
                ((ARGBPixel *)pixel)->g = (((ULONG)pargb[0].g + pargb[1].g + pargb[2].g + pargb[3].g)/4);
                ((ARGBPixel *)pixel)->b = (((ULONG)pargb[0].b + pargb[1].b + pargb[2].b + pargb[3].b)/4);
                break;

            case CS_GRAYLEVEL:
                gray = (GrayPixel *)GetPixelRow( frame, 0, PPTBase );
                pgray[0] = gray[0]; pgray[1] = gray[width-1];
                gray = (GrayPixel *)GetPixelRow( frame, frame->pix->height-1, PPTBase );
                pgray[2] = gray[0]; pgray[3] = gray[width-1];
                ((GrayPixel *)pixel)->g = (((ULONG)pgray[0].g + pgray[1].g + pgray[2].g + pgray[3].g)/4);
                break;
        }
    }

    return res;
}
///

/// GetPixelRow()
/****u* pptsupport/GetPixelRow ******************************************
*
*   NAME
*       GetPixelRow -- get a pointer to chunky data
*
*   SYNOPSIS
*       ptr = GetPixelRow( frame, row )
*       D0                 A0     D0
*
*       ROWPTR GetPixelRow( FRAME *, WORD );
*
*   FUNCTION
*       Return a pointer to a row of data in either 24 bit chunky format
*       or 8 bit chunky graylevel data. In the former case, the data is
*       in RGB - order. You may check the amount of components in
*       frame->pix->components at any time.
*
*   INPUTS
*       frame - the frame handle
*       row - the pixel row you need info from
*
*   RESULT
*       ptr - A pointer to bitmapped data or NULL, if something failed.
*
*   EXAMPLE
*
*   NOTES
*       DO NOT reference a row which you have not GetPixelRow()ed first.
*
*   BUGS
*
*   SEE ALSO
*       PutPixelRow(), GetBitMapRow()
*
*****************************************************************************
*
*   NB: if the index is out of bounds, just return null, because externals
*   are counting on it.  Do not use SetError*()!
*/

Prototype ROWPTR ASM GetPixelRow( REGDECL(a0,FRAME *), REGDECL(d0,WORD), REGDECL(a6,EXTBASE *) );

ROWPTR SAVEDS ASM GetPixelRow( REGPARAM(a0,FRAME *,f),
                               REGPARAM(d0,WORD,row),
                               REGPARAM(a6,EXTBASE *,xd) )
{
    ULONG offset;
    PIXINFO *p;
    VMHANDLE *vmh;
    ROWPTR source;

    // D(bug("GetPixelRow(%08X,%d)\n",f,row));

    LOCK(f);
    p = f->pix;
    vmh = p->vmh;

    if(!p || !vmh) { UNLOCK(f); return NULL; }

    if(row < 0 || row >= p->height) {
        UNLOCK(f);
        return NULL;
    }

    offset = row * ROWLEN(p); /* Beginning of line */

    /* If the beginning is before the area currently in memory OR
       the end of the pixelline is outside the area as well, then
       load to memory, unless there is no vm_fh. */

    /*
     *  if (first byte is not in memory ||
     *      last byte is not in memory) reload();
     */

    if( vmh->vm_fh && ((offset < vmh->begin) || (offset + ROWLEN(p) > vmh->end)) )
    {
        // D(bug("\tLoading new data from %lu\n",offset));
        FlushVMData( vmh, xd ); /* Will save only if there have been changes. */
        LoadVMData( vmh, offset, xd );
    }
    else {
        // D(bug("\tData found completely in memory @ offset %lu\n",offset - vmh->begin));
    }

    source = (ROWPTR)( (ULONG)vmh->data + (offset - vmh->begin) );

#ifdef TMPBUF_SUPPORTED
    /*
     *  Copy the data into the temporary buffer
     */

    memcpy( p->tmpbuf, source, p->bytes_per_row );

    UNLOCK(f);
    return( p->tmpbuf );
#else
    UNLOCK(f);
    return source;
#endif
}
///
/// PutPixelRow()
/****u* pptsupport/PutPixelRow ******************************************
*
*   NAME
*       PutPixelRow -- put the pixel data back into buffer.
*
*   SYNOPSIS
*       PutPixelRow( frame, row, data )
*                    A0     D0   A1
*
*       VOID PutPixelRow( FRAME *, WORD, ROWPTR );
*
*   FUNCTION
*       Put back the data obtained by GetPixelRow().
*
*   INPUTS
*       frame - the frame handle
*       row - the pixel row you read the data from.
*       data - pointer to the buffer returned by GetPixelRow().
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       You MUST call this routine if you wish your changes to be visible.
*
*       Do not use this as a copy routine, in reality no data is copied
*       to make this routine faster. If you have a need to copy stuff from
*       one frame to another, use CopyMem(). In the future, the copying
*       may be possible.
*
*   BUGS
*
*   SEE ALSO
*       GetPixelRow(), GetBitMapRow()
*
*****************************************************************************
*
*
*    In the future, this routine will take lots of other things into account.
*   BUG: Do we really need arg: data?
*
*/

Prototype VOID ASM PutPixelRow( REGDECL(a0,FRAME *), REGDECL(d0,WORD), REGDECL(a1,ROWPTR), REGDECL(a6,EXTBASE *) );

VOID SAVEDS ASM PutPixelRow( REGPARAM(a0,FRAME *,frame),
                             REGPARAM(d0,WORD,row),
                             REGPARAM(a1,ROWPTR,data),
                             REGPARAM(a6,struct PPTBase *,PPTBase) )
{
    ROWPTR dest;
    PIXINFO *p = frame->pix;
    VMHANDLE *vmh = p->vmh;
    ULONG offset;
    WORD col;

#ifdef TMPBUF_SUPPORTED
    if( !data ) return;

    offset = row * ROWLEN(p); /* Beginning of line */

    if( (offset < vmh->begin) || (offset + ROWLEN(p) >= vmh->end) )
    {
        // D(bug("\tLoading new data from %lu\n",offset));
        FlushVMData( vmh, xd ); /* Will save only if there have been changes. */
        LoadVMData( vmh, offset, xd );
    }
    else {
        // D(bug("\tData found completely in memory @ offset %lu\n",offset - vmh->begin));
    }

    dest = (ROWPTR)( (ULONG)vmh->data + (offset - vmh->begin) );

    memcpy( dest, data, ROWLEN(p) );
    // D(bug("\tmemcpy( dest = %08X, data = %08X, length = %lu )\n",dest,data,ROWLEN(p) ));
    vmh->chflag = 1;
#else
#ifdef USE_OLD_ALPHA
    /*
     *  This is the future alpha channel version.
     *  BUG: Should check if parent is the same size than the new one.
     */

    if( frame->alpha && frame->parent ) {
        FRAME *alpha;

        alpha = FindFrame( frame->alpha ); /* BUG: Should cache this */
        if(!alpha) {
            D(bug("Invalid alpha channel %ld!\n",frame->alpha ));
        } else {
            ROWPTR acp, scp;
            WORD col, acol, endx;
            UBYTE a;

            acp = GetPixelRow( alpha, row % alpha->pix->height, xd );
            scp = GetPixelRow( frame->parent, row % frame->parent->pix->height, xd );

            acol = 0;
            endx = frame->pix->width * 3;

            a = *acp;

            for( col = 0; col < endx; col++ ) {
                /*
                 *  BUG: This assumes alpha is in GREY8 format
                 *  BUG: This assumes original is in RGB8 format
                 *  x = alpha * original + (1-alpha) * new;
                 */

                data[col]   = (UBYTE)( (MULU16(a,scp[col]) + MULU16(255-a,data[col]))/255);

                if( (col & 0x03) == 0x03) {
                    if( ++acol > alpha->pix->width ) acol = 0; /* Wrap */
                    a = acp[acol];
                }
            }
        }

        vmh->chflag = 1;

    } else {
        vmh->chflag = 1;
    }
#else
    vmh->chflag = 1;

    if( frame->parent ) {
        if( p->colorspace != frame->parent->pix->colorspace ||
            p->height != frame->parent->pix->height ||
            p->width  != frame->parent->pix->width )
        {
            /* Skip this, since size, or something else has changed */
        } else {
            ROWPTR pcp;
            int components = p->components;

            pcp = GetPixelRow( frame->parent, row, PPTBase );

            for( col = 0; col < p->width; col++ ) {
                if( !IsInArea( frame, row, col ) ) {
                    int color;

                    for( color = 0; color < components; color++ ) {
                        data[color] = pcp[color];
                    }
                }
                pcp  += components;
                data += components;
            }
        } /* sizechange */
    } /* frame->parent */
#endif
#endif

#if 0
    if( IsPreview(frame) ) {
        RedrawPreviewRow( frame, row, xd );
    }
#endif
}
///

/// PutNPixelRows()
/****u* pptsupport/PutNPixelRows ******************************************
*
*   NAME
*       PutNPixelRows -- Write several modified rows.
*
*   SYNOPSIS
*       PutNPixelRows( frame, buffer, startrow, nrows )
*                      A0     A1      D0        D1
*
*       VOID PutNPixelRows( FRAME *, ROWPTR *, WORD, UWORD );
*
*   FUNCTION
*       After doing a GetNPixelRows, you may use this routine to put
*       the rows back into the frame. You MUST do this if you make
*       any modifications.
*
*   INPUTS
*       frame - as usual.
*       buffer - A pointer to an array which is filled with pointers
*           to standard PPT pixel rows. See GetNPixels() autodoc for
*           more info.
*       startrow - where the rows where originally fetched from.
*       nrows - how many rows in total.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       This entry is still very incomplete.
*
*   SEE ALSO
*       GetNPixelRows(), PutPixel(), PutPixelRow().
*
******************************************************************************
*
*/

Prototype VOID ASM PutNPixelRows( REGDECL(a0,FRAME *), REGDECL(a1,ROWPTR []), REGDECL(d0,WORD), REGDECL(d1,UWORD), REGDECL(a6,EXTBASE *) );

VOID SAVEDS ASM PutNPixelRows( REGPARAM(a0,FRAME *,frame), REGPARAM(a1,ROWPTR,buffer[]),
                               REGPARAM(d0,WORD,startrow), REGPARAM(d1,UWORD,nRows),
                               REGPARAM(a6,EXTBASE *,PPTBase) )
{
    frame->pix->vmh->chflag = 1;
}
///
/// GetNPixelRows()
/****u* pptsupport/GetNPixelRows ******************************************
*
*   NAME
*       GetNPixelRows -- Get several rows at once.
*
*   SYNOPSIS
*       rows = GetNPixelRows( frame, buffer, startrow, nrows )
*       D0                    A0     A1      D0        D1
*
*       UWORD GetNPixelRows( FRAME *, ROWPTR *, WORD, UWORD );
*
*   FUNCTION
*
*   INPUTS
*       frame - as usual.
*       buffer - pointer to an array with nrows elements. PPT loads the
*           pointers to the image rows into this array, so that the pointer
*           to startrow goes into buffer[0], pointer to startrow+1 goes
*           to buffer[1], etc. If any of the rows are unfetchable for any
*           reason, the element will contain NULL. Remember to check for it!
*       startrow - the row you wish to start the lines for.
*       nrows - how many rows do you wish to fetch.
*
*   RESULT
*       rows - number of rows successfully loaded into buffer.
*           If zero, an error occurred or you were way out of
*           any reasonable limits.
*
*   EXAMPLE
*
*   NOTES
*       You may attempt to get negative rows, but then both the array and
*       the return value will reflect this. For example:
*           foo = GetNPixelRows(frame,buffer,-5,10);
*
*       In this case, foo will be 5 and buffer[0..4] will be NULL and
*       buffer[5..9] will be valid pointers.
*
*   BUGS
*
*   SEE ALSO
*       PutNPixelRows(), GetPixelRow(), GetPixel().
*
******************************************************************************
*
*   BUG: Should allocate extra buffer room in case the rows do not fit
*   into memory.
*   BUG: Should maybe return -1 on error and reserve zero for real stuff
*/

Prototype UWORD ASM GetNPixelRows( REGDECL(a0,FRAME *), REGDECL(a1,ROWPTR []), REGDECL(d0,WORD), REGDECL(d1,UWORD), REGDECL(a6,EXTBASE *) );

UWORD SAVEDS ASM GetNPixelRows( REGPARAM(a0,FRAME *,frame),  REGPARAM(a1,ROWPTR,buffer[]),
                                REGPARAM(d0,WORD,startrow), REGPARAM(d1,UWORD,nrows),
                                REGPARAM(a6,EXTBASE *,xd) )
{
    ULONG linelen = ROWLEN( frame->pix );
    ULONG offset, chunklen;
    VMHANDLE *vmh = frame->pix->vmh;
    int i,j;

    /*
     *  Check if all the rows can be put in the buffer at the same time
     */

    if( (chunklen = nrows * linelen) > (vmh->end - vmh->begin) ) {
        /*
         *  Allocate extra room that can be discarded on exit
         *  BUG: This version does not do this, however
         */
        return 0; /* Error */
    }

    if( startrow >= 0 )
        offset = startrow * linelen; /* Beginning of line */
    else
        offset = 0;

    /*
     *  if (first byte is not in memory ||
     *      last byte is not in memory) reload();
     */

    if( vmh->vm_fh && ((offset < vmh->begin) ||
                      (offset + chunklen >= vmh->end)) )
    {
        // D(bug("\tLoading new data from %lu\n",offset));
        FlushVMData( vmh, xd ); /* Will save only if there have been changes. */
        LoadVMData( vmh, offset, xd );
    }
    else {
        // D(bug("\tData found completely in memory @ offset %lu\n",offset - vmh->begin));
    }

    /*
     *  Flush data into memory
     */

    for( i = 0, j = 0; i < nrows; i++ ) {
        if(startrow + i < 0 || startrow + i >= frame->pix->height)
            buffer[i] = NULL;
        else {
            buffer[i] = (ROWPTR) ((ULONG)vmh->data + (offset - vmh->begin) + (linelen * j) );
            j++; /* Real line counter */
        }
    }

    return (UWORD)j;
}
///

/// GetPixel()

/****i* pptsupport/GetPixel ******************************************
*
*   NAME
*       GetPixel -- Get data for one pixel only.
*
*   SYNOPSIS
*       ptr = GetPixel( frame, row, column );
*       D0              A0     D0   D1
*
*   FUNCTION
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
*       This entry still very incomplete.
*
*   SEE ALSO
*       PutPixel(), GetPixelRow().
*
******************************************************************************
*
*    Give a pixel at wanted location. (0,0) is at upper left hand corner.
*    BUG: Does not take different size pixels into account.
*/

Prototype APTR ASM GetPixel( REGDECL(a0,FRAME *), REGDECL(d0,WORD), REGDECL(d1,WORD), REGDECL(a6,EXTBASE *) );

APTR SAVEDS ASM GetPixel( REGPARAM(a0,FRAME *,f), REGPARAM(d0,WORD,row),
                          REGPARAM(d1,WORD,column), REGPARAM(a6,EXTBASE *,xd) )
{
    ROWPTR cp;

    if(cp = GetPixelRow( f, row, xd )) {
        return cp + column*f->pix->components;
    }
    return NULL;
}
///
/// PutPixel()
/****i* pptsupport/PutPixel ******************************************
*
*   NAME
*       PutPixel -- Put one modified pixel only.
*
*   SYNOPSIS
*       PutPixel( frame, row, column, item );
*                 A0     D0   D1      A1
*
*   FUNCTION
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
*       This entry still very incomplete.
*
*   SEE ALSO
*       GetPixel(), PutPixelRow().
*
******************************************************************************
*
*   Note: data is copied.
*/

Prototype VOID ASM PutPixel( REGDECL(a0,FRAME *), REGDECL(d0,WORD), REGDECL(d1,WORD), REGDECL(a1,APTR), REGDECL(a6,EXTBASE *) );

VOID SAVEDS ASM PutPixel( REGPARAM(a0,FRAME *,f),
                          REGPARAM(d0,WORD,row), REGPARAM(d1,WORD,column),
                          REGPARAM(a1,APTR,data),
                          REGPARAM(a6,EXTBASE *,xd) )
{
    ROWPTR cp;

    if( IsInArea( f, row, column ) ) {
        if(cp = GetPixelRow( f, row, xd )) {
            cp += column*f->pix->components;
            memmove( cp, data, f->pix->components );
            f->pix->vmh->chflag = 1;
            // PutPixelRow( f, row, cp, xd );
        }
    }
}
///

/// GetBitMapRow()
/****i* pptsupport/GetBitMapRow ******************************************
*
*   NAME
*       GetBitMapRow -- get a pointer to a rendered image
*
*   SYNOPSIS
*       ptr = GetBitMapRow( frame, row );
*       D0                  A0     D0
*
*       UBYTE *GetBitMapRow( FRAME *, WORD );
*
*   FUNCTION
*
*   INPUTS
*       frame - easy.
*       row - The row you wish to have data from
*
*   RESULT
*       ptr - A pointer to bitmapped data or NULL, if something failed.
*           Don't forget to check it!
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       This routine is currently under development, do not assume the
*       interface would stay the same.
*
*   SEE ALSO
*       GetPixelRow().
*
*****************************************************************************
*
*   Calls up the renderobject->GetRow().
*   BUG: Should do something about the rdo->ExtBase?
*
*/

Prototype UBYTE * ASM GetBitMapRow( REGDECL(a0,FRAME *), REGDECL(d0,WORD), REGDECL(a6,EXTBASE *) );

UBYTE * SAVEDS ASM GetBitMapRow( REGPARAM(a0,FRAME *,frame), REGPARAM(d0,WORD,row),
                                 REGPARAM(a6,EXTBASE *,PPTBase) )
{
    struct RenderObject *rdo = frame->renderobject;

    if(rdo == NULL) return NULL;

    if( rdo->GetRow ) {
        return (*rdo->GetRow)( rdo, row );
    } else {
        return NULL;
    }
}
///

/// ClearProgress()
/****u* pptsupport/ClearProgress ******************************************
*
*   NAME
*       ClearProgress -- Clear progress display.
*
*   SYNOPSIS
*       ClearProgress( frame )
*                      A0
*
*       VOID ClearProgress( FRAME * );
*
*   FUNCTION
*       Clears the progress display, resets variables and renders the
*       'idle' - text on the status display.
*
*   INPUTS
*       frame - as usual.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       There should not be any real reason for you to call this routine.
*       So, keep away from it.
*
*   BUGS
*
*   SEE ALSO
*       InitProgress(), Progress().
*
******************************************************************************
*
*    A short-hand to UpdateProgress( f, NULL, 0, xd )
*/

Prototype VOID ASM ClearProgress( REGDECL(a0,FRAME *), REGDECL(a6,EXTBASE *) );

VOID SAVEDS ASM ClearProgress( REGPARAM(a0,FRAME *,f), REGPARAM(a6,EXTBASE *,PPTBase) )
{
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    D(bug("ClearProgress()\n"));

    UpdateProgress( f, NULL, 0, PPTBase );

    LOCK(f);
    f->progress_min = 0L;
    f->progress_diff = 1; /* Just in case */
    UNLOCK(f);
}
///
/// InitProgress()
/****u* pptsupport/InitProgress ******************************************
*
*   NAME
*       InitProgress -- Initialize progress display
*
*   SYNOPSIS
*       InitProgress( frame, txt, min, max )
*                     A0     A1   D0   D1
*
*       VOID InitProgress( FRAME *, UBYTE *, ULONG, ULONG );
*
*   FUNCTION
*       Initialize progress display to use with Progress().
*
*   INPUTS
*       frame - as usual.
*       txt - a string to be displayed to the user while your algorithm
*           is running.
*       min - the minimum value of your counter
*       max - the maximum value of your counter
*
*       The counter is the value you pass to the Progress() - function.
*
*   RESULT
*
*   EXAMPLE
*       A normal loop in which you can process the data might look as
*       follows (frame is the FRAME * sent to you by ppt and tags is
*       the TagItem array:
*
*       ...
*
*       InitProgress( frame, "Mongering...",
*                     frame->selbox.MinY, frame->selbox.MaxY );
*       for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row ++ ) {
*           ROWPTR cp;
*           cp = GetPixelRow( frame, row );
*           MongerARow( cp );
*           if(Progress( frame, row )) {
*               break;
*           }
*       }
*       FinishProgress( frame );
*       ...
*
*   NOTES
*       Use always before Progress() and do not forget to call
*       FinishProgress()!
*
*   BUGS
*
*   SEE ALSO
*       Progress(), ClearProgress(), FinishProgress().
*
******************************************************************************
*   Makes sure the infowindow is open.
*   If a parent exists, we'll use its infowindow instead of our own.
*/

Prototype VOID ASM InitProgress( REGDECL(a0,FRAME *), REGDECL(a1,char *), REGDECL(d0,ULONG), REGDECL(d1,ULONG),REGDECL(a6,EXTBASE *) );

SAVEDS VOID ASM InitProgress( REGPARAM(a0,FRAME *,f),
                              REGPARAM(a1,char *,txt),
                              REGPARAM(d0,ULONG, min),
                              REGPARAM(d1,ULONG, max),
                              REGPARAM(a6,EXTBASE *,PPTBase) )
{
    struct ExecBase *SysBase = PPTBase->lb_Sys;
#ifdef USE_TIMER_PROCESS
    struct Device *TimerBase = PPTBase->lb_Timer;
#endif

    D(bug("InitProgress('%s',%lu,%lu)\n",txt,min,max));

    if(!CheckPtr(f,"InitProgress: frame")) return;

    if( IsPreview(f) ) return;

    LOCK(f);
    f->progress_min = min;
    f->progress_diff= (max-min);
    f->progress_old = 0L;
#ifdef USE_TIMER_PROCESS
    ReadEClock( &(f->eclock) );
#endif
    UNLOCK(f);

    UpdateProgress( f, txt, 0, PPTBase);

    if( f->parent )
        OpenInfoWindow( f->parent->mywin, PPTBase );
    else
        OpenInfoWindow( f->mywin, PPTBase );
}
///
/// Progress()
/****u* pptsupport/Progress ******************************************
*
*   NAME
*       Progress -- update Progress display.
*
*   SYNOPSIS
*       break = Progress( frame, done )
*       D0                A0     D0
*
*       BOOL Progress( FRAME *, ULONG );
*
*   FUNCTION
*       Updates progress display.
*
*   INPUTS
*       frame - As usual.
*       done - your progress indicator. PPT will transform this into
*           a display bar by heeding the values you stated in the
*           InitProgress() call. The value must be between those
*           values.
*
*   RESULT
*       break - TRUE, if the user wants you to quit. You should exit as
*           soon as possible.
*
*   EXAMPLE
*       See autodoc for InitProgress().
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       InitProgress(), FinishProgress(), ClearProgress().
*
******************************************************************************
*
*    This will someday replace UpdateProgress() completely. Until that, it just
*    calls UpdateProgress().
*
*    Returns TRUE, if a BREAK signal has been encountered, FALSE otherwise.
*/

Prototype BOOL ASM Progress( REGDECL(a0,FRAME *), REGDECL(d0,ULONG), REGDECL(a6,EXTBASE *) );

BOOL SAVEDS ASM Progress( REGPARAM(a0,FRAME *,f),
                          REGPARAM(d0,ULONG,done),
                          REGPARAM(a6,EXTBASE *,PPTBase) )
{
    APTR DOSBase = PPTBase->lb_DOS;
    APTR SysBase = PPTBase->lb_Sys;
    struct Device *TimerBase = PPTBase->lb_Timer;
    struct PPTMessage *pmsg;
#ifdef USE_TIMER_PROCESS
    struct EClockVal  ev, evold;
    ULONG  etick;
#endif

#ifndef USE_TIMER_PROCESS
    if( done - f->progress_old > f->progress_diff/globals->userprefs->progress_step ) {
        UpdateProgress( f, NEGNUL, MAXPROGRESS * (done - f->progress_min) / f->progress_diff, PPTBase );
        f->progress_old = done;
        // WaitForReply( PPTMSG_UPDATEPROGRESS, PPTBase );
    }
#else
    etick = ReadEClock(&ev);
    SHLOCK(f);
    evold = f->eclock;
    UNLOCK(f);

    /*
     *  Theoretically, we don't need frame locking, because only this
     *  routine can change the values.  And nobody is going to kick the bucket
     *  from under us.
     */

    if( (ev.ev_lo - evold.ev_lo) > etick/2 || (ev.ev_hi != evold.ev_hi) ) {
        UpdateProgress( f, NEGNUL,
                        MAXPROGRESS * (done - f->progress_min) / f->progress_diff,
                        PPTBase );
        LOCK(f);
        f->eclock = ev;
        UNLOCK(f);
        // WaitForReply( PPTMSG_UPDATEPROGRESS, PPTBase );
    }

#endif

    /*
     *  Check and reply to all pending messages, if this is on a separate
     *  task.
     */

    if( FindTask(NULL) != globals->maintask ) {
        while( pmsg = (struct PPTMessage *)GetMsg( PPTBase->mport ) ) {
            if( pmsg->msg.mn_Node.ln_Type == NT_REPLYMSG ) {
                FreePPTMsg( pmsg, PPTBase );
            } else {
                ReplyMsg( pmsg );
            }
        }

        /*
         *   Check for break signal.
         */

        if(CheckSignal(SIGBREAKF_CTRL_C)) {
            D(bug("***** BREAK *****\n"));
            SetErrorCode( f, PERR_BREAK );
            return TRUE;
        }

        if(CheckSignal(SIGBREAKF_CTRL_F)) {
            if( f->mywin ) {
                WindowToFront(f->mywin->win);
                ActivateWindow(f->mywin->win);
            }
        }
    }

    return FALSE;
}
///
/// FinishProgress()
/****u* pptsupport/FinishProgress ******************************************
*
*   NAME
*       FinishProgress -- finish progress display.
*
*   SYNOPSIS
*       FinishProgress( frame )
*                       A0
*
*       VOID FinishProgress( FRAME * );
*
*   FUNCTION
*       Calling this function means that you are finished and you wish no
*       longer to use the progress display. If you still have something to do,
*       use InitProgress() again. Normally, you should call this function right
*       when you're finished. You may still do cleanups, etc., but nothing
*       slow, otherwise it might confuse the user.
*
*       Note that this routines does not actually close the progress display,
*       but renders it at 100% done.  PPT will close the window when you
*       return from the plugin.
*
*   INPUTS
*       frame - As usual.
*
*   RESULT
*
*   EXAMPLE
*       See autodoc for InitProgress().
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       InitProgress(), ClearProgress(), Progress().
*
******************************************************************************
*
*   Currently just a shorthand, like ClearProgress().
*/

Prototype VOID ASM FinishProgress( REGDECL(a0,FRAME *), REGDECL(a6,EXTBASE *) );

VOID SAVEDS ASM FinishProgress( REGPARAM(a0,FRAME *,frame),
                                REGPARAM(a6,EXTBASE *,PPTBase) )
{
    D(bug("FinishProgress()\n"));
    UpdateProgress( frame, NEGNUL, MAXPROGRESS, PPTBase );
}
///
/// CloseProgress()
/****i* pptsupport/CloseProgress ******************************************
*
*   NAME
*       CloseProgress -- closes progress display (V5)
*
*   SYNOPSIS
*       CloseProgress( frame )
*                       A0
*
*       VOID CloseProgress( FRAME * );
*
*   FUNCTION
*       TBA
*
*   INPUTS
*       frame - As usual.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       InitProgress(), ClearProgress(), Progress().
*
******************************************************************************
*
*
*/

Prototype VOID ASM CloseProgress( REGDECL(a0,FRAME *), REGDECL(a6,struct PPTBase *) );

VOID SAVEDS ASM CloseProgress( REGPARAM(a0,FRAME *,frame),
                               REGPARAM(a6,struct PPTBase *,PPTBase) )
{
    INFOWIN *iw;

    D(bug("CloseProgress()\n"));
    if( frame ) {
        if( frame->parent ) {
            iw = frame->parent->mywin;
        } else {
            iw = frame->mywin;
        }
        CloseInfoWindow( iw, PPTBase );
    }
}
///

/*
    Updates progress for a given frame. Currently updates only infowindow
    progress counter. done must be between MINPROGRESS and MAXPROGRESS. The
    infowindow need not be open for the update. However, frame must not be
    NULL. If txt == NULL, then the window is considered idle.
    However, if txt == NEGNUL, it just won't be updated.

    Try NOT to call directly!
*/

Prototype VOID ASM UpdateProgress( REGDECL(a0,FRAME *), REGDECL(a1,UBYTE *), REGDECL(d0,ULONG), REGDECL(a6,struct PPTBase *) );

SAVEDS VOID ASM UpdateProgress( REGPARAM(a0,FRAME *,f),
                                REGPARAM(a1,UBYTE *,txt),
                                REGPARAM(d0,ULONG, done),
                                REGPARAM(a6,EXTBASE *,PPTBase) )
{
    INFOWIN *iw;
    struct Window *win;
    struct Gadget *status,*progress;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    if(f) {

        // D(bug("Updateprogress(%s,%lu)\n",txt,done));

        if( IsPreview( f ) ) return; /* No preview frames are updated */

        SHLOCK(f);

        if(f->parent) {
            if(!CheckPtr(f->parent, "parent")) {
                Signal( FindTask(NULL), SIGBREAKF_CTRL_C );
                UNLOCK(f);
                return;
            }
            iw = f->parent->mywin;
        } else {
            iw = f->mywin;
        }

        UNLOCK(f);

        if(iw == NULL) {
            return;
        }

        if(!CheckPtr(iw,"infowin")) {
            Signal( FindTask(NULL), SIGBREAKF_CTRL_C );
            return;
        }

        if( FindTask(NULL) == globals->maintask ) {
            LOCK(iw);
            win         = iw->win;
            status      = (struct Gadget *)iw->GO_status;
            progress    = (struct Gadget *)iw->GO_progress;

            if(txt != NEGNUL) {

                if(txt)
                    XSetGadgetAttrs( PPTBase, status, win, NULL, INFO_TextFormat, txt, TAG_END );
                else
                    XSetGadgetAttrs( PPTBase, status, win, NULL, INFO_TextFormat, "«idle»", TAG_END );

            }

            XSetGadgetAttrs( PPTBase, progress, win, NULL, PROGRESS_Done, done, TAG_END );
            UNLOCK(iw);
        } else {
            struct ProgressMsg *msg;

            msg = (struct ProgressMsg *)AllocPPTMsg( sizeof(struct ProgressMsg), PPTBase );
            msg->pmsg.frame = f;
            msg->pmsg.code  = PPTMSG_UPDATEPROGRESS;
            msg->done       = done;
            msg->pmsg.data  = txt;
            if( (txt != NEGNUL) && (txt != NULL)) {
                strcpy( msg->text, txt );
                msg->pmsg.data = msg->text;
            }
            SendPPTMsg( globals->mport, msg, PPTBase );
        }

    } /* if(f) */
}

/*
    Deletes whatever MakeFrameName() added to the end of the string.
    Arguments are modified!
*/
Prototype VOID DeleteNameCount( UBYTE *str );

SAVEDS VOID DeleteNameCount( UBYTE *str )
{
    UBYTE *s;

    s = strchr( str, '[' );
    if( s ) {
        if( (str[strlen(str)-1] == ']') && (*(s-1) == ' ') ) *(s-1) = '\0';
    }
}

/*
    This routine will invent a new name for a given buffer. If oldname !=
    NULL, uses oldname as a base. If, however, another name exists by the
    same name, adds an unique identifier at the end. Returns a pointer to the new
    name. length contains the maximum length available in the newname buffer.

    newname may point to the same place as oldname, in which case the name
    is modified in place.  Of course, newname must always be != NULL
*/

Prototype UBYTE *  ASM MakeFrameName( REGDECL(a0,UBYTE *), REGDECL(a1,UBYTE *), REGDECL(d0,ULONG),REGDECL(a6,EXTBASE *) );

SAVEDS UBYTE * ASM MakeFrameName( REGPARAM(a0,UBYTE *,oldname),
                                  REGPARAM(a1,UBYTE *,newname),
                                  REGPARAM(d0,ULONG, length),
                                  REGPARAM(a6,EXTBASE *,xd) )
{
    struct Node *nd;
    UBYTE *s, tmp[40];
    static ULONG tmpcount = 0;
    ULONG appcount,maxappcount = 0;
    struct ExecBase *SysBase = xd->lb_Sys;

    if(oldname) {
        SHLOCKGLOB();
        for( nd = globals->frames.lh_Head; nd->ln_Succ; nd = nd->ln_Succ ) {
            s = strchr( nd->ln_Name, '[' );
            if( strncmp( oldname, nd->ln_Name, (s == NULL) ? 1024 : (ULONG)s - (ULONG)nd->ln_Name - 1 ) == 0 ) {
                // printf("*** Match found: %s vs. %s\n",oldname,nd->ln_Name);
                if(s) {
                    sscanf(s,"[%lu]",&appcount); appcount++;
                }
                else
                    appcount = 1;
                if(appcount > maxappcount) maxappcount = appcount;
            }
        }
        UNLOCKGLOB();

        if( newname != oldname )
            strncpy( newname, oldname, length );

        if(maxappcount > 0) {
            int difflen;

            sprintf( tmp, " [%lu]", maxappcount );
            difflen = length - strlen(tmp) - strlen(newname);
            if(difflen < 0) { /* Doesn't fit */
                UBYTE *loc, *t;
                t = tmp;
                difflen = -difflen;
                loc = newname + strlen(newname) - difflen - 1;
                while( *loc++ = *t++ ); /* Copy it. */
            } else {
                strcat( newname, tmp );
            }
        }

    } else {
        sprintf(newname,"Unknown.%lu",++tmpcount);
    }
    return newname;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

