/* Automatically generated header! Do not edit! */

#ifndef _INLINE_IOMOD_H
#define _INLINE_IOMOD_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef IOMOD_BASE_NAME
#define IOMOD_BASE_NAME IOModuleBase
#endif /* !IOMOD_BASE_NAME */

#define IOCheck(handle, num_bytes, buffer, pptbase) \
	LP4(0x24, BOOL, IOCheck, BPTR, handle, d0, LONG, num_bytes, d1, UBYTE *, buffer, a0, struct PPTBase *, pptbase, a3, \
	, IOMOD_BASE_NAME)

#define IOGetArgs(format, frame, tags, pptbase) \
	LP4(0x36, PERROR, IOGetArgs, ULONG, format, d1, FRAME *, frame, a0, struct TagItem *, tags, a1, struct PPTBase *, pptbase, a3, \
	, IOMOD_BASE_NAME)

#define IOInquire(attribute, pptbase) \
	LP2(0x1e, ULONG, IOInquire, ULONG, attribute, d0, struct PPTBase *, pptbase, a3, \
	, IOMOD_BASE_NAME)

#define IOLoad(handle, frame, tags, pptbase) \
	LP4(0x2a, PERROR, IOLoad, BPTR, handle, d0, FRAME *, frame, a0, struct TagItem *, tags, a1, struct PPTBase *, pptbase, a3, \
	, IOMOD_BASE_NAME)

#define IOSave(handle, format, frame, tags, pptbase) \
	LP5(0x30, PERROR, IOSave, BPTR, handle, d0, ULONG, format, d1, FRAME *, frame, a0, struct TagItem *, tags, a1, struct PPTBase *, pptbase, a3, \
	, IOMOD_BASE_NAME)

#endif /* !_INLINE_IOMOD_H */
