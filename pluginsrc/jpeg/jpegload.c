

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

#include <setjmp.h>

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/

void my_init_source( j_decompress_ptr cinfo )
{
    my_src_ptr src = (my_src_ptr) cinfo->src;

    src->start_of_file = TRUE;
}

boolean my_fill_input_buffer( j_decompress_ptr cinfo )
{
    my_src_ptr src = (my_src_ptr) cinfo->src;
    my_error_ptr jerr = (my_error_ptr) cinfo->err;
    LONG nbytes;
    struct Library *DOSBase, *SysBase;

    DOSBase = jerr->PPTBase->lb_DOS;

//    PDebug("\tReading from file...\n");
//    PDebug("\tRead(fh=%08X,buf=%08X,%lu)\n",src->fh,src->buffer, INPUT_BUF_SIZE);
//    Wait(0L);
    nbytes = Read(src->fh, src->buffer, INPUT_BUF_SIZE);
    if(nbytes <= 0) {
        if(src->start_of_file)
            ERREXIT(cinfo, JERR_INPUT_EMPTY);
        WARNMS(cinfo,JWRN_JPEG_EOF);
        src->buffer[0]=(JOCTET)0xFF;
        src->buffer[1]=(JOCTET)JPEG_EOI;
        nbytes = 2;
    }
    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = nbytes;
    src->start_of_file = FALSE;

    return TRUE;
}


void my_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    my_src_ptr src = (my_src_ptr) cinfo->src;

    /* BUG: Should use Seek() */

//    PDebug("\tSkipping %lu bytes\n",num_bytes);
    if (num_bytes > 0) {
        while (num_bytes > (long) src->pub.bytes_in_buffer) {
            num_bytes -= (long) src->pub.bytes_in_buffer;
            (void) my_fill_input_buffer(cinfo);
        }
        src->pub.next_input_byte += (size_t) num_bytes;
        src->pub.bytes_in_buffer -= (size_t) num_bytes;
    }
}

void term_source( j_decompress_ptr cinfo )
{
    /* NO CODE */
}

void jpeg_amiga_src( j_decompress_ptr cinfo, BPTR fh )
{
    my_src_ptr src;
    struct Library *SysBase;

    SysBase = (struct Library *) SYSBASE();

    if (cinfo->src == NULL) {     /* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
            SIZEOF(struct my_source_mgr));
        src = (my_src_ptr) cinfo->src;
        src->buffer = (JOCTET *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
            INPUT_BUF_SIZE * SIZEOF(JOCTET));
    }

    src = (my_src_ptr) cinfo->src;
    src->pub.init_source = my_init_source;
    src->pub.fill_input_buffer = my_fill_input_buffer;
    src->pub.skip_input_data = my_skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart;
    src->pub.term_source = term_source;
    src->fh = fh;
    src->pub.bytes_in_buffer = 0;
    src->pub.next_input_byte = NULL;
}

