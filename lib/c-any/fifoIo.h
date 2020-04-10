/** @file
 * @brief Names for stdin/stdout-Fifos.
 *
 * This module can be used as a platform-independend way to perform (mostly) text input and output.
 * Depending on the platform, an interrupt-driven IO-handler should take care of the real transmission of characters.
 *
 * The Fifos can be changed at runtime and the initial values are empty Fifos to avoid memory corruption. The change
 * is performed by simple pointer assignment.
 */

#include <fifo.h>

extern Fifo * volatile fi;	///< input Fifo
extern Fifo * volatile fo;	///< output Fifo

/** This function should be called, after some characters are written. This wakes up the transmission service routine
 * that is suspended every time the transmission Fifo runs dry.
 */
extern void fifoIoWakeUp(void);

