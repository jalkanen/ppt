
#ifndef PPTLIB_PROTOS_H
#define PPTLIB_PROTOS_H

/*

    Here are the prototypes for the PPT support link library functions.

    $Id: pptsupp_protos.h 6.0 1999/09/05 16:20:15 jj Exp jj $

*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef PPT_H
#include <ppt.h>
#endif

FRAME *  NewFrame( ULONG, ULONG, UBYTE );
FRAME *  DupFrame( FRAME *, ULONG );
VOID     RemFrame( FRAME * );
VOID     ClearProgress( FRAME * );
VOID     FinishProgress( FRAME * );
ROWPTR   GetPixelRow( FRAME *, WORD );
VOID     PutPixelRow( FRAME *, WORD, APTR );
UWORD    GetNPixelRows( FRAME *, ROWPTR *, WORD, UWORD );
VOID     PutNPixelRows( FRAME *, ROWPTR *, WORD, UWORD );
APTR     GetPixel( FRAME *, WORD, WORD );
VOID     PutPixel( FRAME *, WORD, WORD, APTR );
PERROR   BeginLoad( FRAME * );
VOID     EndLoad( FRAME * );
VOID     UpdateProgress( FRAME *, UBYTE *, ULONG );
VOID     InitProgress( FRAME *, char *, ULONG, ULONG );
BOOL     Progress( FRAME *, ULONG );
ULONG    TagData( Tag, const struct TagItem * );
FRAME *  MakeFrame( FRAME * );
PERROR   InitFrame( FRAME *);
UBYTE *  GetBitMapRow( FRAME *, WORD );
int      AskReqA( FRAME *, struct TagItem * );
#ifdef __SASC
int      AskReq( FRAME *, ... );
#endif
VOID     PlanarToChunky( UBYTE **, ROWPTR, ULONG, UWORD );
VOID     SetErrorCode( FRAME *, PERROR );
VOID     SetErrorMsg( FRAME *, UBYTE * );
STRPTR   GetStr( struct LocaleString * );
BOOL     StartInput( FRAME *, ULONG, struct PPTMessage * );
VOID     StopInput( FRAME * );
FRAME *  FindFrame( ULONG );
PERROR   GetBackgroundColor( FRAME *, ROWPTR );
PERROR   PutOptions( STRPTR, APTR, ULONG );
APTR     GetOptions( STRPTR );
PERROR   AddExtension( FRAME *, STRPTR, APTR, ULONG, ULONG );
struct Extension *FindExtension( FRAME *, STRPTR );
PERROR   RemoveExtension( FRAME *, STRPTR );

FRAME *  ObtainPreviewFrameA( FRAME *, struct TagItem * );
FRAME *  ObtainPreviewFrame( FRAME *, ... );
VOID     ReleasePreviewFrame( FRAME * );
PERROR   RenderFrame( FRAME *, struct RastPort *, struct IBox *, ULONG );
PERROR   CopyFrameData( FRAME *, FRAME *, ULONG );

VOID     CloseProgress( FRAME * );
LONG     SetRexxVariable( FRAME *, STRPTR, STRPTR );

ULONG    SPrintFA( STRPTR, STRPTR, APTR );
#ifdef __SASC
ULONG    SPrintF( STRPTR, STRPTR, ... );
#endif

/*
 *  The following functions are link-only.
 */

VOID     PDebug( const char *, ... );
VOID     Stop(VOID);
VOID     DReq( const char *, ... );
void     DumpMem( unsigned char *, long, char );

#ifdef _DCC
int      AskReq( EXTBASE *, FRAME *, Tag, ... );
PERROR   ExecA4( FRAME *, struct TagItem *, EXTBASE *ExtBase );
ULONG    pgeta4(VOID);
VOID     prela4(VOID);
BOOL     OpenFloats(VOID);
VOID     CloseFloats(VOID);
#endif

#endif /* PPTLIB_PROTOS_H */
