/*
    PROJECT: PPT
    MODULE : infowin.c

    This module contains code for handling infowindows.
 */

#include <defs.h>
#include <misc.h>
#include <gui.h>

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#include <pragma/bgui_pragmas.h>

/*-------------------------------------------------------------------------*/

Prototype void          UpdateInfoWindow( INFOWIN *iw, EXTDATA *xd );
Prototype void          PutIWToSleep( INFOWIN * );
Prototype void          AwakenIW( INFOWIN * );
Prototype Object *      GimmeInfoWindow( EXTDATA *, INFOWIN * );
Prototype void          UpdateIWSelbox( FRAME *f );

/*-------------------------------------------------------------------------*/

/*
    Opens one info window. Please note
    that iw should be filled already, otherwise trouble might arise.
    Returns NULL in case of failure, otherwise pointer to the newly
    created object.
*/

/// GimmeInfoWindow()

Object *
GimmeInfoWindow( EXTDATA *xd, INFOWIN *iw )
{
    ULONG args[14];
    FRAME *f;
    APTR BGUIBase = xd->lb_BGUI, SysBase = xd->lb_Sys;

    f = (FRAME *)iw->myframe;
    args[0] = (ULONG) f->pix->width;
    args[1] = (ULONG) f->pix->height;
    args[2] = (ULONG) f->pix->origdepth;
    if(f->origtype)
        args[3] = (ULONG) f->origtype->info.nd.ln_Name;
    else
        args[3] = (ULONG) "Unknown";
    args[4] = (ULONG) PICSIZE( f->pix);

    args[7] = (ULONG) f->fullname;

    args[8] = args[9] = args[10] = args[11] = 0L;
    args[12] = (ULONG)f->selbox.MinX;
    args[13] = (ULONG)f->selbox.MinX;


    SHLOCKGLOB(); /* We don't want anyone else to muck about here */
    if( iw->WO_win == NULL ) { /* Do we exist yet? */
        iw->WO_win = WindowObject,
                WINDOW_Title, f->nd.ln_Name,
                WINDOW_ScreenTitle, std_ppt_blurb,
                WINDOW_SizeGadget, TRUE,
                WINDOW_MenuStrip, PPTMenus,
                WINDOW_ScaleWidth, 25,
                WINDOW_Font, globals->userprefs->mainfont,
                WINDOW_Screen, MAINSCR,
                WINDOW_SharedPort, MAINWIN->UserPort,
                WINDOW_HelpFile, "PROGDIR:docs/ppt.guide",
                WINDOW_HelpNode, "Infowindow",
                WINDOW_MasterGroup,
                    VGroupObject, HOffset(4), VOffset(4), Spacing(4),
                        StartMember,
                            iw->GO_File = InfoObject,
                                LAB_Label,"File:",
                                LAB_Place, PLACE_LEFT,
                                ButtonFrame, FRM_Flags, FRF_RECESSED,
                                INFO_TextFormat, ISEQ_B"%s",
                                INFO_Args, &args[7],
                            EndObject,
                        EndMember,
                        StartMember,
                            HGroupObject, Spacing(4),
                                StartMember,
                                    iw->GO_Info = InfoObject,
                                        INFO_TextFormat, ISEQ_C"%lux%lux%lu %s picture",
                                        INFO_Args, &args[0],
                                    EndObject,
                                EndMember,
                            EndObject,
                        EndMember,
                        StartMember,
                            HorizSeparator,
                        EndMember,
                        StartMember,
                            iw->GO_Selbox = InfoObject,
                                Label("Area:"), Place(PLACE_LEFT),
                                INFO_TextFormat,  "(%ld,%ld)-(%ld,%ld)",
                                INFO_Args, &args[8],
                            EndObject,
                        EndMember,
#if 0
                        StartMember,
                            HorizSeparator,
                        EndMember,
#endif
                        StartMember,
                            iw->GO_status = InfoObject,
                                LAB_Label,"Status:", LAB_Place, PLACE_LEFT,
                                ButtonFrame, FRM_Flags, FRF_RECESSED,
                                INFO_TextFormat, "«idle»",
                            EndObject,
                        EndMember,
                        StartMember,
                            iw->GO_progress = HorizProgress("Done:",MINPROGRESS,MAXPROGRESS,0),
                        EndMember,
                        StartMember,
                            HGroupObject, Spacing(4),
#if 0
                                StartMember,
                                    iw->GO_Load=GenericDButton("_Load",GID_IW_LOAD),
                                EndMember,
                                StartMember,
                                    iw->GO_Save=GenericDButton("_Save",GID_IW_SAVE),
                                EndMember,
#endif
                                StartMember,
                                    iw->GO_Filter=GenericButton("_Process",GID_IW_PROCESS),
                                EndMember,
                                StartMember,
                                    iw->GO_Break=GenericDButton("_Break",GID_IW_BREAK),
                                EndMember,
                                StartMember,
                                    iw->GO_Display=GenericButton("_Render",GID_IW_DISPLAY),
                                EndMember,
                                StartMember,
                                    iw->GO_Close=GenericButton("_Close",GID_IW_CLOSE),
                                EndMember,
                            EndObject, /* Vgroup */
                        EndMember,
                    EndObject, /* Master Vgroup */
                EndObject; /* WindowObject */
    } /* does object exist? */

    if( iw->WO_win ) {
        if( iw->win = WindowOpen( iw->WO_win )) {
            /* NULL CODE */
        } else {
            DisposeObject( iw->WO_win );
            iw->WO_win = NULL;
        }
    }

    UNLOCKGLOB();
    return( iw->WO_win );
}
///

void UpdateInfoWindow( INFOWIN *iw, EXTDATA *xd )
{
    ULONG args[6];
    FRAME *f;

    if(iw == NULL)
        return;

    if(!CheckPtr( iw, "UpdateInfoWindow: IW" ))
        return;

    f = (FRAME *)iw->myframe;
    args[0] = (ULONG) f->pix->width;
    args[1] = (ULONG) f->pix->height;
    args[2] = (ULONG) f->pix->origdepth;
    if(f->origtype)
        args[3] = (ULONG) f->origtype->info.nd.ln_Name;
    else
        args[3] = (ULONG) "Unknown";
    args[4] = (ULONG) PICSIZE( f->pix);

    args[5] = (ULONG) f->fullname;

    XSetGadgetAttrs(xd, (struct Gadget *)iw->GO_File, iw->win, NULL, INFO_Args, &args[5], TAG_END );
    XSetGadgetAttrs(xd, (struct Gadget *)iw->GO_Info, iw->win, NULL, INFO_Args, &args[0], TAG_END );
}

void UpdateIWSelbox( FRAME *f )
{
    ULONG args[6];
    INFOWIN *iw = f->mywin;

    if(iw) {
        args[0] = (ULONG)f->selbox.MinX;
        args[1] = (ULONG)f->selbox.MinY;
        args[2] = (ULONG)f->selbox.MaxX;
        args[3] = (ULONG)f->selbox.MaxY;
        args[4] = (ULONG)f->selbox.MinX;
        args[5] = (ULONG)f->selbox.MinX;

        SetGadgetAttrs( (struct Gadget *)iw->GO_Selbox, iw->win, NULL, INFO_Args, &args[0], TAG_END );
    }
}

/*
    This routine will disable most gadgets in an infowindow,
    so that the user would not accidentally press them. Safe to call even if iw == NULL
    BUG: Should account for a possible open display window and do something about it
*/

/// PutIWToSleep()

void PutIWToSleep( INFOWIN *iw )
{
    if(iw) {
        SetGadgetAttrs( (struct Gadget *)iw->GO_Display, iw->win, NULL, GA_Disabled, TRUE, TAG_END);
#if 0
        SetGadgetAttrs( (struct Gadget *)iw->GO_Load, iw->win, NULL, GA_Disabled, TRUE, TAG_END);
        SetGadgetAttrs( (struct Gadget *)iw->GO_Save, iw->win, NULL, GA_Disabled, TRUE, TAG_END);
#endif
        SetGadgetAttrs( (struct Gadget *)iw->GO_Filter, iw->win, NULL, GA_Disabled, TRUE, TAG_END);
        SetGadgetAttrs( (struct Gadget *)iw->GO_Break, iw->win, NULL, GA_Disabled, FALSE, TAG_END);
    }
}

///

/*
    The opposite of previous function.
*/

/// AwakenIW()

void AwakenIW( INFOWIN *iw )
{
    if(iw) {
        SetGadgetAttrs( (struct Gadget *)iw->GO_Display, iw->win, NULL, GA_Disabled, FALSE, TAG_END);
#if 0
        SetGadgetAttrs( (struct Gadget *)iw->GO_Load, iw->win, NULL, GA_Disabled, FALSE, TAG_END);
        SetGadgetAttrs( (struct Gadget *)iw->GO_Save, iw->win, NULL, GA_Disabled, FALSE, TAG_END);
#endif
        SetGadgetAttrs( (struct Gadget *)iw->GO_Filter, iw->win, NULL, GA_Disabled, FALSE, TAG_END);
        SetGadgetAttrs( (struct Gadget *)iw->GO_Break, iw->win, NULL, GA_Disabled, TRUE, TAG_END);
    }
}

///
