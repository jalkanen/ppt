/*
    PROJECT: ppt
    MODULE:  freeselect.c

    Freeform selection routines.

    $Id: freeselect.c,v 6.1 1999/11/28 18:20:10 jj Exp $

 */

#include "defs.h"
#include "misc.h"

#include <proto/timer.h>

#include <math.h>

#ifndef GRAPHICS_GFXMACROS_H
#include <graphics/gfxmacros.h>
#endif

typedef struct EdgeRec_T * EdgePtr;

typedef struct EdgeRec_T {
    int yUpper;
    float xIntersect;
    float dxPerScan;
    EdgePtr next;
} EdgeRec;


#ifdef DEBUG_MODE
VOID PrintList( char *name, EdgePtr list )
{
    EdgePtr p, q;
    int i = 0;

    q = list;
    p = q->next;

    bug("EDGEPTR_LIST %s : (list=%08X):\n",name, list);

    while( p ) {
        bug("\tEdge %d @ %08X: yUpper=%d, xIntersect=%.2f, dxPerScan=%.2f\n",
             i, p, p->yUpper, p->xIntersect, p->dxPerScan );

        p = p->next;

        i++;
    }
}
#endif

/*------------------------------------------------------------------------*/

/// InsertEdge()
VOID InsertEdge( EdgePtr list, EdgePtr edge )
{
    EdgePtr p, q;

    q = list;
    p = q->next;

    while( p ) {
        if( edge->xIntersect < p->xIntersect ) {
            p = NULL;
        } else {
            q = p;
            p = p->next;
        }
    }
    edge->next = q->next;
    q->next = edge;
}
///
/// YNext()
int YNext( int k, int cnt, Point *pts )
{
    int j;

    if( k > cnt ) {
        j = 0;
    } else {
        j = k+1;
    }

    while( pts[k].y == pts[j].y ) {
        if( j > cnt ) {
            j = 0;
        } else {
            j++;
        }
    }
    return pts[j].y;
}
///
/// MakeEdgeRec()
VOID MakeEdgeRec( Point lower, Point upper, int yComp, EdgePtr *edges, EdgePtr edge )
{
    edge->dxPerScan = (upper.x - lower.x) / (upper.y - lower.y);
    edge->xIntersect = lower.x;
    if( upper.y < yComp )
        edge->yUpper = upper.y - 1;
    else
        edge->yUpper = upper.y;

    InsertEdge( edges[lower.y], edge );
}

///
/// BuildEdgeList()
VOID BuildEdgeList( int cnt, Point *pts, EdgePtr *edges )
{
    EdgePtr edge;
    Point v1,v2;
    int yPrev,i;

    D(bug("\tBuildEdgeList(cnt=%d)\n",cnt));

    v1 = pts[cnt-1];
    yPrev = pts[cnt-2].y;
    for( i = 0; i < cnt; i++ ) {
        v2 = pts[i];

        if( v1.y != v2.y ) {
            /* non-horizontal line */
            edge = smalloc( sizeof( EdgeRec ) );
            bzero(edge,sizeof(edge));

            if( v1.y < v2.y )
                MakeEdgeRec( v1, v2, YNext(i,cnt,pts), edges, edge );
            else
                MakeEdgeRec( v2, v1, yPrev, edges, edge );
        }

        yPrev = v1.y;
        v1 = v2;
    }
}
///
/// BuildActiveList()
VOID BuildActiveList( int scan, EdgePtr *edges, EdgePtr active )
{
    EdgePtr p, q;

    p = edges[scan]->next;
    while( p ) {
        q = p->next;
        InsertEdge( active, p );
        p = q;
    }
}
///
/// FillScan()
VOID FillScan( ROWPTR line, int scan, EdgePtr active )
{
    EdgePtr p1, p2;
    int i;

    D(PrintList( "active", active ));

    p1 = active->next;

    while( p1 && p1->next ) {
        p2 = p1->next;
        for( i = floor(p1->xIntersect); i < floor(p2->xIntersect)-1; i++ ) {
            line[i] = 1;
        }
        p1 = p2->next;
        // p1 = p2;
    }
}
///
/// DeleteAfter()
VOID DeleteAfter( EdgePtr q )
{
    EdgePtr p;

    p = q->next;
    q->next = p->next;

    D(bug("Freeing EdgePtr @ %08x\n",p ));
    sfree( p );
}
///
/// UpdateActiveList()
VOID UpdateActiveList( int scan, EdgePtr active )
{
    EdgePtr p, q;

    q = active;
    p = active->next;

    while( p ) {
        if( scan >= p->yUpper ) {
            p = p->next;
            DeleteAfter( q );
        } else {
            p->xIntersect = p->xIntersect + p->dxPerScan;
            q = p;
            p = p->next;
        }
    }
}
///
/// ResortActiveList()
VOID ResortActiveList( EdgePtr active )
{
    EdgePtr p, q;

    p = active->next;
    active->next = NULL;

    while( p ) {
        q = p->next;
        InsertEdge( active, p );
        p = q;
    }
}
///
/// ScanFill()
VOID
ScanFill( FRAME *frame )
{
    int cnt = frame->selection.nVertices;
    Point *pts = frame->selection.vertices;
    EdgePtr *edges, active;
    int scan, i;

    edges = pmalloc( sizeof(EdgePtr) * (frame->pix->height+1) );

    D(bug("ScanFill: allocating edges\n"));

    for( i = 0; i <= frame->pix->height; i++ ) {
        edges[i] = smalloc( sizeof(EdgeRec) );
        edges[i]->next = NULL;
    }

    active = smalloc( sizeof(EdgeRec) );
    active->next = NULL;

    BuildEdgeList( cnt, pts, edges );

    for( scan = 0; scan < frame->pix->height; scan++ ) {

        D(bug("    Line=%d\n",scan));

        D(bug("List: edges[%d]\n",scan));
        D(PrintList( "", edges[scan] ));

        BuildActiveList( scan, edges, active );

        // D(bug("List: edges[%d]\n",scan));
        // D(PrintList( "", edges[scan] ));

        D(PrintList( "active", active ));

        if( active->next ) {
            ROWPTR cp;

            cp = GetPixelRow( frame->selection.mask, scan, globxd );

            bzero( cp, frame->pix->bytes_per_row );

            FillScan( cp, scan, active );

            PutPixelRow( frame->selection.mask, scan, cp, globxd );

            UpdateActiveList( scan, active );
            ResortActiveList( active );
        }

    }

    D(bug("    ScanFill done\n"));

#if 1

    for( i = 0; i <= frame->pix->height; i++ ) {
        if( edges[i] ) {
            sfree( edges[i] );
        }
    }
#endif

#if 1
    if( active ) {
        EdgePtr e,ep;

        D(PrintList( "active", active ));

        ep = active->next;

        while( ep ) {
            e = ep->next;
            sfree( ep );
            ep = e;
        }

        sfree( active );
    }
#endif

    pfree(edges);
}
///

/// FreeMakeBoundingBox
Local
VOID FreeMakeBoundingBox( FRAME *frame )
{
    struct Selection *selection = &frame->selection;
    struct Rectangle rect = {32767,32767,-1,1};
    int i;

    for(i = 0; i < selection->nVertices; i++ ) {
        WORD x, y;

        x = selection->vertices[i].x;
        y = selection->vertices[i].y;

        if( x < rect.MinX ) rect.MinX = x;
        if( x > rect.MaxX ) rect.MaxX = x;

        if( y < rect.MinY ) rect.MinY = y;
        if( y > rect.MaxY ) rect.MaxY = y;
    }

    D(bug("Calculated bounding box: (%d,%d)-(%d,%d)\n",
           rect.MinX,rect.MinY, rect.MaxX, rect.MaxY ));

    frame->selbox = rect;
}
///

/// ButtonDown
Local
VOID FreeButtonDown( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc = msg->xloc;
    WORD yloc = msg->yloc;

    if( IsFrameBusy(frame) ) return;

    if( IsAreaSelected(frame) && (selection->selstatus & SELF_DRAWN) ) { /* Delete old. */
        EraseSelection( frame );
    }

    /*
     *  Just mark the area as started.
     */

    selection->vertices[0].x = xloc;
    selection->vertices[0].y = yloc;
    selection->nVertices = 1;

    selection->selstatus |= SELF_BUTTONDOWN;

    frame->selbox.MinX = 0;

    D(bug("Marked freearea start (%d,%d)\n",xloc,yloc));
}
///
/// MouseMove
Local
VOID FreeMouseMove( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    WORD xloc = msg->xloc;
    WORD yloc = msg->yloc;

    if( IsAreaSelected(frame) && (selection->selstatus & SELF_BUTTONDOWN) ) {

        if( selection->vertices[selection->nVertices-1].x != xloc ||
            selection->vertices[selection->nVertices-1].y != yloc )
        {

            EraseSelection( frame );

            selection->vertices[selection->nVertices].x = xloc;
            selection->vertices[selection->nVertices].y = yloc;

            selection->nVertices++;

            // BUG: should alloc more space, if running out.

            D(bug("Vertex %d: (%d,%d)\n", selection->nVertices-1, xloc, yloc ));

            // BUG: Uses lots of time.
            FreeMakeBoundingBox( frame );
            DrawSelection( frame, DSBF_INTERIM );
            UpdateIWSelbox( frame, FALSE );
        } else {
            D(bug("Vertex %d rejected, because it was the same as before (%d,%d)\n",
                   selection->nVertices, xloc, yloc ));
        }
    }
}
///
/// ButtonUp
Local
VOID FreeButtonUp( FRAME *frame, struct MouseLocationMsg *msg )
{
    struct Selection *selection = &frame->selection;
    // WORD xloc = msg->xloc;
    // WORD yloc = msg->yloc;

    if( IsFrameBusy(frame) ) return;

    EraseSelection( frame );

    // Make sure it's closing.
    selection->vertices[selection->nVertices] = selection->vertices[0];
    selection->nVertices++;

    D(bug("Marked final vertex %d at (%d,%d)\n",selection->nVertices-1,
          selection->vertices[0].x, selection->vertices[0].y ));

    FreeMakeBoundingBox( frame );

    ScanFill( frame );

    UpdateIWSelbox( frame, TRUE );
    DrawSelection( frame, 0L );

    selection->selstatus &= ~SELF_BUTTONDOWN;
}
///

/// DrawArea
Local
VOID DrawArea( FRAME *frame, Point *coords, WORD nPoints )
{
    struct Selection *selection = &frame->selection;
    DISPLAY *d = frame->disp;
    struct RastPort *rPort = d->win->RPort;
    struct IBox *abox;
    WORD x,y, winwidth, winheight, xoffset, yoffset;
    int i;

    if( selection->nVertices > 2 ) {
        GetAttr( AREA_AreaBox, d->RenderArea, (ULONG *) &abox );
        winwidth  = abox->Width;
        winheight = abox->Height;
        xoffset   = abox->Left;
        yoffset   = abox->Top;

        SetDrMd(rPort, COMPLEMENT);
        SetDrPt(rPort, (UWORD) frame->disp->selpt);    // Uses just lower 16 bits

        x = MULS16((selection->vertices[0].x - frame->zoombox.Left),winwidth) / frame->zoombox.Width + xoffset;
        y = MULS16((selection->vertices[0].y - frame->zoombox.Top),winheight) / frame->zoombox.Height + yoffset;

        Move( rPort, x, y );

        for( i = 1; i < selection->nVertices; i++ ) {
            x = MULS16((selection->vertices[i].x - frame->zoombox.Left),winwidth) / frame->zoombox.Width + xoffset;
            y = MULS16((selection->vertices[i].y - frame->zoombox.Top),winheight) / frame->zoombox.Height + yoffset;

            Draw( rPort, x, y );
        }
    }
}
///

/// Draw&Erase
Local
VOID FreeDraw( FRAME *frame, ULONG flags )
{
    struct Selection *selection = &frame->selection;

    DrawArea( frame, selection->vertices, selection->nVertices );
}

Local
VOID FreeErase( FRAME *frame )
{
    struct Selection *selection = &frame->selection;

    DrawArea( frame, selection->vertices, selection->nVertices );
}
///

/// IsInArea
Local
BOOL FreeIsInArea( FRAME *frame, WORD row, WORD col )
{
    ROWPTR cp;

    cp = GetPixelRow( frame->selection.mask, row, globxd );
    if( cp[col] )
        return TRUE;

    return FALSE;
}
///

/// InitFreeSelection()
#define NOTSAMESIZE(x,y) (( (x)->pix->width != (y)->pix->width) \
                         || ((x)->pix->height != (y)->pix->height) )

Prototype PERROR InitFreeSelection( FRAME *frame );

PERROR InitFreeSelection( FRAME *frame )
{
    struct Selection *selection = &frame->selection;
    FRAME *mask = selection->mask;

    D(bug("InitFreeSelection(%08X)\n",frame));

    if( !mask || NOTSAMESIZE( mask, frame ) ) {

        if( mask ) RemFrame( mask, globxd );

        mask = MakeFrame( frame, globxd );
        mask->pix->components = 1;
        mask->pix->colorspace = CS_UNKNOWN;

        if( InitFrame( mask, globxd ) != PERR_OK )
            return PERR_ERROR;

        frame->selection.mask = mask;
    }

    if( !selection->vertices ) {
        if( NULL == (selection->vertices = smalloc( sizeof(Point) * 1024 ))) {
            Panic( "Can't alloc vertices!!!" );
        }
    }

    selection->ButtonDown        = FreeButtonDown;
    selection->ButtonUp          = FreeButtonUp;
    // s->ControlButtonDown = LassoRectControlButtonDown;
    selection->DrawSelection     = FreeDraw;
    selection->EraseSelection    = FreeErase;
    selection->MouseMove         = FreeMouseMove;
    selection->IsInArea          = FreeIsInArea;
    //s->Rescale           = LassoRectRescale;
    //s->Copy              = LassoRectCopy;

    return PERR_OK;
}

///
