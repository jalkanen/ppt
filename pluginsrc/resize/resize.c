/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : resize

    PPT and this file are (C) Janne Jalkanen 1995-2000.

    BUG: Does not save its arguments (though it does not make sense
         anyway).

    $Id: resize.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>

#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Resize"



/*
    Need to recreate some bgui macros to use my own tag routines. Don't worry,
    you might not need these.
*/

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
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Enlarges the image by adding black\n"
    "borders around it\n";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name,              (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,            (ULONG)"Janne Jalkanen 1996-2000",
    PPTX_InfoTxt,           (ULONG)infoblurb,

    PPTX_RexxTemplate,      (ULONG)"NEWHEIGHT=NH/N/A,NEWWIDTH=NW/N/A,FILL/K",

    PPTX_ColorSpaces,       CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB,

    PPTX_NoNewFrame,        TRUE,

    PPTX_SupportsGetArgs,   TRUE,

    TAG_END, 0L
};

typedef enum {
    FILL_BLACK = 0,
    FILL_WHITE,
    FILL_BACKGROUND,
    FILL_BORDER
} Fill_T;

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

ULONG ht, wt, fl = FILL_BLACK;

const char *fill_labels[] = {
    "Black",
    "White",
    "Background",
    "Border",
    NULL
};


struct TagItem height[] = {
    ARSLIDER_Min,   0,
    ARSLIDER_Default,0,
    ARSLIDER_Max,   4096,
    AROBJ_Value,    (ULONG)&ht,
    AROBJ_Label,    (ULONG)"Height",
    TAG_DONE
};

struct TagItem width[] = {
    ARSLIDER_Min,   0,
    ARSLIDER_Default,0,
    ARSLIDER_Max,   4096,
    AROBJ_Value,    (ULONG)&wt,
    AROBJ_Label,    (ULONG)"Width",
    TAG_DONE
};

struct TagItem fill[] = {
    AROBJ_Label,    (ULONG)"Fill with",
    AROBJ_Value,    (ULONG)&fl,
    ARCYCLE_Labels, (ULONG)fill_labels,
    TAG_DONE
};

struct TagItem win[] =  {
    AR_SliderObject,  (ULONG)height,
    AR_SliderObject,  (ULONG)width,
    AR_CycleObject,   (ULONG)fill,
    AR_Text,          (ULONG)"Choose new height and width",
    AR_HelpNode,      (ULONG) "effects.guide/Resize",
    TAG_DONE
};


PERROR ParseRexxArgs( FRAME *frame, ULONG *args, struct PPTBase *PPTBase )
{
    ht = * ((ULONG *)args[0]);
    wt = * ((ULONG *)args[1]);

    if( args[2] ) {
        for( fl = FILL_BLACK; fill_labels[fl]; fl++ ) {
            if( strnicmp( fill_labels[fl], (STRPTR)args[2], strlen((STRPTR)args[2]) ) == 0 )
                break;
        }
        if( !fill_labels[fl] ) {
            SetErrorMsg( frame, "Invalid FILL argument");
            return PERR_ERROR;
        }
    }

    return PERR_OK;
}

EFFECTGETARGS(frame,tags,PPTBase,IOModuleBase)
{
    PERROR res = PERR_OK;
    ULONG *args;
    STRPTR buffer;

    height[0].ti_Data = height[1].ti_Data = (ULONG)frame->pix->height;
    width[0].ti_Data = width[1].ti_Data = (ULONG)frame->pix->width;

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( ParseRexxArgs( frame, args, PPTBase ) != PERR_OK ) {
            return PERR_ERROR;
        }
    }

    if( (res = AskReqA( frame, win )) == PERR_OK ) {
        SPrintF( buffer, "NEWWIDTH %d NEWHEIGHT %ld FILL %s",
                         wt, ht, fill_labels[fl] );
    }

    return res;
}

EFFECTEXEC(frame,tags,PPTBase,IOModuleBase)
{
    FRAME *newframe;
    ULONG *args;
    WORD  top, left, right, bottom;
    ROWPTR pixelbuf = NULL, scp;
    int comps;

    // PDebug(MYNAME": Exec()\n");

    height[0].ti_Data = height[1].ti_Data = (ULONG)frame->pix->height;
    width[0].ti_Data = width[1].ti_Data = (ULONG)frame->pix->width;

    /*
     *  Check REXX
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        if( ParseRexxArgs( frame, args, PPTBase ) != PERR_OK ) {
            return NULL;
        }
    } else {
        if( AskReqA( frame, win ) != PERR_OK ) {
            SetErrorCode( frame, PERR_CANCELED );
            return NULL;
        }
    }


    /*
     *  Do some sanity checks and calculate the position of the old
     *  image within the new image.
     */

    if( ht < frame->pix->height ) ht = frame->pix->height;
    if( wt < frame->pix->width  ) wt = frame->pix->width;

    top    = (ht - frame->pix->height) / 2;
    left   = (wt - frame->pix->width) / 2;
    bottom = top  + frame->pix->height;
    right  = left + frame->pix->width;

    comps  = frame->pix->components;

    /*
     *  System-dependant initializations
     */

    switch( fl ) {
        case FILL_BACKGROUND:
            pixelbuf = AllocVec( 10, 0L );
            GetBackgroundColor( frame, pixelbuf );
            break;
    }

    if(newframe = MakeFrame( frame ) ) {
        newframe->pix->height = (WORD) ht;
        newframe->pix->width  = (WORD) wt;
        if( InitFrame( newframe ) == PERR_OK ) {
            ROWPTR dcp;
            WORD row,col;

            InitProgress( frame, "Resizing...", 0, ht );

            for( row = 0; row < ht; row++ ) {

                if(Progress( frame, row ) ) {
                    SetErrorCode( frame, PERR_BREAK );
                    RemFrame( newframe );
                    newframe = NULL;
                    goto errorexit;
                }

                dcp = GetPixelRow( newframe, row );

                if(!dcp) { // This should never happen.
                    SetErrorCode( frame, PERR_FILEREAD );
                    RemFrame( newframe);
                    newframe = NULL;
                    goto errorexit;
                }

                if( row < top || row >= bottom ) {
                    /*
                     *  Top and bottom rows
                     */

                    switch( fl ) {
                        case FILL_BLACK:
                            memset( dcp, 0, newframe->pix->bytes_per_row );
                            break;
                        case FILL_WHITE:
                            memset( dcp, 0xFF, newframe->pix->bytes_per_row );
                            break;
                        case FILL_BACKGROUND:
                            for( col = 0; col < wt; col++ ) {
                                memcpy( dcp+col*comps, pixelbuf, comps );
                            }
                            break;

                        case FILL_BORDER:
                            scp = GetPixelRow( frame, (row < top) ? 0 : frame->pix->height-1);
                            memcpy( dcp+left*comps, scp,
                                    frame->pix->bytes_per_row );
                            for( col = 0; col < left; col++ ) {
                                memcpy( dcp+col*comps, scp, comps );
                            }
                            for( col = right; col < wt; col++ ) {
                                memcpy( dcp+col*comps, scp+frame->pix->bytes_per_row-comps, comps );
                            }
                            break;
                    }
                } else {
                    scp = GetPixelRow( frame, row-top );

                    switch( fl ) {
                        case FILL_BLACK:
                            memset( dcp, 0, left*comps );
                            memset( dcp+right*comps, 0,
                                    (wt-right)*comps );
                            break;

                        case FILL_WHITE:
                            memset( dcp, 0xFF, left*comps );
                            memset( dcp+right*comps, 0xFF,
                                    (wt-right)*comps );
                            break;

                        case FILL_BACKGROUND:
                            for( col = 0; col < left; col++ ) {
                                memcpy( dcp+col*comps, pixelbuf, comps );
                            }
                            for( col = right; col < wt; col++ ) {
                                memcpy( dcp+col*comps, pixelbuf, comps );
                            }
                            break;

                        case FILL_BORDER:
                            for( col = 0; col < left; col++ ) {
                                memcpy( dcp+col*comps, scp, comps );
                            }
                            for( col = right; col < wt; col++ ) {
                                memcpy( dcp+col*comps, scp+frame->pix->bytes_per_row - comps,comps );
                            }
                            break;
                    }
                    memcpy( dcp+left*comps, scp,
                            frame->pix->bytes_per_row );
                }

                PutPixelRow( newframe, row, dcp );
            }

            FinishProgress( frame );
        }
    }

errorexit:
    if(pixelbuf) FreeVec( pixelbuf );

    return newframe;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

