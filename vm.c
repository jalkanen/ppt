/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt
    MODULE : vm.c

    Virtual memory handling routines.

*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include <defs.h>
#include <misc.h>

#include <pragma/dos_pragmas.h>
#include <pragma/bgui_pragmas.h>


/*----------------------------------------------------------------------*/
/* Defines */

#define TMPBUF          65536L


/*----------------------------------------------------------------------*/
/* Global variables */

/*----------------------------------------------------------------------*/
/* Internal prototypes */

Prototype int       LoadVMData( VMHANDLE *, ULONG, EXTDATA * );
Prototype int       SaveVMData( VMHANDLE *, ULONG, EXTDATA * );
Prototype VMHANDLE *CreateVMData( ULONG, EXTDATA * );
Prototype int       DeleteVMData( VMHANDLE *, EXTDATA * );
Prototype int       FlushVMData( VMHANDLE *, EXTDATA * );


/*----------------------------------------------------------------------*/
/* Code */

/*
    This routine will load a chunk to the memory. Loads data
    from offset to offset + size.

    BUG: test which are more common, forward or backward refs.
    BUG: no error checking whatsoever.
    BUG: should really use mode OFFSET_CURRENT for speed.
*/

int LoadVMData( VMHANDLE *vmh, ULONG offset, EXTDATA *xd )
{
    LONG seekpos, bufsiz;
    struct Library *DOSBase = xd->lb_DOS;

//    DEBUG("LoadVMData()\n");
    bufsiz  = vmh->end - vmh->begin;

#if 0
    /* If we can fit more than two rows into the buffer, then we put
       the load position around the offset required. */

    if(bufsiz > (ROWLEN(f->pix) >> 1))
        seekpos = offset - (bufsiz >> 1);
    else
        seekpos = offset;
#else
    seekpos = offset - (bufsiz >> 1); /* BUG! Should do something abt this */
#endif

    if(seekpos < 0) seekpos = 0; /* Check for boundaries */
    if(seekpos >= vmh->last - bufsiz) seekpos = vmh->last - bufsiz;

//    DEBUG("\tSeek %X to %x : seekpos = %ld, bufsiz = %lu\n",vmh->vm_fh, vmh->data, (LONG)seekpos - (LONG)vmh->end, bufsiz);

//    vmh->end = Seek(vmh->vm_fh, (LONG) seekpos - (LONG)vmh->end, OFFSET_CURRENT );

    Seek(vmh->vm_fh, seekpos, OFFSET_BEGINNING);
    Read(vmh->vm_fh, vmh->data, bufsiz);

    /* Update begin and end */
    vmh->begin = seekpos;
    vmh->end   = seekpos + bufsiz;

//    DEBUG("\tNew begin and end: %lu ---> %lu\n",vmh->begin, vmh->end);
    return PERR_OK;
}


/*
    Save the buffer back to disk. Clears chflag.

    BUG: no error checking, returns OK always
    BUG: should really use OFFSET_CURRENT for speed
*/

int SaveVMData( VMHANDLE *vmh, ULONG offset, EXTDATA *xd )
{
    LONG bufsiz,seekpos;
    struct Library *DOSBase = xd->lb_DOS;

//    DEBUG("SaveVMData()\n");
    bufsiz = vmh->end - vmh->begin;
    seekpos = (LONG)offset;

//    DEBUG("\tSaving data @ %x : seekpos = %ld, bufsiz = %lu\n",vmh->data,seekpos - (LONG)vmh->end,bufsiz);
//    f->pix->end = Seek(vmh->vm_fh, seekpos - (LONG)vmh->end, OFFSET_CURRENT );
    Seek(vmh->vm_fh, seekpos, OFFSET_BEGINNING);
    Write(vmh->vm_fh, vmh->data, bufsiz);

    vmh->chflag = 0;
    return PERR_OK;
}


/*
    This will flush the current buffer back to disk.
    BUG: should really observe any changes.
*/

int FlushVMData( VMHANDLE *vmh, EXTDATA *xd )
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
    It will also load the first buffer to mem to update begin and end.

    Returns NULL on failure.
*/

VMHANDLE *CreateVMData( ULONG size, EXTDATA *xd )
{
    char vmfile[MAXPATHLEN],t[40];
    BPTR fh;
    struct Library *DOSBase = xd->lb_DOS, *BGUIBase = xd->lb_BGUI;
    BOOL tmp_name_ok = FALSE;
    VMHANDLE *vmh;
    static ULONG id = 0;

    D(bug("CreateVMData( size = %lu )\n",size));

    vmh = pmalloc( sizeof(VMHANDLE) );
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

    ULONG siz = size;

    if(siz > globals->userprefs->vmbufsiz * 1024L )
        siz = globals->userprefs->vmbufsiz * 1024L;

    vmh->vm_fh = fh;
    vmh->begin = 0L;
    vmh->end   = siz;
    vmh->last  = size;

    if(!fh) {
        D(bug("Couldn't open file %s\n",vmfile));
        XReq( NEGNUL, NULL, "Couldn't open swap file!\n\nPlease check VM settings and directory.");
        pfree(vmh);
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

int DeleteVMData( VMHANDLE *vmh, EXTDATA *xd )
{
    char vmfile[MAXPATHLEN],t[30];
    struct Library *DOSBase = xd->lb_DOS;

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

    pfree( vmh );

    D(bug("Done!\n"));

    return PERR_OK;
}



/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

