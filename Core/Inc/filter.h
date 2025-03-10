#ifndef INC_FILTER_H_
#define INC_FILTER_H_

#include <stdint.h>
#include "drivers/modem_ll.h"

#ifdef USE_FPU

/**
 * @brief BPF filter with 2200 Hz tone 6 dB preemphasis (it actually attenuates 1200 Hz tone by 6 dB)
 */
static const int16_t bpf1200[8] =
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
static const int16_t bpf1200Inv[8] =
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
static const int16_t bpf300[15] =
{
	186, 8887, 8184, -1662, -10171, -8509, 386, 5394, 386, -8509, -10171, -1662, 8184, 8887, 186,
};

#define BPF_MAX_TAPS 15

//fs=9600 Hz, raised cosine, fc=300 Hz (BR=600 Bd), beta=0.8, N=14, gain=65536
static const int16_t lpf300[14] =
{
	4385, 4515, 4627, 4720, 4793, 4846, 4878, 4878, 4846, 4793, 4720, 4627, 4515, 4385,
};

//I don't remember what are this filter parameters,
//but it seems to be the best among all I have tested
static const int16_t lpf1200[15] =
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
static const int16_t lpf9600[9] = {497, 2360, 7178, 13992, 17478, 13992, 7178, 2360, 497};

#define LPF_MAX_TAPS 15

#define FILTER_MAX_TAPS ((LPF_MAX_TAPS > BPF_MAX_TAPS) ? LPF_MAX_TAPS : BPF_MAX_TAPS)

struct Filter
{
	const int16_t *coeffs;
	uint8_t taps;
	int32_t samples[FILTER_MAX_TAPS];
	uint8_t gainShift;
};

static int32_t filter(struct Filter *filter, int32_t input)
{
	int32_t out = 0;

	for(uint8_t i = filter->taps - 1; i > 0; i--)
		filter->samples[i] = filter->samples[i - 1]; //shift old samples

	filter->samples[0] = input; //store new sample
	for(uint8_t i = 0; i < filter->taps; i++)
	{
		out += (int32_t)filter->coeffs[i] * filter->samples[i];
	}
	return out >> filter->gainShift;
}


#else

/**
 * @brief BPF filter with 2200 Hz tone 6 dB preemphasis (it actually attenuates 1200 Hz tone by 6 dB)
 */
static const int16_t bpf1200[8] =
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
static const int16_t bpf1200Inv[8] =
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
static const int16_t bpf300[15] =
{
	186, 8887, 8184, -1662, -10171, -8509, 386, 5394, 386, -8509, -10171, -1662, 8184, 8887, 186,
};

#define BPF_MAX_TAPS 15

//fs=9600 Hz, raised cosine, fc=300 Hz (BR=600 Bd), beta=0.8, N=14, gain=65536
static const int16_t lpf300[14] =
{
	4385, 4515, 4627, 4720, 4793, 4846, 4878, 4878, 4846, 4793, 4720, 4627, 4515, 4385,
};

//I don't remember what are this filter parameters,
//but it seems to be the best among all I have tested
static const int16_t lpf1200[15] =
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
static const int16_t lpf9600[9] = {497, 2360, 7178, 13992, 17478, 13992, 7178, 2360, 497};

#define LPF_MAX_TAPS 15

#define FILTER_MAX_TAPS ((LPF_MAX_TAPS > BPF_MAX_TAPS) ? LPF_MAX_TAPS : BPF_MAX_TAPS)

struct Filter
{
	const int16_t *coeffs;
	uint8_t taps;
	int32_t samples[FILTER_MAX_TAPS];
	uint8_t gainShift;
};

static int32_t filter(struct Filter *filter, int32_t input)
{
	int32_t out = 0;

	for(uint8_t i = filter->taps - 1; i > 0; i--)
		filter->samples[i] = filter->samples[i - 1]; //shift old samples

	filter->samples[0] = input; //store new sample
	for(uint8_t i = 0; i < filter->taps; i++)
	{
		out += (int32_t)filter->coeffs[i] * filter->samples[i];
	}
	return out >> filter->gainShift;
}

#endif

#endif
