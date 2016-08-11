/* Automatically generated header! Do not edit! */

#ifndef _INLINE_MODULE_H
#define _INLINE_MODULE_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef MODULE_BASE_NAME
#define MODULE_BASE_NAME ModuleBase
#endif /* !MODULE_BASE_NAME */

#define Inquire(attribute, pptbase) \
	LP2(0x1e, ULONG, Inquire, ULONG, attribute, d0, EXTBASE *, pptbase, a3, \
	, MODULE_BASE_NAME)

#endif /* !_INLINE_MODULE_H */
