/*
    PROJECT: ppt
    MODULE : load.c

    $Id: load.c,v 6.2 1999/11/25 23:15:46 jj Exp $

    Code for loaders...

    This code is executed in multiple threads.
*/

#include "defs.h"
#include "misc.h"

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

#include <libraries/asl.h>

#include <clib/utility_protos.h>
#include <clib/alib_protos.h>

#include "proto/iomod.h"

#include "gui.h"

#include <ctype.h>

/*---------------------------------------------------------------------*/
/* Prototypes */

Local     PERROR         DoTheLoad( FRAME *, EXTBASE *, char *, char *, char *, BOOL );
Prototype VOID ASM       LoadPicture( REGDECL(a0,UBYTE *) );
Prototype FRAME *        RunLoad( char *fullname, UBYTE *loader, UBYTE *argstr );

/*---------------------------------------------------------------------*/
/* Global variables */


/*---------------------------------------------------------------------*/
/* Local constants */

/* Tells the default endings for a given external module. Note that the
   index to this array is counted at NT_USER-NT_<type>. If you put long
   patterns, change PPNCBUFSIZE */

const char *external_patterns[] = {
    "#?.iomod","#?.effect","#?."REXX_EXTENSION
};


/*---------------------------------------------------------------------*/
/* Code */

/*
    Works as a front end to the LoadPicture() call. argstr is
    for AREXX. Returns the frame address or NULL for failures.
*/
FRAME *RunLoad( char *fullname, UBYTE *loader, UBYTE *argstr )
{
    char argbuf[ARGBUF_SIZE], t[MAXPATHLEN];
    struct Process *p = NULL;
    FRAME *frame;

    D(bug("RunLoad()\n"));

    frame = NewFrame(0,0,0,globxd); /* Allocate with no buffers. */
    if(frame == NULL) {
        D(bug("ERROR: Unable to allocate a new frame in RunLoad()\n"));
        return NULL;
    }

    if(ObtainFrame( frame, BUSY_LOADING ) == FALSE) {
        RemFrame( frame, globxd );
        return NULL;
    }

    if( fullname ) {
        UBYTE *s;

        /* BUG: Stupid way of setting the zero at the end of the string */
        bzero( frame->path, MAXPATHLEN );
        s = PathPart( fullname );
        strncpy(frame->path,fullname, (s-fullname));

        MakeFrameName( FilePart(fullname), frame->name, NAMELEN, globxd );
    } else {
        strcpy(frame->path,"");
        MakeFrameName( NULL, frame->name, NAMELEN, globxd );
    }

    sprintf(argbuf,"%lu PATH=\"%s\" NAME=\"%s\"",frame,frame->path, frame->name);

    if(argstr) {
        sprintf(t," %s",argstr);
        strcat( argbuf, t );
    }

    if(loader) {
        sprintf(t," LOADER=\"%s\"", loader);
        strcat( argbuf, t );
    }

    /*
     *  Info window allocation must be done by this task, otherwise
     *  the BGUI messages can go haywire
     */

    if(AllocInfoWindow( frame, globxd ) != PERR_OK ) {
        ReleaseFrame(frame);
        RemFrame(frame,globxd);
        return NULL;
    }

    /*
     *  Lock the frame so that the other task will block until we've
     *  added this frame to the temporary frame list.
     */

    LOCK(frame);

#ifdef DEBUG_MODE
    p = CreateNewProcTags( NP_Entry, LoadPicture, NP_Cli, FALSE,
                           NP_Output, frame->debug_handle = OpenDebugFile( DFT_Load ),
                           NP_CloseOutput,TRUE, NP_Name, "PPT_Load",
                           NP_StackSize, globals->userprefs->extstacksize,
                           NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf, TAG_END );
#else
    p = CreateNewProcTags( NP_Entry, LoadPicture, NP_Cli, FALSE, NP_Output, Open("NIL:",MODE_NEWFILE),
                           NP_CloseOutput, TRUE, NP_Name, "PPT_Load",
                           NP_StackSize, globals->userprefs->extstacksize,
                           NP_Priority, globals->userprefs->extpriority, NP_Arguments, argbuf, TAG_END );
#endif

    if(!p) {
        Req(NEGNUL,NULL,GetStr(MSG_PERR_NO_NEW_PROCESS));
        DeleteInfoWindow( frame->mywin,globxd );
        ReleaseFrame( frame );
        RemFrame(frame,globxd);
        frame = NULL;
    } else {
        frame->currproc = p;

        /*
         *  Add the frame into the temporary frame list, from which it will
         *  be moved to the real system list if the loading succeeds.
         *  In the end, we'll release the lock and allow the other
         *  task to proceed.
         */

        LOCKGLOB();
        D(bug("\tAdding %08X to frames list...\n",frame));

        AddFrame( frame ); // BUG: Should check

        UNLOCKGLOB();
        UNLOCK(frame);
    }

    return frame;
}

/// Filechecking routines
/*
    Make a simple check using the loader's IOCheck()-routine.

    Returns:  1 if found
              0 if check failed
             -1 on error (no checker)
*/
Local
LONG CheckOne( BPTR fh, LOADER *ld, UBYTE *init_bytes, EXTBASE *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    struct Library *IOModuleBase = NULL;
    ULONG res;

    IOModuleBase = OpenModule( ld, 0L, PPTBase);

    if(IOModuleBase) {

        D(bug("\tChecking with '%s'...\n", ld->info.nd.ln_Name));
        Seek( fh, 0L, OFFSET_BEGINNING ); /* Ensure we are at the beginning of the file */
        res = IOCheck( fh, INIT_BYTES, init_bytes, PPTBase );
        CloseModule( IOModuleBase, PPTBase );

        if(res) {
            D(bug("\t\tMatch found!", ld->info.nd.ln_Name ));
            return 1;
        }

    } else {
        D(bug("Error opening loader %s!\n",ld->info.nd.ln_Name));
        return -1;
    }

    return 0;
}

/*
    This routine goes through all the loaders and does a Check() for
    every one that has a PPTX_PostFixPattern defined.
*/

Local
LOADER *CheckFilePattern( BPTR fh, STRPTR filename, UBYTE *init_bytes, EXTBASE *PPTBase )
{
    struct Node *cn;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    UBYTE  pattbuf[MAXPATTERNLEN*2+8]; /* Some extra */

    D(bug("CheckFilePattern(%s)\n",filename));

    for( cn = globals->loaders.lh_Head; cn->ln_Succ; cn = cn->ln_Succ ) {
        LOADER *ld;

        ld = (LOADER *)cn;

        D(bug("\t%s...\n", ld->info.realname));

        if( ld->postfixpat[0] ) {
            if(ParsePatternNoCase( ld->postfixpat, pattbuf, MAXPATTERNLEN*2+2 ) >= 0 ) {
                if( MatchPatternNoCase(pattbuf, filename) ) {
                    D(bug("\t\tProbable match found by %s\n", ld->info.realname ));

                    if( CheckOne( fh, ld, init_bytes, PPTBase ) > 0 ) {
                        return ld;
                    } else {
                        D(bug("\t\tbut it was not recognized...\n"));
                    }
                }
            } else {
                InternalError("Pattern overflow!");
            }
        }

    } /* for */

    return NULL;
}

/*
    This routine goes through all loaders and calls the Check() - routine
    for the given filehandle.Returns NULL, if no recognition was possible,
    otherwise pointer to the loader who recognised the file. Please note
    that fh should be open upon entering.
 */

Local
LOADER *CheckFileH( EXTBASE *PPTBase, UBYTE *init_bytes, BPTR fh )
{
    struct Node *cn;

    D(bug("CheckFileH( %08X )\n",fh));

    for( cn = globals->loaders.lh_Head; cn->ln_Succ; cn = cn->ln_Succ ) {

        if( CheckOne( fh, (LOADER *)cn, init_bytes, PPTBase ) > 0) {
            return (LOADER *)cn;
        }

    } /* for */

    return NULL;
}

/*
    Master routine for checking file types.
*/
Local
LOADER *CheckFileType( BPTR fh, STRPTR filename, EXTBASE *PPTBase )
{
    LOADER *ld;
    APTR DOSBase = PPTBase->lb_DOS;
    UBYTE init_bytes[INIT_BYTES+2]; // buffer space
    LONG err;

    /*
     *  Read the INIT_BYTES bytes from the file into memory
     *  so that IOCheck()-routine can do the faster check.
     */

    SetIoErr(0L);
    FRead( fh, init_bytes, INIT_BYTES, 1 );
    if((err = IoErr()) > 0) {
        char buf[80];

        Fault(err, "Error while checking:", buf, 79 );
        D(bug(buf));

        return NULL;
    }

    /*
     *  First, do the postfix pattern based check
     *  Then,  go through each of the loaders and make the check.
     */

    if(!(ld = CheckFilePattern(fh, filename, init_bytes, PPTBase))) {
        ld = CheckFileH(PPTBase, init_bytes, fh);
    }

    return ld;
}
///

/// LoadPicture()
/*
    This is just a simple front-end to the loader routines.
*/
VOID SAVEDS ASM LoadPicture( REGPARAM(a0,UBYTE *,argstr) )
{
    EXTBASE *PPTBase;
    APTR DOSBase;
    struct PPTMessage *msg;
    FRAME *frame = NULL;
    char *path = NULL, *name = NULL, *loadername = NULL;
    int res = PERR_GENERAL;
    ULONG *optarray = NULL;
    BOOL rexx;

    D(bug("LoadPicture(%s)\n",argstr));

    if( (PPTBase = NewExtBase(TRUE)) == NULL) {
        D(bug("LIB BASE ALLOCATION FAILED\n"));
        goto errorexit;
    }

    /*
     *  Read possible REXX commands
     */

    if(optarray = ParseDOSArgs( argstr, "FRAME/A/N,PATH/K,ARGS/K,LOADER/K,NAME/K,REXX/S", PPTBase ) ) {
        frame = (FRAME *) *( (ULONG *)optarray[0]) ;
        if(optarray[1]) /* A path was given */
            path = (UBYTE *)optarray[1];

        if(optarray[4]) /* A name was given */
            name = (UBYTE *)optarray[4];

        if(optarray[3]) /* The loader type was given */
            loadername = (UBYTE *)optarray[3];

        rexx = (BOOL)optarray[5];

    } else {
        InternalError( "LoadPicture(): REXX message of incorrect format" );
        goto errorexit;
    }

    if(NewTaskProlog(frame,PPTBase) != PERR_OK) goto errorexit;

    DOSBase = PPTBase->lb_DOS;

    res = DoTheLoad( frame, PPTBase, path, name, loadername, rexx );

errorexit:
    if(optarray)
        FreeDOSArgs( optarray, PPTBase );

    msg = AllocPPTMsg( sizeof(struct PPTMessage), PPTBase );
    msg->frame = frame;
    msg->code = PPTMSG_LOADDONE;
    msg->data = (APTR)res;

    /* Send the message */
    SendPPTMsg( globals->mport, msg, PPTBase );

    WaitDeathMessage( PPTBase );

    EmptyMsgPort( PPTBase->mport, PPTBase );

    if(PPTBase) RelExtBase(PPTBase);
}
///

/// PERROR DoTheLoad( FRAME *frame, EXTBASE *PPTBase, char *path, char *name, char *loadername, BOOL rexx )

/*
    This routine takes care of all picture loading, etc. Name and path must
    point to valid directions.
*/

Local
PERROR DoTheLoad( FRAME *frame, EXTBASE *PPTBase, char *path, char *name, char *loadername, BOOL rexx )
{
    APTR DOSBase = PPTBase->lb_DOS, UtilityBase = PPTBase->lb_Utility, SysBase = PPTBase->lb_Sys;
    BPTR fh = NULL;
    struct Library *IOModuleBase = NULL;
    LOADER *ld = NULL;
    UBYTE ec[80];
    BOOL res = PERR_OK, nofile = FALSE;
    PERROR errcode = PERR_OK;
    struct TagItem loadertags[] = {
        PPTX_FileName, NULL,
        {TAG_END}
    };
    char fullname[MAXPATHLEN];
    int a;


    if(!CheckPtr(frame,"DoTheLoad() - frame"))
        return PERR_ERROR;

    D(bug("DoTheLoad(%s,%s,%s)\n",path ? path : "NULL",
                                  name ? name : "NULL",
                                  loadername ? loadername : "NULL" ));

    if( loadername ) {
        SHLOCKGLOB();
        ld = (LOADER *) FindIName( &globals->loaders, loadername );
        UNLOCKGLOB();
    }

    if( ld == NULL ) {
        nofile = FALSE; /* We're gonna need the file anyway */
    } else {
        IOModuleBase = OpenModule( ld, 0L, PPTBase );
        if(IOModuleBase) {
            nofile = IOInquire( PPTX_NoFile, PPTBase );
            CloseModule(IOModuleBase,PPTBase);
        } else {
            InternalError("Invalid module detected!");
        }
    }

    /*
     *  Attempt to open the file, if one was specified.
     *  BUG: What if there was none???
     */

    if( (ld == NULL) || ( nofile == FALSE ) ) {

        if( path && name && name[0] != '\0' ) {

            strcpy(fullname, path);
            AddPart(fullname, name, MAXPATHLEN);

            DeleteNameCount( fullname );

            loadertags[0].ti_Data = (ULONG)fullname;

            fh = Open( fullname, MODE_OLDFILE );
            if(!fh) {
                errcode = IoErr();
                Fault( errcode, "", ec, 79 );
                if( rexx ) {
                    SetErrorCode( frame, PERR_FILEOPEN );
                } else {
                    XReq(NEGNUL,NULL,
                         XGetStr(mLOAD_CANNOT_OPEN_FILE), fullname, errcode, ec );
                }
                res = PERR_FILEOPEN;
                goto errexit;
            }
            D(bug("\tOpened file\n"));
        } else {
            XReq(NEGNUL,NULL,XGetStr(mLOAD_NO_FILE_SPECIFIED) );
            return PERR_FAILED;
        }
    } else {
        D(bug("\tLoader does not need a file!\n"));
    }

    /*
     *  Attempt to recognise the file, either from the user or the file.
     */

    if( res == PERR_OK  ) {

        /*
         *  Make a guess about the loader by asking each of them separately
         *  if they can recognize this file.  Unless we were told which
         *  loader to use, naturally.
         */

        if( !ld ) {
            UpdateProgress(frame,XGetStr(mLOAD_CHECKING_FILETYPE),0L, PPTBase);
            OpenInfoWindow(frame->mywin,PPTBase);

            if( fh ) {
                ld = CheckFileType( fh, name, PPTBase );
            } else {
                InternalError("No file or loader specified!");
                res = PERR_FAILED;
            }
        }

        if( ld ) {

            D(bug("\tRecognised file type, now loading...\n"));

            IOModuleBase = OpenModule( ld, 0L, PPTBase );

            if(IOModuleBase) {
                D(APTR foo);

                if(fh) Seek(fh,0L,OFFSET_BEGINNING); /* Ensure we are in the beginning. */

                frame->origtype = ld;

                D(bug("*==*==*==*==*==*==*==*==*==*==*==*==*==*==*==*==*\n"));
                D(foo=StartBench());

                errcode = IOLoad( fh, frame, loadertags, PPTBase );

                D(StopBench(foo));
                D(bug("*==*==*==*==*==*==*==*==*==*==*==*==*==*==*==*==*\n"));

                if(MasterQuit)
                    goto errexit;

                switch(errcode) {
                    case PERR_OK:
                        D(bug("Load successfull\n"));
                        break;

                    case PERR_WARNING:

                        /*
                         *  Display an error message.
                         */

                        a = XReq( NEGNUL, XGetStr(MSG_KEEP_DISCARD_GAD),
                                  XGetStr(mWARNING_FROM_LOADER),
                                  fullname, GetErrorMsg(frame,PPTBase) );

                        if( a == 0 ) {
                            res = PERR_FAILED;
                        } else {
                            res = PERR_OK;
                            ClearError( frame );
                        }
                        break;

                    case PERR_BREAK:
                    case PERR_CANCELED:
                        D(bug("Break or cancel\n"));
                        res = PERR_FAILED;
                        break;

                    default:
                        D(bug("Loader reports error\n"));

                        if( frame->doerror && frame->errorcode != PERR_CANCELED
                            && frame->errorcode != PERR_BREAK && !rexx) {

                                XReq(NEGNUL,XGetStr(mUNDERSTOOD),
                                     XGetStr(mERROR_FROM_LOADER),
                                     ld->info.nd.ln_Name, fullname, GetErrorMsg(frame,PPTBase) );

                                ClearError( frame );
                            }

                        res = PERR_FAILED;
                        break;

                } /* switch */

            } else {
                XReq(NEGNUL,NULL,"\nLoader %s cannot load?!?\n",ld->info.nd.ln_Name);
                res = PERR_FAILED;
            }
        } else {
            XReq(NEGNUL, NULL, XGetStr(mLOAD_COULDNT_RECOGNIZE), fullname );
            res = PERR_FAILED;
        }
    }

errexit:

    if( IOModuleBase ) CloseModule( IOModuleBase, PPTBase );

    if( fh ) {
        Close(fh);
        D(bug("Closed file\n"));
    }

    CloseInfoWindow( frame->mywin, PPTBase );

    ClearProgress( frame, PPTBase );

    return res;
}

///

/// PERROR FetchExternals(const char *path, UBYTE type)

/*
    This routine loads external modules to the system. You should give it
    a pointer to the path and a const telling which type should be loaded
*/

Prototype PERROR FetchExternals(const char *path, UBYTE type, STRPTR lead );

PERROR FetchExternals(const char *path, UBYTE type, STRPTR lead )
{
    struct ExAllControl *eac;
    BPTR lock;
    BOOL more;
    struct ExAllData *ead, *eadorig;
    char ppcbuf[PPNCBUFSIZE], filename[MAXPATHLEN+1], dispbuf[128];

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

    eadorig = ead = (struct ExAllData *)pmalloc( EXALLBUFSIZE );
    if(!ead) {
        UnLock(lock);
        FreeDosObject(DOS_EXALLCONTROL, NULL);
        return PERR_OUTOFMEMORY;
    }

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
                D(bug("Loading file %s\n",filename));

                /* Update display */

                if( lead ) {
                    strcpy( dispbuf, lead );
                    strcat( dispbuf, ead->ed_Name );

                    UpdateStartupWindow( dispbuf );
                }

                if( OpenExternal(filename,type) != PERR_OK ) {
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
///

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

