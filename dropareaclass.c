/*
    PROJECT: ppt
    MODULE : AreaClass.c

    Originally by Jan van den Baard, 1995
    Modified by Janne Jalkanen, 1995

    $Id: dropareaclass.c,v 1.7 1997/08/17 00:26:20 jj Exp $
*/

#include "defs.h"
#include "misc.h"
#include "gui.h"

#include <clib/alib_protos.h>

#include <pragmas/intuition_pragmas.h>
#include <pragmas/bgui_pragmas.h>
#include <pragmas/utility_pragmas.h>

#include <stdlib.h>

#include "dropareaclass.h"


/*------------------------------------------------------------------------*/
/* Prototypes */

Prototype   Class *InitDropAreaClass( void );
Prototype   BOOL FreeDropAreaClass( Class * );

/*------------------------------------------------------------------------*/
/* Global variables. */

Class *DropAreaClass;

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
        APTR            DropEntry;
        WORD            DropX;
        WORD            DropY;
        ULONG           AllowDrop;
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
 */

Local
SAVEDS ASM ULONG DropAreaDispatch( REG(a0) Class *cl, REG(a2) Object *obj, REG(a1) Msg msg )
{
    AOD                     *data;
    struct TagItem          *tag, *tstate;
    ULONG                    rc = 0L;
    APTR                     entry;

    switch ( msg->MethodID ) {

        case    OM_NEW:

            /*
             *  Let the superclass create the object.
             */
            if ( rc = DoSuperMethodA( cl, obj, msg )) {

                /*
                 *      Get instance data.
                 */
                data = ( AOD * )INST_DATA( cl, rc );

                /*
                 *      Clear data (dunno if the system
                 *      does this ????)
                 */
                bzero(( char * )data, sizeof( AOD ));

                /*
                 *      Read attributes.
                 */
                tstate = OPSET( msg )->ops_AttrList;
                while ( tag = NextTagItem( &tstate )) {
                    switch ( tag->ti_Tag ) {
                        case    BT_DropObject:
                            data->AllowDrop = tag->ti_Data;
                            break;
                    }
                }

                return(rc);
            }

            break;

        case    OM_SET:
        case    OM_UPDATE:
            rc = DoSuperMethodA( cl, obj, msg );

            if( tag = FindTagItem(AREA_DropEntry, OPSET(msg)->ops_AttrList ) ) {
                data = ( AOD * )INST_DATA( cl, obj );
                data->DropEntry = (APTR)tag->ti_Data;
            }

            if( tag = FindTagItem(BT_DropObject, OPSET(msg)->ops_AttrList ) ) {
                data = ( AOD * )INST_DATA( cl, obj );
                data->AllowDrop = tag->ti_Data;
            }

            break;

        case    OM_GET:
            /*
             *      Do we know the requested
             *      attribute?
             */
            switch( OPGET( msg )->opg_AttrID ) {

                case AREA_DropEntry:
                    data = ( AOD * )INST_DATA( cl, obj );
                    *( (ULONG *) OPGET( msg )->opg_Storage ) = ( ULONG )data->DropEntry;
                    rc = 1L;
                    break;

                case BT_DropObject:
                    data = ( AOD * )INST_DATA( cl, obj );
                    *( (ULONG *) OPGET( msg )->opg_Storage ) = ( ULONG )data->AllowDrop;
                    rc = 1L;
                    break;

                default:
                    rc = DoSuperMethodA( cl, obj, msg );
                    break;
            }

            break;

        case    BASE_DRAGQUERY:
            /*
             *  If the request came from us, let the superclass
             *  worry about it.
             */

            if( QUERY(msg)->bmdp_Source == obj )
                return( DoSuperMethodA( cl, obj, msg ));

            data = (AOD *)INST_DATA( cl, obj );

            /*
             *  Allow drops from the main listview only and if we're droppable.
             */

            if( QUERY( msg )->bmdp_Source == framew.Frames && data->AllowDrop ) {
                rc = BQR_ACCEPT;
            } else {
                rc = BQR_REJECT;
            }
            break;

        case    BASE_DROPPED:
            data = (AOD *)INST_DATA( cl, obj );

            if( (entry = (APTR) FirstSelected( DROP(msg)->bmd_Source ))) {

                data->DropEntry = entry;

                /*
                 *  Notify main program
                 */

                Notify( obj, GPRENDER( msg )->gpr_GInfo, 0L,
                    GA_ID, GADGET( obj )->GadgetID,
                    TAG_END );
            }
            rc = 1;
            break;

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
       Initialize the class. This is only called during the main task,
       so we don't need to open any special libraries.
*/

Class *InitDropAreaClass( void )
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
            cl->cl_Dispatcher.h_Entry = ( HOOKFUNC )DropAreaDispatch;
        }
    }
    return( cl );
}

/*
       Kill the class. Also, we are running on the main task now, so there's
       no need to open any libraries.
*/

BOOL FreeDropAreaClass( Class *cl )
{
    return( FreeClass( cl ));
}
