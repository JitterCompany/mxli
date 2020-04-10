//HEADER
#include <simpleMath.h>

//SLICE
Uint32 uint32Power(Uint32 x, Uint32 y) {
	Uint32 p = 1;
	for (int i=0; i<y; ++i, p*=x);

	return p;
}

//SLICE
int boundedSigned(int value, int bitMask) {
	const int maximum = bitMask>>1;
	const int minimum = -maximum-1;
	const int shift = bitMask+1;

	if (value>maximum) return value-shift;
	else if (value<minimum) return value+shift;
	else return value;
}

//SLICE
void regulatorPiOffset(RegulatorPi *regulator, int offset) {
	regulator->integratedInput = offset*1000/regulator->milliI;
}

//SLICE
int regulatorPiApply(RegulatorPi *regulator, int input) {
	const int output = (regulator->milliP*input + regulator->milliI*regulator->integratedInput)/1000;
	regulator->integratedInput += input;
	return output;
}

//SLICE
int lowPassApply(LowPass *lowPass, int value) {
	lowPass->value = (lowPass->milliSpeed*value + lowPass->value*(1000-lowPass->milliSpeed))/1000;
	return lowPass->value;
}


//SLICE
bool edgeDetectBool(EdgeDetectBool *state, bool value) {
	bool edge = (*state==0) || (1+value != *state);
	*state = 1+value;
	return edge;
}

//SLICE
bool edgeDetectBoolValue(const EdgeDetectBool *state) {
	return *state==2;
}

//SLICE
bool edgeDetectInt(EdgeDetectInt *state, int value) {
	const bool edge = value!=state->value || !state->initialized;

	state->value = value;
	state->initialized = true;

	return edge;
}

//SLICE
int edgeDetectIntValue(const EdgeDetectInt *state) {
	return state->value;
}

//SLICE
unsigned bitReverse(unsigned value, unsigned length) {
	unsigned result = 0;
	for (int i=0; i<length; ++i) result |= value & 1<<i ? 1<<length-1-i : 0;

	return result;
}

//SLICE
Uint16 changeEndian16(Uint16 value) {
	return value>>8 | (value<<8);
}

//SLICE
int changeEndian24(int value) {
	return	0
		| value>>16 & 0x0000FF
		| value     & 0x00FF00
		| value<<16 & 0xFF0000
		;
}

//SLICE
int changeEndian32(int value) {
	return	0
		| value>>24 & 0x000000FF
		| value>>8  & 0x0000FF00
		| value<<8  & 0x00FF0000
		| value<<24 & 0xFF000000
		;
}

//SLICE
Uint16 onesComplementAdd16(Uint16 a, Uint16 b) {
	Uint32 sum = a+b;
	return ((sum & 1<<16) == 0) ? sum : 0xFFFF & sum+1;
}

//SLICE
/*
Int32 fractionInt32(Int32 z1, Int32 z2, Int32 n1, Int32 n2) {
	return int64Div( z1*(Int64)z2, n1*(Int64)n2);
}

Int32 fractionInt32Saturate(Int32 z1, Int32 z2, Int32 n1, Int32 n2) {
	Int64 result64 = int64Div( ((Int64)z1)*((Int64)z2), ((Int64)n1)*((Int64)n2));
	if (result64>=0x7FFFFFFFllu) return 0x7FFFFFFF ;
	else if (result64< -(0x80000000ll)) return 0x80000000;
	else return result64;
}
*/

//SLICE
void diff2Init(Diff2* diff2, Int32 value) {
	diff2->v[0] = value;
	diff2->v[1] = value;
	diff2->v[2] = value;
}

//SLICE
void diff2Feed(Diff2* diff2, Int32 value) {
	diff2->v[2] = diff2->v[1];
	diff2->v[1] = diff2->v[0];
	diff2->v[0] = value;
}

//SLICE
Int32 diff2Derive(Diff2* diff2, int order) {
	switch(order) {
		case 0: return diff2->v[0];
		case 1: return diff2->v[0]-diff2->v[1];
		case 2: return diff2->v[0]-2*diff2->v[1]+diff2->v[2];
		default: return 0;
	}
}

//SLICE
Int32 limitMovement(Int32 value, Int32 wishValue, Int32 maxOffset) {
	Int32 diff = wishValue-value;		// diff could be 0x8000 0000 here
	if (diff>=0) return diff<=maxOffset ? wishValue : value + maxOffset;
	//else return (-diff<=maxOffset) ? wishValue : value - maxOffset;	// this fails if diff is -2^31
	else return (diff >= -maxOffset ? wishValue : value - maxOffset);
}

//SLICE
bool schmittTriggerRisingEdge(SchmittTrigger *trigger, int adValue) {
	const bool previousValue = trigger->value;
	if (adValue<=trigger->lower) trigger->value = false;
	if (adValue>=trigger->upper) trigger->value = true;
	return !previousValue && trigger->value;
}

//SLICE
bool schmittTriggerFallingEdge(SchmittTrigger *trigger, int adValue) {
	const bool previousValue = trigger->value;
	if (adValue<=trigger->lower) trigger->value = false;
	if (adValue>=trigger->upper) trigger->value = true;
	return previousValue && !trigger->value;
}

