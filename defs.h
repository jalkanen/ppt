/*
    PROJECT : ppt
    MODULE  : defs.h

    $Id: defs.h,v 1.27 1998/10/14 20:31:21 nobody Exp $

    Main include files and some definitions.
    Everything in here should be constant and not subject to much change.
 */

#ifndef DEFS_H
#define DEFS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef GRAPHICS_MODEID_H
#include <graphics/modeid.h>    /* Should not really include this*/
#endif

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#ifndef PROTO_BGUI_H
#include <proto/bgui.h>
#endif

#ifndef PROTO_EXEC_H
#define __USE_SYSBASE
#include <proto/exec.h>
#endif

#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif

#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/rexxsyslib.h>
#include <proto/asl.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>


/*------------------------------------------------------------------*/
/* Useful macros for my own use */

#ifdef DEBUG_MODE
#define D(x)    x;
#define L(x)    x;
#define bug     DEBUG
#else
#define D(x)    ;
#define L(x)    ;
#endif

#define GD(x)   /* Define to (x) to activate debugging for globals */

/* These three macros exist for simplicity. They are used
   for locking the global base. */

#define LOCKGLOB()    \
    { \
        GD(bug("\t++ LOCK(globals)\n")); \
        ObtainSemaphore( &globals->phore ); \
    }

#define SHLOCKGLOB()  \
    { \
        GD(bug("\t++ SHLOCK(globals)\n")); \
        ObtainSemaphoreShared( &globals->phore ); \
    }

#define UNLOCKGLOB()  \
    { \
        ReleaseSemaphore( &globals->phore ); \
        GD(bug("\t-- UNLOCK(globals)\n")); \
    }

#define LOCK(x)       \
    { \
        L(bug("\t++ LOCK(%08X)\n", x )); \
        ObtainSemaphore( &((x)->lock) ); \
    }

#define SHLOCK(x)     \
    { \
        L(bug("\t++ SHLOCK(%08X)\n", x )); \
        ObtainSemaphoreShared( &((x)->lock) ); \
    }

#define UNLOCK(x)     \
    { \
        ReleaseSemaphore( &((x)->lock) ); \
        L(bug("\t-- UNLOCK(%08X)\n", x )); \
    }

/*
 *  Quiet versions of the same lock-system.  These will never
 *  generate the locking messages.
 */

#define QLOCK(x)    ObtainSemaphore( &((x)->lock) )
#define QSHLOCK(x)  ObtainSemaphoreShared( &((x)->lock) )
#define QUNLOCK(x)  ReleaseSemaphore( &((x)->lock) )

/*
 *  Some generic macros for typecasting
 */

#define STR(x) ((STRPTR)(x))

/*
 *  Some macros for common variables
 */

#define MAINWIN       (globals->maindisp->win)
#define MAINSCR       (globals->maindisp->scr)
#define USERPREFS     (globals->userprefs)
#define DOCONFIRM     (globals->userprefs->confirm)

#undef TMPBUF_SUPPORTED

/*
 *  Useful version checking macros
 */

#define GFXV39              ( GfxBase->LibNode.lib_Version >= 39 )
#define IS_AGA              (((struct GfxBase *)GfxBase)->ChipRevBits0 & GFXF_AA_ALICE)
#define SYSV39              ( SysBase->LibNode.lib_Version >= 39 )

/*------------------------------------------------------------------*/
/* Internal type definitions, which are not in ppt.h */

/*
    This is for OpenDebugFile()
*/

enum DebugFile_T {
    DFT_Load = 0,
    DFT_Save,
    DFT_Effect,
    DFT_Render,
    DFT_Main
};



/*------------------------------------------------------------------*/
/* Mainly buffer constants. You shouldn't need to change. */

#define EXALLBUFSIZE        2048    /* for ExAll() in load.c */
#define PPNCBUFSIZE         40      /* for ParsePatternNoCase() */
#define MIN_VMBUFSIZ        96      /* in Kb. 32 (max pic width) * 3 (bytes/pixel) */
#define ERRBUFLEN           80      /* Max no of characters allowed in an error message
                                       from an external module. See PPTX_ErrMsg. */
#define ARGBUF_SIZE         256     /* Length of frame->argbuf */
#define DEFAULT_EXTSTACK    10000   /* Size of external processes stacks */
#define INIT_BYTES          16      /* How many bytes are read from a file
                                       and passed down to the Check() routine
                                       of a loader. */
/*
 *  These two are for the PPT internal memory allocation scheme.
 */

#define POOL_PUDDLESIZE     8192
#define POOL_THRESHSIZE     8192

/*------------------------------------------------------------------*/
/* Internal defaults. These you might want to change. But they can
   be changed from preferences anyway. */

#define DEFAULT_PREFS_FILE  "ENVARC:PPT.prefs" /* BUG: A bad place! */
#define DEFAULT_VM_DIR      "T:"
#define DEFAULT_VM_BUFSIZ   120     /* In Kb */
#define DEFAULT_MAXUNDO     4
#define DEFAULT_MODULEPATH  "modules"
#define DEFAULT_REXXPATH    "rexx"

#ifdef  DEBUG_MODE
#define DEFAULT_STARTUPDIR  "Data:Gfx/Pics"
#else
#define DEFAULT_STARTUPDIR  ""
#endif

#define DEFAULT_STARTUPFILE ""
#define DEFAULT_EXTNICEVAL  10
#define DEFAULT_EXTPRIORITY -1

#define DEFAULT_PROGRESS_FILESIZE 256000 /* Bytes, before a progress display is shown */
#define DEFAULT_PROGRESS_STEP 40

/* Main window defaults */
#define DEFAULT_SCRDEPTH    4       /* Make a 16 color screen by default */
#define DEFAULT_SCRHEIGHT   200
#define DEFAULT_SCRWIDTH    640
#define DEFAULT_DISPID      HIRES_KEY

#define DEFAULT_PREVIEWMODE PWMODE_MEDIUM

#define DEFAULT_CONFIRM     TRUE

#define VM_FILENAME         "PPT_VM_FILE"
#define PPTPUBSCREENNAME    "PPT"

/* This gives the amount of days after which PPT starts to complain
   about expiration */
#define NAG_PERIOD          (2*30)  /* In days */

/*------------------------------------------------------------------*/
/* Miscallaneous constants. */

#define VM_SAFEBOUNDARY     128     /* If hit happens this close to boundaries, then reload file. */
#define FONT_MAXHEIGHT      30      /* Won't allow fonts bigger than this.*/
#define BGUI_VERSION_REQUIRED 41L

/*------------------------------------------------------------------*/
/* Miscallaneous flags */

/*
    frame->selstatus flag definitions. Should probably be named
    frame->flags.
*/
#define SELF_RECTANGLE      0x01
#define SELF_BUTTONDOWN     0x02
#define SELF_CONTROLDOWN    0x04


/*
    Flags for AddExtEntries()
*/

#define AEE_SAVECM          0x00000001  /* Colormapped */
#define AEE_SAVETC          0x00000002  /* Truecolor */
#define AEE_LOAD            0x00000004  /* Loaders */

#define AEE_SAVE            (AEE_SAVECM | AEE_SAVETC)
#define AEE_ALL             ~0


/*
    Flags for DrawSelectBox()
 */

#define DSBF_INTERIM        (1<<0)
#define DSBF_FIXEDRECT      (1<<1)

/*------------------------------------------------------------------*/
/* DICE prototype stuff */

#define Prototype           extern
#define Local               static

/*------------------------------------------------------------------*/
/* Compatability stuff */


#ifdef _DCC
#define SAVEDS __geta4
#define ASM
#define REG(x) __ ## x
#else
#ifdef __GCC__
#define SAVEDS
#define ASM
#define REG(x)
#else
#define SAVEDS __saveds
#define ASM    __asm
#define REG(x) register __ ## x
#endif
#endif

#ifndef PPT_H
#include "ppt_real.h"
#endif

#ifndef GUI_H
#include "gui.h"
#endif

#ifndef RENDER_H
#include "render.h"
#endif

#ifndef REXX_H
#include "rexx.h"
#endif

#ifndef ppt_CAT_H
#include "ppt_cat.h"
#endif

#include "protos.h"         /* Created by hand */
#include "machine-protos.h" /* Automagically created */

#endif /* DEFS_H */

