/*
    PROJECT: ppt
    MODULE:  testloader

    A new style loader.  PPT and this file are (C) Janne Jalkanen 1998.

    $Id: loader.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
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

    PPTX_Load,              TRUE,
    PPTX_ColorSpaces,       CSF_RGB|CSF_GRAYLEVEL|CSF_LUT,

    /*
     *  Here are some pretty standard definitions. All iomodules should have
     *  these defined.
     */

    PPTX_Name,              (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,            (ULONG)"My Own Name",
    PPTX_InfoTxt,           (ULONG)infoblurb,

    PPTX_SupportsGetArgs,   TRUE,

    PPTX_RexxTemplate,      (ULONG)NULL,

    PPTX_PreferredPostFix,(ULONG)".jpg",
    PPTX_PostFixPattern,(ULONG)"#?.(jpg|jpeg|jfif)",

    TAG_END, 0L
};

struct Values {
    ULONG foobar;
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

/****** iomodule/IOCheck ******************************************
*
*   NAME
*       IOCheck - Checks if this module can handle files of this type.
*
*   SYNOPSIS
*       success = IOCheck( fh, len, buf, PPTBase )
*       D0                 D0  D1   A0   A3
*
*       BOOL IOCheck( BPTR, ULONG, UBYTE *, struct PPTBase * );
*
*   FUNCTION
*       Checks if the given file is in a correct format.
*
*   INPUTS
*       fh - open filehandle to the file to be checked.
*       len - how many bytes are in buf
*       buf - len bytes from the beginning of the file
*
*   RESULT
*       Returns TRUE, if this IO module can handle this file,
*       otherwise FALSE.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*/

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    D(bug("IOCheck()\n"));
    return FALSE;
}

/****** iomodule/IOLoad ******************************************
*
*   NAME
*       IOLoad - Loads an image from the disk.
*
*   SYNOPSIS
*       result = IOLoad( fh, frame, tags, PPTBase )
*       D0               D0  A0     A1    A3
*
*       PERROR IOLoad( BPTR, FRAME *, struct TagItem *, struct PPTBase * );
*
*   FUNCTION
*       This function should perform the actual loading.
*       Please see the actual example code.
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       Entry incomplete
*
*   SEE ALSO
*
*****************************************************************************
*
*/

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    D(bug("IOLoad()\n"));
    SetErrorMsg(frame,"This is just a test");
    return PERR_ERROR;
}

/****** iomodule/IOSave ******************************************
*
*   NAME
*       IOSave - Saves the frame.
*
*   SYNOPSIS
*       result = IOSave( fh, type, frame, tags, PPTBase )
*       D0               D0  D1    A0     A1    A3
*
*       PERROR IOSave( BPTR, ULONG, FRAME *, struct TagItem *, struct PPTBase * );
*
*   FUNCTION
*       This function should save the frame.
*
*   INPUTS
*       type - Tells the type of the file.  If CSF_LUT, then
*           you should use GetBitmapRow() to read the data from the
*           frame, as it is in bitmapped format.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       Entry incomplete
*
*   SEE ALSO
*
*****************************************************************************
*
*/

static
PERROR ParseRexxArgs( struct Values *val, ULONG *args, struct PPTBase *PPTBase )
{
    /* This is just a sample */
    if( args[0] ) {
        val->foobar = PEEKL( args[0] ); // Reads a LONG
    }

    return PERR_OK;
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

/****** iomodule/IOGetArgs ******************************************
*
*   NAME
*       IOGetArgs (added in PPT v6)
*
*   SYNOPSIS
*       result = IOGetArgs( type, frame, tags, PPTBase )
*       D0                  D1    A0     A1    A3
*
*       PERROR IOGetArgs( ULONG, FRAME *, struct TagItem *, struct PPTBase * );
*
*   FUNCTION
*       Gets an argument string from the user for later use in AREXX.
*
*   INPUTS
*       tags - This array contains a special PPTX_ArgBuffer tag,
*           whose ti_Data points to a string where you should
*           then print your argument array.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       IO module version should be over 3 or over.  It is not actually checked
*       but it makes life slightly easier.
*
*   BUGS
*       Entry incomplete
*
*   SEE ALSO
*       effect/EffectGetArgs
*
*****************************************************************************
*
*/

IOGETARGS( type, frame, tags, PPTBase, EffectBase )
{
    struct Values *opt, val;
    ULONG *origargs;
    STRPTR buffer;
    PERROR res = PERR_OK;

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( origargs = (ULONG *)TagData( PPTX_RexxArgs, tags ) )
        res = ParseRexxArgs( &val, origargs, PPTBase );

    if( res == PERR_OK ) {
        /*
         *  Start the GUI
         */
    }

    return res;
}


/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

