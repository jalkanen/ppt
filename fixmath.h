/*
 *  This is a header file for fixed point maths.
 *  It can be easily configured to run on any precision,
 *  just change the defines below.
 *
 *  $Id: fixmath.h,v 1.1 1996/05/06 21:44:53 jj Exp $
 *
 *  The fixed point type is always signed. Must be 32 bits.
 */

#ifndef FIXMATH_H
#define FIXMATH_H

typedef signed long fixed;

/*
 *  This define tells how much bits accuracy we should use.
 *  10 = the system is accurate to 1024ths.
 *  We make a reasonable estimation, unless the user has
 *  overriden it by setting his own FIX_DECIMALBITS before
 *  including this file.
 *
 *  I don't suggest setting it higher than 16, because the
 *  values overflow pretty easily during multiplication.
 */

#ifndef FIX_DECIMALBITS
#define FIX_DECIMALBITS     12
#endif

/*
 *  These calculate the minimum and maximum values
 */

#define FIX_INTEGERBITS     (31-FIX_DECIMALBITS)

#define FIX_MAX             ((1 << FIX_INTEGERBITS) - 1)
#define FIX_MIN             (-(1 << FIX_INTEGERBITS))

/*
 *  Some useful values
 */

#define FIX_SHIFT           (FIX_DECIMALBITS)
#define FIX_DIVISOR         (1<<FIX_DECIMALBITS)

#define FIX_ZERO            0L
#define FIX_ONE             (1 << FIX_SHIFT)
#define FIX_HALF            (FIX_ONE >> 1)

/*
 *  Fixed point calculations:
 *
 *  Addition : just say a+b
 *  Subtraction: just say a-b
 *  Multiplication: use FIXMUL()
 *  Division: just use FIXDIV()
 *    BUG: The division loses some decimals...
 */

#define FIXMUL(a,b)         ((a*b) >> FIX_SHIFT)
#define FIXDIV(a,b)         ((a/b) << FIX_SHIFT)

/*
 *  Type conversions
 */

#define FIX2INT(a)          (a >> FIX_SHIFT)
#define INT2FIX(i)          (i << FIX_SHIFT)

#define FIX2FLOAT(a)        ( (float)(a) / FIX_DIVISOR )
#define FLOAT2FIX(f)        ( (fixed)(f * FIX_DIVISOR) )

#endif /* FIXMATH_H */

