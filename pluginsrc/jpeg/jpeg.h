/*------------------------------------------------------------------------*/
/*
    PROJECT: ppt
    MODULE : jpeg.h

    DESCRIPTION:

 */
/*------------------------------------------------------------------------*/

#ifndef JPEG_H
#define JPEG_H


/*------------------------------------------------------------------------*/
/* Includes */

#ifndef PPTPLUGIN_H
#include <pptplugin.h>
#endif

#include <dos/dos.h>
#include <stdio.h>
#include <setjmp.h>
#include <jconfig.h>
#include <jinclude.h>
#include <jpeglib.h>
#include <jerror.h>

/*------------------------------------------------------------------------*/
/* Type defines & structures */

#define MARKER_SOI      0xD8        /* JPEG Start Of Image */

#define MYNAME          "JPEG"

#define INPUT_BUF_SIZE  4096
#define OUTPUT_BUF_SIZE 4096


/* Error manager */


struct my_error_mgr {
    struct jpeg_error_mgr pub;
    struct PPTBase *PPTBase;
    FRAME   *frame;
    jmp_buf setjmp_buffer;
    char    err_buf[256]; /* Used to store warning stuff. */
};

typedef struct my_error_mgr *my_error_ptr;

/* Source manager */

struct my_source_mgr {
    struct jpeg_source_mgr pub;
    BPTR fh;
    UBYTE *buffer;
    BOOL start_of_file;
};

typedef struct my_source_mgr *my_src_ptr;

/* Destination manager */

typedef struct {
    struct jpeg_destination_mgr pub;
    BPTR fh;
    UBYTE *buffer;
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;




/*------------------------------------------------------------------------*/
/* Prototypes */

extern PERROR SaveTC( BPTR, FRAME *,struct TagItem *,struct PPTBase * );

extern void my_init_source( j_decompress_ptr );
extern boolean my_fill_input_buffer( j_decompress_ptr );
extern void my_skip_input_data( j_decompress_ptr, long );
extern void jpeg_amiga_src( j_decompress_ptr, BPTR );
extern void jpeg_amiga_dst( j_compress_ptr, BPTR );

extern int mysetjmp( jmp_buf );
extern void mylongjmp( jmp_buf, int );

/*------------------------------------------------------------------------*/
/* Globals */



#endif /* JPEG_H */

/*------------------------------------------------------------------------*/
/*                          END OF HEADER FILE                            */
/*------------------------------------------------------------------------*/

