/*
    PROJECT: ppt
    MODULE : load.c

    $Id: load.c,v 1.3 1995/10/02 21:35:09 jj Exp $

    Code for loaders...
*/

#include <defs.h>
#include <misc.h>

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef DOS_DOSTAGS_H
#include <dos/dostags.h>
#endif

#ifndef DOS_EXALL_H
#include <dos/exall.h>
#endif

#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif

#include <clib/utility_protos.h>
#include <clib/alib_protos.h>

#include <gui.h>

#include <ctype.h>

/*---------------------------------------------------------------------*/
/* Prototypes */

Local     PERROR      DoTheLoad( FRAME *, EXTDATA *, char *, char *, char * );
Prototype PERROR      FetchExternals(const char *path, UBYTE type);
Prototype __D0 PERROR BeginLoad( __A0 FRAME *,  __A6 EXTDATA * );
Prototype void        EndLoad( __A0 FRAME *, __A6 EXTDATA * );
Prototype void        LoadPicture( __A0 UBYTE * );
Prototype FRAME *     RunLoad( char *fullname, UBYTE *loader, UBYTE *argstr );


/*---------------------------------------------------------------------*/
/* Local constants */

/* Tells the default endings for a given external module. Note that the
   index to this array is counted at NT_USER-NT_<type>. If you put long
   patterns, change PPNCBUFSIZE */

const char *external_patterns[] = {
    "#?.loader","#?.effect"
};


/*---------------------------------------------------------------------*/
/* Code */

/*
    Works as a front end to the LoadPicture() call. argstr is
    for AREXX. Returns the frame address or NULL for failures.
*/
FRAME *RunLoad( char *fullname, UBYTE *loader, UBYTE *argstr )
{
    char argbuf[ARGBUF_SIZE], t[80];
    struct Process *p = NULL;
    FRAME *frame;

    D(bug("RunLoad()\n"));

    frame = NewFrame(0,0,0,globxd); /* Allocate with no buffers. */
    if(frame == NULL) return NULL;

    if(ObtainFrame( frame, BUSY_LOADING ) == FALSE)
        return NULL;

    if( fullname ) {
        strcpy(frame->fullname,fullname);
        MakeFrameName( FilePart(fullname), frame->name, NAMELEN, globxd );
    } else {
        strcpy(frame->fullname,"");
        MakeFrameName( NULL, frame->name, NAMELEN, globxd );
    }

    sprintf(argbuf,"%lu PATH=\"%s\"",frame,frame->fullname);

    if(argstr) {
        sprintf(t," ARGS=\"%s\"",argstr);
        strcat( argbuf, t );
    }

    if(loader) {
        sprintf(t," LOADER=\"%s\"", loader);
        strcat( argbuf, t );
    }

#ifdef DEBUG_MODE
    p = CreateNewProcTags( NP_Entry, LoadPicture, NP_Cli, TRUE, NP_Output, Open("dtmp:load.log",MODE_READWRITE),
                           NP_CloseOutput,TRUE, NP_Name, "PPT Load",
                           NP_Priority, -1, NP_Arguments, argbuf, TAG_END );
#else
    p = CreateNewProcTags( NP_Entry, LoadPicture, NP_Cli, FALSE, NP_Output, Open("NIL:",MODE_NEWFILE),
                           NP_CloseOutput, TRUE, NP_Name, "PPT Load",
                           NP_Priority, -1, NP_Arguments, argbuf, TAG_END );
#endif

errorexit:
    if(!p) {
        Req(NEGNUL,NULL,"Couldn't spawn a new process");
        // DeleteInfoWindow( frame->mywin,globxd );
        ReleaseFrame( frame );
        RemFrame(frame,globxd);
        frame = NULL;
    } else {
        frame->currproc = p;
    }

    return frame;
}


/*
    This routine goes through all loaders and calls the Check() - routine
    for the given filehandle.Returns NULL, if no recognition was possible,
    otherwise pointer to the loader who recognised the file. Please note
    that fh should be open upon entering.
 */

Local
LOADER *CheckFileH( EXTDATA *xd, BPTR fh )
{
    static __D0 int (*L_Check)( __D0 BPTR, __A6 EXTDATA * );
    struct Node *cn;
    APTR DOSBase = xd->lb_DOS, UtilityBase = xd->lb_Utility;

    D(bug("CheckFileH( %08X )\n",fh));


    for( cn = globals->loaders.lh_Head; cn->ln_Succ; cn = cn->ln_Succ ) {
        L_Check = (FPTR)GetTagData( PPTX_Check,(ULONG)NULL,((LOADER *)cn)->info.tags );
        if(L_Check) {
            Seek( fh, 0L, OFFSET_BEGINNING ); /* Ensure we are at the beginning of the file */
            if( (*L_Check)( fh, xd ) == TRUE ) {
                D(bug("\tMatch found by %s\n", cn->ln_Name ));
                return (LOADER *)cn;
            }
        } else {
            D(bug("Loader %s is not able to check for data\n",cn->ln_Name));
        }
    } /* for */

    return NULL;
}

/*
    This is just a simple front-end to the loader routines.
*/
__geta4 void LoadPicture( __A0 UBYTE *argstr )
{
    EXTDATA *xd;
    APTR DOSBase;
    struct PPTMessage *msg;
    FRAME *frame;
    char *fullname = NULL, *loadername = NULL;
    int res = PERR_GENERAL;
    ULONG *optarray = NULL;

    D(bug("LoadPicture(%s)\n",argstr));

    if( (xd = NewExtBase(TRUE)) == NULL) {
        D(bug("LIB BASE ALLOCATION FAILED\n"));
        goto errorexit;
    }

    /*
     *  Read possible REXX commands
     */

    if(optarray = ParseDOSArgs( argstr, "FRAME/A/N,PATH/K,ARGS/K,LOADER/K", xd ) ) {
        frame = (FRAME *) *( (ULONG *)optarray[0]) ;
        if(optarray[1]) /* A path was given */
            fullname = (UBYTE *)optarray[1];

        if(optarray[3]) /* The loader type was given */
            loadername = (UBYTE *)optarray[3];

    } else {
        InternalError( "LoadPicture(): REXX message of incorrect format" );
        goto errorexit;
    }


    DOSBase = xd->lb_DOS;

    res = DoTheLoad( frame, xd, fullname, FilePart(fullname), loadername );

errorexit:
    if(optarray)
        FreeDOSArgs( optarray, xd );

    msg = InitPPTMsg();
    msg->frame = frame;
    msg->code = PPTMSG_LOADDONE;
    msg->data = (APTR)res;

    /* Send the message */
    DoPPTMsg( globals->mport, msg );

    PurgePPTMsg( msg ); /* Remove it */

    RelExtBase(xd);
}

/*
    This routine takes care of all picture loading, etc. Name and path must
    point to valid directions.
*/

PERROR DoTheLoad( FRAME *frame, EXTBASE *xd, char *fullname, char *name, char *loadername )
{
    APTR DOSBase = xd->lb_DOS, UtilityBase = xd->lb_Utility, SysBase = xd->lb_Sys;
    BPTR fh = NULL;
    static __D0 int (*L_Load)( __A0 FRAME *,__D0 BPTR, __A6 EXTDATA *, __A1 struct TagItem * );
    LOADER *ld = NULL;
    BOOL res = PERR_OK;
    char ec[80] = "";
    int errcode = PERR_OK;
    struct TagItem loadertags[] = {
        { PPTX_ErrMsg, ec },
        {TAG_END}
    };

    if(!CheckPtr(frame,"DoTheLoad() - frame"))
        return PERR_ERROR;

    D(bug("DoTheLoad(%s,%s,%s)\n",fullname ? fullname : "NULL",
                                  name ? name : "NULL",
                                  loadername ? loadername : "NULL" ));

    OpenInfoWindow( frame->mywin, xd );

    /*
     *  Attempt to open the file, if one was specified.
     */

    if( fullname && fullname[0] != '\0') {
        fh = Open( fullname, MODE_OLDFILE );
        if(!fh) {
            errcode = IoErr();
            Fault( errcode, "", ec, 79 );
            XReq(NEGNUL,NULL, "Failed to open file '%s' due to:\n"ISEQ_C"Error %ld %s", fullname, errcode, ec );
            res = PERR_WONTOPEN;
            goto errexit;
        }
        D(bug("\tOpened file\n"));
    }

    /*
     *  Attempt to recognise the file, either from the user or the file.
     */

    if( res == PERR_OK  ) {

        /*
         *  Fetch the loader, if necessary
         */

        if( loadername ) {
            SHLOCKGLOB();
            ld = (LOADER *) FindName( &globals->loaders, loadername );
            UNLOCKGLOB();
        } else {
            UpdateProgress(frame,"Checking file type...",0L, xd);
            if( fh ) {
                ld = CheckFileH( xd, fh );
            } else {
                InternalError("No file or loader specified!");
                res = PERR_FAILED;
            }
        }

        if( ld ) {

            D(bug("\tRecognised file type, now loading...\n"));
            L_Load = (FPTR)GetTagData( PPTX_Load, (ULONG)NULL, ld->info.tags );
            if(L_Load) {
                ld->info.usecount++;

                if(fh) Seek(fh,0L,OFFSET_BEGINNING); /* Ensure we are in the beginning. */

                frame->origtype = ld;

                D(bug("*==*==*==*==*==*==*==*==*==*==*==*==*==*==*==*==*\n"));
                D(APTR foo);
                D(foo=StartBench());
                errcode = (*L_Load)( frame, fh, xd, loadertags );
                D(StopBench(foo));
                D(bug("*==*==*==*==*==*==*==*==*==*==*==*==*==*==*==*==*\n"));

                ld->info.usecount--;

                if(MasterQuit)
                    goto errexit;

                if( errcode == PERR_OK && strlen(ec) == 0) {
                    D(bug("Load successfull\n"));
                } else {
                    D(bug("Loader reports error\n"));
                    if( frame->lasterror )
                        frame->lasterror = PERR_OK; /* clear error. */
                    else {
                        if(strlen(ec) != 0) {
                            XReq(NEGNUL,NULL,"Loader %s reports error while reading file\n"ISEQ_C"'%s':\n\n%s",
                                              ld->info.nd.ln_Name, fullname, ec);
                        } else {
                            XReq(NEGNUL,NULL,"Loader %s reports error while reading file\n"ISEQ_C"'%s':\n\n%s",
                                              ld->info.nd.ln_Name, fullname, ErrorMsg(errcode) );
                        }
                    }
                    res = PERR_GENERAL;
                }

            } else {
                XReq(NEGNUL,NULL,"Loader %s cannot load?!?",ld->info.nd.ln_Name);
                res = PERR_GENERAL;
            }
        } else {
            XReq(NEGNUL, NULL, "Cannot recognize file '%s'!", fullname );
            res = PERR_GENERAL;
        }
    }

errexit:

    if( fh ) Close(fh);

    CloseInfoWindow( frame->mywin, xd );

    return res;
}


/****** pptsupport/BeginLoad ******************************************
*
*   NAME
*       BeginLoad -- Initialize frame for loading.
*
*   SYNOPSIS
*       error = BeginLoad( frame );
*       D0                 A0
*
*       PERROR BeginLoad( FRAME * );
*
*   FUNCTION
*       When your loader code is called, PPT will allocate an empty FRAME
*       structure for you. However, it does not know how large the picture
*       is going to be, so it expects you to fill in the necessary data
*       and then call BeginLoad() so that PPT can initialize all necessary
*       internal data structures like virtual memory.
*
*   INPUTS
*       frame - a FILLED FRAME structure for which the loading is about
*           to commence.
*
*   RESULT
*       error - a standard PPT error code. PERR_OK for A-OK.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       EndLoad().
*
******************************************************************************
*
*/

__geta4 __D0 PERROR BeginLoad( __A0 FRAME *frame, __A6 EXTDATA *xd )
{
    APTR SysBase = xd->lb_Sys, DOSBase = xd->lb_DOS;
    int res;

    D(bug("BeginLoad( %08X )\n",frame));

    /*
     *  Add the frame into the temporary frame list, from which it will
     *  be moved to the real system list if the loading succeeds. We must
     *  do this so that EndLoad() won't release us from an non-existant list.
     */

    LOCKGLOB();
    D(bug("\tAdding %08X to templist...\n",frame));

    AddTail( &globals->tempframes, (struct Node *)frame );

    UNLOCKGLOB();


    /*
     *  Next, initialize the frame
     */

    if( (res = InitFrame( frame, xd )) != PERR_OK) {
        D(bug("\tInitFrame() failed, returning %lu\n",res));
        return res;
    }

    /*
     *  Update Info window
     */

    frame->disp->depth = frame->pix->origdepth;
    if(frame->disp->depth > 8) frame->disp->depth = 8;
    frame->disp->ncolors = 1 << frame->disp->depth;

    UpdateInfoWindow( frame->mywin, xd );

    return PERR_OK;
}

/****** pptsupport/EndLoad ******************************************
*
*   NAME
*       EndLoad -- Finish up loading a frame.
*
*   SYNOPSIS
*       EndLoad( frame )
*                A0
*
*       VOID EndLoad( FRAME * );
*
*   FUNCTION
*       After you have finished your loading, you should call this
*       routine. It does not matter whether your loading failed or
*       not, just call it so that PPT knows about it.
*
*   INPUTS
*       frame - obvious?
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       BeginLoad().
*
******************************************************************************
*
*/

__geta4 void EndLoad( __A0 FRAME *frame, __A6 EXTDATA *xd )
{
    APTR SysBase = xd->lb_Sys;

    D(bug("EndLoad(%08X)\n",frame));

    if(!frame) return;

    SHLOCKGLOB();

    Remove((struct Node *)frame); /* Remove it from the tempframes list */

    UNLOCKGLOB();
}



/*
    This routine loads external modules to the system. You should give it
    a pointer to the path and a const telling which type should be loaded
*/

PERROR FetchExternals(const char *path, UBYTE type)
{
    struct ExAllControl *eac;
    BPTR lock;
    BOOL more;
    struct ExAllData *ead, *eadorig;
    char ppcbuf[PPNCBUFSIZE], filename[MAXPATHLEN+1];

    D(bug("FetchExternals(%s,%u)\n",path,type));

    lock = Lock(path, ACCESS_READ);
    if(!lock) {
        D(bug("cannot lock %s\n",path));
        return PERR_WONTOPEN;
    }

    eac = AllocDosObject(DOS_EXALLCONTROL,NULL);
    if(!eac) {
        D(bug("cannot allocdosobject\n"));
        UnLock(lock);
        return PERR_GENERAL;
    }

//    printf("\tPPNC pattern : %s\n",external_patterns[NT_USER - type]);

    ParsePatternNoCase(external_patterns[NT_USER-type],ppcbuf,PPNCBUFSIZE);
    eac->eac_MatchString = ppcbuf;
    eac->eac_LastKey = 0; /* required by DOS */

//    puts("\tAllocDosObject() succeeded");

    eadorig = ead = (struct ExAllData *)pmalloc( EXALLBUFSIZE );
    if(!ead) {
        UnLock(lock);
        FreeDosObject(DOS_EXALLCONTROL, NULL);
        return PERR_OUTOFMEMORY;
    }
//    puts("\tAllocVec() succeeded");

    /* Go through all files in given directory, determine if they fit the
       pattern and attempt to load them, if they seem like the given stuff.
       */

    do {
//        puts("\tExall()");
        more = ExAll( lock, ead, EXALLBUFSIZE, ED_TYPE, eac );
        if( (!more) && (IoErr() != ERROR_NO_MORE_ENTRIES) ) {
            /* ExAll() failed*/
            D(bug("\tExAll() failed\n"));
            break;
        }
        if( eac->eac_Entries == 0 ) {
        //    puts("\tNo entries");
            continue;
        }
//        DEBUG("\t***FOUND %d ENTRIES***\n",eac->eac_Entries);

        do {
            // printf("\t\tFound file %s of type %lu\n",ead->ed_Name,ead->ed_Type);

            if (ead->ed_Type < 0) { /* Is file? */
                strncpy( filename,path, MAXPATHLEN );
                AddPart( filename, ead->ed_Name, MAXPATHLEN );
                D(bug("\t\tLoading file %s\n",filename));
                if( OpenExternal(filename) != PERR_OK ) {
                    Req(NEGNUL,NULL,"Loading external %s failed!",filename);
                }

            } /* if(ead...) */

            ead = ead->ed_Next;
        } while(ead);


    } while(more);

    pfree(eadorig);
    FreeDosObject(DOS_EXALLCONTROL,eac);
    UnLock(lock);
    return PERR_OK;
}
