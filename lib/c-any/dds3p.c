#include <dds3p.h>

void ddsInterrupt(Dds3p *dds, int tUs) {
	const unsigned waveTime = 1<<dds->internal.timeBits;

	const int p0 = (dds->phi+dds->tuning.p) >> (32 -dds->internal.polesBits -dds->internal.timeBits) & (waveTime-1);
	dds->ps[0] = p0;
	dds->ps[1] = p0 +(1*waveTime/3) & waveTime-1;
	dds->ps[2] = p0 +(2*waveTime/3) & waveTime-1;

	// calculate new phase
	const unsigned phi1 = dds->phi + dds->tuning.w;
	const int phi1Us = tUs;

	// check for zero crossing
	if (phi1 < dds->phi) {
		// calculate zero crossing using softTimer
		const int zUs = dds->internal.phiUs
			+ (phi1Us - dds->internal.phiUs)	* (unsigned)(-dds->phi)
								/ (unsigned)(phi1 - dds->phi);

		// store new cycle and zero crossing times
		indexMarkUpdate(&dds->indexMark,zUs);
	}
	dds->phi = phi1;
	dds->internal.phiUs = phi1Us;	// save for next try
}

