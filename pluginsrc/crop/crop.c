/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : Crop

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    $Id: crop.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Crop"

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
    "Crops to the selected area";

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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1997",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"",

    PPTX_ColorSpaces,   ~0,

    PPTX_NoNewFrame,    TRUE,

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


EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    FRAME *newframe = NULL;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    if(newframe = MakeFrame( frame ) ) {
        newframe->pix->width = frame->selbox.MaxX - frame->selbox.MinX;
        newframe->pix->height = frame->selbox.MaxY - frame->selbox.MinY;

        InitProgress( frame, "Cropping...", frame->selbox.MinY, frame->selbox.MaxY );

        if( InitFrame( newframe ) == PERR_OK ) {
            WORD row, drow = 0;
            ULONG offset, amount;

            offset = frame->pix->components * frame->selbox.MinX;
            amount = frame->pix->components * newframe->pix->width;

            for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
                ROWPTR scp, dcp;

                if( Progress( frame, row ) ) {
                    RemFrame( newframe );
                    SetErrorCode( frame, PERR_BREAK );
                    return NULL;
                }

                scp = GetPixelRow( frame, row );
                dcp = GetPixelRow( newframe, drow );

                CopyMem( scp+offset, dcp, amount );
                PutPixelRow( newframe, drow, dcp );
                drow++;
            }
            FinishProgress( frame );
        }
    }

    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

