/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : EmbedFile

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    $Id: embedfile.c,v 1.3 1998/12/10 21:44:51 jj Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#undef DEBUG_MODE

#include <pptplugin.h>

#include <libraries/asl.h>

#include <string.h>
#include <ctype.h>

#include "sha.h"
#include "stegano.h"
#include "d3des.h"

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "EmbedFile"

#define DESBLOCKSIZE 24 /* For 3-des */

/*----------------------------------------------------------------------*/
/* Internal prototypes */

VOID OutputBuffer( FRAME *frame, UBYTE *buf, int bytes, struct PPTBase *PPTBase );

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

#ifdef _PPC
    PPTX_CPU,           (ULONG)AFF_PPC,
#endif
    TAG_END, 0L
};

struct Values {
    char filename[256];
};

UBYTE desBuffer[DESBLOCKSIZE];
int desCounter;

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
#ifndef __PPC__
void __regargs __chkabort(void) {}
#endif
void __regargs _CXBRK(void) {}
#endif

/*
 *  PowerPC definitions, otherwise they won't compile.
 */

#ifdef __PPC__
#include <powerup/ppclib/interface.h>

struct TagItem *__MyTagArray = MyTagArray;
void *__LIBEffectExec = EffectExec;
void *__LIBEffectInquire = EffectExec;

extern _m68kDoMethodA();

__inline ULONG DoMethodA( Object *obj, Msg msg )
{
    struct Caos c;

    c.caos_Un.Function = (APTR)_m68kDoMethodA;
    c.M68kCacheMode = IF_CACHEFLUSHALL;
    c.PPCCacheMode  = IF_CACHEFLUSHALL;
    c.a0 = (ULONG)obj;
    c.a1 = (ULONG)msg;

    return PPCCallM68k( &c );
}

ULONG DoMethod(Object *obj, ULONG MethodID, ... )
{
    return DoMethodA( obj, (Msg)&MethodID );
}

#else /* 68k code */
#endif

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

ULONG ReqA( struct PPTBase *PPTBase, UBYTE *gadgets, UBYTE *body, ULONG *args )
{
    struct Library          *BGUIBase = PPTBase->lb_BGUI;
    struct bguiRequest      req = { 0L };
    ULONG                   res;
    struct Screen           *wscr = NULL;

    wscr = PPTBase->g->maindisp->scr;

    req.br_GadgetFormat     = gadgets;
    req.br_TextFormat       = body;
    req.br_Screen           = wscr;
    req.br_Title            = "EmbedFile results";
    req.br_Flags            = BREQF_XEN_BUTTONS;
    req.br_ReqPos           = POS_CENTERSCREEN;

    res =  BGUI_RequestA( NULL, &req, args );

    return res;
}

ULONG Req( struct PPTBase *PPTBase, UBYTE *gadgets, UBYTE *body, ... )
{
    return( ReqA( PPTBase, gadgets, body, (ULONG *) (&body +1) ) );
}


INLINE UBYTE EncodePixel( UBYTE p, int bit )
{
    return (UBYTE)( (p & 0xFE) | bit );
}

BOOL EncryptInit( char *passphrase )
{
    UBYTE key[DESBLOCKSIZE];

    desCounter = 0;
    make3key( passphrase, key );
    des3key( key, EN0 );
    return TRUE;
}

VOID EncryptFinish( FRAME *frame, struct PPTBase *PPTBase )
{
    ULONG key[96] = {0};

    OutputBuffer( frame, desBuffer, desCounter, PPTBase );
    use3key( key ); /* Make sure the key is deleted */
    return;
}

/*
 *  Writes the encrypted data out
 */

VOID OutputBuffer( FRAME *frame, UBYTE *buf, int bytes, struct PPTBase *PPTBase )
{
    int i, byte;
    ULONG row, col;
    UBYTE *cp, comps = frame->pix->components, v;

    for( byte = 0; byte < bytes; byte++ ) {
        UBYTE b;

        b = buf[byte];

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
    }
}

PERROR PutByte( FRAME *frame, UBYTE b, struct PPTBase *PPTBase )
{
    UBYTE tmpbuffer[DESBLOCKSIZE];
    desBuffer[desCounter++] = b;

    if( desCounter == DESBLOCKSIZE ) {
        D3des( desBuffer, tmpbuffer );
        // bcopy( desBuffer, tmpbuffer, DESBLOCKSIZE );
        OutputBuffer( frame, tmpbuffer, DESBLOCKSIZE, PPTBase );
        desCounter = 0;
    }
    return PERR_OK;
}

#if 0
/*
 *  BUG: This assumes all components are 8 bit wide
 */

PERROR PutByte( FRAME *frame, UBYTE b, struct PPTBase *PPTBase )
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
#endif

PERROR PutString( FRAME *frame, STRPTR s, struct PPTBase *PPTBase )
{
    PERROR res = PERR_OK;

    while(*s && (res == PERR_OK) ) {
        res = PutByte(frame, *s++, PPTBase);
    }
    return res;
}

PERROR PutLong( FRAME *frame, LONG number, struct PPTBase *PPTBase )
{
    PERROR res;

    res=PutByte( frame, (UBYTE)( (number >> 24) & 0xFF), PPTBase );
    res=PutByte( frame, (UBYTE)( (number >> 16) & 0xFF), PPTBase );
    res=PutByte( frame, (UBYTE)( (number >> 8) & 0xFF), PPTBase );
    res=PutByte( frame, (UBYTE)( (number >> 0) & 0xFF), PPTBase );

    return res;
}

FRAME *EmbedFile( FRAME *frame, struct Values *v, UBYTE *passphrase, struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    BPTR fh, lock;
    FRAME *res = frame;
    LONG c, msgsize, coversize, filesize;
    struct FileInfoBlock *fib;

    if(MakeKeyMaterial( frame, passphrase, PPTBase ) != PERR_OK)
        return NULL;

    EncryptInit( passphrase );

    if(fib = (struct FileInfoBlock *)AllocDosObject( DOS_FIB, NULL )) {

        if( fh = Open( v->filename, MODE_OLDFILE ) ) {

            if( lock = DupLockFromFH( fh ) ) {
                if( Examine( lock, fib ) ) {
                    LONG count = 0;

                    filesize = fib->fib_Size;

                    msgsize = filesize + strlen(v->filename) + 8;
                    coversize = frame->pix->width * frame->pix->height * frame->pix->components;
                    if( msgsize < coversize / 8) {

                        InitProgress( frame, "Embedding file...", 0, fib->fib_Size );

                        /* First, the cookie */

                        PutByte( frame, 0x4d, PPTBase );
                        PutByte( frame, 0xa0, PPTBase );

                        /* A status byte, unused */
                        PutByte( frame, 0, PPTBase );

                        PutString( frame, FilePart(v->filename), PPTBase );
                        PutByte( frame, '\0', PPTBase );
                        PutLong( frame, fib->fib_Size, PPTBase );

                        while( (c = FGetC(fh)) != -1) {
                            if(Progress( frame, count++ )) {
                                res = NULL;
                                break;
                            }
                            PutByte( frame, (UBYTE)c, PPTBase );
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

    EncryptFinish( frame, PPTBase );

    if( res ) {
        Req( PPTBase, "OK", "\n"ISEQ_C ISEQ_B"Embedding information:\n\n"
                            ISEQ_N ISEQ_L
                            "File length:     %lu bytes\n"
                            "Cover size:      %lu bytes\n"
                            "Cover bits used: %lu %%\n\n",
                            filesize,
                            coversize,
                            8 * 100 * msgsize/coversize);
    }

    return res;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
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
            ASLFR_Screen,       PPTBase->g->maindisp->scr,
            ASLFR_Locale,       PPTBase->locale,
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
            frame = EmbedFile(frame, &v, passphrase, PPTBase);
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

