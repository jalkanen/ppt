
#include "ppt.h"
#include "proto/pptsupp.h"

#define SAVEDS __saveds
#define ASM    __asm
#define REG(x) register __ ## x

extern struct TagItem *_MyTagArray;
extern void *_LIBEffectExec;
extern void *_LIBEffectInquire;
extern long CallPPCFunction(void *, ...);

SAVEDS ASM ULONG EffectInquire( REG(d0) ULONG attr, REG(a5) EXTBASE *ExtBase )
{
    return TagData( attr, _MyTagArray );
}

SAVEDS ASM FRAME *EffectExec( REG(a0) FRAME *frame,
                              REG(a1) struct TagItem *tags,
                              REG(a5) EXTBASE *ExtBase )

{
    return (FRAME *)CallPPCFunction(_LIBEffectExec,frame,tags,ExtBase);
}

