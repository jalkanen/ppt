#include <exec/types.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>

#include <utility/tagitem.h>

#include <pragmas/exec_pragmas.h>
#include <pragmas/utility_pragmas.h>
#include <pragmas/dos_pragmas.h>



#include "ppt.h"
#include "fortify.h"
#include "jpeg.h"

/*-------------------------------------------------------------------------*/

void my_init_dst( j_compress_ptr cinfo )
{
    my_dest_ptr dest = (my_dest_ptr)cinfo->dest;

    dest->buffer = (UBYTE *)
        (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_IMAGE,
                                   OUTPUT_BUF_SIZE);
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

boolean my_empty_dst( j_compress_ptr cinfo )
{
    my_dest_ptr dest = (my_dest_ptr)cinfo->dest;
    my_error_ptr jerr = (my_error_ptr)cinfo->err;
    APTR DOSBase,SysBase;

    DOSBase = jerr->PPTBase->lb_DOS;
//    SysBase = (struct Library *) (* ((ULONG *)4L));
//    DOSBase = OpenLibrary( "dos.library", 33 );

    if(Write( dest->fh, dest->buffer, OUTPUT_BUF_SIZE ) != OUTPUT_BUF_SIZE) {
        ERREXIT(cinfo,JERR_FILE_WRITE);
    }

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

//    CloseLibrary(DOSBase);
    return TRUE;
}

void my_term_dst( j_compress_ptr cinfo )
{
    my_dest_ptr dest = (my_dest_ptr)cinfo->dest;
    ULONG datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
    my_error_ptr jerr = (my_error_ptr)cinfo->err;
    APTR DOSBase, SysBase;

    DOSBase = jerr->PPTBase->lb_DOS;
//    SysBase = (struct Library *) (* ((ULONG *)4L));
//    DOSBase = OpenLibrary( "dos.library", 33 );

    if(datacount > 0) {
        if(Write( dest->fh, dest->buffer, datacount ) != datacount) {
            ERREXIT(cinfo,JERR_FILE_WRITE);
        }
    }
    Flush( dest->fh );
//    CloseLibrary(DOSBase);
}

void jpeg_amiga_dst( j_compress_ptr cinfo, BPTR fh )
{
    my_dest_ptr dest;

    if(cinfo->dest == NULL) {
        cinfo->dest = (struct jpeg_destination_mgr *)
            (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                      sizeof(my_destination_mgr));
    }

    dest = (my_dest_ptr) cinfo->dest;
    dest->pub.init_destination = my_init_dst;
    dest->pub.empty_output_buffer = my_empty_dst;
    dest->pub.term_destination = my_term_dst;
    dest->fh = fh;
}
