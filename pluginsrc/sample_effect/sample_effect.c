/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : an example effect.

    PPT and this file are (C) Janne Jalkanen 1995-1998.

    $Id: sample_effect.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
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

#define MYNAME      "Dummy"


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
    "This is just a sample effect,\n"
    "which does nothing.";

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

    PPTX_Author,            (ULONG)"My Own Name",
    PPTX_InfoTxt,           (ULONG)infoblurb,

    PPTX_RexxTemplate,      (ULONG)NULL,

    PPTX_SupportsGetArgs,   TRUE,

    PPTX_ColorSpaces,       CSF_RGB|CSF_GRAYLEVEL,

    TAG_END, 0L
};

/*
    This is a container for the internal values for
    this object.  It is saved in the Exec() routine
 */

struct Values {
    int foobar; /* Use for whatever you like */
};

#if defined( __GNUC__ )
const BYTE LibName[]="sample_effect.effect";
const BYTE LibIdString[]="sample_effect 1.0";
const UWORD LibVersion=1;
const UWORD LibRevision=0;
ADD2LIST(LIBEffectInquire,__FuncTable__,22);
ADD2LIST(LIBEffectExec,__FuncTable__,22);
/* The following two definitions are only required
   if you link with libinitr */
ADD2LIST(LIBEffectInquire,__LibTable__,22);
ADD2LIST(LIBEffectExec,__LibTable__,22);

/* Make GNU C specific declarations. __UtilityBase is
   required for the libnix integer multiplication */
struct ExecBase *SysBase = NULL;
struct Library *__UtilityBase = NULL;
#endif

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

/*
    My replacement for BGUI_NewObject() - routine. It is just a simple
    varargs stub.

    Delete if you don't need it.
*/

Object *MyNewObject( struct PPTBase *PPTBase, ULONG classid, Tag tag1, ... )
{
    struct Library *BGUIBase = PPTBase->lb_BGUI;

    return(BGUI_NewObjectA( classid, (struct TagItem *) &tag1));
}


/*
    This routine is called upon the OpenLibrary() the software
    makes.  You could use this to open up your own libraries
    or stuff.

    Return 0 if everything went OK or something else if things
    failed.
*/

LIBINIT
{
#if defined(__GNUC__)
    SysBase = SYSBASE();

    if( NULL == (__UtilityBase = OpenLibrary("utility.library",37L))) {
        return 1L;
    }

#endif
    return 0;
}


LIBCLEANUP
{
#if defined(__GNUC__)
    if( __UtilityBase ) CloseLibrary(__UtilityBase);
#endif
}

/****** effects/EffectInquire ******************************************
*
*   NAME
*       EffectInquire -- Inform about an effect
*
*   SYNOPSIS
*       data = EffectInquire( attribute, PPTBase )
*       D0                    D0         A3
*
*       ULONG EffectInquire( ULONG, struct PPTBase * );
*
*   FUNCTION
*       Return an attribute concerning this effect.
*
*   INPUTS
*       attribute - The attribute PPT wishes information on.
*
*   RESULT
*       data - The data concerning this attribute.
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

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    ULONG data;

    data = TagData( attr, MyTagArray );

    return data;
}

/****** effects/EffectExec ******************************************
*
*   NAME
*       EffectExec -- The actual execution routine.
*
*   SYNOPSIS
*       newframe = EffectExec( frame, tags, PPTBase )
*       D0                     A0     A1    A3
*
*       FRAME *EffectExec( FRAME *, struct TagItem *, struct PPTBase * );
*
*   FUNCTION
*       This function performs the actual effect.  During the execution,
*       you should utilize the InitProgress()/Progress()/FinishProgress()
*       calls so that the user knows that something is happening.
*
*       You should note that this function may be called several times
*       from different tasks, so that it definately should be re-entrant
*       (ie use no global variables but allocate everything from the stack).
*
*   INPUTS
*       frame - The source frame for the effect. If the PPTX_NoNewFrame
*           was set to FALSE (which is the default) in the module tag
*           array, this is actually also the destination frame. You can
*           modify the data in it any way you like. If the tag was set to
*           TRUE, then you will get the original frame to which you should
*           not make modifications, but you should use MakeFrame(),
*           InitFrame() or DupFrame() to create the new frame.
*       tags - Optional arguments (like the REXX message) can
*           be found from this array. See the documentation on the REXX
*           interface for more documentation.
*
*   RESULT
*       newframe - The frame which contains the modified data. If you made
*           a new frame using the MakeFrame() / DupFrame() calls, you should
*           return a pointer to that frame. If you just modified the
*           existing data, you should return the frame pointer you got
*           from PPT in the first place.
*
*           If an error occurred, you should set the appropriate error code
*           with SetErrorCode/Msg() and return NULL. Of course, in this case
*           any frames allocated by you must be released with RemFrame().
*
*   EXAMPLE
*       See the examples directory.
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

static
PERROR ParseRexxArgs( struct Values *val, ULONG *args, struct PPTBase *PPTBase )
{
    /* This is just a sample */
    if( args[0] ) {
        val->foobar = PEEKL( args[0] ); // Reads a LONG
    }

    return PERR_OK;
}

EFFECTEXEC(frame, tags, PPTBase, EffectBase)
{
    /*
     *  I'm just adding extra variables here, so that I can again
     *  delete what I don't need.
     */

    ULONG signals = 0L, sig, rc;
    BOOL quit = FALSE;
    struct Library *BGUIBase = PPTBase->lb_BGUI, *SysBase = (struct Library*)PPTBase->lb_Sys;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    FRAME *newframe = NULL;
    ULONG *args;
    struct Values *opt, val;

    if( opt = GetOptions(MYNAME) ) {
        val = *opt;
    }

    /*
     *  Check for REXX arguments for this effect.  Every effect should be able
     *  to accept AREXX commands!
     */

    if( args = (ULONG *)TagData( PPTX_RexxArgs, tags ) ) {
        ParseRexxArgs( &val, args, PPTBase );
    } else {
        /*
         *  Starts up the GUI.
         */
    }

    /*
     *  Do the actual effect
     */

    // newframe = DoEffect(&val, PPTBase);

    /*
     *  Save the options to the PPT internal system
     */

    PutOptions( MYNAME, &val, sizeof(val) );

    return newframe;
}

/****** effects/EffectGetArgs ******************************************
*
*   NAME
*       EffectGetArgs (added in PPT v6)
*
*   SYNOPSIS
*       result = EffectGetArgs( frame, tags, PPTBase )
*       D0                      A0     A1    A3
*
*       PERROR EffectGetArgs( FRAME *, struct TagItem *, struct PPTBase * );
*
*   FUNCTION
*       This function should query the user for the options and
*       then print it to a string buffer.  The format should be
*       exactly the same as for the REXX message, so that PPT can
*       then call this effect multiple times using the same
*       argument string.
*
*   INPUTS
*       frame - The source frame for the effect. If the PPTX_NoNewFrame
*           was set to FALSE (which is the default) in the module tag
*           array, this is actually also the destination frame. You can
*           modify the data in it any way you like. If the tag was set to
*           TRUE, then you will get the original frame to which you should
*           not make modifications, but you should use MakeFrame(),
*           InitFrame() or DupFrame() to create the new frame.
*       tags - Optional arguments (like the REXX message) can
*           be found from this array. See the documentation on the REXX
*           interface for more documentation.
*
*   RESULT
*       Returns standard PPT error code.
*
*   EXAMPLE
*       See the example effect source codes.
*
*   NOTES
*       Effect version should be over 3 or over.  It is not actually checked
*       but it makes life slightly easier.
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*/

EFFECTGETARGS( frame, tags, PPTBase, EffectBase )
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

