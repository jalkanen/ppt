/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <string.h>
#include <exec/memory.h>

// #define DEBUG_MODE 1

#define MYNAME      "HistEq"

#define MAX_RADIUS  64

#define GID_OK          1
#define GID_CANCEL      2
#define GID_METHOD      3
#define GID_RADIUS      4
#define GID_RADIUSI     5

struct Values {
    ULONG method;
    ULONG radius;
    struct IBox winpos;
};

extern PERROR GetValues( FRAME *frame, struct Values *v, struct PPTBase *PPTBase );

