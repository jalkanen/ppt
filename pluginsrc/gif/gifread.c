/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)    | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

/*
    Modified heavily by JJ 1996.
*/

#include "GIF.h"

#define __USE_SYSBASE
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>

int GetDataBlock (BPTR, struct GIF *, unsigned char *buf, struct PPTBase * );
int GetCode ( BPTR fd, struct GIF *, int code_size, int flag, struct PPTBase * );
int LWZReadByte( BPTR, struct GIF *, int, int, struct PPTBase *PPTBase);

/*
    OK.
*/
PERROR
ReadColorMap(BPTR fd, struct GIF *GifScreen, struct PPTBase *PPTBase )
{
    int             i;
    unsigned char   rgb[3];
    int             flag;
    UBYTE           *buffer;
    APTR            DOSBase = PPTBase->lb_DOS;

    flag   = TRUE;
    buffer = (UBYTE *)GifScreen->ColorMap;

    for (i = 0; i < GifScreen->BitPixel; ++i) {
        if (! ReadOK(fd, rgb, sizeof(rgb))) {
            return PERR_WARNING;
        }

        buffer[i*3 + CM_RED]   = rgb[0] ;
        buffer[i*3 + CM_GREEN] = rgb[1] ;
        buffer[i*3 + CM_BLUE]  = rgb[2] ;

        flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
    }

    GifScreen->GrayScale = flag;

    return PERR_OK;
}

/*
    Not yet converted.
*/
int
DoExtension(BPTR fh, int label, struct GIF *gif, struct PPTBase *PPTBase)
{
    static char     buf[256];
    char            *str;

    switch (label) {
            case 0x01:              /* Plain Text Extension */
            str = "Plain Text Extension";
#if 0
            if (GetDataBlock(fh, gif, (unsigned char*) buf, PPTBase) == 0)
                ;

            lpos   = LM_to_uint(buf[0], buf[1]);
            tpos   = LM_to_uint(buf[2], buf[3]);
            width  = LM_to_uint(buf[4], buf[5]);
            height = LM_to_uint(buf[6], buf[7]);
            cellw  = buf[8];
            cellh  = buf[9];
            foreground = buf[10];
            background = buf[11];

            while (GetDataBlock(fh, gif, (unsigned char*) buf, PPTBase) != 0) {
                PPM_ASSIGN(image[ypos][xpos],
                    cmap[CM_RED][v],
                    cmap[CM_GREEN][v],
                    cmap[CM_BLUE][v]);
                ++index;
            }

            return FALSE;
#else
            break;
#endif
        case 0xff:              /* Application Extension */
            str = "Application Extension";
            break;
        case 0xfe:              /* Comment Extension */
            str = "Comment Extension";
            while (GetDataBlock(fh, gif, (unsigned char*) buf, PPTBase) != 0) {
                D(bug("gif comment: %s\n", buf ));
            }
            return FALSE;

        case 0xf9:              /* Graphic Control Extension */
            str = "Graphic Control Extension";
            (void) GetDataBlock(fh, gif, (unsigned char*) buf, PPTBase);

            gif->gif89.disposal    = (buf[0] >> 2) & 0x7;
            gif->gif89.inputFlag   = (buf[0] >> 1) & 0x1;
            gif->gif89.delayTime   = LM_to_uint(buf[1],buf[2]);
            if ((buf[0] & 0x1) != 0) {
                D(bug("GIF has transparency @ %d\n", buf[3] ));
                gif->gif89.transparent = buf[3];
            }

            while (GetDataBlock(fh, gif,(unsigned char*) buf, PPTBase) != 0)
                ;
            return FALSE;

        default:
            str = buf;
            D(sprintf(buf, "UNKNOWN (0x%02x)", label));
            break;
    }

    D(bug("got a '%s' extension", str ));

    while (GetDataBlock(fh, gif, (unsigned char*) buf, PPTBase) != 0)
        ;

    return FALSE;
}

/*
    OK.

    Return number of bytes read or -1 on error.
*/
int
GetDataBlock(BPTR fd, struct GIF *g, UBYTE *buf, struct PPTBase *PPTBase)
{
    unsigned char   count;
    APTR DOSBase = PPTBase->lb_DOS;

    if (! ReadOK(fd,&count,1)) {
        SetErrorMsg( g->Frame, "Couldn't read block count!" );
        return -1;
    }

    g->ZeroDataBlock = count == 0;

    if ((count != 0) && (! ReadOK(fd, buf, count))) {
        SetErrorMsg( g->Frame, "Corrupt GIF file! (EOF too early)" );
        return -1;
    }

    return count;
}

/*
    Return -1 on error.
*/
int
GetCode(BPTR fd, struct GIF *g, int code_size, int flag, struct PPTBase *PPTBase)
{
    int                     i, j, ret;
    unsigned char           count;

    if (flag) {
        g->curbit = 0;
        g->lastbit = 0;
        g->done = FALSE;
        g->last_byte = 0;
        return 0;
    }

    if ( (g->curbit+code_size) >= g->lastbit) {
        if (g->done) {
            if (g->curbit >= g->lastbit) {
                SetErrorMsg(g->Frame,"Ran off the end of my bits" );
            }
            return -1;
        }
        g->gcbuf[0] = g->gcbuf[g->last_byte-2];
        g->gcbuf[1] = g->gcbuf[g->last_byte-1];

        if ((count = GetDataBlock(fd, g, &(g->gcbuf[2]), PPTBase)) == 0)
            g->done = TRUE;

        if( count < 0 ) return -1;

        g->last_byte = 2 + count;
        g->curbit = (g->curbit - g->lastbit) + 16;
        g->lastbit = (2+count) << 3 ;
    }

    ret = 0;
    for (i = g->curbit, j = 0; j < code_size; ++i, ++j) {
        ret |= ((g->gcbuf[ i >> 3 ] & (1 << (i & 0x07))) != 0) << j;
    }

    g->curbit += code_size;

    return ret;
}

PERROR
InitLWZData( struct GIF *gif, struct PPTBase *PPTBase )
{
    APTR SysBase = PPTBase->lb_Sys;

    gif->fresh = FALSE;
    gif->code_size = gif->set_code_size = gif->max_code = gif->max_code_size = 0;
    gif->firstcode = gif->oldcode = 0;
    gif->clear_code = gif->end_code = 0;
    gif->sp = NULL;

    gif->table = AllocVec( 2 * (1<<MAX_LWZ_BITS) * sizeof(int), MEMF_CLEAR|MEMF_ANY );
    gif->stack = AllocVec( (1<<MAX_LWZ_BITS)*2*sizeof(int), MEMF_CLEAR|MEMF_ANY );

    if(!gif->table || !gif->stack) return PERR_OUTOFMEMORY;

    return PERR_OK;
}

VOID
FreeLWZData( struct GIF *gif, struct PPTBase *PPTBase )
{
    APTR SysBase = PPTBase->lb_Sys;

    if( gif->table ) FreeVec( gif->table );
    if( gif->stack ) FreeVec( gif->stack );
}

/*
    Return values:

    >= 0  : valid code
    -2    : warning
    -3    : warning
    -4    : fatal error
*/
int
LWZReadByte( BPTR fd, struct GIF *g, int flag, int input_code_size, struct PPTBase *PPTBase)
{
    int             code, incode;
    int             i;
    int             *table = g->table;

    if (flag) {
        g->set_code_size = input_code_size;
        g->code_size = g->set_code_size+1;
        g->clear_code = 1 << g->set_code_size ;
        g->end_code = g->clear_code + 1;
        g->max_code_size = 2*(g->clear_code);
        g->max_code = g->clear_code+2;

        GetCode(fd, g, 0, TRUE, PPTBase);

        g->fresh = TRUE;

        for (i = 0; i < g->clear_code; ++i) {
            table[i<<1]     = 0;
            table[(i<<1)+1] = i;
        }
        for (; i < (1<<MAX_LWZ_BITS); ++i)
            table[i<<1] = table[1] = 0;

        g->sp = g->stack;

        return 0;
    } else if (g->fresh) {
        g->fresh = FALSE;
        do {
            g->firstcode = g->oldcode = GetCode(fd, g, g->code_size, FALSE, PPTBase);
            if( g->firstcode < 0 ) return -4;
        } while (g->firstcode == g->clear_code);
        return g->firstcode;
    }

    if (g->sp > g->stack)
        return *( --(g->sp));

    while ((code = GetCode(fd, g, g->code_size, FALSE, PPTBase)) >= 0) {
        if (code == g->clear_code) {
            for (i = 0; i < g->clear_code; ++i) {
                table[i<<1]     = 0;
                table[(i<<1)+1] = i;
            }

            for (; i < (1<<MAX_LWZ_BITS); ++i)
                table[i<<1] = table[(i<<1)+1] = 0;

            g->code_size = g->set_code_size+1;
            g->max_code_size = (g->clear_code) << 1;
            g->max_code = g->clear_code+2;
            g->sp = g->stack;
            g->firstcode = g->oldcode = GetCode(fd, g, g->code_size, FALSE, PPTBase);
            return g->firstcode;

        } else if (code == g->end_code) {
            int             count;
            unsigned char   buf[260];

            if (g->ZeroDataBlock) {
                // SetErrorMsg( g->Frame, "Zero data block" );
                return -2;
            }

            while ((count = GetDataBlock(fd, g, buf, PPTBase)) > 0)
                ;

            if (count != 0) {
                SetErrorMsg(g->Frame,"Missing EOD in data stream (common occurence)");
                return -2;
            }

            // SetErrorMsg( g->Frame, "EOF detected" );

            return -2;
        }

        incode = code;

        if (code >= g->max_code) {
            *(g->sp)++ = g->firstcode;
            code = g->oldcode;
        }

        while (code >= g->clear_code) {
            *(g->sp)++ = table[(code<<1)+1];
            if (code == table[(code<<1)]) {
                SetErrorMsg( g->Frame, "Circular entry!\n");
                return -3;
            }
            code = table[code<<1];
        }

        *(g->sp)++ = g->firstcode = table[(code<<1)+1];

        if ((code = g->max_code) <(1<<MAX_LWZ_BITS)) {
            table[code<<1]   = g->oldcode;
            table[(code<<1)+1] = g->firstcode;
            ++(g->max_code);
            if ((g->max_code >= g->max_code_size) &&
            (g->max_code_size < (1<<MAX_LWZ_BITS))) {
                g->max_code_size <<= 1;
                ++(g->code_size);
            }
        }

        g->oldcode = incode;

        if (g->sp > g->stack)
            return *--(g->sp);
    }

    return code;
}

/*
    OK

    BUG: Progress does not work correctly on interlaced gifs.
    BUG: frame is irrelevant.
*/

PERROR
ReadImage(BPTR          fd,
          FRAME *       frame,
          struct GIF *  GifScreen,
          struct PPTBase *     PPTBase )
{
    unsigned char   c;
    int             v;
    int             xpos = 0, ypos = 0, pass = 0;
    ROWPTR          cp;
    APTR            DOSBase = PPTBase->lb_DOS;
    UBYTE           *cmap = (UBYTE *)GifScreen->ColorMap;
    int error;

    /*
    **  Initialize the Compression routines
    */

    if (! ReadOK(fd,&c,1)) {
        SetErrorMsg(frame,"EOF / read error on image data" );
        return PERR_FAILED;
    }

    if ((error = LWZReadByte(fd, GifScreen, TRUE, c, PPTBase)) < 0) {
        SetErrorMsg(frame,"Error while reading image" );
        if( error == -4 )
            return PERR_FAILED;
        else
            return PERR_WARNING;
    }

    cp = GetPixelRow( frame, 0 );

    while ((v = LWZReadByte(fd,GifScreen,FALSE,c,PPTBase)) >= 0 ) {
        ULONG xp3,v3;

        xp3 = xpos * frame->pix->components;
        v3 = v*3;

        switch( frame->pix->colorspace ) {
            case CS_RGB:
                cp[xp3 + CM_RED]   = cmap[v3+CM_RED];
                cp[xp3 + CM_GREEN] = cmap[v3+CM_GREEN];
                cp[xp3 + CM_BLUE]  = cmap[v3+CM_BLUE];
                break;

            case CS_GRAYLEVEL:
                cp[xpos] = cmap[v3];
                break;

            case CS_ARGB:
                if(v == GifScreen->gif89.transparent)
                    cp[xp3] = 0xFF;
                else
                    cp[xp3] = 0;

                cp[xp3 + 1] = cmap[v3+CM_RED];
                cp[xp3 + 2] = cmap[v3+CM_GREEN];
                cp[xp3 + 3] = cmap[v3+CM_BLUE];

                break;
        }

        ++xpos;

        if (xpos == GifScreen->Width) {

            PutPixelRow( frame, ypos, cp );


            /*
             *  Next line.
             */

            xpos = 0;
            if (GifScreen->Interlace) {

                if(Progress( frame, (ypos + pass*GifScreen->Height)/4 ) )
                    return PERR_BREAK;

                switch (pass) {
                    case 0:
                    case 1:
                        ypos += 8; break;
                    case 2:
                        ypos += 4; break;
                    case 3:
                        ypos += 2; break;
                }

                if (ypos >= GifScreen->Height) {
                    ++pass;
                    switch (pass) {
                        case 1:
                            ypos = 4; break;
                        case 2:
                            ypos = 2; break;
                        case 3:
                            ypos = 1; break;
                        default:
                            goto fini;
                    }
                }
            } else {
                if(Progress( frame, ypos ))
                    return PERR_BREAK;

                ++ypos;
            }

            cp = GetPixelRow( frame, ypos );

        }
        if (ypos >= GifScreen->Height)
            break;
    }

    // if( v == -2 ) return PERR_WARNING;
    if( v == -3 || v == -1 ) return PERR_FAILED;

fini:

    if (LWZReadByte(fd,GifScreen,FALSE,c,PPTBase) >= 0) {
        SetErrorMsg(frame, "Too much input data");
        return PERR_WARNING;
    }

    FinishProgress( frame );

    return PERR_OK;
}
