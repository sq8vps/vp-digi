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

#ifndef UART_H_
#define UART_H_

#define UARTBUFLEN 250

#include "stm32f1xx.h"
#include <stdint.h>
#include "usbd_cdc_if.h"

typedef enum
{
	MODE_KISS,
	MODE_TERM,
	MODE_MONITOR,
} Uart_mode;

typedef enum
{
	DATA_KISS,
	DATA_TERM,
	DATA_NOTHING,
} Uart_data_type;

typedef struct
{
	volatile USART_TypeDef *port; //UART peripheral
	uint32_t baudrate; //baudrate 1200-115200
	Uart_data_type rxflag; //rx status
	uint8_t txflag; //tx status (1 when transmitting)
	uint8_t bufrx[UARTBUFLEN]; //buffer for rx data
	uint16_t bufrxidx; //rx data buffer index
	uint8_t buftx[UARTBUFLEN]; //circular tx buffer
	uint16_t buftxrd, buftxwr; //tx buffer indexes
	Uart_mode mode; //uart mode
	uint8_t enabled;
	uint32_t kissTimer;
} Uart;

Uart uart1, uart2;

Uart_mode USBmode;
Uart_data_type USBrcvd;
uint8_t USBint; //USB "interrupt" flag


/**
 * \brief Copy KISS frame(s) from input buffer to APRS TX buffer
 * \param[in] *buf Input buffer
 * \param[in] len Input buffer size
 */
uint8_t Uart_txKiss(uint8_t *buf, uint16_t len);


/**
 * \brief Send single byte using USB
 * \param[in] data Byte
 */
void uartUSB_sendByte(uint8_t data);


/**
 * \brief Start buffer transmission
 * \param[in] *port UART
 */
void uart_transmitStart(Uart *port);

/**
 * \brief Store byte in TX buffer
 * \param[in] *port UART
 * \param[in] data Data
 */
void uart_sendByte(Uart *port, uint8_t data);

/**
 * \brief Store string in TX buffer
 * \apram[in] *port UART
 * \param[in] *data Buffer
 * \param[in] len Buffer length or 0 for null-terminated string
 */
void uart_sendString(Uart *port, uint8_t *data, uint16_t datalen);

/**
 * \brief Send string using USB
 * \param[in] *data Buffer
 * \param[in] len Buffer length or 0 for null-terminated string
 */
void uartUSB_sendString(uint8_t* data, uint16_t len);

/**
 * \brief Store number (in ASCII format) in TX buffer
 * \param[in] *port UART
 * \param[in] n Number
 */
void uart_sendNumber(Uart *port, int32_t n);

/**
 * \brief Send number (in ASCII format) using USB
 * \param[in] n Number
 */
void uartUSB_sendNumber(int32_t n);


/**
 * \brief Initialize UART structures
 * \param[in] *port UART
 * \param[in] *uart Physical UART peripheral
 * \param[in] baud Baudrate
 */
void uart_init(Uart *port, USART_TypeDef *uart, uint32_t baud);

/**
 * \brief Configure and enable/disable UART
 * \param[in] *port UART
 * \param[in] state 0 - disable, 1 - enable
 */
void uart_config(Uart *port, uint8_t state);

/**
 * \brief Clear RX buffer and flags
 * \param[in] *port UART
 */
void uart_clearRx(Uart *port);

#endif
