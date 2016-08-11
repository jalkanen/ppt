/* Automatically generated header! Do not edit! */

#ifndef _INLINE_PPTSUPP_H
#define _INLINE_PPTSUPP_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef PPTSUPP_BASE_NAME
#define PPTSUPP_BASE_NAME PPTBase
#endif /* !PPTSUPP_BASE_NAME */

#define AddExtension(frame, name, data, len, flags) \
	LP5(0xd2, PERROR, AddExtension, FRAME *, frame, a0, STRPTR, name, a1, APTR, data, a2, ULONG, len, d0, ULONG, flags, d1, \
	, PPTSUPP_BASE_NAME)

#define AskReqA(frame, objectlist) \
	LP2(0x96, int, AskReqA, FRAME *, frame, a0, struct TagItem *, objectlist, a1, \
	, PPTSUPP_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AskReq(a0, tags...) \
	({ULONG _tags[] = { tags }; AskReqA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define ClearProgress(frame) \
	LP1NR(0x84, ClearProgress, FRAME *, frame, a0, \
	, PPTSUPP_BASE_NAME)

#define CloseProgress(frame) \
	LP1NR(0xfc, CloseProgress, FRAME *, frame, a0, \
	, PPTSUPP_BASE_NAME)

#define CopyFrameData(src, dest, flags) \
	LP3(0xf6, PERROR, CopyFrameData, FRAME *, src, a0, FRAME *, dest, a1, ULONG, flags, d0, \
	, PPTSUPP_BASE_NAME)

#define DupFrame(frame, flags) \
	LP2(0x36, FRAME *, DupFrame, FRAME *, frame, a0, ULONG, flags, d0, \
	, PPTSUPP_BASE_NAME)

#define FindExtension(frame, name) \
	LP2(0xd8, struct Extension *, FindExtension, FRAME *, frame, a0, STRPTR, name, a1, \
	, PPTSUPP_BASE_NAME)

#define FindFrame(id) \
	LP1(0x3c, FRAME *, FindFrame, ULONG, id, d0, \
	, PPTSUPP_BASE_NAME)

#define FinishProgress(frame) \
	LP1NR(0x7e, FinishProgress, FRAME *, frame, a0, \
	, PPTSUPP_BASE_NAME)

#define GetBackgroundColor(frame, pixel) \
	LP2(0xc0, PERROR, GetBackgroundColor, FRAME *, frame, a0, ROWPTR, pixel, a1, \
	, PPTSUPP_BASE_NAME)

#define GetBitMapRow(frame, row) \
	LP2(0x66, UBYTE *, GetBitMapRow, FRAME *, frame, a0, WORD, row, d0, \
	, PPTSUPP_BASE_NAME)

#define GetNPixelRows(frame, buf, row, nRows) \
	LP4(0x5a, UWORD, GetNPixelRows, FRAME *, frame, a0, ROWPTR *, buf, a1, WORD, row, d0, UWORD, nRows, d1, \
	, PPTSUPP_BASE_NAME)

#define GetOptions(name) \
	LP1(0xc6, APTR, GetOptions, STRPTR, name, a0, \
	, PPTSUPP_BASE_NAME)

#define GetPixel(frame, row, col) \
	LP3(0x42, APTR, GetPixel, FRAME *, frame, a0, WORD, row, d0, WORD, col, d1, \
	, PPTSUPP_BASE_NAME)

#define GetPixelRow(frame, row) \
	LP2(0x4e, ROWPTR, GetPixelRow, FRAME *, frame, a0, WORD, row, d0, \
	, PPTSUPP_BASE_NAME)

#define GetStr(string) \
	LP1(0xa8, STRPTR, GetStr, struct LocaleString *, string, a0, \
	, PPTSUPP_BASE_NAME)

#define InitFrame(frame) \
	LP1(0x2a, PERROR, InitFrame, FRAME *, frame, a0, \
	, PPTSUPP_BASE_NAME)

#define InitProgress(frame, txt, min, max) \
	LP4NR(0x72, InitProgress, FRAME *, frame, a0, char *, txt, a1, ULONG, min, d0, ULONG, max, d1, \
	, PPTSUPP_BASE_NAME)

#define MakeFrame(frame) \
	LP1(0x24, FRAME *, MakeFrame, FRAME *, frame, a0, \
	, PPTSUPP_BASE_NAME)

#define NewFrame(height, width, components) \
	LP3(0x1e, FRAME *, NewFrame, ULONG, height, d0, ULONG, width, d1, UBYTE, components, d2, \
	, PPTSUPP_BASE_NAME)

#define ObtainPreviewFrameA(frame, tags) \
	LP2(0xe4, FRAME *, ObtainPreviewFrameA, FRAME *, frame, a0, struct TagItem *, tags, a1, \
	, PPTSUPP_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ObtainPreviewFrame(a0, tags...) \
	({ULONG _tags[] = { tags }; ObtainPreviewFrameA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define PlanarToChunky(source, dest, width, depth) \
	LP4NR(0x9c, PlanarToChunky, UBYTE **, source, a0, ROWPTR, dest, a1, ULONG, width, d0, UWORD, depth, d1, \
	, PPTSUPP_BASE_NAME)

#define Progress(frame, done) \
	LP2(0x78, BOOL, Progress, FRAME *, frame, a0, ULONG, done, d0, \
	, PPTSUPP_BASE_NAME)

#define PutNPixelRows(frame, buf, row, nRows) \
	LP4NR(0x60, PutNPixelRows, FRAME *, frame, a0, ROWPTR *, buf, a1, WORD, row, d0, UWORD, nRows, d1, \
	, PPTSUPP_BASE_NAME)

#define PutOptions(name, data, len) \
	LP3(0xcc, PERROR, PutOptions, STRPTR, name, a0, APTR, data, a1, ULONG, len, d0, \
	, PPTSUPP_BASE_NAME)

#define PutPixel(frame, row, col, data) \
	LP4NR(0x48, PutPixel, FRAME *, frame, a0, WORD, row, d0, WORD, col, d1, APTR, data, a1, \
	, PPTSUPP_BASE_NAME)

#define PutPixelRow(frame, row, data) \
	LP3NR(0x54, PutPixelRow, FRAME *, frame, a0, WORD, row, d0, APTR, data, a1, \
	, PPTSUPP_BASE_NAME)

#define ReleasePreviewFrame(frame) \
	LP1NR(0xea, ReleasePreviewFrame, FRAME *, frame, a0, \
	, PPTSUPP_BASE_NAME)

#define RemFrame(frame) \
	LP1NR(0x30, RemFrame, FRAME *, frame, a0, \
	, PPTSUPP_BASE_NAME)

#define RemoveExtension(frame, name) \
	LP2(0xde, PERROR, RemoveExtension, FRAME *, frame, a0, STRPTR, name, a1, \
	, PPTSUPP_BASE_NAME)

#define RenderFrame(frame, rastport, location, flags) \
	LP4(0xf0, PERROR, RenderFrame, FRAME *, frame, a0, struct RastPort *, rastport, a1, struct IBox *, location, a2, ULONG, flags, d0, \
	, PPTSUPP_BASE_NAME)

#define SPrintFA(buffer, format, args) \
	LP3(0x108, ULONG, SPrintFA, STRPTR, buffer, a0, STRPTR, format, a1, APTR, args, a2, \
	, PPTSUPP_BASE_NAME)

#define SetErrorCode(frame, error) \
	LP2NR(0x8a, SetErrorCode, FRAME *, frame, a0, PERROR, error, d0, \
	, PPTSUPP_BASE_NAME)

#define SetErrorMsg(frame, errorstring) \
	LP2NR(0x90, SetErrorMsg, FRAME *, frame, a0, UBYTE *, errorstring, a1, \
	, PPTSUPP_BASE_NAME)

#define SetRexxVariable(frame, var, value) \
	LP3(0x102, LONG, SetRexxVariable, FRAME *, frame, a0, STRPTR, var, a1, STRPTR, value, a2, \
	, PPTSUPP_BASE_NAME)

#define StartInput(frame, mid, area) \
	LP3(0xb4, BOOL, StartInput, FRAME *, frame, a0, ULONG, mid, d0, struct PPTMessage *, area, a1, \
	, PPTSUPP_BASE_NAME)

#define StopInput(frame) \
	LP1NR(0xba, StopInput, FRAME *, frame, a0, \
	, PPTSUPP_BASE_NAME)

#define TagData(tagvalue, list) \
	LP2(0xae, ULONG, TagData, Tag, tagvalue, d0, const struct TagItem *, list, a0, \
	, PPTSUPP_BASE_NAME)

#endif /* !_INLINE_PPTSUPP_H */
