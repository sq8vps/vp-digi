/*
Copyright 2020-2025 Piotr Wilkon
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

#ifndef DRIVERS_MODEM_LL_STM32F302_H_
#define DRIVERS_MODEM_LL_STM32F302_H_

#if defined(STM32F302xC)

#define USE_FPU 1 /**< Use FPU - F302 has one */
#define MODEM_LL_DAC_MAX 4095 /**< Maximum value for DAC - 4095 for 12-bit DAC */

#include <stdint.h>
#include "stm32f3xx.h"

/**
 * TIM1 is used for pushing samples to DAC (clocked at 18 MHz)
 * TIM3 is the baudrate generator for TX (clocked at 18 MHz)
 * TIM2 is the RX sampling timer with no software interrupt, but it directly calls DMA
 */

#define MODEM_LL_DMA_INTERRUPT_HANDLER DMA1_Channel2_IRQHandler
#define MODEM_LL_DAC_INTERRUPT_HANDLER TIM1_UP_TIM16_IRQHandler
#define MODEM_LL_BAUDRATE_TIMER_INTERRUPT_HANDLER TIM3_IRQHandler

#define MODEM_LL_DMA_IRQ DMA1_Channel2_IRQn
#define MODEM_LL_DAC_IRQ TIM1_UP_TIM16_IRQn
#define MODEM_LL_BAUDRATE_TIMER_IRQ TIM3_IRQn

#define MODEM_LL_DMA_TRANSFER_COMPLETE_FLAG (DMA1->ISR & DMA_ISR_TCIF2)
#define MODEM_LL_DMA_CLEAR_TRANSFER_COMPLETE_FLAG() (DMA1->IFCR |= DMA_IFCR_CTCIF2)

#define MODEM_LL_BAUDRATE_TIMER_CLEAR_INTERRUPT_FLAG() (TIM3->SR &= ~TIM_SR_UIF)
#define MODEM_LL_BAUDRATE_TIMER_ENABLE() (TIM3->CR1 = TIM_CR1_CEN)
#define MODEM_LL_BAUDRATE_TIMER_DISABLE() (TIM3->CR1 &= ~TIM_CR1_CEN)
#define MODEM_LL_BAUDRATE_TIMER_SET_RELOAD_VALUE(val) (TIM3->ARR = (val))

#define MODEM_LL_DAC_TIMER_CLEAR_INTERRUPT_FLAG (TIM1->SR &= ~TIM_SR_UIF)
#define MODEM_LL_DAC_TIMER_SET_RELOAD_VALUE(val) (TIM1->ARR = (val))
#define MODEM_LL_DAC_TIMER_SET_CURRENT_VALUE(val) (TIM1->CNT = (val))
#define MODEM_LL_DAC_TIMER_ENABLE() (TIM1->CR1 |= TIM_CR1_CEN)
#define MODEM_LL_DAC_TIMER_DISABLE() (TIM1->CR1 &= ~TIM_CR1_CEN)

#define MODEM_LL_ADC_TIMER_ENABLE() (TIM2->CR1 |= TIM_CR1_CEN)
#define MODEM_LL_ADC_TIMER_DISABLE() (TIM2->CR1 &= ~TIM_CR1_CEN)

#define MODEM_LL_DAC_PUT_VALUE(value) (DAC1->DHR12R1 = (value))

static inline void MODEM_LL_DCD_LED_ON(void)                           
{                                 
    GPIOB->BSRR = GPIO_BSRR_BR_8; 
    GPIOB->BSRR = GPIO_BSRR_BS_9; 
}                   

static inline void MODEM_LL_DCD_LED_OFF(void)                               
{                                 
    GPIOB->BSRR = GPIO_BSRR_BR_8; 
    GPIOB->BSRR = GPIO_BSRR_BR_9; 
}                     

#define MODEM_LL_SET_TX_ATTENUATOR(state) (GPIOA->BSRR = (state ? GPIO_BSRR_BR_3 : GPIO_BSRR_BS_3))

/**
 * @brief Enable PTT
 * @param output Output number (AIOC only, meaningless on other platforms)
 */
static inline void MODEM_LL_PTT_ON(uint8_t output)
{
    GPIOB->BSRR = GPIO_BSRR_BR_9;
    GPIOB->BSRR = GPIO_BSRR_BS_8;
    switch (output)
    {
    case 0:
        GPIOA->BSRR = GPIO_BSRR_BS_0;
        break;
    case 1:
        GPIOA->BSRR = GPIO_BSRR_BS_1;
        break;
    }
}

/**
 * @brief Enable PTT
 * @param output Output number (AIOC only, meaningless on other platforms)
 */
static inline void MODEM_LL_PTT_OFF(uint8_t output)
{
    GPIOB->BSRR = GPIO_BSRR_BR_8;
    GPIOB->BSRR = GPIO_BSRR_BR_9;
    switch (output)
    {
    case 0:
        GPIOA->BSRR = GPIO_BSRR_BR_0;
        break;
    case 1:
        GPIOA->BSRR = GPIO_BSRR_BR_1;
        break;
    }
}

/**
 * @brief Initialize clocks
 */
static void MODEM_LL_INITIALIZE_RCC(void)                                               
{                                       
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;  
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;  
    RCC->AHBENR |= RCC_AHBENR_ADC12EN; 
    RCC->APB1ENR |= RCC_APB1ENR_DAC1EN; 
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;   
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; 
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; 
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; 
}

/**
 * @brief Initialize PTT outputs and LEDs
 */
static void MODEM_LL_INITIALIZE_OUTPUTS(void)                                                        
{                                               
    /* DCD and PTT LEDs: between PB8 and PB9 */ 
    GPIOB->MODER &= ~GPIO_MODER_MODER8;         
    GPIOB->MODER |= GPIO_MODER_MODER8_0;        
    GPIOB->OTYPER &= ~GPIO_OTYPER_OT_8;         
    GPIOB->BSRR = GPIO_BSRR_BR_8;               
    GPIOB->MODER &= ~GPIO_MODER_MODER9;         
    GPIOB->MODER |= GPIO_MODER_MODER9_0;        
    GPIOB->OTYPER &= ~GPIO_OTYPER_OT_9;         
    GPIOB->BSRR = GPIO_BSRR_BR_9;               
    /* PTT: PA0, PA1 */                         
    GPIOA->MODER &= ~GPIO_MODER_MODER0;         
    GPIOA->MODER |= GPIO_MODER_MODER0_0;        
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT_0;         
    GPIOA->BSRR = GPIO_BSRR_BR_0;               
    GPIOA->MODER &= ~GPIO_MODER_MODER1;         
    GPIOA->MODER |= GPIO_MODER_MODER1_0;        
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT_1;         
    GPIOA->BSRR = GPIO_BSRR_BR_1;          
    /* PA3: open-drain TX attenuator */
    GPIOA->MODER &= ~GPIO_MODER_MODER3;
    GPIOA->MODER |= GPIO_MODER_MODER3_0;
    GPIOA->OTYPER |= GPIO_OTYPER_OT_3;
    GPIOA->BSRR = GPIO_BSRR_BS_3; //open drain = no attenuation by default
}

/**
 * @brief Set RX gain
 * @param gain New RX gain: 1, 2, 4, 8 or 16
 * @note Invalid values have no effect
 * @note Only used for AIOC rev. >= 1.2, has no effect on other platforms 
 */
static void MODEM_LL_SET_GAIN(uint8_t gain)
{
    switch(gain)
    {
        case 1:
            //adc gain=1, switch to follower mode later
            OPAMP2->CSR &= ~OPAMP_CSR_PGGAIN; //bias gain=16
            OPAMP2->CSR |= OPAMP_CSR_PGGAIN_0 | OPAMP_CSR_PGGAIN_1;
            break;
        case 2:
            OPAMP2->CSR &= ~OPAMP_CSR_PGGAIN; //bias gain=8
            OPAMP2->CSR |= OPAMP_CSR_PGGAIN_1;
            OPAMP1->CSR &= ~OPAMP_CSR_PGGAIN; //adc gain=2
            //3.3%*2*8=52.8%
            break;
        case 4:
            OPAMP2->CSR &= ~OPAMP_CSR_PGGAIN; //bias gain=4
            OPAMP2->CSR |= OPAMP_CSR_PGGAIN_0;
            OPAMP1->CSR &= ~OPAMP_CSR_PGGAIN; //adc gain=4
            OPAMP1->CSR |= OPAMP_CSR_PGGAIN_0;
            //3.3%*4*4=52.8%
            break;
        case 8:
            OPAMP2->CSR &= ~OPAMP_CSR_PGGAIN; //bias gain=2
            OPAMP1->CSR &= ~OPAMP_CSR_PGGAIN; //adc gain=8
            OPAMP1->CSR |= OPAMP_CSR_PGGAIN_1;
            //3.3%*8*2=52.8%
            break;
        case 16:
            //bias gain=1, switch to follower mode later
            OPAMP1->CSR &= ~OPAMP_CSR_PGGAIN; //adc gain=16
            OPAMP1->CSR |= OPAMP_CSR_PGGAIN_0 | OPAMP_CSR_PGGAIN_1;
            //3.3%*16*1=52.8%
            break;
        default:
        	return;
    }

    if(1 == gain)
    {
        //adc gain at 1 (follower), bias gain at 16 (just enable PGA here)
        OPAMP1->CSR |= OPAMP_CSR_VMSEL;
        OPAMP2->CSR &= ~OPAMP_CSR_VMSEL;
        OPAMP2->CSR |= OPAMP_CSR_VMSEL_1;
    }
    else if(16 == gain)
    {
        //adc gain at 16 (just enable PGA here), bias gain at 1 (follower)
        OPAMP1->CSR &= ~OPAMP_CSR_VMSEL;
        OPAMP1->CSR |= OPAMP_CSR_VMSEL_1;
        OPAMP2->CSR |= OPAMP_CSR_VMSEL;
    }
    else
    {
    	//in any other case, enable PGA on both op amps
        OPAMP1->CSR &= ~OPAMP_CSR_VMSEL;
        OPAMP1->CSR |= OPAMP_CSR_VMSEL_1;
        OPAMP2->CSR &= ~OPAMP_CSR_VMSEL;
        OPAMP2->CSR |= OPAMP_CSR_VMSEL_1;
    }
}

/**
 * @brief Initialize ADC
 * @return Utilized ADC pointer
 */
static volatile ADC_TypeDef* MODEM_LL_INITIALIZE_ADC(int32_t *bias)
{                         
    //OPAMP2 as ADC bias generator 
    OPAMP2->CSR |= OPAMP_CSR_FORCEVP; //reference voltage as non-inverting input
    OPAMP2->CSR &= ~OPAMP_CSR_VMSEL; //programmable gain amplifier mode
    OPAMP2->CSR |= OPAMP_CSR_VMSEL_1;
    OPAMP2->CSR &= ~OPAMP_CSR_CALSEL; //reference = 3.3% VDDA
    OPAMP2->CSR &= ~OPAMP_CSR_PGGAIN; //x16 gain to get 52.8% VDDA bias
    OPAMP2->CSR |= OPAMP_CSR_PGGAIN_0 | OPAMP_CSR_PGGAIN_1;
    OPAMP2->CSR &= ~OPAMP_CSR_TSTREF; //make sure reference voltage is enabled
    OPAMP2->CSR |= OPAMP_CSR_OPAMPxEN; //enable op amp

    //OPAMP1 as ADC preamp
    OPAMP1->CSR &= ~OPAMP_CSR_FORCEVP; //make sure reference voltage is not used
    OPAMP1->CSR &= ~OPAMP_CSR_VPSEL_1; //PA5 (signal input) as non-inverting inptu
    OPAMP1->CSR |= OPAMP_CSR_VPSEL_0;
    OPAMP1->CSR |= OPAMP_CSR_VMSEL; //input follower mode for now
    OPAMP1->CSR |= OPAMP_CSR_OPAMPxEN; //enable op amp

    MODEM_LL_SET_GAIN(1);

    /* ADC input: PB2 (ADC2 channel 12) or PA5 via OPAMP1 (ADC1 channel 3) */               
    GPIOB->MODER |= GPIO_MODER_MODER2; 
    GPIOA->MODER |= GPIO_MODER_MODER5; 
	/*/4 prescaler */                  
	RCC->CFGR2 &= ~RCC_CFGR2_ADCPRE12; 
	RCC->CFGR2 |= RCC_CFGR2_ADCPRE12_DIV4;    

	//configure ADC1 channel 3 first and check if there is bias (AIOC rev. >= 1.2)
	ADC1->CFGR |= ADC_CFGR_CONT;        
	ADC1->CFGR &= ~ADC_CFGR_EXTEN;            
	/* 61.5 cycle sampling = 292 kHz */      
	ADC1->SMPR1 &= ~ADC_SMPR1_SMP3;    
	ADC1->SMPR1 |= ADC_SMPR1_SMP3_2 | ADC_SMPR1_SMP3_0;   
	ADC1->SQR1 &= ~ADC_SQR1_SQ1;
	ADC1->SQR1 |= (3 << ADC_SQR1_SQ1_Pos); //channel 3
	ADC1->SQR1 &= ~ADC_SQR1_L; //single conversion
	ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_3;
	/* Enable voltage regulator and perform calibration */    
	ADC1->CR &= ~ADC_CR_ADVREGEN;
	ADC1->CR |= ADC_CR_ADVREGEN_0;
	HAL_Delay(20);
	ADC1->CR &= ~ADC_CR_ADCALDIF;
	ADC1->CR |= ADC_CR_ADCAL;
	while(ADC1->CR & ADC_CR_ADCAL)
		;
	HAL_Delay(20);
	/* Enable ADC */
	ADC1->CR |= ADC_CR_ADEN;
	while(!(ADC1->ISR & ADC_ISR_ADRDY))
		;
	ADC1->CR |= ADC_CR_ADSTART;

	*bias = 4325; //ADC bias is around 4325, because the opamp generates the bias at 3.3%*16=52.8% of VDDA

	//collect some samples and average them
	ADC1->DR;
	uint32_t sum = 0;
	for(uint8_t i = 0; i < 16; i++)
	{
		while(!(ADC1->ISR & ADC_ISR_EOC))
			;
		sum += ADC1->DR;
	}
	sum /= 16;

	//check whether the average is between 40% and 60% of the ADC range, that is, whether there is a proper bias present
	if((sum < (uint32_t)(0.4f * 4096.f)) || ((sum > (uint32_t)(0.6f * 4096.f))))
	{
		//if not, disable ADC1 and set up ADC2 (old AIOC revision)
		ADC1->CR |= ADC_CR_ADSTP;
		while(ADC1->CR & ADC_CR_ADSTP)
			;
		ADC1->CR |= ADC_CR_ADDIS;

		ADC2->CFGR |= ADC_CFGR_CONT;        
		ADC2->CFGR &= ~ADC_CFGR_EXTEN;            
		/* 61.5 cycle sampling = 292 kHz */      
		ADC2->SMPR2 &= ~ADC_SMPR2_SMP12;    
		ADC2->SMPR2 |= ADC_SMPR2_SMP12_2 | ADC_SMPR2_SMP12_0;   
		ADC2->SQR1 &= ~ADC_SQR1_SQ1;
		ADC2->SQR1 |= (12 << ADC_SQR1_SQ1_Pos); //channel 12
		ADC2->SQR1 &= ~ADC_SQR1_L; //single conversion
		ADC2->DIFSEL &= ~ADC_DIFSEL_DIFSEL_12;
		/* Enable voltage regulator and perform calibration */    
		ADC2->CR &= ~ADC_CR_ADVREGEN;
		ADC2->CR |= ADC_CR_ADVREGEN_0;
		HAL_Delay(20);
		ADC2->CR &= ~ADC_CR_ADCALDIF;
		ADC2->CR |= ADC_CR_ADCAL;
		while(ADC2->CR & ADC_CR_ADCAL)
			;
		/* Enable ADC */
		ADC2->CR |= ADC_CR_ADEN;
		while(!(ADC2->ISR & ADC_ISR_ADRDY))
			;
		ADC2->CR |= ADC_CR_ADSTART;

		*bias = 4095; //assume bias to be in the middle in old AIOC revision

		return ADC2;
	}
	else
		return ADC1;
}

/**
 * @brief Initialize DMA
 * @param *buffer Target memory buffer
 * @param count Number of words to be copied
 * @param *adc Source ADC
 */
static void MODEM_LL_INITIALIZE_DMA(volatile void *buffer, uint16_t count, volatile ADC_TypeDef *adc)                                                                         
{                                                                                 
    /* 16 bit memory region */                                                    
    DMA1_Channel2->CCR |= DMA_CCR_MSIZE_0;                                        
    DMA1_Channel2->CCR &= ~DMA_CCR_MSIZE_1;                                       
    DMA1_Channel2->CCR |= DMA_CCR_PSIZE_0;                                        
    DMA1_Channel2->CCR &= ~DMA_CCR_PSIZE_1;                                       
    /* Enable memory pointer increment, circular mode and interrupt generation */ 
    DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_TCIE;             
    DMA1_Channel2->CNDTR = count;                          
    DMA1_Channel2->CPAR = (uintptr_t)&(adc->DR);                                 
    DMA1_Channel2->CMAR = (uintptr_t)(buffer);                                      
    DMA1_Channel2->CCR |= DMA_CCR_EN;                                             
}

/**
 * @brief Initialize primary DAC
 */
static void MODEM_LL_INITIALIZE_DAC(void)
{
	DAC1->CR &= DAC_CR_WAVE1;
	DAC1->CR &= ~DAC_CR_TEN1;
	DAC1->CR |= DAC_CR_BOFF1;
	DAC1->CR |= DAC_CR_EN1;
}

static void MODEM_LL_ADC_TIMER_INITIALIZE(void)                                                                    
{                                                       
    /* 72 / 9 = 8 MHz */                                
    TIM2->PSC = 8;                                      
    /* enable DMA call instead of standard interrupt */ 
    TIM2->DIER |= TIM_DIER_UDE;                         
}

static void MODEM_LL_DAC_TIMER_INITIALIZE(void)                        
{                                   
    /* 72 / 4 = 18 MHz */           
    TIM1->PSC = 3;                  
    TIM1->DIER |= TIM_DIER_UIE;     
}

static void MODEM_LL_BAUDRATE_TIMER_INITIALIZE(void)                              
{                                        
    /* 72 / 4 = 18 MHz */                
    TIM3->PSC = 3;                       
    TIM3->DIER |= TIM_DIER_UIE;          
}                                      

#define MODEM_LL_ADC_SET_SAMPLE_RATE(rate) (TIM2->ARR = (8000000 / (rate)) - 1)

#define MODEM_LL_DAC_TIMER_CALCULATE_STEP(frequency) ((18000000 / (frequency)) - 1)

#define MODEM_LL_BAUDRATE_TIMER_CALCULATE_STEP(frequency) ((18000000 / (frequency)) - 1)

#endif

#endif /* DRIVERS_MODEM_LL_H_ */
