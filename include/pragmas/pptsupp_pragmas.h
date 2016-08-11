#ifndef PRAGMAS_PPTSUPP_PRAGMAS_H
#define PRAGMAS_PPTSUPP_PRAGMAS_H

#ifndef CLIB_PPTSUPP_PROTOS_H
#include <clib/pptsupp_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(PPTBase,0x01E,NewFrame(d0,d1,d2))
#pragma amicall(PPTBase,0x024,MakeFrame(a0))
#pragma amicall(PPTBase,0x02A,InitFrame(a0))
#pragma amicall(PPTBase,0x030,RemFrame(a0))
#pragma amicall(PPTBase,0x036,DupFrame(a0,d0))
#pragma amicall(PPTBase,0x03C,FindFrame(d0))
#pragma amicall(PPTBase,0x042,GetPixel(a0,d0,d1))
#pragma amicall(PPTBase,0x048,PutPixel(a0,d0,d1,a1))
#pragma amicall(PPTBase,0x04E,GetPixelRow(a0,d0))
#pragma amicall(PPTBase,0x054,PutPixelRow(a0,d0,a1))
#pragma amicall(PPTBase,0x05A,GetNPixelRows(a0,a1,d0,d1))
#pragma amicall(PPTBase,0x060,PutNPixelRows(a0,a1,d0,d1))
#pragma amicall(PPTBase,0x066,GetBitMapRow(a0,d0))
#pragma amicall(PPTBase,0x072,InitProgress(a0,a1,d0,d1))
#pragma amicall(PPTBase,0x078,Progress(a0,d0))
#pragma amicall(PPTBase,0x07E,FinishProgress(a0))
#pragma amicall(PPTBase,0x084,ClearProgress(a0))
#pragma amicall(PPTBase,0x08A,SetErrorCode(a0,d0))
#pragma amicall(PPTBase,0x090,SetErrorMsg(a0,a1))
#pragma amicall(PPTBase,0x096,AskReqA(a0,a1))
#pragma amicall(PPTBase,0x09C,PlanarToChunky(a0,a1,d0,d1))
#pragma amicall(PPTBase,0x0A8,GetStr(a0))
#pragma amicall(PPTBase,0x0AE,TagData(d0,a0))
#pragma amicall(PPTBase,0x0B4,StartInput(a0,d0,a1))
#pragma amicall(PPTBase,0x0BA,StopInput(a0))
#pragma amicall(PPTBase,0x0C0,GetBackgroundColor(a0,a1))
#pragma amicall(PPTBase,0x0C6,GetOptions(a0))
#pragma amicall(PPTBase,0x0CC,PutOptions(a0,a1,d0))
#pragma amicall(PPTBase,0x0D2,AddExtension(a0,a1,a2,d0,d1))
#pragma amicall(PPTBase,0x0D8,FindExtension(a0,a1))
#pragma amicall(PPTBase,0x0DE,RemoveExtension(a0,a1))
#pragma amicall(PPTBase,0x0E4,ObtainPreviewFrameA(a0,a1))
#pragma amicall(PPTBase,0x0EA,ReleasePreviewFrame(a0))
#pragma amicall(PPTBase,0x0F0,RenderFrame(a0,a1,a2,d0))
#pragma amicall(PPTBase,0x0F6,CopyFrameData(a0,a1,d0))
#pragma amicall(PPTBase,0x0FC,CloseProgress(a0))
#pragma amicall(PPTBase,0x102,SetRexxVariable(a0,a1,a2))
#pragma amicall(PPTBase,0x108,SPrintFA(a0,a1,a2))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma libcall PPTBase NewFrame             01E 21003
#pragma libcall PPTBase MakeFrame            024 801
#pragma libcall PPTBase InitFrame            02A 801
#pragma libcall PPTBase RemFrame             030 801
#pragma libcall PPTBase DupFrame             036 0802
#pragma libcall PPTBase FindFrame            03C 001
#pragma libcall PPTBase GetPixel             042 10803
#pragma libcall PPTBase PutPixel             048 910804
#pragma libcall PPTBase GetPixelRow          04E 0802
#pragma libcall PPTBase PutPixelRow          054 90803
#pragma libcall PPTBase GetNPixelRows        05A 109804
#pragma libcall PPTBase PutNPixelRows        060 109804
#pragma libcall PPTBase GetBitMapRow         066 0802
#pragma libcall PPTBase InitProgress         072 109804
#pragma libcall PPTBase Progress             078 0802
#pragma libcall PPTBase FinishProgress       07E 801
#pragma libcall PPTBase ClearProgress        084 801
#pragma libcall PPTBase SetErrorCode         08A 0802
#pragma libcall PPTBase SetErrorMsg          090 9802
#pragma libcall PPTBase AskReqA              096 9802
#pragma libcall PPTBase PlanarToChunky       09C 109804
#pragma libcall PPTBase GetStr               0A8 801
#pragma libcall PPTBase TagData              0AE 8002
#pragma libcall PPTBase StartInput           0B4 90803
#pragma libcall PPTBase StopInput            0BA 801
#pragma libcall PPTBase GetBackgroundColor   0C0 9802
#pragma libcall PPTBase GetOptions           0C6 801
#pragma libcall PPTBase PutOptions           0CC 09803
#pragma libcall PPTBase AddExtension         0D2 10A9805
#pragma libcall PPTBase FindExtension        0D8 9802
#pragma libcall PPTBase RemoveExtension      0DE 9802
#pragma libcall PPTBase ObtainPreviewFrameA  0E4 9802
#pragma libcall PPTBase ReleasePreviewFrame  0EA 801
#pragma libcall PPTBase RenderFrame          0F0 0A9804
#pragma libcall PPTBase CopyFrameData        0F6 09803
#pragma libcall PPTBase CloseProgress        0FC 801
#pragma libcall PPTBase SetRexxVariable      102 A9803
#pragma libcall PPTBase SPrintFA             108 A9803
#endif
#ifdef __STORM__
#pragma tagcall(PPTBase,0x096,AskReq(a0,a1))
#pragma tagcall(PPTBase,0x0E4,ObtainPreviewFrame(a0,a1))
#pragma tagcall(PPTBase,0x108,SPrintF(a0,a1,a2))
#endif
#ifdef __SASC_60
#pragma tagcall PPTBase AskReq               096 9802
#pragma tagcall PPTBase ObtainPreviewFrame   0E4 9802
#pragma tagcall PPTBase SPrintF              108 A9803
#endif

#endif	/*  PRAGMAS_PPTSUPP_PRAGMA_H  */