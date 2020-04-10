#ifndef __quadratic_h
#define __quadratic_h

/** @file
 * @brief Solving quadratic equations using 64bit integer math.
 *
 */

#include <int64Math.h>
#include <int32x2.h>

/** Solves the equation x^2 + px + q = 0
 * TESTED: 22.11.2016
 * @param e the fixed point position
 * @param p linear coefficient in above equation
 * @param q constant part in above equation
 * @return a pair of solutions. These may be different (2 solutions), identical (1 solution)  or both int32Undefined()
 *   (no solution). The first component is always the smaller one
 */
Int32x2 int32MidnightPq_en (int e, Int32 p, Int32 q);

/** Solves the equation ax^2 + bx + c = 0
 * TESTED: 22.11.2016
 * @param e the fixed point position
 * @param a quadratic coefficient in above equation
 * @param b linear coefficient in above equation
 * @param c constant part in above equation
 * @return a pair of solutions. These may be different (2 solutions), identical (1 solution) or both int32Undefined()
 *   (no solution). The first component is always the smaller one
 */
Int32x2 int32MidnightAbc_en (int e, Int32 a, Int32 b, Int32 c);

#endif
