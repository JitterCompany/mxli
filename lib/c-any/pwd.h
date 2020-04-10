#ifndef __pwd_h
#define __pwd_h

#include <stdbool.h>

/** @file
 * @brief A simple pulse width decoder.
 */

/** Pulse width decoder.
 */
typedef struct {
	int		tPulseWidthMin;
	int		tPulseWidthMax;
	int		tPrevious;
} Pwd;

static inline void pwdInit(Pwd *pwd, unsigned pulseWidth, unsigned tolerance) {
	pwd->tPulseWidthMin = pulseWidth-tolerance;
	pwd->tPulseWidthMax = pulseWidth+tolerance;
} 

static inline bool pwdDecode(Pwd *pwd, int t) {
	const int deltaT = t-pwd->tPrevious;
	pwd->tPrevious = t;
	return pwd->tPulseWidthMin<=deltaT && deltaT<=pwd->tPulseWidthMax;
}
	
#endif
