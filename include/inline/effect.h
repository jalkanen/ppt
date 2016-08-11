/* Automatically generated header! Do not edit! */

#ifndef _INLINE_EFFECT_H
#define _INLINE_EFFECT_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef EFFECT_BASE_NAME
#define EFFECT_BASE_NAME EffectBase
#endif /* !EFFECT_BASE_NAME */

#define EffectExec(frame, tags, pptbase) \
	LP3(0x24, FRAME *, EffectExec, FRAME *, frame, a0, struct TagItem *, tags, a1, struct PPTBase *, pptbase, a3, \
	, EFFECT_BASE_NAME)

#define EffectGetArgs(frame, tags, pptbase) \
	LP3(0x2a, PERROR, EffectGetArgs, FRAME *, frame, a0, struct TagItem *, tags, a1, struct PPTBase *, pptbase, a3, \
	, EFFECT_BASE_NAME)

#define EffectInquire(attribute, pptbase) \
	LP2(0x1e, ULONG, EffectInquire, ULONG, attribute, d0, struct PPTBase *, pptbase, a3, \
	, EFFECT_BASE_NAME)

#endif /* !_INLINE_EFFECT_H */
