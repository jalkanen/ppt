/*
    PROJECT: ppt
    MODULE:  pptplugin.h

    This header file should contain all necessary definitions and includes
    for different compilers for writing PPT plugins.

    At the moment I support only SAS/C. Feel free to send
    me patches if you get it to work with other compilers.

    $Id: pptplugin.h,v 6.1 1999/11/28 18:48:54 jj Exp jj $

*/

#ifndef PPT_PPTPLUGIN_H
#define PPT_PPTPLUGIN_H

/*----------------------------------------------------------------------*/
/*
 *   Compiler specific definitions.  Please put here whatever you
 *   need for the definitions for your compiler.
 */

/* DICE */
#if defined(_DCC)
#define SAVEDS      __geta4
#define ASM
#define REG(x)      __ ## x
#define GREG(x)
#define FAR         __far
#define ALIGNED
#define REGARGS     __regargs
#define STDARGS     __stdargs
#define PUTREG(x,a)
#define GETREG(x)
#define INLINE
#else

/* SAS/C */
# if defined(__SASC)
# define SAVEDS     __saveds
# define ASM        __asm
# define REG(x)     register __ ## x
# define GREG(x)
# define FAR        __far
# define ALIGNED    __aligned
# define REGARGS    __regargs
# define STDARGS    __stdargs
# define PUTREG(x,a) putreg(x,a)
# define GETREG(x)   getreg(x)
# define INLINE     __inline
# define GETDS()    getreg(REG_A4)
# define PUTDS(x)   putreg(REG_A4,x)
# else

/* GCC */
#  if defined(__GNUC__)

#   define SAVEDS   __saveds
#   define ASM
#   define REG(x)   register
#   define GREG(x)  __asm( #x )
#   define FAR
#   define ALIGNED  __aligned
#   define REGARGS  __regargs
#   define STDARGS  __stdargs
#   define PUTREG(x,a) /* These require definitions. */
#   define GETREG(x)
#   define GETDS()  __getds()
#   define PUTDS(x) __putds(x)
#   define INLINE   __inline
    /* add symbol a to list b (type c (22=text 24=data 26=bss)) */
#   define ADD2LIST(a,b,c) asm(".stabs \"_" #b "\"," #c ",0,0,_" #a )

#   undef  __USEOLDEXEC__
#   define __NOLIBBASE__

static unsigned long __inline __getds()
{
    unsigned long result;
    asm("movel a4,%0" : "=d" (result) );
    return result;
}

static void __inline __putds(unsigned long x)
{
    asm("movel %0,a4" :: "d" (x) );
}

#  else
#   error This compiler not yet supported
#  endif /* __GNUC__ */
# endif /* __SASC */
#endif /* _DCC */

/*----------------------------------------------------------------------*/
/*
 *  Debugging definitions.  These are mainly for my own use, but you
 *  might find them useful.
 *
 *  Using SLOW_DEBUG_MODE causes some restrictions on what you might
 *  want to use D(x) macro for, and it also assumes you have an automatic
 *  library opener in your compiler, since it requires dos.library.
 */

#ifdef DEBUG_MODE

# ifdef SLOW_DEBUG_MODE
#  define D(x)  { x; Delay(50); }
# else
#  define D(x)    x;
# endif

#define bug     PDebug
#else
#define D(x)
#define bug     a_function_that_does_not_exist
#endif

/*----------------------------------------------------------------------*/
/*
 *  Start of include files.  Here, you should include whatever you think
 *  is necessary for your compiler.
 */

#include <exec/types.h>

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/bgui.h>

#if defined(__GNUC__)

#include <inline/exec.h>
#include <inline/intuition.h>
#include <inline/utility.h>
#include <inline/dos.h>

# if !defined(_INLINE_BGUI_H)
#  include <inline/bgui.h>
# endif
#endif

#if defined(__SASC)
#include <dos.h>
#endif

/*
 *   These are required, however. Make sure that these are in your include path!
 */

#include <ppt.h>
#include <proto/pptsupp.h>
#include <proto/iomod.h>
#include <proto/effect.h>

/*
 *   Just some extra, again.
 */

#include <stdio.h>
#include <stdarg.h>

/*----------------------------------------------------------------------*/
/*
 *  Declarations - just add your own compiler definitions here.  In
 *  theory, if the above declarations are correct, they should work
 *  with pretty much every compiler.  However, you might want to
 *  consider to do something with LIBINIT and LIBCLEANUP (called
 *  during LibOpen() and LibClose() respectively).
 */

#if defined(__GNUC__)
#define LIBINIT \
    int SAVEDS __UserLibInit( struct Library *EffectBase )

#define LIBCLEANUP \
    VOID SAVEDS __UserLibCleanup( VOID )

#else
#define LIBINIT \
    int SAVEDS ASM __UserLibInit( REG(a6) struct Library *EffectBase )

#define LIBCLEANUP \
    VOID SAVEDS ASM __UserLibCleanup( REG(a6) struct Library *EffectBase )
#endif

#define EFFECTINQUIRE(a,p,e) \
    ULONG SAVEDS ASM LIBEffectInquire( REG(d0) ULONG a GREG(d0), \
                                       REG(a3) struct PPTBase *p GREG(a3), \
                                       REG(a6) struct Library *e GREG(a6))

#define EFFECTEXEC(f,t,p,e) \
    FRAME * SAVEDS ASM LIBEffectExec( REG(a0) FRAME *f GREG(a0), \
                                      REG(a1) struct TagItem *t GREG(a1), \
                                      REG(a3) struct PPTBase *p GREG(a3), \
                                      REG(a6) struct Library *e GREG(a6) )

#define EFFECTGETARGS(f,t,p,e) \
    PERROR SAVEDS ASM LIBEffectGetArgs( REG(a0) FRAME *f GREG(a0), \
                                        REG(a1) struct TagItem *t GREG(a1), \
                                        REG(a3) struct PPTBase *p GREG(a3), \
                                        REG(a6) struct Library *e GREG(a6) )

#define IOINQUIRE(a,p,i) \
    ULONG SAVEDS ASM LIBIOInquire( REG(d0) ULONG a GREG(d0), \
                                   REG(a3) struct PPTBase *p GREG(a3), \
                                   REG(a6) struct Library *i GREG(a6))

#define IOLOAD(fh,frame,tags,p,i) \
    PERROR SAVEDS ASM LIBIOLoad( REG(d0) BPTR fh GREG(d0), \
                                 REG(a0) FRAME *frame GREG(a0), \
                                 REG(a1) struct TagItem *tags GREG(a1), \
                                 REG(a3) struct PPTBase *p GREG(a3), \
                                 REG(a6) struct Library *i GREG(a6))

#define IOSAVE(fh,format,frame,tags,p,i) \
    PERROR SAVEDS ASM LIBIOSave( REG(d0) BPTR fh GREG(d0), \
                                 REG(d1) ULONG format GREG(d1), \
                                 REG(a0) FRAME *frame GREG(a0), \
                                 REG(a1) struct TagItem *tags GREG(a1), \
                                 REG(a3) struct PPTBase *p GREG(a3), \
                                 REG(a6) struct Library *i GREG(a6))

#define IOCHECK(fh,len,buf,p,i) \
    BOOL SAVEDS ASM LIBIOCheck( REG(d0) BPTR fh GREG(d0), \
                                REG(d1) LONG len GREG(d1), \
                                REG(a0) UBYTE *buf GREG(a0), \
                                REG(a3) struct PPTBase *p GREG(a3), \
                                REG(a6) struct Library *i GREG(a6))

#define IOGETARGS(format,frame,tags,p,i) \
    PERROR SAVEDS ASM LIBIOGetArgs( REG(d1) ULONG format GREG(d1), \
                                    REG(a0) FRAME *frame GREG(a0), \
                                    REG(a1) struct TagItem *tags GREG(a1), \
                                    REG(a3) struct PPTBase *p GREG(a3), \
                                    REG(a6) struct Library *i GREG(a6))


/*---------------------------------------------------------------------*/
/*
 *   Need to recreate some bgui macros to use my own tag routines. Don't worry,
 *   you might not need these.
 */

#if defined(REDEFINE_BGUI)
#undef  HGroupObject
#define HGroupObject          MyNewObject( PPTBase, BGUI_GROUP_GADGET
#undef  VGroupObject
#define VGroupObject          MyNewObject( PPTBase, BGUI_GROUP_GADGET, GROUP_Style, GRSTYLE_VERTICAL
#undef  ButtonObject
#define ButtonObject          MyNewObject( PPTBase, BGUI_BUTTON_GADGET
#undef  CheckBoxObject
#define CheckBoxObject        MyNewObject( PPTBase, BGUI_CHECKBOX_GADGET
#undef  WindowObject
#define WindowObject          MyNewObject( PPTBase, BGUI_WINDOW_OBJECT
#undef  SeperatorObject
#define SeperatorObject       MyNewObject( PPTBase, BGUI_SEPERATOR_GADGET
#undef  WindowOpen
#define WindowOpen(wobj)      (struct Window *)DoMethod( wobj, WM_OPEN )
#endif

#endif /* PPT_PPTPLUGIN_H */
