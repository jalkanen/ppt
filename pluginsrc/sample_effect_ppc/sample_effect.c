/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : an example effect.

    PPT and this file are (C) Janne Jalkanen 1995-1998.

*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#ifdef DEBUG_MODE
#define D(x)    x;
#define bug     PDebug
#else
#define D(x)
#define bug     a_function_that_does_not_exist
#endif

#ifdef _M68000
#define SAVEDS __saveds
#define ASM            // __asm
#define REG(x)         // register __ ## x
#define FAR    __far
#else
// PPC decls
#define SAVEDS  __saveds
#define ASM
#define REG(x)
#define FAR     __far
#endif


/*
    Here are some includes you might find useful. Actually, not all
    of them are required, but I find it easier to delete extra files
    than add up forgotten ones.
*/

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/bgui.h>

/*
    These are required, however. Make sure that these are in your include path!
*/

#include <ppt.h>
#include <proto/pptsupp.h>

/*
    Just some extra, again.
*/

#include <stdio.h>
#include <stdarg.h>



/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Dummy"

/*
    Need to recreate some bgui macros to use my own tag routines. Don't worry,
    you might not need these.
*/

#define HGroupObject          MyNewObject( ExtBase, BGUI_GROUP_GADGET
#define VGroupObject          MyNewObject( ExtBase, BGUI_GROUP_GADGET, GROUP_Style, GRSTYLE_VERTICAL
#define ButtonObject          MyNewObject( ExtBase, BGUI_BUTTON_GADGET
#define CheckBoxObject        MyNewObject( ExtBase, BGUI_CHECKBOX_GADGET
#define WindowObject          MyNewObject( ExtBase, BGUI_WINDOW_OBJECT
#define SeperatorObject       MyNewObject( ExtBase, BGUI_SEPERATOR_GADGET
#define WindowOpen(wobj)      (struct Window *)DoMethod( wobj, WM_OPEN )


/*----------------------------------------------------------------------*/
/* Internal prototypes */

ASM FRAME *EffectExec( REG(r1) FRAME *, REG(r2) struct TagItem *, REG(r3) struct PPTBase *PPTBase );
ASM ULONG EffectInquire( REG(r1) ULONG, REG(r3) struct PPTBase *PPTBase );


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

    PPTX_Name,          (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,        (ULONG)"My Own Name",
    PPTX_InfoTxt,       (ULONG)infoblurb,

    PPTX_RexxTemplate,  (ULONG)NULL,

    PPTX_ColorSpaces,   CSF_RGB|CSF_GRAYLEVEL,

    TAG_END, 0L
};

/*
    This is a container for the internal values for
    this object.  It is saved in the Exec() routine
 */

struct Values {
    int foobar; /* Use for whatever you like */
};

#ifdef __PPC__
struct TagItem *__MyTagArray = MyTagArray;
void *__LIBEffectExec = EffectExec;
void *__LIBEffectInquire = EffectExec;
#endif

/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
#ifndef __PPC__
void __regargs __chkabort(void) {}
#endif
void __regargs _CXBRK(void) {}
#endif

#if 0
/*
    My replacement for BGUI_NewObject() - routine. It is just a simple
    varargs stub.

    Delete if you don't need it.
*/

Object *MyNewObject( EXTBASE *ExtBase, ULONG classid, Tag tag1, ... )
{
    struct Library *BGUIBase = ExtBase->lb_BGUI;

    return(BGUI_NewObjectA( classid, (struct TagItem *) &tag1));
}
#endif

/*
    This routine is called upon the OpenLibrary() the software
    makes.  You could use this to open up your own libraries
    or stuff.

    Return 0 if everything went OK or something else if things
    failed.
*/

SAVEDS ASM int __UserLibInit( REG(r2) struct Library *EffectBase )
{
    return 0;
}


SAVEDS ASM VOID __UserLibCleanup( REG(r2) struct Library *EffectBase )
{
}


/****** effects/EffectInquire ******************************************
*
*   NAME
*       EffectInquire -- Inform about an effect
*
*   SYNOPSIS
*       data = EffectInquire( attribute, ExtBase )
*       D0                    D0         A5
*
*       ULONG EffectInquire( ULONG, EXTBASE * );
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

SAVEDS ASM ULONG EffectInquire( REG(r1) ULONG attr,
                                REG(r3) struct PPTBase *PPTBase )
{
    return TagData( attr, MyTagArray );
}

/****** effects/EffectExec ******************************************
*
*   NAME
*       EffectExec -- The actual execution routine.
*
*   SYNOPSIS
*       newframe = EffectExec( frame, tags, ExtBase )
*       D0                     A0     A1    A5
*
*       FRAME *Exec( FRAME *, struct TagItem *, EXTBASE * );
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

SAVEDS ASM FRAME *EffectExec( REG(r1) FRAME *frame,
                              REG(r2) struct TagItem *tags,
                              REG(r3) struct PPTBase *PPTBase )
{
    /*
     *  I'm just adding extra variables here, so that I can again
     *  delete what I don't need.
     */

    ULONG signals = 0L, sig, rc;
    BOOL quit = FALSE;
    struct Library *BGUIBase = PPTBase->lb_BGUI, *SysBase = (struct Library*)PPTBase->lb_Sys;
    struct Library *IntuitionBase = PPTBase->lb_Intuition, *DOSBase = PPTBase->lb_DOS;
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
        /* This is just a sample */
        if( args[0] ) {
            val.foobar = (*(LONG *)args[0] ); // Reads a LONG
        }
    } else {
        /*
         *  Starts up the GUI.
         */
    }

    /*
     *  Do the actual effect
     */

    // newframe = DoEffect(&v, ExtBase);

    /*
     *  Save the options to the PPT internal system
     */

    PutOptions( MYNAME, &val, sizeof(val) );

    return newframe;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

