/*
    Convolute.h
*/

#include <pptplugin.h>
#include <intuition/intuition.h>

#define MYNAME      "Convolute"


struct convargs {
    int weights[7][7];
    int size; /* 3,5 or 7 */
    int bias;
    int div;
    char name[40];
    struct IBox winpos;
};

struct Preset {
    ULONG id;
    const char *name;
    int size,bias,div;
    int weights[7][7];
};

extern int LoadConvFilter( struct PPTBase *, char *, struct convargs * );
extern void SaveConvFilter( struct PPTBase *, char *, struct convargs * );

extern int DoConvolute( FRAME *src, FRAME *dest, struct convargs *cargs, struct PPTBase *PPTBase );


extern const struct Preset presets[];

#define GIDBASE     (1000)

#define GID_W00     (GIDBASE + 0)
#define GID_W01     (GIDBASE + 1)
#define GID_W02     (GIDBASE + 2)
#define GID_W03     (GIDBASE + 3)
#define GID_W04     (GIDBASE + 4)
#define GID_W05     (GIDBASE + 5)
#define GID_W06     (GIDBASE + 6)

#define GID_W10     (GIDBASE + 10)
#define GID_W11     (GIDBASE + 11)
#define GID_W12     (GIDBASE + 12)
#define GID_W13     (GIDBASE + 13)
#define GID_W14     (GIDBASE + 14)
#define GID_W15     (GIDBASE + 15)
#define GID_W16     (GIDBASE + 16)

#define GID_W20     (GIDBASE + 20)
#define GID_W21     (GIDBASE + 21)
#define GID_W22     (GIDBASE + 22)
#define GID_W23     (GIDBASE + 23)
#define GID_W24     (GIDBASE + 24)
#define GID_W25     (GIDBASE + 25)
#define GID_W26     (GIDBASE + 26)

#define GID_W30     (GIDBASE + 30)
#define GID_W31     (GIDBASE + 31)
#define GID_W32     (GIDBASE + 32)
#define GID_W33     (GIDBASE + 33)
#define GID_W34     (GIDBASE + 34)
#define GID_W35     (GIDBASE + 35)
#define GID_W36     (GIDBASE + 36)

#define GID_W40     (GIDBASE + 40)
#define GID_W41     (GIDBASE + 41)
#define GID_W42     (GIDBASE + 42)
#define GID_W43     (GIDBASE + 43)
#define GID_W44     (GIDBASE + 44)
#define GID_W45     (GIDBASE + 45)
#define GID_W46     (GIDBASE + 46)

#define GID_W50     (GIDBASE + 50)
#define GID_W51     (GIDBASE + 51)
#define GID_W52     (GIDBASE + 52)
#define GID_W53     (GIDBASE + 53)
#define GID_W54     (GIDBASE + 54)
#define GID_W55     (GIDBASE + 55)
#define GID_W56     (GIDBASE + 56)

#define GID_W60     (GIDBASE + 60)
#define GID_W61     (GIDBASE + 61)
#define GID_W62     (GIDBASE + 62)
#define GID_W63     (GIDBASE + 63)
#define GID_W64     (GIDBASE + 64)
#define GID_W65     (GIDBASE + 65)
#define GID_W66     (GIDBASE + 66)

#define GID_OK      (GIDBASE + 100)
#define GID_CANCEL  (GIDBASE + 101)
#define GID_LOAD    (GIDBASE + 102)
#define GID_SAVE    (GIDBASE + 103)
#define GID_CLEAR   (GIDBASE + 104)

#define GID_NAME    (GIDBASE + 110)
#define GID_BIAS    (GIDBASE + 111)
#define GID_DIV     (GIDBASE + 112)

/* Preset operators */
#define OPR(N)      (GIDBASE + 1000 + N)

#define GID_AVG_33  OPR(0)
#define GID_AVG_55  OPR(1)
#define GID_AVG_77  OPR(2)

#define GID_BLUR4_33 OPR(10)
#define GID_BLUR8_33 OPR(11)

#define GID_LAPLACE_4 OPR(20)
#define GID_LAPLACE_8 OPR(21)
#define GID_ROBERTS_1 OPR(22)
#define GID_ROBERTS_2 OPR(23)

#define GID_PREWITT_1 OPR(30)
#define GID_PREWITT_2 OPR(31)
#define GID_SOBEL_1   OPR(32)
#define GID_SOBEL_2   OPR(33)
#define GID_KIRSCH_1  OPR(34)
#define GID_KIRSCH_2  OPR(35)

