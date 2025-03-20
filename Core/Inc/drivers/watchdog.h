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

#ifndef DRIVERS_WATCHDOG_H_
#define DRIVERS_WATCHDOG_H_

#if defined(STM32F103xB) || defined(STM32F103x8) || defined(STM32F302xC)

#if defined(STM32F103xB) || defined(STM32F103x8)
#include "stm32f1xx.h"
#elif defined(STM32F302xC)
#include "stm32f3xx.h"
#endif

/**
 * @brief Initialize watchdog
 */
void WdogInit(void)
{
	IWDG->KR = 0x5555; //configuration mode
	IWDG->PR = 0b101; //prescaler: 40 kHz/128=312.5 Hz
	IWDG->RLR = 0xFFF; //reload register -> timeout every 13.1 s 
	IWDG->KR = 0xCCCC; //start
}

/**
 * @brief Restart watchdog
 * @attention Must be called continuously in main loop
 */
void WdogReset(void)
{
	IWDG->KR = 0xAAAA; //reset
}

#endif

#endif /* DRIVERS_WATCHDOG_H_ */
