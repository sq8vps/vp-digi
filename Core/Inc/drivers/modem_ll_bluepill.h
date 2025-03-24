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

#ifndef DRIVERS_MODEM_LL_STM32F103_H_
#define DRIVERS_MODEM_LL_STM32F103_H_

#if defined(STM32F103xB) || defined(STM32F103x8)

#include <stdint.h>
#include "stm32f1xx.h"

#define MODEM_LL_DAC_MAX 255 /**< PWM max value */

/**
 * TIM1 is used for pushing samples to DAC (R2R or PWM) (clocked at 18 MHz)
 * TIM3 is the baudrate generator for TX (clocked at 18 MHz)
 * TIM4 is the PWM generator with no software interrupt
 * TIM2 is the RX sampling timer with no software interrupt, but it directly calls DMA
 */

#define MODEM_LL_DMA_INTERRUPT_HANDLER DMA1_Channel2_IRQHandler
#define MODEM_LL_DAC_INTERRUPT_HANDLER TIM1_UP_IRQHandler
#define MODEM_LL_BAUDRATE_TIMER_INTERRUPT_HANDLER TIM3_IRQHandler

#define MODEM_LL_DMA_IRQ DMA1_Channel2_IRQn
#define MODEM_LL_DAC_IRQ TIM1_UP_IRQn
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

#define MODEM_LL_DAC_PUT_VALUE(value) (TIM4->CCR1 = (value))

static inline void MODEM_LL_DCD_LED_ON(void)
{
    GPIOC->BSRR = GPIO_BSRR_BR13;
    GPIOB->BSRR = GPIO_BSRR_BS5;
}

static inline void MODEM_LL_DCD_LED_OFF(void)
{
    GPIOC->BSRR = GPIO_BSRR_BS13;
    GPIOB->BSRR = GPIO_BSRR_BR5;
}

#define MODEM_LL_PTT_ON(output) (GPIOB->BSRR = GPIO_BSRR_BS7)
#define MODEM_LL_PTT_OFF(output) (GPIOB->BSRR = GPIO_BSRR_BR7)

static void MODEM_LL_INITIALIZE_RCC(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
}

static void MODEM_LL_INITIALIZE_OUTPUTS(void)
{
	/* DCD LEDs: PC13 (cathode driven - built-in LED on Blue Pill) and PB5 (anode driven) */
	GPIOC->CRH |= GPIO_CRH_MODE13_1;
	GPIOC->CRH &= ~GPIO_CRH_MODE13_0;
	GPIOC->CRH &= ~GPIO_CRH_CNF13;
	GPIOB->CRL |= GPIO_CRL_MODE5_1;
	GPIOB->CRL &= ~GPIO_CRL_MODE5_0;
	GPIOB->CRL &= ~GPIO_CRL_CNF5;
	/* PTT: PB7 */
	GPIOB->CRL |= GPIO_CRL_MODE7_1;
	GPIOB->CRL &= ~GPIO_CRL_MODE7_0;
	GPIOB->CRL &= ~GPIO_CRL_CNF7;
}

/**
 * @brief Initialize ADC
 * @return Utilized ADC pointer
 */
static volatile ADC_TypeDef* MODEM_LL_INITIALIZE_ADC(int32_t *bias)
{
	/* ADC input: PA0 */
	GPIOA->CRL &= ~GPIO_CRL_CNF0;
	GPIOA->CRL &= ~GPIO_CRL_MODE0;
	/*/6 prescaler */
	RCC->CFGR |= RCC_CFGR_ADCPRE_1;
	RCC->CFGR &= ~RCC_CFGR_ADCPRE_0;
	ADC1->CR2 |= ADC_CR2_CONT;
	ADC1->CR2 |= ADC_CR2_EXTSEL;
	ADC1->SQR1 &= ~ADC_SQR1_L;
	/* 41.5 cycle sampling */
	ADC1->SMPR2 |= ADC_SMPR2_SMP0_2;
	ADC1->SQR3 &= ~ADC_SQR3_SQ1;
	ADC1->CR2 |= ADC_CR2_ADON;
	/* calibrate */
	ADC1->CR2 |= ADC_CR2_RSTCAL;
	while (ADC1->CR2 & ADC_CR2_RSTCAL)
		;
	ADC1->CR2 |= ADC_CR2_CAL;
	while (ADC1->CR2 & ADC_CR2_CAL)
		;
	ADC1->CR2 |= ADC_CR2_EXTTRIG;
	ADC1->CR2 |= ADC_CR2_SWSTART;

	*bias = 4095;
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
	/* enable memory pointer increment, circular mode and interrupt generation */
	DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_TCIE;
	DMA1_Channel2->CNDTR = count;
	DMA1_Channel2->CPAR = (uintptr_t)&(adc->DR);
	DMA1_Channel2->CMAR = (uintptr_t)buffer;
	DMA1_Channel2->CCR |= DMA_CCR_EN;
}

/**
 * @brief Initialize DAC
 */
static void MODEM_LL_INITIALIZE_DAC(void)
{
	/* PWM output: PB6 */
	GPIOB->CRL |= GPIO_CRL_CNF6_1;
	GPIOB->CRL |= GPIO_CRL_MODE6;
	GPIOB->CRL &= ~GPIO_CRL_CNF6_0;

    /* 72 / 3 = 24 MHz to provide 8 bit resolution at around 100 kHz */
    TIM4->PSC = 2;
    /* 24 MHz / 256 = 94 kHz */
    TIM4->ARR = 255;
    TIM4->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CR1 |= TIM_CR1_CEN;
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
