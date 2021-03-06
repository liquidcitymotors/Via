
#ifndef INC_VIA_DSP_H_
#define INC_VIA_DSP_H_
#include <stdlib.h>
#include <stdint.h>


#ifdef BUILD_F373

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f3xx.h"

/** \file dsp.hpp
 * Some math functions.
 * Fixed point arithmetic, interpolation, averaging.
 */

///
/// Circular buffer, max length 8.
///
typedef struct {
	/// Data buffer
	int32_t buff[8];
	/// Write head position
	int32_t writeIndex;
} lilBuffer;

///
/// Circular buffer, max length 32.
///
typedef struct {
	/// Data buffer
	int32_t buff[32];
	/// Write head position
	int32_t writeIndex;
} buffer;

///
/// Circular buffer, max length 256.
///
typedef struct {
	/// Data buffer
	int32_t buff[256];
	/// Write head position
	int32_t writeIndex;
} longBuffer;

///
/// Write to value buffer at current write index and increment index.
///
static inline void writeLilBuffer(lilBuffer* buffer, int32_t value) {
	buffer->buff[(buffer->writeIndex++) & 7] = value;
}

///
/// Read value at index in buffer relative to write position.
///
static inline int32_t readLilBuffer(lilBuffer* buffer, int32_t Xn) {
	return buffer->buff[(buffer->writeIndex + (~Xn)) & 7];
}

///
/// Write to value buffer at current write index and increment index.
///
static inline void writeBuffer(buffer* buffer, int32_t value) {
	buffer->buff[(buffer->writeIndex++) & 31] = value;
}

///
/// Read value at index in buffer relative to write position.
///
static inline int32_t readBuffer(buffer* buffer, int32_t Xn) {
	return buffer->buff[(buffer->writeIndex + (~Xn)) & 31];
}

///
/// Write to value long buffer at current write index and increment index.
///
static inline void writeLongBuffer(longBuffer* buffer, int32_t value) {
	buffer->buff[(buffer->writeIndex++) & 255] = value;
}

/// Read value at index relative to write position.
static inline int32_t readLongBuffer(longBuffer* buffer, int32_t Xn) {
	return buffer->buff[(buffer->writeIndex + (~Xn)) & 255];
}

/**
 *
 * Absolute value for 32 bit int32_t
 *
 */

inline int32_t int32_abs(int32_t n) {
  int32_t mask = n >> 31;
  return ((n + mask) ^ mask);
}

/*
 *
 * Fixed point math
 * (mostly 16 bit)
 *
 */

/// Multiply 16.16 point numbers, overflows when in0*in1 > 1<<48, no saturation.
static inline int32_t fix16_mul(int32_t in0, int32_t in1) {

	int32_t lsb;
	  int32_t msb;

	  // multiply the inputs, store the top 32 bits in msb and bottom in lsb

	  __asm ("SMULL %[result_1], %[result_2], %[input_1], %[input_2]"
	    : [result_1] "=r" (lsb), [result_2] "=r" (msb)
	    : [input_1] "r" (in0), [input_2] "r" (in1)
	  );

	  // reconstruct the result with a left shift by 16
	  // pack the bottom halfword of msb into the top halfword of the result
	  // top halfword of lsb goes into the bottom halfword of the result

	  return __ROR(__PKHBT(msb, lsb, 0), 16);

}

/// Multiply 8.24 point numbers, overflows when in0*in1 > 1<<56, no saturation.
static inline int32_t fix24_mul(int32_t in0, int32_t in1) {
	int64_t result = (uint64_t) in0 * in1;
	return result >> 24;
}

/// Multiply 8.24 point numbers, overflows when in0*in1 > 1<<64, no saturation.
static inline int32_t fix48_mul(uint32_t in0, uint32_t in1) {
	// taken from the fixmathlib library
	int64_t result = (uint64_t) in0 * in1;
	return result >> 48;
}

/*
 *
 * Interpolation
 *
 */

/// Linear interpolation. in0 and in1 can be full scale 32 bit, frac ranges from 0 - 0xFFFF
static inline int32_t fix16_lerp(int32_t in0, int32_t in1, int32_t frac) {
	// maybe could do this better by shoving frac up by 16,
	// loading in0 in the msb of a 64 bit accumulator,
	// and taking the topword of SMLLA
	return in0 + fix16_mul(in1 - in0, frac);
}

/// Faster linear interpolation. in0 and in1 must be 16 bit, frac ranges from 0 - 0xFFFF
static inline int32_t fast16_16_lerp(int32_t in0, int32_t in1, int32_t frac) {
	return in0 + (((in1 - in0) * frac) >> 16);
}

/// Fastest linear interpolation. in0 and in1 must be 16 bit, frac ranges from 0 - 0x7FFF
static inline int32_t fast_fix15_lerp(int32_t in0, int32_t in1, int32_t frac) {

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (in1 - in0), [input_2] "r" (frac), [input_3] "r" (in0 >> 1)
	);

	return in0 << 1;
}

/// Fastest linear interpolation, good for sample values
static inline int32_t fast_15_16_lerp(int32_t in0, int32_t in1, int32_t frac) {

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac), [input_2] "r" (in1 - in0), [input_3] "r" (in0)
	);

	return in0;
}

/// Bilinear interpolation using fast_fix15_lerp, frac0 interpolates between in0 and in1 then in2 and in3, frac 1 interpolates between those results
static inline int32_t fix15_bilerp(int32_t in0, int32_t in1, int32_t in2, int32_t in3, int32_t frac0,
		int32_t frac1) {

	in0 = fast_fix15_lerp(in0, in1, frac0);
	in2 = fast_fix15_lerp(in2, in3, frac0);

	return fast_fix15_lerp(in0, in2, frac1);
}

/// Bilinear interpolation alt form, frac0 interpolates between in0 and in1 then in2 and in3, frac 1 interpolates between those results
static inline int32_t fix15_bilerp_alt(int32_t in0, int32_t in1, int32_t in2, int32_t in3,
		int32_t frac0, int32_t frac1) {

	int32_t invFrac = 32767 - frac0;

	__asm ("SMULWB %[result_1], %[input_1], %[input_2]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (in0), [input_2] "r" (invFrac)
	);

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (in1), [input_2] "r" (frac0), [input_3] "r" (in0)
	);

	__asm ("SMULWB %[result_1], %[input_1], %[input_2]"
			: [result_1] "=r" (in2)
			: [input_1] "r" (in2), [input_2] "r" (invFrac)
	);

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in2)
			: [input_1] "r" (in3), [input_2] "r" (frac0), [input_3] "r" (in2)
	);

	__asm ("SMULWB %[result_1], %[input_1], %[input_2]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (in0 << 1), [input_2] "r" (32767 - frac1)
	);

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (in2 << 1), [input_2] "r" (frac1), [input_3] "r" (in0)
	);

	return in0 << 1;
}

static inline int32_t fast_15_16_lerp_prediff(int32_t in0, int32_t frac) {

	__asm ("SMLAWT %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac), [input_2] "r" (in0), [input_3] "r" (in0 & 0xFFFF)
	);

	return in0;
}

// inherits those values

static inline int32_t fast_15_16_bilerp(int32_t in0, int32_t in1, int32_t in2, int32_t in3,
		int32_t frac0, int32_t frac1) {

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac0), [input_2] "r" (in1 - in0), [input_3] "r" (in0)
	);

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in2)
			: [input_1] "r" (frac0), [input_2] "r" (in3 - in2), [input_3] "r" (in2)
	);

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac1), [input_2] "r" (in2 - in0), [input_3] "r" (in0)
	);

	return in0;
}

// assume in0 and in have the base value and the difference to the next point packed in bottom and top word
// frac0 and frac1 can be up to 16 bits

static inline int32_t fast_15_16_bilerp_prediff(int32_t in0, int32_t in1, int32_t frac0,
		int32_t frac1) {

	__asm ("SMLAWT %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac0), [input_2] "r" (in0), [input_3] "r" (in0 & 0xFFFF)
	);

	__asm ("SMLAWT %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in1)
			: [input_1] "r" (frac0), [input_2] "r" (in1), [input_3] "r" (in1 & 0xFFFF)
	);

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac1), [input_2] "r" (in1 - in0), [input_3] "r" (in0)
	);

	return in0;
}

static inline int32_t fast_15_16_bilerp_prediff_delta(int32_t in0, int32_t in1, int32_t frac0,
		int32_t frac1, int32_t * delta) {

	__asm ("SMLAWT %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac0), [input_2] "r" (in0), [input_3] "r" (in0 & 0xFFFF)
	);

	__asm ("SMLAWT %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in1)
			: [input_1] "r" (frac0), [input_2] "r" (in1), [input_3] "r" (in1 & 0xFFFF)
	);

	*delta = ((uint32_t) in1 - in0) >> 31;

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac1), [input_2] "r" (in1 - in0), [input_3] "r" (in0)
	);

	return in0;
}

static inline int32_t fast_15_16_bilerp_prediff_deltaValue(int32_t in0, int32_t in1, int32_t frac0,
		int32_t frac1, int32_t * delta) {

	__asm ("SMLAWT %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac0), [input_2] "r" (in0), [input_3] "r" (in0 & 0xFFFF)
	);

	__asm ("SMLAWT %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in1)
			: [input_1] "r" (frac0), [input_2] "r" (in1), [input_3] "r" (in1 & 0xFFFF)
	);

	*delta = in1 - in0;

	__asm ("SMLAWB %[result_1], %[input_1], %[input_2], %[input_3]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (frac1), [input_2] "r" (in1 - in0), [input_3] "r" (in0)
	);

	return in0;
}

static inline int32_t fix32_mul_signed(int32_t in0, int32_t in1) {
	__asm ("SMMUL %[result_1], %[input_1], %[input_2]"
			: [result_1] "=r" (in0)
			: [input_1] "r" (in0), [input_2] "r" (in1)
	);

	return in0;
}

//static inline int32_t fix32_mul_unsigned(int32_t in0, int32_t in1) {
//	return
//}

// overkill probably, assumes 9 tables and 517 samples padded with two at the start and end of each waveform
// this is fortunately the default table load
// morph should have a max value of the table size

static inline int32_t getSampleQuinticSpline(uint32_t phase, uint32_t morph,
		uint32_t *fullTableHoldArray, int32_t *delta) {

	/* in this function, we use our phase position to get the sample to give to our dacs using a quintic spline interpolation technique
	 essentially, we need to get 6 pairs of sample values and two "fractional arguments" (where are we at in between those sample values)
	 one fractional argument determines the relative position in a linear interpolation between the sample values (morph)
	 the other is the fractional phase argument used in the interpolation calculation
	 */

	uint32_t LnSample; // indicates the left most sample in the neighborhood of sample values around the phase pointer
	uint32_t LnFamily; // indicates the nearest neighbor (wavetable) to our morph value in the family
	uint32_t phaseFrac; // indicates the fractional distance between the nearest sample values in terms of phase
	uint32_t morphFrac; // indicates the fractional distance between our nearest neighbors in the family
	uint32_t * leftIndex;

	// we do a lot of tricky bitshifting to take advantage of the structure of a 16 bit fixed point number
	// truncate phase then add two to find the left neighboring sample of the phase pointer

	LnSample = phase >> 16;

	// bit shifting to divide by the correct power of two takes a 12 bit number (our morph) and returns the a quotient in the range of our family size

	LnFamily = morph >> 16;

	leftIndex = (fullTableHoldArray + LnFamily * 517) + LnSample;

	// determine the fractional part of our phase phase by masking off the integer

	phaseFrac = 0xFFFF & phase;

	// we have to calculate the fractional portion and get it up to full scale q16

	morphFrac = (morph & 0xFFFF);

	// perform the 6 linear interpolations to get the sample values and apply morph
	// TODO track delta change in the phase event array

	int32_t sample0 = fast_15_16_lerp_prediff(*leftIndex, morphFrac);
	int32_t sample1 = fast_15_16_lerp_prediff(*(leftIndex + 1), morphFrac);
	int32_t sample2 = fast_15_16_lerp_prediff(*(leftIndex + 2), morphFrac);
	int32_t sample3 = fast_15_16_lerp_prediff(*(leftIndex + 3), morphFrac);
	int32_t sample4 = fast_15_16_lerp_prediff(*(leftIndex + 4), morphFrac);
	int32_t sample5 = fast_15_16_lerp_prediff(*(leftIndex + 5), morphFrac);

	// a version of the spline forumula from Josh Scholar on the musicdsp mailing list
	// https://web.archive.org/web/20170705065209/http://www.musicdsp.org/showArchiveComment.php?ArchiveID=60

	int32_t out = (sample2 + fix24_mul(699051, fix16_mul(phaseFrac, ((sample3-sample1)*16 + (sample0-sample4)*2
					+ fix16_mul(phaseFrac, ((sample3+sample1)*16 - sample0 - sample2*30 - sample4
							+ fix16_mul(phaseFrac, (sample3*66 - sample2*70 - sample4*33 + sample1*39 + sample5*7 - sample0*9
									+ fix16_mul(phaseFrac, ( sample2*126 - sample3*124 + sample4*61 - sample1*64 - sample5*12 + sample0*13
											+ fix16_mul(phaseFrac, ((sample3-sample2)*50 + (sample1-sample4)*25 + (sample5-sample0) * 5
											))
									))
							))
					))
			))
		));

	*delta = ((uint32_t) (sample3 - sample2)) >> 31;

	return __USAT(out, 15);
}

static inline int32_t getSampleQuinticSplineDeltaValue(uint32_t phase, uint32_t morph,
		uint32_t *fullTableHoldArray, int32_t *delta, uint32_t interpOff) {

	/* in this function, we use our phase position to get the sample to give to our dacs using a quintic spline interpolation technique
	 essentially, we need to get 6 pairs of sample values and two "fractional arguments" (where are we at in between those sample values)
	 one fractional argument determines the relative position in a linear interpolation between the sample values (morph)
	 the other is the fractional phase argument used in the interpolation calculation
	 */

	uint32_t LnSample; // indicates the left most sample in the neighborhood of sample values around the phase pointer
	uint32_t LnFamily; // indicates the nearest neighbor (wavetable) to our morph value in the family
	uint32_t phaseFrac; // indicates the fractional distance between the nearest sample values in terms of phase
	uint32_t morphFrac; // indicates the fractional distance between our nearest neighbors in the family
	uint32_t * leftIndex;

	// we do a lot of tricky bitshifting to take advantage of the structure of a 16 bit fixed point number
	// truncate phase then add two to find the left neighboring sample of the phase pointer

	LnSample = phase >> 16;

	// bit shifting to divide by the correct power of two takes a 12 bit number (our morph) and returns the a quotient in the range of our family size

	LnFamily = morph >> 16;

	leftIndex = (fullTableHoldArray + LnFamily * 517) + LnSample;

	// determine the fractional part of our phase phase by masking off the integer

	phaseFrac = (0xFFFF & phase);

	// we have to calculate the fractional portion and get it up to full scale q16

	morphFrac = (morph & 0xFFFF);

	// perform the 6 linear interpolations to get the sample values and apply morph
	// TODO track delta change in the phase event array

	int32_t sample0 = fast_15_16_lerp_prediff(*leftIndex, morphFrac);
	int32_t sample1 = fast_15_16_lerp_prediff(*(leftIndex + 1), morphFrac);
	int32_t sample2 = fast_15_16_lerp_prediff(*(leftIndex + 2), morphFrac);
	int32_t sample3 = fast_15_16_lerp_prediff(*(leftIndex + 3), morphFrac);
	int32_t sample4 = fast_15_16_lerp_prediff(*(leftIndex + 4), morphFrac);
	int32_t sample5 = fast_15_16_lerp_prediff(*(leftIndex + 5), morphFrac);

	// a version of the spline forumula from Josh Scholar on the musicdsp mailing list
	// https://web.archive.org/web/20170705065209/http://www.musicdsp.org/showArchiveComment.php?ArchiveID=60

	int32_t out = (sample2 + fix24_mul(699051, fix16_mul(phaseFrac, ((sample3-sample1)*16 + (sample0-sample4)*2
					+ fix16_mul(phaseFrac, ((sample3+sample1)*16 - sample0 - sample2*30 - sample4
							+ fix16_mul(phaseFrac, (sample3*66 - sample2*70 - sample4*33 + sample1*39 + sample5*7 - sample0*9
									+ fix16_mul(phaseFrac, ( sample2*126 - sample3*124 + sample4*61 - sample1*64 - sample5*12 + sample0*13
											+ fix16_mul(phaseFrac, ((sample3-sample2)*50 + (sample1-sample4)*25 + (sample5-sample0) * 5
											))
									))
							))
					))
			))
		));

	out += (sample2 - out) * interpOff;

	*delta = (sample3 - sample2);

	return __USAT(out, 15);
}


/*
 *
 * Fold/wrap
 *
 */

static inline int32_t foldSignal16Bit(int32_t phaseIn) {

	// fold into the space [0, 2^16)
	// check the lsb of abs(input) >> 15 (is phaseIn / 2^15 even or odd)
	// mathematical conditional that adds the modulo 2^15 for odd and the inversion of that in 15 bit space for even

	return ((phaseIn >> 16) & 1) ?
			(65535 - (phaseIn & 0xFFFF)) : (phaseIn & 0xFFFF);

}

static inline uint32_t foldSignal25Bit(uint32_t phaseIn) {

	// fold into the space [0, 2^25)
	// check the lsb of abs(input) >> 15 (is phaseIn / 2^15 even or odd)
	// mathematical conditional that adds the modulo 2^15 for odd and the inversion of that in 15 bit space for even

//	return ((phaseIn >> 25) & 1) ?
//			(0x1FFFFFF - (phaseIn & 0x1FFFFFF)) : (phaseIn & 0x1FFFFFF);

	return __USAT((abs((int32_t) phaseIn << 6) - 1) >> 6, 25);

}

static inline int32_t wrapSignal16Bit(int32_t phaseIn) {

	// wrap into the space [0, 2^15) by calculating the input modulo 2^15

	return phaseIn & 0xFFFF;

}

/*
 *
 * Odball notes
 *
 *
 */

static inline int32_t wavetableDelta(int32_t in0, int32_t in1, int32_t frac0) {

	// subtract both the difference and the base sample from the left sample with those of the right sample
	in0 = __QSUB(in1, in0);
	// pack a 1 in the bottom 16 bits and the fractional interpolation in the top
	frac0 = __PKHBT(1, frac0, 16);
	// multiply halfwords and accumulate
	return __SMLAD(frac0, in0, 0) >> 31;

}

#ifdef __cplusplus
}
#endif

#endif

#ifdef BUILD_VIRTUAL

#include <cstdint>
//#include <algorithm>
typedef struct {
	int32_t buff[32];
	int32_t writeIndex;
} buffer;

typedef struct {
	int32_t buff[256];
	int32_t writeIndex;
} longBuffer;

static inline void writeBuffer(buffer* buffer, int32_t value) {
	buffer->buff[(buffer->writeIndex++) & 31] = value;
}

static inline int32_t readBuffer(buffer* buffer, int32_t Xn) {
	return buffer->buff[(buffer->writeIndex + (~Xn)) & 31];
}

static inline void writeLongBuffer(longBuffer* buffer, int32_t value) {
	buffer->buff[(buffer->writeIndex++) & 255] = value;
}

static inline int32_t readLongBuffer(longBuffer* buffer, int32_t Xn) {
	return buffer->buff[(buffer->writeIndex + (~Xn)) & 255];
}

static inline int32_t __USAT(int32_t X, uint32_t Y) {

	if (X > ((1 << Y) - 1)) {
		return ((1 << Y) - 1);
	} else if (X < 0) {
		return 0;
	} else {
		return X;
	}

}

static inline int32_t __SSAT(int32_t X, uint32_t Y) {
	
#define __MAX ((1 << (Y - 1)) - 1)

	if (X > __MAX) {
		return __MAX;
	} else if (X < -__MAX) {
		return -__MAX;
	} else {
		return X;
	}

}

inline int32_t int32_abs(int32_t n) {
  int32_t mask = n >> 31;
  return ((n + mask) ^ mask);
}

static inline int32_t fix16_mul(int32_t in0, int32_t in1) {
	int64_t result = (uint64_t) in0 * in1;
	return result >> 16;
}

static inline int32_t fix24_mul(int32_t in0, int32_t in1) {
	int64_t result = (uint64_t) in0 * in1;
	return result >> 24;
}

static inline int32_t fix48_mul(uint32_t in0, uint32_t in1) {
	// taken from the fixmathlib library
	int64_t result = (uint64_t) in0 * in1;
	return result >> 48;
}

static inline int32_t fix16_lerp(int32_t in0, int32_t in1, int32_t frac) {
	return in0 + fix16_mul(in1 - in0, frac);
}

// in0 and in1 can be full scale 32 bit, frac max is 15 bits (!!!)

static inline int32_t fast_fix15_lerp(int32_t in0, int32_t in1, int32_t frac) {
	return in0 + fix16_mul(in1 - in0, frac << 1);
}

static inline int32_t fast_15_16_lerp(int32_t in0, int32_t in1, int32_t frac) {

	return in0 + (((in1 - in0) * frac) >> 16);
}

// kludged to hard code a half word swap
static inline uint32_t __ROR(uint32_t in, uint32_t rotate) {

	return (in >> 16) | ((in & 0xFFFF) << 16);

}

// same value ranges

// lower quality but faster?
static inline int32_t fix15_bilerp(int32_t in0, int32_t in1, int32_t in2, int32_t in3, int32_t frac0,
		int32_t frac1) {

	in0 = fast_fix15_lerp(in0, in1, frac0);
	in2 = fast_fix15_lerp(in2, in3, frac0);

	return fast_fix15_lerp(in0, in2, frac1);
}

static inline int32_t fast_15_16_lerp_prediff(int32_t in0, int32_t frac) {

	return (in0 & 0xFFFF) + (((in0 >> 16)*frac) >> 16);
}

// assume in0 and in have the base value and the difference to the next point packed in bottom and top word
// frac0 and frac1 can be up to 16 bits

static inline int32_t fast_15_16_bilerp_prediff(int32_t in0, int32_t in1, int32_t frac0,
		int32_t frac1) {

	in0 = (in0 & 0xFFFF) + (((in0 >> 16)*frac0) >> 16);

	in1 = (in1 & 0xFFFF) + (((in1 >> 16)*frac0) >> 16);

	return in0 + ((in1 - in0)*frac1 >> 16); 

}

static inline int32_t fast_15_16_bilerp_prediff_delta(int32_t in0, int32_t in1, int32_t frac0,
		int32_t frac1, int32_t * delta) {

	in0 = (in0 & 0xFFFF) + (((in0 >> 16)*frac0) >> 16);

	in1 = (in1 & 0xFFFF) + (((in1 >> 16)*frac0) >> 16);

	*delta = ((uint32_t) in1 - in0) >> 31;

	return in0 + ((in1 - in0)*frac1 >> 16); 

}

static inline int32_t fast_15_16_bilerp_prediff_deltaValue(int32_t in0, int32_t in1, int32_t frac0,
		int32_t frac1, int32_t * delta) {

	in0 = (in0 & 0xFFFF) + (((in0 >> 16)*frac0) >> 16);

	in1 = (in1 & 0xFFFF) + (((in1 >> 16)*frac0) >> 16);

	*delta = in1 - in0;

	return in0 + ((in1 - in0)*frac1 >> 16); 

}


// overkill probably, assumes 9 tables and 517 samples padded with two at the start and end of each waveform
// this is fortunately the default table load
// morph should have a max value of the table size

static inline int32_t getSampleQuinticSpline(uint32_t phase, uint32_t morph,
		uint32_t *fullTableHoldArray, int32_t *delta) {

	/* in this function, we use our phase position to get the sample to give to our dacs using a quintic spline interpolation technique
	 essentially, we need to get 6 pairs of sample values and two "fractional arguments" (where are we at in between those sample values)
	 one fractional argument determines the relative position in a linear interpolation between the sample values (morph)
	 the other is the fractional phase argument used in the interpolation calculation
	 */

	uint32_t LnSample; // indicates the left most sample in the neighborhood of sample values around the phase pointer
	uint32_t LnFamily; // indicates the nearest neighbor (wavetable) to our morph value in the family
	uint32_t phaseFrac; // indicates the fractional distance between the nearest sample values in terms of phase
	uint32_t morphFrac; // indicates the fractional distance between our nearest neighbors in the family
	uint32_t * leftIndex;

	// we do a lot of tricky bitshifting to take advantage of the structure of a 16 bit fixed point number
	// truncate phase then add two to find the left neighboring sample of the phase pointer

	LnSample = phase >> 16;

	// bit shifting to divide by the correct power of two takes a 12 bit number (our morph) and returns the a quotient in the range of our family size

	LnFamily = morph >> 16;

	leftIndex = (fullTableHoldArray + LnFamily * 517) + LnSample;

	// determine the fractional part of our phase phase by masking off the integer

	phaseFrac = 0xFFFF & phase;

	// we have to calculate the fractional portion and get it up to full scale q16

	morphFrac = (morph & 0xFFFF);

	// perform the 6 linear interpolations to get the sample values and apply morph
	// TODO track delta change in the phase event array

	int32_t sample0 = fast_15_16_lerp_prediff(*leftIndex, morphFrac);
	int32_t sample1 = fast_15_16_lerp_prediff(*(leftIndex + 1), morphFrac);
	int32_t sample2 = fast_15_16_lerp_prediff(*(leftIndex + 2), morphFrac);
	int32_t sample3 = fast_15_16_lerp_prediff(*(leftIndex + 3), morphFrac);
	int32_t sample4 = fast_15_16_lerp_prediff(*(leftIndex + 4), morphFrac);
	int32_t sample5 = fast_15_16_lerp_prediff(*(leftIndex + 5), morphFrac);

	// a version of the spline forumula from Josh Scholar on the musicdsp mailing list
	// https://web.archive.org/web/20170705065209/http://www.musicdsp.org/showArchiveComment.php?ArchiveID=60

	int32_t out = (sample2 + fix24_mul(699051, fix16_mul(phaseFrac, ((sample3-sample1)*16 + (sample0-sample4)*2
					+ fix16_mul(phaseFrac, ((sample3+sample1)*16 - sample0 - sample2*30 - sample4
							+ fix16_mul(phaseFrac, (sample3*66 - sample2*70 - sample4*33 + sample1*39 + sample5*7 - sample0*9
									+ fix16_mul(phaseFrac, ( sample2*126 - sample3*124 + sample4*61 - sample1*64 - sample5*12 + sample0*13
											+ fix16_mul(phaseFrac, ((sample3-sample2)*50 + (sample1-sample4)*25 + (sample5-sample0) * 5
											))
									))
							))
					))
			))
		));

	*delta = ((uint32_t) (sample3 - sample2)) >> 31;

	return __USAT(out, 15);
}

static inline int32_t getSampleQuinticSplineDeltaValue(uint32_t phase, uint32_t morph,
		uint32_t *fullTableHoldArray, int32_t *delta, uint32_t interpOff) {

	/* in this function, we use our phase position to get the sample to give to our dacs using a quintic spline interpolation technique
	 essentially, we need to get 6 pairs of sample values and two "fractional arguments" (where are we at in between those sample values)
	 one fractional argument determines the relative position in a linear interpolation between the sample values (morph)
	 the other is the fractional phase argument used in the interpolation calculation
	 */

	uint32_t LnSample; // indicates the left most sample in the neighborhood of sample values around the phase pointer
	uint32_t LnFamily; // indicates the nearest neighbor (wavetable) to our morph value in the family
	uint32_t phaseFrac; // indicates the fractional distance between the nearest sample values in terms of phase
	uint32_t morphFrac; // indicates the fractional distance between our nearest neighbors in the family
	uint32_t * leftIndex;

	// we do a lot of tricky bitshifting to take advantage of the structure of a 16 bit fixed point number
	// truncate phase then add two to find the left neighboring sample of the phase pointer

	LnSample = phase >> 16;

	// bit shifting to divide by the correct power of two takes a 12 bit number (our morph) and returns the a quotient in the range of our family size

	LnFamily = morph >> 16;

	leftIndex = (fullTableHoldArray + LnFamily * 517) + LnSample;

	// determine the fractional part of our phase phase by masking off the integer

	phaseFrac = (0xFFFF & phase);

	// we have to calculate the fractional portion and get it up to full scale q16

	morphFrac = (morph & 0xFFFF);

	// perform the 6 linear interpolations to get the sample values and apply morph
	// TODO track delta change in the phase event array

	int32_t sample0 = fast_15_16_lerp_prediff(*leftIndex, morphFrac);
	int32_t sample1 = fast_15_16_lerp_prediff(*(leftIndex + 1), morphFrac);
	int32_t sample2 = fast_15_16_lerp_prediff(*(leftIndex + 2), morphFrac);
	int32_t sample3 = fast_15_16_lerp_prediff(*(leftIndex + 3), morphFrac);
	int32_t sample4 = fast_15_16_lerp_prediff(*(leftIndex + 4), morphFrac);
	int32_t sample5 = fast_15_16_lerp_prediff(*(leftIndex + 5), morphFrac);

	// a version of the spline forumula from Josh Scholar on the musicdsp mailing list
	// https://web.archive.org/web/20170705065209/http://www.musicdsp.org/showArchiveComment.php?ArchiveID=60

	int32_t out = (sample2 + fix24_mul(699051, fix16_mul(phaseFrac, ((sample3-sample1)*16 + (sample0-sample4)*2
					+ fix16_mul(phaseFrac, ((sample3+sample1)*16 - sample0 - sample2*30 - sample4
							+ fix16_mul(phaseFrac, (sample3*66 - sample2*70 - sample4*33 + sample1*39 + sample5*7 - sample0*9
									+ fix16_mul(phaseFrac, ( sample2*126 - sample3*124 + sample4*61 - sample1*64 - sample5*12 + sample0*13
											+ fix16_mul(phaseFrac, ((sample3-sample2)*50 + (sample1-sample4)*25 + (sample5-sample0) * 5
											))
									))
							))
					))
			))
		));

	out += (sample2 - out) * interpOff;

	*delta = (sample3 - sample2);

	return __USAT(out, 15);
}


/*
 *
 * Fold/wrap
 *
 */

static inline int32_t foldSignal16Bit(int32_t phaseIn) {

	// fold into the space [0, 2^16)
	// check the lsb of abs(input) >> 15 (is phaseIn / 2^15 even or odd)
	// mathematical conditional that adds the modulo 2^15 for odd and the inversion of that in 15 bit space for even

	return ((phaseIn >> 16) & 1) ?
			(65535 - (phaseIn & 0xFFFF)) : (phaseIn & 0xFFFF);

}


static inline int32_t foldSignal25Bit(int32_t phaseIn) {

	// fold into the space [0, 2^25)
	// check the lsb of abs(input) >> 15 (is phaseIn / 2^15 even or odd)
	// mathematical conditional that adds the modulo 2^15 for odd and the inversion of that in 15 bit space for even

	return ((phaseIn >> 25) & 1) ?
			(0x1FFFFFF - (phaseIn & 0x1FFFFFF)) : (phaseIn & 0x1FFFFFF);

}

static inline int32_t wrapSignal16Bit(int32_t phaseIn) {

	// wrap into the space [0, 2^15) by calculating the input modulo 2^15

	return phaseIn & 0xFFFF;

}

#endif

/**
 *
 * Expo lookup table for 1v/oct
 *
 */

#define expotable10oct {65536, 65654, 65773, 65891, 66010, 66130, 66249, 66369, 66489, 66609, 66729, 66850, 66971, 67092, 67213, 67334, 67456, 67578, 67700, 67822, 67945, 68067, 68190, 68314, 68437, 68561, 68685, 68809, 68933, 69057, 69182, 69307, 69432, 69558, 69684, 69809, 69936, 70062, 70189, 70315, 70442, 70570, 70697, 70825, 70953, 71081, 71209, 71338, 71467, 71596, 71725, 71855, 71985, 72115, 72245, 72376, 72507, 72638, 72769, 72900, 73032, 73164, 73296, 73429, 73561, 73694, 73827, 73961, 74094, 74228, 74362, 74497, 74631, 74766, 74901, 75036, 75172, 75308, 75444, 75580, 75717, 75853, 75991, 76128, 76265, 76403, 76541, 76679, 76818, 76957, 77096, 77235, 77375, 77514, 77655, 77795, 77935, 78076, 78217, 78359, 78500, 78642, 78784, 78926, 79069, 79212, 79355, 79498, 79642, 79786, 79930, 80074, 80219, 80364, 80509, 80655, 80800, 80946, 81093, 81239, 81386, 81533, 81680, 81828, 81976, 82124, 82272, 82421, 82570, 82719, 82868, 83018, 83168, 83318, 83469, 83620, 83771, 83922, 84074, 84226, 84378, 84530, 84683, 84836, 84989, 85143, 85297, 85451, 85605, 85760, 85915, 86070, 86225, 86381, 86537, 86694, 86850, 87007, 87164, 87322, 87480, 87638, 87796, 87955, 88113, 88273, 88432, 88592, 88752, 88912, 89073, 89234, 89395, 89557, 89718, 89881, 90043, 90206, 90369, 90532, 90695, 90859, 91023, 91188, 91353, 91518, 91683, 91849, 92015, 92181, 92347, 92514, 92681, 92849, 93017, 93185, 93353, 93522, 93691, 93860, 94029, 94199, 94370, 94540, 94711, 94882, 95053, 95225, 95397, 95570, 95742, 95915, 96088, 96262, 96436, 96610, 96785, 96960, 97135, 97310, 97486, 97662, 97839, 98015, 98193, 98370, 98548, 98726, 98904, 99083, 99262, 99441, 99621, 99801, 99981, 100162, 100343, 100524, 100706, 100888, 101070, 101252, 101435, 101619, 101802, 101986, 102170, 102355, 102540, 102725, 102911, 103097, 103283, 103470, 103657, 103844, 104031, 104219, 104408, 104596, 104785, 104975, 105164, 105354, 105545, 105735, 105926, 106118, 106309, 106501, 106694, 106887, 107080, 107273, 107467, 107661, 107856, 108051, 108246, 108441, 108637, 108834, 109030, 109227, 109425, 109622, 109820, 110019, 110217, 110417, 110616, 110816, 111016, 111217, 111418, 111619, 111821, 112023, 112225, 112428, 112631, 112834, 113038, 113243, 113447, 113652, 113857, 114063, 114269, 114476, 114682, 114890, 115097, 115305, 115514, 115722, 115931, 116141, 116351, 116561, 116771, 116982, 117194, 117405, 117618, 117830, 118043, 118256, 118470, 118684, 118898, 119113, 119328, 119544, 119760, 119976, 120193, 120410, 120628, 120846, 121064, 121283, 121502, 121721, 121941, 122162, 122382, 122603, 122825, 123047, 123269, 123492, 123715, 123939, 124162, 124387, 124611, 124837, 125062, 125288, 125514, 125741, 125968, 126196, 126424, 126652, 126881, 127110, 127340, 127570, 127801, 128032, 128263, 128495, 128727, 128959, 129192, 129426, 129660, 129894, 130129, 130364, 130599, 130835, 131072, 131308, 131546, 131783, 132021, 132260, 132499, 132738, 132978, 133218, 133459, 133700, 133942, 134184, 134426, 134669, 134912, 135156, 135400, 135645, 135890, 136135, 136381, 136628, 136875, 137122, 137370, 137618, 137866, 138115, 138365, 138615, 138865, 139116, 139368, 139619, 139872, 140124, 140378, 140631, 140885, 141140, 141395, 141650, 141906, 142163, 142419, 142677, 142935, 143193, 143451, 143711, 143970, 144230, 144491, 144752, 145014, 145276, 145538, 145801, 146064, 146328, 146593, 146858, 147123, 147389, 147655, 147922, 148189, 148457, 148725, 148994, 149263, 149532, 149803, 150073, 150344, 150616, 150888, 151161, 151434, 151707, 151982, 152256, 152531, 152807, 153083, 153359, 153637, 153914, 154192, 154471, 154750, 155029, 155310, 155590, 155871, 156153, 156435, 156718, 157001, 157284, 157569, 157853, 158138, 158424, 158710, 158997, 159284, 159572, 159860, 160149, 160439, 160729, 161019, 161310, 161601, 161893, 162186, 162479, 162772, 163066, 163361, 163656, 163952, 164248, 164545, 164842, 165140, 165438, 165737, 166037, 166337, 166637, 166938, 167240, 167542, 167845, 168148, 168452, 168756, 169061, 169366, 169672, 169979, 170286, 170594, 170902, 171211, 171520, 171830, 172140, 172451, 172763, 173075, 173388, 173701, 174015, 174329, 174644, 174960, 175276, 175592, 175910, 176227, 176546, 176865, 177184, 177504, 177825, 178146, 178468, 178791, 179114, 179437, 179762, 180086, 180412, 180738, 181064, 181391, 181719, 182047, 182376, 182706, 183036, 183367, 183698, 184030, 184362, 184695, 185029, 185363, 185698, 186034, 186370, 186707, 187044, 187382, 187720, 188059, 188399, 188740, 189081, 189422, 189764, 190107, 190451, 190795, 191140, 191485, 191831, 192177, 192525, 192872, 193221, 193570, 193920, 194270, 194621, 194973, 195325, 195678, 196031, 196386, 196740, 197096, 197452, 197809, 198166, 198524, 198883, 199242, 199602, 199963, 200324, 200686, 201048, 201412, 201776, 202140, 202505, 202871, 203238, 203605, 203973, 204341, 204711, 205080, 205451, 205822, 206194, 206566, 206940, 207314, 207688, 208063, 208439, 208816, 209193, 209571, 209950, 210329, 210709, 211090, 211471, 211853, 212236, 212619, 213003, 213388, 213774, 214160, 214547, 214935, 215323, 215712, 216102, 216492, 216883, 217275, 217668, 218061, 218455, 218850, 219245, 219641, 220038, 220435, 220834, 221233, 221632, 222033, 222434, 222836, 223238, 223642, 224046, 224451, 224856, 225262, 225669, 226077, 226486, 226895, 227305, 227715, 228127, 228539, 228952, 229365, 229780, 230195, 230611, 231028, 231445, 231863, 232282, 232702, 233122, 233543, 233965, 234388, 234811, 235236, 235661, 236086, 236513, 236940, 237368, 237797, 238227, 238657, 239088, 239520, 239953, 240387, 240821, 241256, 241692, 242129, 242566, 243004, 243443, 243883, 244324, 244765, 245207, 245650, 246094, 246539, 246984, 247430, 247878, 248325, 248774, 249223, 249674, 250125, 250577, 251029, 251483, 251937, 252393, 252849, 253305, 253763, 254221, 254681, 255141, 255602, 256064, 256526, 256990, 257454, 257919, 258385, 258852, 259320, 259788, 260258, 260728, 261199, 261671, 262144, 262617, 263092, 263567, 264043, 264520, 264998, 265477, 265956, 266437, 266918, 267401, 267884, 268368, 268853, 269338, 269825, 270312, 270801, 271290, 271780, 272271, 272763, 273256, 273750, 274244, 274740, 275236, 275733, 276231, 276731, 277231, 277731, 278233, 278736, 279239, 279744, 280249, 280756, 281263, 281771, 282280, 282790, 283301, 283813, 284326, 284839, 285354, 285870, 286386, 286903, 287422, 287941, 288461, 288982, 289505, 290028, 290552, 291077, 291602, 292129, 292657, 293186, 293716, 294246, 294778, 295310, 295844, 296378, 296914, 297450, 297988, 298526, 299065, 299606, 300147, 300689, 301233, 301777, 302322, 302868, 303415, 303964, 304513, 305063, 305614, 306166, 306719, 307274, 307829, 308385, 308942, 309500, 310059, 310620, 311181, 311743, 312306, 312870, 313436, 314002, 314569, 315138, 315707, 316277, 316849, 317421, 317995, 318569, 319145, 319721, 320299, 320878, 321458, 322038, 322620, 323203, 323787, 324372, 324958, 325545, 326133, 326722, 327313, 327904, 328497, 329090, 329685, 330280, 330877, 331475, 332074, 332674, 333275, 333877, 334480, 335084, 335690, 336296, 336904, 337512, 338122, 338733, 339345, 339958, 340572, 341188, 341804, 342422, 343040, 343660, 344281, 344903, 345526, 346150, 346776, 347402, 348030, 348659, 349289, 349920, 350552, 351185, 351820, 352455, 353092, 353730, 354369, 355009, 355651, 356293, 356937, 357582, 358228, 358875, 359524, 360173, 360824, 361476, 362129, 362783, 363439, 364095, 364753, 365412, 366072, 366734, 367396, 368060, 368725, 369391, 370059, 370727, 371397, 372068, 372740, 373414, 374088, 374764, 375441, 376119, 376799, 377480, 378162, 378845, 379529, 380215, 380902, 381590, 382280, 382970, 383662, 384355, 385050, 385745, 386442, 387141, 387840, 388541, 389243, 389946, 390651, 391356, 392063, 392772, 393481, 394192, 394904, 395618, 396333, 397049, 397766, 398485, 399205, 399926, 400648, 401372, 402097, 402824, 403552, 404281, 405011, 405743, 406476, 407210, 407946, 408683, 409422, 410161, 410902, 411645, 412388, 413133, 413880, 414628, 415377, 416127, 416879, 417632, 418387, 419143, 419900, 420658, 421418, 422180, 422943, 423707, 424472, 425239, 426007, 426777, 427548, 428321, 429094, 429870, 430646, 431424, 432204, 432985, 433767, 434551, 435336, 436122, 436910, 437700, 438490, 439283, 440076, 440871, 441668, 442466, 443265, 444066, 444868, 445672, 446477, 447284, 448092, 448902, 449713, 450525, 451339, 452155, 452972, 453790, 454610, 455431, 456254, 457078, 457904, 458731, 459560, 460390, 461222, 462056, 462890, 463727, 464564, 465404, 466245, 467087, 467931, 468776, 469623, 470472, 471322, 472173, 473026, 473881, 474737, 475595, 476454, 477315, 478177, 479041, 479907, 480774, 481642, 482513, 483384, 484258, 485133, 486009, 486887, 487767, 488648, 489531, 490415, 491301, 492189, 493078, 493969, 494861, 495756, 496651, 497549, 498447, 499348, 500250, 501154, 502059, 502966, 503875, 504786, 505698, 506611, 507526, 508443, 509362, 510282, 511204, 512128, 513053, 513980, 514909, 515839, 516771, 517705, 518640, 519577, 520516, 521456, 522398, 523342, 524288, 525235, 526184, 527134, 528087, 529041, 529997, 530954, 531913, 532874, 533837, 534802, 535768, 536736, 537706, 538677, 539650, 540625, 541602, 542581, 543561, 544543, 545527, 546512, 547500, 548489, 549480, 550473, 551467, 552463, 553462, 554462, 555463, 556467, 557472, 558479, 559488, 560499, 561512, 562526, 563543, 564561, 565581, 566603, 567626, 568652, 569679, 570709, 571740, 572773, 573807, 574844, 575883, 576923, 577965, 579010, 580056, 581104, 582154, 583205, 584259, 585315, 586372, 587432, 588493, 589556, 590621, 591688, 592757, 593828, 594901, 595976, 597053, 598131, 599212, 600295, 601379, 602466, 603554, 604645, 605737, 606831, 607928, 609026, 610126, 611229, 612333, 613439, 614548, 615658, 616770, 617885, 619001, 620119, 621240, 622362, 623487, 624613, 625741, 626872, 628005, 629139, 630276, 631415, 632555, 633698, 634843, 635990, 637139, 638290, 639443, 640599, 641756, 642916, 644077, 645241, 646407, 647574, 648744, 649916, 651091, 652267, 653445, 654626, 655809, 656994, 658181, 659370, 660561, 661754, 662950, 664148, 665348, 666550, 667754, 668960, 670169, 671380, 672593, 673808, 675025, 676245, 677467, 678691, 679917, 681145, 682376, 683609, 684844, 686081, 687321, 688563, 689807, 691053, 692301, 693552, 694805, 696060, 697318, 698578, 699840, 701104, 702371, 703640, 704911, 706185, 707461, 708739, 710019, 711302, 712587, 713875, 715165, 716457, 717751, 719048, 720347, 721648, 722952, 724258, 725567, 726878, 728191, 729507, 730825, 732145, 733468, 734793, 736120, 737450, 738783, 740118, 741455, 742794, 744136, 745481, 746828, 748177, 749529, 750883, 752239, 753598, 754960, 756324, 757690, 759059, 760431, 761805, 763181, 764560, 765941, 767325, 768711, 770100, 771491, 772885, 774282, 775681, 777082, 778486, 779892, 781302, 782713, 784127, 785544, 786963, 788385, 789809, 791236, 792666, 794098, 795533, 796970, 798410, 799852, 801297, 802745, 804195, 805648, 807104, 808562, 810023, 811486, 812953, 814421, 815893, 817367, 818844, 820323, 821805, 823290, 824777, 826267, 827760, 829256, 830754, 832255, 833758, 835265, 836774, 838286, 839800, 841317, 842837, 844360, 845886, 847414, 848945, 850479, 852015, 853555, 855097, 856642, 858189, 859740, 861293, 862849, 864408, 865970, 867535, 869102, 870672, 872245, 873821, 875400, 876981, 878566, 880153, 881743, 883336, 884932, 886531, 888133, 889737, 891345, 892955, 894569, 896185, 897804, 899426, 901051, 902679, 904310, 905944, 907580, 909220, 910863, 912508, 914157, 915809, 917463, 919121, 920781, 922445, 924112, 925781, 927454, 929129, 930808, 932490, 934175, 935862, 937553, 939247, 940944, 942644, 944347, 946053, 947762, 949475, 951190, 952909, 954630, 956355, 958083, 959814, 961548, 963285, 965026, 966769, 968516, 970266, 972019, 973775, 975534, 977296, 979062, 980831, 982603, 984378, 986157, 987939, 989723, 991512, 993303, 995098, 996895, 998696, 1000501, 1002308, 1004119, 1005933, 1007751, 1009572, 1011396, 1013223, 1015053, 1016887, 1018725, 1020565, 1022409, 1024256, 1026107, 1027961, 1029818, 1031678, 1033542, 1035410, 1037280, 1039154, 1041032, 1042913, 1044797, 1046684, 1048576, 1050470, 1052368, 1054269, 1056174, 1058082, 1059994, 1061909, 1063827, 1065749, 1067675, 1069604, 1071536, 1073472, 1075412, 1077355, 1079301, 1081251, 1083205, 1085162, 1087122, 1089086, 1091054, 1093025, 1095000, 1096978, 1098960, 1100946, 1102935, 1104927, 1106924, 1108924, 1110927, 1112934, 1114945, 1116959, 1118977, 1120999, 1123024, 1125053, 1127086, 1129122, 1131162, 1133206, 1135253, 1137304, 1139359, 1141418, 1143480, 1145546, 1147615, 1149689, 1151766, 1153847, 1155931, 1158020, 1160112, 1162208, 1164308, 1166411, 1168519, 1170630, 1172745, 1174864, 1176986, 1179113, 1181243, 1183377, 1185515, 1187657, 1189803, 1191952, 1194106, 1196263, 1198425, 1200590, 1202759, 1204932, 1207109, 1209290, 1211475, 1213663, 1215856, 1218053, 1220253, 1222458, 1224667, 1226879, 1229096, 1231317, 1233541, 1235770, 1238002, 1240239, 1242480, 1244725, 1246974, 1249226, 1251483, 1253744, 1256010, 1258279, 1260552, 1262830, 1265111, 1267397, 1269687, 1271981, 1274279, 1276581, 1278887, 1281198, 1283513, 1285832, 1288155, 1290482, 1292814, 1295149, 1297489, 1299833, 1302182, 1304534, 1306891, 1309253, 1311618, 1313988, 1316362, 1318740, 1321122, 1323509, 1325901, 1328296, 1330696, 1333100, 1335509, 1337921, 1340339, 1342760, 1345186, 1347617, 1350051, 1352490, 1354934, 1357382, 1359834, 1362291, 1364752, 1367218, 1369688, 1372163, 1374642, 1377126, 1379614, 1382106, 1384603, 1387105, 1389611, 1392121, 1394637, 1397156, 1399681, 1402209, 1404743, 1407281, 1409823, 1412370, 1414922, 1417478, 1420039, 1422605, 1425175, 1427750, 1430330, 1432914, 1435503, 1438096, 1440694, 1443297, 1445905, 1448517, 1451134, 1453756, 1456382, 1459014, 1461650, 1464290, 1466936, 1469586, 1472241, 1474901, 1477566, 1480236, 1482910, 1485589, 1488273, 1490962, 1493656, 1496354, 1499058, 1501766, 1504479, 1507197, 1509921, 1512649, 1515381, 1518119, 1520862, 1523610, 1526362, 1529120, 1531883, 1534650, 1537423, 1540201, 1542983, 1545771, 1548564, 1551362, 1554165, 1556973, 1559785, 1562604, 1565427, 1568255, 1571088, 1573927, 1576770, 1579619, 1582473, 1585332, 1588196, 1591066, 1593940, 1596820, 1599705, 1602595, 1605491, 1608391, 1611297, 1614208, 1617125, 1620046, 1622973, 1625906, 1628843, 1631786, 1634734, 1637688, 1640646, 1643611, 1646580, 1649555, 1652535, 1655521, 1658512, 1661508, 1664510, 1667517, 1670530, 1673548, 1676572, 1679601, 1682635, 1685675, 1688721, 1691772, 1694829, 1697891, 1700958, 1704031, 1707110, 1710194, 1713284, 1716379, 1719480, 1722587, 1725699, 1728817, 1731940, 1735070, 1738204, 1741345, 1744491, 1747643, 1750800, 1753963, 1757132, 1760307, 1763487, 1766673, 1769865, 1773063, 1776266, 1779475, 1782690, 1785911, 1789138, 1792370, 1795608, 1798852, 1802102, 1805358, 1808620, 1811888, 1815161, 1818441, 1821726, 1825017, 1828315, 1831618, 1834927, 1838242, 1841563, 1844891, 1848224, 1851563, 1854908, 1858259, 1861617, 1864980, 1868350, 1871725, 1875107, 1878494, 1881888, 1885288, 1888695, 1892107, 1895525, 1898950, 1902381, 1905818, 1909261, 1912711, 1916166, 1919628, 1923096, 1926571, 1930052, 1933539, 1937032, 1940532, 1944038, 1947550, 1951068, 1954593, 1958125, 1961663, 1965207, 1968757, 1972314, 1975878, 1979447, 1983024, 1986606, 1990196, 1993791, 1997393, 2001002, 2004617, 2008239, 2011867, 2015502, 2019144, 2022792, 2026446, 2030107, 2033775, 2037450, 2041131, 2044818, 2048513, 2052214, 2055922, 2059636, 2063357, 2067085, 2070820, 2074561, 2078309, 2082064, 2085826, 2089594, 2093369, 2097152, 2100940, 2104736, 2108539, 2112348, 2116165, 2119988, 2123818, 2127655, 2131499, 2135350, 2139208, 2143073, 2146945, 2150824, 2154710, 2158603, 2162503, 2166410, 2170324, 2174245, 2178173, 2182108, 2186051, 2190000, 2193957, 2197921, 2201892, 2205870, 2209855, 2213848, 2217848, 2221855, 2225869, 2229890, 2233919, 2237955, 2241998, 2246049, 2250107, 2254172, 2258245, 2262325, 2266412, 2270507, 2274609, 2278719, 2282836, 2286960, 2291092, 2295231, 2299378, 2303532, 2307694, 2311863, 2316040, 2320225, 2324417, 2328616, 2332823, 2337038, 2341260, 2345490, 2349728, 2353973, 2358226, 2362487, 2366755, 2371031, 2375315, 2379606, 2383905, 2388212, 2392527, 2396850, 2401180, 2405518, 2409864, 2414218, 2418580, 2422950, 2427327, 2431713, 2436106, 2440507, 2444917, 2449334, 2453759, 2458192, 2462634, 2467083, 2471540, 2476005, 2480479, 2484960, 2489450, 2493948, 2498453, 2502967, 2507489, 2512020, 2516558, 2521105, 2525660, 2530223, 2534794, 2539374, 2543962, 2548558, 2553162, 2557775, 2562396, 2567026, 2571664, 2576310, 2580965, 2585628, 2590299, 2594979, 2599667, 2604364, 2609069, 2613783, 2618506, 2623236, 2627976, 2632724, 2637480, 2642245, 2647019, 2651802, 2656593, 2661392, 2666201, 2671018, 2675843, 2680678, 2685521, 2690373, 2695234, 2700103, 2704981, 2709868, 2714764, 2719669, 2724583, 2729505, 2734437, 2739377, 2744326, 2749284, 2754252, 2759228, 2764213, 2769207, 2774210, 2779222, 2784243, 2789274, 2794313, 2799362, 2804419, 2809486, 2814562, 2819647, 2824741, 2829845, 2834957, 2840079, 2845210, 2850351, 2855501, 2860660, 2865828, 2871006, 2876193, 2881389, 2886595, 2891810, 2897035, 2902269, 2907512, 2912765, 2918028, 2923300, 2928581, 2933873, 2939173, 2944483, 2949803, 2955133, 2960472, 2965820, 2971179, 2976547, 2981924, 2987312, 2992709, 2998116, 3003533, 3008959, 3014395, 3019842, 3025298, 3030763, 3036239, 3041725, 3047220, 3052725, 3058241, 3063766, 3069301, 3074847, 3080402, 3085967, 3091543, 3097128, 3102724, 3108330, 3113946, 3119571, 3125208, 3130854, 3136510, 3142177, 3147854, 3153541, 3159239, 3164947, 3170665, 3176393, 3182132, 3187881, 3193641, 3199411, 3205191, 3210982, 3216783, 3222595, 3228417, 3234250, 3240093, 3245947, 3251812, 3257687, 3263572, 3269469, 3275376, 3281293, 3287222, 3293161, 3299110, 3305071, 3311042, 3317024, 3323017, 3329021, 3335035, 3341061, 3347097, 3353144, 3359202, 3365271, 3371351, 3377443, 3383545, 3389658, 3395782, 3401917, 3408063, 3414220, 3420389, 3426569, 3432759, 3438961, 3445174, 3451399, 3457635, 3463881, 3470140, 3476409, 3482690, 3488982, 3495286, 3501601, 3507927, 3514265, 3520614, 3526975, 3533347, 3539731, 3546126, 3552533, 3558951, 3565381, 3571823, 3578276, 3584741, 3591217, 3597705, 3604205, 3610717, 3617241, 3623776, 3630323, 3636882, 3643453, 3650035, 3656630, 3663236, 3669855, 3676485, 3683127, 3689782, 3696448, 3703126, 3709817, 3716519, 3723234, 3729961, 3736700, 3743451, 3750214, 3756989, 3763777, 3770577, 3777390, 3784214, 3791051, 3797900, 3804762, 3811636, 3818523, 3825422, 3832333, 3839257, 3846193, 3853142, 3860104, 3867078, 3874064, 3881064, 3888076, 3895100, 3902137, 3909187, 3916250, 3923326, 3930414, 3937515, 3944629, 3951756, 3958895, 3966048, 3973213, 3980392, 3987583, 3994787, 4002005, 4009235, 4016479, 4023735, 4031005, 4038288, 4045584, 4052893, 4060215, 4067551, 4074900, 4082262, 4089637, 4097026, 4104428, 4111844, 4119273, 4126715, 4134171, 4141640, 4149123, 4156619, 4164129, 4171652, 4179189, 4186739, 4194304, 4201881, 4209473, 4217078, 4224697, 4232330, 4239976, 4247637, 4255311, 4262999, 4270701, 4278417, 4286147, 4293891, 4301648, 4309420, 4317206, 4325006, 4332820, 4340648, 4348490, 4356347, 4364217, 4372102, 4380001, 4387915, 4395842, 4403784, 4411741, 4419711, 4427696, 4435696, 4443710, 4451738, 4459781, 4467839, 4475911, 4483997, 4492099, 4500215, 4508345, 4516490, 4524650, 4532825, 4541014, 4549219, 4557438, 4565672, 4573920, 4582184, 4590463, 4598756, 4607065, 4615389, 4623727, 4632081, 4640450, 4648834, 4657233, 4665647, 4674076, 4682521, 4690981, 4699456, 4707947, 4716452, 4724974, 4733510, 4742062, 4750630, 4759213, 4767811, 4776425, 4785055, 4793700, 4802361, 4811037, 4819729, 4828437, 4837161, 4845900, 4854655, 4863426, 4872213, 4881015, 4889834, 4898668, 4907519, 4916385, 4925268, 4934166, 4943081, 4952011, 4960958, 4969921, 4978900, 4987896, 4996907, 5005935, 5014979, 5024040, 5033117, 5042210, 5051320, 5060446, 5069589, 5078748, 5087924, 5097116, 5106325, 5115551, 5124793, 5134052, 5143328, 5152620, 5161930, 5171256, 5180599, 5189958, 5199335, 5208729, 5218139, 5227567, 5237012, 5246473, 5255952, 5265448, 5274961, 5284491, 5294039, 5303604, 5313186, 5322785, 5332402, 5342036, 5351687, 5361356, 5371042, 5380746, 5390468, 5400207, 5409963, 5419737, 5429529, 5439339, 5449166, 5459011, 5468874, 5478755, 5488653, 5498569, 5508504, 5518456, 5528426, 5538414, 5548421, 5558445, 5568487, 5578548, 5588627, 5598724, 5608839, 5618972, 5629124, 5639294, 5649483, 5659690, 5669915, 5680159, 5690421, 5700702, 5711002, 5721320, 5731656, 5742012, 5752386, 5762779, 5773190, 5783621, 5794070, 5804538, 5815025, 5825531, 5836056, 5846600, 5857163, 5867746, 5878347, 5888967, 5899607, 5910266, 5920944, 5931641, 5942358, 5953094, 5963849, 5974624, 5985419, 5996232, 6007066, 6017919, 6028791, 6039684, 6050596, 6061527, 6072479, 6083450, 6094441, 6105451, 6116482, 6127533, 6138603, 6149694, 6160805, 6171935, 6183086, 6194257, 6205448, 6216660, 6227892, 6239143, 6250416, 6261708, 6273021, 6284355, 6295709, 6307083, 6318478, 6329894, 6341330, 6352787, 6364265, 6375763, 6387282, 6398822, 6410383, 6421964, 6433567, 6445190, 6456835, 6468501, 6480187, 6491895, 6503624, 6515374, 6527145, 6538938, 6550752, 6562587, 6574444, 6586322, 6598221, 6610142, 6622085, 6634049, 6646035, 6658042, 6670071, 6682122, 6694195, 6706289, 6718405, 6730543, 6742703, 6754886, 6767090, 6779316, 6791564, 6803834, 6816127, 6828441, 6840778, 6853138, 6865519, 6877923, 6890349, 6902798, 6915270, 6927763, 6940280, 6952819, 6965380, 6977965, 6990572, 7003202, 7015854, 7028530, 7041229, 7053950, 7066694, 7079462, 7092252, 7105066, 7117902, 7130762, 7143646, 7156552, 7169482, 7182435, 7195411, 7208411, 7221435, 7234482, 7247552, 7260646, 7273764, 7286906, 7300071, 7313260, 7326473, 7339710, 7352970, 7366255, 7379564, 7392896, 7406253, 7419634, 7433039, 7446468, 7459922, 7473400, 7486902, 7500428, 7513979, 7527555, 7541155, 7554780, 7568429, 7582103, 7595801, 7609525, 7623273, 7637046, 7650844, 7664666, 7678514, 7692387, 7706285, 7720208, 7734156, 7748129, 7762128, 7776152, 7790201, 7804275, 7818375, 7832501, 7846652, 7860828, 7875030, 7889258, 7903512, 7917791, 7932096, 7946427, 7960784, 7975167, 7989575, 8004010, 8018471, 8032958, 8047471, 8062011, 8076576, 8091168, 8105786, 8120431, 8135102, 8149800, 8164524, 8179275, 8194053, 8208857, 8223688, 8238546, 8253430, 8268342, 8283280, 8298246, 8313238, 8328258, 8343304, 8358378, 8373479, 8388608, 8403763, 8418946, 8434157, 8449395, 8464660, 8479953, 8495274, 8510623, 8525999, 8541403, 8556834, 8572294, 8587782, 8603297, 8618841, 8634412, 8650012, 8665640, 8681296, 8696981, 8712694, 8728435, 8744205, 8760003, 8775830, 8791685, 8807569, 8823482, 8839423, 8855393, 8871392, 8887420, 8903477, 8919563, 8935678, 8951822, 8967995, 8984198, 9000430, 9016691, 9032981, 9049301, 9065650, 9082029, 9098438, 9114876, 9131344, 9147841, 9164369, 9180926, 9197513, 9214130, 9230778, 9247455, 9264162, 9280900, 9297668, 9314466, 9331294, 9348153, 9365042, 9381962, 9398913, 9415894, 9432905, 9449948, 9467021, 9484125, 9501260, 9518426, 9535623, 9552851, 9570110, 9587400, 9604722, 9622075, 9639459, 9656875, 9674322, 9691800, 9709311, 9726852, 9744426, 9762031, 9779668, 9797337, 9815038, 9832771, 9850536, 9868333, 9886162, 9904023, 9921917, 9939843, 9957801, 9975792, 9993815, 10011871, 10029959, 10048081, 10066234, 10084421, 10102641, 10120893, 10139178, 10157497, 10175849, 10194233, 10212651, 10231102, 10249587, 10268105, 10286656, 10305241, 10323860, 10342512, 10361198, 10379917, 10398671, 10417458, 10436279, 10455134, 10474024, 10492947, 10511905, 10530897, 10549923, 10568983, 10588078, 10607208, 10626372, 10645571, 10664804, 10684072, 10703375, 10722713, 10742085, 10761493, 10780936, 10800414, 10819927, 10839475, 10859059, 10878678, 10898333, 10918023, 10937748, 10957510, 10977306, 10997139, 11017008, 11036912, 11056853, 11076829, 11096842, 11116890, 11136975, 11157096, 11177254, 11197448, 11217678, 11237945, 11258249, 11278589, 11298966, 11319380, 11339831, 11360318, 11380843, 11401405, 11422004, 11442640, 11463313, 11484024, 11504772, 11525558, 11546381, 11567242, 11588141, 11609077, 11630051, 11651063, 11672113, 11693201, 11714327, 11735492, 11756694, 11777935, 11799214, 11820532, 11841888, 11863283, 11884716, 11906188, 11927699, 11949249, 11970838, 11992465, 12014132, 12035838, 12057583, 12079368, 12101192, 12123055, 12144958, 12166900, 12188882, 12210903, 12232965, 12255066, 12277207, 12299389, 12321610, 12343871, 12366173, 12388515, 12410897, 12433320, 12455784, 12478287, 12500832, 12523417, 12546043, 12568710, 12591418, 12614167, 12636957, 12659788, 12682661, 12705575, 12728530, 12751526, 12774565, 12797644, 12820766, 12843929, 12867134, 12890381, 12913670, 12937002, 12960375, 12983790, 13007248, 13030748, 13054291, 13077876, 13101504, 13125175, 13148888, 13172644, 13196443, 13220285, 13244170, 13268098, 13292070, 13316085, 13340143, 13364244, 13388390, 13412579, 13436811, 13461087, 13485407, 13509772, 13534180, 13558632, 13583128, 13607669, 13632254, 13656883, 13681557, 13706276, 13731039, 13755847, 13780699, 13805597, 13830540, 13855527, 13880560, 13905638, 13930761, 13955930, 13981144, 14006404, 14031709, 14057061, 14082458, 14107900, 14133389, 14158924, 14184505, 14210132, 14235805, 14261525, 14287292, 14313104, 14338964, 14364870, 14390823, 14416823, 14442870, 14468964, 14495105, 14521293, 14547529, 14573812, 14600143, 14626521, 14652946, 14679420, 14705941, 14732510, 14759128, 14785793, 14812506, 14839268, 14866078, 14892937, 14919844, 14946800, 14973804, 15000857, 15027959, 15055110, 15082310, 15109560, 15136858, 15164206, 15191603, 15219050, 15246546, 15274092, 15301688, 15329333, 15357029, 15384774, 15412570, 15440416, 15468312, 15496259, 15524256, 15552304, 15580402, 15608551, 15636751, 15665002, 15693304, 15721657, 15750061, 15778517, 15807024, 15835583, 15864193, 15892855, 15921568, 15950334, 15979151, 16008021, 16036943, 16065917, 16094943, 16124022, 16153153, 16182337, 16211573, 16240863, 16270205, 16299601, 16329049, 16358551, 16388106, 16417714, 16447376, 16477092, 16506861, 16536684, 16566561, 16596492, 16626476, 16656516, 16686609, 16716757, 16746959, 16777216, 16807527, 16837893, 16868314, 16898790, 16929321, 16959907, 16990549, 17021246, 17051998, 17082806, 17113669, 17144589, 17175564, 17206595, 17237682, 17268825, 17300025, 17331281, 17362593, 17393962, 17425388, 17456871, 17488410, 17520006, 17551660, 17583370, 17615138, 17646964, 17678846, 17710787, 17742785, 17774841, 17806954, 17839126, 17871356, 17903645, 17935991, 17968396, 18000860, 18033382, 18065963, 18098602, 18131301, 18164059, 18196876, 18229752, 18262688, 18295683, 18328738, 18361853, 18395027, 18428261, 18461556, 18494910, 18528325, 18561800, 18595336, 18628932, 18662589, 18696307, 18730085, 18763925, 18797826, 18831788, 18865811, 18899896, 18934042, 18968251, 19002521, 19036852, 19071246, 19105702, 19140221, 19174801, 19209444, 19244150, 19278919, 19313750, 19348644, 19383601, 19418622, 19453705, 19488852, 19524063, 19559337, 19594675, 19630077, 19665542, 19701072, 19736666, 19772324, 19808047, 19843834, 19879686, 19915603, 19951584, 19987631, 20023742, 20059919, 20096162, 20132469, 20168843, 20205282, 20241787, 20278357, 20314994, 20351698, 20388467, 20425303, 20462205, 20499174, 20536210, 20573313, 20610483, 20647720, 20685024, 20722396, 20759835, 20797342, 20834916, 20872559, 20910269, 20948048, 20985895, 21023810, 21061794, 21099846, 21137967, 21176157, 21214416, 21252744, 21291142, 21329608, 21368144, 21406750, 21445426, 21484171, 21522987, 21561872, 21600828, 21639855, 21678951, 21718119, 21757357, 21796666, 21836046, 21875497, 21915020, 21954613, 21994279, 22034016, 22073825, 22113706, 22153659, 22193684, 22233781, 22273951, 22314193, 22354508, 22394896, 22435357, 22475891, 22516498, 22557179, 22597933, 22638760, 22679662, 22720637, 22761687, 22802810, 22844008, 22885280, 22926627, 22968049, 23009545, 23051117, 23092763, 23134485, 23176282, 23218155, 23260103, 23302127, 23344227, 23386403, 23428655, 23470984, 23513389, 23555870, 23598429, 23641064, 23683776, 23726566, 23769433, 23812377, 23855399, 23898498, 23941676, 23984931, 24028265, 24071677, 24115167, 24158736, 24202384, 24246110, 24289916, 24333800, 24377764, 24421807, 24465930, 24510133, 24554415, 24598778, 24643220, 24687743, 24732347, 24777031, 24821795, 24866641, 24911568, 24956575, 25001664, 25046835, 25092087, 25137421, 25182837, 25228335, 25273915, 25319577, 25365322, 25411150, 25457060, 25503053, 25549130, 25595289, 25641532, 25687859, 25734269, 25780763, 25827341, 25874004, 25920750, 25967581, 26014497, 26061497, 26108583, 26155753, 26203009, 26250350, 26297776, 26345288, 26392886, 26440571, 26488341, 26536197, 26584140, 26632170, 26680286, 26728489, 26776780, 26825158, 26873623, 26922175, 26970815, 27019544, 27068360, 27117264, 27166257, 27215338, 27264508, 27313767, 27363115, 27412552, 27462078, 27511694, 27561399, 27611195, 27661080, 27711055, 27761121, 27811277, 27861523, 27911861, 27962289, 28012809, 28063419, 28114122, 28164916, 28215801, 28266779, 28317848, 28369010, 28420264, 28471611, 28523051, 28574584, 28626209, 28677928, 28729741, 28781647, 28833647, 28885740, 28937928, 28990211, 29042587, 29095058, 29147625, 29200286, 29253042, 29305893, 29358840, 29411883, 29465021, 29518256, 29571586, 29625013, 29678537, 29732157, 29785874, 29839689, 29893600, 29947609, 30001715, 30055919, 30110221, 30164621, 30219120, 30273717, 30328412, 30383207, 30438100, 30493093, 30548185, 30603376, 30658667, 30714058, 30769549, 30825141, 30880832, 30936625, 30992518, 31048512, 31104608, 31160804, 31217103, 31273503, 31330005, 31386609, 31443315, 31500123, 31557035, 31614049, 31671166, 31728386, 31785710, 31843137, 31900668, 31958303, 32016042, 32073886, 32131834, 32189886, 32248044, 32306306, 32364674, 32423147, 32481726, 32540411, 32599202, 32658099, 32717102, 32776212, 32835429, 32894753, 32954184, 33013722, 33073368, 33133122, 33192984, 33252953, 33313032, 33373218, 33433514, 33493918, 33554432, 33615054, 33675787, 33736629, 33797581, 33858643, 33919815, 33981098, 34042492, 34103997, 34165612, 34227339, 34289178, 34351128, 34413190, 34475365, 34537651, 34600051, 34662563, 34725187, 34787925, 34850777, 34913742, 34976820, 35040013, 35103320, 35166741, 35230277, 35293928, 35357693, 35421574, 35485570, 35549682, 35613909, 35678253, 35742713, 35807290, 35871983, 35936793, 36001720, 36066764, 36131926, 36197205, 36262603, 36328119, 36393753, 36459505, 36525377, 36591367, 36657477, 36723706, 36790055, 36856523, 36923112, 36989821, 37056650, 37123601, 37190672, 37257864, 37325178, 37392614, 37460171, 37527850, 37595652, 37663576, 37731623, 37799793, 37868085, 37936502, 38005042, 38073705, 38142493, 38211405, 38280442, 38349603, 38418889, 38488301, 38557838, 38627500, 38697289, 38767203, 38837244, 38907411, 38977705, 39048126, 39118674, 39189350, 39260154, 39331085, 39402144, 39473332, 39544649, 39616094, 39687669, 39759372, 39831206, 39903169, 39975262, 40047485, 40119839, 40192324, 40264939, 40337686, 40410564, 40483574, 40556715, 40629989, 40703396, 40776934, 40850606, 40924411, 40998349, 41072421, 41146626, 41220966, 41295440, 41370049, 41444792, 41519670, 41594684, 41669833, 41745118, 41820539, 41896096, 41971790, 42047620, 42123588, 42199692, 42275935, 42352315, 42428833, 42505489, 42582284, 42659217, 42736289, 42813501, 42890852, 42968343, 43045974, 43123745, 43201657, 43279710, 43357903, 43436238, 43514714, 43593332, 43672092, 43750995, 43830040, 43909227, 43988558, 44068032, 44147650, 44227412, 44307318, 44387368, 44467562, 44547902, 44628387, 44709017, 44789793, 44870714, 44951782, 45032997, 45114358, 45195866, 45277521, 45359324, 45441275, 45523374, 45605621, 45688017, 45770561, 45853255, 45936098, 46019091, 46102234, 46185527, 46268970, 46352564, 46436310, 46520206, 46604254, 46688454, 46772806, 46857311, 46941968, 47026778, 47111741, 47196858, 47282129, 47367553, 47453132, 47538866, 47624754, 47710798, 47796997, 47883352, 47969863, 48056530, 48143354, 48230335, 48317472, 48404768, 48492221, 48579832, 48667601, 48755529, 48843615, 48931861, 49020266, 49108831, 49197556, 49286441, 49375487, 49464694, 49554062, 49643591, 49733282, 49823136, 49913151, 50003329, 50093670, 50184175, 50274842, 50365674, 50456670, 50547830, 50639155, 50730644, 50822300, 50914120, 51006107, 51098260, 51190579, 51283065, 51375718, 51468539, 51561527, 51654683, 51748008, 51841501, 51935163, 52028994, 52122995, 52217166, 52311507, 52406018, 52500700, 52595553, 52690577, 52785773, 52881142, 52976682, 53072395, 53168281, 53264340, 53360573, 53456979, 53553560, 53650316, 53747246, 53844351, 53941631, 54039088, 54136720, 54234529, 54332515, 54430677, 54529017, 54627535, 54726231, 54825104, 54924157, 55023389, 55122799, 55222390, 55322160, 55422111, 55522242, 55622554, 55723047, 55823722, 55924579, 56025618, 56126839, 56228244, 56329832, 56431603, 56533558, 56635697, 56738021, 56840529, 56943223, 57046103, 57149168, 57252419, 57355857, 57459482, 57563294, 57667294, 57771481, 57875857, 57980422, 58085175, 58190117, 58295250, 58400572, 58506084, 58611787, 58717681, 58823766, 58930043, 59036512, 59143173, 59250027, 59357075, 59464315, 59571749, 59679378, 59787200, 59895218, 60003431, 60111839, 60220443, 60329243, 60438240, 60547434, 60656825, 60766414, 60876201, 60986186, 61096370, 61206753, 61317335, 61428117, 61539099, 61650282, 61761665, 61873250, 61985037, 62097025, 62209216, 62321609, 62434206, 62547006, 62660010, 62773218, 62886630, 63000247, 63114070, 63228098, 63342332, 63456773, 63571420, 63686275, 63801337, 63916607, 64032085, 64147772, 64263668, 64379773, 64496088, 64612613, 64729349, 64846295, 64963453, 65080823, 65198404, 65316198, 65434205, 65552425, 65670859, 65789507, 65908368, 66027445, 66146737, 66266244, 66385968, 66505907, 66626064, 66746437, 66867028, 66987837, 67108864, 67230109, 67351574, 67473258, 67595162, 67717286, 67839631, 67962197, 68084984, 68207994, 68331225, 68454679, 68578356, 68702257, 68826381, 68950730, 69075303, 69200102, 69325126, 69450375, 69575851, 69701554, 69827484, 69953641, 70080027, 70206640, 70333483, 70460554, 70587856, 70715387, 70843148, 70971141, 71099364, 71227819, 71356507, 71485427, 71614580, 71743966, 71873586, 72003440, 72133528, 72263852, 72394411, 72525206, 72656238, 72787506, 72919011, 73050754, 73182735, 73314954, 73447412, 73580110, 73713047, 73846224, 73979642, 74113301, 74247202, 74381345, 74515729, 74650357, 74785228, 74920342, 75055701, 75191304, 75327152, 75463246, 75599586, 75736171, 75873004, 76010084, 76147411, 76284987, 76422811, 76560884, 76699207, 76837779, 76976602, 77115676, 77255001, 77394578, 77534407, 77674488, 77814823, 77955411, 78096253, 78237349, 78378701, 78520308, 78662170, 78804289, 78946665, 79089298, 79232189, 79375338, 79518745, 79662412, 79806338, 79950524, 80094971, 80239679, 80384648, 80529879, 80675372, 80821128, 80967148, 81113431, 81259979, 81406792, 81553869, 81701213, 81848822, 81996699, 82144842, 82293253, 82441933, 82590881, 82740098, 82889584, 83039341, 83189368, 83339667, 83490236, 83641078, 83792193, 83943580, 84095241, 84247176, 84399385, 84551870, 84704630, 84857666, 85010978, 85164568, 85318434, 85472579, 85627003, 85781705, 85936687, 86091949, 86247491, 86403315, 86559420, 86715807, 86872476, 87029429, 87186665, 87344185, 87501990, 87660080, 87818455, 87977117, 88136065, 88295301, 88454824, 88614636, 88774736, 88935125, 89095804, 89256774, 89418034, 89579586, 89741429, 89903565, 90065994, 90228716, 90391733, 90555043, 90718649, 90882551, 91046748, 91211243, 91376034, 91541123, 91706511, 91872197, 92038183, 92204468, 92371054, 92537941, 92705129, 92872620, 93040413, 93208509, 93376909, 93545613, 93714622, 93883936, 94053556, 94223483, 94393717, 94564258, 94735107, 94906265, 95077732, 95249509, 95421597, 95593995, 95766705, 95939727, 96113061, 96286709, 96460670, 96634945, 96809536, 96984442, 97159664, 97335202, 97511058, 97687231, 97863723, 98040533, 98217663, 98395113, 98572883, 98750975, 98929389, 99108124, 99287183, 99466565, 99646272, 99826303, 100006659, 100187341, 100368350, 100549685, 100731349, 100913340, 101095660, 101278310, 101461289, 101644600, 101828241, 102012214, 102196520, 102381158, 102566130, 102751437, 102937078, 103123054, 103309367, 103496016, 103683002, 103870327, 104057989, 104245991, 104434332, 104623014, 104812036, 105001400, 105191106, 105381155, 105571547, 105762284, 105953365, 106144791, 106144791}

#define revexpotable10oct {268435456, 267951348, 267468113, 266985749, 266504256, 266023631, 265543872, 265064979, 264586950, 264109782, 263633475, 263158028, 262683437, 262209703, 261736823, 261264795, 260793619, 260323293, 259853815, 259385183, 258917397, 258450454, 257984354, 257519094, 257054673, 256591089, 256128342, 255666429, 255205350, 254745101, 254285683, 253827094, 253369331, 252912394, 252456281, 252000991, 251546521, 251092872, 250640040, 250188025, 249736825, 249286439, 248836865, 248388102, 247940149, 247493003, 247046663, 246601129, 246156398, 245712469, 245269341, 244827012, 244385480, 243944745, 243504805, 243065658, 242627303, 242189738, 241752963, 241316975, 240881774, 240447358, 240013724, 239580874, 239148803, 238717512, 238286999, 237857262, 237428300, 237000111, 236572695, 236146050, 235720174, 235295066, 234870725, 234447149, 234024337, 233602288, 233181000, 232760471, 232340701, 231921688, 231503430, 231085927, 230669177, 230253178, 229837930, 229423430, 229009678, 228596673, 228184412, 227772894, 227362119, 226952084, 226542789, 226134232, 225726412, 225319328, 224912977, 224507359, 224102473, 223698317, 223294890, 222892191, 222490217, 222088969, 221688444, 221288642, 220889561, 220491199, 220093556, 219696630, 219300419, 218904924, 218510141, 218116071, 217722711, 217330060, 216938118, 216546883, 216156353, 215766527, 215377405, 214988984, 214601264, 214214243, 213827919, 213442293, 213057362, 212673125, 212289582, 211906730, 211524568, 211143095, 210762311, 210382213, 210002801, 209624073, 209246028, 208868665, 208491982, 208115979, 207740654, 207366005, 206992033, 206618734, 206246109, 205874156, 205502874, 205132261, 204762317, 204393040, 204024429, 203656483, 203289200, 202922579, 202556620, 202191321, 201826680, 201462698, 201099371, 200736700, 200374683, 200013319, 199652606, 199292544, 198933131, 198574367, 198216249, 197858778, 197501951, 197145767, 196790226, 196435326, 196081067, 195727446, 195374462, 195022116, 194670405, 194319328, 193968884, 193619072, 193269891, 192921340, 192573418, 192226122, 191879454, 191533410, 191187991, 190843194, 190499019, 190155465, 189812531, 189470215, 189128516, 188787434, 188446966, 188107113, 187767873, 187429244, 187091226, 186753818, 186417018, 186080826, 185745240, 185410259, 185075882, 184742108, 184408936, 184076366, 183744394, 183413022, 183082247, 182752069, 182422486, 182093497, 181765102, 181437299, 181110087, 180783466, 180457433, 180131989, 179807131, 179482859, 179159172, 178836069, 178513548, 178191609, 177870251, 177549472, 177229272, 176909649, 176590603, 176272131, 175954235, 175636911, 175320160, 175003980, 174688371, 174373330, 174058858, 173744953, 173431614, 173118840, 172806630, 172494983, 172183898, 171873375, 171563411, 171254006, 170945159, 170636869, 170329136, 170021957, 169715332, 169409260, 169103740, 168798771, 168494352, 168190483, 167887161, 167584386, 167282157, 166980473, 166679334, 166378737, 166078682, 165779169, 165480196, 165181762, 164883866, 164586507, 164289685, 163993398, 163697645, 163402426, 163107739, 162813584, 162519959, 162226863, 161934296, 161642257, 161350745, 161059758, 160769296, 160479358, 160189942, 159901049, 159612677, 159324824, 159037491, 158750676, 158464378, 158178597, 157893331, 157608579, 157324341, 157040616, 156757402, 156474699, 156192506, 155910822, 155629646, 155348976, 155068814, 154789156, 154510002, 154231353, 153953205, 153675559, 153398414, 153121769, 152845623, 152569974, 152294823, 152020168, 151746009, 151472343, 151199172, 150926493, 150654305, 150382609, 150111403, 149840685, 149570456, 149300714, 149031459, 148762690, 148494405, 148226603, 147959285, 147692449, 147426094, 147160220, 146894825, 146629908, 146365470, 146101508, 145838022, 145575012, 145312476, 145050413, 144788823, 144527704, 144267057, 144006880, 143747172, 143487932, 143229160, 142970854, 142713014, 142455639, 142198729, 141942282, 141686297, 141430774, 141175712, 140921109, 140666966, 140413281, 140160054, 139907283, 139654969, 139403109, 139151703, 138900751, 138650252, 138400204, 138150607, 137901460, 137652763, 137404514, 137156713, 136909359, 136662451, 136415988, 136169969, 135924395, 135679263, 135434573, 135190324, 134946516, 134703148, 134460219, 134217728, 133975674, 133734056, 133492874, 133252128, 133011815, 132771936, 132532489, 132293475, 132054891, 131816737, 131579014, 131341718, 131104851, 130868411, 130632397, 130396809, 130161646, 129926907, 129692591, 129458698, 129225227, 128992177, 128759547, 128527336, 128295544, 128064171, 127833214, 127602675, 127372550, 127142841, 126913547, 126684665, 126456197, 126228140, 126000495, 125773260, 125546436, 125320020, 125094012, 124868412, 124643219, 124418432, 124194051, 123970074, 123746501, 123523331, 123300564, 123078199, 122856234, 122634670, 122413506, 122192740, 121972372, 121752402, 121532829, 121313651, 121094869, 120876481, 120658487, 120440887, 120223679, 120006862, 119790437, 119574401, 119358756, 119143499, 118928631, 118714150, 118500055, 118286347, 118073025, 117860087, 117647533, 117435362, 117223574, 117012168, 116801144, 116590500, 116380235, 116170350, 115960844, 115751715, 115542963, 115334588, 115126589, 114918965, 114711715, 114504839, 114298336, 114092206, 113886447, 113681059, 113476042, 113271394, 113067116, 112863206, 112659664, 112456488, 112253679, 112051236, 111849158, 111647445, 111446095, 111245108, 111044484, 110844222, 110644321, 110444780, 110245599, 110046778, 109848315, 109650209, 109452462, 109255070, 109058035, 108861355, 108665030, 108469059, 108273441, 108078176, 107883263, 107688702, 107494492, 107300632, 107107121, 106913959, 106721146, 106528681, 106336562, 106144791, 105953365, 105762284, 105571547, 105381155, 105191106, 105001400, 104812036, 104623014, 104434332, 104245991, 104057989, 103870327, 103683002, 103496016, 103309367, 103123054, 102937078, 102751437, 102566130, 102381158, 102196520, 102012214, 101828241, 101644600, 101461289, 101278310, 101095660, 100913340, 100731349, 100549685, 100368350, 100187341, 100006659, 99826303, 99646272, 99466565, 99287183, 99108124, 98929389, 98750975, 98572883, 98395113, 98217663, 98040533, 97863723, 97687231, 97511058, 97335202, 97159664, 96984442, 96809536, 96634945, 96460670, 96286709, 96113061, 95939727, 95766705, 95593995, 95421597, 95249509, 95077732, 94906265, 94735107, 94564258, 94393717, 94223483, 94053556, 93883936, 93714622, 93545613, 93376909, 93208509, 93040413, 92872620, 92705129, 92537941, 92371054, 92204468, 92038183, 91872197, 91706511, 91541123, 91376034, 91211243, 91046748, 90882551, 90718649, 90555043, 90391733, 90228716, 90065994, 89903565, 89741429, 89579586, 89418034, 89256774, 89095804, 88935125, 88774736, 88614636, 88454824, 88295301, 88136065, 87977117, 87818455, 87660080, 87501990, 87344185, 87186665, 87029429, 86872476, 86715807, 86559420, 86403315, 86247491, 86091949, 85936687, 85781705, 85627003, 85472579, 85318434, 85164568, 85010978, 84857666, 84704630, 84551870, 84399385, 84247176, 84095241, 83943580, 83792193, 83641078, 83490236, 83339667, 83189368, 83039341, 82889584, 82740098, 82590881, 82441933, 82293253, 82144842, 81996699, 81848822, 81701213, 81553869, 81406792, 81259979, 81113431, 80967148, 80821128, 80675372, 80529879, 80384648, 80239679, 80094971, 79950524, 79806338, 79662412, 79518745, 79375338, 79232189, 79089298, 78946665, 78804289, 78662170, 78520308, 78378701, 78237349, 78096253, 77955411, 77814823, 77674488, 77534407, 77394578, 77255001, 77115676, 76976602, 76837779, 76699207, 76560884, 76422811, 76284987, 76147411, 76010084, 75873004, 75736171, 75599586, 75463246, 75327152, 75191304, 75055701, 74920342, 74785228, 74650357, 74515729, 74381345, 74247202, 74113301, 73979642, 73846224, 73713047, 73580110, 73447412, 73314954, 73182735, 73050754, 72919011, 72787506, 72656238, 72525206, 72394411, 72263852, 72133528, 72003440, 71873586, 71743966, 71614580, 71485427, 71356507, 71227819, 71099364, 70971141, 70843148, 70715387, 70587856, 70460554, 70333483, 70206640, 70080027, 69953641, 69827484, 69701554, 69575851, 69450375, 69325126, 69200102, 69075303, 68950730, 68826381, 68702257, 68578356, 68454679, 68331225, 68207994, 68084984, 67962197, 67839631, 67717286, 67595162, 67473258, 67351574, 67230109, 67108864, 66987837, 66867028, 66746437, 66626064, 66505907, 66385968, 66266244, 66146737, 66027445, 65908368, 65789507, 65670859, 65552425, 65434205, 65316198, 65198404, 65080823, 64963453, 64846295, 64729349, 64612613, 64496088, 64379773, 64263668, 64147772, 64032085, 63916607, 63801337, 63686275, 63571420, 63456773, 63342332, 63228098, 63114070, 63000247, 62886630, 62773218, 62660010, 62547006, 62434206, 62321609, 62209216, 62097025, 61985037, 61873250, 61761665, 61650282, 61539099, 61428117, 61317335, 61206753, 61096370, 60986186, 60876201, 60766414, 60656825, 60547434, 60438240, 60329243, 60220443, 60111839, 60003431, 59895218, 59787200, 59679378, 59571749, 59464315, 59357075, 59250027, 59143173, 59036512, 58930043, 58823766, 58717681, 58611787, 58506084, 58400572, 58295250, 58190117, 58085175, 57980422, 57875857, 57771481, 57667294, 57563294, 57459482, 57355857, 57252419, 57149168, 57046103, 56943223, 56840529, 56738021, 56635697, 56533558, 56431603, 56329832, 56228244, 56126839, 56025618, 55924579, 55823722, 55723047, 55622554, 55522242, 55422111, 55322160, 55222390, 55122799, 55023389, 54924157, 54825104, 54726231, 54627535, 54529017, 54430677, 54332515, 54234529, 54136720, 54039088, 53941631, 53844351, 53747246, 53650316, 53553560, 53456979, 53360573, 53264340, 53168281, 53072395, 52976682, 52881142, 52785773, 52690577, 52595553, 52500700, 52406018, 52311507, 52217166, 52122995, 52028994, 51935163, 51841501, 51748008, 51654683, 51561527, 51468539, 51375718, 51283065, 51190579, 51098260, 51006107, 50914120, 50822300, 50730644, 50639155, 50547830, 50456670, 50365674, 50274842, 50184175, 50093670, 50003329, 49913151, 49823136, 49733282, 49643591, 49554062, 49464694, 49375487, 49286441, 49197556, 49108831, 49020266, 48931861, 48843615, 48755529, 48667601, 48579832, 48492221, 48404768, 48317472, 48230335, 48143354, 48056530, 47969863, 47883352, 47796997, 47710798, 47624754, 47538866, 47453132, 47367553, 47282129, 47196858, 47111741, 47026778, 46941968, 46857311, 46772806, 46688454, 46604254, 46520206, 46436310, 46352564, 46268970, 46185527, 46102234, 46019091, 45936098, 45853255, 45770561, 45688017, 45605621, 45523374, 45441275, 45359324, 45277521, 45195866, 45114358, 45032997, 44951782, 44870714, 44789793, 44709017, 44628387, 44547902, 44467562, 44387368, 44307318, 44227412, 44147650, 44068032, 43988558, 43909227, 43830040, 43750995, 43672092, 43593332, 43514714, 43436238, 43357903, 43279710, 43201657, 43123745, 43045974, 42968343, 42890852, 42813501, 42736289, 42659217, 42582284, 42505489, 42428833, 42352315, 42275935, 42199692, 42123588, 42047620, 41971790, 41896096, 41820539, 41745118, 41669833, 41594684, 41519670, 41444792, 41370049, 41295440, 41220966, 41146626, 41072421, 40998349, 40924411, 40850606, 40776934, 40703396, 40629989, 40556715, 40483574, 40410564, 40337686, 40264939, 40192324, 40119839, 40047485, 39975262, 39903169, 39831206, 39759372, 39687669, 39616094, 39544649, 39473332, 39402144, 39331085, 39260154, 39189350, 39118674, 39048126, 38977705, 38907411, 38837244, 38767203, 38697289, 38627500, 38557838, 38488301, 38418889, 38349603, 38280442, 38211405, 38142493, 38073705, 38005042, 37936502, 37868085, 37799793, 37731623, 37663576, 37595652, 37527850, 37460171, 37392614, 37325178, 37257864, 37190672, 37123601, 37056650, 36989821, 36923112, 36856523, 36790055, 36723706, 36657477, 36591367, 36525377, 36459505, 36393753, 36328119, 36262603, 36197205, 36131926, 36066764, 36001720, 35936793, 35871983, 35807290, 35742713, 35678253, 35613909, 35549682, 35485570, 35421574, 35357693, 35293928, 35230277, 35166741, 35103320, 35040013, 34976820, 34913742, 34850777, 34787925, 34725187, 34662563, 34600051, 34537651, 34475365, 34413190, 34351128, 34289178, 34227339, 34165612, 34103997, 34042492, 33981098, 33919815, 33858643, 33797581, 33736629, 33675787, 33615054, 33554432, 33493918, 33433514, 33373218, 33313032, 33252953, 33192984, 33133122, 33073368, 33013722, 32954184, 32894753, 32835429, 32776212, 32717102, 32658099, 32599202, 32540411, 32481726, 32423147, 32364674, 32306306, 32248044, 32189886, 32131834, 32073886, 32016042, 31958303, 31900668, 31843137, 31785710, 31728386, 31671166, 31614049, 31557035, 31500123, 31443315, 31386609, 31330005, 31273503, 31217103, 31160804, 31104608, 31048512, 30992518, 30936625, 30880832, 30825141, 30769549, 30714058, 30658667, 30603376, 30548185, 30493093, 30438100, 30383207, 30328412, 30273717, 30219120, 30164621, 30110221, 30055919, 30001715, 29947609, 29893600, 29839689, 29785874, 29732157, 29678537, 29625013, 29571586, 29518256, 29465021, 29411883, 29358840, 29305893, 29253042, 29200286, 29147625, 29095058, 29042587, 28990211, 28937928, 28885740, 28833647, 28781647, 28729741, 28677928, 28626209, 28574584, 28523051, 28471611, 28420264, 28369010, 28317848, 28266779, 28215801, 28164916, 28114122, 28063419, 28012809, 27962289, 27911861, 27861523, 27811277, 27761121, 27711055, 27661080, 27611195, 27561399, 27511694, 27462078, 27412552, 27363115, 27313767, 27264508, 27215338, 27166257, 27117264, 27068360, 27019544, 26970815, 26922175, 26873623, 26825158, 26776780, 26728489, 26680286, 26632170, 26584140, 26536197, 26488341, 26440571, 26392886, 26345288, 26297776, 26250350, 26203009, 26155753, 26108583, 26061497, 26014497, 25967581, 25920750, 25874004, 25827341, 25780763, 25734269, 25687859, 25641532, 25595289, 25549130, 25503053, 25457060, 25411150, 25365322, 25319577, 25273915, 25228335, 25182837, 25137421, 25092087, 25046835, 25001664, 24956575, 24911568, 24866641, 24821795, 24777031, 24732347, 24687743, 24643220, 24598778, 24554415, 24510133, 24465930, 24421807, 24377764, 24333800, 24289916, 24246110, 24202384, 24158736, 24115167, 24071677, 24028265, 23984931, 23941676, 23898498, 23855399, 23812377, 23769433, 23726566, 23683776, 23641064, 23598429, 23555870, 23513389, 23470984, 23428655, 23386403, 23344227, 23302127, 23260103, 23218155, 23176282, 23134485, 23092763, 23051117, 23009545, 22968049, 22926627, 22885280, 22844008, 22802810, 22761687, 22720637, 22679662, 22638760, 22597933, 22557179, 22516498, 22475891, 22435357, 22394896, 22354508, 22314193, 22273951, 22233781, 22193684, 22153659, 22113706, 22073825, 22034016, 21994279, 21954613, 21915020, 21875497, 21836046, 21796666, 21757357, 21718119, 21678951, 21639855, 21600828, 21561872, 21522987, 21484171, 21445426, 21406750, 21368144, 21329608, 21291142, 21252744, 21214416, 21176157, 21137967, 21099846, 21061794, 21023810, 20985895, 20948048, 20910269, 20872559, 20834916, 20797342, 20759835, 20722396, 20685024, 20647720, 20610483, 20573313, 20536210, 20499174, 20462205, 20425303, 20388467, 20351698, 20314994, 20278357, 20241787, 20205282, 20168843, 20132469, 20096162, 20059919, 20023742, 19987631, 19951584, 19915603, 19879686, 19843834, 19808047, 19772324, 19736666, 19701072, 19665542, 19630077, 19594675, 19559337, 19524063, 19488852, 19453705, 19418622, 19383601, 19348644, 19313750, 19278919, 19244150, 19209444, 19174801, 19140221, 19105702, 19071246, 19036852, 19002521, 18968251, 18934042, 18899896, 18865811, 18831788, 18797826, 18763925, 18730085, 18696307, 18662589, 18628932, 18595336, 18561800, 18528325, 18494910, 18461556, 18428261, 18395027, 18361853, 18328738, 18295683, 18262688, 18229752, 18196876, 18164059, 18131301, 18098602, 18065963, 18033382, 18000860, 17968396, 17935991, 17903645, 17871356, 17839126, 17806954, 17774841, 17742785, 17710787, 17678846, 17646964, 17615138, 17583370, 17551660, 17520006, 17488410, 17456871, 17425388, 17393962, 17362593, 17331281, 17300025, 17268825, 17237682, 17206595, 17175564, 17144589, 17113669, 17082806, 17051998, 17021246, 16990549, 16959907, 16929321, 16898790, 16868314, 16837893, 16807527, 16777216, 16746959, 16716757, 16686609, 16656516, 16626476, 16596492, 16566561, 16536684, 16506861, 16477092, 16447376, 16417714, 16388106, 16358551, 16329049, 16299601, 16270205, 16240863, 16211573, 16182337, 16153153, 16124022, 16094943, 16065917, 16036943, 16008021, 15979151, 15950334, 15921568, 15892855, 15864193, 15835583, 15807024, 15778517, 15750061, 15721657, 15693304, 15665002, 15636751, 15608551, 15580402, 15552304, 15524256, 15496259, 15468312, 15440416, 15412570, 15384774, 15357029, 15329333, 15301688, 15274092, 15246546, 15219050, 15191603, 15164206, 15136858, 15109560, 15082310, 15055110, 15027959, 15000857, 14973804, 14946800, 14919844, 14892937, 14866078, 14839268, 14812506, 14785793, 14759128, 14732510, 14705941, 14679420, 14652946, 14626521, 14600143, 14573812, 14547529, 14521293, 14495105, 14468964, 14442870, 14416823, 14390823, 14364870, 14338964, 14313104, 14287292, 14261525, 14235805, 14210132, 14184505, 14158924, 14133389, 14107900, 14082458, 14057061, 14031709, 14006404, 13981144, 13955930, 13930761, 13905638, 13880560, 13855527, 13830540, 13805597, 13780699, 13755847, 13731039, 13706276, 13681557, 13656883, 13632254, 13607669, 13583128, 13558632, 13534180, 13509772, 13485407, 13461087, 13436811, 13412579, 13388390, 13364244, 13340143, 13316085, 13292070, 13268098, 13244170, 13220285, 13196443, 13172644, 13148888, 13125175, 13101504, 13077876, 13054291, 13030748, 13007248, 12983790, 12960375, 12937002, 12913670, 12890381, 12867134, 12843929, 12820766, 12797644, 12774565, 12751526, 12728530, 12705575, 12682661, 12659788, 12636957, 12614167, 12591418, 12568710, 12546043, 12523417, 12500832, 12478287, 12455784, 12433320, 12410897, 12388515, 12366173, 12343871, 12321610, 12299389, 12277207, 12255066, 12232965, 12210903, 12188882, 12166900, 12144958, 12123055, 12101192, 12079368, 12057583, 12035838, 12014132, 11992465, 11970838, 11949249, 11927699, 11906188, 11884716, 11863283, 11841888, 11820532, 11799214, 11777935, 11756694, 11735492, 11714327, 11693201, 11672113, 11651063, 11630051, 11609077, 11588141, 11567242, 11546381, 11525558, 11504772, 11484024, 11463313, 11442640, 11422004, 11401405, 11380843, 11360318, 11339831, 11319380, 11298966, 11278589, 11258249, 11237945, 11217678, 11197448, 11177254, 11157096, 11136975, 11116890, 11096842, 11076829, 11056853, 11036912, 11017008, 10997139, 10977306, 10957510, 10937748, 10918023, 10898333, 10878678, 10859059, 10839475, 10819927, 10800414, 10780936, 10761493, 10742085, 10722713, 10703375, 10684072, 10664804, 10645571, 10626372, 10607208, 10588078, 10568983, 10549923, 10530897, 10511905, 10492947, 10474024, 10455134, 10436279, 10417458, 10398671, 10379917, 10361198, 10342512, 10323860, 10305241, 10286656, 10268105, 10249587, 10231102, 10212651, 10194233, 10175849, 10157497, 10139178, 10120893, 10102641, 10084421, 10066234, 10048081, 10029959, 10011871, 9993815, 9975792, 9957801, 9939843, 9921917, 9904023, 9886162, 9868333, 9850536, 9832771, 9815038, 9797337, 9779668, 9762031, 9744426, 9726852, 9709311, 9691800, 9674322, 9656875, 9639459, 9622075, 9604722, 9587400, 9570110, 9552851, 9535623, 9518426, 9501260, 9484125, 9467021, 9449948, 9432905, 9415894, 9398913, 9381962, 9365042, 9348153, 9331294, 9314466, 9297668, 9280900, 9264162, 9247455, 9230778, 9214130, 9197513, 9180926, 9164369, 9147841, 9131344, 9114876, 9098438, 9082029, 9065650, 9049301, 9032981, 9016691, 9000430, 8984198, 8967995, 8951822, 8935678, 8919563, 8903477, 8887420, 8871392, 8855393, 8839423, 8823482, 8807569, 8791685, 8775830, 8760003, 8744205, 8728435, 8712694, 8696981, 8681296, 8665640, 8650012, 8634412, 8618841, 8603297, 8587782, 8572294, 8556834, 8541403, 8525999, 8510623, 8495274, 8479953, 8464660, 8449395, 8434157, 8418946, 8403763, 8388608, 8373479, 8358378, 8343304, 8328258, 8313238, 8298246, 8283280, 8268342, 8253430, 8238546, 8223688, 8208857, 8194053, 8179275, 8164524, 8149800, 8135102, 8120431, 8105786, 8091168, 8076576, 8062011, 8047471, 8032958, 8018471, 8004010, 7989575, 7975167, 7960784, 7946427, 7932096, 7917791, 7903512, 7889258, 7875030, 7860828, 7846652, 7832501, 7818375, 7804275, 7790201, 7776152, 7762128, 7748129, 7734156, 7720208, 7706285, 7692387, 7678514, 7664666, 7650844, 7637046, 7623273, 7609525, 7595801, 7582103, 7568429, 7554780, 7541155, 7527555, 7513979, 7500428, 7486902, 7473400, 7459922, 7446468, 7433039, 7419634, 7406253, 7392896, 7379564, 7366255, 7352970, 7339710, 7326473, 7313260, 7300071, 7286906, 7273764, 7260646, 7247552, 7234482, 7221435, 7208411, 7195411, 7182435, 7169482, 7156552, 7143646, 7130762, 7117902, 7105066, 7092252, 7079462, 7066694, 7053950, 7041229, 7028530, 7015854, 7003202, 6990572, 6977965, 6965380, 6952819, 6940280, 6927763, 6915270, 6902798, 6890349, 6877923, 6865519, 6853138, 6840778, 6828441, 6816127, 6803834, 6791564, 6779316, 6767090, 6754886, 6742703, 6730543, 6718405, 6706289, 6694195, 6682122, 6670071, 6658042, 6646035, 6634049, 6622085, 6610142, 6598221, 6586322, 6574444, 6562587, 6550752, 6538938, 6527145, 6515374, 6503624, 6491895, 6480187, 6468501, 6456835, 6445190, 6433567, 6421964, 6410383, 6398822, 6387282, 6375763, 6364265, 6352787, 6341330, 6329894, 6318478, 6307083, 6295709, 6284355, 6273021, 6261708, 6250416, 6239143, 6227892, 6216660, 6205448, 6194257, 6183086, 6171935, 6160805, 6149694, 6138603, 6127533, 6116482, 6105451, 6094441, 6083450, 6072479, 6061527, 6050596, 6039684, 6028791, 6017919, 6007066, 5996232, 5985419, 5974624, 5963849, 5953094, 5942358, 5931641, 5920944, 5910266, 5899607, 5888967, 5878347, 5867746, 5857163, 5846600, 5836056, 5825531, 5815025, 5804538, 5794070, 5783621, 5773190, 5762779, 5752386, 5742012, 5731656, 5721320, 5711002, 5700702, 5690421, 5680159, 5669915, 5659690, 5649483, 5639294, 5629124, 5618972, 5608839, 5598724, 5588627, 5578548, 5568487, 5558445, 5548421, 5538414, 5528426, 5518456, 5508504, 5498569, 5488653, 5478755, 5468874, 5459011, 5449166, 5439339, 5429529, 5419737, 5409963, 5400207, 5390468, 5380746, 5371042, 5361356, 5351687, 5342036, 5332402, 5322785, 5313186, 5303604, 5294039, 5284491, 5274961, 5265448, 5255952, 5246473, 5237012, 5227567, 5218139, 5208729, 5199335, 5189958, 5180599, 5171256, 5161930, 5152620, 5143328, 5134052, 5124793, 5115551, 5106325, 5097116, 5087924, 5078748, 5069589, 5060446, 5051320, 5042210, 5033117, 5024040, 5014979, 5005935, 4996907, 4987896, 4978900, 4969921, 4960958, 4952011, 4943081, 4934166, 4925268, 4916385, 4907519, 4898668, 4889834, 4881015, 4872213, 4863426, 4854655, 4845900, 4837161, 4828437, 4819729, 4811037, 4802361, 4793700, 4785055, 4776425, 4767811, 4759213, 4750630, 4742062, 4733510, 4724974, 4716452, 4707947, 4699456, 4690981, 4682521, 4674076, 4665647, 4657233, 4648834, 4640450, 4632081, 4623727, 4615389, 4607065, 4598756, 4590463, 4582184, 4573920, 4565672, 4557438, 4549219, 4541014, 4532825, 4524650, 4516490, 4508345, 4500215, 4492099, 4483997, 4475911, 4467839, 4459781, 4451738, 4443710, 4435696, 4427696, 4419711, 4411741, 4403784, 4395842, 4387915, 4380001, 4372102, 4364217, 4356347, 4348490, 4340648, 4332820, 4325006, 4317206, 4309420, 4301648, 4293891, 4286147, 4278417, 4270701, 4262999, 4255311, 4247637, 4239976, 4232330, 4224697, 4217078, 4209473, 4201881, 4194304, 4186739, 4179189, 4171652, 4164129, 4156619, 4149123, 4141640, 4134171, 4126715, 4119273, 4111844, 4104428, 4097026, 4089637, 4082262, 4074900, 4067551, 4060215, 4052893, 4045584, 4038288, 4031005, 4023735, 4016479, 4009235, 4002005, 3994787, 3987583, 3980392, 3973213, 3966048, 3958895, 3951756, 3944629, 3937515, 3930414, 3923326, 3916250, 3909187, 3902137, 3895100, 3888076, 3881064, 3874064, 3867078, 3860104, 3853142, 3846193, 3839257, 3832333, 3825422, 3818523, 3811636, 3804762, 3797900, 3791051, 3784214, 3777390, 3770577, 3763777, 3756989, 3750214, 3743451, 3736700, 3729961, 3723234, 3716519, 3709817, 3703126, 3696448, 3689782, 3683127, 3676485, 3669855, 3663236, 3656630, 3650035, 3643453, 3636882, 3630323, 3623776, 3617241, 3610717, 3604205, 3597705, 3591217, 3584741, 3578276, 3571823, 3565381, 3558951, 3552533, 3546126, 3539731, 3533347, 3526975, 3520614, 3514265, 3507927, 3501601, 3495286, 3488982, 3482690, 3476409, 3470140, 3463881, 3457635, 3451399, 3445174, 3438961, 3432759, 3426569, 3420389, 3414220, 3408063, 3401917, 3395782, 3389658, 3383545, 3377443, 3371351, 3365271, 3359202, 3353144, 3347097, 3341061, 3335035, 3329021, 3323017, 3317024, 3311042, 3305071, 3299110, 3293161, 3287222, 3281293, 3275376, 3269469, 3263572, 3257687, 3251812, 3245947, 3240093, 3234250, 3228417, 3222595, 3216783, 3210982, 3205191, 3199411, 3193641, 3187881, 3182132, 3176393, 3170665, 3164947, 3159239, 3153541, 3147854, 3142177, 3136510, 3130854, 3125208, 3119571, 3113946, 3108330, 3102724, 3097128, 3091543, 3085967, 3080402, 3074847, 3069301, 3063766, 3058241, 3052725, 3047220, 3041725, 3036239, 3030763, 3025298, 3019842, 3014395, 3008959, 3003533, 2998116, 2992709, 2987312, 2981924, 2976547, 2971179, 2965820, 2960472, 2955133, 2949803, 2944483, 2939173, 2933873, 2928581, 2923300, 2918028, 2912765, 2907512, 2902269, 2897035, 2891810, 2886595, 2881389, 2876193, 2871006, 2865828, 2860660, 2855501, 2850351, 2845210, 2840079, 2834957, 2829845, 2824741, 2819647, 2814562, 2809486, 2804419, 2799362, 2794313, 2789274, 2784243, 2779222, 2774210, 2769207, 2764213, 2759228, 2754252, 2749284, 2744326, 2739377, 2734437, 2729505, 2724583, 2719669, 2714764, 2709868, 2704981, 2700103, 2695234, 2690373, 2685521, 2680678, 2675843, 2671018, 2666201, 2661392, 2656593, 2651802, 2647019, 2642245, 2637480, 2632724, 2627976, 2623236, 2618506, 2613783, 2609069, 2604364, 2599667, 2594979, 2590299, 2585628, 2580965, 2576310, 2571664, 2567026, 2562396, 2557775, 2553162, 2548558, 2543962, 2539374, 2534794, 2530223, 2525660, 2521105, 2516558, 2512020, 2507489, 2502967, 2498453, 2493948, 2489450, 2484960, 2480479, 2476005, 2471540, 2467083, 2462634, 2458192, 2453759, 2449334, 2444917, 2440507, 2436106, 2431713, 2427327, 2422950, 2418580, 2414218, 2409864, 2405518, 2401180, 2396850, 2392527, 2388212, 2383905, 2379606, 2375315, 2371031, 2366755, 2362487, 2358226, 2353973, 2349728, 2345490, 2341260, 2337038, 2332823, 2328616, 2324417, 2320225, 2316040, 2311863, 2307694, 2303532, 2299378, 2295231, 2291092, 2286960, 2282836, 2278719, 2274609, 2270507, 2266412, 2262325, 2258245, 2254172, 2250107, 2246049, 2241998, 2237955, 2233919, 2229890, 2225869, 2221855, 2217848, 2213848, 2209855, 2205870, 2201892, 2197921, 2193957, 2190000, 2186051, 2182108, 2178173, 2174245, 2170324, 2166410, 2162503, 2158603, 2154710, 2150824, 2146945, 2143073, 2139208, 2135350, 2131499, 2127655, 2123818, 2119988, 2116165, 2112348, 2108539, 2104736, 2100940, 2097152, 2093369, 2089594, 2085826, 2082064, 2078309, 2074561, 2070820, 2067085, 2063357, 2059636, 2055922, 2052214, 2048513, 2044818, 2041131, 2037450, 2033775, 2030107, 2026446, 2022792, 2019144, 2015502, 2011867, 2008239, 2004617, 2001002, 1997393, 1993791, 1990196, 1986606, 1983024, 1979447, 1975878, 1972314, 1968757, 1965207, 1961663, 1958125, 1954593, 1951068, 1947550, 1944038, 1940532, 1937032, 1933539, 1930052, 1926571, 1923096, 1919628, 1916166, 1912711, 1909261, 1905818, 1902381, 1898950, 1895525, 1892107, 1888695, 1885288, 1881888, 1878494, 1875107, 1871725, 1868350, 1864980, 1861617, 1858259, 1854908, 1851563, 1848224, 1844891, 1841563, 1838242, 1834927, 1831618, 1828315, 1825017, 1821726, 1818441, 1815161, 1811888, 1808620, 1805358, 1802102, 1798852, 1795608, 1792370, 1789138, 1785911, 1782690, 1779475, 1776266, 1773063, 1769865, 1766673, 1763487, 1760307, 1757132, 1753963, 1750800, 1747643, 1744491, 1741345, 1738204, 1735070, 1731940, 1728817, 1725699, 1722587, 1719480, 1716379, 1713284, 1710194, 1707110, 1704031, 1700958, 1697891, 1694829, 1691772, 1688721, 1685675, 1682635, 1679601, 1676572, 1673548, 1670530, 1667517, 1664510, 1661508, 1658512, 1655521, 1652535, 1649555, 1646580, 1643611, 1640646, 1637688, 1634734, 1631786, 1628843, 1625906, 1622973, 1620046, 1617125, 1614208, 1611297, 1608391, 1605491, 1602595, 1599705, 1596820, 1593940, 1591066, 1588196, 1585332, 1582473, 1579619, 1576770, 1573927, 1571088, 1568255, 1565427, 1562604, 1559785, 1556973, 1554165, 1551362, 1548564, 1545771, 1542983, 1540201, 1537423, 1534650, 1531883, 1529120, 1526362, 1523610, 1520862, 1518119, 1515381, 1512649, 1509921, 1507197, 1504479, 1501766, 1499058, 1496354, 1493656, 1490962, 1488273, 1485589, 1482910, 1480236, 1477566, 1474901, 1472241, 1469586, 1466936, 1464290, 1461650, 1459014, 1456382, 1453756, 1451134, 1448517, 1445905, 1443297, 1440694, 1438096, 1435503, 1432914, 1430330, 1427750, 1425175, 1422605, 1420039, 1417478, 1414922, 1412370, 1409823, 1407281, 1404743, 1402209, 1399681, 1397156, 1394637, 1392121, 1389611, 1387105, 1384603, 1382106, 1379614, 1377126, 1374642, 1372163, 1369688, 1367218, 1364752, 1362291, 1359834, 1357382, 1354934, 1352490, 1350051, 1347617, 1345186, 1342760, 1340339, 1337921, 1335509, 1333100, 1330696, 1328296, 1325901, 1323509, 1321122, 1318740, 1316362, 1313988, 1311618, 1309253, 1306891, 1304534, 1302182, 1299833, 1297489, 1295149, 1292814, 1290482, 1288155, 1285832, 1283513, 1281198, 1278887, 1276581, 1274279, 1271981, 1269687, 1267397, 1265111, 1262830, 1260552, 1258279, 1256010, 1253744, 1251483, 1249226, 1246974, 1244725, 1242480, 1240239, 1238002, 1235770, 1233541, 1231317, 1229096, 1226879, 1224667, 1222458, 1220253, 1218053, 1215856, 1213663, 1211475, 1209290, 1207109, 1204932, 1202759, 1200590, 1198425, 1196263, 1194106, 1191952, 1189803, 1187657, 1185515, 1183377, 1181243, 1179113, 1176986, 1174864, 1172745, 1170630, 1168519, 1166411, 1164308, 1162208, 1160112, 1158020, 1155931, 1153847, 1151766, 1149689, 1147615, 1145546, 1143480, 1141418, 1139359, 1137304, 1135253, 1133206, 1131162, 1129122, 1127086, 1125053, 1123024, 1120999, 1118977, 1116959, 1114945, 1112934, 1110927, 1108924, 1106924, 1104927, 1102935, 1100946, 1098960, 1096978, 1095000, 1093025, 1091054, 1089086, 1087122, 1085162, 1083205, 1081251, 1079301, 1077355, 1075412, 1073472, 1071536, 1069604, 1067675, 1065749, 1063827, 1061909, 1059994, 1058082, 1056174, 1054269, 1052368, 1050470, 1048576, 1046684, 1044797, 1042913, 1041032, 1039154, 1037280, 1035410, 1033542, 1031678, 1029818, 1027961, 1026107, 1024256, 1022409, 1020565, 1018725, 1016887, 1015053, 1013223, 1011396, 1009572, 1007751, 1005933, 1004119, 1002308, 1000501, 998696, 996895, 995098, 993303, 991512, 989723, 987939, 986157, 984378, 982603, 980831, 979062, 977296, 975534, 973775, 972019, 970266, 968516, 966769, 965026, 963285, 961548, 959814, 958083, 956355, 954630, 952909, 951190, 949475, 947762, 946053, 944347, 942644, 940944, 939247, 937553, 935862, 934175, 932490, 930808, 929129, 927454, 925781, 924112, 922445, 920781, 919121, 917463, 915809, 914157, 912508, 910863, 909220, 907580, 905944, 904310, 902679, 901051, 899426, 897804, 896185, 894569, 892955, 891345, 889737, 888133, 886531, 884932, 883336, 881743, 880153, 878566, 876981, 875400, 873821, 872245, 870672, 869102, 867535, 865970, 864408, 862849, 861293, 859740, 858189, 856642, 855097, 853555, 852015, 850479, 848945, 847414, 845886, 844360, 842837, 841317, 839800, 838286, 836774, 835265, 833758, 832255, 830754, 829256, 827760, 826267, 824777, 823290, 821805, 820323, 818844, 817367, 815893, 814421, 812953, 811486, 810023, 808562, 807104, 805648, 804195, 802745, 801297, 799852, 798410, 796970, 795533, 794098, 792666, 791236, 789809, 788385, 786963, 785544, 784127, 782713, 781302, 779892, 778486, 777082, 775681, 774282, 772885, 771491, 770100, 768711, 767325, 765941, 764560, 763181, 761805, 760431, 759059, 757690, 756324, 754960, 753598, 752239, 750883, 749529, 748177, 746828, 745481, 744136, 742794, 741455, 740118, 738783, 737450, 736120, 734793, 733468, 732145, 730825, 729507, 728191, 726878, 725567, 724258, 722952, 721648, 720347, 719048, 717751, 716457, 715165, 713875, 712587, 711302, 710019, 708739, 707461, 706185, 704911, 703640, 702371, 701104, 699840, 698578, 697318, 696060, 694805, 693552, 692301, 691053, 689807, 688563, 687321, 686081, 684844, 683609, 682376, 681145, 679917, 678691, 677467, 676245, 675025, 673808, 672593, 671380, 670169, 668960, 667754, 666550, 665348, 664148, 662950, 661754, 660561, 659370, 658181, 656994, 655809, 654626, 653445, 652267, 651091, 649916, 648744, 647574, 646407, 645241, 644077, 642916, 641756, 640599, 639443, 638290, 637139, 635990, 634843, 633698, 632555, 631415, 630276, 629139, 628005, 626872, 625741, 624613, 623487, 622362, 621240, 620119, 619001, 617885, 616770, 615658, 614548, 613439, 612333, 611229, 610126, 609026, 607928, 606831, 605737, 604645, 603554, 602466, 601379, 600295, 599212, 598131, 597053, 595976, 594901, 593828, 592757, 591688, 590621, 589556, 588493, 587432, 586372, 585315, 584259, 583205, 582154, 581104, 580056, 579010, 577965, 576923, 575883, 574844, 573807, 572773, 571740, 570709, 569679, 568652, 567626, 566603, 565581, 564561, 563543, 562526, 561512, 560499, 559488, 558479, 557472, 556467, 555463, 554462, 553462, 552463, 551467, 550473, 549480, 548489, 547500, 546512, 545527, 544543, 543561, 542581, 541602, 540625, 539650, 538677, 537706, 536736, 535768, 534802, 533837, 532874, 531913, 530954, 529997, 529041, 528087, 527134, 526184, 525235, 524288, 523342, 522398, 521456, 520516, 519577, 518640, 517705, 516771, 515839, 514909, 513980, 513053, 512128, 511204, 510282, 509362, 508443, 507526, 506611, 505698, 504786, 503875, 502966, 502059, 501154, 500250, 499348, 498447, 497549, 496651, 495756, 494861, 493969, 493078, 492189, 491301, 490415, 489531, 488648, 487767, 486887, 486009, 485133, 484258, 483384, 482513, 481642, 480774, 479907, 479041, 478177, 477315, 476454, 475595, 474737, 473881, 473026, 472173, 471322, 470472, 469623, 468776, 467931, 467087, 466245, 465404, 464564, 463727, 462890, 462056, 461222, 460390, 459560, 458731, 457904, 457078, 456254, 455431, 454610, 453790, 452972, 452155, 451339, 450525, 449713, 448902, 448092, 447284, 446477, 445672, 444868, 444066, 443265, 442466, 441668, 440871, 440076, 439283, 438490, 437700, 436910, 436122, 435336, 434551, 433767, 432985, 432204, 431424, 430646, 429870, 429094, 428321, 427548, 426777, 426007, 425239, 424472, 423707, 422943, 422180, 421418, 420658, 419900, 419143, 418387, 417632, 416879, 416127, 415377, 414628, 413880, 413133, 412388, 411645, 410902, 410161, 409422, 408683, 407946, 407210, 406476, 405743, 405011, 404281, 403552, 402824, 402097, 401372, 400648, 399926, 399205, 398485, 397766, 397049, 396333, 395618, 394904, 394192, 393481, 392772, 392063, 391356, 390651, 389946, 389243, 388541, 387840, 387141, 386442, 385745, 385050, 384355, 383662, 382970, 382280, 381590, 380902, 380215, 379529, 378845, 378162, 377480, 376799, 376119, 375441, 374764, 374088, 373414, 372740, 372068, 371397, 370727, 370059, 369391, 368725, 368060, 367396, 366734, 366072, 365412, 364753, 364095, 363439, 362783, 362129, 361476, 360824, 360173, 359524, 358875, 358228, 357582, 356937, 356293, 355651, 355009, 354369, 353730, 353092, 352455, 351820, 351185, 350552, 349920, 349289, 348659, 348030, 347402, 346776, 346150, 345526, 344903, 344281, 343660, 343040, 342422, 341804, 341188, 340572, 339958, 339345, 338733, 338122, 337512, 336904, 336296, 335690, 335084, 334480, 333877, 333275, 332674, 332074, 331475, 330877, 330280, 329685, 329090, 328497, 327904, 327313, 326722, 326133, 325545, 324958, 324372, 323787, 323203, 322620, 322038, 321458, 320878, 320299, 319721, 319145, 318569, 317995, 317421, 316849, 316277, 315707, 315138, 314569, 314002, 313436, 312870, 312306, 311743, 311181, 310620, 310059, 309500, 308942, 308385, 307829, 307274, 306719, 306166, 305614, 305063, 304513, 303964, 303415, 302868, 302322, 301777, 301233, 300689, 300147, 299606, 299065, 298526, 297988, 297450, 296914, 296378, 295844, 295310, 294778, 294246, 293716, 293186, 292657, 292129, 291602, 291077, 290552, 290028, 289505, 288982, 288461, 287941, 287422, 286903, 286386, 285870, 285354, 284839, 284326, 283813, 283301, 282790, 282280, 281771, 281263, 280756, 280249, 279744, 279239, 278736, 278233, 277731, 277231, 276731, 276231, 275733, 275236, 274740, 274244, 273750, 273256, 272763, 272271, 271780, 271290, 270801, 270312, 269825, 269338, 268853, 268368, 267884, 267401, 266918, 266437, 265956, 265477, 264998, 264520, 264043, 263567, 263092, 262617, 262144, 261671, 261199, 260728, 260258, 259788, 259320, 258852, 258385, 257919, 257454, 256990, 256526, 256064, 255602, 255141, 254681, 254221, 253763, 253305, 252849, 252393, 251937, 251483, 251029, 250577, 250125, 249674, 249223, 248774, 248325, 247878, 247430, 246984, 246539, 246094, 245650, 245207, 244765, 244324, 243883, 243443, 243004, 242566, 242129, 241692, 241256, 240821, 240387, 239953, 239520, 239088, 238657, 238227, 237797, 237368, 236940, 236513, 236086, 235661, 235236, 234811, 234388, 233965, 233543, 233122, 232702, 232282, 231863, 231445, 231028, 230611, 230195, 229780, 229365, 228952, 228539, 228127, 227715, 227305, 226895, 226486, 226077, 225669, 225262, 224856, 224451, 224046, 223642, 223238, 222836, 222434, 222033, 221632, 221233, 220834, 220435, 220038, 219641, 219245, 218850, 218455, 218061, 217668, 217275, 216883, 216492, 216102, 215712, 215323, 214935, 214547, 214160, 213774, 213388, 213003, 212619, 212236, 211853, 211471, 211090, 210709, 210329, 209950, 209571, 209193, 208816, 208439, 208063, 207688, 207314, 206940, 206566, 206194, 205822, 205451, 205080, 204711, 204341, 203973, 203605, 203238, 202871, 202505, 202140, 201776, 201412, 201048, 200686, 200324, 199963, 199602, 199242, 198883, 198524, 198166, 197809, 197452, 197096, 196740, 196386, 196031, 195678, 195325, 194973, 194621, 194270, 193920, 193570, 193221, 192872, 192525, 192177, 191831, 191485, 191140, 190795, 190451, 190107, 189764, 189422, 189081, 188740, 188399, 188059, 187720, 187382, 187044, 186707, 186370, 186034, 185698, 185363, 185029, 184695, 184362, 184030, 183698, 183367, 183036, 182706, 182376, 182047, 181719, 181391, 181064, 180738, 180412, 180086, 179762, 179437, 179114, 178791, 178468, 178146, 177825, 177504, 177184, 176865, 176546, 176227, 175910, 175592, 175276, 174960, 174644, 174329, 174015, 173701, 173388, 173075, 172763, 172451, 172140, 171830, 171520, 171211, 170902, 170594, 170286, 169979, 169672, 169366, 169061, 168756, 168452, 168148, 167845, 167542, 167240, 166938, 166637, 166337, 166037, 165737}

class ExpoConverter {

public:
	/// Store expo table in memory
	static constexpr uint32_t expoTable[4096] = expotable10oct;

	uint32_t convert(uint32_t in) {
		return expoTable[in];
	}

};

class RevExpoConverter {

public:
	/// Store expo table in memory
	static constexpr uint32_t expoTable[4096] = expotable10oct;

	uint32_t convert(uint32_t in) {
		return expoTable[in];
	}

};

class Sine {


public:

	static constexpr int32_t big_sine[4097] = {16383, 16408, 16433, 16458, 16484, 16509, 16534, 16559, 16584, 16609, 16634, 16659, 16685, 16710, 16735, 16760, 16785, 16810, 16835, 16860, 16886, 16911, 16936, 16961, 16986, 17011, 17036, 17061, 17086, 17112, 17137, 17162, 17187, 17212, 17237, 17262, 17287, 17312, 17337, 17363, 17388, 17413, 17438, 17463, 17488, 17513, 17538, 17563, 17588, 17613, 17638, 17663, 17688, 17714, 17739, 17764, 17789, 17814, 17839, 17864, 17889, 17914, 17939, 17964, 17989, 18014, 18039, 18064, 18089, 18114, 18139, 18164, 18189, 18214, 18239, 18264, 18289, 18314, 18339, 18364, 18389, 18413, 18438, 18463, 18488, 18513, 18538, 18563, 18588, 18613, 18638, 18663, 18687, 18712, 18737, 18762, 18787, 18812, 18837, 18862, 18886, 18911, 18936, 18961, 18986, 19010, 19035, 19060, 19085, 19110, 19134, 19159, 19184, 19209, 19233, 19258, 19283, 19308, 19332, 19357, 19382, 19407, 19431, 19456, 19481, 19505, 19530, 19555, 19579, 19604, 19629, 19653, 19678, 19702, 19727, 19752, 19776, 19801, 19825, 19850, 19874, 19899, 19924, 19948, 19973, 19997, 20022, 20046, 20071, 20095, 20120, 20144, 20169, 20193, 20217, 20242, 20266, 20291, 20315, 20339, 20364, 20388, 20413, 20437, 20461, 20486, 20510, 20534, 20559, 20583, 20607, 20631, 20656, 20680, 20704, 20728, 20753, 20777, 20801, 20825, 20849, 20874, 20898, 20922, 20946, 20970, 20994, 21018, 21043, 21067, 21091, 21115, 21139, 21163, 21187, 21211, 21235, 21259, 21283, 21307, 21331, 21355, 21379, 21403, 21427, 21451, 21474, 21498, 21522, 21546, 21570, 21594, 21618, 21641, 21665, 21689, 21713, 21736, 21760, 21784, 21808, 21831, 21855, 21879, 21902, 21926, 21950, 21973, 21997, 22021, 22044, 22068, 22091, 22115, 22138, 22162, 22185, 22209, 22232, 22256, 22279, 22303, 22326, 22350, 22373, 22396, 22420, 22443, 22466, 22490, 22513, 22536, 22560, 22583, 22606, 22629, 22653, 22676, 22699, 22722, 22745, 22769, 22792, 22815, 22838, 22861, 22884, 22907, 22930, 22953, 22976, 22999, 23022, 23045, 23068, 23091, 23114, 23137, 23160, 23183, 23206, 23228, 23251, 23274, 23297, 23320, 23342, 23365, 23388, 23411, 23433, 23456, 23479, 23501, 23524, 23546, 23569, 23592, 23614, 23637, 23659, 23682, 23704, 23727, 23749, 23772, 23794, 23816, 23839, 23861, 23884, 23906, 23928, 23951, 23973, 23995, 24017, 24040, 24062, 24084, 24106, 24128, 24150, 24173, 24195, 24217, 24239, 24261, 24283, 24305, 24327, 24349, 24371, 24393, 24415, 24437, 24458, 24480, 24502, 24524, 24546, 24567, 24589, 24611, 24633, 24654, 24676, 24698, 24719, 24741, 24763, 24784, 24806, 24827, 24849, 24870, 24892, 24913, 24935, 24956, 24978, 24999, 25020, 25042, 25063, 25084, 25106, 25127, 25148, 25169, 25191, 25212, 25233, 25254, 25275, 25296, 25317, 25338, 25359, 25380, 25401, 25422, 25443, 25464, 25485, 25506, 25527, 25548, 25569, 25589, 25610, 25631, 25652, 25672, 25693, 25714, 25734, 25755, 25776, 25796, 25817, 25837, 25858, 25878, 25899, 25919, 25940, 25960, 25980, 26001, 26021, 26041, 26062, 26082, 26102, 26122, 26143, 26163, 26183, 26203, 26223, 26243, 26263, 26283, 26303, 26323, 26343, 26363, 26383, 26403, 26423, 26443, 26463, 26482, 26502, 26522, 26542, 26561, 26581, 26601, 26620, 26640, 26660, 26679, 26699, 26718, 26738, 26757, 26777, 26796, 26815, 26835, 26854, 26873, 26893, 26912, 26931, 26950, 26970, 26989, 27008, 27027, 27046, 27065, 27084, 27103, 27122, 27141, 27160, 27179, 27198, 27217, 27236, 27255, 27273, 27292, 27311, 27330, 27348, 27367, 27385, 27404, 27423, 27441, 27460, 27478, 27497, 27515, 27534, 27552, 27570, 27589, 27607, 27625, 27644, 27662, 27680, 27698, 27716, 27735, 27753, 27771, 27789, 27807, 27825, 27843, 27861, 27879, 27897, 27914, 27932, 27950, 27968, 27986, 28003, 28021, 28039, 28056, 28074, 28092, 28109, 28127, 28144, 28162, 28179, 28197, 28214, 28231, 28249, 28266, 28283, 28301, 28318, 28335, 28352, 28369, 28386, 28404, 28421, 28438, 28455, 28472, 28489, 28505, 28522, 28539, 28556, 28573, 28590, 28606, 28623, 28640, 28656, 28673, 28690, 28706, 28723, 28739, 28756, 28772, 28789, 28805, 28822, 28838, 28854, 28870, 28887, 28903, 28919, 28935, 28951, 28968, 28984, 29000, 29016, 29032, 29048, 29064, 29079, 29095, 29111, 29127, 29143, 29158, 29174, 29190, 29206, 29221, 29237, 29252, 29268, 29283, 29299, 29314, 29330, 29345, 29360, 29376, 29391, 29406, 29422, 29437, 29452, 29467, 29482, 29497, 29512, 29527, 29542, 29557, 29572, 29587, 29602, 29617, 29632, 29646, 29661, 29676, 29691, 29705, 29720, 29734, 29749, 29763, 29778, 29792, 29807, 29821, 29836, 29850, 29864, 29878, 29893, 29907, 29921, 29935, 29949, 29963, 29977, 29991, 30005, 30019, 30033, 30047, 30061, 30075, 30089, 30102, 30116, 30130, 30143, 30157, 30171, 30184, 30198, 30211, 30225, 30238, 30251, 30265, 30278, 30291, 30305, 30318, 30331, 30344, 30357, 30371, 30384, 30397, 30410, 30423, 30436, 30449, 30461, 30474, 30487, 30500, 30513, 30525, 30538, 30551, 30563, 30576, 30588, 30601, 30613, 30626, 30638, 30650, 30663, 30675, 30687, 30700, 30712, 30724, 30736, 30748, 30760, 30772, 30784, 30796, 30808, 30820, 30832, 30844, 30856, 30867, 30879, 30891, 30902, 30914, 30926, 30937, 30949, 30960, 30972, 30983, 30994, 31006, 31017, 31028, 31040, 31051, 31062, 31073, 31084, 31095, 31106, 31117, 31128, 31139, 31150, 31161, 31172, 31183, 31194, 31204, 31215, 31226, 31236, 31247, 31257, 31268, 31278, 31289, 31299, 31310, 31320, 31330, 31341, 31351, 31361, 31371, 31381, 31391, 31401, 31411, 31421, 31431, 31441, 31451, 31461, 31471, 31481, 31490, 31500, 31510, 31519, 31529, 31539, 31548, 31558, 31567, 31576, 31586, 31595, 31604, 31614, 31623, 31632, 31641, 31651, 31660, 31669, 31678, 31687, 31696, 31705, 31713, 31722, 31731, 31740, 31749, 31757, 31766, 31775, 31783, 31792, 31800, 31809, 31817, 31826, 31834, 31842, 31851, 31859, 31867, 31875, 31884, 31892, 31900, 31908, 31916, 31924, 31932, 31940, 31947, 31955, 31963, 31971, 31979, 31986, 31994, 32001, 32009, 32017, 32024, 32032, 32039, 32046, 32054, 32061, 32068, 32076, 32083, 32090, 32097, 32104, 32111, 32118, 32125, 32132, 32139, 32146, 32153, 32160, 32166, 32173, 32180, 32186, 32193, 32200, 32206, 32213, 32219, 32225, 32232, 32238, 32245, 32251, 32257, 32263, 32269, 32276, 32282, 32288, 32294, 32300, 32306, 32311, 32317, 32323, 32329, 32335, 32340, 32346, 32352, 32357, 32363, 32368, 32374, 32379, 32385, 32390, 32395, 32401, 32406, 32411, 32416, 32422, 32427, 32432, 32437, 32442, 32447, 32452, 32457, 32461, 32466, 32471, 32476, 32480, 32485, 32490, 32494, 32499, 32503, 32508, 32512, 32517, 32521, 32525, 32530, 32534, 32538, 32542, 32546, 32550, 32554, 32558, 32562, 32566, 32570, 32574, 32578, 32582, 32585, 32589, 32593, 32596, 32600, 32604, 32607, 32611, 32614, 32617, 32621, 32624, 32627, 32631, 32634, 32637, 32640, 32643, 32646, 32649, 32652, 32655, 32658, 32661, 32664, 32667, 32669, 32672, 32675, 32677, 32680, 32683, 32685, 32688, 32690, 32692, 32695, 32697, 32699, 32702, 32704, 32706, 32708, 32710, 32712, 32714, 32716, 32718, 32720, 32722, 32724, 32726, 32727, 32729, 32731, 32733, 32734, 32736, 32737, 32739, 32740, 32742, 32743, 32744, 32746, 32747, 32748, 32749, 32750, 32751, 32752, 32753, 32754, 32755, 32756, 32757, 32758, 32759, 32760, 32760, 32761, 32762, 32762, 32763, 32763, 32764, 32764, 32765, 32765, 32765, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32767, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32765, 32765, 32765, 32764, 32764, 32763, 32763, 32762, 32762, 32761, 32760, 32760, 32759, 32758, 32757, 32756, 32755, 32754, 32753, 32752, 32751, 32750, 32749, 32748, 32747, 32746, 32744, 32743, 32742, 32740, 32739, 32737, 32736, 32734, 32733, 32731, 32729, 32727, 32726, 32724, 32722, 32720, 32718, 32716, 32714, 32712, 32710, 32708, 32706, 32704, 32702, 32699, 32697, 32695, 32692, 32690, 32688, 32685, 32683, 32680, 32677, 32675, 32672, 32669, 32667, 32664, 32661, 32658, 32655, 32652, 32649, 32646, 32643, 32640, 32637, 32634, 32631, 32627, 32624, 32621, 32617, 32614, 32611, 32607, 32604, 32600, 32596, 32593, 32589, 32585, 32582, 32578, 32574, 32570, 32566, 32562, 32558, 32554, 32550, 32546, 32542, 32538, 32534, 32530, 32525, 32521, 32517, 32512, 32508, 32503, 32499, 32494, 32490, 32485, 32480, 32476, 32471, 32466, 32461, 32457, 32452, 32447, 32442, 32437, 32432, 32427, 32422, 32416, 32411, 32406, 32401, 32395, 32390, 32385, 32379, 32374, 32368, 32363, 32357, 32352, 32346, 32340, 32335, 32329, 32323, 32317, 32311, 32306, 32300, 32294, 32288, 32282, 32276, 32269, 32263, 32257, 32251, 32245, 32238, 32232, 32225, 32219, 32213, 32206, 32200, 32193, 32186, 32180, 32173, 32166, 32160, 32153, 32146, 32139, 32132, 32125, 32118, 32111, 32104, 32097, 32090, 32083, 32076, 32068, 32061, 32054, 32046, 32039, 32032, 32024, 32017, 32009, 32001, 31994, 31986, 31979, 31971, 31963, 31955, 31947, 31940, 31932, 31924, 31916, 31908, 31900, 31892, 31884, 31875, 31867, 31859, 31851, 31842, 31834, 31826, 31817, 31809, 31800, 31792, 31783, 31775, 31766, 31757, 31749, 31740, 31731, 31722, 31713, 31705, 31696, 31687, 31678, 31669, 31660, 31651, 31641, 31632, 31623, 31614, 31604, 31595, 31586, 31576, 31567, 31558, 31548, 31539, 31529, 31519, 31510, 31500, 31490, 31481, 31471, 31461, 31451, 31441, 31431, 31421, 31411, 31401, 31391, 31381, 31371, 31361, 31351, 31341, 31330, 31320, 31310, 31299, 31289, 31278, 31268, 31257, 31247, 31236, 31226, 31215, 31204, 31194, 31183, 31172, 31161, 31150, 31139, 31128, 31117, 31106, 31095, 31084, 31073, 31062, 31051, 31040, 31028, 31017, 31006, 30994, 30983, 30972, 30960, 30949, 30937, 30926, 30914, 30902, 30891, 30879, 30867, 30856, 30844, 30832, 30820, 30808, 30796, 30784, 30772, 30760, 30748, 30736, 30724, 30712, 30700, 30687, 30675, 30663, 30650, 30638, 30626, 30613, 30601, 30588, 30576, 30563, 30551, 30538, 30525, 30513, 30500, 30487, 30474, 30461, 30449, 30436, 30423, 30410, 30397, 30384, 30371, 30357, 30344, 30331, 30318, 30305, 30291, 30278, 30265, 30251, 30238, 30225, 30211, 30198, 30184, 30171, 30157, 30143, 30130, 30116, 30102, 30089, 30075, 30061, 30047, 30033, 30019, 30005, 29991, 29977, 29963, 29949, 29935, 29921, 29907, 29893, 29878, 29864, 29850, 29836, 29821, 29807, 29792, 29778, 29763, 29749, 29734, 29720, 29705, 29691, 29676, 29661, 29646, 29632, 29617, 29602, 29587, 29572, 29557, 29542, 29527, 29512, 29497, 29482, 29467, 29452, 29437, 29422, 29406, 29391, 29376, 29360, 29345, 29330, 29314, 29299, 29283, 29268, 29252, 29237, 29221, 29206, 29190, 29174, 29158, 29143, 29127, 29111, 29095, 29079, 29064, 29048, 29032, 29016, 29000, 28984, 28968, 28951, 28935, 28919, 28903, 28887, 28870, 28854, 28838, 28822, 28805, 28789, 28772, 28756, 28739, 28723, 28706, 28690, 28673, 28656, 28640, 28623, 28606, 28590, 28573, 28556, 28539, 28522, 28505, 28489, 28472, 28455, 28438, 28421, 28404, 28386, 28369, 28352, 28335, 28318, 28301, 28283, 28266, 28249, 28231, 28214, 28197, 28179, 28162, 28144, 28127, 28109, 28092, 28074, 28056, 28039, 28021, 28003, 27986, 27968, 27950, 27932, 27914, 27897, 27879, 27861, 27843, 27825, 27807, 27789, 27771, 27753, 27735, 27716, 27698, 27680, 27662, 27644, 27625, 27607, 27589, 27570, 27552, 27534, 27515, 27497, 27478, 27460, 27441, 27423, 27404, 27385, 27367, 27348, 27330, 27311, 27292, 27273, 27255, 27236, 27217, 27198, 27179, 27160, 27141, 27122, 27103, 27084, 27065, 27046, 27027, 27008, 26989, 26970, 26950, 26931, 26912, 26893, 26873, 26854, 26835, 26815, 26796, 26777, 26757, 26738, 26718, 26699, 26679, 26660, 26640, 26620, 26601, 26581, 26561, 26542, 26522, 26502, 26482, 26463, 26443, 26423, 26403, 26383, 26363, 26343, 26323, 26303, 26283, 26263, 26243, 26223, 26203, 26183, 26163, 26143, 26122, 26102, 26082, 26062, 26041, 26021, 26001, 25980, 25960, 25940, 25919, 25899, 25878, 25858, 25837, 25817, 25796, 25776, 25755, 25734, 25714, 25693, 25672, 25652, 25631, 25610, 25589, 25569, 25548, 25527, 25506, 25485, 25464, 25443, 25422, 25401, 25380, 25359, 25338, 25317, 25296, 25275, 25254, 25233, 25212, 25191, 25169, 25148, 25127, 25106, 25084, 25063, 25042, 25020, 24999, 24978, 24956, 24935, 24913, 24892, 24870, 24849, 24827, 24806, 24784, 24763, 24741, 24719, 24698, 24676, 24654, 24633, 24611, 24589, 24567, 24546, 24524, 24502, 24480, 24458, 24437, 24415, 24393, 24371, 24349, 24327, 24305, 24283, 24261, 24239, 24217, 24195, 24173, 24150, 24128, 24106, 24084, 24062, 24040, 24017, 23995, 23973, 23951, 23928, 23906, 23884, 23861, 23839, 23816, 23794, 23772, 23749, 23727, 23704, 23682, 23659, 23637, 23614, 23592, 23569, 23546, 23524, 23501, 23479, 23456, 23433, 23411, 23388, 23365, 23342, 23320, 23297, 23274, 23251, 23228, 23206, 23183, 23160, 23137, 23114, 23091, 23068, 23045, 23022, 22999, 22976, 22953, 22930, 22907, 22884, 22861, 22838, 22815, 22792, 22769, 22745, 22722, 22699, 22676, 22653, 22629, 22606, 22583, 22560, 22536, 22513, 22490, 22466, 22443, 22420, 22396, 22373, 22350, 22326, 22303, 22279, 22256, 22232, 22209, 22185, 22162, 22138, 22115, 22091, 22068, 22044, 22021, 21997, 21973, 21950, 21926, 21902, 21879, 21855, 21831, 21808, 21784, 21760, 21736, 21713, 21689, 21665, 21641, 21618, 21594, 21570, 21546, 21522, 21498, 21474, 21451, 21427, 21403, 21379, 21355, 21331, 21307, 21283, 21259, 21235, 21211, 21187, 21163, 21139, 21115, 21091, 21067, 21043, 21018, 20994, 20970, 20946, 20922, 20898, 20874, 20849, 20825, 20801, 20777, 20753, 20728, 20704, 20680, 20656, 20631, 20607, 20583, 20559, 20534, 20510, 20486, 20461, 20437, 20413, 20388, 20364, 20339, 20315, 20291, 20266, 20242, 20217, 20193, 20169, 20144, 20120, 20095, 20071, 20046, 20022, 19997, 19973, 19948, 19924, 19899, 19874, 19850, 19825, 19801, 19776, 19752, 19727, 19702, 19678, 19653, 19629, 19604, 19579, 19555, 19530, 19505, 19481, 19456, 19431, 19407, 19382, 19357, 19332, 19308, 19283, 19258, 19233, 19209, 19184, 19159, 19134, 19110, 19085, 19060, 19035, 19010, 18986, 18961, 18936, 18911, 18886, 18862, 18837, 18812, 18787, 18762, 18737, 18712, 18687, 18663, 18638, 18613, 18588, 18563, 18538, 18513, 18488, 18463, 18438, 18413, 18389, 18364, 18339, 18314, 18289, 18264, 18239, 18214, 18189, 18164, 18139, 18114, 18089, 18064, 18039, 18014, 17989, 17964, 17939, 17914, 17889, 17864, 17839, 17814, 17789, 17764, 17739, 17714, 17688, 17663, 17638, 17613, 17588, 17563, 17538, 17513, 17488, 17463, 17438, 17413, 17388, 17363, 17337, 17312, 17287, 17262, 17237, 17212, 17187, 17162, 17137, 17112, 17086, 17061, 17036, 17011, 16986, 16961, 16936, 16911, 16886, 16860, 16835, 16810, 16785, 16760, 16735, 16710, 16685, 16659, 16634, 16609, 16584, 16559, 16534, 16509, 16484, 16458, 16433, 16408, 16383, 16358, 16333, 16308, 16282, 16257, 16232, 16207, 16182, 16157, 16132, 16107, 16081, 16056, 16031, 16006, 15981, 15956, 15931, 15906, 15880, 15855, 15830, 15805, 15780, 15755, 15730, 15705, 15680, 15654, 15629, 15604, 15579, 15554, 15529, 15504, 15479, 15454, 15429, 15403, 15378, 15353, 15328, 15303, 15278, 15253, 15228, 15203, 15178, 15153, 15128, 15103, 15078, 15052, 15027, 15002, 14977, 14952, 14927, 14902, 14877, 14852, 14827, 14802, 14777, 14752, 14727, 14702, 14677, 14652, 14627, 14602, 14577, 14552, 14527, 14502, 14477, 14452, 14427, 14402, 14377, 14353, 14328, 14303, 14278, 14253, 14228, 14203, 14178, 14153, 14128, 14103, 14079, 14054, 14029, 14004, 13979, 13954, 13929, 13904, 13880, 13855, 13830, 13805, 13780, 13756, 13731, 13706, 13681, 13656, 13632, 13607, 13582, 13557, 13533, 13508, 13483, 13458, 13434, 13409, 13384, 13359, 13335, 13310, 13285, 13261, 13236, 13211, 13187, 13162, 13137, 13113, 13088, 13064, 13039, 13014, 12990, 12965, 12941, 12916, 12892, 12867, 12842, 12818, 12793, 12769, 12744, 12720, 12695, 12671, 12646, 12622, 12597, 12573, 12549, 12524, 12500, 12475, 12451, 12427, 12402, 12378, 12353, 12329, 12305, 12280, 12256, 12232, 12207, 12183, 12159, 12135, 12110, 12086, 12062, 12038, 12013, 11989, 11965, 11941, 11917, 11892, 11868, 11844, 11820, 11796, 11772, 11748, 11723, 11699, 11675, 11651, 11627, 11603, 11579, 11555, 11531, 11507, 11483, 11459, 11435, 11411, 11387, 11363, 11339, 11315, 11292, 11268, 11244, 11220, 11196, 11172, 11148, 11125, 11101, 11077, 11053, 11030, 11006, 10982, 10958, 10935, 10911, 10887, 10864, 10840, 10816, 10793, 10769, 10745, 10722, 10698, 10675, 10651, 10628, 10604, 10581, 10557, 10534, 10510, 10487, 10463, 10440, 10416, 10393, 10370, 10346, 10323, 10300, 10276, 10253, 10230, 10206, 10183, 10160, 10137, 10113, 10090, 10067, 10044, 10021, 9997, 9974, 9951, 9928, 9905, 9882, 9859, 9836, 9813, 9790, 9767, 9744, 9721, 9698, 9675, 9652, 9629, 9606, 9583, 9560, 9538, 9515, 9492, 9469, 9446, 9424, 9401, 9378, 9355, 9333, 9310, 9287, 9265, 9242, 9220, 9197, 9174, 9152, 9129, 9107, 9084, 9062, 9039, 9017, 8994, 8972, 8950, 8927, 8905, 8882, 8860, 8838, 8815, 8793, 8771, 8749, 8726, 8704, 8682, 8660, 8638, 8616, 8593, 8571, 8549, 8527, 8505, 8483, 8461, 8439, 8417, 8395, 8373, 8351, 8329, 8308, 8286, 8264, 8242, 8220, 8199, 8177, 8155, 8133, 8112, 8090, 8068, 8047, 8025, 8003, 7982, 7960, 7939, 7917, 7896, 7874, 7853, 7831, 7810, 7788, 7767, 7746, 7724, 7703, 7682, 7660, 7639, 7618, 7597, 7575, 7554, 7533, 7512, 7491, 7470, 7449, 7428, 7407, 7386, 7365, 7344, 7323, 7302, 7281, 7260, 7239, 7218, 7197, 7177, 7156, 7135, 7114, 7094, 7073, 7052, 7032, 7011, 6990, 6970, 6949, 6929, 6908, 6888, 6867, 6847, 6826, 6806, 6786, 6765, 6745, 6725, 6704, 6684, 6664, 6644, 6623, 6603, 6583, 6563, 6543, 6523, 6503, 6483, 6463, 6443, 6423, 6403, 6383, 6363, 6343, 6323, 6303, 6284, 6264, 6244, 6224, 6205, 6185, 6165, 6146, 6126, 6106, 6087, 6067, 6048, 6028, 6009, 5989, 5970, 5951, 5931, 5912, 5893, 5873, 5854, 5835, 5816, 5796, 5777, 5758, 5739, 5720, 5701, 5682, 5663, 5644, 5625, 5606, 5587, 5568, 5549, 5530, 5511, 5493, 5474, 5455, 5436, 5418, 5399, 5381, 5362, 5343, 5325, 5306, 5288, 5269, 5251, 5232, 5214, 5196, 5177, 5159, 5141, 5122, 5104, 5086, 5068, 5050, 5031, 5013, 4995, 4977, 4959, 4941, 4923, 4905, 4887, 4869, 4852, 4834, 4816, 4798, 4780, 4763, 4745, 4727, 4710, 4692, 4674, 4657, 4639, 4622, 4604, 4587, 4569, 4552, 4535, 4517, 4500, 4483, 4465, 4448, 4431, 4414, 4397, 4380, 4362, 4345, 4328, 4311, 4294, 4277, 4261, 4244, 4227, 4210, 4193, 4176, 4160, 4143, 4126, 4110, 4093, 4076, 4060, 4043, 4027, 4010, 3994, 3977, 3961, 3944, 3928, 3912, 3896, 3879, 3863, 3847, 3831, 3815, 3798, 3782, 3766, 3750, 3734, 3718, 3702, 3687, 3671, 3655, 3639, 3623, 3608, 3592, 3576, 3560, 3545, 3529, 3514, 3498, 3483, 3467, 3452, 3436, 3421, 3406, 3390, 3375, 3360, 3344, 3329, 3314, 3299, 3284, 3269, 3254, 3239, 3224, 3209, 3194, 3179, 3164, 3149, 3134, 3120, 3105, 3090, 3075, 3061, 3046, 3032, 3017, 3003, 2988, 2974, 2959, 2945, 2930, 2916, 2902, 2888, 2873, 2859, 2845, 2831, 2817, 2803, 2789, 2775, 2761, 2747, 2733, 2719, 2705, 2691, 2677, 2664, 2650, 2636, 2623, 2609, 2595, 2582, 2568, 2555, 2541, 2528, 2515, 2501, 2488, 2475, 2461, 2448, 2435, 2422, 2409, 2395, 2382, 2369, 2356, 2343, 2330, 2317, 2305, 2292, 2279, 2266, 2253, 2241, 2228, 2215, 2203, 2190, 2178, 2165, 2153, 2140, 2128, 2116, 2103, 2091, 2079, 2066, 2054, 2042, 2030, 2018, 2006, 1994, 1982, 1970, 1958, 1946, 1934, 1922, 1910, 1899, 1887, 1875, 1864, 1852, 1840, 1829, 1817, 1806, 1794, 1783, 1772, 1760, 1749, 1738, 1726, 1715, 1704, 1693, 1682, 1671, 1660, 1649, 1638, 1627, 1616, 1605, 1594, 1583, 1572, 1562, 1551, 1540, 1530, 1519, 1509, 1498, 1488, 1477, 1467, 1456, 1446, 1436, 1425, 1415, 1405, 1395, 1385, 1375, 1365, 1355, 1345, 1335, 1325, 1315, 1305, 1295, 1285, 1276, 1266, 1256, 1247, 1237, 1227, 1218, 1208, 1199, 1190, 1180, 1171, 1162, 1152, 1143, 1134, 1125, 1115, 1106, 1097, 1088, 1079, 1070, 1061, 1053, 1044, 1035, 1026, 1017, 1009, 1000, 991, 983, 974, 966, 957, 949, 940, 932, 924, 915, 907, 899, 891, 882, 874, 866, 858, 850, 842, 834, 826, 819, 811, 803, 795, 787, 780, 772, 765, 757, 749, 742, 734, 727, 720, 712, 705, 698, 690, 683, 676, 669, 662, 655, 648, 641, 634, 627, 620, 613, 606, 600, 593, 586, 580, 573, 566, 560, 553, 547, 541, 534, 528, 521, 515, 509, 503, 497, 490, 484, 478, 472, 466, 460, 455, 449, 443, 437, 431, 426, 420, 414, 409, 403, 398, 392, 387, 381, 376, 371, 365, 360, 355, 350, 344, 339, 334, 329, 324, 319, 314, 309, 305, 300, 295, 290, 286, 281, 276, 272, 267, 263, 258, 254, 249, 245, 241, 236, 232, 228, 224, 220, 216, 212, 208, 204, 200, 196, 192, 188, 184, 181, 177, 173, 170, 166, 162, 159, 155, 152, 149, 145, 142, 139, 135, 132, 129, 126, 123, 120, 117, 114, 111, 108, 105, 102, 99, 97, 94, 91, 89, 86, 83, 81, 78, 76, 74, 71, 69, 67, 64, 62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 39, 37, 35, 33, 32, 30, 29, 27, 26, 24, 23, 22, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 6, 5, 4, 4, 3, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 26, 27, 29, 30, 32, 33, 35, 37, 39, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 67, 69, 71, 74, 76, 78, 81, 83, 86, 89, 91, 94, 97, 99, 102, 105, 108, 111, 114, 117, 120, 123, 126, 129, 132, 135, 139, 142, 145, 149, 152, 155, 159, 162, 166, 170, 173, 177, 181, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 241, 245, 249, 254, 258, 263, 267, 272, 276, 281, 286, 290, 295, 300, 305, 309, 314, 319, 324, 329, 334, 339, 344, 350, 355, 360, 365, 371, 376, 381, 387, 392, 398, 403, 409, 414, 420, 426, 431, 437, 443, 449, 455, 460, 466, 472, 478, 484, 490, 497, 503, 509, 515, 521, 528, 534, 541, 547, 553, 560, 566, 573, 580, 586, 593, 600, 606, 613, 620, 627, 634, 641, 648, 655, 662, 669, 676, 683, 690, 698, 705, 712, 720, 727, 734, 742, 749, 757, 765, 772, 780, 787, 795, 803, 811, 819, 826, 834, 842, 850, 858, 866, 874, 882, 891, 899, 907, 915, 924, 932, 940, 949, 957, 966, 974, 983, 991, 1000, 1009, 1017, 1026, 1035, 1044, 1053, 1061, 1070, 1079, 1088, 1097, 1106, 1115, 1125, 1134, 1143, 1152, 1162, 1171, 1180, 1190, 1199, 1208, 1218, 1227, 1237, 1247, 1256, 1266, 1276, 1285, 1295, 1305, 1315, 1325, 1335, 1345, 1355, 1365, 1375, 1385, 1395, 1405, 1415, 1425, 1436, 1446, 1456, 1467, 1477, 1488, 1498, 1509, 1519, 1530, 1540, 1551, 1562, 1572, 1583, 1594, 1605, 1616, 1627, 1638, 1649, 1660, 1671, 1682, 1693, 1704, 1715, 1726, 1738, 1749, 1760, 1772, 1783, 1794, 1806, 1817, 1829, 1840, 1852, 1864, 1875, 1887, 1899, 1910, 1922, 1934, 1946, 1958, 1970, 1982, 1994, 2006, 2018, 2030, 2042, 2054, 2066, 2079, 2091, 2103, 2116, 2128, 2140, 2153, 2165, 2178, 2190, 2203, 2215, 2228, 2241, 2253, 2266, 2279, 2292, 2305, 2317, 2330, 2343, 2356, 2369, 2382, 2395, 2409, 2422, 2435, 2448, 2461, 2475, 2488, 2501, 2515, 2528, 2541, 2555, 2568, 2582, 2595, 2609, 2623, 2636, 2650, 2664, 2677, 2691, 2705, 2719, 2733, 2747, 2761, 2775, 2789, 2803, 2817, 2831, 2845, 2859, 2873, 2888, 2902, 2916, 2930, 2945, 2959, 2974, 2988, 3003, 3017, 3032, 3046, 3061, 3075, 3090, 3105, 3120, 3134, 3149, 3164, 3179, 3194, 3209, 3224, 3239, 3254, 3269, 3284, 3299, 3314, 3329, 3344, 3360, 3375, 3390, 3406, 3421, 3436, 3452, 3467, 3483, 3498, 3514, 3529, 3545, 3560, 3576, 3592, 3608, 3623, 3639, 3655, 3671, 3687, 3702, 3718, 3734, 3750, 3766, 3782, 3798, 3815, 3831, 3847, 3863, 3879, 3896, 3912, 3928, 3944, 3961, 3977, 3994, 4010, 4027, 4043, 4060, 4076, 4093, 4110, 4126, 4143, 4160, 4176, 4193, 4210, 4227, 4244, 4261, 4277, 4294, 4311, 4328, 4345, 4362, 4380, 4397, 4414, 4431, 4448, 4465, 4483, 4500, 4517, 4535, 4552, 4569, 4587, 4604, 4622, 4639, 4657, 4674, 4692, 4710, 4727, 4745, 4763, 4780, 4798, 4816, 4834, 4852, 4869, 4887, 4905, 4923, 4941, 4959, 4977, 4995, 5013, 5031, 5050, 5068, 5086, 5104, 5122, 5141, 5159, 5177, 5196, 5214, 5232, 5251, 5269, 5288, 5306, 5325, 5343, 5362, 5381, 5399, 5418, 5436, 5455, 5474, 5493, 5511, 5530, 5549, 5568, 5587, 5606, 5625, 5644, 5663, 5682, 5701, 5720, 5739, 5758, 5777, 5796, 5816, 5835, 5854, 5873, 5893, 5912, 5931, 5951, 5970, 5989, 6009, 6028, 6048, 6067, 6087, 6106, 6126, 6146, 6165, 6185, 6205, 6224, 6244, 6264, 6284, 6303, 6323, 6343, 6363, 6383, 6403, 6423, 6443, 6463, 6483, 6503, 6523, 6543, 6563, 6583, 6603, 6623, 6644, 6664, 6684, 6704, 6725, 6745, 6765, 6786, 6806, 6826, 6847, 6867, 6888, 6908, 6929, 6949, 6970, 6990, 7011, 7032, 7052, 7073, 7094, 7114, 7135, 7156, 7177, 7197, 7218, 7239, 7260, 7281, 7302, 7323, 7344, 7365, 7386, 7407, 7428, 7449, 7470, 7491, 7512, 7533, 7554, 7575, 7597, 7618, 7639, 7660, 7682, 7703, 7724, 7746, 7767, 7788, 7810, 7831, 7853, 7874, 7896, 7917, 7939, 7960, 7982, 8003, 8025, 8047, 8068, 8090, 8112, 8133, 8155, 8177, 8199, 8220, 8242, 8264, 8286, 8308, 8329, 8351, 8373, 8395, 8417, 8439, 8461, 8483, 8505, 8527, 8549, 8571, 8593, 8616, 8638, 8660, 8682, 8704, 8726, 8749, 8771, 8793, 8815, 8838, 8860, 8882, 8905, 8927, 8950, 8972, 8994, 9017, 9039, 9062, 9084, 9107, 9129, 9152, 9174, 9197, 9220, 9242, 9265, 9287, 9310, 9333, 9355, 9378, 9401, 9424, 9446, 9469, 9492, 9515, 9538, 9560, 9583, 9606, 9629, 9652, 9675, 9698, 9721, 9744, 9767, 9790, 9813, 9836, 9859, 9882, 9905, 9928, 9951, 9974, 9997, 10021, 10044, 10067, 10090, 10113, 10137, 10160, 10183, 10206, 10230, 10253, 10276, 10300, 10323, 10346, 10370, 10393, 10416, 10440, 10463, 10487, 10510, 10534, 10557, 10581, 10604, 10628, 10651, 10675, 10698, 10722, 10745, 10769, 10793, 10816, 10840, 10864, 10887, 10911, 10935, 10958, 10982, 11006, 11030, 11053, 11077, 11101, 11125, 11148, 11172, 11196, 11220, 11244, 11268, 11292, 11315, 11339, 11363, 11387, 11411, 11435, 11459, 11483, 11507, 11531, 11555, 11579, 11603, 11627, 11651, 11675, 11699, 11723, 11748, 11772, 11796, 11820, 11844, 11868, 11892, 11917, 11941, 11965, 11989, 12013, 12038, 12062, 12086, 12110, 12135, 12159, 12183, 12207, 12232, 12256, 12280, 12305, 12329, 12353, 12378, 12402, 12427, 12451, 12475, 12500, 12524, 12549, 12573, 12597, 12622, 12646, 12671, 12695, 12720, 12744, 12769, 12793, 12818, 12842, 12867, 12892, 12916, 12941, 12965, 12990, 13014, 13039, 13064, 13088, 13113, 13137, 13162, 13187, 13211, 13236, 13261, 13285, 13310, 13335, 13359, 13384, 13409, 13434, 13458, 13483, 13508, 13533, 13557, 13582, 13607, 13632, 13656, 13681, 13706, 13731, 13756, 13780, 13805, 13830, 13855, 13880, 13904, 13929, 13954, 13979, 14004, 14029, 14054, 14079, 14103, 14128, 14153, 14178, 14203, 14228, 14253, 14278, 14303, 14328, 14353, 14377, 14402, 14427, 14452, 14477, 14502, 14527, 14552, 14577, 14602, 14627, 14652, 14677, 14702, 14727, 14752, 14777, 14802, 14827, 14852, 14877, 14902, 14927, 14952, 14977, 15002, 15027, 15052, 15078, 15103, 15128, 15153, 15178, 15203, 15228, 15253, 15278, 15303, 15328, 15353, 15378, 15403, 15429, 15454, 15479, 15504, 15529, 15554, 15579, 15604, 15629, 15654, 15680, 15705, 15730, 15755, 15780, 15805, 15830, 15855, 15880, 15906, 15931, 15956, 15981, 16006, 16031, 16056, 16081, 16107, 16132, 16157, 16182, 16207, 16232, 16257, 16282, 16308, 16333, 16358, 16383};
	int32_t * tableRead;

	int32_t freq = 0;
	uint32_t phase = 0;
	int32_t pm = 0;
	int32_t lastPM = 0;

	inline int32_t evaluateFromFlash(uint32_t phase) {
		int32_t index = phase >> 20;
		int32_t frac = (phase >> 4) & 0xFFFF;
		return fast_15_16_lerp(big_sine[index], big_sine[index + 1], frac);
	}

	inline void updatePM(int32_t pmInput) {
		pm = pmInput - lastPM;
		lastPM = pmInput;
	}

	inline int32_t evaluatePM(void) {
		phase += freq + pm;
		int32_t index = phase >> 20;
		int32_t frac = (phase >> 4) & 0xFFFF;
		return fast_15_16_lerp_prediff(tableRead[index], frac);
	}

	inline int32_t evaluatePMFM(int32_t fm) {
		phase += fix16_mul(freq, fm) + pm;
		int32_t index = phase >> 20;
		int32_t frac = (phase >> 4) & 0xFFFF;
		return fast_15_16_lerp_prediff(tableRead[index], frac);
	}

	inline int32_t evaluateSineBeat(int32_t newIndex) {
		phase = newIndex * freq;
		// scale to the wavetable size in 16 bit fixed point
		// wavetable size is 4096 (12 bits)
		int32_t index = phase >> 20;
		int32_t frac = (phase >> 4) & 0xFFFF;
		// linear interpolate between 15 bit values and return 12 bit value
		return fast_15_16_lerp_prediff(tableRead[index], frac) >> 7;
	}

};

#endif /* INC_DSP_INLINES_H_ */
