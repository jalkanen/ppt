/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : ExtractFile

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    $Id: extractfile.c,v 1.1 2001/10/25 16:16:57 jalkanen Exp $
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

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "ExtractFile"

#define DESBLOCKSIZE 24

/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Extracts an embedded file from an image.\n"
    "Uses SHA code by Uwe Hollerbach.";

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

    PPTX_RexxTemplate,  (ULONG)"DIR/A,PASSPHRASE/K",

    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL,

    TAG_END, 0L
};

struct Values {
    char drawer[256];
};

UBYTE desBuffer[DESBLOCKSIZE];
int desCounter;

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif


EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}


INLINE UBYTE DecodePixel( UBYTE p )
{
    return (UBYTE)( p & 0x01 );
}

VOID EncryptInit( char *passphrase )
{
    UBYTE key[DESBLOCKSIZE];

    desCounter = DESBLOCKSIZE; // Force start
    make3key( passphrase, key );
    des3key( key, DE1 );
}

VOID EncryptFinish( FRAME *frame, struct PPTBase *PPTBase )
{
    ULONG key[96] = {0};

    use3key( key ); /* Make sure the key is deleted */
}

/*
 *  Fills up the buffer from the image.  It can read extra bytes, we don't care.
 */

int ReadBuffer( FRAME *frame, UBYTE *buf, struct PPTBase *PPTBase )
{
    ULONG byte, row, col;
    UBYTE *cp, comps = frame->pix->components;
    int i, c;

    for( c = 0; c < DESBLOCKSIZE; c++ ) {
        byte = 0;

        for( i = 0; i < 8; i++ ) {

            NextBitLoc( &col, &row, frame->pix->bytes_per_row, frame->pix->height );

            cp = GetPixel( frame, (WORD)row, (WORD)(col/comps) );
            byte |= DecodePixel(*(cp + col%comps));
            D(bug("\t%bit %d (%3d,%3d): %d\n", i, row, col/comps, DecodePixel(*(cp + col%comps))));
            byte = byte << 1;
        }

        byte >>= 1; // Otherwise one too much.

        buf[c] = (UBYTE)byte;
        D(bug("\tByte = %02X (%c)\n", byte, isprint(byte) ? byte : '?'));
    }

    return c;
}

int GetByte( FRAME *frame, struct PPTBase *PPTBase )
{
    UBYTE tmpbuf[DESBLOCKSIZE];

    if( desCounter == DESBLOCKSIZE ) {
        ReadBuffer( frame, tmpbuf, PPTBase );
        D3des( tmpbuf, desBuffer );
        // bcopy( tmpbuf, desBuffer, DESBLOCKSIZE );
        desCounter = 0;
    }

    return( desBuffer[desCounter++] );
}

#if 0
int GetByte( FRAME *frame, struct PPTBase *PPTBase )
{
    ULONG row, col, byte;
    UBYTE *cp, comps = frame->pix->components;
    int i;

    D(bug("GetByte():\n"));

    byte = 0;

    for( i = 0; i < 8; i++ ) {

        NextBitLoc( &col, &row, frame->pix->bytes_per_row, frame->pix->height );

        cp = GetPixel( frame, (WORD)row, (WORD)(col/comps) );
        byte |= DecodePixel(*(cp + col%comps));
        D(bug("\t%bit %d (%3d,%3d): %d\n", i, row, col/comps, DecodePixel(*(cp + col%comps))));
        byte = byte << 1;
    }

    byte >>= 1; // Otherwise one too much.

    D(bug("\tByte = %02X (%c)\n", byte, isprint(byte) ? byte : '?'));

    return (int)byte;
}
#endif

// Returns # of characters read
LONG GetString(FRAME *frame, UBYTE *str, ULONG len, struct PPTBase *PPTBase)
{
    int c, count = 0;

    while( (c = GetByte(frame, PPTBase)) && (count < len) ) {
        *str++ = (UBYTE)c;
        ++count;
    }

    *str = '\0'; // Sentinel

    return count;
}


LONG GetLong( FRAME *frame, struct PPTBase *PPTBase )
{
    LONG res;

    res = (GetByte(frame,PPTBase)<<24) +
          (GetByte(frame,PPTBase)<<16) +
          (GetByte(frame,PPTBase)<<8) +
           GetByte(frame,PPTBase);

    return res;
}

FRAME *ExtractFile( FRAME *frame, struct Values *v, char *path, struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    BPTR fh;
    FRAME *res = frame;
    LONG length, bytes = 0;

    /*
     *  File length
     */

    length = GetLong( frame, PPTBase );
    D(bug("File length: %lu\n",length));

    if( fh = Open( path, MODE_NEWFILE ) ) {
        InitProgress(frame, "Extracting file...", 0, length );

        while(bytes++ < length) {
            int b;

            if( Progress( frame, bytes ) ) {
                Close( fh );
                return NULL;
            }

            b = GetByte( frame, PPTBase );

            FPutC( fh, b );
        }
        Close( fh );

        FinishProgress( frame );
    } else {
        SetErrorCode( frame, PERR_FILEOPEN );
        res = NULL;
    }

    return res;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    ULONG *args;
    PERROR res = PERR_OK;
    struct Values *opt, v = {""};
    char filename[256], *path, passphrase[256] = "";
    int cookie_hi, cookie_lo;
    struct TagItem phraseobj[] = { AROBJ_Value, NULL, ARSTRING_MaxChars, 255, TAG_DONE };

    D(bug(MYNAME));

    if( opt = GetOptions(MYNAME) ) {
        v = *opt;
    }

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );

    /*
     *  Check for passphrase
     */

    if( args ) {
        if( args[1] ) {
            strncpy( passphrase, (STRPTR)args[1], 255 );
        }
    } else {
        phraseobj[0].ti_Data = (ULONG)passphrase;
        if( AskReq( frame, AR_Text, "\nPlease enter passphrase\n",
                           AR_StringObject, &phraseobj,
                           AR_HelpNode, "effects.guide/EmbedObject",
                           TAG_DONE) != PERR_OK )
        {
            return NULL;
        }
    }

    /*
     *  First, make the key.
     */

    if(MakeKeyMaterial( frame, passphrase, PPTBase ) != PERR_OK )
        return NULL;

    EncryptInit( passphrase );

    /*
     *  Check cookie
     */

    cookie_hi = GetByte( frame, PPTBase );
    cookie_lo = GetByte( frame, PPTBase );

    if( cookie_hi != 0x4d || cookie_lo != 0xa0 ) {
        SetErrorMsg( frame, "No message detected." );
        return NULL;
    }

    GetByte( frame, PPTBase ); // Status

    /*
     *  Get initial filename from image
     */

    GetString( frame, filename, 255, PPTBase );

    D(bug("Filename to extract to: '%s'\n",filename));

    if( args ) {
        strncpy( v.drawer, (STRPTR)args[0], 255 );
    } else {
        Object *freq;

        freq = FileReqObject,
            ASLFR_Screen,       PPTBase->g->maindisp->scr,
            ASLFR_Locale,       PPTBase->locale,
            ASLFR_InitialDrawer,v.drawer,
            ASLFR_InitialFile,  filename,
            ASLFR_TitleText,    "Extract file to?",
            ASLFR_DoSaveMode,   TRUE,
        EndObject;

        switch( DoMethod( freq, FRM_DOREQUEST ) ) {
            case FRQ_OK:
                GetAttr( FRQ_Path, freq, (ULONG *)&path );
                break;
            default:
                res = PERR_FAILED;
                frame = NULL;
                break;
        }
    }

    if(res == PERR_OK)
        frame = ExtractFile(frame, &v, path, PPTBase);

    EncryptFinish( frame, PPTBase );

    PutOptions( MYNAME, &v, sizeof(v) );

    return frame;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

