/*
    PROJECT: ppt
    MODULE:  testloader

    A new style loader.  PPT and this file are (C) Janne Jalkanen 1998.

    $Id: djvu.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/

#include <pptplugin.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Dummy"

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "This is just a sample io module,\n"
    "which does nothing.";

/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {

    /*
     *  Tells the capabilities of this loader/saver unit.
     */

    PPTX_Load,          TRUE,
    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL|CSF_LUT,

    /*
     *  Here are some pretty standard definitions. All iomodules should have
     *  these defined.
     */

    PPTX_Name,          (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,        (ULONG)"My Own Name",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)NULL,

    PPTX_PreferredPostFix,(ULONG)".jpg",
    PPTX_PostFixPattern,(ULONG)"#?.(jpg|jpeg|jfif)",

    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

LIBINIT
{
    return 0;
}

LIBCLEANUP
{
}

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

/*
    This must always exist!
*/

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    D(bug("IOCheck()\n"));
    return FALSE;
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    D(bug("IOLoad()\n"));
    SetErrorMsg(frame,"This is just a test");
    return PERR_ERROR;
}

/*
    Format can be any of CSF_* - flags
*/
IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    D(bug("IOSave(type=%08X)\n",format));
    SetErrorMsg(frame,"This is just a test");
    return PERR_UNKNOWNTYPE;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

