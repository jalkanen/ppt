/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt
    MODULE : vm.c

    Virtual memory handling routines.

    $Id: vm.c,v 6.0 1999/10/14 16:22:43 jj Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "defs.h"
#include "misc.h"

#ifndef PRAGMAS_DOS_PRAGMAS_H
#include <pragmas/dos_pragmas.h>
#endif

#ifndef PRAGMAS_BGUI_PRAGMAS_H
#include <pragmas/bgui_pragmas.h>
#endif

#ifndef DOS_DOSTAGS_H
#include <dos/dostags.h>
#endif

/*----------------------------------------------------------------------*/
/* Defines */

#define TMPBUF          65536L
#undef  USE_SLOW_SEEK
#undef  VM_DEBUG


/*
    V(bug()) is our a local debugging command
*/

#ifdef VM_DEBUG
#define V(x)            x
#else
#define V(x)
#endif

/*----------------------------------------------------------------------*/
/* Global variables */

/*----------------------------------------------------------------------*/
/* Internal prototypes */


/*----------------------------------------------------------------------*/
/* Code */

/*
    Allocates the vmem handle and initializes it to sensible values
 */

Prototype VMHANDLE *AllocVMHandle(struct PPTBase *);

VMHANDLE *AllocVMHandle(struct PPTBase *PPTBase)
{
    VMHANDLE *vmh;

    vmh = smalloc( sizeof(VMHANDLE) );
    if(!vmh)
        return NULL;

    bzero( vmh, sizeof(VMHANDLE) );

    return vmh;
}

/*
    Frees the vmem handle allocated by AllocVMHandle()
    BUG: Does not remove resources.
 */

Prototype VOID FreeVMHandle( VMHANDLE *vmh, struct PPTBase * );

VOID FreeVMHandle( VMHANDLE *vmh, struct PPTBase *PPTBase )
{
    if( vmh ) sfree(vmh);
}

/*
    This routine will load a chunk to the memory. Loads data
    from offset to offset + size.

    BUG: test which are more common, forward or backward refs.
    BUG: no error checking whatsoever.
    BUG: should really use mode OFFSET_CURRENT for speed.
*/

Prototype PERROR    LoadVMData( VMHANDLE *, ULONG, struct PPTBase * );

PERROR LoadVMData( VMHANDLE *vmh, ULONG offset, struct PPTBase *xd )
{
    LONG seekpos, bufsiz;
    struct DosLibrary *DOSBase = xd->lb_DOS;
    LONG dir;

    V(bug("LoadVMData()\n"));

    if( vmh->vm_fh == 0L ) {
        return PERR_OK; /* No file handle */
    }

    bufsiz  = vmh->end - vmh->begin;

#if 0
    /* If we can fit more than two rows into the buffer, then we put
       the load position around the offset required. */

    if(bufsiz > (ROWLEN(f->pix) >> 1))
        seekpos = offset - (bufsiz >> 1);
    else
        seekpos = offset;
#else
#if 1
    dir = offset - vmh->begin; /* > 0, if we're going forward, < 0, if backwards */

    if( dir > 0 )
        seekpos = offset - (bufsiz >> 2);
    else
        seekpos = offset - bufsiz + (bufsiz >> 2);
#else
    seekpos = offset - (bufsiz >> 1);
#endif
#endif

    if(seekpos < 0) seekpos = 0; /* Check for boundaries */
    if(seekpos >= vmh->last - bufsiz) seekpos = vmh->last - bufsiz;

    V(bug("\tSeek %X to %x : seekpos = %ld, bufsiz = %lu\n",vmh->vm_fh, vmh->data, (LONG)seekpos - (LONG)vmh->end, bufsiz));

#ifdef USE_SLOW_SEEK
    Seek(vmh->vm_fh, seekpos, OFFSET_BEGINNING);
#else
    Seek( vmh->vm_fh, (LONG) seekpos - (LONG)vmh->end, OFFSET_CURRENT );
#endif

    Read(vmh->vm_fh, vmh->data, bufsiz);

    /* Update begin and end */
    vmh->begin = seekpos;
    vmh->end   = seekpos + bufsiz;

    V(bug("\tNew begin and end: %lu ---> %lu\n",vmh->begin, vmh->end));

    return PERR_OK;
}


/*
    Save the buffer back to disk. Clears chflag.

    BUG: no error checking, returns OK always
    BUG: should really use OFFSET_CURRENT for speed
*/

Prototype PERROR    SaveVMData( VMHANDLE *, ULONG, struct PPTBase * );

PERROR SaveVMData( VMHANDLE *vmh, ULONG offset, struct PPTBase *xd )
{
    LONG bufsiz,seekpos;
    struct DosLibrary *DOSBase = xd->lb_DOS;

    V(bug("SaveVMData()\n"));

    if( !vmh->vm_fh ) {
        return PERR_OK;
    }

    bufsiz = vmh->end - vmh->begin;
    seekpos = (LONG)offset;

    V(bug("\tSaving data @ %x : seekpos = %ld, bufsiz = %lu\n",vmh->data,seekpos - (LONG)vmh->end,bufsiz));

#ifdef USE_SLOW_SEEK
    Seek(vmh->vm_fh, seekpos, OFFSET_BEGINNING);
#else
    vmh->end = Seek( vmh->vm_fh, (LONG) seekpos - (LONG)vmh->end, OFFSET_CURRENT );
#endif

    Write(vmh->vm_fh, vmh->data, bufsiz);

    vmh->chflag = 0;
    return PERR_OK;
}


/*
    This will flush the current buffer back to disk.
    BUG: should really observe any changes.
*/

Prototype PERROR    FlushVMData( VMHANDLE *, struct PPTBase * );

PERROR FlushVMData( VMHANDLE *vmh, struct PPTBase *xd )
{
    ULONG offset;

    if(vmh->chflag) {
        offset = vmh->begin;
        return(SaveVMData( vmh, offset, xd ));
    }
    return PERR_OK;
}


/*
    Creates a new VM file for usage. The frame must be initialized properly
    before calling (that is, xsize and ysize must be correct)
    The file handle is left to point at the beginning of the file.
*/

Prototype PERROR CreateVMData( VMHANDLE *, const ULONG, struct PPTBase * );

PERROR CreateVMData( VMHANDLE *vmh, const ULONG size, struct PPTBase *PPTBase )
{
    char vmfile[MAXPATHLEN],t[40];
    BPTR fh;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    struct Library *UtilityBase = PPTBase->lb_Utility;
    BOOL tmp_name_ok = FALSE;
    static ULONG id = 0;
    ULONG siz = size;

    D(bug("CreateVMData( size = %lu )\n",size));

    /*
     *  Create VM file name by using vm_id tag.
     */

    while(tmp_name_ok == FALSE) {
        BPTR lock;

        /*
         *  Pick up a new identification code
         */

        if( UtilityBase->lib_Version >= 39 ) {
            vmh->vm_id = GetUniqueID();
        } else {
            LOCKGLOB();
            vmh->vm_id = id++;
            UNLOCKGLOB();
        }

        strcpy(vmfile,globals->userprefs->vmdir);
        sprintf(t,"%s.%X",VM_FILENAME,vmh->vm_id);
        AddPart(vmfile,t,MAXPATHLEN);

        /*
         *  If locking succeeds, then the file must already exist.
         */

        lock = Lock(vmfile, ACCESS_READ);
        if( lock == 0L ) {
            tmp_name_ok = TRUE;
        }

        UnLock( lock ); /* Zero is harmless */
    }

    fh = Open(vmfile,MODE_NEWFILE);

    /*
     *  Initialize VM filehandle
     */


    if(siz > globals->userprefs->vmbufsiz * 1024L )
        siz = globals->userprefs->vmbufsiz * 1024L;

    vmh->vm_fh = fh;
    vmh->begin = 0L;
    vmh->end   = siz;
    vmh->last  = size;

    if(!fh) {
        D(bug("Couldn't open file %s\n",vmfile));
        XReq( NEGNUL, NULL, XGetStr(mVM_NO_SWAP_FILE) );
        return PERR_FILEOPEN;
    }

    D(bug("\tPreparing file '%s' as VM (%lu bytes)\n",vmfile,size));

    if(SetFileSize(fh,size,OFFSET_BEGINNING ) == -1) {
        D(bug("\tSetFileSize() failed!\n"));
        DeleteVMData( vmh, PPTBase );
        XReq( NEGNUL, NULL, XGetStr(mVM_CANNOT_CREATE_FILE),size);
        return PERR_OUTOFMEMORY;
    }


    Seek( fh, vmh->end, OFFSET_BEGINNING );

    return PERR_OK;
}


Prototype PERROR CloseVMFile( VMHANDLE *vmh, struct PPTBase *PPTBase );

PERROR CloseVMFile( VMHANDLE *vmh, struct PPTBase *PPTBase )
{
    char vmfile[MAXPATHLEN+1];
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    if( vmh->vm_fh ) {
        NameFromFH( vmh->vm_fh, vmfile, MAXPATHLEN );

        D(bug("\tDeleting file '%s'...",vmfile));

        if(Close(vmh->vm_fh) != FALSE) {
            if( DeleteFile(vmfile) == FALSE) {
                D(bug("WARNING! Unable to delete VM file %s\n",vmfile));
            }
            vmh->vm_fh = NULL;
        } else {
            D(bug("WARNING! Unable to close filehandle\n"));
        }
    }
    return PERR_OK;
}

/*
    Handle the closing of the vm file. Do not use the VMHANDLE for
    anything after calling this, since it is freed.
*/

Prototype PERROR    DeleteVMData( VMHANDLE *, struct PPTBase * );

PERROR DeleteVMData( VMHANDLE *vmh, struct PPTBase *PPTBase )
{
    D(bug("DeleteVMData()\n"));

    CloseVMFile( vmh, PPTBase );
    FreeVMHandle(vmh, PPTBase);

    D(bug("Done!\n"));

    return PERR_OK;
}


/*
    Use this operation to restore the sanity of the VM handles, if
    you have done anything with the actual files. This is NOT
    recommended, however. You should only use LoadVMData()
    and SaveVMData().

    BUG: Does not return a proper error value.
*/

Prototype PERROR    SanitizeVMData( VMHANDLE *, struct PPTBase * );

PERROR SanitizeVMData( VMHANDLE *vmh, struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    ULONG size = vmh->last;

    V(bug("SanitizeVMData()\n"));

    if( vmh->vm_fh ) {

        if(size > (vmh->end - vmh->begin) )
            size = vmh->end - vmh->begin;

        Seek( vmh->vm_fh, size, OFFSET_BEGINNING );
        vmh->begin = 0;
        vmh->end   = size;
        LoadVMData( vmh, 0L, PPTBase );
    }

    return PERR_OK;
}

/*
 *  Cleans old files from the VM directory.
 *
 *  BUG: Relies on the Delete command to be found
 *       from the user's path.
 */

Prototype PERROR    CleanVMDirectory( struct PPTBase * );

PERROR CleanVMDirectory( struct PPTBase *PPTBase )
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    struct TagItem tags[] = {
        SYS_Input,  0L,
        SYS_Output, 0L,
        TAG_DONE,   0L
    };
    char buffer[256] = VM_FILENAME, path[256];

    tags[0].ti_Data = Open("NIL:",MODE_NEWFILE);
    tags[1].ti_Data = Open("NIL:",MODE_NEWFILE);

    strcat(buffer,"#?");

    strcpy( path, globals->userprefs->vmdir );
    AddPart( path, buffer, 256 );
    sprintf(buffer, "Delete %s QUIET", path );

    D(bug("Emptying VM dir: '%s'\n",buffer));
    SystemTagList( buffer, tags );

    return PERR_OK;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

