#ifndef PRAGMAS_MODULE_PRAGMAS_H
#define PRAGMAS_MODULE_PRAGMAS_H

#ifndef CLIB_MODULE_PROTOS_H
#include <clib/module_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(ModuleBase,0x01E,Inquire(d0,a3))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma libcall ModuleBase Inquire              01E B002
#endif
#ifdef __STORM__
#endif
#ifdef __SASC_60
#endif

#endif	/*  PRAGMAS_MODULE_PRAGMA_H  */