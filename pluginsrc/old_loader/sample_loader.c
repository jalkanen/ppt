/*
    This is an example loader for PPT

    PPT is (C) Janne Jalkanen 1995.

    Just add your own code and water.
*/

/*
    First, some compiler stuff to make this compile on SAS/C too.
*/

#ifdef _DCC
#define SAVEDS __geta4
#define ASM
#define REG(x) __ ## x
#else
#define SAVEDS __saveds
#define ASM    __asm
#define REG(x) register __ ## x
#endif

/*-------------------------------------------------------------------*/
/* Includes */

#include <exec/types.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>

#include <pragma/exec_pragmas.h>
#include <pragma/dos_pragmas.h>
#include <pragma/utility_pragmas.h>


#include <utility/tagitem.h>

#include "ppt.h"
#include "example.h"

#include "clib/pptsupp_protos.h"
#include <pragma/pptsupp_pragmas.h>

#include <stdio.h>


/*------------------------------------------------------------------*/
/* Defines */

#define MYNAME          "Example"
#define VERSION         0
#define REVISION        0
#define VERSIONSTRING   "$VER: "MYNAME" V0.0. ("__DATE__")."

/*------------------------------------------------------------------*/
/* Don't mind. If you see this part, I just forgot to delete it
   from the release. */
#ifdef FORTIFY
APTR SysBase;
#endif

/*------------------------------------------------------------------*/
/* Prototypes */

static PERROR Purge( REG(a6) EXTBASE * );
static PERROR Init( REG(a6) EXTBASE * );
static BOOL   Check( REG(d0) BPTR, REG(a6) EXTBASE * );
static PERROR Load( REG(d0) BPTR, REG(a0) FRAME *, REG(a1) struct TagItem *, REG(a6) EXTBASE * );
static PERROR SaveTC( REG(d0) BPTR, REG(a0) FRAME *, REG(a1) struct TagItem *, REG(a6) EXTBASE * );
static PERROR SaveBM( REG(d0) BPTR, REG(a0) FRAME *, REG(a1) struct TagItem *, REG(a6) EXTBASE * );

/*------------------------------------------------------------------*/
/* Global variables and constants */

const char infoblurb[] = "Foobar\n";

const struct TagItem MyTagArray[] = {
    PPTX_Version,           VERSION,
    PPTX_Revision,          REVISION,
    PPTX_Load,              Load,
    PPTX_Init,              Init,
    PPTX_Purge,             Purge,
    PPTX_Check,             Check,
    PPTX_SaveTrueColor,     SaveTC,
    PPTX_SaveColorMapped,   SaveBM,
    PPTX_Name,              MYNAME,
    PPTX_VerStr,            VERSIONSTRING,
    PPTX_Author,            "My Own Name, 1996",
    PPTX_InfoTxt,           infoblurb,
    TAG_DONE, 0L
};

/*------------------------------------------------------------------*/
/* Code */

/*
    This function is of the same format for all externals.
*/
SAVEDS ASM PERROR Purge( REG(a6) EXTBASE *ExtBase )
{
    PDebug(MYNAME": Purge()\n");
    return PERR_OK;
}

/*
    This function is of the same format for all externals.
*/
SAVEDS ASM PERROR Init( REG(a6) EXTBASE *ExtBase )
{
    PDebug(MYNAME": Init()\n");

    return PERR_OK;
}

/****** loaders/Load ******************************************
*
*   NAME
*       Load -- Load an image.
*
*   SYNOPSIS
*       result = Load( fh, frame, tags, ExtBase )
*       D0             D0  A0     A1    A6
*
*       PERROR Load( BPTR, FRAME *, struct TagItem *, EXTBASE * );
*
*   FUNCTION
*       Provides the main entry point to a load module.  This function
*       should load the contents of the given filehandle and put it into
*       the frame.
*
*   INPUTS
*       fh - The opened file.  This is a standard Amiga file handle as
*           returned by Open.  Do NOT Close() this.
*       frame - The frame you should fill in.  Note that this frame
*           has been built with MakeFrame() so it does not have any resources
*           attached to it yet.  It will become a fully functional frame
*           at the BeginLoad() - call.
*       tags - A TagItem array, which can contain any of the PPT standard
*           tags.  The following tags may appear:
*
*           PPTX_RexxArgs (ULONG *) - pointer to arguments sent by PPT.
*               This array will contain arguments as parsed by
*               dos.library/ReadArgs() using your PPTX_RexxTemplate as
*               a template.  Read, do not write!
*
*       ExtBase - as usual, pointer to the ExtBase handle which you can
*           use to get some PPT global variables and/or call PPT support
*           functions.
*
*   RESULT
*       result - can be of any of the three following constants:
*
*           PERR_OK - image was loaded OK.
*           PERR_WARNING - a non-fatal error happened.  The image
*               can still be used but it might be damaged.  The user
*               will be queried if he/she wants to keep the image.
*           PERR_ERROR - could not load the image.  The frame will
*               be discarded.
*
*   EXAMPLE
*       <TBA>
*
*   NOTES
*       The loader/saver situation is about to change soonish.  Do not
*       use anything you read in this autodoc, because the loaders
*       will soon become standard Amiga libraries as effects already are.
*
*   BUGS
*       This entry still incomplete.
*
*   SEE ALSO
*
*****************************************************************************
*
*/

SAVEDS ASM PERROR Load( REG(d0) BPTR fh,
                        REG(a0) FRAME *frame,
                        REG(a1) struct TagItem *tags,
                        REG(a6) EXTBASE *ExtBase )
{
    struct Library *UtilityBase = ExtBase->lb_Utility;
    struct Library *DOSBase = ExtBase->lb_DOS, *SysBase = ExtBase->lb_Sys;
    UBYTE *cp;
    UWORD height, width, crow;

    PDebug(MYNAME" : Load()\n");

    /*
     *  Allocate & Initialize load object
     */


    /*
     *  Get picture size and initialize the Frame
     */

    frame->pix->height = height;
    frame->pix->width = width;
    frame->pix->components = 3;
    frame->pix->origdepth = 24;

    InitProgress( frame, "Loading "MYNAME" picture...", 0, height );

    if(BeginLoad( frame ) != PERR_OK) {
        /* Do clean up */
        return;
    }

    while( crow < height ) {
        if(Progress(frame, crow)) {
            break;
        }

        cp = GetPixelRow( frame, crow );

        /*
         *  Get the actual row of pixels, then decompress it into cp
         */

    } /* while */

errorexit:
    /*
     *  Release allocated resources
     */

    return PERR_OK;
}

/****** loaders/SaveBM ******************************************
*
*   NAME
*       SaveBM -- Save bitmapped (rendered) images.
*
*   SYNOPSIS
*       result = SaveBM( handle, frame, tags, ExtBase )
*       D0               D0      A0     A1    A6
*
*       PERROR SaveBM( BPTR, FRAME *, struct TagItem *, EXTBASE * );
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*       result - can be of any of the three following constants:
*
*           PERR_OK - image was saved OK.
*           PERR_WARNING - a non-fatal error happened.  The image
*               can still be used but it might be damaged.
*           PERR_ERROR - could not save the image.  The file will
*               be discarded.
*
*   EXAMPLE
*
*   NOTES
*       The loader/saver situation is about to change soonish.  Do not
*       use anything you read in this autodoc, because the loaders
*       will soon become standard Amiga libraries as effects already are.
*
*   BUGS
*       This entry is incomplete.
*
*   SEE ALSO
*
*****************************************************************************
*
*/

SAVEDS ASM PERROR SaveBM( REG(d0) BPTR handle,
                          REG(a0) FRAME *frame,
                          REG(a1) struct TagItem *tags,
                          REG(a6) EXTBASE *ExtBase )
{
    /* NULL CODE */
}

/****** loaders/SaveTC ******************************************
*
*   NAME
*       SaveTC -- Save Truecolor images. (24/8 bit).
*
*   SYNOPSIS
*       result = SaveTC( handle, frame, tags, ExtBase )
*       D0               D0      A0     A1    A6
*
*       PERROR SaveTC( BPTR, FRAME *, struct TagItem *, EXTBASE * );
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*       result - can be of any of the three following constants:
*
*           PERR_OK - image was saved OK.
*           PERR_WARNING - a non-fatal error happened.  The image
*               can still be used but it might be damaged.
*           PERR_ERROR - could not save the image.  The file will
*               be discarded.
*
*   EXAMPLE
*
*   NOTES
*       The loader/saver situation is about to change soonish.  Do not
*       use anything you read in this autodoc, because the loaders
*       will soon become standard Amiga libraries as effects already are.
*
*   BUGS
*       Entry incomplete.
*
*   SEE ALSO
*
*****************************************************************************
*
*/

SAVEDS ASM PERROR SaveTC( REG(D0) BPTR handle,
                          REG(A0) FRAME *frame,
                          REG(A1) struct TagItem *tags,
                          REG(A6) EXTBASE *ExtBase )

{
    /* NULL code. */
}


/****** loaders/Check ******************************************
*
*   NAME
*       Check -- Check if this module can recognize this file.
*
*   SYNOPSIS
*       result = Check( fh, ExtBase )
*       D0              D0  A6
*
*       BOOL Check( BPTR, EXTBASE * );
*
*   FUNCTION
*       This function should check the file pointed to by the file
*       handle fh and return TRUE if this loader can handle a file
*       of this type and FALSE otherwise.
*
*   INPUTS
*       fh - A filehandle for you to check. This is guaranteed to
*           be open, readable and positioned at the beginning (PPT
*           makes a Seek(fh, OFFSET_BEGINNING, 0L) before calling
*           this routine.
*
*       ExtBase - as usual, pointer to the ExtBase handle which you can
*           use to get some PPT global variables and/or call PPT support
*           functions.
*
*   RESULT
*       result - TRUE, if this module can handle this filetype,
*           FALSE, otherwise.
*
*   EXAMPLE
*
*   NOTES
*       This function is not required in a loader module, but of course
*       then you must invoke this loader by hand via the "Load As..."
*       function.
*
*       The loader/saver situation is about to change soonish.  Do not
*       use anything you read in this autodoc, because the loaders
*       will soon become standard Amiga libraries as effects already are.
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*/

SAVEDS ASM BOOL Check( REG(d0) BPTR fh,
                       REG(a6) EXTBASE *ExtBase )
{
    struct Library *DOSBase = ExtBase->lb_DOS; /* You're gonna need the read-stuff anyways */

    PDebug(MYNAME": Check()\n");

    /*
     *  Read bytes from file, then return TRUE if we can handle this
     *  type of file
     */

    return FALSE; /* By default, reject the stuff. */
}

/*-----------------------------------------------------------------*/
/*                          END OF CODE                            */
/*-----------------------------------------------------------------*/
