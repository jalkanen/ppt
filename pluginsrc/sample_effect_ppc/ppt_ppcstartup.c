#define DEBUG_MODE
#include <exec/types.h>
ULONG _start(int function, ULONG arg1, ULONG arg2, ULONG arg3 );
ULONG ___main(int function, ULONG arg1, ULONG arg2, ULONG arg3 )
{
    if( function & 0x80000000 )
        return _start(function,arg1,arg2,arg3);
    else
        return 20;
}

/*
 *  A custom-built SAS/C startup code for the SC PPC compiler
 *  and PPT.  Does auto-initialization and other stuff, but
 *  does not parse the command line.  Dies, if started from
 *  command line.
 *
 *  $Id: ppt_ppcstartup.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
 */

#ifdef DEBUG_MODE
#define D(x) x
#define bug  printf
#else
#define D(x)
#define bug  function_that_does_not_exist
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <constructor.h>
#include <workbench/startup.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <exec/memory.h>
#include <powerup/gcclib/powerup_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <ppt.h>

extern FRAME *LIBEffectExec( FRAME *, struct TagItem *, EXTBASE * );
extern ULONG LIBEffectInquire( ULONG, EXTBASE * );
extern int __UserLibInit( struct Library * );
extern void __UserLibCleanup( struct Library * );

struct CTDT {
    long priority;
    int (*fp)(void);
};

static __stdargs struct CTDT *sort_ctdt(struct CTDT **last);

static jmp_buf __exit_jmpbuf;
static int __exit_return;
struct DosLibrary *DOSBase;
struct ExecBase *SysBase;

long __PPC_SHELL_START;   /* special symbol so P5 patch knows it's ok to load this */

BPTR __curdir;

/*
 *  gpr3 = frame, gpr4 = taglist, gpr5 = extbase
 */

ULONG _start(int function, ULONG arg1, ULONG arg2, ULONG arg3 )
{
    ULONG ret;
    struct CTDT *ctdt, *last_ctdt;

    SysBase = *(struct ExecBase **)4;
    DOSBase = (void *)OpenLibrary("dos.library",0);
    if (DOSBase == NULL) return NULL;

    D(bug("_start( function=%08X, arg1=%lu, arg2=%lu, arg3=%lu\n",
          function, arg1, arg2, arg3 ));

    /* grab the current directory */
    __curdir = CurrentDir(0);
    CurrentDir(__curdir);

   /* LD merges all the contructors and destructors together in random */
   /* order. All destructors are negated, and then an unsigned sort  */
   /* is preformed, so the constructors come out first, followed by */
   /* the destructors in reverse order */

   ctdt = sort_ctdt(&last_ctdt);

   while (ctdt < last_ctdt && ctdt->priority >= 0)
   {
       if (ctdt->fp() != 0)
       {
           /* skip the remaining constructors */
           while (ctdt < last_ctdt && ctdt->priority >= 0)
              ctdt++;
           ret = NULL;
           goto cleanup;
       }
       ctdt++;
   }

/***
*     Call user's main program
***/

/* We I get setjmp ported, I'll do a setjmp here, then */
/* have exit() do a longjmp back, and return            */
if ((ret = setjmp(__exit_jmpbuf)) == 0)
   {
        if( __UserLibInit( NULL ) == 0 ) {
            switch( function ) {
                case 0x80000000:
                    ret = (ULONG)LIBEffectExec( (FRAME*)arg1, (struct TagItem *)arg2, (EXTBASE *)arg3 );
                    break;
                case 0x80000001:
                    ret = LIBEffectInquire( arg1, (EXTBASE *) arg3 );
                    break;
                default:
                    ret = NULL;
            }
        }
        __UserLibCleanup( NULL );
        exit( (int) ret );
   }
else ret = __exit_return;


cleanup:
   /* call destructors here */
   while (ctdt < last_ctdt)
   {
      ctdt->fp();
      ctdt++;
   }

   CloseLibrary((void *)DOSBase);

   return ret;
}


void __stdargs _XCEXIT(long d0)
{
    /* this will longjmp back to main when longjmp is ready */
    __exit_return = d0;
    longjmp(__exit_jmpbuf, 1);
}


int __stdargs _STI_0_dummy(void)
{
    /* dummy constructor there is something in the modules .ctdt section */
    return 0;
}

struct __stdargs CTDT *get_last_ctdt(void);

static int __stdargs comp_ctdt(struct CTDT *a, struct CTDT *b)
{
    if (a->priority == b->priority) return 0;
    if ((unsigned long)a->priority < (unsigned long) b->priority) return -1;
    return 1;    
}

static __stdargs struct CTDT *sort_ctdt(struct CTDT **last)
{
    extern void *__builtin_getsectionaddr(int);
    struct CTDT *ctdt;
    struct CTDT *last_ctdt;
    
    ctdt = __builtin_getsectionaddr(4);  /* the ctdt section is pointed to by sym 4 */;

    last_ctdt = get_last_ctdt();         /* from end.o */
    
    qsort(ctdt, last_ctdt - ctdt, sizeof(*ctdt), comp_ctdt);
    
    *last = last_ctdt;
    
    return ctdt;
}
    
