/*
    Debugging code.

    $Id: debug.c,v 1.18 1999/11/25 23:13:06 jj Exp $
*/

#include "defs.h"

Prototype VOID Debug_InternalError( const char *, const char *, int );
Prototype BPTR OpenDebugFile( enum DebugFile_T );
Prototype VOID DeleteDebugFiles( VOID );
Prototype VOID SetDebugDir( char * );

/*------------------------------------------------------------------------*/
/* Globals */

char debugdir[256] = "dtmp:";
const char *debugfilenames[] = { "load", "save", "effect", "render", "ppt", "getargs" };
int debugfilecounters[] = { 0, 0, 0, 0, 0, 0 };

/*------------------------------------------------------------------------*/
/* General debugging code. */

/*
    Display an internal error message. If the debug output is active,
    write out a message to the console as well.
*/
SAVEDS VOID Debug_InternalError( const char *txt, const char *file, int line )
{
    D(bug("***** SOFT ERROR: %s/%ld : %s\n",file,line,txt));

    Req( NEGNUL, NULL,
         GetStr(mPPT_SOFT_ERROR),
         file, line, txt );
}

VOID SetDebugDir( char *dir )
{
    strncpy( debugdir, dir, 255 );
}

/*
    Opens a debug file for the separate tasks.
*/

BPTR OpenDebugFile( enum DebugFile_T type )
{
    BPTR fh = NULL;
    char buffer[40], path[256];

    if( type == DFT_Main )
        strcpy( buffer, "ppt.log" );
    else
        sprintf( buffer, "%s_%d.log", debugfilenames[type], debugfilecounters[type]++ );

    strcpy( path, debugdir );
    if( AddPart(path, buffer, 255) ) {
        if(type != DFT_Main ) D(bug("Attempting to open file called '%s' for debug purposes\n",path));
        fh = Open( path, MODE_NEWFILE );
    }

    if(!fh) fh = Open("NIL:", MODE_NEWFILE );

    return fh;
}

/*
    Remove any debug files still in the debug directory.
*/
VOID DeleteDebugFiles( VOID )
{
#ifdef DEBUG_MODE
    char buffer[256] = "#?.log", path[256];

    strcpy( path, debugdir );
    AddPart( path, buffer, 256 );
    sprintf(buffer, "Delete %s QUIET", path );

    System( buffer, NULL );
#endif
}

/*------------------------------------------------------------------------*/
/* Dependants of FORTIFY */

#ifdef FORTIFY

/* Returns FALSE if this is a bad pointer */

#include "fortify.h"

SAVEDS BOOL Debug_CheckPtr( const char *txt, const APTR addr, const char *file, int line )
{

    if( Fortify_CheckPointer( addr ) == FALSE ) {
        D(bug("***** MEMORY ACCESS ERROR: in %s/%ld, Address=%08X (%s)\n",
              file,line,addr,txt));

        Req( NEGNUL, NULL,
             ISEQ_B ISEQ_C"PPT Soft Error:   "ISEQ_N"Illegal Memory Access\n\n"
             "File '%s', line %ld.  Address = 0x%08lX\n\n"
             "%s\n\n"
             ISEQ_C ISEQ_I
             "A pointer within the software is pointing\n"
             "somewhere it shouldn't.  It may or may not\n"
             "be dangerous, so you should quit the program\n"
             "as soon as you can.  And if you can reproduce\n"
             "this error, please submit a bug report!\n",
             file, line, addr, txt );
        return FALSE;
    }
    return TRUE;
}

#endif /* FORTIFY */

/*------------------------------------------------------------------------*/


#ifndef DEBUG_MODE

#define DumpRenderObject(x)
#define DumpMem(x,y)
#define DumpPixInfo(x)
#define DumpDisplay(x)
#define DumpFrame(x)
#define DumpInfoWin(x)
#define StartBench()    NULL
#define StopBench(x)
#define DoShowColorTable(x,y)

#include <clib/dos_protos.h>

void DEBUG( const char *c, ... )
{
    PutStr("*** ILLEGAL DEBUG MESSAGE ***:");
    PutStr(c);
    PutStr("\n");
}

#else

#ifndef STDIO_H
#include <stdio.h>
#endif

#ifndef CTYPE_H
#include <ctype.h>
#endif

#ifndef STDARG_H
#include <stdarg.h>
#endif

#ifndef DEFS_H
#include <defs.h>
#endif

#include "render.h"

#ifndef PRAGMAS_INTUITION_PRAGMAS_H
#include <pragmas/intuition_pragmas.h>
#endif

#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif

Prototype void DumpRenderObject( struct RenderObject *);
Prototype void DumpMem( ULONG start,  ULONG len);
Prototype void DEBUG(const char *c, ...);
Prototype void DumpPixInfo( PIXINFO *p );
Prototype void DumpDisplay( DISPLAY *d );
Prototype void DumpFrame( FRAME *f );
Prototype void DumpInfoWin( INFOWIN *i );
Prototype APTR StartBench( void );
Prototype void StopBench( APTR );
Prototype void DoShowColorTable( ULONG, COLORMAP * );

struct BenchTime {
    ULONG seconds, micros;
    struct Library *IntuitionBase;
};

/*
    These two are benchmarking functions. Call StartBench() first
    and StopBench() next.
*/

SAVEDS APTR StartBench( void )
{
    struct BenchTime *bt;
    APTR IntuitionBase, SysBase = SYSBASE();

    IntuitionBase = OpenLibrary("intuition.library",36L);
    if(!IntuitionBase) {
        DEBUG("StartBench(): Intuition V36 not found!\n");
        return NULL;
    }

    bt = AllocMem( sizeof( struct BenchTime ),0L );
    if(!bt) {
        DEBUG("StartBench(): Memory allocation error, no benchmarking.\n");
        return NULL;
    }

    bt->IntuitionBase = IntuitionBase;

    CurrentTime( &bt->seconds, &bt->micros );
    return (APTR)bt;
}

/*
    BUG: The microseconds printout is wrong.  Oh well...
 */

SAVEDS void StopBench( APTR bt1 )
{
    struct BenchTime bt2;
    LONG ds, dm;
    APTR SysBase, IntuitionBase = ((struct BenchTime *)bt1)->IntuitionBase;

    CurrentTime( &bt2.seconds, &bt2.micros );

    if(!bt1)
        return;

    ds = (LONG)bt2.seconds - ((struct BenchTime *)bt1)->seconds;
    dm = (LONG)bt2.micros  - ((struct BenchTime *)bt1)->micros;

    if(dm < 0) { ds--; dm += 1000000; }

    DEBUG("BENCHMARK: Operation took %lu.%lu seconds (with overhead)\n",
           ds, dm );

    SysBase = SYSBASE();

    FreeMem( bt1, sizeof( struct BenchTime ) );

    CloseLibrary(IntuitionBase);
}

SAVEDS void DumpMem( ULONG start,  ULONG len)
{
    ULONG i;
    char buf[20] = "";
    unsigned char c;

    DEBUG("Memory dump from %X to %X\n",start,start+len);

    for(i = 0; i < len;i++) {
        if( i % 16 == 0 )
            DEBUG(" %s\n%08X:",buf,start+i);
        if( i % 2 == 0)
            DEBUG(" ");
        c = * (unsigned char *)(start+i);
        DEBUG("%02X",c);
        buf[i%16] = (isprint(c) ? c : '.');
        buf[(i%16)+1] = '\0';
    }
    DEBUG(" %s\n",buf);
}


SAVEDS void DumpPixInfo( PIXINFO *p )
{
    DEBUG("PICTURE:\nColorspace: %d ... Picture size : %u x %u\n",p->colorspace,p->height,p->width);
    DEBUG("data = %X, begin=%lu, end=%lu\n",p->vmh->data,p->vmh->begin,p->vmh->end);
}


SAVEDS void DumpDisplay( DISPLAY *d )
{
    DEBUG("DISPLAY:\nScreen = %X, Window = %x, display ID = %lu\n",
            d->scr,d->win,d->dispid);
}


SAVEDS void DumpFrame( FRAME *f )
{
    DEBUG("****** FRAME at %X ******\n",f);
    if(f) {
        DEBUG("SUCC=%X, PRED=%X, NAME=%s\n",f->nd.ln_Succ, f->nd.ln_Pred, (f->nd.ln_Name) ? f->nd.ln_Name : "NULL" );
        DEBUG("INFOWIN @ %X, currext=%X, path='%s'\n",f->mywin, f->currext, f->path);
        DumpPixInfo( f->pix );
        DumpDisplay( f->disp );
    }
}


SAVEDS void DumpInfoWin( INFOWIN *i )
{
    DEBUG("****** INFOWIN @ %X ******\n",i);
    if(i) {
        DEBUG("FRAME @ %X, BGUIObject @ %X, Window @ %X\n",i->myframe, i->WO_win, i->win);
        DEBUG("id = %lu\n",i->id);
    }
}

SAVEDS void DumpRenderObject( struct RenderObject *r )
{
    DEBUG("****** RENDER OBJECT @ %08X ******\n",r );
    if(r) {
        DEBUG("FRAME: %08X, cp=%08X, buffer=%08X\n",r->frame,r->cp,r->buffer);
        DEBUG("Dither:\n\tdither=%lu\n\tDitherObject=%08X\n",r->dither,r->DitherObject);
        DEBUG("\tDither()=%08X\n\tDestroy()=%08X\n",r->Dither,r->DitherD);

        DEBUG("Palette:\n\tncolors=%d\n\thistograms=%08X\ncolortable=%08X\n",
               r->ncolors,r->histograms,r->colortable);

        DEBUG("DeviceIO:\n\tDispDeviceObject=%08X\n",r->DispDeviceObject);
        DEBUG("\tOpen()=%08X\n\tClose()=%08X\n\tDestroy()=%08X\n\tWriteLine()=%08X\n"
              "\tLoadCMap()=%08X\n\tActivate()=%08X\n",
              r->DispDeviceOpen, r->DispDeviceClose, r->DispDeviceD,
              r->WriteLine, r->LoadCMap, r->ActivateDisplay );
    }
    DEBUG("\n");
}

SAVEDS
void DEBUG(const char *c, ...)
/* A general debug routine. Prints debug information, if the global
   debuglevel == level OR debuglevel has been set to -1.
   The second argument is a printf() - like format string, followed by
   associated arguments. Note: varargs.h & stdarg.h MUST be #included!
*/
{

    va_list va;
    char buf[256];
    static int usecount = 0;

    while(usecount != 0)
        Delay(2L);

    usecount++;
    va_start(va,c);
    vsprintf(buf,c,va);
    va_end(va);

    PutStr(buf);
    Flush(Output());

    --usecount;
}

void DoShowColorTable( ULONG nColors, COLORMAP *colortable )
{
    int i=0;

    DEBUG("Showing colortable @ %08X, %u colors...\n",colortable, nColors );
    do {
        DEBUG("%03d: %02X-%02X-%02X (%3d)",i,colortable[i].r,colortable[i].g,colortable[i].b,colortable[i].a);
        if( i % 4 == 3 )
            DEBUG("\n");
        else
            DEBUG("\t");
    } while ( ++i < nColors );

    DEBUG("\n");
}

Prototype VOID SaveMainPalette(VOID);

VOID SaveMainPalette(VOID)
{
    FILE *fp;
    char buf[80];
    int nColors = 1<<globals->maindisp->depth;
    ULONG colors[256][3];
    int i;

    GetRGB32( globals->maindisp->scr->ViewPort.ColorMap, 0, nColors, &colors[0][0] );

    sprintf(buf,"T:colors_%d.c",globals->maindisp->depth);

    fp = fopen( buf, "w" );
    fprintf(fp,"unsigned char colors_%d[] = {\n", globals->maindisp->depth );

    for( i = 0; i < nColors; i++ ) {
        fprintf( fp, "0x%lX,0x%lX,0x%lX,\n", colors[i][0]>>24,colors[i][1]>>24,colors[i][2]>>24 );
    }

    fprintf(fp, "};\n");

    fclose(fp);
}


/*
    Extremely dangerous code!  DO NOT USE UNLESS YOU REALLY KNOW
    WHAT YOU'RE DOING

    BUG:  Causes Enforcer hits.
*/

Prototype VOID HitNull(VOID);

VOID HitNull(VOID)
{
    UBYTE *a = NULL;

    *a = 0;
}

#endif /* ifndef DEBUG_MODE */
