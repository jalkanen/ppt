
#include "ppcdispatch.h"
#include "pptplugin.h"

#define SAVEDS __saveds
#define ASM    __asm
#define REG(x) register __ ## x

extern struct TagItem *_MyTagArray;
extern void *_LIBEffectExec;
extern void *_LIBEffectInquire;

EFFECTINQUIRE(attr,PPTBase,ModuleBase)
{
    return TagData( attr, _MyTagArray );
}

EFFECTEXEC(frame,tags,PPTBase,ModuleBase)
{
    return (FRAME *)CallPPCFunction(_LIBEffectExec,frame,tags,PPTBase);
}

