#ifndef PROTO_PPTSUPP_H
#define PROTO_PPTSUPP_H

#include <clib/pptsupp_protos.h>

#ifdef __PPC__
# include <ppcpragmas/pptsupp_pragmas.h>
#else
# ifdef __GNUC__
#  include <inline/pptsupp.h>
# else
#  include <pragmas/pptsupp_pragmas.h>
# endif
#endif

#endif
