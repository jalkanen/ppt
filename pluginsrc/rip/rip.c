/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : rip

    PPT and this file are (C) Janne Jalkanen 1998.

    $Id: rip.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

#define MYNAME      "Remove_Isolated_Pixels"

/* Need to recreate some bgui macros to use my own tag routines. */

#define HGroupObject          MyNewObject( PPTBase, BGUI_GROUP_GADGET
#define VGroupObject          MyNewObject( PPTBase, BGUI_GROUP_GADGET, GROUP_Style, GRSTYLE_VERTICAL
#define ButtonObject          MyNewObject( PPTBase, BGUI_BUTTON_GADGET
#define CheckBoxObject        MyNewObject( PPTBase, BGUI_CHECKBOX_GADGET
#define WindowObject          MyNewObject( PPTBase, BGUI_WINDOW_OBJECT
#define SeperatorObject       MyNewObject( PPTBase, BGUI_SEPERATOR_GADGET
#define WindowOpen(wobj)      (struct Window *)DoMethod( wobj, WM_OPEN )


/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Global variables */

const char info_blurb[] =
    "Removes isolated pixels, that is, those\n"
    "whose value differ from those around it\n";

#pragma msg 186 ignore

const struct TagItem MyTagArray[] = {
    PPTX_Name,          MYNAME,
    /* Other tags go here */
    PPTX_Author,        "Janne Jalkanen, 1996-1999",
    PPTX_InfoTxt,       info_blurb,
    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,
    PPTX_RexxTemplate,  "",
    PPTX_SupportsGetArgs, TRUE,
    TAG_END, 0L
};


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
/*
    BUG: Highly unoptimized
*/

EFFECTEXEC(frame,tags,PPTBase,EffectBase)

{
    ROWPTR cp[3];
    WORD row, comps = frame->pix->components;

//    PDebug(MYNAME": Exec()\n");

    InitProgress( frame, "Removing isolated pixels...", frame->selbox.MinY, frame->selbox.MaxY );
//    PDebug("\tBegin\n");

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        UWORD col;

        //PDebug("\Reading around row %d\n",row);

        if(GetNPixelRows(frame, cp, row-1, 3) == 0) { /* Get immediate vicinity */
            frame = NULL;
            goto quit;
        }

        if(Progress(frame, row)) {
            frame = NULL;
            goto quit;
        }

        for(col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {
            UWORD count = 0;
            WORD i;
            UBYTE r,g,b;

            switch(frame->pix->colorspace) {

            case CS_RGB:
                if(col == 0) { /* Pick from right side */
                    r = cp[1][ (col+1) * 3 ];
                    g = cp[1][ (col+1) * 3 +1 ];
                    b = cp[1][ (col+1) * 3 +2 ];
                } else { /* pick from left side */
                    r = cp[1][ (col-1) * 3 ];
                    g = cp[1][ (col-1) * 3 +1 ];
                    b = cp[1][ (col-1) * 3 +2 ];
                }
                count = 0;

                // PDebug("\t(%d,%d) : Ref=(%02X-%02X-%02X)\n",row,col,r,g,b);
                for( i = 0; i <= 2; i++) {
                    if( cp[i] ) { /* Skip NULLs */
                        WORD j;
                        for(j = -1; j <= 1; j++ ) {
                            ULONG offset;
                            offset = (col+j)*3;
                            if( cp[i][ offset ] == r ) {
                                if( cp[i][ offset+1 ] == g ) {
                                    if( cp[i][ offset+2 ] == b ) {
                                        // PDebug("\t\tMatch found at (%d,%d)\n", row+i, col+j);
                                        if( !(i == 1 && j == 0) ) /* Don't count middle */
                                            count++;
                                    }
                                }
                            }
                        }
                    }
                }
                break;


            case CS_ARGB:
                if(col == 0) { /* Pick from right side */
                    r = cp[1][ (col+1) * 4 +1 ];
                    g = cp[1][ (col+1) * 4 +2 ];
                    b = cp[1][ (col+1) * 4 +3 ];
                } else { /* pick from left side */
                    r = cp[1][ (col-1) * 4 + 1];
                    g = cp[1][ (col-1) * 4 + 2];
                    b = cp[1][ (col-1) * 4 + 3];
                }
                count = 0;

                // PDebug("\t(%d,%d) : Ref=(%02X-%02X-%02X)\n",row,col,r,g,b);
                for( i = 0; i <= 2; i++) {
                    if( cp[i] ) { /* Skip NULLs */
                        WORD j;
                        for(j = -1; j <= 1; j++ ) {
                            ULONG offset;
                            offset = (col+j)*4;
                            if( cp[i][ offset + 1 ] == r ) {
                                if( cp[i][ offset+2 ] == g ) {
                                    if( cp[i][ offset+3 ] == b ) {
                                        // PDebug("\t\tMatch found at (%d,%d)\n", row+i, col+j);
                                        if( !(i == 1 && j == 0) ) /* Don't count middle */
                                            count++;
                                    }
                                }
                            }
                        }
                    }
                }
                break;

            case CS_GRAYLEVEL:
                /* Colorspace is greyscale */
                if(col == 0)  /* Pick from right side */
                    r = cp[1][ (col+1) ];
                else  /* pick from left side */
                    r = cp[1][ (col-1) ];
                count = 0;

                // PDebug("\t(%d,%d) : Ref=(%02X-%02X-%02X)\n",row,col,r,g,b);
                for( i = 0; i <= 2; i++) {
                    if( cp[i] ) {
                        WORD j;

                        for(j = -1; j <= 1; j++ ) {
                            if( cp[i][ (col+j) ] == r ) {
                                // PDebug("\t\tMatch found at (%d,%d)\n", row+i, col+j);
                                if( !(i == 1 && j == 0) ) /* Don't count middle */
                                    count++;
                            }
                        }
                    }
                }
                break;
            }

            // PDebug("\t\tcount = %d\n",count);
            /* Was this one alone? */
            if(count == 8) {
                UBYTE *scp, *dcp;
                WORD i;

                // PDebug("\tPixel at (%d,%d) is alone\n",row,col);
                dcp = cp[1] + col*comps;
                if( col == 0 )
                    scp = cp[1]+(col+1)*comps;
                else
                    scp = cp[1]+(col-1)*comps;

                for(i = 0; i < comps; i++) {
                    *dcp++ = *scp++;
                }
            } /* count */

        } /* for(col = ...) */
        PutNPixelRows(frame,cp,row-1,3);
    } /* for(row = ...) */

quit:
    FinishProgress(frame);

    return frame;
}

EFFECTGETARGS(frame, tags, PPTBase, EffectBase)
{
    STRPTR buffer;

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    buffer[0] = '\0'; /* No arguments necessary */

    return PERR_OK;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

