#ifndef PROTO_MODULE_H
#define PROTO_MODULE_H

#include "clib/module_protos.h"

#ifdef __GNUC__
# include "inline/module.h"
#else
# include "pragmas/module_pragmas.h"
#endif
#endif
