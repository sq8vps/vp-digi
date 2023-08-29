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

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "drivers/uart.h"
#include <stdint.h>

/**
 * @brief Send data to all available ports
 * @param mode Output mode/data type
 * @param *data Data buffer
 * @param size Data size
 */
void TermSendToAll(enum UartMode mode, uint8_t *data, uint16_t size);

/**
 * @brief Send signed number to all available ports
 * @param mode Output mode/data type
 * @param n Number to send
 */
void TermSendNumberToAll(enum UartMode mode, int32_t n);


/**
 * @brief Handle "special" terminal cases like backspace or local echo
 * @param *u UART structure
 * @attention Must be called for every received data
 */
void TermHandleSpecial(Uart *u);

/**
 * @brief Parse and process received data
 * @param *src UART structure
 */
void TermParse(Uart *src);

#endif /* DEBUG_H_ */
