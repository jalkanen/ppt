/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt filters
    MODULE : histogram equalization

    By Janne Jalkanen 1995-1996
*/
/*----------------------------------------------------------------------*/

#include "histeq.h"

#include <string.h>

const char *labels[] = { "Global", "Local", NULL };
const ULONG slider2int[] = { SLIDER_Level, STRINGA_LongVal, TAG_DONE };
const ULONG int2slider[] = { STRINGA_LongVal, SLIDER_Level, TAG_DONE };

PERROR GetValues( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    Object *Win, *GO_radius, *GO_radiusI, *GO_method;
    struct Window *win;
    struct IntuitionBase *IntuitionBase = PPTBase->lb_Intuition;
    struct Library *BGUIBase = PPTBase->lb_BGUI;
    char wtitle[80] = MYNAME": ";

    strcat(wtitle, frame->name);

    Win = WindowObject,
        WINDOW_Screen, PPTBase->g->maindisp->scr,
        WINDOW_Title,  wtitle,
        WINDOW_ScreenTitle, "Histogram equalizator",
        WINDOW_ScaleWidth, 30,
        WINDOW_LockHeight, TRUE,
        TAG_SKIP,          (v->winpos.Height == 0) ? 1 : 0,
        WINDOW_Bounds,     &v->winpos,
        WINDOW_MasterGroup,
            VGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                StartMember,
                    InfoObject,
                        INFO_TextFormat, "Set whether you want global or local\n"
                                         "equalization.  In the local case you\n"
                                         "should also specify the range.\n",
                        INFO_MinLines, 3,
                        ButtonFrame,
                        FRM_Flags, FRF_RECESSED,
                    EndObject,
                EndMember,
                StartMember,
                    HGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                        StartMember,
                            GO_method = Cycle( NULL, labels, v->method, GID_METHOD ), Weight(1),
                        EndMember,
                        StartMember,
                            GO_radius = HorizSlider( NULL, 1, MAX_RADIUS, v->radius, GID_RADIUS ),
                        EndMember,
                        StartMember,
                            GO_radiusI = Integer( NULL, v->radius, 3, GID_RADIUSI ), Weight(25),
                        EndMember,
                    EndObject,
                EndMember,
                StartMember,
                    HorizSeparator,
                EndMember,
                StartMember,
                    HGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                        StartMember,
                            Button( "OK", GID_OK ),
                        EndMember,
                        VarSpace( DEFAULT_WEIGHT ),
                        StartMember,
                            Button( "Cancel", GID_CANCEL ),
                        EndMember,
                    EndObject,
                EndMember,
            EndObject,
        EndObject;

    if( Win ) {

        if( v->method == 0 ) {
            SetGadgetAttrs( (struct Gadget *)GO_radius,  NULL, NULL, GA_Disabled, TRUE, TAG_DONE );
            SetGadgetAttrs( (struct Gadget *)GO_radiusI, NULL, NULL, GA_Disabled, TRUE, TAG_DONE );
        }

        AddMap( GO_radius, GO_radiusI, slider2int);
        AddMap( GO_radiusI, GO_radius, int2slider);

        AddCondit( GO_method, GO_radius,  CYC_Active, 0, GA_Disabled, TRUE, GA_Disabled, FALSE );
        AddCondit( GO_method, GO_radiusI, CYC_Active, 0, GA_Disabled, TRUE, GA_Disabled, FALSE );

        win = WindowOpen( Win );
        if( win ) {
            ULONG sigmask, sig;

            GetAttr( WINDOW_SigMask, Win, &sigmask );

            while(1) {
                sig = Wait( sigmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F );

                if( sig & SIGBREAKF_CTRL_F ) {
                    WindowToFront( win );
                    ActivateWindow( win );
                }

                if( sig & SIGBREAKF_CTRL_C ) {
                    SetErrorCode( frame, PERR_BREAK );
                    DisposeObject( Win );
                    return PERR_BREAK;
                }

                if( sig & sigmask ) {
                    ULONG res;

                    while( (res = HandleEvent(Win)) != WMHI_NOMORE ) {
                        switch( res ) {
                            case WMHI_CLOSEWINDOW:
                            case GID_CANCEL:
                                DisposeObject( Win );
                                return PERR_CANCELED;

                            case GID_OK:
                                GetAttr( SLIDER_Level, GO_radius, &v->radius );
                                GetAttr( CYC_Active,   GO_method, &v->method );
                                GetAttr( WINDOW_Bounds, Win, (ULONG *)&v->winpos );
                                DisposeObject( Win );
                                return PERR_OK;

                            default:
                                break;
                        }
                    }
                }
            } /* while */
        } else {
            SetErrorCode( frame, PERR_WINDOWOPEN );
            return PERR_WINDOWOPEN;
        }
    } else {
        SetErrorCode( frame, PERR_WINDOWOPEN );
        return PERR_WINDOWOPEN;
    }

}

