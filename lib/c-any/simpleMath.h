#ifndef __simpleMath_h
#define __simpleMath_h

/** @file
 * @brief Interpolation, smoothing, limiting and regulation using integer math.
 */

#include <stdbool.h>
#include <integers.h>

/** Limits an integer to the specified number of bits.
 * @param value the integer to bit-limit.
 * @param bitMask a value like: 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, ...
 */
int boundedSigned(int value, int bitMask);

/** Low pass filter, formula: value = value * (1-speed) + input*speed .
 */
typedef struct {
	int		milliSpeed;	///< units 1/1000
	int		value;		///<
} LowPass;

/** Feeds a new value into the low pass filter.
 * @param lowPass the lowPass filter object (containing state).
 * @param value a new input value.
 * @return current filter output.
 */
int lowPassApply(LowPass *lowPass, int value);

/** PI regulator. formula output = (input*milliP/1000) + integratedInput * milliI/1000, integratedInput += input . 
 */
typedef struct {
	int		milliP;
	int		milliI;
	int		integratedInput;
} RegulatorPi;

/** Sets the integrating part of the Regulator to offset.
 * @param regulator the PI-regulator object
 * @param offset the desired output at input = 0.
 */
void regulatorPiOffset(RegulatorPi *regulator, int offset);

/** Feeds a new input value into the regulator and calculates a new output value 
 * @param regulator the PI-regulator object.
 * @param input the input signal.
 */
int regulatorPiApply(RegulatorPi *regulator, int input);

typedef char EdgeDetectBool;

/** Returns true, if the boolean value changes its value.
  * @param state the state variable needed.
  * @param value the current boolean value
  * @return true, if value is different from the value at the last call of this function.
  */
bool edgeDetectBool(EdgeDetectBool *state, bool value);

/** Gets the latest value.
 * @param state the edge detector internal state.
 * @return the value of the latest call of edgeDetectBool.
 */
bool edgeDetectBoolValue(const EdgeDetectBool *state);

typedef struct {
	int	value;
	bool	initialized;	///< false at normal initialization
} EdgeDetectInt;

/** Returns true, if the int value changes its value.
  * @param state the state variable needed.
  * @param value the current int value
  * @return true, if value is different from the value at the last call of this function.
  */
bool edgeDetectInt(EdgeDetectInt *state, int value);

/** Gets the latest value.
 * @param state the edge detector internal state.
 * @return the value of the latest call of edgeDetectInt.
 */
int edgeDetectIntValue(const EdgeDetectInt *state);

/** Reverses the order of the lower bits of an integer.
 * @param value the bit pattern to reverse
 * @param length the number of bits to reverse.
 * @return a pattern where bits 0..length-1 are mapped to bits length-1..0.
 */
unsigned bitReverse(unsigned value, unsigned length);

/** Changes the byte-endianess of a 16-bit value.
 */
Uint16 changeEndian16(Uint16 value);

int changeEndian24(int value);

int changeEndian32(int value);

/** Perform a one's complement addition.
 */
Uint16 onesComplementAdd16(Uint16 a, Uint16 b);

/** Calculates z1*z2 / n1*n2 using 64-bits intermediate values. Therefore no overflow will occur as long as
 * the result fits into a 32-bit integer.
 */
Int32 fractionInt32(Int32 z1, Int32 z2, Int32 n1, Int32 n2);
Int32 fractionInt32Saturate(Int32 z1, Int32 z2, Int32 n1, Int32 n2);

typedef struct {
	Int32	v[3];
} Diff2;

void diff2Init(Diff2* diff2, Int32 value);
void diff2Feed(Diff2* diff2, Int32 value);
Int32 diff2Derive(Diff2* diff2, int order);

/** Limits the change in value.
 * @param value current value, any integer.
 * @param wishValue the desired new value, any integer.
 * @param deltaLimit the maximum abs(offset) from value. This must be a positive value.
 * @return the new value.
 */
Int32 limitMovement(Int32 value, Int32 wishValue, Int32 deltaLimit);

typedef struct {
	int	lower;		///< switch off threshold
	int	upper;		///< switch on threshold
	bool	value;		///< current switch value
} SchmittTrigger;

/** Detects a rising edge of the Schmitt-Trigger.
 * @param trigger the Schmitt-trigger object (and state).
 * @param adValue the current value as input to the trigger.
 * @return true, if the upper bound was just exceeded.
 */
bool schmittTriggerRisingEdge(SchmittTrigger *trigger, int adValue);

/** Detects a falling edge of the Schmitt-Trigger.
 * @param trigger the Schmitt-trigger object (and state).
 * @param adValue the current value as input to the trigger.
 * @return true, if the upper bound was just exceeded.
 */
bool schmittTriggerFallingEdge(SchmittTrigger *trigger, int adValue);


#endif
