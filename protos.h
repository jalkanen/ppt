/*
    Hand - made prototypes for PPT

    $Id: protos.h,v 1.22 1999/02/21 20:31:44 jj Exp $
*/

#ifndef PROTOS_H
#define PROTOS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif

#ifndef PPT_H
#include <ppt.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef LIBRARIES_AMIGAGUIDE_H
#include <libraries/amigaguide.h>
#endif

/* render2.a */

extern ASM ULONG RGBDistance( REG(d0) ULONG , REG(d1) ULONG , REG(d2) ULONG  );
extern ASM UWORD BestMatchPen4( REG(a0) UWORD *, REG(d4) UWORD, REG(d0) UBYTE, REG(d1) UBYTE, REG(d2) UBYTE );
extern ASM UWORD BestMatchPen8( REG(a0) COLORMAP *, REG(d4) UWORD, REG(d0) UBYTE, REG(d1) UBYTE, REG(d2) UBYTE );
extern ASM ULONG Plane2Chunk( REG(a0) UBYTE **planes, REG(a1) UBYTE *dest, REG(d0) UWORD width, REG(d1) UBYTE depth );

/* Global variables */

#pragma msg 72 ignore
extern struct ExecBase      *SysBase;
extern struct DosLibrary    *DOSBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Library       *UtilityBase, *BGUIBase, *AslBase, *CyberGfxBase;
extern struct GfxBase       *GfxBase;
#pragma msg 72 warn
extern GLOBALS              *globals;
extern EXTBASE              *globxd;
extern struct FontRequester *fontreq;
extern struct FileRequester *filereq;

extern struct ExtInfoWin    extl, exts, extf;
extern struct ToolWindow    toolw;
extern struct FramesWindow  framew;
extern struct PrefsWindow   prefsw;
extern struct SelectWindow  selectw;
extern struct PPTFileRequester gvLoadFileReq, gvPaletteOpenReq, gvPaletteSaveReq;

extern Class                *DropAreaClass, *PaletteClass, *RenderAreaClass, *ToolbarClass;

extern BOOL                 MasterQuit;

extern struct MsgPort       *MainIDCMPPort, *AppIconPort;

extern struct Hook          HelpHook;
extern AMIGAGUIDECONTEXT    helphandle;

extern const UBYTE *        ColorSpaceNames[];

extern FRAME                *clipframe;

extern struct ToolbarItem   ToolbarList[];

#endif /* PROTOS_H */
