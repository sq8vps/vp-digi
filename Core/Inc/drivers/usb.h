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


#ifndef DRIVERS_USB_H_
#define DRIVERS_USB_H_

#include "defines.h"

#if defined(BLUE_PILL)

#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"

#define USB_FORCE_REENUMERATION() do { \
	/* Pull D+ to ground for a moment to force reenumeration */ \
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; \
	GPIOA->CRH |= GPIO_CRH_MODE12_1; \
	GPIOA->CRH &= ~GPIO_CRH_CNF12; \
	GPIOA->BSRR = GPIO_BSRR_BR12; \
	HAL_Delay(100); \
	GPIOA->CRH &= ~GPIO_CRH_MODE12; \
	GPIOA->CRH |= GPIO_CRH_CNF12_0; \
} while(0); \

#elif defined(AIOC)

#include "stm32f3xx.h"
#include "stm32f3xx_hal.h"

#define USB_FORCE_REENUMERATION() do { \
	/* Pull D+ to ground for a moment to force reenumeration */ \
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN; \
	GPIOA->MODER |= GPIO_MODER_MODER12_0; \
	GPIOA->MODER &= ~GPIO_MODER_MODER12_1; \
	GPIOA->OTYPER &= ~GPIO_OTYPER_OT_12; \
	GPIOA->BSRR = GPIO_BSRR_BR_12; \
	HAL_Delay(20); \
	GPIOA->MODER &= ~GPIO_MODER_MODER12_0; \
	GPIOA->MODER |= GPIO_MODER_MODER12_1; \
} while(0); \

#endif

#endif /* DRIVERS_USB_H_ */
