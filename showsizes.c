/*
 *  Shows PPT structure sizes
 *
 *  (C) Janne Jalkanen 1998
 *
 *  $Id: showsizes.c,v 1.3 1999/05/30 18:16:51 jj Exp $
 */

#include "defs.h"
#include "ppt_real.h"

#include <stdio.h>

struct {
    STRPTR text;
    ULONG  size;
} list[] = {
    "ARRenderMsg",  sizeof(struct ARRenderMsg),
    "ARUpdateMsg",  sizeof(struct ARUpdateMsg),
    "DISPLAY", sizeof(DISPLAY),
    "DispPrefsWindow", sizeof(struct DispPrefsWindow),
    "EDITWIN", sizeof(EDITWIN),
    "EFFECT",  sizeof(EFFECT),
    "EffectWindow", sizeof(struct EffectWindow),
    "Extension", sizeof(struct Extension),
    "EXTERNAL", sizeof(EXTERNAL),
    "ExtInfoWin", sizeof(struct ExtInfoWin),
    "FRAME",   sizeof(FRAME),
    "FramesWindow", sizeof(struct FramesWindow),
    "GLOBALS", sizeof(GLOBALS),
    "INFOWIN", sizeof(INFOWIN),
    "LOADER",  sizeof(LOADER),
    "gPointMessage", sizeof(struct gPointMessage),
    "gRectMessage", sizeof(struct gRectMessage),
    "gFixRectMessage", sizeof(struct gFixRectMessage),
    "gCircleMessage", sizeof(struct gCircleMessage),
    "MouseLocationMsg",sizeof(struct MouseLocationMsg),
    "PaletteWindow", sizeof(struct PaletteWindow),
    "PIXINFO", sizeof(PIXINFO),
    "PPTBase", sizeof(struct PPTBase),
    "PPTMessage", sizeof(struct PPTMessage),
    "PREFS",   sizeof(PREFS),
    "PrefsWindow", sizeof(struct PrefsWindow),
    "PreviewFrame", sizeof(struct PreviewFrame),
    "RexxHost", sizeof(struct RexxHost),
    "REXXARGS", sizeof(REXXARGS),
    "RexxCommand", sizeof(struct RexxCommand),
    "RenderObject", sizeof(struct RenderObject),
    "SaveWin", sizeof(struct SaveWin),
    "SCRIPT",  sizeof(SCRIPT),
    "Selection",sizeof(struct Selection),
    "SelectWindow", sizeof(struct SelectWindow),
    "StemItem", sizeof(struct StemItem),
    "Stem", sizeof(struct Stem),
    "ToolWindow", sizeof(struct ToolWindow),
    "VMHANDLE", sizeof(VMHANDLE),
    0L
};


int main(void)
{
    int i;

//    printf("Sizes of structures used by PPT\n"
//           "===============================\n");

    for(i = 0; list[i].text; i++) {
        printf("%-20s = %lu\n",list[i].text, list[i].size);
    }

    return 0;
}
