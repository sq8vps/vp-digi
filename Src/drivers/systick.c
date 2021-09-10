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

#include "drivers/systick.h"

volatile uint32_t ticks = 0; //SysTick counter

//with HAL enabled, the handler is in stm32f1xx_it.c
//void SysTick_Handler(void)
//{
	//ticks++;
//}

void SysTick_init(void)
{
	SysTick_Config(SystemCoreClock / 100); //SysTick every 10 ms
}
