/*
This file is part of VP-Digi.

VP-Digi is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

VP-Digi is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VP-Digi.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "drivers/modem.h"
#include "drivers/systick.h"
#include "ax25.h"
#include "stm32f1xx.h"
#include <math.h>
#include <stdlib.h>
#include "common.h"


/*
 * Configuration for PLL-based data carrier detection
 * DCD_MAXPULSE is the maximum value of the DCD pulse counter
 * DCD_THRES is the threshold value of the DCD pulse counter. When reached the input signal is assumed to be valid
 * DCD_MAXPULSE and DCD_THRES difference sets the DCD "inertia" so that the DCD state won't change rapidly when a valid signal is present
 * DCD_DEC is the DCD pulse counter decrementation value when symbol changes too far from PLL counter zero
 * DCD_INC is the DCD pulse counter incrementation value when symbol changes near the PLL counter zero
 * DCD_PLLTUNE is the DCD timing coefficient when symbol changes, pll_counter = pll_counter * DCD_PLLTUNE
 * The DCD mechanism is described in afsk_demod().
 * All values were selected by trial and error
 */
#define DCD_MAXPULSE 100
#define DCD_THRES 30
#define DCD_DEC 1
#define DCD_INC 7
#define DCD_PLLTUNE 0

#define NN 8 //samples per symbol
#define DACSINELEN 32 //DAC sine table size
#define PLLINC 536870912 //PLL tick increment value
#define PLLLOCKED 0.74 //PLL adjustment value when locked
#define PLLNOTLOCKED 0.50 //PLL adjustment value when not locked


#define PTT_ON GPIOB->BSRR = GPIO_BSRR_BS7
#define PTT_OFF GPIOB->BSRR = GPIO_BSRR_BR7
#define DCD_ON (GPIOC->BSRR = GPIO_BSRR_BR13)
#define DCD_OFF (GPIOC->BSRR = GPIO_BSRR_BS13)

Afsk_config afskCfg;

struct ModState
{
	TxTestMode txTestState; //current TX test mode
	uint16_t dacSine[DACSINELEN]; //sine samples for DAC
	uint8_t dacSineIdx; //current sine sample index
	uint16_t samples_oversampling[4]; //very raw received samples, filled directly by DMA
	uint8_t currentSymbol; //current symbol for NRZI encoding
	uint16_t txDelay; //TXDelay length in number of bytes
	uint16_t txTail; //TXTail length in number of bytes
	uint8_t markFreq; //mark frequency (inter-sample interval)
	uint8_t spaceFreq; //space frequency (inter-sample interval)
	uint16_t baudRate; //baudrate
	int32_t coeffHiI[NN], coeffLoI[NN], coeffHiQ[NN], coeffLoQ[NN]; //correlator IQ coefficients
};

volatile struct ModState modState;


typedef struct
{
	Emphasis emphasis; //preemphasis/deemphasis
	uint8_t rawSymbols; //raw, unsynchronized symbols
	uint8_t syncSymbols; //synchronized symbols
	int16_t rawSample[8]; //input (raw) samples
	int32_t rxSample[16]; //rx samples after pre/deemphasis filter
	uint8_t rxSampleIdx; //index for the array above
	int64_t lpfSample[16]; //rx samples after final filtering
	uint8_t dcd : 1; //DCD state
	uint64_t RMSenergy; //frame energy counter (sum of samples squared)
	uint32_t RMSsampleCount; //number of samples for RMS
	int32_t pll; //bit recovery PLL counter
	int32_t lastPll; //last bit recovery PLL counter value
	int32_t dcdPll; //DCD PLL main counter
	uint8_t dcdLastSymbol; //last symbol for DCD
	uint8_t dcdCounter; //DCD "pulse" counter (incremented when RX signal is correct)
} Demod;

volatile Demod demod1;
volatile Demod demod2;

uint8_t dcd = 0; //multiplexed DCD state from both demodulators

/**
 * @brief BPF filter with 2200 Hz tone 6 dB preemphasis (it actually attenuates 1200 Hz tone by 6 dB)
 */
const int16_t bpf1200[8] = {
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
const int16_t bpf1200inv[8] = {
        -10513,
        -10854,
         9589,
        23884,
         9589,
        -10854,
        -10513,
         -879
};

/**
 * @brief Output LPF filter to remove data faster than 1200 baud
 * It actually is a 600 Hz filter: symbols can change at 1200 Hz, but it takes 2 "ticks" to return to the same symbol - that's why it's 600 Hz
 */
const int16_t lpf1200[15] = {
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



static void afsk_decode(uint8_t symbol, Demod *dem);
static int32_t afsk_demod(int16_t sample, Demod *dem);
static void afsk_ptt(uint8_t state);

uint8_t Afsk_dcdState(void)
{
	return dcd;
}

uint8_t Afsk_isTxTestOngoing(void)
{
	if(modState.txTestState != TEST_DISABLED)
		return 1;

	return 0;
}

void Afsk_clearRMS(uint8_t modemNo)
{
	if(modemNo == 0)
	{
		demod1.RMSenergy = 0;
		demod1.RMSsampleCount = 0;
	}
	else
	{
		demod2.RMSenergy = 0;
		demod2.RMSsampleCount = 0;
	}
}

uint16_t Afsk_getRMS(uint8_t modemNo)
{
	if(modemNo == 0)
	{
		return sqrtf((float)demod1.RMSenergy / (float)demod1.RMSsampleCount);
	}
	return sqrtf((float)demod2.RMSenergy / (float)demod2.RMSsampleCount);
}


/**
 * @brief Set DCD LED
 * @param[in] state 0 - OFF, 1 - ON
 */
static void afsk_dcd(uint8_t state)
{
//	if(state)
//	{
//		GPIOC->BSRR = GPIO_BSRR_BR13;
//		GPIOB->BSRR = GPIO_BSRR_BS5;
//	}
//	else
//	{
//		GPIOC->BSRR = GPIO_BSRR_BS13;
//		GPIOB->BSRR = GPIO_BSRR_BR5;
//	}
}


/**
 * @brief ISR for demodulator
 * Called at 9600 Hz by DMA
 */
void DMA1_Channel2_IRQHandler(void) __attribute__ ((interrupt));
void DMA1_Channel2_IRQHandler(void)
{
	if(DMA1->ISR & DMA_ISR_TCIF2)
	{
		DMA1->IFCR |= DMA_IFCR_CTCIF2;

		int32_t sample = ((modState.samples_oversampling[0] + modState.samples_oversampling[1] + modState.samples_oversampling[2] + modState.samples_oversampling[3]) >> 1) - 4095; //calculate input sample (decimation)
		uint8_t symbol = 0; //output symbol

		//use 2 demodulators
		symbol = (afsk_demod(sample, (Demod*)&demod1) > 0); //demodulate sample
		afsk_decode(symbol, (Demod*)&demod1); //recover bits, decode NRZI and call higher level function

		symbol = (afsk_demod(sample, (Demod*)&demod2) > 0);
		afsk_decode(symbol, (Demod*)&demod2);

		if(demod1.dcd || demod2.dcd) //DCD on any of the demodulators
		{
			dcd = 1;
			afsk_dcd(1);
		}
		else if((demod1.dcd == 0) && (demod2.dcd == 0)) //no DCD on both demodulators
		{
			dcd = 0;
			afsk_dcd(0);
		}
	}
}





/**
 * @brief ISR for pushing DAC samples
 */
void TIM1_UP_IRQHandler(void) __attribute__ ((interrupt));
void TIM1_UP_IRQHandler(void)
{
	TIM1->SR &= ~TIM_SR_UIF;

	modState.dacSineIdx++;
	modState.dacSineIdx &= (DACSINELEN - 1);

	if(afskCfg.usePWM)
	{
		TIM4->CCR1 = modState.dacSine[modState.dacSineIdx];
	}
	else
	{
		GPIOB->ODR &= ~61440; //zero 4 oldest bits
		GPIOB->ODR |= (modState.dacSine[modState.dacSineIdx] << 12); //write sample to 4 oldest bits

	}
}


/**
 * @brief ISR for baudrate generator timer. NRZI encoding is done here.
 */
void TIM3_IRQHandler(void) __attribute__ ((interrupt));
void TIM3_IRQHandler(void)
{
	TIM3->SR &= ~TIM_SR_UIF;

	if(modState.txTestState == TEST_DISABLED) //transmitting normal data
	{
		if(Ax25_getTxBit() == 0) //get next bit and check if it's 0
		{
			modState.currentSymbol = modState.currentSymbol ? 0 : 1; //change symbol - NRZI encoding
		}
		//if 1, no symbol change
	}
	else //transmit test mode
	{
		modState.currentSymbol = modState.currentSymbol ? 0 : 1; //change symbol
	}

	if(modState.currentSymbol) //current symbol is space
	{

		TIM1->CNT = 0;
		TIM1->ARR = modState.spaceFreq;
	}
	else //mark
	{

		TIM1->CNT = 0;
		TIM1->ARR = modState.markFreq;

	}



}


/**
 * @brief Demodulate received sample (4x oversampling)
 * @param[in] sample Received sample
 * @param[in] *dem Demodulator state
 * @return Current tone (0 or 1)
 */
static int32_t afsk_demod(int16_t sample, Demod *dem)
{

	dem->RMSenergy += ((sample >> 1) * (sample >> 1)); //square the sample and add it to the sum
	dem->RMSsampleCount++; //increment number of samples



	if(dem->emphasis != EMPHASIS_NONE) //preemphasis/deemphasis is used
	{
		int32_t out = 0; //filtered output

		for(uint8_t i = 7; i > 0; i--)
			dem->rawSample[i] = dem->rawSample[i - 1]; //shift old samples

		dem->rawSample[0] = sample; //store new sample
		for(uint8_t i = 0; i < 8; i++)
		{
			if(dem->emphasis == PREEMPHASIS)
				out += bpf1200[i] * dem->rawSample[i]; //use preemphasis
			else
				out += bpf1200inv[i] * dem->rawSample[i]; //use deemphasis
		}

		dem->rxSample[dem->rxSampleIdx] = (out >> 15); //store filtered sample
	}
	else //no pre/deemphasis
	{
		dem->rxSample[dem->rxSampleIdx] = sample; //store incoming sample
	}

	int64_t outLoI = 0, outLoQ = 0, outHiI = 0, outHiQ = 0; //output values after correlating

	dem->rxSampleIdx = (dem->rxSampleIdx + 1) % NN; //increment sample pointer and wrap around if needed

	for(uint8_t i = 0; i < NN; i++) {
		int32_t t = dem->rxSample[(dem->rxSampleIdx + i) % NN]; //read sample
		outLoI += t * modState.coeffLoI[i]; //correlate sample
		outLoQ += t * modState.coeffLoQ[i];
		outHiI += t * modState.coeffHiI[i];
		outHiQ += t * modState.coeffHiQ[i];
	}

	uint64_t hi = 0, lo = 0;


	hi = ((outHiI >> 12) * (outHiI >> 12)) + ((outHiQ >> 12) * (outHiQ >> 12)); //calculate output tone levels
	lo = ((outLoI >> 12) * (outLoI >> 12)) + ((outLoQ >> 12) * (outLoQ >> 12));

	//DCD using PLL
	//PLL is running nominally at 1200 Hz (= baudrate)
	//PLL timer is counting up and eventually overflows to a minimal negative value
	//so it crosses zero in the middle
	//tone change should happen somewhere near this zero-crossing (in ideal case of exactly same TX and RX baudrates)
	//nothing is ideal, so we need to have some region around zero where tone change is expected
	//if tone changed inside this region, then we add something to the DCD pulse counter (and adjust counter phase for the counter to be closer to 0)
	//if tone changes outside this region, then we subtract something from the DCD pulse counter
	//if some DCD pulse threshold is reached, then we claim that the incoming signal is correct and set DCD flag
	//when configured properly, it's generally immune to noise, as the detected tone changes much faster than 1200 baud
	//it's also important to set some maximum value for DCD counter, otherwise the DCD is "sticky"

	dem->dcdPll = (signed)((unsigned)(dem->dcdPll) + ((unsigned)PLLINC)); //keep PLL ticking at the frequency equal to baudrate

	uint8_t dcdSymbol = (hi > lo); //get current symbol

	if(dcdSymbol != dem->dcdLastSymbol) //tone changed
	{
		if(abs(dem->dcdPll) < PLLINC) //tone change occurred near zero
			dem->dcdCounter += DCD_INC; //increase DCD counter
		else //tone change occurred far from zero
		{
			if(dem->dcdCounter >= DCD_DEC) //avoid overflow
				dem->dcdCounter -= DCD_DEC; //decrease DCD counter
		}
		dem->dcdPll = (int)(dem->dcdPll * DCD_PLLTUNE); //adjust PLL
	}

	dem->dcdLastSymbol = dcdSymbol; //store last symbol for symbol change detection


	if(dem->dcdCounter > DCD_MAXPULSE) //maximum DCD counter value reached
		dem->dcdCounter = DCD_MAXPULSE; //avoid "sticky" DCD and counter overflow

	if(dem->dcdCounter > DCD_THRES) //DCD threshold reached
		dem->dcd = 1; //DCD!
	else //below DCD threshold
		dem->dcd = 0; //no DCD

	//filter out signal faster than 1200 baud
	int64_t out = 0;

	for(uint8_t i = 14; i > 0; i--)
	    dem->lpfSample[i] = dem->lpfSample[i - 1];

	dem->lpfSample[0] = (int64_t)hi - (int64_t)lo;
	for(uint8_t i = 0; i < 15; i++)
	{
	    out += lpf1200[i] * dem->lpfSample[i];
	}

	return out > 0;
}




/**
 * @brief Decode received symbol: bit recovery, NRZI decoding and pass the decoded bit to higher level protocol
 * @param[in] symbol Received symbol
 * @param *dem Demodulator state
 */
static void afsk_decode(uint8_t symbol, Demod *dem)
{

	//This function provides bit/clock recovery and NRZI decoding
	//Bit recovery is based on PLL which is described in the function above (DCD PLL)
	//Current symbol is sampled at PLL counter overflow, so symbol transition should occur at PLL counter zero
	dem->lastPll = dem->pll; //store last clock state

	dem->pll = (signed)((unsigned)(dem->pll) + (unsigned)PLLINC); //keep PLL running

	dem->rawSymbols <<= 1; //store received unsynchronized symbol
	dem->rawSymbols |= (symbol & 1);



	if ((dem->pll < 0) && (dem->lastPll > 0)) //PLL counter overflow, sample symbol, decode NRZI and process in higher layer
	{
		dem->syncSymbols <<= 1;	//shift recovered (received, synchronized) bit register

		uint8_t t = dem->rawSymbols & 0x07; //take last three symbols for sampling. Seems that 1 symbol is not enough, but 3 symbols work well
		if(t == 0b111 || t == 0b110 || t == 0b101 || t == 0b011) //if there are 2 or 3 ones, then the received symbol is 1
		{
			dem->syncSymbols |= 1; //push to recovered symbols register
		}
		//if there 2 or 3 zeros, no need to add anything to the register

		//NRZI decoding
		if (((dem->syncSymbols & 0x03) == 0b11) || ((dem->syncSymbols & 0x03) == 0b00)) //two last symbols are the same - no symbol transition - decoded bit 1
		{
			Ax25_bitParse(1, (dem == &demod1) ? 0 : 1);
		}
		else //symbol transition - decoded bit 0
		{
			Ax25_bitParse(0, (dem == &demod1) ? 0 : 1);
		}
	}

	if(((dem->rawSymbols & 0x03) == 0b10) || ((dem->rawSymbols & 0x03) == 0b01)) //if there was a symbol transition, adjust PLL
	{

		if (Ax25_getRxStage((dem == &demod1) ? 0 : 1) != RX_STAGE_FRAME) //not in a frame
		{
			dem->pll = (int)(dem->pll * PLLNOTLOCKED); //adjust PLL faster
		}
		else //in a frame
		{
			dem->pll = (int)(dem->pll * PLLLOCKED); //adjust PLL slower
		}
	}

}


/**
 * @brief Start or restart TX test mode
 * @param[in] type TX test type: TEST_MARK, TEST_SPACE or TEST_ALTERNATING
 */
void Afsk_txTestStart(TxTestMode type)
{
	if(modState.txTestState != TEST_DISABLED) //TX test is already running
		Afsk_txTestStop(); //stop this test

	afsk_ptt(1); //PTT on
	modState.txTestState = type;

	//DAC timer
	TIM1->PSC = 15; //64/16=4 MHz
	TIM1->DIER = TIM_DIER_UIE; //enable interrupt
	TIM1->CR1 |= TIM_CR1_CEN; //enable timer


	TIM2->CR1 &= ~TIM_CR1_CEN; //disable RX timer


	NVIC_DisableIRQ(DMA1_Channel2_IRQn); //disable RX DMA interrupt
	NVIC_EnableIRQ(TIM1_UP_IRQn); //enable timer 1 for PWM

	if(type == TEST_MARK)
	{
		TIM1->ARR = modState.markFreq;
	} else if(type == TEST_SPACE)
	{
		TIM1->ARR = modState.spaceFreq;
	}
	else //alternating tones
	{
		//enable baudrate generator
		TIM3->PSC = 63; //64/64=1 MHz
		TIM3->DIER = TIM_DIER_UIE; //enable interrupt
		TIM3->ARR = modState.baudRate; //set timer interval
		TIM3->CR1 = TIM_CR1_CEN; //enable timer
		NVIC_EnableIRQ(TIM3_IRQn); //enable interrupt in NVIC
	}
}

/**
 * @brief Stop TX test mode
 */
void Afsk_txTestStop(void)
{
	modState.txTestState = TEST_DISABLED;

	TIM3->CR1 &= ~TIM_CR1_CEN; //turn off timers
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 |= TIM_CR1_CEN; //enable RX timer

	NVIC_DisableIRQ(TIM3_IRQn);
	NVIC_DisableIRQ(TIM1_UP_IRQn);
	NVIC_EnableIRQ(DMA1_Channel2_IRQn);

	afsk_ptt(0); //PTT off
}

/**
 * @brief Configure and start TX
 * @warning Transmission should be started with Ax25_transmitBuffer
 */
void Afsk_transmitStart(void)
{
	afsk_ptt(1); //PTT on

	TIM1->PSC = 15;
	TIM1->DIER |= TIM_DIER_UIE;


	TIM3->PSC = 63;
	TIM3->DIER |= TIM_DIER_UIE;
	TIM3->ARR = modState.baudRate;

	TIM3->CR1 = TIM_CR1_CEN;
	TIM1->CR1 = TIM_CR1_CEN;
	TIM2->CR1 &= ~TIM_CR1_CEN;

	NVIC_DisableIRQ(DMA1_Channel2_IRQn);
	NVIC_EnableIRQ(TIM1_UP_IRQn);
	NVIC_EnableIRQ(TIM3_IRQn);

}


/**
 * @brief Stop TX and go back to RX
 */
void Afsk_transmitStop(void)
{

	TIM2->CR1 |= TIM_CR1_CEN;
	TIM3->CR1 &= ~TIM_CR1_CEN;
	TIM1->CR1 &= ~TIM_CR1_CEN;

	NVIC_DisableIRQ(TIM1_UP_IRQn);
	NVIC_DisableIRQ(TIM3_IRQn);
	NVIC_EnableIRQ(DMA1_Channel2_IRQn);

	afsk_ptt(0);

	TIM4->CCR1 = 44; //set around 50% duty cycle
}

/**
 * @brief Controls PTT output
 * @param[in] state 0 - PTT off, 1 - PTT on
 */
static void afsk_ptt(uint8_t state)
{
	if(state)
		PTT_ON;
	else
		PTT_OFF;
}



/**
 * @brief Initialize AFSK module
 */
void Afsk_init(void)
{
	/**
	 * TIM1 is used for pushing samples to DAC (R2R or PWM)
	 * TIM3 is the baudrate generator for TX
	 * TIM4 is the PWM generator with no software interrupt
	 * TIM2 is the RX sampling timer with no software interrupt, but it directly calls DMA
	 */

	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;



//	GPIOC->CRH |= GPIO_CRH_MODE13_1; //DCD LED on PC13
//	GPIOC->CRH &= ~GPIO_CRH_MODE13_0;
//	GPIOC->CRH &= ~GPIO_CRH_CNF13;

	GPIOB->CRH &= ~4294901760; //R2R output on PB12-PB15
	GPIOB->CRH |= 572653568;

	GPIOA->CRL &= ~GPIO_CRL_CNF0; //ADC input on PA0
	GPIOA->CRL &= ~GPIO_CRL_MODE0;


	GPIOB->CRL |= GPIO_CRL_MODE7_1; //PTT output on PB7
	GPIOB->CRL &= ~GPIO_CRL_MODE7_0;
	GPIOB->CRL &= ~GPIO_CRL_CNF7;

//	GPIOB->CRL |= GPIO_CRL_MODE5_1; //2nd DCD LED on PB5
//	GPIOB->CRL &= ~GPIO_CRL_MODE5_0;
//	GPIOB->CRL &= ~GPIO_CRL_CNF5;


	RCC->CFGR |= RCC_CFGR_ADCPRE_1; //ADC prescaler /6
	RCC->CFGR &= ~RCC_CFGR_ADCPRE_0;

	ADC1->CR2 |= ADC_CR2_CONT; //continuous conversion
	ADC1->CR2 |= ADC_CR2_EXTSEL;
	ADC1->SQR1 &= ~ADC_SQR1_L; //1 conversion
    ADC1->SMPR2 |= ADC_SMPR2_SMP0_2; //41.5 cycle sampling
	ADC1->SQR3 &= ~ADC_SQR3_SQ1; //channel 0 is first in the sequence
    ADC1->CR2 |= ADC_CR2_ADON; //ADC on

	ADC1->CR2 |= ADC_CR2_RSTCAL; //calibrate ADC
	while(ADC1->CR2 & ADC_CR2_RSTCAL);
	ADC1->CR2 |= ADC_CR2_CAL;
	while(ADC1->CR2 & ADC_CR2_CAL);

	ADC1->CR2 |= ADC_CR2_EXTTRIG;
	ADC1->CR2 |= ADC_CR2_SWSTART; //start ADC conversion

	//prepare DMA
	DMA1_Channel2->CCR |= DMA_CCR_MSIZE_0; //16 bit memory region
	DMA1_Channel2->CCR &= ~DMA_CCR_MSIZE_1;
    DMA1_Channel2->CCR |= DMA_CCR_PSIZE_0;
	DMA1_Channel2->CCR &= ~DMA_CCR_PSIZE_1;

	DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_CIRC| DMA_CCR_TCIE; //circular mode, memory increment and interrupt
    DMA1_Channel2->CNDTR = 4; //4 samples
	DMA1_Channel2->CPAR = (uint32_t)&(ADC1->DR); //ADC data register address
	DMA1_Channel2->CMAR = (uint32_t)modState.samples_oversampling; //sample buffer address
	DMA1_Channel2->CCR |= DMA_CCR_EN; //enable DMA

	NVIC_EnableIRQ(DMA1_Channel2_IRQn);

	TIM2->PSC = 15; //64/16=4 MHz
	TIM2->DIER |= TIM_DIER_UDE; //enable calling DMA on timer tick
	TIM2->ARR = 103; //4MHz / 104 =~38400 Hz (4*9600 Hz for 4x oversampling)
	TIM2->CR1 |= TIM_CR1_CEN; //enable timer

	modState.markFreq = 103; //set mark frequency
	modState.spaceFreq = 55; //set space frequency

	modState.baudRate = 832; //set baudrate



	for(uint8_t i = 0; i < NN; i++) //calculate correlator coefficients
	{
		modState.coeffLoI[i] = 4095.f * cosf(2.f * 3.1416f * (float)i / (float)NN);
		modState.coeffLoQ[i] = 4095.f * sinf(2.f * 3.1416f * (float)i / (float)NN);
		modState.coeffHiI[i] = 4095.f * cosf(2.f * 3.1416f * (float)i / (float)NN * 2200.f / 1200.f);
		modState.coeffHiQ[i] = 4095.f * sinf(2.f * 3.1416f * (float)i / (float)NN * 2200.f / 1200.f);
	}


	for(uint8_t i = 0; i < DACSINELEN; i++) //calculate DAC sine samples
	{
		if(afskCfg.usePWM)
			modState.dacSine[i] = ((sinf(2.f * 3.1416f * (float)i / (float)DACSINELEN) + 1.f) * 45.f);
		else
			modState.dacSine[i] = ((7.f * sinf(2.f * 3.1416f * (float)i / (float)DACSINELEN)) + 8.f);
	}

	if(afskCfg.flatAudioIn) //when used with flat audio input, use deemphasis and flat modems
	{
		demod1.emphasis = EMPHASIS_NONE;
		demod2.emphasis = DEEMPHASIS;
	}
	else //when used with normal (filtered) audio input, use flat and preemphasis modems
	{
		demod1.emphasis = EMPHASIS_NONE;
		demod2.emphasis = PREEMPHASIS;
	}

	if(afskCfg.usePWM)
	{
		RCC->APB1ENR |= RCC_APB1ENR_TIM4EN; //configure timer

		GPIOB->CRL |= GPIO_CRL_CNF6_1; //configure pin for PWM
		GPIOB->CRL |= GPIO_CRL_MODE6;
		GPIOB->CRL &= ~GPIO_CRL_CNF6_0;

		//set up PWM generation
		TIM4->PSC = 7; //64MHz/8=8MHz
		TIM4->ARR = 80; //8MHz/90=100kHz

		TIM4->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
		TIM4->CCER |= TIM_CCER_CC1E;
		TIM4->CCR1 = 44; //initial duty cycle

		TIM4->CR1 |= TIM_CR1_CEN;
	}

}





/**
 * @}
 */
