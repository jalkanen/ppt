#ifndef PRAGMAS_IOMOD_PRAGMAS_H
#define PRAGMAS_IOMOD_PRAGMAS_H

#ifndef CLIB_IOMOD_PROTOS_H
#include <clib/iomod_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(IOModuleBase,0x01E,IOInquire(d0,a3))
#pragma amicall(IOModuleBase,0x024,IOCheck(d0,d1,a0,a3))
#pragma amicall(IOModuleBase,0x02A,IOLoad(d0,a0,a1,a3))
#pragma amicall(IOModuleBase,0x030,IOSave(d0,d1,a0,a1,a3))
#pragma amicall(IOModuleBase,0x036,IOGetArgs(d1,a0,a1,a3))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma libcall IOModuleBase IOInquire            01E B002
#pragma libcall IOModuleBase IOCheck              024 B81004
#pragma libcall IOModuleBase IOLoad               02A B98004
#pragma libcall IOModuleBase IOSave               030 B981005
#pragma libcall IOModuleBase IOGetArgs            036 B98104
#endif
#ifdef __STORM__
#pragma tagcall(IOModuleBase,0x036,IOGet(d1,a0,a1,a3))
#endif
#ifdef __SASC_60
#pragma tagcall IOModuleBase IOGet                036 B98104
#endif

#endif	/*  PRAGMAS_IOMOD_PRAGMA_H  */