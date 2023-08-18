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

#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>


#define SYSTICK_FREQUENCY 100 //systick frequency in Hz
#define SYSTICK_INTERVAL (1000 / SYSTICK_FREQUENCY) //systick interval in milliseconds

extern volatile uint32_t ticks;

void SysTickInit(void);

uint32_t SysTickGet(void);

void Delay(uint32_t ms);

#endif /* SYSTICK_H_ */
