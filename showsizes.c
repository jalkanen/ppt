/*
 *  Shows PPT structure sizes
 *
 *  (C) Janne Jalkanen 1998
 *
 *  $Id: showsizes.c,v 1.2 1998/09/20 00:39:36 jj Exp $
 */

#include "defs.h"
#include "ppt_real.h"

#include <stdio.h>

struct {
    STRPTR text;
    ULONG  size;
} list[] = {
    "GLOBALS", sizeof(GLOBALS),
    "FRAME",   sizeof(FRAME),
    "DISPLAY", sizeof(DISPLAY),
    "PIXINFO", sizeof(PIXINFO),
    "EXTERNAL", sizeof(EXTERNAL),
    "LOADER",  sizeof(LOADER),
    "EFFECT",  sizeof(EFFECT),
    "SCRIPT",  sizeof(SCRIPT),
    "VMHANDLE", sizeof(VMHANDLE),
    "INFOWIN", sizeof(INFOWIN),
    "PreviewFrame", sizeof(struct PreviewFrame),
    "PREFS",   sizeof(PREFS),
    "PPTBase", sizeof(struct PPTBase),
    "Extension", sizeof(struct Extension),
    "PPTMessage", sizeof(struct PPTMessage),
    "gPointMessage", sizeof(struct gPointMessage),
    "gRectMessage", sizeof(struct gRectMessage),
    "gFixRectMessage", sizeof(struct gFixRectMessage),
    "gCircleMessage", sizeof(struct gCircleMessage),
    "ARUpdateMsg",  sizeof(struct ARUpdateMsg),
    "ARRenderMsg",  sizeof(struct ARRenderMsg),
    "EffectWindow", sizeof(struct EffectWindow),
    "SaveWin", sizeof(struct SaveWin),
    "DispPrefsWindow", sizeof(struct DispPrefsWindow),
    "PaletteWindow", sizeof(struct PaletteWindow),
    "ExtInfoWin", sizeof(struct ExtInfoWin),
    "ToolWindow", sizeof(struct ToolWindow),
    "SelectWindow", sizeof(struct SelectWindow),
    "FramesWindow", sizeof(struct FramesWindow),
    "PrefsWindow", sizeof(struct PrefsWindow),
    "EDITWIN", sizeof(EDITWIN),
    "StemItem", sizeof(struct StemItem),
    "Stem", sizeof(struct Stem),
    "RexxHost", sizeof(struct RexxHost),
    "REXXARGS", sizeof(REXXARGS),
    "RexxCommand", sizeof(struct RexxCommand),
    "RenderObject", sizeof(struct RenderObject),
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
