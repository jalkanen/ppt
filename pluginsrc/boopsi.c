
#include <exec/types.h>
#include <clib/alib_protos.h>

__asm ULONG m68kDoMethodA(register __a0 Object *obj, register __a1 Msg msg)
{
    return DoMethodA(obj,msg);
}

