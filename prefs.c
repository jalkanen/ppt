/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt
    MODULE : prefs.c

    $Id: prefs.c,v 1.28 1999/01/02 22:35:27 jj Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"
#include "gui.h"
#include "version.h"

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef GRAPHICS_TEXT_H
#include <graphics/text.h>
#endif

#include <libraries/asl.h>

#include <clib/asl_protos.h>


#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>



/*----------------------------------------------------------------------*/
/* Defines */

#define PPBUFLEN            256

/*----------------------------------------------------------------------*/
/* Global variables */


PREFS tmpprefs;
DISPLAY tmpdisp;
struct PPTFileRequester gvLoadFileReq    = { 10, 10, 100, 200 },
                        gvPaletteOpenReq = { 10, 10, 100, 200 },
                        gvPaletteSaveReq = { 10, 10, 100, 200 };

const char prefs_init_blurb[] =
    "; PPT v"VERSION" configuration file\n"
    "; PPT is "COPYRIGHT".\n";

/* The option strings. Must be NULL-terminated. */

const char *PrefsOptions[] = {
    "VMDIR",
    "VMBUFSIZ",
    "MAINFONT",
    "LISTFONT",
    "SCRHEIGHT",
    "SCRWIDTH",
    "SCRDEPTH",
    "PUBSCREEN",
    "SCRMODEID",
    "MAXUNDO",
    "COLORPREVIEW",
    "MODULEPATH",
    "FLUSHLIBS",
    "FRAMESWINDOW",
    "TOOLWINDOW",
    "EFFECTSWINDOW",
    "LOADERSWINDOW",
    "SCRIPTSWINDOW",
    "SELECTWINDOW",
    "EXTSTACKSIZE",
    "STARTUPDIR",
    "EXTNICEVAL",
    "EXTPRIORITY",
    "PREVIEWSIZE",
    "CONFIRMREQUESTERS",
    "OPENFILEREQUESTER",
    "OPENPALETTEREQUESTER",
    "SAVEPALETTEREQUESTER",
    "BEGINTOOLBAR",
    NULL,
};

typedef enum {
    VMDIR,
    VMBUFSIZ,
    MAINFONT,
    LISTFONT,
    SCRHEIGHT,
    SCRWIDTH,
    SCRDEPTH,
    PUBSCREEN,
    SCRMODEID,
    MAXUNDO,
    COLORPREVIEW,
    MODULEPATH,
    FLUSHLIBS,
    FRAMESWINDOW,
    TOOLWINDOW,
    EFFECTSWINDOW,
    LOADERSWINDOW,
    SCRIPTSWINDOW,
    SELECTWINDOW,
    EXTSTACKSIZE,
    STARTUPDIR,
    EXTNICEVAL,
    EXTPRIORITY,
    PREVIEWSIZE,
    CONFIRMREQUESTERS,
    OPENFILEREQUESTER,
    OPENPALETTEREQUESTER,
    SAVEPALETTEREQUESTER,
    BEGINTOOLBAR
} Option;



/*----------------------------------------------------------------------*/
/* Internal prototypes */

Prototype int SavePrefs( GLOBALS *, char * );
Prototype int LoadPrefs( GLOBALS *, char * );
Prototype DISPLAY *GetScreenMode( DISPLAY * );
Prototype VOID UpdateFontPrefs( PREFS * );
Prototype VOID UpdatePrefsWindow( PREFS * );
Prototype VOID CopyPrefs( PREFS *, PREFS * );

/*----------------------------------------------------------------------*/
/* Code */

/*
    Returns the ID of option, -1 if not recognised or
    -2 for irrelevant information (comments, etc.)
*/

Prototype int GetOptID( char *optionstrings[], const char *s );

int GetOptID( char *optionstrings[], const char *s )
{
    int id;

    if(*s == ';' || *s == '\n' || strlen(s) == 0)
        return GOID_COMMENT;

    for(id = 0; optionstrings[id] != NULL; id++) {
        if(strcmp(optionstrings[id], s) == 0)
            return id;
    }
    return GOID_UNKNOWN;
}


/*
    Writes an option string into given file.
*/

void WritePrefByID( BPTR fh, Option id, const char *c, ... )
{
    char buf[PPBUFLEN];
    va_list va;

    sprintf(buf,"%s=",PrefsOptions[id]);
    va_start(va,c);
    vsprintf( (char *) ((ULONG)buf + strlen(buf)), c, va );
    va_end(va);
    FPuts(fh, buf);
    FPutC(fh, '\n'); /* Add a CR */
}


/*
    Initializes a prefs struct to defaults.
*/

Local
VOID InitPrefs( PREFS *p )
{
    p->extstacksize     = DEFAULT_EXTSTACK;
    strcpy(p->vmdir, DEFAULT_VM_DIR);
    p->vmbufsiz         = DEFAULT_VM_BUFSIZ;
    p->mainfont = p->listfont = NULL;
    p->pubscrname       = NULL;
    p->maxundo          = DEFAULT_MAXUNDO;
    p->mfontname[0]     = '\0';
    p->lfontname[0]     = '\0';
    p->progress_filesize = DEFAULT_PROGRESS_FILESIZE;
    p->progress_step    = DEFAULT_PROGRESS_STEP;
    p->colorpreview     = FALSE;
    strcpy( p->modulepath, DEFAULT_MODULEPATH );
    p->expungelibs      = FALSE;
    strcpy( p->rexxpath, DEFAULT_REXXPATH );
    strcpy( p->startupdir, DEFAULT_STARTUPDIR );
    strcpy( p->startupfile, DEFAULT_STARTUPFILE );
    p->extniceval       = DEFAULT_EXTNICEVAL;
    p->extpriority      = DEFAULT_EXTPRIORITY;
    p->previewmode      = DEFAULT_PREVIEWMODE;
    p->confirm          = DEFAULT_CONFIRM;
    SetPreviewSize( p );
}

Local
VOID ReadWindowPrefs( const char *s, struct WindowPrefs *p )
{
    sscanf( s, "%hd/%hd/%hd/%hd", &p->initialpos.Left,
                                  &p->initialpos.Top,
                                  &p->initialpos.Width,
                                  &p->initialpos.Height );

    D(bug("Window at (%d,%d), width = %d, height = %d\n",
          p->initialpos.Left, p->initialpos.Top,
          p->initialpos.Width, p->initialpos.Height ));
}

/*
    Temporary kludge until ASLREQ_Bounds is gettable
 */
Local
VOID WriteASLPrefs( BPTR fh, Option id, Object *Req )
{
    LONG top, left, width, height;

    GetAttr( ASLREQ_Left, Req, (ULONG*) &left );
    GetAttr( ASLREQ_Top, Req, (ULONG*)&top );
    GetAttr( ASLREQ_Width, Req, (ULONG*)&width );
    GetAttr( ASLREQ_Height, Req, (ULONG*) &height );

    WritePrefByID( fh, id, "%d/%d/%d/%d",left,top,width,height);
}


/*
    Loads user preferences from the given file. If pfile == NULL,
    then use default.
    Prefs file format:
        string=value
        string=value

    BUG: Few sanity checks.
*/

int LoadPrefs( GLOBALS *g, char *pfile )
{
    BPTR fh;
    UBYTE *buf, *s, *tail, *t;
    PREFS *p = g->userprefs;
    DISPLAY *d = g->maindisp;
    int res;
    LONG tmp;

    InitPrefs( p );

    buf = pmalloc( PPBUFLEN );
    if(!buf)
        return PERR_OUTOFMEMORY;

    /*
     *  Do the write
     */

    fh = Open( pfile ? pfile : DEFAULT_PREFS_FILE, MODE_OLDFILE );
    if(fh) {

        /*
         *  Prefs file was opened correctly.  Kludge alert!
         *  We'll now reset some prefs.
         */

        framew.prefs.initialopen = FALSE;
        toolw.prefs.initialopen = FALSE;

        while(FGets(fh,buf,PPBUFLEN)) {
            if( buf[strlen(buf)-1] = '\n' )
                buf[strlen(buf)-1] = '\0'; /* Remove the final CR */

            s = strtok(buf,"= \t"); /* Skips all whitespaces as well */
            if(s) { /* It was an option! */
                s = strtok(NULL,"\n"); /* This has the added benefit of removing the CR from the end.*/
                switch(GetOptID( PrefsOptions, buf )) {
                    case SCRMODEID:
                        d->dispid = strtol(s, &tail , 0); /* Interprets HEX values correctly */
                        D(bug("Got display ID %08X\n",d->dispid));
                        /* BUG: should check against display database if this mode is available */
                        break;

                    case SCRHEIGHT:
                        d->height = strtol(s, &tail, 0);
                        D(bug("Got height %d\n",d->height));
                        break;

                    case SCRWIDTH:
                        d->width = strtol(s, &tail, 0);
                        D(bug("Got width %d\n",d->width));
                        break;

                    case SCRDEPTH:
                        d->depth = strtol(s, &tail, 0);
                        D(bug("Got depth %d\n",d->depth));
                        if(d->depth < 3) d->depth = 3; /* Must have at least 8 colors. */
                        break;

                    case PUBSCREEN:
                        Req(NULL,NULL,"Opening on a public screen not yet supported");
                        break;

                    case VMDIR:
                        strncpy(p->vmdir,s,MAXPATHLEN);
                        D(bug("Got vmdir '%s'\n",globals->userprefs->vmdir));
                        break;

                    case VMBUFSIZ:
                        tmp = strtol(s,&tail,0);
                        D(bug("Got vmbufsiz %ld Kbytes\n",tmp));
                        if(tmp < MIN_VMBUFSIZ)
                            tmp = MIN_VMBUFSIZ;

                        p->vmbufsiz = tmp;
                        break;

                    case MAXUNDO:
                        tmp = strtol(s, &tail, 0);
                        D(bug("Got %d undo levels\n",tmp ));
                        p->maxundo = (UWORD)tmp;
                        break;

                    case MAINFONT:
                        t = p->mfontname;
                        while( *s != ' ' ) /* BUG: won't stop on illegal input */
                            *t++ = *s++;
                        POKE(t,0);
                        p->mfont.ta_YSize = strtol(s,&tail,0);
                        p->mainfont = &(p->mfont);
                        D(bug("Main font: %s, size %d\n",p->mfont.ta_Name,p->mfont.ta_YSize));
                        break;

                    case LISTFONT:
                        t = p->lfontname;
                        while( *s != ' ' ) /* BUG: won't stop on illegal input */
                            *t++ = *s++;
                        POKE(t,0);
                        p->lfont.ta_YSize = strtol(s,&tail,0);
                        p->listfont = &p->lfont;
                        D(bug("List font: %s, size %d\n",p->lfont.ta_Name,p->lfont.ta_YSize));
                        break;

                    case COLORPREVIEW:
                        if( strncmp( s, "TRUE", 4 ) == 0 ) {
                            p->colorpreview = TRUE;
                        } else {
                            p->colorpreview = FALSE;
                        }
                        D(bug("Preview mode : %d\n",p->colorpreview ));
                        break;

                    case MODULEPATH:
                        strncpy(p->modulepath,s,MAXPATHLEN);
                        D(bug("Module path: %s\n",p->modulepath));
                        break;

                    case FLUSHLIBS:
                        if( strncmp( s, "TRUE", 4 ) == 0 ) {
                            p->expungelibs = TRUE;
                        } else {
                            p->expungelibs = FALSE;
                        }
                        D(bug("Flushlibs mode : %d\n", p->expungelibs ));
                        break;

                    case CONFIRMREQUESTERS:
                        if( strncmp( s, "TRUE", 4 ) == 0 ) {
                            p->confirm = TRUE;
                        } else {
                            p->confirm = FALSE;
                        }
                        D(bug("Confirm mode : %d\n", p->confirm ));
                        break;

                    case FRAMESWINDOW:
                        ReadWindowPrefs( s, &framew.prefs );
                        framew.prefs.initialopen = TRUE;
                        break;

                    case TOOLWINDOW:
                        ReadWindowPrefs( s, &toolw.prefs );
                        toolw.prefs.initialopen = TRUE;
                        break;

                    case SELECTWINDOW:
                        ReadWindowPrefs( s, &selectw.prefs );
                        selectw.prefs.initialopen = TRUE;
                        break;

                    case EFFECTSWINDOW:
                        ReadWindowPrefs( s, &extf.prefs );
                        extf.prefs.initialopen = TRUE;
                        break;

                    case LOADERSWINDOW:
                        ReadWindowPrefs( s, &extl.prefs );
                        extl.prefs.initialopen = TRUE;
                        break;

                    case SCRIPTSWINDOW:
                        ReadWindowPrefs( s, &exts.prefs );
                        exts.prefs.initialopen = TRUE;
                        break;

                    case EXTSTACKSIZE:
                        tmp = strtol(s, &tail, 0);
                        if( tmp >= 4096 ) {
                            p->extstacksize = tmp;
                        } else {
                            Req( NEGNUL, NULL, GetStr(mPREFS_EXTSTACK_LOW), 4096 );
                            p->extstacksize = 4096;
                        }

                        D(bug("Set external stack at %lu\n",p->extstacksize));
                        break;

                    case STARTUPDIR:
                        t = FilePart(s);
                        strncpy(p->startupfile,t,NAMELEN);
                        t = PathPart(s);
                        if( t ) {
                            *t = '\0';
                            strncpy(p->startupdir,s,MAXPATHLEN);
                        }
                        break;

                    case EXTNICEVAL:
                        tmp = strtol(s, &tail, 0);
                        if( tmp >= -20 && tmp <= 20 )
                            p->extniceval = tmp;
                        break;

                    case EXTPRIORITY:
                        tmp = strtol(s, &tail, 0);
                        if( tmp >= -127 && tmp <= 1 )
                            p->extpriority = tmp;
                        break;

                    case PREVIEWSIZE:
                        tmp = strtol( s, &tail, 0 );
                        if( tmp >= PWMODE_OFF && tmp <= PWMODE_LAST ) {
                            p->previewmode = tmp;
                            SetPreviewSize( p );
                        }
                        break;

                    case OPENFILEREQUESTER:
                        ReadWindowPrefs( s, &gvLoadFileReq.prefs );
                        break;

                    case OPENPALETTEREQUESTER:
                        ReadWindowPrefs( s, &gvPaletteOpenReq.prefs );
                        break;

                    case SAVEPALETTEREQUESTER:
                        ReadWindowPrefs( s, &gvPaletteSaveReq.prefs );
                        break;

                    case BEGINTOOLBAR:
                        ReadToolbarConfig( fh );
                        break;

                    case GOID_UNKNOWN:
                        D(bug("Unknown gobble %s\n",buf));
                        break;
                    case GOID_COMMENT:
                        D(bug("Skipped comment line %s\n",buf));
                        break;
                    default:
                        D(bug("%d not supported yet\n",buf));
                        break;
                }  /* switch */
            } /* if */
        } /* while */

        Close(fh); /* BUG: should check */
        res = PERR_OK;
    } else
        res = PERR_INITFAILED;

    pfree(buf);
    return res;
}


/*
    Save preferences to given file. If pfile == NULL, uses default.
*/

int SavePrefs( GLOBALS *g, char *pfile )
{
    BPTR fh;
    int res;
    PREFS *p = g->userprefs;
    DISPLAY *d = g->maindisp;
    struct IBox ib;
    ULONG top,width,height,left;

    fh = Open( pfile ? pfile : DEFAULT_PREFS_FILE, MODE_NEWFILE );
    if(fh) {
        FPuts(fh,prefs_init_blurb);

        WritePrefByID(fh,VMBUFSIZ,"%d",p->vmbufsiz);
        WritePrefByID(fh,SCRMODEID,"0x%08X",d->dispid);
        WritePrefByID(fh,SCRHEIGHT,"%u",d->height);
        WritePrefByID(fh,SCRWIDTH,"%u",d->width);
        WritePrefByID(fh,SCRDEPTH,"%u",d->depth);
        WritePrefByID(fh,MAXUNDO,"%u",p->maxundo );
        WritePrefByID(fh,COLORPREVIEW,"%s",p->colorpreview ? "TRUE" : "FALSE" );
        WritePrefByID(fh,FLUSHLIBS,"%s",p->expungelibs ? "TRUE" : "FALSE" );
        WritePrefByID(fh,CONFIRMREQUESTERS,"%s",p->confirm ? "TRUE" : "FALSE" );
        WritePrefByID(fh,EXTSTACKSIZE,"%u",p->extstacksize);
        WritePrefByID(fh,EXTNICEVAL,"%d",p->extniceval);
        WritePrefByID(fh,EXTPRIORITY,"%d",p->extpriority);
        WritePrefByID(fh,PREVIEWSIZE,"%d",p->previewmode);

        if(p->mainfont)
            WritePrefByID( fh, MAINFONT, "%s %d",p->mainfont->ta_Name,p->mainfont->ta_YSize);

        if(p->listfont)
            WritePrefByID(fh,LISTFONT,"%s %d",p->listfont->ta_Name,p->listfont->ta_YSize);

        if(p->vmdir)
            WritePrefByID(fh,VMDIR,"%s",p->vmdir);

        if(p->pubscrname)
            WritePrefByID(fh,PUBSCREEN,"%s",p->pubscrname);

        if(p->modulepath)
            WritePrefByID(fh,MODULEPATH,"%s",p->modulepath);

        if(p->startupdir[0]) {
            UBYTE path[MAXPATHLEN+1];

            strcpy( path, p->startupdir );
            AddPart( path, p->startupfile, MAXPATHLEN );
            WritePrefByID(fh,STARTUPDIR,"%s",path);
        }

        if( framew.win ) {
            GetAttr(WINDOW_Bounds, framew.Win, (ULONG *) &ib );
            WritePrefByID( fh, FRAMESWINDOW, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);
        }

        if( toolw.win ) {
            GetAttr(WINDOW_Bounds, toolw.Win, (ULONG *) &ib );
            WritePrefByID( fh, TOOLWINDOW, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);
        }

        if( selectw.win ) {
            GetAttr(WINDOW_Bounds, selectw.Win, (ULONG *) &ib );
            WritePrefByID( fh, SELECTWINDOW, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);
        }

        if( extf.win ) {
            GetAttr(WINDOW_Bounds, extf.Win, (ULONG *) &ib );
            WritePrefByID( fh, EFFECTSWINDOW, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);
        }

        if( extl.win ) {
            GetAttr(WINDOW_Bounds, extl.Win, (ULONG *) &ib );
            WritePrefByID( fh, LOADERSWINDOW, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);
        }

        if( exts.win ) {
            GetAttr(WINDOW_Bounds, exts.Win, (ULONG *) &ib );
            WritePrefByID( fh, SCRIPTSWINDOW, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);
        }

#if 0
        GetAttr( ASLREQ_Bounds, gvLoadFileReq.Req, (ULONG *)&ib );
        WritePrefByID( fh, OPENFILEREQUESTER, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);

        GetAttr( ASLREQ_Bounds, gvPaletteOpenReq.Req, (ULONG *)&ib );
        WritePrefByID( fh, OPENPALETTEREQUESTER, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);

        GetAttr( ASLREQ_Bounds, gvPaletteSaveReq.Req, (ULONG *)&ib );
        WritePrefByID( fh, SAVEPALETTEREQUESTER, "%d/%d/%d/%d",ib.Left,ib.Top,ib.Width,ib.Height);
#endif
        WriteASLPrefs( fh, OPENFILEREQUESTER, gvLoadFileReq.Req );
        WriteASLPrefs( fh, OPENPALETTEREQUESTER, gvPaletteOpenReq.Req );
        WriteASLPrefs( fh, SAVEPALETTEREQUESTER, gvPaletteSaveReq.Req );

        WriteToolbarConfig( fh );

        Close(fh);
        res = PERR_OK;
    } else
        res = PERR_WONTOPEN;

    return res;
}



/*
    Asks for a screen mode. Returns NULL on no changes.
    BUG: should save the requester attributes.
    BUG: Does not really belong here.
    BUG: Does not check any return values.
*/

DISPLAY *GetScreenMode( DISPLAY *orig )
{
    struct ScreenModeRequester *scr;
    DISPLAY *d;

    BusyAllWindows(globxd);

    if(orig) {
        scr = AllocAslRequestTags( ASL_ScreenModeRequest,
                                    ASLSM_Screen, MAINSCR,
                                    ASLSM_Locale, globxd->locale,
                                    ASLSM_InitialDisplayID, orig->dispid,
                                    ASLSM_InitialDisplayWidth,orig->width,
                                    ASLSM_InitialDisplayHeight,orig->height,
                                    ASLSM_InitialDisplayDepth,orig->depth,
                                    ASLSM_DoAutoScroll, TRUE,
                                    ASLSM_DoWidth, TRUE, ASLSM_DoHeight,TRUE, ASLSM_DoDepth,TRUE,
                                    ASLSM_DoOverscanType, FALSE,
                                    TAG_END );
        d = orig;
    } else {
        scr = AllocAslRequestTags( ASL_ScreenModeRequest,
                                    ASLSM_Screen, MAINSCR,
                                    ASLSM_DoOverscanType, FALSE,
                                    ASLSM_DoAutoScroll, TRUE,
                                    ASLSM_DoWidth, TRUE, ASLSM_DoHeight,TRUE, ASLSM_DoDepth,TRUE,
                                    TAG_END );
        d = AllocDisplay( globxd );
    }

    if(AslRequest( scr, NULL )) {
        d->dispid = scr->sm_DisplayID;
        d->height = scr->sm_DisplayHeight; /* BUG: should maybe use bitmapheight? */
        d->width = scr->sm_DisplayWidth;
        d->depth = scr->sm_DisplayDepth;
    } else {
        if(!orig)
            FreeDisplay( d, globxd );
        d = NULL;
    }

    FreeAslRequest(scr);

    AwakenAllWindows(globxd);

    return d;
}

VOID CopyPrefs( PREFS *src, PREFS *dst )
{
    D(bug("CopyPrefs(from=%08X, to=%08X\n",src,dst));

    bcopy(src, dst, sizeof(PREFS));

    /*
     *  Gotta set up some pointers, though.
     *  BUG: Does not copy pubscreen
     */

    if(src->mainfont) dst->mainfont = &(dst->mfont);
    if(src->listfont) dst->listfont = &(dst->lfont);

    dst->mfont.ta_Name = dst->mfontname;
    dst->lfont.ta_Name = dst->lfontname;
}

/*----------------------------------------------------------------------*/
/* GUI stuff. */

/*
    Updates font preferences

    BUG: Defaults should understand system defaults and not use topaz.
*/

VOID UpdateFontPrefs( PREFS *p )
{
    char buf[NAMELEN];

    D(bug("UpdateFontPrefs(%08X)\n",p));

    if( p->mainfont )
        sprintf( buf,"%s %d", p->mainfont->ta_Name, p->mainfont->ta_YSize );
    else
        strcpy( buf, "topaz.font 8" );

    SetGadgetAttrs( GAD(prefsw.MainFont), prefsw.win, NULL, INFO_TextFormat, buf, TAG_END );

    if( p->listfont )
        sprintf( buf,"%s %d", p->listfont->ta_Name, p->listfont->ta_YSize );
    else
        strcpy( buf, "topaz.font 8" );

    SetGadgetAttrs( GAD(prefsw.ListFont), prefsw.win, NULL, INFO_TextFormat, buf, TAG_END );
}

/*
    BUG: Is this routine actually needed or something?
*/
VOID UpdatePrefsWindow( PREFS *p )
{
    D(bug("UpdatePrefsWindow(%08X)\n",p));

    UpdateFontPrefs(p);
    SetGadgetAttrs( GAD(prefsw.VMDir), prefsw.win, NULL, STRINGA_TextVal, p->vmdir, TAG_END );
    SetGadgetAttrs( GAD(prefsw.PageSize), prefsw.win, NULL, STRINGA_LongVal, p->vmbufsiz, TAG_END );
    SetGadgetAttrs( GAD(prefsw.MaxUndo), prefsw.win, NULL, STRINGA_LongVal, (ULONG)p->maxundo, TAG_END);
}

Prototype VOID SetPreviewSize( PREFS *p );

VOID SetPreviewSize( PREFS *p )
{
    switch( p->previewmode ) {
        case PWMODE_OFF:
            p->previewheight = p->previewwidth = 0;
            break;
        case PWMODE_SMALL: /* 2700 pixels */
            p->previewheight = 45;
            p->previewwidth = 60;
            break;
        case PWMODE_MEDIUM: /* 7500 pixels */
            p->previewheight = 75;
            p->previewwidth = 100;
            break;
        case PWMODE_LARGE: /* 19200 pixels */
            p->previewheight = 120;
            p->previewwidth = 160;
            break;
        case PWMODE_HUGE: /* 76800 pixels */
            p->previewheight = 240;
            p->previewwidth  = 320;
            break;
    }
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

