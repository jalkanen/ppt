#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>

#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>
#include <graphics/scale.h>
#include <graphics/gfxbase.h>
#include <gadgets/colorwheel.h>
#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/bgui.h>
#include <proto/utility.h>

#include <string.h>
#include <stdio.h>

#include "colorgadget.h"

#define GID_QUIT  1
#define GID_COLOR 2

struct Library *BGUIBase;

Class *ColorClass = NULL;

int main(void)
{
    Object *Win, *Color;
    BOOL quit = FALSE;
    ULONG sigmask;
    struct Window *win;
    ULONG rc;
    struct Screen *scr = NULL;

    BGUIBase = OpenLibrary("bgui.library",0L);

    ColorClass = InitColorClass();
    if(!ColorClass) {
        printf("Couldn't alloc class\n");
        goto errexit;
    }

    scr = LockPubScreen(NULL);

    printf(">Allocing object\n");

    Color = NewObject( ColorClass, NULL,
                       // GROUP_Style, GRSTYLE_VERTICAL,
                       NeXTFrame,
                       FrameTitle("Color"),
                       COLOR_ColorWheel, TRUE,
                       COLOR_Screen, scr,
                       TAG_DONE );
    if(!Color) {
        printf("Couldn't alloc object\n");
        goto errexit;
    }

    printf(">allocating window\n");

    Win = WindowObject,
        WINDOW_ScaleWidth, 25,
        WINDOW_ScaleHeight, 25,
        WINDOW_Title,"Testing colorgadget",
        WINDOW_MasterGroup,
            VGroupObject, NormalSpacing, NormalHOffset, NormalVOffset,
                StartMember,
                    Color,
                EndMember,
                StartMember,
                    HorizSeparator,
                EndMember,
                StartMember,
                    Button("Quit",GID_QUIT), FixMinHeight,
                EndMember,
            EndObject,
    EndObject;

    if(Win) {
        printf(">allocation OK\n");
        if( win = WindowOpen(Win)) {
            printf(">window opened OK\n");
            GetAttr( WINDOW_SigMask, Win, &sigmask );

            while(!quit) {
                ULONG sig;

                sig = Wait(sigmask);

                if( sig & sigmask ) {
                    while(( rc = HandleEvent(Win) ) != WMHI_NOMORE) {
                        switch(rc) {
                            case GID_QUIT:
                                quit = TRUE;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
        DisposeObject(Win);
    } else {
        printf(">allocation failed\n");
    }


errexit:
    if(ColorClass) FreeColorClass(ColorClass);
    if(BGUIBase) CloseLibrary(BGUIBase);

    if( scr ) UnlockPubScreen( NULL, scr );

    return 0;
}

