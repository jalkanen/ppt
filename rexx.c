/*
    PROJECT: ppt
    MODULE : rexx.c

    $Id: rexx.c,v 6.1 1999/11/25 23:18:47 jj Exp $

    AREXX interface to PPT. Based heavily on ArexxBox
    by Michael Baltzer.
*/


#include "defs.h"
#include "misc.h"
#include "rexx.h"

#include "version.h"

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/rexxsyslib_protos.h>
#include <clib/utility_protos.h>

#include <proto/iomod.h>
#include <proto/effect.h>

#define FRAMEID( x )    (ULONG)(*((ULONG *)(x)))
#define NUMERIC( x )    (*((ULONG *)(x)))
#define STRING( x )     (UBYTE *)(x)

#define MAX_REXX_ASKREQ_ITEMS       8
#define MAX_REXX_ASKREQ_CYCLELABELS 20

/*
 *  This defines the value used in the ASKREQ rexx command for
 *  AR_FLOAT type objects
 */

#define RX_DIVISOR                  1000

/*--------------------------------------------------------------------------*/
/* Private types */

struct RexxAskReqValue {
    LONG type;
    char name[NAMELEN+1];
    char buffer[MAXPATHLEN+1];
    LONG value;
};

typedef enum
{
    ASI_INT,
    ASI_STRING
} StemType;

/*--------------------------------------------------------------------------*/
/* Prototypes */

Prototype struct RexxHost *InitRexx( char * );
Prototype int ExitRexx( struct RexxHost * );
Prototype void HandleRexxCommands( struct RexxHost * );
Prototype PPTREXXARGS *FindRexxWaitItem( struct PPTMessage * );
Prototype void ReplyRexxWaitItem( PPTREXXARGS * );
Prototype struct RexxMsg *SendRexxCommand( struct RexxHost *, char *, BPTR );

Local void FreeRexxCommand( struct RexxMsg * );
Local void ReplyRexxCommand( struct RexxMsg  *, long, long,char * );
Local struct Stem *AllocStem( UBYTE * );
Local VOID FreeStem( struct Stem *s );
Local PERROR AddStemItem( struct Stem *, UBYTE *, ULONG, StemType );
Local VOID FreeStemItem( struct StemItem * );
Local VOID RexxReply( struct RexxMsg *, PPTREXXARGS * );
Local BOOL DoRexxCommand( struct RexxMsg *, char *, PPTREXXARGS * );

/*--------------------------------------------------------------------------*/
/* Variables & other stuff. */

/// Prototypes
Local void rx_version( PPTREXXARGS *, struct RexxMsg * );
Local void rx_author( PPTREXXARGS *, struct RexxMsg * );
Local void rx_process( PPTREXXARGS *, struct RexxMsg * );
Local void rx_quit( PPTREXXARGS *, struct RexxMsg * );
Local void rx_load( PPTREXXARGS *, struct RexxMsg * );
Local void rx_setarea( PPTREXXARGS *, struct RexxMsg * );
Local void rx_deleteframe( PPTREXXARGS *, struct RexxMsg * );
Local void rx_frameinfo( PPTREXXARGS *, struct RexxMsg * );
Local void rx_getarea( PPTREXXARGS *, struct RexxMsg * );
Local void rx_save( PPTREXXARGS *, struct RexxMsg * );
Local void rx_saveas( PPTREXXARGS *, struct RexxMsg * );
Local void rx_listeffects( PPTREXXARGS *, struct RexxMsg * );
Local void rx_listloaders( PPTREXXARGS *, struct RexxMsg * );
Local void rx_effectinfo( PPTREXXARGS *, struct RexxMsg * );
Local void rx_loaderinfo( PPTREXXARGS *, struct RexxMsg * );
Local void rx_setrenderprefs( PPTREXXARGS *, struct RexxMsg * );
Local void rx_getrenderprefs( PPTREXXARGS *, struct RexxMsg * );
Local void rx_renameframe( PPTREXXARGS *, struct RexxMsg * );
Local void rx_copyframe( PPTREXXARGS *, struct RexxMsg * );
Local void rx_crop( PPTREXXARGS *, struct RexxMsg * );
Local void rx_render( PPTREXXARGS *, struct RexxMsg * );
Local void rx_ppt_to_front( PPTREXXARGS *, struct RexxMsg * );
Local void rx_ppt_to_back( PPTREXXARGS *, struct RexxMsg * );
Local void rx_render_to_front( PPTREXXARGS *, struct RexxMsg * );
Local void rx_closerender( PPTREXXARGS *, struct RexxMsg * );
Local void rx_askfile( PPTREXXARGS *, struct RexxMsg * );
Local void rx_askreq( PPTREXXARGS *, struct RexxMsg * );
Local void rx_showerror( PPTREXXARGS *, struct RexxMsg * );
Local void rx_show( PPTREXXARGS *, struct RexxMsg * );
Local void rx_hide( PPTREXXARGS *, struct RexxMsg * );
Local void rx_getargs( PPTREXXARGS *, struct RexxMsg * );
///

/// rx_commands[] and other variables
struct RexxCommand rx_commands[] = {
    "PROCESS",      "FRAME/A/N,EFFECT/A,ARGS/F",rx_process,

    "GETARGS",      "FRAME/N,PLUGIN,ARGS/F",  rx_getargs,

    "HIDE",         "FRAME/A/N",                rx_hide,
    "SHOW",         "FRAME/A/N",                rx_show,

    "SETAREA",      "FRAME/A/N,XBEGIN/N,YBEGIN/N,XEND/N,YEND/N,ALL/S",
                                                rx_setarea,

    "ASKFILE",      "TITLE/A,POSITIVE=POS,INITIALDRAWER=ID/K,INITIALFILE=IF/K,INITIALPATTERN=IP/K,SAVE/S",
                                                rx_askfile,
    "ASKREQ",       "TEXT/A,POSITIVE=POS/K,NEGATIVE=NEG/K,GAD1,GAD2,GAD3,GAD4,GAD5,GAD6,GAD7,GAD8,GAD9",
                                                rx_askreq,

    "DELETEFRAME",  "FRAME/A/N,FORCE/S",        rx_deleteframe,
    "EFFECTINFO",   "EFFECT/A,STEM/A",          rx_effectinfo,
    "IOMODULEINFO", "IOMODULE/A,STEM/A",        rx_loaderinfo,
    "FRAMEINFO",    "FRAME/A/N,STEM/A",         rx_frameinfo,
    "FREEFRAME",    "FRAME/A/N",                NULL,
    "LOADFRAME",    "FILE/A",                   rx_load,
    "RENAMEFRAME",  "FRAME/A/N,NAME/A",         rx_renameframe,
    "COPYFRAME",    "FRAME/A/N",                rx_copyframe,

    "LISTEFFECTS",  "STEM/A",                   rx_listeffects,
    "LISTIOMODULES","STEM/A,ONLYLOADERS/S,ONLYSAVERS/S",
                                                rx_listloaders,
    "RESERVEFRAME", "FRAME/A/N",                NULL,
    "SAVEFRAME",    "FRAME/A/N,COLORMAPPED/S,ARGS/F",
                                                rx_save,
    "SAVEFRAMEAS",  "FRAME/A/N,FILENAME/A,FORMAT/A/K,COLORMAPPED/S,ARGS/F",
                                                rx_saveas,

    "SHOWERROR",    "ERROR/A,LINE/N",           rx_showerror,

    "GETAREA",      "FRAME/A/N,STEM/A",         rx_getarea,

    "RENDER",       "FRAME/A/N",                rx_render,
    "RENDER_TO_FRONT","FRAME/A/N",              rx_render_to_front,
    "CLOSERENDER",  "FRAME/A/N",                rx_closerender,
    "PPT_TO_FRONT", NULL,                       rx_ppt_to_front,
    "PPT_TO_BACK",  NULL,                       rx_ppt_to_back,
    "CROP",         "FRAME/A/N",                rx_crop,
    "SETRENDERPREFS","FRAME/A/N,NCOLORS/K/N,MODE/K,DITHER/K,MODEID/K/N,FORCEBW/S",
                                                rx_setrenderprefs,
    "GETRENDERPREFS","FRAME/A/N,STEM/A",        rx_getrenderprefs,
    "VERSION",      NULL,                       rx_version,
    "QUIT",         "FORCE/S",                  rx_quit,
    "AUTHOR",       NULL,                       rx_author,
    NULL
};

char result[MAXPATHLEN+1];
struct List RexxWaitList;
struct RexxHost *rxhost = NULL;
Object *RexxFileReq = NULL;

const char *arnames[] = {
    "SLIDER",
    "STRING",
    "CHECKBOX",
    "CYCLE",
    "FLOAT",
    NULL
};

const Tag artags[] = {
    AR_SliderObject,
    AR_StringObject,
    AR_CheckBoxObject,
    AR_CycleObject,
    AR_FloatObject
};

/*
 *  English names for REXX.
 */

const char *ColorSpaceNamesE[] = {
    "Unknown",
    "Greyscale",
    "RGB",
    "Colormapped",
    "ARGB"
};
///


/*--------------------------------------------------------------------------*/

/// AllocRA() FreeRA() RexxVar()
Local
PPTREXXARGS *AllocRA(VOID)
{
    PPTREXXARGS *ra;
    ra = smalloc( sizeof(PPTREXXARGS) );
    if( ra ) bzero( ra, sizeof(PPTREXXARGS) );
    return ra;
}

Local
VOID FreeRA( PPTREXXARGS *ra )
{
    if( ra->result && ra->free_result ) sfree( ra->result );
    if( ra ) sfree( ra );
}

/*
    As utility/GetTagData()
*/
char *RexxVar( struct RexxMsg *rm, const char *name, const char *def )
{
    const char *ans;

    if( GetRexxVar( rm, name, &ans ) )
        ans = def;

    return ans;
}
///

/// ASKREQ

/*
    AskReq:
        TEXT/A,POS/K,NEG/K,GAD1,...

    TEXT = whatever
    RESULT ~= 0, if cancelled.
*/

Local
LONG rx_FetchLong(struct RexxMsg *rm, char *base, char *extension, LONG def)
{
    char varbuf[80],*ans;
    LONG res;

    sprintf(varbuf,"%s.%s",base,extension);
    if(GetRexxVar( rm, varbuf, &ans ) ) {
        res = def;
    } else {
        res = atol( ans );
    }
    return res;
}

Local
FLOAT rx_FetchFloat(struct RexxMsg *rm, char *base, char *extension, FLOAT def)
{
    char varbuf[80],*ans;
    FLOAT res;

    sprintf(varbuf,"%s.%s",base,extension);
    if(GetRexxVar( rm, varbuf, &ans ) ) {
        res = def;
    } else {
        res = atof( ans );
    }
    return res;
}

Local
char *rx_FetchString(struct RexxMsg *rm, char *base, char *extension, const char *def)
{
    char varbuf[80];

    sprintf(varbuf,"%s.%s",base,extension);
    return RexxVar(rm,varbuf,def);
}

Local PERROR
rx_PutString(struct RexxMsg *rm, char *base, char *extension, const char *val)
{
    char varbuf[80];

    sprintf(varbuf,"%s.%s",base,extension);
    D(bug("\trx_PutString(%s := %s)\n",varbuf,val));
    SetRexxVar( (struct Message *) rm, varbuf, val, strlen(val) );
    return PERR_OK;
}

Local PERROR
rx_PutLong(struct RexxMsg *rm, char *base, char *extension, LONG val)
{
    char varbuf[80],resbuf[30];

    sprintf(varbuf,"%s.%s",base,extension);
    sprintf(resbuf,"%ld",val);
    D(bug("\trx_PutLong(%s := %ld)\n",varbuf,val));
    SetRexxVar( (struct Message *) rm, varbuf, resbuf, strlen(resbuf) );
    return PERR_OK;
}

Local PERROR
rx_PutFloat(struct RexxMsg *rm, char *base, char *extension, FLOAT val)
{
    char varbuf[80],resbuf[30];

    sprintf(varbuf,"%s.%s",base,extension);
    sprintf(resbuf,"%lf",val);
    D(bug("\trx_PutLong(%s := %lf)\n",varbuf,val));
    SetRexxVar( (struct Message *) rm, varbuf, resbuf, strlen(resbuf) );
    return PERR_OK;
}

/*
    .TYPE=SLIDER
    .MAX=100
    .MIN=0
    .DEFAULT=0
 */
Local
PERROR parse_arslider(struct RexxMsg *rm,struct TagItem *tags,char *name,int s)
{
    D(bug("\tARSLIDER\n"));

    tags[s+0].ti_Tag = ARSLIDER_Max;
    tags[s+0].ti_Data = rx_FetchLong(rm,name,"MAX",100);
    tags[s+1].ti_Tag = ARSLIDER_Min;
    tags[s+1].ti_Data = rx_FetchLong(rm,name,"MIN",0);
    tags[s+2].ti_Tag = ARSLIDER_Default;
    tags[s+2].ti_Data = rx_FetchLong(rm,name,"DEFAULT",0);

    return PERR_OK;
}

/*
    .TYPE=FLOAT
    .MAX = 100.0
    .MIN = 0.0
    .DEFAULT = 0.0
    .FORMATSTRING = "%.3f"
 */

#define TO_INTEGER(x) (ULONG)( (x) * RX_DIVISOR)
#define TO_FLOAT(f)   (FLOAT)((FLOAT)(f) / (FLOAT)RX_DIVISOR)

Local
PERROR parse_arfloat(struct RexxMsg *rm,struct TagItem *tags,char *name, int s)
{
    D(bug("\tARFLOAT\n"));

    tags[s+0].ti_Tag = ARFLOAT_Min;
    tags[s+0].ti_Data = TO_INTEGER(rx_FetchFloat(rm,name,"MIN",0.0F));
    tags[s+1].ti_Tag = ARFLOAT_Max;
    tags[s+1].ti_Data = TO_INTEGER(rx_FetchFloat(rm,name,"MAX",100.0F));
    tags[s+2].ti_Tag = ARFLOAT_Default;
    tags[s+2].ti_Data = TO_INTEGER(rx_FetchFloat(rm,name,"DEFAULT",0.0F));
    tags[s+3].ti_Tag = ARFLOAT_FormatString;
    tags[s+3].ti_Data = (ULONG)rx_FetchString(rm,name,"FORMATSTRING","%.3f");
    tags[s+4].ti_Tag = ARFLOAT_Divisor;
    tags[s+4].ti_Data = RX_DIVISOR;

    return PERR_OK;
}

/*
    .TYPE=CHECKBOX
    .SELECTED=(1|0)
*/
Local
PERROR parse_archeckbox(struct RexxMsg *rm,struct TagItem *tags,char *name,int s)
{
    tags[s+0].ti_Tag = ARCHECKBOX_Selected;
    tags[s+0].ti_Data = rx_FetchLong(rm,name,"SELECTED",0);
    return PERR_OK;
}

/*
    .TYPE=STRING
    .MAXCHARS=number
    .INITIALSTRING=text
*/
Local
PERROR parse_arstring(struct RexxMsg *rm,struct TagItem *tags,char *name,int s)
{
    LONG mc;

    tags[s+0].ti_Tag = ARSTRING_MaxChars;
    mc = rx_FetchLong(rm,name,"MAXCHARS",80);
    if( mc > MAXPATHLEN ) mc = MAXPATHLEN;
    tags[s+0].ti_Data = mc;
    tags[s+1].ti_Tag = ARSTRING_InitialString;
    tags[s+1].ti_Data = (ULONG)rx_FetchString(rm,name,"INITIALSTRING","");
    return PERR_OK;
}

/*
    .TYPE=CYCLE
    .ACTIVE=number
    .POPUP=(0|1)
    .LABELS=A|B|C|D
*/

Local
PERROR parse_arcycle(struct RexxMsg *rm,struct TagItem *tags,char *name,int s)
{
    char *arg, gadgets[256], **labels;
    int i;

    labels = pzmalloc( (MAX_REXX_ASKREQ_CYCLELABELS+1)*sizeof(char *) );
    if(!labels)
        return PERR_OUTOFMEMORY;

    tags[s+0].ti_Tag  = ARCYCLE_Active;
    tags[s+0].ti_Data = rx_FetchLong(rm,name,"ACTIVE",0);
    tags[s+1].ti_Tag  = ARCYCLE_Popup;
    tags[s+1].ti_Data = rx_FetchLong(rm,name,"POPUP",1);
    tags[s+2].ti_Tag  = ARCYCLE_Labels;
    tags[s+2].ti_Data = (ULONG)labels;

    /*
     *  Build an array suitable for ARCYCLE_Labels.  The entries must be
     *  separated by a bar.
     *  BUG: strtok() is not a re-entrable function!
     */

    arg = rx_FetchString(rm,name,"LABELS","");
    if(!arg) return PERR_INVALIDARGS;
    strncpy(gadgets,arg,256);
    LOCKGLOB();
    for(arg = strtok(gadgets,"|"),i=0; arg && i<MAX_REXX_ASKREQ_CYCLELABELS; arg = strtok(NULL,"|"),i++ ) {
        labels[i] = smalloc(strlen(arg)+1);
        D(bug("\tAllocated %d bytes @%08X\n",strlen(arg)+1,labels[i]));
        if(!labels[i]) { UNLOCKGLOB(); return PERR_OUTOFMEMORY; }
        strcpy(labels[i],arg);
    }
    UNLOCKGLOB();

    return PERR_OK;
}

Local
void rx_askreq( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    struct TagItem win[MAX_REXX_ASKREQ_ITEMS+5] = {0};
    struct TagItem objs[MAX_REXX_ASKREQ_ITEMS+1][10] = {0};
    struct RexxAskReqValue *vals;
    int i, objstart, total_items,itemstart,argstart;
    PERROR res = PERR_OK;
    char *base;

    vals = pmalloc( (MAX_REXX_ASKREQ_ITEMS+1) * sizeof(struct RexxAskReqValue) );
    if(!vals) {
        ra->rc = -20;
        ra->rc2 = (long)ErrorMsg(PERR_OUTOFMEMORY,globxd);
        return;
    }
    D(bug("rx_askreq()\n"));

    /*
     *  Build the window information
     */

    win[0].ti_Tag = AR_Title; win[0].ti_Data = (ULONG)GetStr(mRX_REQUEST);

    if( ra->args[1] ) {
        win[1].ti_Tag = AR_Positive; win[1].ti_Data = ra->args[1];
    } else {
        win[1].ti_Tag = TAG_IGNORE;
    }

    if( ra->args[2] ) {
        win[2].ti_Tag = AR_Negative; win[2].ti_Data = ra->args[2];
    } else {
        win[2].ti_Tag = TAG_IGNORE;
    }

    win[3].ti_Tag = AR_Text; win[3].ti_Data = ra->args[0];

    itemstart = 4; // First free space in win[] for objects
    argstart  = 3; // First actual REXX stem for objects
    objstart  = 2; // Room for AROBJ_Value

    /*
     *  Set in actual objects
     */

    for(i = 0; i < MAX_REXX_ASKREQ_ITEMS; i++ ) {
        LONG t;

        if( base = (char *)ra->args[argstart+i] ) {
            D(bug("\tSTEM: %s\n",base));

            t = MatchStringList(rx_FetchString(rm,base,"TYPE",""),arnames);
            if( t == -1 ) {
                D(bug("Unknown object type specified\n"));
                res = PERR_UNKNOWNTYPE;
            } else {
                vals[i].type = win[i+itemstart].ti_Tag = artags[t];
                win[i+itemstart].ti_Data = (ULONG) objs[i];
                strncpy(vals[i].name,base,39);

                /*
                 *  Parse generic information
                 */

                objs[i][0].ti_Tag = AROBJ_Label;
                objs[i][0].ti_Data = (ULONG)rx_FetchString(rm,base,"LABEL","");

                objs[i][1].ti_Tag = AROBJ_Value;

                /*
                 *  Parse type - specific information
                 */

                switch(vals[i].type) {
                    case AR_SliderObject:
                        objs[i][1].ti_Data = (ULONG) &(vals[i].value);
                        res = parse_arslider(rm,objs[i],base,objstart);
                        break;

                    case AR_CheckBoxObject:
                        objs[i][1].ti_Data = (ULONG) &(vals[i].value);
                        res = parse_archeckbox(rm,objs[i],base,objstart);
                        break;

                    case AR_StringObject:
                        objs[i][1].ti_Data = (ULONG) &(vals[i].buffer[0]);
                        res = parse_arstring(rm,objs[i],base,objstart);
                        break;

                    case AR_CycleObject:
                        objs[i][1].ti_Data = (ULONG) &(vals[i].value);
                        res = parse_arcycle(rm,objs[i],base,objstart);
                        break;

                    case AR_FloatObject:
                        objs[i][1].ti_Data = (ULONG) &(vals[i].value);
                        res = parse_arfloat(rm,objs[i],base,objstart);
                        break;

                    default:
                        InternalError("Enum out of bounds!");
                        res = PERR_ERROR;
                }
            }

            if( res != PERR_OK ) {
                ra->rc = -10;
                ra->rc2 = (LONG)ErrorMsg(res,globxd);
                break;
            }
        } else {
            break;
        }
    }

    total_items = i;

    /*
     * Do the request, if there has been no errors so far
     */

    if(res == PERR_OK) {
        res = AskReqA(NULL,&win[0],globxd);
        sprintf(result,"%d",res);
        ra->result = result;
    }

    /*
     *  Do cleanup and writing the values back, if necessary
     */

    for(i = 0; i < total_items; i++) {
        switch(vals[i].type) {
            int j; char **labels;

            case AR_CycleObject:
                labels = (char **) objs[i][objstart+2].ti_Data;
                if(!CheckPtr(labels,"REXX/AR_CycleObject labels") )
                    break;

                for(j = 0; labels[j] != NULL; j++) {
                    D(bug("\tReleasing label @%08X\n",labels[j]));
                    sfree(labels[j]);
                }
                pfree(labels);
                /* FALLTHROUGH OK */

            case AR_SliderObject:
            case AR_CheckBoxObject:
                if( res == PERR_OK )
                    rx_PutLong(rm,vals[i].name,"VALUE",vals[i].value);
                break;

            case AR_StringObject:
                if( res == PERR_OK )
                    rx_PutString(rm,vals[i].name,"VALUE",vals[i].buffer);
                break;

            case AR_FloatObject:
                if( res == PERR_OK )
                    rx_PutFloat(rm,vals[i].name,"VALUE",TO_FLOAT(vals[i].value));
                break;

            default:
                break;
        }
    }

    pfree(vals);
}

///

/// SHOWERROR
/*
    Showerror
        LINE/N,TEXT/A
*/

Local
void rx_showerror( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    char linebuf[80] = "";

    if( ra->args[1] )
        sprintf(linebuf,GetStr(mRX_ERROR_ON_LINE),NUMERIC(ra->args[1]));

    Req(NEGNUL,GetStr(MSG_OK_GAD),
        ISEQ_U "\n" ISEQ_B ISEQ_C "  %s  \n" ISEQ_N
        "\n"
        ISEQ_I "%s\n" ISEQ_N
        "%s",
        GetStr(mRX_SHOWERROR_TEXT),
        ra->args[0],
        linebuf);
}

///


/// LIST*

/*
    Lists the loaders. The stem.0 will be the amount of effects with
    the name of the first loader in stem.1, the next in stem.2 etc.

    You may specify loaders/savers by using the switches.
*/
Local
void rx_listloaders( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    struct Stem *stem;
    struct Node *cn = globals->loaders.lh_Head, *nn;
    char buf[8];
    long count = 1;
    BOOL showloaders, showsavers;

    showloaders = ra->args[1];
    showsavers  = ra->args[2];

    stem = AllocStem( (UBYTE *)ra->args[0] ); /* BUG: should check */

    while( nn = cn->ln_Succ ) {
        BOOL ok;
        ok = TRUE;

        if( showloaders ) {
            if( !((LOADER *)cn)->canload )
                ok = FALSE;
        }

        if( showsavers ) {
            if( ((LOADER *)cn)->saveformats == CSF_NONE ) {
                    ok = FALSE;
            }
        }

        if(ok) {
            sprintf(buf,"%u",count);
            AddStemItem( stem, buf, (ULONG) cn->ln_Name, ASI_STRING ); /* This is a string. BUG: should check  */
            count++;
        }
        cn = nn;
    }

    /* Finally, add the amount of effects. */
    AddStemItem( stem, "0", count-1, ASI_INT );

    ra->stem = stem;
}

/*
    Lists the effects. The stem.0 will be the amount of effects with
    the name of the first effect in stem.1, the next in stem.2 etc.
*/
Local
void rx_listeffects( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    struct Stem *stem;
    struct Node *cn = globals->effects.lh_Head, *nn;
    char buf[8];
    long count = 1;

    stem = AllocStem( (UBYTE *)ra->args[0] ); /* BUG: should check */

    while( nn = cn->ln_Succ ) {
        sprintf(buf,"%u",count);
        AddStemItem( stem, buf, (ULONG) cn->ln_Name, ASI_STRING ); /* This is a string. BUG: should check  */
        cn = nn;
        count++;
    }

    /* Finally, add the amount of effects. */
    AddStemItem( stem, "0", count-1, ASI_INT );

    ra->stem = stem;
}
///

/// Rendering routines

Local
void rx_closerender( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;

    CloseRender( frame, globxd );
    DisableMenuItem( MID_CLOSERENDER );
}

Local
void rx_render_to_front( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;
    struct RenderObject *rdo;
    PERROR res;

    if( rdo = frame->renderobject ) {
        if( rdo->ActivateDisplay ) {
            if( (res = rdo->ActivateDisplay(rdo)) != PERR_OK ) {
                ra->rc = -10;
                ra->rc2 = (long)ErrorMsg( res, globxd );
            }
        }
    } else {
        ra->rc = -10;
        ra->rc2 = (long)"No rendered image";
    }
}

Local
void rx_render( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;

    LOCK(frame);

    D(bug("RX_RENDER: Frame %08X\n",frame));

    OpenRender( frame, globxd );

    if( DoRender( frame->renderobject ) != PERR_OK) {
        ra->rc = -10; ra->rc2 = (long)"Cannot spawn subtask";
        ra->proc = NULL;
    } else {
        ra->frame = frame;
        ra->proc  = frame->currproc;
        EnableMenuItem( MID_CLOSERENDER );
    }

    UNLOCK(frame);
}

/*
    SETRENDERPREFS FRAME/A/N,NCOLORS/K/N,MODE/K,DITHER/K,MODEID/K/N

    BUG: Definately should use the labels in gui.c
*/

const char *modes[] = { "Color", "EHB", "HAM6", "HAM8", NULL };
const char *dithers[] = { "None", "Floyd-Steinberg", NULL };

Local
void rx_setrenderprefs( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;
    LONG nColors, mode = -1, dither = -1;
    ULONG modeid, depth;
    BOOL forcebw;

    /*
     *  First, set up the number of colors.
     */

    if( ra->args[1] )
        nColors = NUMERIC(ra->args[1]);
    else
        nColors = frame->disp->ncolors;

    /*
     *  Then, check up the display mode.
     */

    if( ra->args[2] ) {
        mode = MatchStringList( STRING(ra->args[2]), modes );
        if( mode == -1 ) {
            ra->rc  = -10;
            ra->rc2 = (long)"Invalid mode";
            return;
        }
    } else {
        mode = frame->disp->renderq;
    }

    /*
     *  Select the Dither mode.
     */

    if( ra->args[3] ) {
        dither = MatchStringList( STRING(ra->args[3]), dithers );
        if( dither == -1 ) {
            ra->rc  = -10;
            ra->rc2 = (long)"Invalid dither type";
            return;
        }
    } else {
        dither = frame->disp->dither;
    }

    /*
     *  Set up the display ID.
     */

    if( ra->args[4] ) {
        modeid = (ULONG) NUMERIC( ra->args[4] );
    } else {
        modeid = frame->disp->dispid;
    }

    /*
     *  Check for FORCEBW
     */

    forcebw = (BOOL) ra->args[5];

    /*
     *  Check for combinations.
     *  BUG: There's duplicate code in main.c (dispprefswindow handling).
     */

    switch(mode) {
        case RENDER_NORMAL: /* Normal color */
            modeid &= ~(HAM_KEY);
            depth   = GetMinimumDepth(nColors);
            break;

        case RENDER_EHB:
            nColors = 64;
            depth   = 6;
            break;

        case RENDER_HAM6:
            modeid  |= HAM_KEY;
            nColors = 16;
            depth   = 6;
            break;

        case RENDER_HAM8:
            modeid  |= HAM_KEY;
            depth   = 8;
            nColors = 64;
            break;

        default:
            InternalError("Invalid render mode in SetRenderPrefs()");
            ra->rc = 10;
            return;
    }


    /*
     *  Set up the render preferences.
     */

    frame->disp->ncolors = nColors;
    frame->disp->depth   = depth;
    frame->disp->renderq = mode;
    frame->disp->dither  = dither;
    frame->disp->dispid  = modeid;
    frame->disp->forcebw = forcebw;
}


/*
    GETRENDERPREFS FRAME/A/N,STEM/A

    Returns a stem with the following elements:

        MODEID = screen mode id
        DEPTH  = screen depth
        NCOLORS = number of colors
        DITHER = current dither mode
        MODE   = which screen type is to be used?
        FORCEBW = Force B/W
*/
Local
void rx_getrenderprefs( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;
    struct Stem *stem;

    stem = AllocStem( STRING( ra->args[1] ));
    if(!stem) {
        ra->rc = -20; ra->rc2 = (long)ErrorMsg( PERR_OUTOFMEMORY, globxd );
        return;
    }

    AddStemItem( stem, "MODEID", frame->disp->dispid, ASI_INT );
    AddStemItem( stem, "DEPTH", frame->disp->depth, ASI_INT );
    AddStemItem( stem, "NCOLORS", frame->disp->ncolors, ASI_INT );
    AddStemItem( stem, "DITHER", (ULONG)dithers[frame->disp->dither], ASI_STRING );
    AddStemItem( stem, "MODE", (ULONG)modes[frame->disp->renderq], ASI_STRING );
    AddStemItem( stem, "FORCEBW", frame->disp->forcebw, ASI_INT );

    ra->stem = stem;
}

///

/// Info routines (frameinfo,effectinfo,loaderinfo)

/*
    This should return a STEM full of frame info.
    BUG: AddStemItem return values are ignored.
*/
Local
void rx_frameinfo( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;
    struct Stem *stem;

    D(bug("FrameInfo()\n"));

    stem = AllocStem( (UBYTE *)ra->args[1] ); /* BUG: Should check */

    SHLOCK(frame);
    AddStemItem( stem, "NAME", (ULONG)frame->name, ASI_STRING );
    AddStemItem( stem, "HEIGHT", (ULONG)frame->pix->height, ASI_INT );
    AddStemItem( stem, "WIDTH", (ULONG)frame->pix->width, ASI_INT );
    AddStemItem( stem, "COLORSPACE", (ULONG)ColorSpaceNamesE[frame->pix->colorspace], ASI_STRING );
    AddStemItem( stem, "COMPONENTS", (ULONG)frame->pix->components, ASI_INT );
    AddStemItem( stem, "DPIX", (ULONG)frame->pix->DPIX, ASI_INT );
    AddStemItem( stem, "DPIY", (ULONG)frame->pix->DPIY, ASI_INT );
    AddStemItem( stem, "BYTESPERROW", (ULONG)frame->pix->bytes_per_row, ASI_INT );
    AddStemItem( stem, "PATH", (ULONG)frame->path, ASI_STRING );
    AddStemItem( stem, "HIDDEN", (ULONG) (frame->disp->win ? FALSE : TRUE), ASI_INT );
    UNLOCK(frame);

    ra->stem = stem;
}

Local
void rx_effectinfo( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    EXTERNAL    *effect;
    struct Stem *stem = NULL;
    struct Library *EffectBase;

    effect = (EXTERNAL *)FindName( &globals->effects, (UBYTE *)ra->args[0] );
    if(!effect) {
        ra->rc  = -10;
        ra->rc2 = (long)"Effect not found";
    } else {
        EffectBase = OpenModule( effect, 0L, globxd );
        if( !EffectBase ) {
            ra->rc = -10;
            ra->rc2 = (long)"Couldn't open effect";
        } else {
            stem = AllocStem( (UBYTE *) ra->args[1] );
            AddStemItem( stem, "NAME", (ULONG) effect->nd.ln_Name, ASI_STRING );
            AddStemItem( stem, "VERSION", EffectBase->lib_Version, ASI_INT );
            AddStemItem( stem, "REVISION", EffectBase->lib_Revision, ASI_INT );
            AddStemItem( stem, "AUTHOR", EffectInquire( PPTX_Author, globxd ) , ASI_STRING );
            AddStemItem( stem, "INFOTXT", EffectInquire( PPTX_InfoTxt, globxd ), ASI_STRING );
            AddStemItem( stem, "REXXTEMPLATE", EffectInquire( PPTX_RexxTemplate, globxd), ASI_STRING );
            AddStemItem( stem, "GETARGS", EffectInquire( PPTX_SupportsGetArgs, globxd), ASI_INT );
            CloseModule( EffectBase, globxd );
        }

        ra->stem = stem;
    }
}

Local
VOID rx_AddCSName( char *buf, ULONG cs )
{
    strcat(buf,ColorSpaceNamesE[cs]);
    strcat(buf,"|");
}

Local
VOID rx_DecodeColorSpaceBits( char *buf, ULONG csf )
{
    strcpy(buf, "");

    if (csf & CSF_RGB)
        rx_AddCSName( buf, CS_RGB );
    if (csf & CSF_GRAYLEVEL)
        rx_AddCSName( buf, CS_GRAYLEVEL );
    if (csf & CSF_LUT)
        rx_AddCSName( buf, CS_LUT );
    if (csf & CSF_ARGB)
        rx_AddCSName( buf, CS_ARGB );

    if (csf)
        buf[strlen(buf) - 1] = '\0';    // Remove last comma.

}

Local
void rx_loaderinfo( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    EXTERNAL    *loader;
    struct Stem *stem = NULL;
    struct Library *IOModuleBase;
    char buf[256];

    loader = (EXTERNAL *)FindName( &globals->loaders, (UBYTE *)ra->args[0] );
    if(!loader) {
        ra->rc  = -10;
        ra->rc2 = (long)"Loader not found";
    } else {
        IOModuleBase = OpenModule( loader, 0L, globxd );
        if( !IOModuleBase ) {
            ra->rc = -10;
            ra->rc2 = (long)"IO Module not found!";
        } else {
            stem = AllocStem( (UBYTE *) ra->args[1] );
            AddStemItem( stem, "NAME", (ULONG) loader->nd.ln_Name, ASI_STRING );
            AddStemItem( stem, "VERSION", IOModuleBase->lib_Version, ASI_INT );
            AddStemItem( stem, "REVISION", IOModuleBase->lib_Revision, ASI_INT );
            AddStemItem( stem, "AUTHOR", IOInquire( PPTX_Author, globxd ), ASI_STRING );
            AddStemItem( stem, "INFOTXT", IOInquire(PPTX_InfoTxt, globxd), ASI_STRING );
            AddStemItem( stem, "REXXTEMPLATE", IOInquire( PPTX_RexxTemplate, globxd), ASI_STRING );
            AddStemItem( stem, "LOAD", IOInquire( PPTX_Load, globxd ), ASI_INT );
            rx_DecodeColorSpaceBits(buf, IOInquire( PPTX_ColorSpaces, globxd ) );
            AddStemItem( stem, "SAVEFORMATS", (ULONG)buf, ASI_STRING );
            AddStemItem( stem, "PREFERREDPOSTFIX", IOInquire( PPTX_PreferredPostFix, globxd), ASI_STRING );
            CloseModule( IOModuleBase, globxd );
        }
        ra->stem = stem;
    }
}

///

/// CROP

Local
void rx_crop( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame, *newframe;

    if( newframe = Edit(frame, EDITCMD_CROPFRAME) ) {
        ReplaceFrame(frame, newframe);
        UpdateInfoWindow( newframe->mywin,globxd);
        UpdateMainWindow( newframe );
        DisplayFrame( newframe );
        CloseRender( newframe,globxd ); /* BUG: Maybe should call HandleMainIDCMP(GID_CLOSE_RENDER)? */
    } else {
        ra->rc = -10;
        ra->rc2 = (ULONG)"Couldn't crop?!?";
    }

}
///

/// SETAREA, GETAREA
/*
    Return currently selected area
    BUG: AddStemItem() return values are ignored

    GETAREA FRAME/A/N,STEM/A
*/
Local
void rx_getarea( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;
    struct Stem *stem;

    stem = AllocStem( (UBYTE *)ra->args[1] );
    if(!stem) {
        ra->rc = -20; ra->rc2 = (long)ErrorMsg( PERR_OUTOFMEMORY, globxd );
        return;
    }

    SHLOCK(frame);
    AddStemItem( stem, "MINX", (ULONG)frame->selbox.MinX, ASI_INT );
    AddStemItem( stem, "MINY", (ULONG)frame->selbox.MinY, ASI_INT );
    AddStemItem( stem, "MAXX", (ULONG)frame->selbox.MaxX, ASI_INT );
    AddStemItem( stem, "MAXY", (ULONG)frame->selbox.MaxY, ASI_INT );
    UNLOCK(frame);

    ra->stem = stem;
}

Local
void rx_setarea( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;

    LOCK(frame);

    if(ra->args[5]) { /* ALL defined */
        frame->selbox.MinX = frame->selbox.MinY = 0L;
        frame->selbox.MaxX = frame->pix->width;
        frame->selbox.MaxY = frame->pix->height;
        /* BUG: should set infowin also */
    } else {
        if(ra->args[1] && ra->args[2]) {
            frame->selbox.MinX = (WORD) *((ULONG*)(ra->args[1]));
            frame->selbox.MinY = (WORD) *((ULONG*)(ra->args[2]));
        } else {
            ra->rc = -10; /* Not enough points defined. */
            ra->rc2= (ULONG) "Not enough points";
            UNLOCK(frame);
            return;
        }
        if(ra->args[3] && ra->args[4]) {
            frame->selbox.MaxX = (WORD) *((ULONG*)(ra->args[3]));
            frame->selbox.MaxY = (WORD) *((ULONG*)(ra->args[4]));
        } else {
            ra->rc = -10;
            ra->rc2= (ULONG) "Not enough points";
            UNLOCK(frame);
            return;
        }
    }
    UNLOCK(frame);
}
///

/// Frame handling functions: DELETE,RENAME,COPY
/* Delete a frame. */
Local
void rx_deleteframe( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;
    ULONG res;

    if(!ra->args[1]) { /* FORCE was not defined */
        res = Req( GetFrameWin(frame), "Yes|No", "\nAre you sure you want to delete frame\n'%s'?\n",frame->nd.ln_Name);
        if( res == 0 )
            return;
    }
    DeleteFrame( frame );
}


Local
void rx_renameframe( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame;
    APTR  entry;
    char  tmpbuf[NAMELEN];

    strncpy( tmpbuf, (STRPTR) ra->args[1], NAMELEN-1 );

    LOCK(frame);

    LockList( framew.Frames );

    if( entry = (APTR) FirstEntry( framew.Frames ) ) {
        do {
            if( strcmp( ParseListEntry(entry), frame->name ) == 0 ) {
                MakeFrameName( tmpbuf, tmpbuf, NAMELEN-1, globxd );
                strncpy( frame->name, tmpbuf, NAMELEN-1 );
                // BUG: following may fail.  Also, should probably use DoMainList()
                ReplaceEntry( framew.win, framew.Frames, entry, ConstructListName( frame ) );
                UpdateFrameInfo( frame );
                break;
            }
        } while( entry = (APTR)NextEntry( framew.Frames, entry ) );
    }

    UnlockList( framew.win, framew.Frames );

    UNLOCK(frame);
}

Local
void rx_copyframe( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame = ra->frame, *newframe;

    LOCK(frame);

    if( newframe = DupFrame( frame, DFF_COPYDATA|DFF_MAKENEWNAME, globxd ) ) {
        AllocInfoWindow( newframe, globxd );
        AddFrame( newframe );
        DoMainList( newframe );
        // GuessDisplay( newframe );
        UpdateMainWindow( newframe );
        newframe->reqrender = 1;
        DisplayFrame( newframe );
        sprintf(result,"%lu", newframe->ID );
        ra->result = result;
    } else {
        ra->rc = -20;
        ra->rc2= (ULONG) "Couldn't allocate new frame";
    }

    UNLOCK(frame);
}
///

/// File handling

/*
    TITLE/A,POSITIVE=POS,INITIALDRAWER=ID/K,INITIALFILE=IF/K,INITIALPATTERN=IP/K,SAVE/S
 */


Local
void rx_askfile( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    UBYTE *initialdrawer, *initialfile, *path;

    if( !RexxFileReq ) {
        GetAttr( FRQ_Drawer, gvLoadFileReq.Req, (ULONG *)&initialdrawer );
        GetAttr( FRQ_File, gvLoadFileReq.Req, (ULONG *)&initialfile );

        RexxFileReq = FileReqObject,
            ASLFR_Locale,       globxd->locale,
            ASLFR_InitialDrawer,initialdrawer,
            ASLFR_InitialFile,  initialfile,
        EndObject;
        if(!RexxFileReq) {
            ra->rc = -20;
            ra->rc2= (LONG)"Couldn't alloc file requester";
            return;
        }
    }

    SetAttrs( RexxFileReq,
              ASLFR_Screen,       MAINSCR,
              ASLFR_TitleText,    ra->args[0],
              ra->args[1] ? TAG_IGNORE : TAG_SKIP, 1,
              ASLFR_PositiveText, ra->args[1],
              ra->args[2] ? TAG_IGNORE : TAG_SKIP, 1,
              ASLFR_InitialDrawer,ra->args[2],
              ra->args[3] ? TAG_IGNORE : TAG_SKIP, 1,
              ASLFR_InitialFile,  ra->args[3],
              ASLFR_InitialPattern,ra->args[4] ? ra->args[4] : (LONG)"#?",
              ASLFR_DoPatterns,   ra->args[4] ? TRUE : FALSE,
              ASLFR_DoSaveMode,   ra->args[5],
              TAG_DONE );

    if( DoRequest(RexxFileReq) != FRQ_OK ) {
        ra->rc = -5;
        ra->rc2 = (LONG)ErrorMsg(PERR_CANCELED,globxd);
    } else {
        GetAttr(FRQ_Path, RexxFileReq, (ULONG *)&path );
        strncpy(result, path, MAXPATHLEN);
        ra->result = result;
    }
}

Local
void rx_load( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    FRAME *frame;

    frame = RunLoad( (UBYTE *)ra->args[0], NULL, "REXX" );
    if(frame) {
        SHLOCK(frame);
        ra->frame = frame;
        ra->proc  = frame->currproc; /* Signal : not immediate command. */
        UNLOCK(frame);
    } else {
        ra->proc  = NULL;
        ra->rc    = 10;
    }
}

Local
void rx_saveas( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    UBYTE argstr[MAXPATHLEN+NAMELEN+80];
    UBYTE path[MAXPATHLEN+1],*s;
    FRAME *frame = ra->frame;

    D(bug("SAVE AS frame %lu, path=%s, type=%s (%s), args=%s\n",
           ra->args[0],ra->args[1], ra->args[2], ra->args[3] ? "COLORMAPPED" : "TRUECOLOR",
           ra->args[4] ? ra->args[4] : (ULONG)"N/A" ));

    /*
     *  Separate the name and path from each other, as the save command
     *  needs them separate.
     */

    strncpy(path,(STRPTR) ra->args[1],MAXPATHLEN);
    s = PathPart(path);
    if( s ) *s = '\0';

    /*
     *  Prepare the message for the save handler process.
     */

    sprintf(argstr,"PATH=\"%s\" FORMAT=%s TYPE=%d ARGS=\"%s\" NAME=\"%s\"",
                    path, ra->args[2],
                    ra->args[3], ra->args[4] ? ra->args[4] : (ULONG)"",
                    FilePart( (STRPTR) ra->args[1]) );

    if(RunSave( frame, argstr ) == PERR_OK ) {
        ra->proc  = frame->currproc;
    } else {
        ra->proc = NULL;
        ra->rc   = 10;
    }
}

Local
void rx_save( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    // UBYTE argstr[300]; /* BUG: Magic */
    // FRAME *frame = ra->frame;

    D(bug("SAVE frame %lu, (%s), args=%s\n",
           ra->args[0],ra->args[1] ? "COLORMAPPED" : "TRUECOLOR",
           ra->args[2] ));

    /*
     *  Prepare the message for the save handler process.
     */

#if 0
    sprintf(argstr,"PATH=\"%s\" FORMAT=%s TYPE=%d ARGS=\"%s\"",
                    ra->args[1], ra->args[2],
                    ra->args[3], ra->args[4] );

    if(RunSave( frame, argstr ) == PERR_OK ) {
        ra->frame = frame;
        ra->proc  = frame->currproc;
    } else {
        ra->proc = NULL;
        ra->rc   = 10;
    }
#endif
}

///

/// QUIT, PPT_TO_FRONT, PPT_TO_BACK

Local
void rx_quit( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    extern int quit;

    if( !ra->args[0]) {
        if( Req( NEGNUL, "Yes|No", "\nDo you "ISEQ_I"really"ISEQ_N" wish to quit?\n") ) {
            quit = 1;
        }
    } else
        quit = 1;
    if(!quit) {
        ra->rc = -5; /* Signal: didn't quit */
        ra->rc2 = (ULONG) "User cancelled quit";
    }
}

Local
void rx_ppt_to_front( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    if( MAINSCR )
        ScreenToFront( MAINSCR );
}

Local
void rx_ppt_to_back( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    if( MAINSCR )
        ScreenToBack( MAINSCR );
}

///

/// PROCESS

/*
    Start processing of a frame.

    Note that the rxargs is not released until the effect has stopped!
*/
Local
void rx_process( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    char argstr[MAXPATHLEN], *rxargs = NULL;
    FRAME *frame;

    frame = ra->frame;

    /*
     *  Allocate room for the argument string, or if it does not exist,
     *  for the NULL string.
     */

    if( rxargs = smalloc( (ra->args[2] ? strlen(STR(ra->args[2])) : 0) + 1 ) ) {
        strcpy( rxargs, ra->args[2] ? STR(ra->args[2]) : (STRPTR)"" );
    } else {
        ra->rc = 10; ra->rc2 = PERR_OUTOFMEMORY;
        ra->proc = NULL;
        return;
    }

    LOCK(frame);

    D(bug("Calling frame %08X, effect %s with ", frame, ra->args[1]));
    D(bug("args '%s'\n",rxargs));
    sprintf(argstr,"NAME=\"%s\" REXX ARGS=%lu",ra->args[1],rxargs);

    if( RunFilter( frame, argstr ) != PERR_OK) {
        ra->rc = -10; ra->rc2 = (long)"Cannot spawn subtask";
        ra->proc = NULL;
    } else {
        ra->frame = frame;
        ra->proc  = frame->currproc;
        ra->process_args = rxargs;
    }

    UNLOCK(frame);
}
///

/*
    Get the arguments from the external module.
 */

Local
void rx_getargs( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    EXTERNAL    *module;
    char        argstr[MAXPATHLEN], *rxargs = NULL;
    FRAME       *frame;

    D(bug("GETARGS\n"));

    if( !(frame = ra->frame) ) {
        if( NULL == (frame = NewFrame( 0, 0, 0, globxd ))) {
            ra->rc = 10; ra->rc2 = (long) "Couldn't allocate a temporary frame";
            return;
        }

        frame->istemporary = TRUE;
    }

    /*
     *  Allocate room for the argument string, or if it does not exist,
     *  for the NULL string.
     */

    if( rxargs = smalloc( (ra->args[2] ? strlen(STR(ra->args[2])) : 0) + 1 ) ) {
        strcpy( rxargs, ra->args[2] ? STR(ra->args[2]) : (STRPTR)"" );
    } else {
        ra->rc = 10; ra->rc2 = PERR_OUTOFMEMORY;
        ra->proc = NULL;
        return;
    }

    if( module = (EXTERNAL *)FindIName( &globals->effects, (UBYTE *)ra->args[1] ) ) {
        sprintf(argstr,"%lu",rxargs);
        if( RunGetArgs( frame, module, argstr ) != PERR_OK) {
            ra->rc = -10; ra->rc2 = (long)"Cannot spawn subtask";
            ra->proc = NULL;
        } else {
            ra->frame = frame;
            ra->proc  = frame->currproc;
            ra->process_args = rxargs;
        }
    } else if( module = (EXTERNAL *)FindIName( &globals->loaders, (UBYTE *)ra->args[1] )) {

    } else {
        ra->rc = -10;
        ra->rc2 = (long)"External not found";
    }
}

/// HIDE & SHOW

/*
    We will fail quietly, as it is less intrusive to the
    user.
 */

Local
void rx_hide( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    HideDisplayWindow( ra->frame );
}

Local
void rx_show( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    ShowDisplayWindow( ra->frame );
}

///

/// Miscallaneous

/* Return version information. */
Local
void rx_version( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    sprintf(result,"%u",VERNUM);
    ra->result = result;
}

/* Return author name. Boring. */
Local
void rx_author( PPTREXXARGS *ra, struct RexxMsg *rm )
{
    ra->result = "Janne Jalkanen";
}

///

/// DoRexxCommand()
/*
    Dispatch commands. Return TRUE for immediate commands.
*/

Local
BOOL DoRexxCommand( struct RexxMsg *msg, char *arg0, PPTREXXARGS *ra )
{
    char *args;
    ULONG *optarray = NULL;
    int i = 0;

    D(bug("REXX Msg: %s\n",arg0));

    ra->args  = NULL;
    ra->frame = NULL;

    /*
     *  Parse messages.
     */

    args = strchr( arg0, ' ' ); /* Take away first cmd */
    if(args)
        *args++ = '\0'; /* Put a NIL on the first space found. */
    else
        args = "";

    while( rx_commands[i].name ) {
        long arg0len, commandlen;

        arg0len = strlen(arg0);
        commandlen = strlen( rx_commands[i].name );

        if(strnicmp( rx_commands[i].name, arg0, MAX( arg0len, commandlen )) == 0) {

            /*
             *  Match found, thus we shall check the commands.
             *  Check first if there are any arguments. If there are,
             *  call ReadArgs()
             */

            if( !(optarray = ParseDOSArgs( args, rx_commands[i].args ?
                                                 rx_commands[i].args : "", globxd ))) {
                D(bug("ReadArgs() failed\n"));
                ra->rc = -10;
                ra->rc2 = (ULONG) "Arguments do not match template";
                goto fail;
            } else {
                ra->args = optarray;
            }

            /*
             *  Check if the command requires a frame. If it does,
             *  extract the info and store it in the rexxargs structure.
             */

            if( rx_commands[i].args ) {
                if( strncmp( rx_commands[i].args, "FRAME", 5 ) == 0 ) {

                    if( args ) {
                        FRAME *frame;

                        frame = FindFrame( FRAMEID(optarray[0]) );
                        if(!frame) {
                            ra->rc = -10; ra->rc2 = (long)"Frame not found";
                            goto fail;
                        }

                        if( !CheckPtr(frame,"REXX: FindFrame() result") ) {
                            ra->rc = -10; ra->rc2 = (long)"Memory access error";
                            goto fail;
                        }
                        ra->frame = frame;
                    } else {
                        ra->rc = -10; ra->rc2 = (long)"Expected a frame id";
                        goto fail;
                    }
                }
            }

            if(rx_commands[i].func) {
                (*rx_commands[i].func)( ra, msg );
            } else {
                D(bug("\tUnsupported command %s!",arg0));
                ra->rc = -10;
                ra->rc2 = (ULONG) "Unsupported command";
            }

            /*
             *  Free any arguments we reserved
             */
fail:
            if(!ra->proc && optarray) {
                FreeDOSArgs( optarray, globxd );
            }

            if(ra->proc)
                return FALSE;

            return(TRUE);
        }
        i++;
    }

    /*
     *  We fall through here if the code was unsupported
     */

    ra->rc  = -5;
    ra->rc2 = (long)"Unknown command";

    return TRUE;
}
///
/// HandleRexxCommands( struct RexxHost *)
/*
    Main AREXX event dispatcher.

    If the REXX command received was of immediate type, we'll execute
    it and reply to it, but if it was not (a process was started), then we
    put into the wait list to wait the completion.
*/

void HandleRexxCommands( struct RexxHost *host )
{
    int immediate;
    struct RexxMsg *rmsg;

    /* Get RX message */

    while( rmsg = (struct RexxMsg *)GetMsg( host->port ) ) {

        /*
         *  If this is a reply, remove it since we did allocate the space
         */

        if(rmsg->rm_Node.mn_Node.ln_Type == NT_REPLYMSG ) {
            /* Destroy the message */
            FreeRexxCommand( rmsg );
            --host->replies;
        } else {
            PPTREXXARGS *ra;

            if(!(ra = AllocRA() ))
                return;

            /*
             *  It must be a command for us.
             *  Send it over to DoRexxCommand() for parsing and execution
             */

            immediate = DoRexxCommand( rmsg, ARG0(rmsg), ra );

            /* if this command was immediate, reply to rexx message */

            if( immediate ) {
                D(bug("\tImmediate command executed\n"));
                RexxReply( rmsg, ra );
                FreeRA( ra );
            } else {
                D(bug("\tMoving REXX to wait list\n"));
                /*
                 *  Else move this to rexx wait list to wait for task completion
                 */
                LOCKGLOB();
                Forbid();
                ra->msg = rmsg;
                Remove( (struct Node *)rmsg );
                AddTail( &RexxWaitList, (struct Node *)ra );
                Permit();
                UNLOCKGLOB();
            }

        } /* if !replymsg */
    }
}
///

/// SimulateRexxCommand()
/*
    Use to put a pseudo rexx command into the queue.  This is necessary
    if PPT starts an effect by using its own internal args.

    See Composite() for example.

    Returns a pointer to the added PPTREXXARGS.
 */

Prototype PPTREXXARGS *SimulateRexxCommand( FRAME *frame, char *command );

PPTREXXARGS *
SimulateRexxCommand( FRAME *frame, char *command )
{
    PPTREXXARGS *ra = NULL;

    if( ra = AllocRA() ) {
        ra->frame = frame;

        if(ra->process_args = smalloc( strlen(command)+1 )) {
            strcpy( ra->process_args, command );

            LOCKGLOB();
            AddTail( &RexxWaitList, (struct Node *) ra );
            UNLOCKGLOB();
        } else {
            FreeRA( ra );
            ra = NULL;
        }

    }

    return ra;
}
///

/// RexxReply() & ReplyRexxCommand()
/*
    Master reply interface. Does STEM and possible future VAR
    setting correctly.
*/

#define STEMITEM(x)         ((struct StemItem *)(x))

Local
void RexxReply( struct RexxMsg *msg, PPTREXXARGS *ra )
{
    ULONG rc = 0;

    D(bug("RexxReply()\n"));

    if( ra->stem ) {
        struct Node *nd = (struct Node *)ra->stem->list.mlh_Head, *ln;

        while( ln = nd->ln_Succ ) {
            UBYTE buffer[80];
            sprintf(buffer,"%s.%s", ra->stem->name, STEMITEM(nd)->name );
            // D(bug("\tSTEM   %s := %s\n",buffer, STEMITEM(nd)->value));
            rc |= SetRexxVar( (struct Message *)msg,
                              buffer,
                              ((struct StemItem *)nd) -> value,
                              strlen( ((struct StemItem *)nd) ->value ) );
            nd = ln;
        }

        FreeStem( ra->stem );
        ra->stem = NULL;

        if(rc) {
            ra->rc = -10;
            ra->rc2 = (long)"STEM setting failed!";
        }
    }

    ReplyRexxCommand( msg, ra->rc, ra->rc2, ra->result );
}

/*
    Replies to a REXX command sent from outside.
    primary   --> RC  (always)
    secondary --> RC2 (if primary != 0 ) (RESULT will be 0)
    result    --> RESULT ( if primary == 0 )
*/

Local
void ReplyRexxCommand( struct RexxMsg  *rexxmessage,
                       long            primary,
                       long            secondary,
                       char            *result )
{
    D(bug("ReplyRexxCommand()\n"));

    if( rexxmessage->rm_Action & RXFF_RESULT ) {
        if( primary == 0 ) { /* No vika, thus put result into secondary */
            secondary = result ?
                        (long) CreateArgstring( result, strlen(result) ) : (long) NULL;
        } else {
            char buf[16];

            if( primary > 0 ) {
                sprintf( buf, "%ld", secondary );
                result = buf;
            } else {
                primary = -primary;
                result = (char *) secondary;
            }

            SetRexxVar( (struct Message *) rexxmessage, "RC2", result, strlen(result) );

            secondary = 0;
        }
    } else if( primary < 0 )
        primary = -primary;

    rexxmessage->rm_Result1 = primary; /* RC */
    rexxmessage->rm_Result2 = secondary; /* RESULT */
    D(bug("\tRC=%ld, RESULT=%ld\n",primary,secondary));
    ReplyMsg( (struct Message *) rexxmessage );
}
///

/// FreeRexxCommand()
/*
    Free REXX command message.
*/

Local
void FreeRexxCommand( struct RexxMsg *rexxmessage )
{
    if( !rexxmessage->rm_Result1 && rexxmessage->rm_Result2 )
        DeleteArgstring( (char *) rexxmessage->rm_Result2 );

    if( rexxmessage->rm_Stdin )
        Close( rexxmessage->rm_Stdin );

    if( rexxmessage->rm_Stdout && rexxmessage->rm_Stdout != rexxmessage->rm_Stdin )
        Close( rexxmessage->rm_Stdout );

    DeleteArgstring( (char *) ARG0(rexxmessage) );
    DeleteRexxMsg( rexxmessage );
}
///

/// FindRexxWaitItem()
Prototype PPTREXXARGS *FindRexxWaitItemFrame( FRAME *frame );

PPTREXXARGS *FindRexxWaitItemFrame( FRAME *frame )
{
    struct Node *cn = RexxWaitList.lh_Head;

    SHLOCKGLOB();

    while(cn->ln_Succ) {
        PPTREXXARGS *ra;
        ra = (PPTREXXARGS *)cn;
        if(ra->frame == frame) {
            UNLOCKGLOB();
            return ra;
        }
        cn = cn->ln_Succ;
    }
    UNLOCKGLOB();
    return NULL;
}

PPTREXXARGS *FindRexxWaitItem( struct PPTMessage *pmsg )
{
    return FindRexxWaitItemFrame( pmsg->frame );
}
///

/// ReplyRexxWaitItem()
void ReplyRexxWaitItem( PPTREXXARGS *ra )
{
    D(bug("ReplyRexxWaitItem(ra=%08X)\n",ra));
    if( ra->args ) FreeDOSArgs( ra->args, globxd );
    if( ra->msg )  RexxReply( ra->msg, ra );
    LOCKGLOB();
    Remove( (struct Node *)ra);
    UNLOCKGLOB();

    /*
     *  Release the structure and allocated args
     */

    if( ra->process_args ) sfree( ra->process_args );
    FreeRA(ra);
}
///
/// EmptyRexxWaitItemList()
Local
VOID EmptyRexxWaitItemList(VOID)
{
    struct Node *cn = RexxWaitList.lh_Head, *nn;

    SHLOCKGLOB();

    while(nn = cn->ln_Succ) {
        PPTREXXARGS *ra;
        ra = (PPTREXXARGS *)cn;

        ra->rc = -20;
        ra->rc2 = (long)"BREAK: Host closing down";

        ReplyRexxWaitItem( ra ); /* ra will be invalid after this */

        cn = nn;
    }
    UNLOCKGLOB();
}
///

/*-------------------------------------------------------------------------*/

/// SetRexxVariable
/****u* pptsupport/SetRexxVariable *******************************************
*
*   NAME
*       SetRexxVariable -- sets a variable in the context of the frame. (V5)
*
*   SYNOPSIS
*       result = SetRexxVariable( frame, variable, value );
*
*       LONG SetRexxVariable( FRAME *, STRPTR, STRPTR )
*       D0                    A0       A1      A2
*
*   FUNCTION
*       This function sets an AREXX variable of the currently executing
*       script.  Use with care!  Do not meddle with variables such
*       as 'RC', 'RC2', or 'RESULT', since PPT uses them.
*
*   INPUTS
*       frame - the usual
*       variable - the name of the variable you wish to set
*       value - the string value
*
*   RESULT
%ld\n",res));  @    }  8    return res;(}-€    ///€   /*è   *
*       that is, 0 for success, and != 0 for failure.
*
*   EXAMPLE
*
*   NOTES
*       The SetRexxVar() parameter length is determined run-time
*       with strlen().
*
*   BUGS
*
*   SEE ALSO
*       amiga.lib/SetRexxVar
*
***********************************************************************
*
*
*/

Prototype LONG ASM SetRexxVariable( REGPARAM(a0,FRAME *,frame),REGPARAM(a1,STRPTR,var),REGPARAM(a2,STRPTR,value) );

SAVEDS
LONG ASM SetRexxVariable( REGPARAM(a0,FRAME *,frame),
                          REGPARAM(a1,STRPTR,var),
                          REGPARAM(a2,STRPTR,value ) )
{
    PPTREXXARGS *ra;
    LONG res = 0;

    D(bug("SetRexxVariable( frame=%08lX, var='%s', value='%s'\n", frame, var, value));

    if( ra = FindRexxWaitItemFrame( frame ) ) {
        res = SetRexxVar( ra->msg, var, value, strlen(value) );
        D(bug("\tSetRexxVar() = %ld\n",res));
    }

    return res;
}
///

/*-------------------------------------------------------------------------*/
/// SendRexxCommand()

/*
    Send a script file to be processed by rexxmast.

    buff is copied, so it may be allocated off stack.
*/
struct RexxMsg *SendRexxCommand( struct RexxHost *host, char *buff, BPTR fh )
{
    struct MsgPort *rexxport;
    struct RexxMsg *rexx_command_message;

    D(bug("SendRexxCommand( %s )\n", buff));

    Forbid();

    /*
     *  Find the rexx master port.
     */

    if( (rexxport = FindPort(RXSDIR)) == NULL ) {
        Permit();
        return( NULL );
    }

    /*
     *  Create a new message, with the given extension
     */

    if( (rexx_command_message = CreateRexxMsg( host->port, REXX_EXTENSION, host->port->mp_Node.ln_Name)) == NULL ) {
        Permit();
        return( NULL );
    }

    /*
     *  Put our arguments into it.
     */

    if( (rexx_command_message->rm_Args[0] = CreateArgstring(buff,strlen(buff))) == NULL ) {
        DeleteRexxMsg(rexx_command_message);
        Permit();
        return( NULL );
    }

    /*
     *  Some flags...
     */

    rexx_command_message->rm_Action = RXCOMM | RXFF_RESULT;
    rexx_command_message->rm_Stdin  = fh;
    rexx_command_message->rm_Stdout = fh;

    PutMsg( rexxport, &rexx_command_message->rm_Node );
    Permit();

    /*
     *  Increase the amount of replies to be waited for.
     */

    ++host->replies;
    return( rexx_command_message );
}

///

/*-------------------------------------------------------------------------*/
/// STEM handling functions.

Local
struct Stem *AllocStem( UBYTE * name )
{
    struct Stem *s;

    D(bug("AllocStem( %s )\n",name ));

    s = smalloc( sizeof(struct Stem) );
    NewList( (struct List *)&(s->list) );
    strncpy( s->name, name, 79 ); /* BUG: Magic */
    return s;
}

Local
VOID FreeStem( struct Stem *s )
{
    struct Node *nd = (struct Node *) s->list.mlh_Head, *ln;

    D(bug("Releasing Stem @%08X...",s ));

    /*
     *  Release all nodes.
     */

    while( ln = nd->ln_Succ ) {
        FreeStemItem( (struct StemItem *)nd );
        nd = ln;
    }

    sfree( s );
    D(bug("done\n"));
}

Local
VOID FreeStemItem( struct StemItem *si )
{
    if(si->name) sfree(si->name);
    if(si->value) sfree(si->value);
    sfree(si);
}

/*
    If string == TRUE, then the added value is in reality a pointer
    to a string.

    Strings will be allocated new space and they are copied to the stem.

    BUG: No error checking
*/
Local
PERROR AddStemItem( struct Stem *stem, UBYTE *name, ULONG value, StemType type )
{
    struct StemItem *si;

    D(bug("AddStemItem( stem = %08X, name = %s, value = %lu...",stem,name,value));

    if(!stem)
        return PERR_GENERAL;

    si = smalloc( sizeof(struct StemItem) );
    if(!si)
        return PERR_OUTOFMEMORY;

    bzero( si, sizeof(struct StemItem) );

    si->name = smalloc( strlen( name ) + 1 );
    if(!si->name) {
        FreeStemItem( si );
        return PERR_OUTOFMEMORY;
    }

    strcpy( si->name, name );

    if(type == ASI_STRING) {
        si->value = smalloc( strlen( (UBYTE *) value) + 1 );
        if(!si->value) {
            FreeStemItem( si );
            return PERR_OUTOFMEMORY;
        }
        strcpy( si->value, (UBYTE *)value );
    } else {
        si->value = smalloc( 16 );
        if(!si->value) {
            FreeStemItem( si );
            return PERR_OUTOFMEMORY;
        }
        sprintf(si->value,"%ld",value );
    }

    AddTail( (struct List *)&(stem->list), (struct Node *)si );

    D(bug("done\n"));

    return PERR_OK;
}

///

/*-------------------------------------------------------------------------*/
/// REXX initialization & exit

/*
    These two routines take care of REXX port initialization, etc.
*/
struct RexxHost *InitRexx( char *basename )
{
    struct RexxHost *host;
    int ext = 0;

    if(RexxSysBase == NULL) {
        D(bug("Rexxsysbase didn't open\n"));
        return NULL; /* Couldn't open Rexxsyslib.library */
    }

    NewList( &RexxWaitList );

    if( !(host = pzmalloc(sizeof *host)) )
        return NULL;

    strcpy( host->portname, basename );

    if( !host->portname[0] ) {
        pfree( host );
        return NULL;
    }

    while( FindPort(host->portname) )
        sprintf( host->portname, "%s.%d", basename, ++ext );

    if( !(host->port = CreatePort(host->portname, 0)) ) {
        pfree( host );
        return NULL;
    }

    if( !(host->rdargs = AllocDosObject(DOS_RDARGS, NULL)) ) {
        DeletePort( host->port );
        pfree( host );
        return NULL;
    }

    host->rdargs->RDA_Flags = RDAF_NOPROMPT;

    D(bug("Initialized REXX port as '%s'\n",host->portname ));

    return( host );
}

int ExitRexx( struct RexxHost *host )
{
    struct RexxMsg *rexxmsg;

    D(bug("Closing AREXX port\n"));

    /*
     *  Remove any allocated resources
     */

    if( RexxFileReq ) DisposeObject( RexxFileReq );

    /*
     *  Empty waiting rexx processes
     */

    EmptyRexxWaitItemList();

    /*
     *  Remove new messages
     */

    if( host ) {
        while( host->replies > 0 ) {
            WaitPort( host->port );

            while( rexxmsg = (struct RexxMsg *)GetMsg(host->port) ) {
                if( rexxmsg->rm_Node.mn_Node.ln_Type == NT_REPLYMSG ) {
                    FreeRexxCommand( rexxmsg );
                    --host->replies;
                }
                else
                    ReplyRexxCommand( rexxmsg, -20, (long) "Host closing down", NULL );
            }
        }

        if( host->rdargs ) FreeDosObject( DOS_RDARGS, host->rdargs );
        if( host->port ) DeletePort( host->port );
        pfree( host );
    }

    return PERR_OK;
}
///

/*--------------------------------------------------------------------------*/
/*                                END OF CODE                               */
/*--------------------------------------------------------------------------*/

