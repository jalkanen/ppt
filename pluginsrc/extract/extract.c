/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : extract

    PPT and this file are (C) Janne Jalkanen 1996-2000

    $Id: extract.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
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

#define MYNAME      "Extract"


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
    "Extract colors out of a RGB image\n";

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

    PPTX_Author,        (ULONG)"Janne Jalkanen 1996-2000",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)"RED/S,GREEN/S,BLUE/S,ALPHA/S",

    PPTX_ColorSpaces,   CSF_RGB|CSF_ARGB,

    PPTX_SupportsGetArgs, TRUE,

    TAG_END, 0L
};

struct Values {
    ULONG saveda,savedr,savedg,savedb;
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

LONG aa, rr, gg, bb;

struct TagItem alpha[] = {
    ARCHECKBOX_Selected, TRUE,
    AROBJ_Value, (ULONG) &aa,
    AROBJ_Label, (ULONG) "Extract Alpha?",
    TAG_DONE
};

struct TagItem red[] = {
    ARCHECKBOX_Selected, TRUE,
    AROBJ_Value, (ULONG) &rr,
    AROBJ_Label, (ULONG) "Extract Red?",
    TAG_DONE
};
struct TagItem green[] = {
    ARCHECKBOX_Selected, TRUE,
    AROBJ_Value, (ULONG) &gg,
    AROBJ_Label, (ULONG) "Extract Green?",
    TAG_DONE
};
struct TagItem blue[] = {
    ARCHECKBOX_Selected, TRUE,
    AROBJ_Value, (ULONG) &bb,
    AROBJ_Label, (ULONG) "Extract Blue?",
    TAG_DONE
};

struct TagItem win[] = {
    AR_CheckBoxObject,  (ULONG)alpha,
    AR_CheckBoxObject,  (ULONG)red,
    AR_CheckBoxObject,  (ULONG)green,
    AR_CheckBoxObject,  (ULONG)blue,
    AR_Text,            (ULONG)"Check in those colors\nyou wish to extract",
    AR_HelpNode,        (ULONG)"effects.guide/Extract",
    TAG_DONE
};

VOID ParseRexxArgs( struct Values *v, ULONG *args )
{
    v->savedr = args[0];
    v->savedg = args[1];
    v->savedb = args[2];
    v->saveda = args[3];
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    WORD row, col;
    UBYTE colorspace = frame->pix->colorspace;
    struct Values v = {TRUE}, *arg;

    if( arg = GetOptions(MYNAME) )
        v = *arg;

    args = (ULONG *) TagData( PPTX_RexxArgs, tags );

    alpha[0].ti_Data = v.saveda;
    red[0].ti_Data   = v.savedr;
    green[0].ti_Data = v.savedg;
    blue[0].ti_Data  = v.savedb;

    if( args ) {
        ParseRexxArgs( &v, args );
    } else {
        /* Don't show any extra requesters */
        if( colorspace == CS_ARGB )
            win[0].ti_Tag = AR_CheckBoxObject;
        else
            win[0].ti_Tag = TAG_IGNORE;

        if( AskReqA( frame, win ) != PERR_OK ) {
            return NULL;
        }
        v.savedr = rr;
        v.savedg = gg;
        v.savedb = bb;
        v.saveda = aa;
    }

    /*
     *  Make sure that we'll never extract alpha channel, if it
     *  does not exist
     */

    InitProgress( frame, "Extracting components...",
                  frame->selbox.MinY, frame->selbox.MaxY );

    for( row = frame->selbox.MinY; row < frame->selbox.MaxY; row++ ) {
        ROWPTR cp, dcp;

        if( Progress( frame, row ) )
            return NULL;

        dcp = cp = GetPixelRow( frame, row );

        dcp += frame->selbox.MinX * frame->pix->components;

        for( col = frame->selbox.MinX; col < frame->selbox.MaxX; col++ ) {

            if( colorspace == CS_ARGB ) {
                if(!v.saveda) dcp[0] = 0;
                dcp++;
            }
            if( !v.savedr ) dcp[0] = 0;
            if( !v.savedg ) dcp[1] = 0;
            if( !v.savedb ) dcp[2] = 0;
            dcp += 3;
        }

        PutPixelRow( frame, row, cp );
    }

    FinishProgress( frame );

    PutOptions(MYNAME,&v,sizeof(struct Values));

    return frame;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    WORD row, col;
    UBYTE colorspace = frame->pix->colorspace;
    struct Values v = {TRUE}, *arg;
    STRPTR buffer;
    PERROR res;

    if( arg = GetOptions(MYNAME) )
        v = *arg;

    args = (ULONG *) TagData( PPTX_RexxArgs, tags );
    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    alpha[0].ti_Data = v.saveda;
    red[0].ti_Data   = v.savedr;
    green[0].ti_Data = v.savedg;
    blue[0].ti_Data  = v.savedb;

    if( args ) {
        ParseRexxArgs( &v, args );
    }

    /* Don't show any extra requesters */
    if( colorspace == CS_ARGB )
        win[0].ti_Tag = AR_CheckBoxObject;
    else
        win[0].ti_Tag = TAG_IGNORE;

    if( ( res = AskReqA( frame, win ) ) == PERR_OK ) {
        v.savedr = rr;
        v.savedg = gg;
        v.savedb = bb;
        v.saveda = aa;

        SPrintF(buffer,"%s %s %s %s",
                       v.saveda ? "ALPHA" : "",
                       v.savedr ? "RED" : "",
                       v.savedg ? "GREEN" : "",
                       v.savedb ? "BLUE" : "" );

        PutOptions(MYNAME,&v,sizeof(struct Values));
    }


    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

