#ifndef c_linux__bezierToSource
#define c_linux__bezierToSource

/** @file
 * @brief Writing fixed-point points and Bezier curves as includable C-files.
 */

#include <int32x2.h>
#include <int32x2Fifo.h>

bool int32x2CurvePrintAsCFile_en (int e, const char* fn, const char* curveName, const Int32x2Curve *curve);
bool int32x2CurvePrintAsGnuplot_en (int e, const char* fn, const Int32x2Curve *curve);
bool int32x2CurvePrintAsCFileBezierCommands_en (int e, const char* fn, const char* curveName, const Int32x2Curve *curve, Int32 speed_en);


/** Speed-corrected version, polar coordinates. x=r, y=angle. This is MUCH MORE than just writing as C-Code.
 * @param e fixed point position
 * @param fn output file name
 * @param curveName the C-language base name of the array/enums that contains the curve points.
 * @param curve the curve definition
 * @param speed_en the physical speed along the curve.
 * @param id starting id number for the segments.
 * @return true, if successfully written, false otherwise.
 */
bool int32x2CurvePrintAsCFileBezierCommandsPolar_en (int e, const char* fn, const char* curveName, const Int32x2Curve *curve, Int32 speed_en, int id);

#endif
