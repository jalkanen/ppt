/*
 *  JPEG loader, main code
 *
 *  This file and PPT are (C) Janne Jalkanen 1998
 *
 *  $Id: main.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
 */


#include "pptplugin.h"
#include "fortify.h"
#include "jpeg.h"
#include <setjmp.h>

/*------------------------------------------------------------------*/

const char infoblurb[] =
    "Loads and saves JPEG/JFIF images\n"
    "This is based on the work of the\n"
    "Independent JPEG group. Thanks.\n"
    "libjpeg version 6b.";

/*------------------------------------------------------------------*/

#pragma msg 186 ignore
#pragma msg 92 ignore

const struct TagItem MyTagArray[] = {
    PPTX_Load,              TRUE,
    PPTX_Name,              MYNAME,
    PPTX_Author,            "Janne Jalkanen",
    PPTX_InfoTxt,           &infoblurb[0],
    PPTX_ColorSpaces,       CSF_RGB|CSF_GRAYLEVEL,
    PPTX_RexxTemplate,      "COMPRESSIONLEVEL/N,PROGRESSIVE/S,OPTIMIZE/S",
    PPTX_ReqPPTVersion,     3,

    PPTX_PreferredPostFix,  ".jpg",
    PPTX_PostFixPattern,    "#?.(jpg|jpeg|jfif)",
#ifdef _M68030
# ifdef _M68040
    PPTX_CPU,               AFF_68040|AFF_FPU40,
# else
    PPTX_CPU,               AFF_68030,
# endif
#endif

    PPTX_SupportsGetArgs,   TRUE,
    TAG_DONE
};

struct Values {
    LONG quality;
    LONG progressive;
    LONG optimize;
};

/*------------------------------------------------------------------*/

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

void my_error_exit( j_common_ptr cinfo )
{
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    D(bug("\tError exit\n"));
    (*cinfo->err->output_message)(cinfo);

    D(bug("\tLongjumping...\n"));
#ifdef _DCC
    mylongjmp( myerr->setjmp_buffer,1 );
#else
    longjmp( myerr->setjmp_buffer, 1 );
#endif
}

/*
    This routine will write the message given into the error
    buffer sent to us by PPT master
*/

void my_output_message( j_common_ptr cinfo )
{
    struct PPTBase *PPTBase;
    char buf[80];

    my_error_ptr myerr = (my_error_ptr)cinfo->err;

    PPTBase = myerr->PPTBase;

    D(bug("\tFormatting Message\n"));
    (*cinfo->err->format_message) (cinfo,buf);

    SetErrorMsg( myerr->frame, buf );
}

void my_emit_message( j_common_ptr cinfo, int msg_level )
{
  my_error_ptr myerr = (my_error_ptr)cinfo->err;

  if (msg_level < 0) {
      /*
       *  It's a warning. We ignore all trace messages.
       */

      if (cinfo->err->num_warnings == 0 || cinfo->err->trace_level >= 3)
           (*cinfo->err->format_message) (cinfo, myerr->err_buf);

      cinfo->err->num_warnings++;

  }

}

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTagArray );
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    volatile struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    UBYTE **buffer, *cp;
    ULONG row_stride;
    volatile struct PPTBase *PPTBase2 = PPTBase;
    PERROR res = PERR_OK;
    volatile BOOL loading = FALSE;

    Fortify_EnterScope();
    D(bug(MYNAME": Load()\n"));

    /*
     *  Set error handlers. We use our own setjmp() if we use DICE
     */

    Fortify_EnterScope();
    D(bug("\tSetting error handlers\n"));
    bzero( &jerr, sizeof(struct my_error_mgr) ); /* Make sure everything is NULL */
    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = my_error_exit;
    jerr.pub.output_message = my_output_message;
    jerr.pub.emit_message = my_emit_message;
    jerr.PPTBase = PPTBase;
    jerr.frame = frame;

#ifdef _DCC
    if( mysetjmp( jerr.setjmp_buffer ) != 0) {
#else
    if( setjmp( jerr.setjmp_buffer ) != 0) {
#endif
        /* Here we come after error_exit() */
        D(bug("\tError exit jump\n"));
        PPTBase = PPTBase2; /* Retrieve pointer */
        D(bug("\tDestroying cinfo\n"));
        jpeg_destroy_decompress(&cinfo);
        Fortify_OutputAllMemory();
        D(bug("\tReturning NULL\n"));
        return PERR_FAILED;
    }

    /*
     *  Initialize decompression object and our own structures
     */

    Fortify_EnterScope();
    D(bug("\tCreate decompress object\n"));
    jpeg_create_decompress(&cinfo);

    Fortify_EnterScope();
    D(bug("\tSet input filehandle\n"));
    jpeg_amiga_src(&cinfo, fh);

    Fortify_EnterScope();
    D(bug("\tRead JPEG headers\n"));
    jpeg_read_header(&cinfo, TRUE);

    D(bug("\tBuild new frame (%d x %d)\n",cinfo.image_width, cinfo.image_height));
    D(bug("Other info: cinfo = %08X, sizeof(cinfo) = %lu\n",&cinfo, sizeof(cinfo)));

//    Fortify_OutputAllMemory();

#ifdef _M68881
    if( PPTBase->lb_Sys->AttnFlags & AFF_68881 ) {
        D(bug("\tUsing floating point code\n"));
        cinfo.dct_method = JDCT_FLOAT;
    }
#endif

    /*
     *  Set frame information
     */

    InitProgress(frame,"Loading JPEG/JFIF image",0,cinfo.image_height);

    frame->pix->width = cinfo.image_width;
    frame->pix->height = cinfo.image_height;

    if(cinfo.jpeg_color_space == JCS_GRAYSCALE) {
        frame->pix->colorspace = CS_GRAYLEVEL;
        frame->pix->origdepth  = 8;
        frame->pix->components = 1;
    } else {
        frame->pix->colorspace = CS_RGB;
        frame->pix->origdepth  = 24;
        frame->pix->components = 3;
    }

    if( cinfo.density_unit == 2 ) { /* Dots/cm */
        frame->pix->DPIX = (cinfo.X_density * 254) / 100;
        frame->pix->DPIY = (cinfo.Y_density * 254) / 100;
    } else {
        frame->pix->DPIX = cinfo.X_density;
        frame->pix->DPIY = cinfo.Y_density;
    }

    /*
     *  Begin real decoding loop
     */

    if( (res = InitFrame(frame)) == PERR_OK) {
        loading = TRUE;
        jpeg_start_decompress(&cinfo);
        Fortify_EnterScope();
        row_stride = cinfo.output_width * cinfo.output_components;
        buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
        D(bug("\tBegin scan loop, allocated room for %lu pixels\n", row_stride));

        while(cinfo.output_scanline < cinfo.output_height) {
            if( Progress(frame,cinfo.output_scanline)) {
                jpeg_abort(&cinfo);
                res = PERR_BREAK;
                goto errorexit;
            }

            cp = (UBYTE *)GetPixelRow( frame, cinfo.output_scanline );
//            PDebug("\tGot Pixel Line #%u @ %08X\n",cinfo.output_scanline, cp );
            jpeg_read_scanlines(&cinfo, buffer, 1);

            memcpy( cp, (APTR)((ULONG *)buffer)[0], row_stride );

//            PDebug("\tCopying data from %08X to %08X\n",((ULONG *)buffer)[0], cp );
            PutPixelRow(frame,cinfo.output_scanline - 1,cp);

        }

        D(bug("\tFinished reading\n"));
        Fortify_LeaveScope();
        jpeg_finish_decompress(&cinfo);
    }

    /*
     *  End decoding loop and exit.
     */

    FinishProgress( frame );
    loading = FALSE;

errorexit:

    if(jerr.pub.num_warnings != 0) {
        SetErrorMsg( frame, jerr.err_buf );
        res = PERR_WARNING;
    }

    D(bug("\tDestroy decompress\n"));
    jpeg_destroy_decompress(&cinfo);
    Fortify_LeaveScope();

    return res;
}

IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    if( format & CSF_LUT )
        return PERR_UNKNOWNTYPE;

    return SaveTC( fh, frame, tags, PPTBase );
}

PERROR ParseRexxArgs( struct Values *v, ULONG *args, struct PPTBase *PPTBase )
{
    if( args[0] ) {
        v->quality = * ((int *)args[0]);
        if( v->quality < 1 || v->quality > 100 ) {
            return PERR_INVALIDARGS;
        }
        v->progressive = args[1];
        v->optimize    = args[2];
    }

    return PERR_OK;
}

PERROR DoSaveGUI( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    struct TagItem slider[] = { AROBJ_Value, NULL,
                                ARSLIDER_Default, 75,
                                ARSLIDER_Min, 0, ARSLIDER_Max, 100,
                                TAG_END };
    struct TagItem progr[] = { AROBJ_Value, NULL,
                               ARCHECKBOX_Selected, FALSE,
                               AROBJ_Label, (ULONG)"Use progressive coding?",
                               TAG_END };
    struct TagItem optimize[]= {AROBJ_Value, NULL,
                               ARCHECKBOX_Selected, FALSE,
                               AROBJ_Label, (ULONG)"Optimize?",
                               TAG_END };
    struct TagItem saveq[] = { AR_SliderObject, NULL,
                               AR_CheckBoxObject, NULL,
                               AR_CheckBoxObject, NULL,
                               AR_Text, "\nPlease select JPEG compression level:\n"
                                        ISEQ_I ISEQ_KEEP"    (less = smaller images but worse quality,\n"
                                        "    more = bigger images but better quality)\n"ISEQ_N,
                               AR_HelpNode, "loaders.guide/JPEG",
                               TAG_END };
    PERROR res;

    slider[0].ti_Data = (ULONG)&v->quality;
    progr[0].ti_Data = (ULONG)&v->progressive;
    optimize[0].ti_Data = (ULONG)&v->optimize;
    saveq[0].ti_Data = (ULONG)slider;
    saveq[1].ti_Data = (ULONG)progr;
    saveq[2].ti_Data = (ULONG)optimize;
    slider[1].ti_Data = v->quality;
    progr[1].ti_Data  = v->progressive;

    res = AskReqA( frame, saveq );

    return res;
}

/*
    Truecolor save.
*/

PERROR SaveTC( BPTR fh, FRAME *frame, struct TagItem *tags, struct PPTBase *PPTBase )
{
    volatile struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    int res = PERR_OK;
    ULONG *args;
    struct Values v = {75,FALSE,FALSE}, *opt;

    if( opt = GetOptions(MYNAME)) {
        v = *opt;
    }

    D(bug(MYNAME": SaveTC()\n"));
    D(bug("Frame = %08X, tags = %08X, struct PPTBase = %08X\n",frame,tags,PPTBase));

    /*
     *  Call PPT GUI maker, if there was not a rexx argument.
     */

    args = (ULONG *)TagData( PPTX_RexxArgs, tags );
    if( args ) {
        ParseRexxArgs( &v, args, PPTBase );
    } else {
        if( (res = DoSaveGUI( frame, &v, PPTBase ) != PERR_OK )) {
            return res;
        }
    }

    PutOptions(MYNAME, &v, sizeof(struct Values));

    /*
     *  Initialize JPEG handlers
     */

    Fortify_EnterScope();
    D(bug("\tSetting error handlers\n"));
    bzero( &jerr, sizeof(struct my_error_mgr) ); /* Make sure everything is NULL */
    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = my_error_exit;
    jerr.pub.output_message = my_output_message;
    jerr.pub.emit_message = my_emit_message;
    jerr.PPTBase = PPTBase;
    jerr.frame = frame;

#ifdef _DCC
    if( mysetjmp( jerr.setjmp_buffer ) != 0) {
#else
    if( setjmp( jerr.setjmp_buffer ) != 0) {
#endif
        /* Here we come after error_exit() */
        D(bug("\tError exit jump\n"));
        D(bug("\tDestroying cinfo\n"));
        jpeg_destroy_compress(&cinfo);
        Fortify_OutputAllMemory();
        D(bug("\tReturning NULL\n"));
        return PERR_FAILED;
    }

    /*
     *  Initialize compression object and our own structures
     */

    Fortify_EnterScope();
    D(bug("\tCreate compress object\n"));
    jpeg_create_compress(&cinfo);

    Fortify_EnterScope();
    D(bug("\tSet output filehandle\n"));
    jpeg_amiga_dst(&cinfo, fh);

    /*
     *  Important variables
     */

    cinfo.image_width = frame->pix->width;
    cinfo.image_height = frame->pix->height;
    cinfo.input_components = frame->pix->components;
    cinfo.data_precision = frame->pix->bits_per_component;

    cinfo.density_unit = 1; /* Dots per inch */
    if( frame->pix->DPIX && frame->pix->DPIY ) {
        cinfo.X_density = frame->pix->DPIX;
        cinfo.Y_density = frame->pix->DPIY;
    }

    if( frame->pix->colorspace == CS_RGB) {
        cinfo.in_color_space = JCS_RGB;
        InitProgress(frame,"Saving 24 bit JPEG/JFIF image",0,cinfo.image_height);
    } else {
        cinfo.in_color_space = JCS_GRAYSCALE;
        InitProgress(frame,"Saving 8 bit JPEG/JFIF image",0,cinfo.image_height);
    }

    jpeg_set_defaults( &cinfo );

    /*
     *  Begin compression
     */

    jpeg_set_quality( &cinfo, v.quality, TRUE ); /* Force baseline compatibility */
    D(bug("\tJPEG quality: %d\n",v.quality));

    if(v.progressive) {
        D(bug("\tSaving progressive JPEG\n"));
        jpeg_simple_progression( &cinfo );
    }

#ifdef _M68881
    /* Check for FPU in this machine */

    if( PPTBase->lb_Sys->AttnFlags & AFF_68881 ) {
        D(bug("\tUsing floating point code\n"));
        cinfo.dct_method = JDCT_FLOAT;
    }
#endif

    cinfo.optimize_coding = (boolean)v.optimize;

    jpeg_start_compress( &cinfo, TRUE );

    while(cinfo.next_scanline < cinfo.image_height) {
        ROWPTR cp;
        JSAMPROW row_ptr[1];
        cp = GetPixelRow( frame, cinfo.next_scanline );

        if( Progress(frame, cinfo.next_scanline ) ) {
            jpeg_abort_compress(&cinfo);
            res = PERR_BREAK;
            goto errorexit;
        }

        row_ptr[0] = cp;
        jpeg_write_scanlines(&cinfo,row_ptr,1);
    }

    jpeg_finish_compress( &cinfo );
    FinishProgress(frame);

errorexit:
    jpeg_destroy_compress( &cinfo );

    Fortify_LeaveScope();
    return res;
}

IOGETARGS(format,frame,tags,PPTBase,IOModuleBase)
{
    struct Values v = {75,FALSE,FALSE}, *opt;
    PERROR res;
    STRPTR buffer;
    ULONG *origargs;

    if( format & CSF_LUT )
        return PERR_UNKNOWNTYPE;

    if( opt = GetOptions(MYNAME)) {
        v = *opt;
    }

    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );
    origargs = (ULONG *)TagData( PPTX_RexxArgs, tags );

    if( origargs )
        res = ParseRexxArgs( &v, origargs, PPTBase );

    if( res == PERR_OK ) {
        if( (res = DoSaveGUI( frame, &v, PPTBase )) == PERR_OK ) {
            SPrintF( buffer, "COMPRESSIONLEVEL %d %s %s",
                            v.quality,
                            v.progressive ? "PROGRESSIVE" : "",
                            v.optimize ? "OPTIMIZE" : "" );
        }
    }

    return res;
}

/*
    BUG: Should check for a complete JFIF file header.
*/

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    if( buf[0] == 0xFF && buf[1] == MARKER_SOI && len >= 2 )
        return TRUE;

    return FALSE;
}


