/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt
    MODULE : vm.c

    Virtual memory handling routines.

    $Id: vm.c,v 1.7 1996/11/17 22:10:23 jj Exp $
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

Prototype PERROR    LoadVMData( VMHANDLE *, ULONG, EXTBASE * );
Prototype PERROR    SaveVMData( VMHANDLE *, ULONG, EXTBASE * );
Prototype VMHANDLE *CreateVMData( ULONG, EXTBASE * );
Prototype PERROR    DeleteVMData( VMHANDLE *, EXTBASE * );
Prototype PERROR    FlushVMData( VMHANDLE *, EXTBASE * );
Prototype PERROR    SanitizeVMData( VMHANDLE *, EXTBASE * );
Prototype PERROR    CleanVMDirectory( EXTBASE * );

/*----------------------------------------------------------------------*/
/* Code */

/*
    This routine will load a chunk to the memory. Loads data
    from offset to offset + size.

    BUG: test which are more common, forward or backward refs.
    BUG: no error checking whatsoever.
    BUG: should really use mode OFFSET_CURRENT for speed.
*/

PERROR LoadVMData( VMHANDLE *vmh, ULONG offset, EXTBASE *xd )
{
    LONG seekpos, bufsiz;
    struct DosLibrary *DOSBase = xd->lb_DOS;
    LONG dir;

    V(bug("LoadVMData()\n"));

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

PERROR SaveVMData( VMHANDLE *vmh, ULONG offset, EXTBASE *xd )
{
    LONG bufsiz,seekpos;
    struct DosLibrary *DOSBase = xd->lb_DOS;

    V(bug("SaveVMData()\n"));
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

PERROR FlushVMData( VMHANDLE *vmh, EXTBASE *xd )
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

    Returns NULL on failure.
*/

VMHANDLE *CreateVMData( ULONG size, EXTBASE *xd )
{
    char vmfile[MAXPATHLEN],t[40];
    BPTR fh;
    struct DosLibrary *DOSBase = xd->lb_DOS;
    BOOL tmp_name_ok = FALSE;
    VMHANDLE *vmh;
    static ULONG id = 0;
    ULONG siz = size;

    D(bug("CreateVMData( size = %lu )\n",size));

    vmh = smalloc( sizeof(VMHANDLE) );
    if(!vmh)
        return NULL;

    bzero( vmh, sizeof(VMHANDLE) );

    vmh->vm_id = id++;

    /*
     *  Create VM file name by using vm_id tag.
     */

    LOCKGLOB();
    while(tmp_name_ok == FALSE) {
        strcpy(vmfile,globals->userprefs->vmdir);
        sprintf(t,"%s.%X",VM_FILENAME,vmh->vm_id);
        AddPart(vmfile,t,MAXPATHLEN);
        fh = Open(vmfile, MODE_OLDFILE);
        if( fh ) { /* This is bad, since it already exists */
            Close(fh);
            vmh->vm_id++; /* Try next free id */
        }
        else
            tmp_name_ok = TRUE;
    }
    UNLOCKGLOB();

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
        XReq( NEGNUL, NULL, "Couldn't open swap file!\n\nPlease check VM settings and directory.");
        sfree(vmh);
        return NULL;
    }

    D(bug("\tPreparing file '%s' as VM (%lu bytes)\n",vmfile,size));

    if(SetFileSize(fh,size,OFFSET_BEGINNING ) == -1) {
        D(bug("\tSetFileSize() failed!\n"));
        DeleteVMData( vmh, xd );
        XReq( NEGNUL, NULL, ISEQ_C"Unable to create virtual memory!\n(You are probably low on disk space)\n\nI need %lu bytes free",size);
        return NULL;
    }


    Seek( fh, vmh->end, OFFSET_BEGINNING );

    return vmh;
}



/*
    Handle the closing of the vm file. Do not use the VMHANDLE for
    anything after calling this, since it is freed.
*/

PERROR DeleteVMData( VMHANDLE *vmh, EXTBASE *xd )
{
    char vmfile[MAXPATHLEN],t[30];
    struct DosLibrary *DOSBase = xd->lb_DOS;

    D(bug("DeleteVMData()\n"));

    strcpy(vmfile,globals->userprefs->vmdir);
    sprintf(t,"%s.%X",VM_FILENAME,vmh->vm_id);
    AddPart(vmfile,t,MAXPATHLEN);

    D(bug("\tDeleting file '%s'...",vmfile));

    if(Close(vmh->vm_fh) != FALSE) {
        if( DeleteFile(vmfile) == FALSE) {
            D(bug("WARNING! Unable to delete VM file %s\n",vmfile));
        }
    } else {
        D(bug("WARNING! Unable to close filehandle\n"));
    }

    sfree( vmh );

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

PERROR SanitizeVMData( VMHANDLE *vmh, EXTBASE *xb )
{
    struct DosLibrary *DOSBase = xb->lb_DOS;
    ULONG size = vmh->last;

    V(bug("SanitizeVMData()\n"));

    if(size > (vmh->end - vmh->begin) )
        size = vmh->end - vmh->begin;

    Seek( vmh->vm_fh, size, OFFSET_BEGINNING );
    vmh->begin = 0;
    vmh->end   = size;
    LoadVMData( vmh, 0L, xb );

    return PERR_OK;
}

PERROR CleanVMDirectory( EXTBASE *ExtBase )
{
    struct DosLibrary *DOSBase = ExtBase->lb_DOS;
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

