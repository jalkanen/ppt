/*----------------------------------------------------------------------*/
/*
    PROJECT: ppt effects
    MODULE : the rotator effect.

    PPT and this file are (C) Janne Jalkanen 1995.

    $Id: rotate.c,v 1.1 2001/10/25 16:23:01 jalkanen Exp $
*/
/*----------------------------------------------------------------------*/

#undef DEBUG_MODE

/*----------------------------------------------------------------------*/
/* Includes */

#include <pptplugin.h>
#include <stdlib.h>
#include <math.h>

#ifdef __SASC
#include <dos.h>
#endif

/*----------------------------------------------------------------------*/
/* Defines */

/*
    You should define this to your module name. Try to use something
    short, as this is the name that is visible in the PPT filter listing.
*/

#define MYNAME      "Rotate"


/*----------------------------------------------------------------------*/
/* Type definitions */

typedef enum {
    NearestNeighbour = 0,
    Linear,
    Bicubic
} Interpolation_T;

const char *interp_labels[] = {
    "NearestNeighbour",
    "Linear",
#ifdef DEBUG_MODE
    "Bicubic",
#endif
    NULL
};


struct Values {
    LONG angle;
    Interpolation_T mode;
};

/*----------------------------------------------------------------------*/
/* Internal prototypes */

static FRAME *DoRotate( FRAME *, ULONG, LONG, Interpolation_T, struct PPTBase * );


/*----------------------------------------------------------------------*/
/* Global variables. Generally, you should keep these to the minimum,
   as it may well be that two copies of this same code is run at
   the same time. */

/*
    Just a simple string describing this effect.
*/

const char infoblurb[] =
    "Rotates an image between -180 and 180 degrees.\n\n";


/*
    This is the global array describing your effect. For a more detailed
    description on how to interpret and use the tags, see docs/tags.doc.
*/

const struct TagItem MyTagArray[] = {
    /*
     *  Here are some pretty standard definitions. All filters should have
     *  these defined.
     */

    PPTX_Name,              (ULONG) MYNAME,

    /*
     *  Other tags go here. These are not required, but very useful to have.
     */

    PPTX_Author,            (ULONG) "Janne Jalkanen 1997-2000",
    PPTX_InfoTxt,           (ULONG) infoblurb,

    PPTX_ColorSpaces,       CSF_ARGB|CSF_RGB|CSF_GRAYLEVEL,

    PPTX_RexxTemplate,      (ULONG)"ANGLE/A/N,INTERPOLATION=INT/K",

    PPTX_NoNewFrame,        TRUE,
    PPTX_ReqPPTVersion,     4,

    PPTX_SupportsGetArgs,   TRUE,

#ifdef _M68881
# ifdef _M68030
#  ifdef _M68060
    PPTX_CPU,               AFF_68060|AFF_68881,
#  else
    PPTX_CPU,               AFF_68030|AFF_68881,
#  endif
# endif
#endif

    TAG_END, 0L
};

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /*M_PI*/

#define SCALE       4096
#define HALFSCALE   2048

#define BB           (0.3333333333333333333333)
#define BC           (0.3333333333333333333333)

#define SQR(x) ((x)*(x))

/*----------------------------------------------------------------------*/
/* Code */

#ifndef _DCC
void __regargs __chkabort(void) {} /* Disable SAS/C Ctrl-C detection */
void __regargs _CXBRK(void) {}     /* Disable the handler as well */
#endif

/*
   BUG: angle must be between -180 & 180

*/
static
FRAME *DoRotate( FRAME *src, ULONG which, LONG angle, Interpolation_T mode, struct PPTBase *PPTBase )
{
    int rows = src->pix->height, cols = src->pix->width, newrows;
    int newcols, row, col;
    float fangle;
    FRAME *dest;
    WORD comps = src->pix->components;
    float center_x, center_y, x_disp, y_disp;
    float c_sin, c_cos;

    D(bug("DoRotate()\n"));

    /*
     *  Calculate the rotating angle
     */

    fangle = (float)angle * M_PI / 180.0;     /* convert to radians */

    c_sin = sin(fangle);
    c_cos = cos(fangle);

    /*
     *  Calculate the new image size
     */

    {
        float x1,x2,x3,y1,y2,y3;

        /* Top right corner (x=cols,y=rows) */
        x1 = fabs(cols * c_cos - rows * c_sin);
        y1 = fabs(cols * c_sin + rows * c_cos);

        /* Bottom right corner (x=cols,y=0) */
        x2 = fabs(cols * c_cos);
        y2 = fabs(cols * c_sin);

        /* Top left corner (x = 0, y = rows ) */
        x3 = fabs( - rows * c_sin);
        y3 = fabs(rows * c_cos);

        if( (angle >= 0 && angle <= 90) || (angle < -90) ) { /* 1st, 3rd q */
            newrows = (int)(fabs(y1)+0.5);
            newcols = (int)(fabs(x2)+fabs(x3)+0.5);
        } else {
            newrows = (int)(fabs(y2)+fabs(y3)+0.5);
            newcols = (int)(fabs(x1)+0.5);
        }
    }

    center_x = (cols-1)/2.0;
    center_y = (rows-1)/2.0;

    x_disp = (newcols-1)/2.0; // + 0.5;
    y_disp = (newrows-1)/2.0; // + 0.5;

    /*
     *  Create the new image
     */

    dest = MakeFrame(src);
    dest->pix->width = newcols;
    dest->pix->height = newrows;

    D(bug("\tOrigheight = %ld, origwidth = %ld\n",rows,cols));
    D(bug("\tRotating angle = %ld = %.2f rad\n",angle,fangle));
    D(bug("\tNew height = %ld, new width = %ld\n",newrows, newcols ));

    if( InitFrame( dest ) != PERR_OK ) {
        RemFrame( dest );
        return NULL;
    }

    InitProgress( src, "Rotating...", 0, newrows );

    /*
     *  Do the rotating
     */

    for( row = 0; row < newrows; row++ ) {
        ROWPTR cp;

        cp = GetPixelRow( dest, row );

        if(Progress( src, row )) {
            RemFrame(dest);
            SetErrorCode(src,PERR_BREAK);
            return NULL;
        }

        for( col = 0; col < newcols; col++ ) {
            float x,y;
            int i;

            x = (col-x_disp) * c_cos + (row-y_disp) * c_sin + center_x;
            y = -(col-x_disp) * c_sin + (row-y_disp) * c_cos + center_y;

            // D(bug("Mapping (%.2f,%.2f) to (%d,%d)\n", y,x,row,col));

            if( x < -0.49 || y < -0.49 || x > (cols-1) || y > (rows-1) ) {
                for( i = 0; i < comps; i++ ) {
                    cp[comps*col+i] = 0; /* Black */
                }
            } else {
                ROWPTR scp, scp2[5];
                float a, b, dist;
                UWORD grows;
                float sum;
                int i,j;

                switch(mode) {
                    int xx,yy;

                    case NearestNeighbour:
                        scp = GetPixelRow( src, (int) (y+0.5) );
                        for(i = 0; i < comps; i++) {
                            cp[comps*col+i] = scp[(int)(x+0.5)*comps+i];
                        }
                        break;

                    case Linear:
                        a = x - (int)(x);
                        b = y - (int)(y);
                        grows = GetNPixelRows( src, scp2, (int) (y),2 );
                        xx = (int)(x+0.5);
                        yy = (int)(x+0.5);

                        if(grows == 2) {
                            for(i = 0; i < comps; i++) {
                                cp[comps*col+i] = (1.0-a)*(1.0-b) * scp2[0][xx*comps+i]
                                        + a*(1.0-b) * scp2[0][(xx+1)*comps+i]
                                        + b*(1.0-a) * scp2[1][(xx)*comps+i]
                                        + a*b*scp2[1][(int)(xx+1)*comps+i];
                            }
                        } else {
                            for(i = 0; i < comps; i++ ) {
                                cp[col] = (1.0-a)*(1.0-b) * scp2[0][(int)(x+0.5)*comps+i]
                                        + a*(1.0-b) * scp2[0][(int)(x+1.5)*comps+i];
                            }
                        }
                        break;

                    case Bicubic:
                        a = x - (int)(x);
                        b = y - (int)(y);
                        sum = 0.0;
                        grows = GetNPixelRows( src, scp2, (int)(y)-1, 4 );
                        for( i = -1; i <= 2; i++ ) {
                            for(j = -1; j <= 2; j++ ) {
                                if( scp2[i+1] ) {
                                    float h3, alpha;

                                    if( fabs(j-a) > 0.001 ) // BUG: Arbitrary epsilon
                                        alpha = atan( fabs(i-b) / fabs(j-a) );
                                    else
                                        alpha = M_PI/2;

                                    if( alpha > M_PI/4 ) alpha = M_PI/2-alpha;

                                    dist = sqrt( SQR(a-j) + SQR(b-i) ) * cos(alpha);
                                    // dist = sqrt( SQR(j-a) + SQR(i-b) );
                                    // dist = MAX( (abs(j-a) ),(abs(i-b)) );

                                    if( dist < 1.0 ) {
                                        h3 = 1.0 - 2.0 * SQR(dist) + dist*dist*dist;
                                    } else {
                                        if( dist < 2.0 ) {
                                            h3 = 4.0 - 8.0*dist + 5.0*SQR(dist) - dist*dist*dist;
                                        } else {
                                            h3 = 0.0;
                                        }
                                    }

                                    sum += (h3 * (float)scp2[i+1][(int)(x+0.5)+j]);
                                }
                            }
                        }

                        // D(bug("Sum is %lf\n",sum));

                        if( sum > 255.0 )
                            cp[col] = 255;
                        else if( sum < 0.0 )
                            cp[col] = 0;
                        else
                            cp[col] = (UBYTE)sum;
                        break;

                }
            }

        }

        PutPixelRow( dest, row, cp );
    }

    FinishProgress( src );

#ifdef _DCC
    CloseFloats();
#endif

    return dest;
}



struct Library *MathIeeeDoubBasBase = NULL, *MathIeeeDoubTransBase = NULL;
struct DosLibrary *DOSBase;

LIBINIT
{
    if( MathIeeeDoubBasBase = OpenLibrary("mathieeedoubbas.library",0L) ) {
        if( MathIeeeDoubTransBase = OpenLibrary("mathieeedoubtrans.library", 0L )) {
            return 0;
        }
        CloseLibrary(MathIeeeDoubBasBase);
    }
    return 1;
}

LIBCLEANUP
{
    if( MathIeeeDoubBasBase ) CloseLibrary(MathIeeeDoubBasBase);
    if( MathIeeeDoubTransBase ) CloseLibrary(MathIeeeDoubTransBase);
}

EFFECTINQUIRE(attr,PPTBase,EffectBase)
{
    return TagData( attr, MyTagArray );
}

ASM ULONG MyHookFunc( REG(a0) struct Hook *hook,
                      REG(a2) Object *obj,
                      REG(a1) struct ARUpdateMsg *msg )
{
    /*
     *  Set up A4 so that globals still work
     */

    PUTREG(REG_A4, (long) hook->h_Data);

    switch( msg->MethodID ) {
        case ARM_UPDATE:
            D(bug("ARM_UPDATE\n"));
            break;
        case ARM_RENDER:
            D(bug("ARM_RENDER\n"));
            break;
    }

    return ARR_DONE;
}

PERROR DoGUI( FRAME *frame, struct Values *v, struct PPTBase *PPTBase )
{
    struct Hook pwhook = {{0}, MyHookFunc, 0L, NULL};
    struct TagItem rot[] = { ARSLIDER_Min, -180,
                             ARSLIDER_Default, 0,
                             ARSLIDER_Max, 180,
                             AROBJ_Value, NULL,
                             // AROBJ_PreviewHook, NULL,
                             TAG_END };

    struct TagItem ant[] = { ARCYCLE_Active, 0,
                             AROBJ_Value, NULL,
                             AROBJ_Label, (ULONG)"Interpolation",
                             ARCYCLE_Labels,(ULONG)interp_labels,
                             TAG_END };

    struct TagItem win[] = { AR_Text, (ULONG)"\nSelect rotation angle!\n",
                             AR_SliderObject, 0,
                             AR_CycleObject, 0,
                             AR_HelpNode, (ULONG) "effects.guide/Rotate",
                             // AR_RenderHook, NULL,
                             TAG_END };
    FRAME *pwframe;
    PERROR res = PERR_OK;

    rot[3].ti_Data = (ULONG) &v->angle;
    rot[1].ti_Data = (ULONG) v->angle;
    // rot[4].ti_Data = (ULONG) &pwhook;

    ant[0].ti_Data = (ULONG) v->mode;
    ant[1].ti_Data = (ULONG) &v->mode;
    win[1].ti_Data = (ULONG) rot; win[2].ti_Data = (ULONG) ant;
    // win[4].ti_Data = (ULONG) &pwhook;

    pwhook.h_Data = (void *)GETREG(REG_A4); // Save register

    pwframe = ObtainPreviewFrame( frame, NULL );

    res = AskReqA( frame, win );

    if( pwframe ) ReleasePreviewFrame( pwframe );

    return res;
}

VOID GetDefaults( struct Values *v, struct PPTBase *PPTBase )
{
    struct Values *av;

    /* Defaults */

    v->angle = 0; v->mode = 0;

    if( av = GetOptions(MYNAME) )
        *v = *av;

}

PERROR ParseRexxArgs( ULONG *args, struct Values *v, struct PPTBase *PPTBase )
{
    if( args[0] )
        v->angle = *( (LONG *)args[0] );

    if( args[1] )
        v->mode = 1; // BUG:

    return PERR_OK;
}

EFFECTEXEC(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    FRAME *newframe;
    PERROR res;
    struct Values v;

    D(bug(MYNAME": Exec()\n"));

    GetDefaults( &v, PPTBase );

    args = (ULONG *) TagData( PPTX_RexxArgs, tags );

    if( args ) {
        ParseRexxArgs( args, &v, PPTBase );
    } else {

        if( ( res = DoGUI( frame, &v, PPTBase )) != PERR_OK) {
            SetErrorCode( frame, res );
            return NULL;
        }
    }

    D(bug("Got angle %ld\n", v.angle ));

    if( v.angle < -180 || v.angle > 180 ) {
        SetErrorCode( frame, PERR_INVALIDARGS );
        return NULL;
    } else {
        newframe = DoRotate( frame, 0, v.angle, v.mode, PPTBase );
    }

    PutOptions( MYNAME, &v, sizeof(struct Values) );

    return newframe;
}

EFFECTGETARGS(frame,tags,PPTBase,EffectBase)
{
    ULONG *args;
    PERROR res;
    struct Values v;
    STRPTR buffer;

    GetDefaults( &v, PPTBase );

    args   = (ULONG *) TagData( PPTX_RexxArgs, tags );
    buffer = (STRPTR) TagData( PPTX_ArgBuffer, tags );

    if( args )
        ParseRexxArgs( args, &v, PPTBase );

    res = DoGUI( frame, &v, PPTBase );

    if( res == PERR_OK ) {
        // (ULONG)"ANGLE/A/N,INTERPOLATION=INT/K",
        SPrintF(buffer,"ANGLE %d INTERPOLATION %s",
                        v.angle,
                        interp_labels[v.mode] );
    }

    return res;
}

/*----------------------------------------------------------------------*/
/*                            END OF CODE                               */
/*----------------------------------------------------------------------*/

