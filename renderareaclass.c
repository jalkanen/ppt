/*
    PROJECT: ppt
    MODULE : RenderAreaClass.c

    $Id: renderareaclass.c,v 1.1 1997/08/31 22:26:44 jj Exp $
*/

#include "defs.h"
#include "misc.h"
#include "gui.h"

#include <clib/alib_protos.h>

#include <pragmas/intuition_pragmas.h>
#include <pragmas/bgui_pragmas.h>
#include <pragmas/utility_pragmas.h>

#include <stdlib.h>

#include "renderareaclass.h"

/*------------------------------------------------------------------------*/
/* Global variables. */

Class *RenderAreaClass = NULL;

//
//      Compiler stuff.
//
#ifdef _DCC
#define SAVEDS __geta4
#define ASM
#define REG(x) __ ## x
#else
#define SAVEDS __saveds
#define ASM __asm
#define REG(x) register __ ## x
#endif

/*
 *      AreaClass object instance data.
 */

typedef struct {
    EXTBASE *ExtBase;
    FRAME   *frame;
} AOD;


/*
 *      Some cast macros.
 */

#define OPSET(x)        ((struct opSet *)x)
#define OPGET(x)        ((struct opGet *)x)
#define GPRENDER(x)     ((struct gpRender *)x)
#define GADGET(x)       ((struct Gadget *)x)
#define GDIM(x)         ((struct grmDimensions *)x)

#define QUERY(x)        ((struct bmDragPoint *)x)
#define DROP(x)         ((struct bmDropped *)x)

/*
 *      Send a notification.
 */
Local
ULONG Notify( Object *obj, struct GadgetInfo *gi, ULONG flags, Tag tag1, ... )
{
    return( DoMethod( obj, OM_NOTIFY, ( struct TagItem * )&tag1, gi, flags ));
}

/*
 *      Class dispatcher. We _may_ be running in a different task,
 *      so we'd better open up any libraries we use.
 *
 *      DON'T FORGET TO SET THE LIBRARY BASE IF YOU USE IT!
 */

Local
SAVEDS ASM ULONG RenderAreaDispatch( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) Msg msg )
{
    AOD                     *data;
    struct TagItem          *tag, *tstate;
    ULONG                    rc = 0L;
    struct IBox             *ibox;
    struct Library          *UtilityBase;

    switch ( msg->MethodID ) {

        case    OM_NEW:

            /*
             *  Let the superclass create the object.
             */
            if ( rc = DoSuperMethodA( cl, obj, msg )) {
                data = ( AOD * )INST_DATA( cl, rc );

                UtilityBase = OpenLibrary("utility.library",0L);

                tstate = OPSET( msg )->ops_AttrList;

                data->frame = NULL;
                data->ExtBase = (EXTBASE *)GetTagData( RAC_ExtBase, NULL, tstate);

                /*
                 *  If no extbase has been defined, we'll die.
                 */

                if(!data->ExtBase) {
                     CoerceMethod( cl, ( Object * )rc, OM_DISPOSE );
                     CloseLibrary(UtilityBase);
                     return NULL;
                }

                /*
                 *      Read attributes.
                 */

                while ( tag = NextTagItem( &tstate )) {
                    switch ( tag->ti_Tag ) {
                        case    RAC_Frame:
                            data->frame = ( FRAME * )tag->ti_Data;
                            break;
                    }
                }

                CloseLibrary(UtilityBase);

                return(rc);
            }

            break;

        case    OM_SET:
        case    OM_UPDATE:
            rc = DoSuperMethodA( cl, obj, msg );
            break;

        case    OM_GET:
            /*
             *      Do we know the requested
             *      attribute?
             */
            switch( OPGET( msg )->opg_AttrID ) {

                case RAC_Frame:
                    data = ( AOD * )INST_DATA( cl, obj );
                    *( (ULONG *) OPGET( msg )->opg_Storage ) = ( ULONG )data->frame;
                    rc = 1L;
                    break;

                default:
                    rc = DoSuperMethodA( cl, obj, msg );
                    break;
            }

            break;

#if 0
        case    GM_RENDER:
            data = ( AOD * )INST_DATA( cl, obj );
            DoSuperMethod( cl, obj, OM_GET, BT_InnerBox, &ibox );
            if( data->frame ) {
                D(bug("Rendering in Rastport @%08X, (%d,%d)->(%d,%d)\n",
                       GPRENDER(msg)->gpr_RPort, ibox->Top, ibox->Left,
                       ibox->Height, ibox->Width ));
                QuickRender( data->frame, GPRENDER(msg)->gpr_RPort,
                             ibox->Top, ibox->Left,
                             ibox->Height, ibox->Width, data->ExtBase );
            }
            break;
#endif
        default:
            /*
             *    All the rest goes to the
             *    superclass.
             */
            rc = DoSuperMethodA( cl, obj, msg );
            break;
    }
    return( rc );
}


/*
    Initialize the class.

    This may be called just once, at the start-up.
*/

Prototype Class *InitRenderAreaClass(VOID);

Class *InitRenderAreaClass(VOID)
{
    Class *super, *cl = NULL;

    /*
     *      Obtain the AreaClass pointer which
     *      will be our superclass.
     */
    if ( super = BGUI_GetClassPtr( BGUI_AREA_GADGET )) {

        /*
         *      Create the class.
         */
        if ( cl = MakeClass( NULL, NULL, super, sizeof( AOD ), 0L )) {
            /*
             *      Setup dispatcher.
             */
            cl->cl_Dispatcher.h_Entry = ( HOOKFUNC )RenderAreaDispatch;
        }
    }
    return( cl );
}

/*
       Kill the class. Also, we are running on the main task now, so there's
       no need to open any libraries.
*/

Prototype BOOL FreeRenderAreaClass( Class *cl );

BOOL FreeRenderAreaClass( Class *cl )
{
    return( FreeClass( cl ));
}
