/*
    Hand - made prototypes for PPT

    $Id: protos.h,v 1.23 1999/03/31 13:23:50 jj Exp $
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

#include <utility/utility.h> /* UtilityBase */

/* render2.a */

extern ULONG ASM RGBDistance( REGDECL(d0,ULONG), REGDECL(d1,ULONG), REGDECL(d2,ULONG)  );
extern UWORD ASM BestMatchPen4( REGDECL(a0,UWORD *), REGDECL(d4,UWORD), REGDECL(d0,UBYTE), REGDECL(d1,UBYTE), REGDECL(d2,UBYTE) );
extern UWORD ASM BestMatchPen8( REGDECL(a0,COLORMAP *), REGDECL(d4,UWORD), REGDECL(d0,UBYTE), REGDECL(d1,UBYTE), REGDECL(d2,UBYTE) );
extern ULONG ASM Plane2Chunk( REGDECL(a0,UBYTE **), REGDECL(a1,UBYTE *), REGDECL(d0,UWORD), REGDECL(d1,UBYTE) );

/* Global variables */

#pragma msg 72 ignore
extern struct ExecBase      *SysBase;
extern struct DosLibrary    *DOSBase;
extern struct IntuitionBase *IntuitionBase;
extern struct UtilityBase   *UtilityBase;
extern struct Library       *BGUIBase, *AslBase, *CyberGfxBase;
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
