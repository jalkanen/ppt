/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt
    MODULE : prefs.c

    $Id: prefs.c,v 1.23 1998/01/04 16:37:01 jj Exp $
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

#define GOID_COMMENT        -1
#define GOID_UNKNOWN        -2

/*----------------------------------------------------------------------*/
/* Global variables */


PREFS tmpprefs;
DISPLAY tmpdisp;

const char prefs_init_blurb[] =
    "; PPT v"VERSION" configuration file\n"
    "; PPT is "COPYRIGHT".\n";

/* The option strings. Must be NULL-terminated. */

const char *optionstrings[] = {
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
    PREVIEWSIZE
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

Option GetOptID( const char *s )
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

    sprintf(buf,"%s=",optionstrings[id]);
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
    p->extstacksize = DEFAULT_EXTSTACK;
    strcpy(p->vmdir, DEFAULT_VM_DIR);
    p->vmbufsiz = DEFAULT_VM_BUFSIZ;
    p->mainfont = p->listfont = NULL;
    p->pubscrname= NULL;
    p->maxundo = DEFAULT_MAXUNDO;
    p->mfontname[0] = '\0';
    p->lfontname[0] = '\0';
    p->progress_filesize = DEFAULT_PROGRESS_FILESIZE;
    p->progress_step = DEFAULT_PROGRESS_STEP;
    p->colorpreview = FALSE;
    strcpy( p->modulepath, DEFAULT_MODULEPATH );
    p->expungelibs  = FALSE;
    strcpy( p->rexxpath, DEFAULT_REXXPATH );
    strcpy( p->startupdir, DEFAULT_STARTUPDIR );
    strcpy( p->startupfile, DEFAULT_STARTUPFILE );
    p->extniceval = DEFAULT_EXTNICEVAL;
    p->extpriority = DEFAULT_EXTPRIORITY;
    p->previewmode   = DEFAULT_PREVIEWMODE;
    SetPreviewSize( p );
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

        framew.initialopen = FALSE;
        toolw.initialopen = FALSE;

        while(FGets(fh,buf,PPBUFLEN)) {
            buf[strlen(buf)-1] = '\0'; /* Remove the final CR */
            s = strtok(buf,"= \t"); /* Skips all whitespaces as well */
            if(s) { /* It was an option! */
                s = strtok(NULL,"\n"); /* This has the added benefit of removing the CR from the end.*/
                switch(GetOptID( buf )) {
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

                    case FRAMESWINDOW:
                        sscanf( s, "%hd/%hd/%hd/%hd", &framew.initialpos.Left,
                                                  &framew.initialpos.Top,
                                                  &framew.initialpos.Width,
                                                  &framew.initialpos.Height );
                        D(bug("Frames window at (%d,%d), width = %d, height = %d\n",
                               framew.initialpos.Left, framew.initialpos.Top,
                               framew.initialpos.Width, framew.initialpos.Height ));

                        framew.initialopen = TRUE;
                        break;

                    case TOOLWINDOW:
                        sscanf( s, "%hd/%hd/%hd/%hd", &toolw.initialpos.Left,
                                                  &toolw.initialpos.Top,
                                                  &toolw.initialpos.Width,
                                                  &toolw.initialpos.Height );
                        D(bug("Tool window at (%d,%d), width = %d, height = %d\n",
                               toolw.initialpos.Left, toolw.initialpos.Top,
                               toolw.initialpos.Width, toolw.initialpos.Height ));


                        toolw.initialopen = TRUE;
                        break;

                    case SELECTWINDOW:
                        sscanf( s, "%hd/%hd/%hd/%hd", &selectw.initialpos.Left,
                                                  &selectw.initialpos.Top,
                                                  &selectw.initialpos.Width,
                                                  &selectw.initialpos.Height );
                        D(bug("Select window at (%d,%d), width = %d, height = %d\n",
                               selectw.initialpos.Left, selectw.initialpos.Top,
                               selectw.initialpos.Width, selectw.initialpos.Height ));


                        selectw.initialopen = TRUE;
                        break;

                    case EFFECTSWINDOW:
                        sscanf( s, "%hd/%hd/%hd/%hd", &extf.initialpos.Left,
                                                  &extf.initialpos.Top,
                                                  &extf.initialpos.Width,
                                                  &extf.initialpos.Height );
                        D(bug("Effects window at (%d,%d), width = %d, height = %d\n",
                               extf.initialpos.Left, extf.initialpos.Top,
                               extf.initialpos.Width, extf.initialpos.Height ));

                        extf.initialopen = TRUE;
                        break;

                    case LOADERSWINDOW:
                        sscanf( s, "%hd/%hd/%hd/%hd", &extl.initialpos.Left,
                                                  &extl.initialpos.Top,
                                                  &extl.initialpos.Width,
                                                  &extl.initialpos.Height );
                        D(bug("Loaders window at (%d,%d), width = %d, height = %d\n",
                               extl.initialpos.Left, extl.initialpos.Top,
                               extl.initialpos.Width, extl.initialpos.Height ));

                        extl.initialopen = TRUE;
                        break;

                    case SCRIPTSWINDOW:
                        sscanf( s, "%hd/%hd/%hd/%hd", &exts.initialpos.Left,
                                                  &exts.initialpos.Top,
                                                  &exts.initialpos.Width,
                                                  &exts.initialpos.Height );
                        D(bug("Scripts window at (%d,%d), width = %d, height = %d\n",
                               exts.initialpos.Left, exts.initialpos.Top,
                               exts.initialpos.Width, exts.initialpos.Height ));

                        exts.initialopen = TRUE;
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
                        if( tmp >= PWMODE_OFF && tmp <= PWMODE_LARGE ) {
                            p->previewmode = tmp;
                            SetPreviewSize( p );
                        }
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
        case PWMODE_SMALL:
            p->previewheight = 45;
            p->previewwidth = 60;
            break;
        case PWMODE_MEDIUM:
            p->previewheight = 60;
            p->previewwidth = 80;
            break;
        case PWMODE_LARGE:
            p->previewheight = 75;
            p->previewwidth = 100;
            break;
    }
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

