/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : EmbedFile

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    $Id: embedfile.c,v 1.1 1998/01/10 20:33:39 jj Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#undef DEBUG_MODE

/*
    First, some compiler stuff to make this compile on SAS/C too.
*/

#ifdef _DCC
#define SAVEDS __geta4
#define ASM
#define REG(x) __ ## x
#define FAR    __far
#define INLINE
#else
#define SAVEDS __saveds
#define ASM    __asm
#define REG(x) register __ ## x
#define FAR    __far
#define INLINE __inline
#endif

#ifdef DEBUG_MODE
#define D(x)    x;
#define bug     PDebug
#else
#define D(x)
#define bug     a_function_that_does_not_exist
#endif


/*
    Here are some includes you might find useful. Actually, not all
    of them are required, but I find it easier to delete extra files
    than add up forgotten ones.
*/

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>
#include <libraries/asl.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/bgui.h>

/*
    These are required, however. Make sure that these are in your include path!
*/

#include <ppt.h>
#include <pragmas/pptsupp_pragmas.h>

/*
    Just some extra, again.
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "sha.h"
#include "stegano.h"

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "EmbedFile"


/*----------------------------------------------------------------------*/
/* Internal prototypes */

extern ASM FRAME *LIBEffectExec( REG(a0) FRAME *, REG(a1) struct TagItem *, REG(a5) EXTBASE * );
extern ASM ULONG LIBEffectInquire( REG(d0) ULONG, REG(a5) EXTBASE * );


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Embeds a file into an image using a technique\n"
    "known as steganography.  Uses SHA code by\n"
    "Uwe Hollerbach\n";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name,          (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,        (ULONG)"Janne Jalkanen 1998",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"FILE/A,PASSPHRASE/K",

    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL,

    TAG_END, 0L
};

struct Values {
    char filename[256];
};

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif


/*
    This routine is called upon the OpenLibrary() the software
    makes.  You could use this to open up your own libraries
    or stuff.

    Return 0 if everything went OK or something else if things
    failed.
*/

SAVEDS ASM int __UserLibInit( REG(a6) struct Library *EffectBase )
{
    return 0;
}


SAVEDS ASM VOID __UserLibCleanup( REG(a6) struct Library *EffectBase )
{
}

SAVEDS ASM ULONG LIBEffectInquire( REG(d0) ULONG attr, REG(a5) EXTBASE *ExtBase )
{
    return TagData( attr, MyTagArray );
}

ULONG ReqA( EXTBASE *ExtBase, UBYTE *gadgets, UBYTE *body, ULONG *args )
{
    struct Library          *BGUIBase = ExtBase->lb_BGUI;
    struct bguiRequest      req = { 0L };
    ULONG                   res;
    struct Screen           *wscr = NULL;

    wscr = ExtBase->g->maindisp->scr;

    req.br_GadgetFormat     = gadgets;
    req.br_TextFormat       = body;
    req.br_Screen           = wscr;
    req.br_Title            = "EmbedFile results";
    req.br_Flags            = BREQF_XEN_BUTTONS;
    req.br_ReqPos           = POS_CENTERSCREEN;

    res =  BGUI_RequestA( NULL, &req, args );

    return res;
}

ULONG Req( EXTBASE *ExtBase, UBYTE *gadgets, UBYTE *body, ... )
{
    return( ReqA( ExtBase, gadgets, body, (ULONG *) (&body +1) ) );
}


INLINE UBYTE EncodePixel( UBYTE p, int bit )
{
    return (UBYTE)( (p & 0xFE) | bit );
}

/*
 *  BUG: This assumes all components are 8 bit wide
 */

PERROR PutByte( FRAME *frame, UBYTE b, EXTBASE *ExtBase )
{
    ULONG row, col;
    UBYTE *cp, comps = frame->pix->components, v;
    int i;

    D(bug("Writing: %02X (%c)\n",b,isprint(b) ? b : '?'));

    for( i = 0; i < 8; i++ ) {
        int bit;

        bit = (b >> (7-i)) & 1;

        NextBitLoc( &col, &row, frame->pix->bytes_per_row, frame->pix->height );

        cp = GetPixel( frame, (WORD)row, (WORD)(col/comps) );

        v = *(cp + (col%comps) );

        D(bug("\t(%3d,%3d): %d : %02X ->",row,col/comps,bit,v));

        v = EncodePixel(v, bit);
        *(cp + (col%comps)) = v;

        D(bug(" %02X\n",v));
        PutPixel( frame, (WORD)row, (WORD)(col/comps), cp );
    }

    return PERR_OK;
}

PERROR PutString( FRAME *frame, STRPTR s, EXTBASE *ExtBase )
{
    PERROR res = PERR_OK;

    while(*s && (res == PERR_OK) ) {
        res = PutByte(frame, *s++, ExtBase);
    }
    return res;
}

PERROR PutLong( FRAME *frame, LONG number, EXTBASE *ExtBase )
{
    PERROR res;

    res=PutByte( frame, (UBYTE)( (number >> 24) & 0xFF), ExtBase );
    res=PutByte( frame, (UBYTE)( (number >> 16) & 0xFF), ExtBase );
    res=PutByte( frame, (UBYTE)( (number >> 8) & 0xFF), ExtBase );
    res=PutByte( frame, (UBYTE)( (number >> 0) & 0xFF), ExtBase );

    return res;
}

FRAME *EmbedFile( FRAME *frame, struct Values *v, UBYTE *passphrase, EXTBASE *ExtBase )
{
    struct DosLibrary *DOSBase = ExtBase->lb_DOS;
    BPTR fh, lock;
    FRAME *res = frame;
    LONG c, msgsize, coversize;
    struct FileInfoBlock *fib;

    if(MakeKeyMaterial( frame, passphrase, ExtBase ) != PERR_OK)
        return NULL;

    if(fib = (struct FileInfoBlock *)AllocDosObject( DOS_FIB, NULL )) {

        if( fh = Open( v->filename, MODE_OLDFILE ) ) {

            if( lock = DupLockFromFH( fh ) ) {
                if( Examine( lock, fib ) ) {
                    LONG count = 0;

                    msgsize = fib->fib_Size + strlen(v->filename) + 8;
                    coversize = frame->pix->width * frame->pix->height * frame->pix->components;
                    if( msgsize < coversize / 8) {

                        InitProgress( frame, "Embedding file...", 0, fib->fib_Size );

                        /* First, the cookie */

                        PutByte( frame, 0x4d, ExtBase );
                        PutByte( frame, 0xa0, ExtBase );

                        /* A status byte, unused */
                        PutByte( frame, 0, ExtBase );

                        PutString( frame, FilePart(v->filename), ExtBase );
                        PutByte( frame, '\0', ExtBase );
                        PutLong( frame, fib->fib_Size, ExtBase );

                        while( (c = FGetC(fh)) != -1) {
                            if(Progress( frame, count++ )) {
                                res = NULL;
                                break;
                            }
                            PutByte( frame, (UBYTE)c, ExtBase );
                        }

                        FinishProgress( frame );
                    } else {
                        SetErrorMsg( frame, "File too large for this cover image" );
                        D(bug("File size = %ld, image size = %ld\n", fib->fib_Size, frame->pix->height*frame->pix->width, frame->pix->components));
                        res = NULL;
                    }
                } else {
                    SetErrorMsg( frame, "Couldn't Examine file!?" );
                    res = NULL;
                }
            } else {
                SetErrorCode( frame, PERR_FILEREAD );
                res = NULL;
            }
            Close( fh );
        } else {
            SetErrorMsg( frame, "Couldn't open designated file" );
            res = NULL;
        }
        FreeDosObject( DOS_FIB, fib );
    } else {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        res = NULL;
    }

    if( res ) {
        Req( ExtBase, "OK", "\n"ISEQ_C ISEQ_B"Embedding information:\n\n"
                            ISEQ_N ISEQ_L
                            "File length:     %lu bytes\n"
                            "Cover size:      %lu bytes\n"
                            "Cover bits used: %lu %%\n\n",
                            fib->fib_Size,
                            coversize,
                            8 * 100 * msgsize/coversize);
    }

    return res;
}

SAVEDS ASM FRAME *LIBEffectExec( REG(a0) FRAME *frame,
                                 REG(a1) struct TagItem *tags,
                                 REG(a5) EXTBASE *ExtBase )
{
    struct Library *BGUIBase = ExtBase->lb_BGUI;
    struct IntuitionBase *IntuitionBase = ExtBase->lb_Intuition;
    struct DosLibrary *DOSBase = ExtBase->lb_DOS;
    ULONG *args;
    PERROR res = PERR_OK;
    struct Values *opt, v = {""};
    UBYTE passphrase[256] = "";

    D(bug(MYNAME));

    if( opt = GetOptions(MYNAME) ) {
        v = *opt;
    }

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );
    if( args ) {
        strncpy( v.filename, (STRPTR)args[0], 255 );
        if( args[1] ) strncpy( passphrase, (STRPTR) args[1], 255 );
    } else {
        Object *freq;
        char tfile[256], *s;

        strcpy( tfile, v.filename );
        if(s = PathPart(tfile)) *s = '\0';

        freq = FileReqObject,
            ASLFR_Screen,       ExtBase->g->maindisp->scr,
            ASLFR_Locale,       ExtBase->locale,
            ASLFR_InitialDrawer,tfile,
            ASLFR_InitialFile,  FilePart( v.filename ),
            ASLFR_TitleText,    "Select file to embed",
        EndObject;

        switch( DoMethod( freq, FRM_DOREQUEST ) ) {
            case FRQ_OK:
                GetAttr( FRQ_Path, freq, (ULONG *)&s );
                strncpy( v.filename, s, 255 );
                break;
            default:
                res = PERR_FAILED;
                frame = NULL;
                break;
        }
    }

    if(res == PERR_OK) {
        struct TagItem phraseobj[] = { AROBJ_Value, NULL, ARSTRING_MaxChars, 255, TAG_DONE };

        phraseobj[0].ti_Data = (ULONG)passphrase;
        if( AskReq( frame, AR_Text, "\nPlease enter passphrase\n",
                           AR_StringObject, &phraseobj,
                           AR_HelpNode, "effects.guide/EmbedObject",
                           TAG_DONE) == PERR_OK )
        {
            frame = EmbedFile(frame, &v, passphrase, ExtBase);
        } else {
            frame = NULL;
        }
    }

    PutOptions( MYNAME, &v, sizeof(v) );

    return frame;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

