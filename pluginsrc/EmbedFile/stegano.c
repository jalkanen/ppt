/*
 *  This file contains the steganographic parts of EmbedFile and ExtractFile
 *
 *  (C) Janne Jalkanen 1998
 *
 *  $Id: stegano.c,v 1.2 2000/11/17 03:12:18 jalkanen Exp $
 */

/*-----------------------------------------------------------------*/

#undef DEBUG_MODE

#ifdef DEBUG_MODE
#define D(x)    x;
#define bug     PDebug
#else
#define D(x)
#define bug     a_function_that_does_not_exist
#endif

#define __USE_SYSBASE
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/bgui.h>

#include <ppt.h>
#include <pragmas/pptsupp_pragmas.h>

#include "sha.h"
#include "stegano.h"
#include "string.h"

// BUG: Should probably not be global, since it can be read off
// the memory at a later time.

UBYTE key[SHA_DIGESTSIZE];
ULONG counter = 0;

/*
 *  Calculates the location of the next pixel.
 *  Attempts to be compatible with the stegano package.
 *  Three iterations are needed so that the distribution is random
 *  enough.
 *  BUG: This is big_endian
 */

VOID NextBitLoc( ULONG *resx, ULONG *resy, UWORD width, UWORD height )
{
    ULONG  x, y, hash;
    SHA_INFO sha = {0};

    y = counter / width;
    x = counter % width;

    // First iteration

    sha_init(&sha);
    sha_update( &sha, key, SHA_DIGESTSIZE );
    sha_update( &sha, (UBYTE *)&x, sizeof(x) );
    sha_final( &sha );
    D(sha_print( &sha ));
    hash = (sha.digest[0] & 0x7fffffff); // Lose the sign

    y = (y+hash) % height;

    // Second iteration

    sha_init(&sha);
    sha_update( &sha, key, SHA_DIGESTSIZE );
    sha_update( &sha, (UBYTE *)&y, sizeof(y) );
    sha_final( &sha );
    D(sha_print( &sha ));
    hash = (sha.digest[0] & 0x7fffffff); // Lose the sign

    x = (x+hash) % width;

    // Third iteration

    sha_init(&sha);
    sha_update( &sha, key, SHA_DIGESTSIZE );
    sha_update( &sha, (UBYTE *)&x, sizeof(x) );
    sha_final( &sha );
    D(sha_print( &sha ));
    hash = (sha.digest[0] & 0x7fffffff); // Lose the sign

    y = (y+hash) % height;

    *resx = x;
    *resy = y;

    ++counter;
}


PERROR MakeKeyMaterial( FRAME *frame, UBYTE *passphrase, struct PPTBase *PPTBase )
{
    ROWPTR cp, tmprow;
    WORD row;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    SHA_INFO sha = {0};
    PERROR res = PERR_OK;

    sha_init( &sha );

    InitProgress(frame,"Building key...", 0, frame->pix->height );

    /*
     *  First, use the passphrase for the key.
     */

    if( strlen(passphrase) > 0 ) sha_update( &sha, passphrase, strlen(passphrase) );

    if( tmprow = AllocVec( frame->pix->bytes_per_row, 0L ) ) {
        for( row = 0; row < frame->pix->height; row++ ) {
            WORD col;

            cp = GetPixelRow( frame, row );

            if( Progress( frame, row ) ) {
                res = PERR_BREAK;
                break;
            }

            for( col = 0; col < frame->pix->bytes_per_row; col++ ) {
                /* Use only significant bytes */
                tmprow[col] = cp[col] & 0xFE;
            }

            sha_update( &sha, tmprow, frame->pix->bytes_per_row );
        }

        // Use the passphrase again (why?)

        if( strlen(passphrase) > 0 ) sha_update( &sha, passphrase, strlen(passphrase) );

        FinishProgress( frame );
        sha_final( &sha );

        memcpy( key, &sha.digest[0], SHA_DIGESTSIZE );

        D(sha_print( &sha ) );

        FreeVec( tmprow );
    } else {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        res = PERR_OUTOFMEMORY;
    }

    return res;
}

#ifdef DEBUG_MODE

void sha_print(SHA_INFO *sha_info)
{
    PDebug("%08lx %08lx %08lx %08lx %08lx\n",
        sha_info->digest[0], sha_info->digest[1], sha_info->digest[2],
        sha_info->digest[3], sha_info->digest[4]);
}


#endif
