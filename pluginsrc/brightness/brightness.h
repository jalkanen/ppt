/*
    balance.h - definitions for balance filter
*/

#ifdef DEBUG_MODE
#define D(x)    x
#define bug     PDebug
#else
#define D(x)
#endif

struct Values {
    LONG brightness;
};


#ifdef MY_OWN_WINDOW
#include <ppt_real.h> /* BUG */

#include <exec/types.h>
#include <intuition/intuition.h>

struct bwin {
    struct Window *win;
    ULONG sigmask;
    Object *Win, *Ok, *Cancel;
};

extern int GetWindow( struct bwin *, EXTBASE * );
extern int HandleIDCMP( ULONG rc, struct bwin *, EXTBASE * );

#define GID_OK              1
#define GID_CANCEL          2
#endif

