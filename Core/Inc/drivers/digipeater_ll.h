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

/*
 * This file is kind of HAL for digipeater external controls and signalization
 */

#ifndef DRIVERS_DIGIPEATER_LL_H_
#define DRIVERS_DIGIPEATER_LL_H_

#include <stdint.h>
#include "defines.h"

#if defined(BLUE_PILL)

#include "stm32f1xx.h"

#define DIGIPEATER_LL_LED_ON() (GPIOB->BSRR = GPIO_BSRR_BS1)
#define DIGIPEATER_LL_LED_OFF() (GPIOB->BSRR = GPIO_BSRR_BR1)

/**
 * @brief Get external digipeater disable state
 * @return True if disabled, false if enabled
 */
#define DIGIPEATER_LL_GET_DISABLE_STATE() (!(GPIOB->IDR & GPIO_IDR_IDR0))

#define DIGIPEATER_LL_INITIALIZE_RCC() do { \
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; \
} while(0); \

#define DIGIPEATER_LL_INITIALIZE_INPUTS_OUTPUTS() do { \
	/* Digi disable input: PB0 (active low - pull up); digi active output: PB1 (active high) */ \
	/* Digi disable input: PB0 */ \
	GPIOB->CRL &= ~GPIO_CRL_MODE0; \
	GPIOB->CRL &= ~GPIO_CRL_CNF0_0; \
	GPIOB->CRL |= GPIO_CRL_CNF0_1; \
	GPIOB->BSRR = GPIO_BSRR_BS0; \
	/* Digi active LED: PB1 */ \
	GPIOB->CRL |= GPIO_CRL_MODE1_1; \
	GPIOB->CRL &= ~GPIO_CRL_MODE1_0; \
	GPIOB->CRL &= ~GPIO_CRL_CNF1; \
} while(0); \

#elif defined(AIOC)

#define DIGIPEATER_LL_LED_ON()

#define DIGIPEATER_LL_LED_OFF()

/**
 * @brief Placeholder on AIOC port
 * @return Always 0
 */
#define DIGIPEATER_LL_GET_DISABLE_STATE() (0)

#define DIGIPEATER_LL_INITIALIZE_RCC()

#define DIGIPEATER_LL_INITIALIZE_INPUTS_OUTPUTS()

#endif

#endif
