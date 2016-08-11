/* Automatically generated header! Do not edit! */

#ifndef _PPCPRAGMA_PPTSUPP_H
#define _PPCPRAGMA_PPTSUPP_H
#ifdef __GNUC__
#ifndef _PPCINLINE__PPTSUPP_H
#include <powerup/ppcinline/pptsupp.h>
#endif
#else

#ifndef POWERUP_PPCLIB_INTERFACE_H
#include <powerup/ppclib/interface.h>
#endif

#ifndef POWERUP_GCCLIB_PROTOS_H
#include <powerup/gcclib/powerup_protos.h>
#endif

#ifndef NO_PPCINLINE_STDARG
#define NO_PPCINLINE_STDARG
#endif/* SAS C PPC inlines */

#ifndef PPTSUPP_BASE_NAME
#define PPTSUPP_BASE_NAME PPTBase
#endif /* !PPTSUPP_BASE_NAME */

#define AddExtension(frame, name, data, len, flags)     _AddExtension(PPTSUPP_BASE_NAME, frame, name, data, len, flags)

static __inline  PERROR
_AddExtension(void *PPTBase, FRAME *frame, STRPTR name, APTR data, ULONG len, ULONG flags)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) name;
        MyCaos.a2               =(ULONG) data;
        MyCaos.d0               =(ULONG) len;
        MyCaos.d1               =(ULONG) flags;
        MyCaos.caos_Un.Offset   =       (-210);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( PERROR)PPCCallOS(&MyCaos));
}

#define AskReqA(frame, objectlist)      _AskReqA(PPTSUPP_BASE_NAME, frame, objectlist)

static __inline  int
_AskReqA(void *PPTBase, FRAME *frame, struct TagItem *objectlist)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) objectlist;
        MyCaos.caos_Un.Offset   =       (-150);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( int)PPCCallOS(&MyCaos));
}

#ifndef NO_PPCINLINE_STDARG
#define AskReq(a0, tags...) \
        ({ULONG _tags[] = { tags }; AskReqA((a0), (struct TagItem *)_tags);})
#endif /* !NO_PPCINLINE_STDARG */

#define ClearProgress(frame)    _ClearProgress(PPTSUPP_BASE_NAME, frame)

static __inline  VOID
_ClearProgress(void *PPTBase, FRAME *frame)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.caos_Un.Offset   =       (-132);
        MyCaos.a6               =(ULONG) PPTBase;       
        (VOID)PPCCallOS(&MyCaos);
}

#define CopyFrameData(src, dest, flags) _CopyFrameData(PPTSUPP_BASE_NAME, src, dest, flags)

static __inline  PERROR
_CopyFrameData(void *PPTBase, FRAME *src, FRAME *dest, ULONG flags)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) src;
        MyCaos.a1               =(ULONG) dest;
        MyCaos.d0               =(ULONG) flags;
        MyCaos.caos_Un.Offset   =       (-246);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( PERROR)PPCCallOS(&MyCaos));
}

#define DupFrame(frame, flags)  _DupFrame(PPTSUPP_BASE_NAME, frame, flags)

static __inline  FRAME *
_DupFrame(void *PPTBase, FRAME *frame, ULONG flags)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) flags;
        MyCaos.caos_Un.Offset   =       (-54);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( FRAME *)PPCCallOS(&MyCaos));
}

#define FindExtension(frame, name)      _FindExtension(PPTSUPP_BASE_NAME, frame, name)

static __inline  struct Extension *
_FindExtension(void *PPTBase, FRAME *frame, STRPTR name)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) name;
        MyCaos.caos_Un.Offset   =       (-216);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( struct Extension *)PPCCallOS(&MyCaos));
}

#define FindFrame(id)   _FindFrame(PPTSUPP_BASE_NAME, id)

static __inline  FRAME *
_FindFrame(void *PPTBase, ULONG id)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.d0               =(ULONG) id;
        MyCaos.caos_Un.Offset   =       (-60);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( FRAME *)PPCCallOS(&MyCaos));
}

#define FinishProgress(frame)   _FinishProgress(PPTSUPP_BASE_NAME, frame)

static __inline  VOID
_FinishProgress(void *PPTBase, FRAME *frame)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.caos_Un.Offset   =       (-126);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define GetBackgroundColor(frame, pixel)        _GetBackgroundColor(PPTSUPP_BASE_NAME, frame, pixel)

static __inline  PERROR
_GetBackgroundColor(void *PPTBase, FRAME *frame, ROWPTR pixel)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) pixel;
        MyCaos.caos_Un.Offset   =       (-192);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( PERROR)PPCCallOS(&MyCaos));
}

#define GetBitMapRow(frame, row)        _GetBitMapRow(PPTSUPP_BASE_NAME, frame, row)

static __inline  UBYTE *
_GetBitMapRow(void *PPTBase, FRAME *frame, WORD row)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) row;
        MyCaos.caos_Un.Offset   =       (-102);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( UBYTE *)PPCCallOS(&MyCaos));
}

#define GetNPixelRows(frame, buf, row, nRows)   _GetNPixelRows(PPTSUPP_BASE_NAME, frame, buf, row, nRows)

static __inline  UWORD
_GetNPixelRows(void *PPTBase, FRAME *frame, ROWPTR *buf, WORD row, UWORD nRows)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) buf;
        MyCaos.d0               =(ULONG) row;
        MyCaos.d1               =(ULONG) nRows;
        MyCaos.caos_Un.Offset   =       (-90);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( UWORD)PPCCallOS(&MyCaos));
}

#define GetOptions(name)        _GetOptions(PPTSUPP_BASE_NAME, name)

static __inline  APTR
_GetOptions(void *PPTBase, STRPTR name)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) name;
        MyCaos.caos_Un.Offset   =       (-198);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( APTR)PPCCallOS(&MyCaos));
}

#define GetPixel(frame, row, col)       _GetPixel(PPTSUPP_BASE_NAME, frame, row, col)

static __inline  APTR
_GetPixel(void *PPTBase, FRAME *frame, WORD row, WORD col)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) row;
        MyCaos.d1               =(ULONG) col;
        MyCaos.caos_Un.Offset   =       (-66);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( APTR)PPCCallOS(&MyCaos));
}

#define GetPixelRow(frame, row) _GetPixelRow(PPTSUPP_BASE_NAME, frame, row)

static __inline  ROWPTR
_GetPixelRow(void *PPTBase, FRAME *frame, WORD row)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) row;
        MyCaos.caos_Un.Offset   =       (-78);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( ROWPTR)PPCCallOS(&MyCaos));
}

#define GetStr(string)  _GetStr(PPTSUPP_BASE_NAME, string)

static __inline  STRPTR
_GetStr(void *PPTBase, struct LocaleString *string)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) string;
        MyCaos.caos_Un.Offset   =       (-168);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( STRPTR)PPCCallOS(&MyCaos));
}

#define InitFrame(frame)        _InitFrame(PPTSUPP_BASE_NAME, frame)

static __inline  PERROR
_InitFrame(void *PPTBase, FRAME *frame)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.caos_Un.Offset   =       (-42);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( PERROR)PPCCallOS(&MyCaos));
}

#define InitProgress(frame, txt, min, max)      _InitProgress(PPTSUPP_BASE_NAME, frame, txt, min, max)

static __inline  VOID
_InitProgress(void *PPTBase, FRAME *frame, char *txt, ULONG min, ULONG max)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) txt;
        MyCaos.d0               =(ULONG) min;
        MyCaos.d1               =(ULONG) max;
        MyCaos.caos_Un.Offset   =       (-114);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define MakeFrame(frame)        _MakeFrame(PPTSUPP_BASE_NAME, frame)

static __inline  FRAME *
_MakeFrame(void *PPTBase, FRAME *frame)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.caos_Un.Offset   =       (-36);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( FRAME *)PPCCallOS(&MyCaos));
}

#define NewFrame(height, width, components)     _NewFrame(PPTSUPP_BASE_NAME, height, width, components)

static __inline  FRAME *
_NewFrame(void *PPTBase, ULONG height, ULONG width, UBYTE components)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.d0               =(ULONG) height;
        MyCaos.d1               =(ULONG) width;
        MyCaos.d2               =(ULONG) components;
        MyCaos.caos_Un.Offset   =       (-30);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( FRAME *)PPCCallOS(&MyCaos));
}

#define ObtainPreviewFrameA(frame, tags)        _ObtainPreviewFrameA(PPTSUPP_BASE_NAME, frame, tags)

static __inline  FRAME *
_ObtainPreviewFrameA(void *PPTBase, FRAME *frame, struct TagItem *tags)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) tags;
        MyCaos.caos_Un.Offset   =       (-228);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( FRAME *)PPCCallOS(&MyCaos));
}

#ifndef NO_PPCINLINE_STDARG
#define ObtainPreviewFrame(a0, tags...) \
        ({ULONG _tags[] = { tags }; ObtainPreviewFrameA((a0), (struct TagItem *)_tags);})
#endif /* !NO_PPCINLINE_STDARG */

#define PlanarToChunky(source, dest, width, depth)      _PlanarToChunky(PPTSUPP_BASE_NAME, source, dest, width, depth)

static __inline  VOID
_PlanarToChunky(void *PPTBase, UBYTE **source, ROWPTR dest, ULONG width, UWORD depth)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) source;
        MyCaos.a1               =(ULONG) dest;
        MyCaos.d0               =(ULONG) width;
        MyCaos.d1               =(ULONG) depth;
        MyCaos.caos_Un.Offset   =       (-156);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define Progress(frame, done)   _Progress(PPTSUPP_BASE_NAME, frame, done)

static __inline  BOOL
_Progress(void *PPTBase, FRAME *frame, ULONG done)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) done;
        MyCaos.caos_Un.Offset   =       (-120);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( BOOL)PPCCallOS(&MyCaos));
}

#define PutNPixelRows(frame, buf, row, nRows)   _PutNPixelRows(PPTSUPP_BASE_NAME, frame, buf, row, nRows)

static __inline  VOID
_PutNPixelRows(void *PPTBase, FRAME *frame, ROWPTR *buf, WORD row, UWORD nRows)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) buf;
        MyCaos.d0               =(ULONG) row;
        MyCaos.d1               =(ULONG) nRows;
        MyCaos.caos_Un.Offset   =       (-96);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define PutOptions(name, data, len)     _PutOptions(PPTSUPP_BASE_NAME, name, data, len)

static __inline  PERROR
_PutOptions(void *PPTBase, STRPTR name, APTR data, ULONG len)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) name;
        MyCaos.a1               =(ULONG) data;
        MyCaos.d0               =(ULONG) len;
        MyCaos.caos_Un.Offset   =       (-204);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( PERROR)PPCCallOS(&MyCaos));
}

#define PutPixel(frame, row, col, data) _PutPixel(PPTSUPP_BASE_NAME, frame, row, col, data)

static __inline  VOID
_PutPixel(void *PPTBase, FRAME *frame, WORD row, WORD col, APTR data)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) row;
        MyCaos.d1               =(ULONG) col;
        MyCaos.a1               =(ULONG) data;
        MyCaos.caos_Un.Offset   =       (-72);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define PutPixelRow(frame, row, data)   _PutPixelRow(PPTSUPP_BASE_NAME, frame, row, data)

static __inline  VOID
_PutPixelRow(void *PPTBase, FRAME *frame, WORD row, APTR data)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) row;
        MyCaos.a1               =(ULONG) data;
        MyCaos.caos_Un.Offset   =       (-84);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define ReleasePreviewFrame(frame)      _ReleasePreviewFrame(PPTSUPP_BASE_NAME, frame)

static __inline  VOID
_ReleasePreviewFrame(void *PPTBase, FRAME *frame)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.caos_Un.Offset   =       (-234);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define RemFrame(frame) _RemFrame(PPTSUPP_BASE_NAME, frame)

static __inline  VOID
_RemFrame(void *PPTBase, FRAME *frame)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.caos_Un.Offset   =       (-48);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define RemoveExtension(frame, name)    _RemoveExtension(PPTSUPP_BASE_NAME, frame, name)

static __inline  PERROR
_RemoveExtension(void *PPTBase, FRAME *frame, STRPTR name)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) name;
        MyCaos.caos_Un.Offset   =       (-222);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( PERROR)PPCCallOS(&MyCaos));
}

#define RenderFrame(frame, rastport, location, flags)   _RenderFrame(PPTSUPP_BASE_NAME, frame, rastport, location, flags)

static __inline  PERROR
_RenderFrame(void *PPTBase, FRAME *frame, struct RastPort *rastport, struct IBox *location, ULONG flags)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) rastport;
        MyCaos.a2               =(ULONG) location;
        MyCaos.d0               =(ULONG) flags;
        MyCaos.caos_Un.Offset   =       (-240);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( PERROR)PPCCallOS(&MyCaos));
}

#define SetErrorCode(frame, error)      _SetErrorCode(PPTSUPP_BASE_NAME, frame, error)

static __inline  VOID
_SetErrorCode(void *PPTBase, FRAME *frame, PERROR error)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) error;
        MyCaos.caos_Un.Offset   =       (-138);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define SetErrorMsg(frame, errorstring) _SetErrorMsg(PPTSUPP_BASE_NAME, frame, errorstring)

static __inline  VOID
_SetErrorMsg(void *PPTBase, FRAME *frame, UBYTE *errorstring)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.a1               =(ULONG) errorstring;
        MyCaos.caos_Un.Offset   =       (-144);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define StartInput(frame, mid, area)    _StartInput(PPTSUPP_BASE_NAME, frame, mid, area)

static __inline  BOOL
_StartInput(void *PPTBase, FRAME *frame, ULONG mid, struct PPTMessage *area)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.d0               =(ULONG) mid;
        MyCaos.a1               =(ULONG) area;
        MyCaos.caos_Un.Offset   =       (-180);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( BOOL)PPCCallOS(&MyCaos));
}

#define StopInput(frame)        _StopInput(PPTSUPP_BASE_NAME, frame)

static __inline  VOID
_StopInput(void *PPTBase, FRAME *frame)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.a0               =(ULONG) frame;
        MyCaos.caos_Un.Offset   =       (-186);
        MyCaos.a6               =(ULONG) PPTBase;       
        ( VOID)PPCCallOS(&MyCaos);
}

#define TagData(tagvalue, list) _TagData(PPTSUPP_BASE_NAME, tagvalue, list)

static __inline  ULONG
_TagData(void *PPTBase, Tag tagvalue, const struct TagItem *list)
{
struct Caos     MyCaos;
        MyCaos.M68kCacheMode    =       IF_CACHEFLUSHALL;
//      MyCaos.M68kStart        =       NULL;
//      MyCaos.M68kSize         =       0;
        MyCaos.PPCCacheMode     =       IF_CACHEFLUSHALL;
//      MyCaos.PPCStart         =       NULL;
//      MyCaos.PPCSize          =       0;
        MyCaos.d0               =(ULONG) tagvalue;
        MyCaos.a0               =(ULONG) list;
        MyCaos.caos_Un.Offset   =       (-174);
        MyCaos.a6               =(ULONG) PPTBase;       
        return(( ULONG)PPCCallOS(&MyCaos));
}

#endif /* SASC Pragmas */
#endif /* !_PPCPRAGMA_PPTSUPP_H */
