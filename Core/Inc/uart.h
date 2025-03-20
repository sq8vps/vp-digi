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

#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "usbd_cdc_if.h"
#include "ax25.h"
#include "drivers/uart_ll.h"

enum UartMode
{
	MODE_KISS,
	MODE_TERM,
	MODE_MONITOR,
	MODE_CALIBRATION,
};

enum UartDataType
{
	DATA_NOTHING = 0,
	DATA_TERM,
	DATA_KISS,
	DATA_USB,
};

typedef struct
{
	struct
	{
		uint32_t baudrate; //baudrate 1200-115200
		enum UartMode defaultMode;
	} config;
	volatile USART_TypeDef *port; //UART peripheral
	volatile enum UartDataType rxType; //rx status
	uint8_t enabled : 1;
	uint8_t isUsb : 1;
	volatile uint8_t rxBuffer[UART_BUFFER_SIZE];
	volatile uint16_t rxBufferHead;
	uint8_t txBuffer[UART_BUFFER_SIZE];
	volatile uint16_t txBufferHead, txBufferTail;
	volatile uint8_t txBufferFull : 1;
	enum UartMode mode;
	volatile uint16_t lastRxBufferHead; //for special characters handling
	volatile uint8_t kissBuffer[AX25_FRAME_MAX_SIZE + 1];
	volatile uint16_t kissBufferHead;
	volatile uint8_t kissProcessingOngoing;
	volatile uint8_t kissTempBuffer[10];
	volatile uint16_t kissTempBufferHead;
} Uart;

#ifdef BLUE_PILL
extern Uart Uart1;
extern Uart Uart2;
#endif
extern Uart UartUsb;


/**
 * @brief Send byte
 * @param[in] *port UART
 * @param[in] data Data
 */
void UartSendByte(Uart *port, uint8_t data);

/**
 * @brief Send string
 * @param *port UART
 * @param *data Buffer
 * @param len Buffer length or 0 for null-terminated string
 */
void UartSendString(Uart *port, void *data, uint16_t datalen);

/**
 * @brief Send signed number
 * @param *port UART
 * @param n Number
 */
void UartSendNumber(Uart *port, int32_t n);


/**
 * @brief Initialize UART structures
 * @param *port UART [prt
 * @param *uart Physical UART peripheral. NULL if USB in CDC mode
 * @param baud Baudrate
 */
void UartInit(Uart *port, USART_TypeDef *uart, uint32_t baud);

/**
 * @brief Configure and enable/disable UART
 * @param *port UART port
 * @param state 0 - disable, 1 - enable
 */
void UartConfig(Uart *port, uint8_t state);

/**
 * @brief Clear RX buffer and flags
 * @param *port UART port
 */
void UartClearRx(Uart *port);

#endif
