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

#ifndef KISS_H_
#define KISS_H_

#include <stdint.h>
#include "uart.h"

/**
 * @brief Convert AX.25 frame to KISS and send
 * @param *port UART structure
 * @param *buf Frame buffer
 * @param size Frame size
 */
void KissSend(Uart *port, uint8_t *buf, uint16_t size);

/**
 * @brief Parse bytes received from UART to form a KISS frame (possibly) and send this frame
 * @param *port UART structure
 * @param data Received byte
 */
void KissParse(Uart *port, uint8_t data);

void KissProcess(Uart *port);

#endif /* KISS_H_ */
