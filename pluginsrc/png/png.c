/*
    This is PNG loader for PPT

    PPT is (C) Janne Jalkanen 1995-1998.

    $Id: png.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/


#include <pptplugin.h>
#include <stdio.h>
#include <exec/memory.h>

/*
 *  We need this to bypass some stupid definitions in png.h
 */

#ifdef   FAR
#undef   FAR
#define  FAR
#endif

#include "png.h"

/*------------------------------------------------------------------*/
/* Defines */

#define MYNAME          "PNG"

/*------------------------------------------------------------------*/
/* Prototypes */

/*------------------------------------------------------------------*/
/* Global variables and constants */

char infoblurb[256] = ""; /* This is filled out at Init() */

struct my_error {
    BPTR    fh;         /* Does not really belong here... */
    FRAME   *frame;
    struct PPTBase *PPTBase;
};

const struct TagItem MyTagArray[] = {
    PPTX_Load,              TRUE,
    PPTX_ColorSpaces,       CSF_RGB|CSF_GRAYLEVEL|CSF_ARGB|CSF_LUT,

    PPTX_Name,              (ULONG)MYNAME,
    PPTX_Author,            (ULONG)"Janne Jalkanen, 1997-1998",
    PPTX_InfoTxt,           (ULONG)infoblurb,

    PPTX_RexxTemplate,      "COMPRESSION/N",
    PPTX_ReqPPTVersion,     3,

    PPTX_PreferredPostFix,  (ULONG)".png",
    PPTX_PostFixPattern,    (ULONG)"#?.png",

    PPTX_SupportsGetArgs,   TRUE,

    TAG_DONE, 0L
};

struct Values {
    LONG compression;
    LONG filter;
    BOOL interlace;
};

/*------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

struct Library *MathIeeeDoubBasBase = NULL,
               *MathIeeeDoubTransBase = NULL,
               *UtilityBase = NULL;

/*
    Update the infoblurb with the information on the PNG library
*/
LIBINIT
{
    strcat(infoblurb,"Loads Portable Network Graphics image files\n"
                     "and also saves them (truecolor format only)\n\n"
                     "libpng version " );

    strcat(infoblurb, png_libpng_ver );
    strcat(infoblurb, ", zlib version " );
    strcat(infoblurb, zlib_version );

    if( UtilityBase = OpenLibrary("utility.library",36L) ) {
        if( MathIeeeDoubBasBase = OpenLibrary("mathieeedoubbas.library",0)) {
            if( MathIeeeDoubTransBase = OpenLibrary("mathieeedoubtrans.library",0) ){
                return 0;
            }
        }
    }

    return 1;
}

LIBCLEANUP
{
    if( UtilityBase ) CloseLibrary(UtilityBase);
    if( MathIeeeDoubBasBase ) CloseLibrary(MathIeeeDoubBasBase);
    if( MathIeeeDoubTransBase ) CloseLibrary(MathIeeeDoubTransBase );
}

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

/*
    Overrides the default PNG error function.
*/
void user_error_fn(png_structp png_ptr, png_const_charp error_msg )
{
    struct my_error *myerr;
    struct PPTBase *PPTBase;

    myerr = (struct my_error *) png_get_error_ptr(png_ptr);
    PPTBase = myerr->PPTBase;

    SetErrorMsg( myerr->frame, error_msg );

    longjmp( png_ptr->jmpbuf,1 );
}

/*
    Doesn't really do anything, since we have no way of
    sending warnings, yet.
*/
void user_warning_fn(png_structp png_ptr, png_const_charp error_msg )
{
    return;
}

/*
    New I/O functions
*/

void user_read_data(png_structp png_ptr, png_bytep data,
                    png_uint_32 length)
{
    struct my_error *myerr;
    struct DosLibrary *DOSBase;
    struct PPTBase *PPTBase;
    LONG err;

    myerr = (struct my_error *)png_get_io_ptr(png_ptr);
    DOSBase = myerr->PPTBase->lb_DOS;
    PPTBase = myerr->PPTBase;

    if( -1 == Read( myerr->fh, data, length ) ) {
        char buf[80];

        if(err = IoErr()) {
            Fault(err, "Read error: ", buf, 60 );
        } else {
            strcpy(buf,"Read error!");
        }

        SetErrorMsg( myerr->frame, buf );
        longjmp( png_ptr->jmpbuf,1 );
    }
}

void user_write_data(png_structp png_ptr, png_bytep data,
                     png_uint_32 length)
{
    struct my_error *myerr;
    struct DosLibrary *DOSBase;
    struct PPTBase *PPTBase;
    LONG err;

    myerr = (struct my_error *)png_get_io_ptr(png_ptr);
    DOSBase = myerr->PPTBase->lb_DOS;
    PPTBase = myerr->PPTBase;

    if( -1 == Write( myerr->fh, data, length ) ) {
        char buf[80];

        if(err = IoErr()) {
            Fault(err, "Write error: ", buf, 60 );
        } else {
            strcpy(buf,"Write error!");
        }

        SetErrorMsg( myerr->frame, buf );
        longjmp( png_ptr->jmpbuf,1 );
    }
}

void user_flush_data(png_structp png_ptr)
{
    struct my_error *myerr;
    struct DosLibrary *DOSBase;
    struct PPTBase *PPTBase;
    LONG err;

    myerr = (struct my_error *)png_get_io_ptr(png_ptr);
    DOSBase = myerr->PPTBase->lb_DOS;
    PPTBase = myerr->PPTBase;

    if( -1 == Flush( myerr->fh ) ) {
        char buf[80];

        if(err = IoErr()) {
            Fault(err, "Write error: ", buf, 60 );
        } else {
            strcpy(buf,"Write error!");
        }

        SetErrorMsg( myerr->frame, buf );
        longjmp( png_ptr->jmpbuf,1 );
    }
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    UWORD height, width, crow, pass, no_passes = 1;
    png_structp png_ptr;
    png_infop   info_ptr, end_info;
    PERROR res = PERR_OK;
    struct my_error myerr;
    UBYTE buf[10];

    // PDebug(MYNAME" : Load()\n");

    myerr.fh      = fh;
    myerr.frame   = frame;
    myerr.PPTBase = PPTBase;

    /*
     * Check if it's a valid PNG file
     */

    Read(fh, buf, 8);

    if( IOCheck(fh, 8, buf, PPTBase) == FALSE ) {
        SetErrorMsg( frame, "This is not a PNG file!" );
        return PERR_FAILED;
    }

    Seek(fh,0L,OFFSET_BEGINNING);

    /*
     *  Allocate & initialize load objects
     */

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                     (void *)&myerr,
                                     user_error_fn,
                                     user_warning_fn);

    if (!png_ptr) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_OUTOFMEMORY;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_OUTOFMEMORY;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_OUTOFMEMORY;
    }

    /*
     *  Set the error handler
     */

    if( setjmp(png_ptr->jmpbuf) ) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        res = PERR_FAILED;
        goto errorexit;
    }

    /*
     *  Set up the custom IO handler we're now using.
     */

    png_set_read_fn(png_ptr, &myerr, user_read_data);

    /*
     *  Get picture size and initialize the Frame
     */

    png_read_info(png_ptr, info_ptr);

    /*
     *  Modify the PNG palette system
     */

    /* expand paletted colors into true rgb */
    if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);

    /* expand grayscale images to full 8 bits */
    if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY &&
        info_ptr->bit_depth < 8)
        png_set_expand(png_ptr);

    /* Accept tRNS hunks and make an alpha channel out of them */

    if( info_ptr->valid & PNG_INFO_tRNS )
        png_set_expand(png_ptr);

    /* Scale down to 8 bits */

    if( info_ptr->bit_depth == 16 )
        png_set_strip_16(png_ptr);

    /* Handle interlacing */

    if (info_ptr->interlace_type)
        no_passes = png_set_interlace_handling(png_ptr);

    /*
     *  Initialization has been done.  We'll now read the
     *  values we're gonna use because they might have changed.
     */

    png_read_update_info(png_ptr, info_ptr);

    height = frame->pix->height = info_ptr->height;
    width  = frame->pix->width  = info_ptr->width;

    switch(info_ptr->color_type ) {
        case PNG_COLOR_TYPE_RGB:
            frame->pix->colorspace = CS_RGB;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            frame->pix->colorspace = CS_ARGB;
            break;
        case PNG_COLOR_TYPE_GRAY:
            frame->pix->colorspace = CS_GRAYLEVEL;
            break;
        default:
            SetErrorMsg(frame,"I cannot handle this type PNG file");
            goto errorexit;
    }

    frame->pix->components = info_ptr->channels;
    frame->pix->origdepth  = info_ptr->pixel_depth;

    InitProgress( frame, "Loading "MYNAME" picture...", 0, height * no_passes );

    if(InitFrame( frame ) != PERR_OK) {
        res = PERR_FAILED;
        goto errorexit;
    }

    /*
     *  Start decoding
     */

    // PDebug("Start decoding\n");

    for( pass = 0; pass < no_passes; pass++ ) {

        // PDebug("Pass %d\n",pass );

        crow = 0;

        while( crow < height ) {
            ROWPTR cp, dcp;

            // PDebug("\tReading row %d\n",crow);

            if(Progress(frame, (pass * height ) + crow)) {
                res = PERR_BREAK;
                goto errorexit;
            }

            cp = GetPixelRow( frame, crow );

            dcp = cp;

            /*
             *  Get the actual row of pixels, then decompress it into cp
             */

            png_read_rows( png_ptr, &dcp, NULL, 1 );

            PutPixelRow( frame, crow, cp );

            crow++;

        } /* while */

    }
    // PDebug("Decoding done.\n");

    /*
     *  Since PNG gives the data to use in RGBA format and
     *  we'd really like to use ARGB, we'll have to convert
     *  it here.  We can't do it during scanning, because the
     *  interlacing scheme will go nuts.
     *
     *  Also, PNG specifies that an alpha value of zero denotes
     *  full transparency and a maximum value means an opaque pixel.
     *  Well, since we're using exactly the opposite, the conversion
     *  is made here, also.
     */

    if( frame->pix->colorspace == CS_ARGB ) {
        for( crow = 0; crow < height; crow++ ) {
            ULONG *cp;
            int i;

            cp = (ULONG *)GetPixelRow( frame, crow );

            // PDebug("\nRow: %d\n",crow);
            for(i = 0; i < frame->pix->width; i++) {
                ULONG v, *s;

                s = (ULONG *)cp;
                v = s[i];

                // PDebug("%08X,",v);
                // if( i % 8 == 7 ) PDebug("\n");

                v = (v >> 8) | ((0xFF-(v & 0xFF)) << 24);
                s[i] = v;
            }

            PutPixelRow( frame, crow, cp );
        }
    }

    FinishProgress( frame );

    // PDebug("Ending reading\n");

    png_read_end( png_ptr, end_info );

errorexit:

    // PDebug("Errorexit reached\n");

    /*
     *  Release allocated resources
     */

    png_destroy_read_struct( &png_ptr, &info_ptr, &end_info );

    return res;
}

/*
    This is not yet implemented, even though PNG is capable of saving
    colormapped files.
*/
PERROR SaveBM( BPTR handle, FRAME *frame, struct Values *v, struct TagItem *tags, struct PPTBase *PPTBase )
{
    UWORD height, crow;
    png_structp png_ptr;
    png_infop  info_ptr;
    PERROR res = PERR_OK;
    struct my_error myerr;
    RGBPixel palette[256];
    UBYTE transparencies[256];
    BOOL has_trans;
    int i;

    myerr.fh      = handle;
    myerr.frame   = frame;
    myerr.PPTBase = PPTBase;

    /*
     *  Allocate & Initialize save object
     */

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                      (void *) &myerr,
                                      user_error_fn,
                                      user_warning_fn);

    if (!png_ptr) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_FAILED;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_FAILED;
    }

    InitProgress( frame, "Saving "MYNAME" colormapped picture...", 0, frame->pix->height );

    /*
     *  Set the error handler and initialize the structures.
     */

    if( setjmp(png_ptr->jmpbuf) ) {
        res = PERR_FAILED;
        goto errorexit;
    }

    png_set_write_fn(png_ptr, &myerr, user_write_data, user_flush_data);

    /*
     *  Set up the file attributes.
     */

    height = info_ptr->height = frame->pix->height;
    info_ptr->width             = frame->pix->width;
    info_ptr->bit_depth         = 8; /*frame->pix->bits_per_component;*/
    info_ptr->interlace_type    = 0; /* No interlaced images so far. */
    info_ptr->channels          = 1;
    info_ptr->color_type        = PNG_COLOR_TYPE_PALETTE;
    info_ptr->valid             |= PNG_INFO_PLTE;
    info_ptr->palette           = palette;
    info_ptr->num_palette       = frame->disp->ncolors;

    png_set_compression_level( png_ptr, v->compression );

    /*
     *  PNG requires that the palette is split into a 3-component
     *  buffer and a 1-component transparency buffer
     */

    has_trans = FALSE;
    for(i = 0; i < frame->disp->ncolors; i++) {
        palette[i].r = frame->disp->colortable[i].r;
        palette[i].g = frame->disp->colortable[i].g;
        palette[i].b = frame->disp->colortable[i].b;
        if(transparencies[i] = frame->disp->colortable[i].a)
            has_trans = TRUE;  // BUG: Invert?
    }

    if( has_trans ) {
        info_ptr->trans     = transparencies;
        info_ptr->num_trans = frame->disp->ncolors;
    }

    png_write_info(png_ptr, info_ptr);

    png_set_packing(png_ptr); /* Pack into smaller */

    crow = 0;

    while( crow < height ) {
        int plane;
        UBYTE *buffer;

        // PDebug("\tWriting row %d\n",crow);

        if(Progress(frame, crow)) {
            SetErrorCode( frame, PERR_BREAK );
            res = PERR_BREAK;
            goto errorexit;
        }

        buffer = GetBitMapRow( frame, crow );

        png_write_rows( png_ptr, (png_bytepp) &buffer, 1 );

        crow++;

    } /* while */

    png_write_end( png_ptr, info_ptr );

    FinishProgress( frame );

errorexit:

    // PDebug("Errorexit reached\n");

    /*
     *  Release allocated resources
     */

    png_destroy_write_struct( &png_ptr, &info_ptr );

    return res;
}



PERROR
SaveTC( BPTR handle, FRAME *frame, struct Values *v, struct TagItem *tags, struct PPTBase *PPTBase )
{
    UWORD height, crow;
    png_structp png_ptr;
    png_infop  info_ptr;
    PERROR res = PERR_OK;
    struct my_error myerr;
    ULONG  *buffer = NULL;

    myerr.fh      = handle;
    myerr.frame   = frame;
    myerr.PPTBase = PPTBase;

    /*
     *  Allocate & Initialize save object
     */

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                      (void *) &myerr,
                                      user_error_fn,
                                      user_warning_fn);

    if (!png_ptr) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_FAILED;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        return PERR_FAILED;
    }

    InitProgress( frame, "Saving "MYNAME" truecolor picture...", 0, frame->pix->height );

    /*
     *  Set the error handler and initialize the structures.
     */

    if( setjmp(png_ptr->jmpbuf) ) {
        res = PERR_FAILED;
        goto errorexit;
    }

    png_set_write_fn(png_ptr, &myerr, user_write_data, user_flush_data);

    /*
     *  Set up the file attributes.
     */

    height = info_ptr->height = frame->pix->height;
    info_ptr->width  = frame->pix->width;
    info_ptr->bit_depth = 8; /*frame->pix->bits_per_component;*/
    info_ptr->interlace_type = 0; /* No interlaced images so far. */
    info_ptr->channels = frame->pix->components;

    png_set_compression_level( png_ptr, v->compression );

    switch(frame->pix->colorspace) {
        case CS_RGB:
            info_ptr->color_type = PNG_COLOR_TYPE_RGB;
            break;

        case CS_ARGB:
            info_ptr->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
            break;

        case CS_GRAYLEVEL:
            info_ptr->color_type = PNG_COLOR_TYPE_GRAY;
            break;
    }

    buffer = AllocVec( frame->pix->width*4, MEMF_ANY);
    if(!buffer) {
        SetErrorCode( frame, PERR_OUTOFMEMORY );
        res = PERR_FAILED;
        goto errorexit;
    }

    png_write_info(png_ptr, info_ptr);

    crow = 0;

    while( crow < height ) {
        ROWPTR cp;

        // PDebug("\tWriting row %d\n",crow);

        if(Progress(frame, crow)) {
            SetErrorCode( frame, PERR_BREAK );
            res = PERR_BREAK;
            goto errorexit;
        }

        cp = GetPixelRow( frame, crow );

        /*
         *  We need to do the ARGB->RGBA conversion now.
         *
         *  Also, we make the alpha inversion procedure.
         */

        if(frame->pix->colorspace == CS_ARGB ) {
            int i;

            for(i = 0; i < frame->pix->width; i++) {
                ULONG v, *s;

                s = (ULONG *)cp;
                v = s[i];

                // PDebug("%08X,",v);
                // if( i % 8 == 7 ) PDebug("\n");

                // Invert alpha.

                v = (v << 8) | (0xFF - ((v & 0xFF000000) >> 24));
                buffer[i] = v;
            }
            png_write_rows( png_ptr, (png_bytepp) &buffer, 1 );
        } else {
            png_write_rows( png_ptr, &cp, 1 );
        }

        crow++;

    } /* while */

    png_write_end( png_ptr, info_ptr );

    FinishProgress( frame );

errorexit:

    // PDebug("Errorexit reached\n");

    /*
     *  Release allocated resources
     */

    png_destroy_write_struct( &png_ptr, &info_ptr );

    if(buffer) FreeVec(buffer);

    return res;

}

PERROR DoGUI( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    struct TagItem compr[] = { AROBJ_Value, NULL,
                               ARSLIDER_Default, 6,
                               ARSLIDER_Min, Z_NO_COMPRESSION,
                               ARSLIDER_Max, Z_BEST_COMPRESSION,
                               AROBJ_Label, "Compression",
                               TAG_DONE };
    PERROR res;

    compr[0].ti_Data = (ULONG) &v->compression;
    compr[1].ti_Data = v->compression;

    res = AskReq( frame, AR_Text, ISEQ_C"\nSelect the PNG file attributes\n"
                                  ISEQ_I"(Compression 9 produces smallest files.)\n",
                         AR_SliderObject, compr,
                         TAG_DONE);

    return res;
}

IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    ULONG *args;
    struct Values v, *opt;
    PERROR res = PERR_OK;

    v.compression = Z_BEST_COMPRESSION;


    /*
     *  First, make interesting queries from the user.
     */

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    if( args = (ULONG *)TagData(PPTX_RexxArgs, tags) ) {
        if( args[0] )
            v.compression = *((LONG *)args[0]);

    } else {
        res = DoGUI( frame, &v, PPTBase );
    }

    if( res == PERR_OK ) {
        PutOptions( MYNAME, &v, sizeof(v) );

        if( format & CSF_LUT )
            res = SaveBM( fh, frame, &v, tags, PPTBase );
        else
            res = SaveTC( fh, frame, &v, tags, PPTBase );
    }

    return res;
}

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    BOOL is_png;

    // PDebug(MYNAME": Check()\n");

    /*
     *  Read bytes from file, then return TRUE if we can handle this
     *  type of file
     */

    is_png = png_check_sig(buf, len);

    return is_png;
}

IOGETARGS(format, frame, tags, PPTBase, IOModuleBase)
{
    ULONG *args;
    struct Values v, *opt;
    PERROR res = PERR_OK;

    v.compression = Z_BEST_COMPRESSION;

    if( opt = GetOptions(MYNAME) )
        v = *opt;

    if( args = (ULONG *)TagData(PPTX_RexxArgs, tags) ) {
        if( args[0] )
            v.compression = *((LONG *)args[0]);

    } else {
        res = DoGUI( frame, &v, PPTBase );
    }

    return res;
}

/*-----------------------------------------------------------------*/
/*                          END OF CODE                            */
/*-----------------------------------------------------------------*/
