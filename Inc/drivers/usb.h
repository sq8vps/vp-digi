/*
Copyright 2020-2023 Piotr Wilkon
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

#include "systick.h"

#if defined(STM32F103xB) || defined(STM32F103x8)

#include "stm32f1xx.h"

#define USB_FORCE_REENUMERATION() { \
	/* Pull D+ to ground for a moment to force reenumeration */ \
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; \
	GPIOA->CRH |= GPIO_CRH_MODE12_1; \
	GPIOA->CRH &= ~GPIO_CRH_CNF12; \
	GPIOA->BSRR = GPIO_BSRR_BR12; \
	Delay(100); \
	GPIOA->CRH &= ~GPIO_CRH_MODE12; \
	GPIOA->CRH |= GPIO_CRH_CNF12_0; \
} \

#endif

#endif /* DRIVERS_USB_H_ */
