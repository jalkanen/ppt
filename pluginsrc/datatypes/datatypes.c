/*
   PROJECT: ppt
   MODULE:  datatypes.iomod

    Loads datatype images.

    $Id: datatypes.c,v 1.1 2001/10/25 16:23:00 jalkanen Exp $
 */

#include <pptplugin.h>

#include <proto/datatypes.h>
#include <proto/graphics.h>

#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <string.h>

/*----------------------------------------------------------------------*/
/* Defines */

/*
   You should define this to your module name. Try to use something
   short, as this is the name that is visible in the PPT filter listing.
 */

#define MYNAME      "Datatypes"

/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
   Just a simple string describing this effect.
 */

const char infoblurb[] =
"This IO Module uses the OS3.0\n"
"datatypes system to load up almost\n"
"any imaginable image type.";

/*
   This is the global array describing your effect. For a more detailed
   description on how to interpret and use the tags, see docs/tags.doc.
 */

const struct TagItem MyTagArray[] =
{

    /*
     *  Tells the capabilities of this loader/saver unit.
     */

    PPTX_Load, TRUE,

    /*
     *  Here are some pretty standard definitions. All iomodules should have
     *  these defined.
     */

    PPTX_Name, (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author, (ULONG) "Janne Jalkanen, 1997-1998",
    PPTX_InfoTxt, (ULONG) infoblurb,

    PPTX_RexxTemplate, (ULONG) NULL,

    PPTX_ReqKickVersion, 39L,
    PPTX_ReqPPTVersion,  3,

    PPTX_PreferredPostFix, NULL,
    PPTX_PostFixPattern, NULL,

    PPTX_Priority, (LONG) -100,
#ifdef _M68020
    PPTX_CPU, AFF_68020,
#endif

    TAG_END, 0L
};


/*----------------------------------------------------------------------*/
/* Code */

#ifdef __SASC
/* Disable SAS/C control-c handling. */
void __regargs __chkabort(void)
{
}
void __regargs _CXBRK(void)
{
}

#endif

struct Library *DataTypesBase = NULL;

LIBINIT
{
    if (DataTypesBase = OpenLibrary("datatypes.library", 0L)) {
        return 0;
    }
    return 1;
}


LIBCLEANUP
{
    if (DataTypesBase)
        CloseLibrary(DataTypesBase);
}

IOINQUIRE(attr,PPTBase,IOModuleBase)
{
    return TagData(attr, MyTagArray);
}

/*
   This must always exist!
 */

IOCHECK(fh,len,buf,PPTBase,IOModuleBase)
{
    struct DataType *dt;
    BOOL res = FALSE;
    BPTR lock;
    struct DosLibrary *DOSBase = PPTBase->lb_DOS;

    D(bug("IOCheck()\n"));

    if (lock = DupLockFromFH(fh)) {

        if (dt = ObtainDataType(DTST_FILE, (APTR) lock, TAG_DONE)) {
            if (dt->dtn_Header->dth_GroupID == GID_PICTURE)
                res = TRUE;

            ReleaseDataType(dt);
        }
        UnLock(lock);
    }
    return res;
}

IOLOAD(fh,frame,tags,PPTBase,IOModuleBase)
{
    PERROR res = PERR_OK;
    Object *obj;
    ULONG modeid;
    struct BitMapHeader *bmh;
    struct BitMap *bitmap;
    char *filename, *datatypename,namebuf[80];
    UBYTE *buf;
    struct ColorRegister *cr;
    struct GfxBase *GfxBase = PPTBase->lb_Gfx;
    struct gpLayout gpl;

    filename = (char *) TagData(PPTX_FileName, tags);

    D(bug("Obtaining DTObject()\n"));
    if (obj = NewDTObject(filename,
                          DTA_SourceType, DTST_FILE,
                          DTA_GroupID, GID_PICTURE,
                          PDTA_Remap, FALSE,
                          TAG_DONE)) {

        if (GetDTAttrs(obj,
                       PDTA_ModeID, &modeid,
                       PDTA_BitMapHeader, &bmh,
                       TAG_DONE) == 2) {

            frame->pix->height = bmh->bmh_Height;
            frame->pix->width = bmh->bmh_Width;
            frame->pix->components = 3;
            frame->pix->colorspace = CS_RGB;
            frame->pix->origmodeid = modeid;

            /*
             *  Call up the rendering routines
             */

            gpl.MethodID = DTM_PROCLAYOUT;
            gpl.gpl_GInfo = NULL;
            gpl.gpl_Initial = TRUE;

            if (DoDTMethodA(obj, NULL, NULL, (Msg) & gpl)) {
                struct DataType *dt;

                D(bug("Got it. Now reading attrs\n"));

                GetDTAttrs(obj,
                           PDTA_BitMap, &bitmap,
                           PDTA_ColorRegisters, &cr,
                           DTA_DataType, &dt,
                           TAG_DONE);

                if( dt )
                    datatypename = dt->dtn_Header->dth_Name;
                else
                    datatypename = "";

                if (bitmap && cr) {
                    UWORD depth;
                    PLANEPTR planes[8];
                    int i;

                    depth = GetBitMapAttr( bitmap, BMA_DEPTH );
                    frame->pix->origdepth = depth;

                    for(i = 0; i < depth; i++)
                        planes[i] = bitmap->Planes[i];

                    if (buf = AllocVec(frame->pix->width, 0L)) {

                        if (InitFrame(frame) == PERR_OK) {
                            WORD row;

                            strcpy(namebuf,"Loading a ");
                            strcat(namebuf,datatypename);
                            strcat(namebuf," datatype image...");

                            InitProgress(frame, namebuf, 0, frame->pix->height);

                            for (row = 0; row < frame->pix->height; row++) {
                                RGBPixel *cp;
                                WORD col;

                                if (Progress(frame, row)) {
                                    res = PERR_BREAK;
                                    break;
                                }

                                cp = (RGBPixel *) GetPixelRow(frame, row);

                                PlanarToChunky(planes, buf,
                                               (ULONG)frame->pix->width,
                                               depth);

                                /*
                                 *  Update counters to reflect our change
                                 */

                                for(i = 0; i < depth; i++)
                                    planes[i] += bitmap->BytesPerRow;

                                for (col = 0; col < frame->pix->width; col++) {
                                    UBYTE c;

                                    c = buf[col];

                                    cp[col].r = cr[c].red;
                                    cp[col].g = cr[c].green;
                                    cp[col].b = cr[c].blue;
                                }

                                PutPixelRow(frame, row, cp);
                            }
                            FinishProgress(frame);
                        }
                        FreeVec(buf);
                    } else {
                        SetErrorCode(frame, PERR_OUTOFMEMORY);
                        res = PERR_ERROR;
                    }

                } else {
                    SetErrorMsg(frame, "Couldn't allocate bitmap");
                    res = PERR_ERROR;
                }

            } else {            /* If(DoDTMethodA())... */
                SetErrorMsg(frame, "Failed to render bitmap");
                res = PERR_ERROR;
            }
        } else {                /* If GetDTAttrsA() */
            SetErrorMsg(frame, "Unable to read bitmap information");
            res = PERR_ERROR;
        }

        DisposeDTObject(obj);
    }
    return res;
}

/*
   Format can be any of CSF_* - flags
 */

IOSAVE(fh,format,frame,tags,PPTBase,IOModuleBase)
{
    return PERR_MISSINGCODE;
}

/*----------------------------------------------------------------------*
 *                            END OF CODE                               *
 *----------------------------------------------------------------------*/
