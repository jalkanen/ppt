#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>

#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>
#include <graphics/scale.h>
#include <graphics/gfxbase.h>
#include <gadgets/colorwheel.h>
#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/bgui.h>
#include <proto/utility.h>

#include <string.h>
#include <stdio.h>

#include "colorgadget.h"

#ifdef _DCC
#define SAVEDS __geta4
#define ASM
#define REG(x) __ ## x
#define REGARGS __regargs
#else
#define SAVEDS __saveds
#define ASM __asm
#define REG(x) register __ ## x
#define REGARGS __regargs
#endif

#define OSVERSION(ver)  GfxBase->LibNode.lib_Version >= (ver)
#define GAD(x)      (( struct Gadget * )x)

Object *BuildColorGadget(ULONG idbase)
{
    Object *obj;

    obj = VGroupObject,
         StartMember,
             HGroupObject,
                 StartMember,
                     md->Red = SliderObject,
                         Label("R"),
                         SLIDER_Min, 0,
                         SLIDER_Max, 255,
                         SLIDER_Level, (md->rgb.cw_Red >> 24),
                     EndObject,
                 EndMember,
                 StartMember,
                     md->RedI = StringObject,
                         STRINGA_IntegerMin, 0,
                         STRINGA_IntegerMax, 255,
                         STRINGA_LongVal, (md->rgb.cw_Red >> 24),
                     EndObject,
                 EndMember,
             EndObject,
         EndMember,
         StartMember,
             HGroupObject,
                 StartMember,
                     md->Green = SliderObject,
                         Label("G"),
                         SLIDER_Min, 0,
                         SLIDER_Max, 255,
                         SLIDER_Level, (md->rgb.cw_Red >> 24),
                     EndObject,
                 EndMember,
                 StartMember,
                     md->GreenI = StringObject,
                         STRINGA_IntegerMin, 0,
                         STRINGA_IntegerMax, 255,
                         STRINGA_LongVal, (md->rgb.cw_Red >> 24),
                     EndObject,
                 EndMember,
             EndObject,
         EndMember,
         StartMember,
             HGroupObject,
                 StartMember,
                     md->Blue = SliderObject,
                         Label("B"),
                         SLIDER_Min, 0,
                         SLIDER_Max, 255,
                         SLIDER_Level, (md->rgb.cw_Red >> 24),
                     EndObject,
                 EndMember,
                 StartMember,
                     md->BlueI = StringObject,
                         STRINGA_IntegerMin, 0,
                         STRINGA_IntegerMax, 255,
                         STRINGA_LongVal, (md->rgb.cw_Red >> 24),
                     EndObject,
                 EndMember,
             EndObject,
         EndMember,
     EndObject;

}

