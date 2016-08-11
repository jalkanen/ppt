#ifndef PRAGMAS_EFFECT_PRAGMAS_H
#define PRAGMAS_EFFECT_PRAGMAS_H

#ifndef CLIB_EFFECT_PROTOS_H
#include <clib/effect_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(EffectBase,0x01E,EffectInquire(d0,a3))
#pragma amicall(EffectBase,0x024,EffectExec(a0,a1,a3))
#pragma amicall(EffectBase,0x02A,EffectGetArgs(a0,a1,a3))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma libcall EffectBase EffectInquire        01E B002
#pragma libcall EffectBase EffectExec           024 B9803
#pragma libcall EffectBase EffectGetArgs        02A B9803
#endif
#ifdef __STORM__
#pragma tagcall(EffectBase,0x02A,EffectGet(a0,a1,a3))
#endif
#ifdef __SASC_60
#pragma tagcall EffectBase EffectGet            02A B9803
#endif

#endif	/*  PRAGMAS_EFFECT_PRAGMA_H  */