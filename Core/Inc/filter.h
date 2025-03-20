#ifndef INC_FILTER_H_
#define INC_FILTER_H_

#include <stdint.h>
#include "drivers/modem_ll.h"

#ifdef USE_FPU

typedef float sample_t;
typedef float coeff_t;

/**
 * @brief BPF filter with 2200 Hz tone 6 dB preemphasis (it actually attenuates 1200 Hz tone by 6 dB)
 */
static const coeff_t bpf1200[8] =
{
		0.022217f, -0.409485f, -0.016907f, 0.594879f, -0.016907f, -0.409485f, 0.022217f, 0.064209f
};

/**
 * @brief BPF filter with 2200 Hz tone 6 dB deemphasis
 */
static const coeff_t bpf1200Inv[8] =
{
		-0.320831f, -0.331238f, 0.292633f, 0.728882f, 0.292633f, -0.331238f, -0.320831f, -0.026825f
};

//fs=9600, rectangular, fc1=1500, fc2=1900, 0 dB @ 1600 Hz and 1800 Hz, N = 15
static const coeff_t bpf300[15] =
{
		0.002838f, 0.135605f, 0.124878f, -0.025360f, -0.155197f, -0.129837f, 0.005890f, 0.082306f, 0.005890f, -0.129837f, -0.155197f, -0.025360f, 0.124878f, 0.135605f, 0.002838f
};

#define BPF_MAX_TAPS 15

//fs=9600 Hz, raised cosine, fc=300 Hz (BR=600 Bd), beta=0.8, N=14
static const coeff_t lpf300[14] =
{
		0.066910f, 0.068893f, 0.070602f, 0.072021f, 0.073135f, 0.073944f, 0.074432f, 0.074432f, 0.073944f, 0.073135f, 0.072021f, 0.070602f, 0.068893f, 0.066910f
};

//I don't remember what are this filter parameters,
//but it seems to be the best among all I have tested
static const coeff_t lpf1200[15] =
{
		-0.187012f, -0.182312f, -0.076385f, 0.125885f, 0.386932f, 0.645508f, 0.835083f, 0.904633f, 0.835083f, 0.645508f, 0.386932f, 0.125885f, -0.076385f, -0.182312f, -0.187012f
};

//fs=38400 Hz, Gaussian, fc=4800 Hz (9600 Bd), N=9, gain=65536
//seems like there is almost no difference between N=9 and any higher order
static const coeff_t lpf9600[9] = {0.007584f, 0.036011f, 0.109528f, 0.213501f, 0.266693f, 0.213501f, 0.109528f, 0.036011f, 0.007584f};

#define LPF_MAX_TAPS 15

#define FILTER_MAX_TAPS ((LPF_MAX_TAPS > BPF_MAX_TAPS) ? LPF_MAX_TAPS : BPF_MAX_TAPS)


#else

typedef int32_t sample_t;
typedef int16_t coeff_t;

/**
 * @brief BPF filter with 2200 Hz tone 6 dB preemphasis (it actually attenuates 1200 Hz tone by 6 dB)
 */
static const coeff_t bpf1200[8] =
{
      728,
    -13418,
     -554,
    19493,
     -554,
    -13418,
      728,
     2104
};

/**
 * @brief BPF filter with 2200 Hz tone 6 dB deemphasis
 */
static const coeff_t bpf1200Inv[8] =
{
	-10513,
	-10854,
	9589,
	23884,
	9589,
	-10854,
	-10513,
	-879
};

//fs=9600, rectangular, fc1=1500, fc2=1900, 0 dB @ 1600 Hz and 1800 Hz, N = 15, gain 65536
static const coeff_t bpf300[15] =
{
	186, 8887, 8184, -1662, -10171, -8509, 386, 5394, 386, -8509, -10171, -1662, 8184, 8887, 186,
};

#define BPF_MAX_TAPS 15

//fs=9600 Hz, raised cosine, fc=300 Hz (BR=600 Bd), beta=0.8, N=14, gain=65536
static const coeff_t lpf300[14] =
{
	4385, 4515, 4627, 4720, 4793, 4846, 4878, 4878, 4846, 4793, 4720, 4627, 4515, 4385,
};

//I don't remember what are this filter parameters,
//but it seems to be the best among all I have tested
static const coeff_t lpf1200[15] =
{
	-6128,
	-5974,
	-2503,
	4125,
	12679,
	21152,
	27364,
	29643,
	27364,
	21152,
	12679,
	4125,
	-2503,
	-5974,
	-6128
};

//fs=38400 Hz, Gaussian, fc=4800 Hz (9600 Bd), N=9, gain=65536
//seems like there is almost no difference between N=9 and any higher order
static const coeff_t lpf9600[9] = {497, 2360, 7178, 13992, 17478, 13992, 7178, 2360, 497};

#define LPF_MAX_TAPS 15

#define FILTER_MAX_TAPS ((LPF_MAX_TAPS > BPF_MAX_TAPS) ? LPF_MAX_TAPS : BPF_MAX_TAPS)


#endif

struct Filter
{
	const coeff_t *coeffs; /**< Filer coefficients */
	uint8_t taps; /**< Filter length */
	sample_t samples[FILTER_MAX_TAPS]; /** Previous input samples */
	uint8_t gainShift; /**< Shift to be applied to output samples, unused on FPU */
};

/**
 * @brief Perform filtering using FIR filter
 * @param *filter FIR filter
 * @param Input sample
 * @return Filtered sample
 */
static sample_t filter(struct Filter *filter, sample_t input)
{
	sample_t out = 0;

	for(uint8_t i = filter->taps - 1; i > 0; i--)
		filter->samples[i] = filter->samples[i - 1]; //shift old samples

	filter->samples[0] = input; //store new sample
	for(uint8_t i = 0; i < filter->taps; i++)
	{
		out += (sample_t)filter->coeffs[i] * filter->samples[i];
	}
	return out
#ifndef USE_FPU
				>> filter->gainShift;
#endif
			;
}

#endif
