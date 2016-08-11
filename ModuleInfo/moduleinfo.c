/*
   This extracts info out of a PPT module and prints it.

   BUG: Does not handle all tags.

   $Id: moduleinfo.c 1.11 1999/11/28 17:40:33 jj Exp jj $
 */

/*--------------------------------------------------------------------------*/

/*
 *  This tells which level of PPT calls we're emulating at some level
 *  or another.
 */

#define PPT_VERSION_SUPPORTED   6

/*
 *  The number of library functions we recognise.
 */

#define XLIB_FUNCS 44

/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <stdlib.h>

#include <exec/execbase.h>

#include <pragmas/pptsupp_pragmas.h>
#include <pragmas/module_pragmas.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#define __USE_SYSBASE
#include <proto/exec.h>
#include <ppt_real.h>

/*--------------------------------------------------------------------------*/
/* Compiler stuff and miscallaneous defines */

#ifdef _DCC
#define SAVEDS  __geta4
#define ASM
#define REG(x)  __ ## x
#define FAR     __far
#define ALIGNED
#define INLINE
#else
#ifdef __GNUC__
#define SAVEDS  __saveds
#define ASM
#define REG(x)  register
#define GREG(x) __asm( #x )
#define ALIGNED __aligned
#define INLINE  __inline
#define __AMIGADATE__ __DATE__
#else
#define SAVEDS  __saveds
#define ASM     __asm
#define REG(x)  register __ ## x
#define GREG(x)
#define ALIGNED __aligned
#define INLINE  __inline
#endif
#endif

#define REGPARAM( reg, type, name ) REG(reg) type name GREG(reg)
#define REGDECL( reg, type ) REG(reg) type GREG(reg)

#define EXTSIZE (LIB_VECTSIZE * XLIB_FUNCS + sizeof(EXTBASE))

VOID RelExtBase(EXTBASE *);

/*--------------------------------------------------------------------------*/

typedef enum {
    Effect,
    IOModule
} Type_T;

const char verstag[] = "$VER: moduleinfo 1.9 (04.02.97) (C) Janne Jalkanen 1996\0$";

EXTBASE *ExtBase = NULL;
struct Library *ModuleBase = NULL;

SAVEDS ASM ULONG Dummy(VOID)
{
    printf("Unsupported function called.\n");
    return 0;
}

SAVEDS ASM ULONG X_TagData(REG(a0) struct TagItem * list, REG(d0) ULONG tag, REG(a6) EXTBASE * foo)
{
    return (GetTagData(tag, NULL, list));
}

SAVEDS ASM ULONG X_SprintFA( REGPARAM(a0,STRPTR,buffer),
                             REGPARAM(a1,STRPTR,format),
                             REGPARAM(a2,APTR,args),
                             REGPARAM(a6,EXTBASE *,PPTBase) )
{
    ULONG res;
    res = vsprintf( buffer, format, args );
    return res;
}

APTR ExtLibData[] =
{
    Dummy,                      // NULL, /* LIB_OPEN */
     Dummy,                     // NULL, /* LIB_CLOSE */
     Dummy,                     // NULL, /* LIB_EXPUNGE */
     Dummy,                     // NULL, /* LIB_RESERVED */
     Dummy,                     // NewFrame,
     Dummy,                     // MakeFrame,
     Dummy,                     // InitFrame,
     Dummy,                     // RemFrame,
     Dummy,                     // DupFrame,

    Dummy,                      // FindFrame,

    Dummy,                      // GetPixel,
     Dummy,                     // NULL, /* BUG: PutPixel */
     Dummy,                     // GetPixelRow,
     Dummy,                     // PutPixelRow,
     Dummy,                     // GetNPixelRows,
     Dummy,                     // PutNPixelRows,
     Dummy,                     // GetBitMapRow,

    Dummy,                      // UpdateProgress,
     Dummy,                     // InitProgress,
     Dummy,                     // Progress,
     Dummy,                     // FinishProgress,
     Dummy,                     // ClearProgress,

    Dummy,                      // SetErrorCode,
     Dummy,                     // SetErrorMsg,

    Dummy,                      // AskReqA,

    Dummy,                      // PlanarToChunky,
     Dummy,                     // NULL, /* BUG: ChunkyToPlanar */

    Dummy,                      // GetStr_External,
     X_TagData,                 // TagData,

    Dummy,                      // StartInput,
     Dummy,                     // StopInput,

    Dummy,                      // GetBackgroundColor,

    Dummy,                      // GetOptions,
    Dummy,                      // PutOptions,

    Dummy,                      // AddExtension,
    Dummy,                      // FindExtension,
    Dummy,                      // RemoveExtension,

    /* Start of V4 additions */

    Dummy,                      // ObtainPreviewFrameA,
    Dummy,                      // ReleasePreviewFrame,
    Dummy,                      // RenderFrame,       /* 40 */
    Dummy,                      // CopyFrameData,

    /* Start of V5 additions */

    Dummy,                      // CloseProgress,
    Dummy,                      // SetRexxVariable,

    /* Start of V6 additions */

    X_SprintFA,                 // SPrintFA,

    (APTR) ~ 0                  /* Marks the end of the table for MakeFunctions() */
};

/*
   Won't close libs, because uses the SAS/C auto-open feature.
 */
VOID CloseLibBases(EXTBASE * xd)
{
    if (xd->mport)
        DeleteMsgPort(xd->mport);       /* Exec call. Safe to do. */
}

/*
   Opens up libraries:

   Fakes this by using the auto-open feature.
 */

PERROR OpenLibBases(EXTBASE * xd)
{
    xd->lib.lib_Version = PPT_VERSION_SUPPORTED;
    xd->lib.lib_Revision = 0L;
    xd->g = NULL;
    xd->mport = CreateMsgPort();        /* Exec call. Safe to do. */
    xd->lb_Sys = SysBase;
    xd->lb_DOS = DOSBase;
    xd->lb_Utility = UtilityBase;
    xd->lb_Intuition = IntuitionBase;
    xd->lb_BGUI = NULL;
    xd->lb_Gfx = GfxBase;
    xd->lb_GadTools = GadToolsBase;
    xd->lb_Locale = NULL;

    return PERR_OK;
}

EXTBASE *NewExtBase(VOID)
{
    EXTBASE *ExtBase = NULL;
    APTR realptr;

    realptr = malloc(EXTSIZE);

    if (realptr) {
        ExtBase = (EXTBASE *) ((ULONG) realptr + (EXTSIZE - sizeof(EXTBASE)));
        bzero(realptr, EXTSIZE);

        if (OpenLibBases(ExtBase) != PERR_OK) {
            free(ExtBase);
            return NULL;
        }
        MakeFunctions(ExtBase, ExtLibData, NULL);
    }
    return ExtBase;
}

/*
   Use to release ExtBase allocated in NewExtBase()
 */

SAVEDS VOID RelExtBase(EXTBASE * xb)
{
    APTR realptr;

    CloseLibBases(xb);
    realptr = (APTR) ((ULONG) xb - (EXTSIZE - sizeof(EXTBASE)));
    free(realptr);
}

char *DecodeCS(ULONG cs)
{
    static char buf[256];

    strcpy(buf, "");

    if (cs == ~0) {
        strcpy(buf, "All");
        return buf;
    }
    if (cs & CSF_RGB)
        strcat(buf, "RGB,");
    if (cs & CSF_GRAYLEVEL)
        strcat(buf, "Graylevel,");
    if (cs & CSF_LUT)
        strcat(buf, "Colormapped,");
    if (cs & CSF_ARGB)
        strcat(buf, "ARGB,");

    if (cs)
        buf[strlen(buf) - 1] = '\0';    // Remove last comma.

    return buf;
}

char *DecodeSF(ULONG sf)
{
    char *s;

    s = DecodeCS(sf);

    if (s[0] == '\0')
        strcpy(s, "<<Saving not supported>>");

    return s;
}

char *DecodeCPU(ULONG flags)
{
    static char buf[256];

    strcpy(buf, "");

    if (!flags) {
        strcpy(buf, "generic 68000");
        return buf;
    }
    if (flags & AFF_68010)
        strcat(buf, "68010,");

    if (flags & AFF_68020)
        strcat(buf, "68020,");

    if (flags & AFF_68030)
        strcat(buf, "68030,");

    if (flags & AFF_68040)
        strcat(buf, "68040,");

    if (flags & AFF_68060)
        strcat(buf, "68060,");

    if (flags & AFF_68881)
        strcat(buf, "68881,");

    if (flags & AFF_68882)
        strcat(buf, "68882,");

    if (flags & AFF_PPC)
        strcat(buf, "PowerPC,");

    buf[strlen(buf) - 1] = '\0';        // Remove last comma.

    return buf;
}

void ShowInfo(char *name)
{
    char buf[512], *s;
    Type_T type;

    s = strrchr(name, '.');
    if (s) {
        if (strcmp(++s, "effect") == 0)
            type = Effect;
        else
            type = IOModule;
    } else {
        printf("Wrong name?!?\n");
        return;
    }

    if (ModuleBase = OpenLibrary(name, 0L)) {
        printf("MODULE: %s  (%s)\n", name, (type == Effect) ? "Effect" : "IO Module");
        printf("  Name:                    %s\n", Inquire(PPTX_Name, ExtBase));
        printf("  Version:                 %d.%d\n", ModuleBase->lib_Version, ModuleBase->lib_Revision);
        printf("  CPU Level:               %s\n", DecodeCPU(Inquire(PPTX_CPU, ExtBase)));

        strncpy(buf, (STRPTR) Inquire(PPTX_InfoTxt, ExtBase), 511);
        printf("  Infotext:                %s\n", strtok(buf, "\n"));
        while (s = strtok(NULL, "\n"))
            printf("                           %s\n", s);

        printf("  Author:                  %s\n", Inquire(PPTX_Author, ExtBase));
        printf("  PPT Revision required:   %d\n", Inquire(PPTX_ReqPPTVersion, ExtBase));
        printf("  OS Version required:     %d\n", Inquire(PPTX_ReqKickVersion, ExtBase));
        s = (char *) Inquire(PPTX_RexxTemplate, ExtBase);
        printf("  REXX arguments:          %s\n", s ? s : "<<REXX not supported>>");
        printf("  Priority:                %d\n", Inquire(PPTX_Priority, ExtBase));
        printf("  GetArgs:                 %s\n", Inquire(PPTX_SupportsGetArgs,ExtBase) ? "supported" : "not supported" );

        if (type == IOModule) {
            printf("  Save formats supported:  %s\n",
                   DecodeSF(Inquire(PPTX_ColorSpaces, ExtBase)));
        } else {
            printf("  Colorspaces handled:     %s\n",
                   DecodeCS(Inquire(PPTX_ColorSpaces, ExtBase)));
        }

        CloseLibrary(ModuleBase);
    } else {
        printf("%s is not a library!\n", name);
    }
}

int main(int argc, char **argv)
{
    int i;
    struct AnchorPath *ap;
    LONG err;
    BOOL quit = FALSE;

    if (argc == 0) {
        argc = _WBArgc;
        argv = _WBArgv;
    }
    ap = malloc(sizeof(struct AnchorPath) + MAXPATHLEN);

    if (!ap)
        return 20;

    if (argc > 1) {

        ExtBase = NewExtBase();

        for (i = 1; i < argc && !quit; i++) {

            bzero(ap, sizeof(struct AnchorPath) + MAXPATHLEN);

            ap->ap_Strlen = MAXPATHLEN - 1;

            if (0 == (err = MatchFirst(argv[i], ap))) {
                ap->ap_Flags &= ~(APF_DODIR);   /* No, we don't want dirs */
                do {
                    /*
                     *  Skip all directories!
                     */

                    if (ap->ap_Info.fib_DirEntryType < 0) {
                        ShowInfo(ap->ap_Buf);
                    }
                    /*
                     *  Check for break
                     */

                    if (CheckSignal(SIGBREAKF_CTRL_C)) {
                        printf("***BREAK\n");
                        quit = TRUE;
                    }
                } while (!quit && 0 == (err = MatchNext(ap)));
                MatchEnd(ap);
            }
            if (err != 0 && err != ERROR_NO_MORE_ENTRIES) {
                char errbuf[81];

                Fault(err, NULL, errbuf, 80);
                printf("%s: Error while reading '%s' : %s\n", argv[0], argv[i], errbuf);
                break;
            }
        }

        RelExtBase(ExtBase);

    } else {
        printf("Usage: %s [module] [module] ...\n", argv[0]);
    }

    free(ap);
}
