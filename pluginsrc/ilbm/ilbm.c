/*----------------------------------------------------------------------*/
/*
    PROJECT:
    MODULE : ilbm.c

    ILBM loader module.

    $Id: ilbm.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Includes */

#include "ilbm.h"
#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */


/*----------------------------------------------------------------------*/
/* Internal prototypes */

/*----------------------------------------------------------------------*/
/* Global variables */

#pragma msg 186 ignore

const char infoblurb[] =
    "Loads and saves IFF-ILBM files.\n"
    "(Bitmapped, EHB, HAM6/8 and 24bit images supported)\n";

const char author[] =
    "Janne Jalkanen, 1995-1998";

const struct TagItem MyTags[] = {
    PPTX_Name,              MYNAME,
    PPTX_InfoTxt,           infoblurb,
    PPTX_Author,            author,
    /* functions */
    PPTX_Load,              TRUE ,
    PPTX_ColorSpaces,       CSF_RGB|CSF_LUT,
    PPTX_RexxTemplate,      "",
    PPTX_ReqPPTVersion,     4,

    PPTX_PreferredPostFix,  (ULONG)".iff",
    PPTX_PostFixPattern,    (ULONG)"#?.(iff|ilbm|lbm)",

    PPTX_SupportsGetArgs,   TRUE,

    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void) {}
void __regargs _CXBRK(void) {}
#endif

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData( attr, MyTags );
}

IOCHECK(handle,len,buf,PPTBase,IOModuleBase)
{
    struct IFFHandle *iff;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    struct Library *IFFParseBase = NULL;
    struct ExecBase *SysBase = PPTBase->lb_Sys;
    BOOL res = FALSE;

    D(bug("ILBM: Check()\n"));
    if( (IFFParseBase = OpenLibrary("iffparse.library",36L)) == NULL)
        return FALSE; /* Big time error :( */

    if( !(iff = AllocIFF()) )
        return FALSE;

    iff->iff_Stream = handle;
    InitIFFasDOS( iff );

//    PDebug("\tIFF handle allocated&initted\n");

    if( OpenIFF(iff, IFFF_READ ) == 0 ) {
        StopChunk( iff, ID_ILBM, ID_BMHD ); /* Search for BMHD chunk */
        if( ParseIFF( iff, IFFPARSE_SCAN ) == 0 ) {
//            PutStr("\tParsed OK\n");
            res = TRUE;
        }
    }

    CloseIFF( iff );
    FreeIFF( iff );
    CloseLibrary(IFFParseBase);
    return res;
}

/*
    This should be blatantly obvious.
*/

void PutErrMsg( FRAME *frame, ULONG code, struct PPTBase *PPTBase )
{
    char *msg;

    switch(code) {
        case SETUP_FAILED:
            msg = "Could not allocate Frame";
            break;
        case IFFPARSE_DID_NOT_OPEN:
            msg = "Couldn't open iffparse.library V36";
            break;
        case FORMAT_NOT_SUPPORTED:
            msg = "Unknown format encountered";
            break;
        case IFFERR_NOTIFF:
            msg = "This is not an IFF file";
            break;
        case IFFERR_MANGLED:
            msg = "Mangled IFF file";
            break;
        case IFFERR_SYNTAX:
            msg = "Not a correct IFF file";
            break;
        case NOT_ILBM:
            msg = "This is not an ILBM file";
            break;
        case PARSEIFF_FAILED:
            msg = "ParseIFF failed";
            break;
        case CANNOT_READ:
            msg = "Can't read IFF file";
            break;
        case MASKING_NOT_SUPPORTED:
            msg = "Masking is not supported";
            break;
        case IFFERR_NOMEM:
        case PERR_OUTOFMEMORY:
            msg = "Out of memory";
            break;
        case UNKNOWN_COMPRESSION:
            msg = "Unknown compression in ILBM";
            break;
        default:
            msg = NULL;
            break;
    }
    if(msg)
        SetErrorMsg(frame, msg);

}

/*
    Main loader.
*/

IOLOAD(handle,f,tags,PPTBase,IOModuleBase)
{
    struct IFFHandle *iff;
    struct StoredProperty *sp;
    struct BitMapHeader *bmhd;
    UBYTE *ctable = NULL;
    PERROR error = PERR_OK;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;
    struct Library  *IFFParseBase = NULL, *UtilityBase = PPTBase->lb_Utility;
    struct ExecBase *SysBase = PPTBase->lb_Sys;

    D(bug(MYNAME": Load()\n"));

    if( (IFFParseBase = OpenLibrary("iffparse.library",36L)) == NULL) {
        PutErrMsg(f,IFFPARSE_DID_NOT_OPEN,PPTBase);
        return PERR_FAILED; /* Big time error :( */
    }


    if( iff = AllocIFF() ) {
        iff->iff_Stream = handle;
        InitIFFasDOS( iff );

        D(bug("\tIFF handle allocated&initted\n"));

        if( OpenIFF(iff, IFFF_READ ) == 0 ) {
            PropChunk( iff, ID_ILBM, ID_BMHD ); /* Search for BMHD chunk */
            PropChunk( iff, ID_ILBM, ID_CMAP );
            PropChunk( iff, ID_ILBM, ID_CAMG );
            PropChunk( iff, ID_ILBM, ID_ANNO );
            PropChunk( iff, ID_ILBM, ID_AUTH );
            StopChunk( iff, ID_ILBM, ID_BODY );

            if( (error = ParseIFF( iff, IFFPARSE_SCAN )) == 0 ) {
                D(bug("\tParsed OK\n"));
                if( sp = FindProp(iff, ID_ILBM, ID_BMHD) ) {
                    bmhd = sp->sp_Data;

                    SetUp( f, PPTBase, bmhd );

                    if( InitFrame( f ) == PERR_OK) {
                        if( sp = FindProp(iff, ID_ILBM, ID_CAMG ))
                            SetUpDisplayMode( sp, f );
                        else
                            f->pix->origmodeid = 0L;

                        if( sp = FindProp(iff, ID_ILBM, ID_CMAP ))
                            ctable = SetUpColors( PPTBase, sp, f->disp->dispid );

                        if( (error = DecodeBody( iff, f, ctable, bmhd, PPTBase, IFFParseBase )) != PERR_OK ) {
                            PutErrMsg( f, error,PPTBase );
                            /*
                             *  Check out non-fatal errors
                             */
                            if( error == IFFERR_MANGLED )
                                error = PERR_WARNING;
                        }

                        if( sp = FindProp(iff, ID_ILBM, ID_AUTH) ) {
                            AddExtension(f,EXTNAME_AUTHOR, sp->sp_Data, sp->sp_Size, EXTF_CSTRING );
                        }

                        if( sp = FindProp(iff, ID_ILBM, ID_ANNO) ) {
                            AddExtension(f,EXTNAME_ANNO, sp->sp_Data, sp->sp_Size, EXTF_CSTRING );
                        }

                        if(ctable)
                            FreeVec( ctable );

                    } else { /* BeginLoad failed. */
                        PutErrMsg( f, SETUP_FAILED, PPTBase );
                        error = PERR_FAILED;
                    }
                } /* BMHD */
            } /* ParseIFF() */
            else {
                PutErrMsg( f, error, PPTBase );
            }

            CloseIFF(iff);

        } else { /* OpenIFF() */
            PutErrMsg( f, IFFERR_NOTIFF, PPTBase );
            error = PERR_FAILED;
        }
        FreeIFF( iff );
    }

    CloseLibrary(IFFParseBase);
    return error;
}

/*
    Save bitmapped images.
*/

PERROR SaveBM(BPTR handle, FRAME *frame, struct TagItem *tags, struct PPTBase *PPTBase )
{
    PERROR error = PERR_OK;
    APTR IFFParseBase, SysBase = PPTBase->lb_Sys;
    struct Extension *ext;
    UBYTE depth = frame->disp->depth;
    struct IFFHandle *iff = NULL;
    int i;

    InitProgress( frame, "Saving ILBM...",0,frame->pix->height );

    if( (IFFParseBase = OpenLibrary("iffparse.library",36L)) == NULL) {
        PutErrMsg(frame,IFFPARSE_DID_NOT_OPEN, PPTBase);
        return PERR_FAILED; /* Big time error :( */
    }

    if( iff = AllocIFF() ) {
        iff->iff_Stream = handle;
        InitIFFasDOS( iff );
        D(bug("Allocated\n"));

        /* Begin writing */
        OpenIFF(iff, IFFF_WRITE);

        /* FORM id */
        if(!(error = PushChunk(iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN ))) {

            /* BMHD */

            if(!(error = PushChunk(iff, ID_ILBM, ID_BMHD, sizeof( struct BitMapHeader)))) {
                struct BitMapHeader bmhd;
                InitBitMapHeader( &bmhd, frame, depth, PPTBase );
                WriteChunkBytes( iff, &bmhd, sizeof(bmhd)  );

                if(!(error = PopChunk(iff))) {

                    /*
                     *  Optional hunks
                     */

                    if( ext = FindExtension(frame,EXTNAME_AUTHOR) ) {
                        if( !PushChunk(iff,ID_ILBM,ID_AUTH, ext->en_Length ) ) {
                            WriteChunkBytes( iff, ext->en_Data, ext->en_Length );
                            PopChunk(iff);
                        }
                    }

                    if( ext = FindExtension(frame,EXTNAME_ANNO) ) {
                        if( !PushChunk(iff,ID_ILBM,ID_ANNO, ext->en_Length ) ) {
                            WriteChunkBytes( iff, ext->en_Data, ext->en_Length );
                            PopChunk(iff);
                        }
                    }

                    /* CAMG */

                    if(!(error = PushChunk(iff, ID_ILBM, ID_CAMG, 4 ))) {
                        WriteChunkBytes( iff, &(frame->disp->dispid), 4 );
                        if(!(error = PopChunk(iff))) {

                            /* CMAP */

                            if(!(error = PushChunk(iff, ID_ILBM, ID_CMAP, IFFSIZE_UNKNOWN ))) {
                                COLORMAP *cmap = frame->disp->colortable;

                                for( i = 0; i < (1<<depth); i++ ) {
                                    WriteChunkBytes( iff, &(cmap[i].r), 3 ); /* ARGB */
                                }

                                if(!(error = PopChunk(iff))) {

                                    /* BODY */
                                    if(!(error = PushChunk(iff, ID_ILBM, ID_BODY, IFFSIZE_UNKNOWN ))) {
                                        error = WriteBMBody( IFFParseBase, iff, frame, PPTBase );
                                        if(!error) {
                                            error = PopChunk(iff);
                                            FinishProgress( frame );
                                        }
                                    } /* BODY */
                                }
                            } /* CMAP */
                        }
                    } /* CAMG */
                }
            } /* BMHD */
        }
        if(!error) error = PopChunk(iff);

        CloseIFF(iff);
        FreeIFF(iff);
    } else {
        PutErrMsg(frame, NO_IFF_HANDLE, PPTBase);
        error = PERR_FAILED;
    }

    if(error) {
        PutErrMsg(frame,error,PPTBase);
        error = PERR_FAILED;
    }

    CloseLibrary(IFFParseBase);
    return error;
}

/*
    Save truecolor images (24 bit)
*/

PERROR SaveTC(BPTR handle, FRAME *frame, struct TagItem *tags, struct PPTBase *PPTBase )
{
    struct IFFHandle *iff;
    APTR IFFParseBase, SysBase = PPTBase->lb_Sys, DOSBase = PPTBase->lb_DOS;
    PERROR error = PERR_OK;

    D(bug(MYNAME": SaveTC()\n"));

    InitProgress(frame, "Saving ILBM24...", 0, frame->pix->height );

    if( (IFFParseBase = OpenLibrary("iffparse.library",36L)) == NULL) {
        PutErrMsg(frame,IFFPARSE_DID_NOT_OPEN,PPTBase);
        return PERR_FAILED; /* Big time error :( */
    }

    if( iff = AllocIFF() ) {
        iff->iff_Stream = handle;
        InitIFFasDOS( iff );
        D(bug("Allocated\n"));

        /* Begin writing */
        OpenIFF(iff, IFFF_WRITE);

        /* FORM id */
        if(!(error = PushChunk(iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN ))) {

            /* BMHD */

            if(!(error = PushChunk(iff, ID_ILBM, ID_BMHD, sizeof( struct BitMapHeader)))) {
                struct BitMapHeader bmhd;
                InitBitMapHeader( &bmhd, frame, 24, PPTBase );
                WriteChunkBytes( iff, &bmhd, sizeof(bmhd)  );

                if(!(error = PopChunk(iff))) {
                    struct Extension *ext;

                    /*
                     *  Optional hunks
                     */

                    if( ext = FindExtension(frame,EXTNAME_AUTHOR) ) {
                        if( !PushChunk(iff,ID_ILBM,ID_AUTH, ext->en_Length ) ) {
                            WriteChunkBytes( iff, ext->en_Data, ext->en_Length );
                            PopChunk(iff);
                        }
                    }

                    if( ext = FindExtension(frame,EXTNAME_ANNO) ) {
                        if( !PushChunk(iff,ID_ILBM,ID_ANNO, ext->en_Length ) ) {
                            WriteChunkBytes( iff, ext->en_Data, ext->en_Length );
                            PopChunk(iff);
                        }
                    }

                    /* CAMG */
                    if(!(error = PushChunk(iff, ID_ILBM, ID_CAMG, 4 ))) {
                        WriteChunkBytes( iff, &(frame->disp->dispid), 4 );
                        if(!(error = PopChunk(iff))) {

                            /* 24BIT does not need CMAP */

                            /* BODY */
                            if(!(error = PushChunk(iff, ID_ILBM, ID_BODY, IFFSIZE_UNKNOWN ))) {
                                WriteBody( IFFParseBase, iff, frame, PPTBase );
                                error = PopChunk(iff);
                                FinishProgress( frame );
                            } /* BODY */
                        }
                    } /* CAMG */
                }
            }
        }
        if(!error) error = PopChunk(iff);

        CloseIFF(iff);
        FreeIFF(iff);
    } else {
        PutErrMsg(frame, NO_IFF_HANDLE,PPTBase);
        error = PERR_FAILED;
    }

    if(error) {
        PutErrMsg(frame,error,PPTBase);
        error = PERR_FAILED;
    }

    CloseLibrary(IFFParseBase);
    return error;
}

IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    if( format == CSF_LUT )
        return SaveBM( fh, frame, tags, PPTBase );

    return SaveTC( fh, frame, tags, PPTBase );
}

IOGETARGS(format,frame,tags,PPTBase,IOModuleBase)
{
    STRPTR buffer;

    buffer = (STRPTR)TagData( PPTX_ArgBuffer, tags );

    strcpy( buffer, "" );

    return PERR_OK;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

