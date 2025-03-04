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

#include "terminal.h"
#include "ax25.h"
#include "common.h"
#include <string.h>
#include <systick.h>
#include <uart.h>
#include "digipeater.h"
#include "kiss.h"

Uart Uart1 = {.defaultMode = MODE_KISS}, Uart2 = {.defaultMode = MODE_KISS}, UartUsb= {.defaultMode = MODE_KISS};

static void handleInterrupt(Uart *port)
{
	if(UART_LL_CHECK_RX_NOT_EMPTY(port->port)) //byte received
	{
		UART_LL_CLEAR_RX_NOT_EMPTY(port->port);
		uint8_t data = port->port->DR;
		port->rxBuffer[port->rxBufferHead++] = data; //store it
		port->rxBufferHead %= UART_BUFFER_SIZE;

		KissParse(port, data);
		TermHandleSpecial(port);

	}
	if(UART_LL_CHECK_RX_IDLE(port->port)) //line is idle, end of data reception
	{
		UART_LL_GET_DATA(port->port); //reset idle flag by dummy read
		if(port->rxBufferHead != 0)
		{
			if(((port->rxBuffer[port->rxBufferHead - 1] == '\r') || (port->rxBuffer[port->rxBufferHead - 1] == '\n'))) //data ends with \r or \n, process as data
			{
				port->rxType = DATA_TERM;
			}
		}
	}
	if(UART_LL_CHECK_TX_EMPTY(port->port)) //TX buffer empty
	{
		if((port->txBufferHead != port->txBufferTail) || port->txBufferFull) //if there is anything to transmit
		{
			UART_LL_PUT_DATA(port->port, port->txBuffer[port->txBufferTail++]);
			port->txBufferTail %= UART_BUFFER_SIZE;
			port->txBufferFull = 0;
		}
		else //nothing more to be transmitted
		{
			UART_LL_DISABLE_TX_EMPTY_INTERRUPT(port->port);
		}
	}
}

void UART_LL_UART1_INTERUPT_HANDLER(void) __attribute__ ((interrupt));
void UART_LL_UART1_INTERUPT_HANDLER(void)
{
	handleInterrupt(&Uart1);
}

void UART_LL_UART2_INTERUPT_HANDLER(void) __attribute__ ((interrupt));
void UART_LL_UART2_INTERUPT_HANDLER(void)
{
	handleInterrupt(&Uart2);
}


void UartSendByte(Uart *port, uint8_t data)
{
	if(!port->enabled)
		return;

	if(port->isUsb)
	{
		CDC_Transmit_FS(&data, 1);
	}
	else
	{
		while(port->txBufferFull)
			;
		port->txBuffer[port->txBufferHead++] = data;
		port->txBufferHead %= UART_BUFFER_SIZE;
		__disable_irq();
		if(port->txBufferHead == port->txBufferTail)
			port->txBufferFull = 1;
		if(0 == (UART_LL_CHECK_ENABLED_TX_EMPTY_INTERRUPT(port->port)))
			UART_LL_ENABLE_TX_EMPTY_INTERRUPT(port->port);
		__enable_irq();
	}
}


void UartSendString(Uart *port, void *data, uint16_t len)
{
	if(0 == len)
		len = strlen((char*)data);

	for(uint16_t i = 0; i < len; i++)
	{
		UartSendByte(port, ((uint8_t*)data)[i]);
	}
}


static unsigned int findHighestPosition(unsigned int n)
{
    unsigned int i = 1;
    while((i * 10) <= n)
        i *= 10;

    return i;
}

void UartSendNumber(Uart *port, int32_t n)
{
	if(n < 0)
		UartSendByte(port, '-');
	n = abs(n);
    unsigned int position = findHighestPosition(n);
    while(position)
    {
        unsigned int number = n / position;
        UartSendByte(port, (number + 48));
        n -= (number * position);
        position /= 10;
    }
}

void UartInit(Uart *port, USART_TypeDef *uart, uint32_t baud)
{
	port->port = uart;
	port->baudrate = baud;
	port->rxType = DATA_NOTHING;
	port->rxBufferHead = 0;
	port->txBufferHead = 0;
	port->txBufferTail = 0;
	port->txBufferFull = 0;
	if(port->defaultMode > MODE_MONITOR)
		port->defaultMode = MODE_KISS;
	port->mode = port->defaultMode;
	port->enabled = 0;
	port->kissBufferHead = 0;
	port->lastRxBufferHead = 0;
	memset((void*)port->rxBuffer, 0, sizeof(port->rxBuffer));
	memset((void*)port->txBuffer, 0, sizeof(port->txBuffer));
	memset((void*)port->kissBuffer, 0, sizeof(port->kissBuffer));
}


void UartConfig(Uart *port, uint8_t state)
{
	if(port->port == UART_LL_UART1_STRUCTURE)
	{
		UART_LL_UART1_INITIALIZE_PERIPHERAL(port->baudrate);

		if(state)
		{
			UART_LL_ENABLE(port->port);
		}
		else
		{
			UART_LL_DISABLE(port->port);
		}

		NVIC_SetPriority(UART_LL_UART1_IRQ, 2);
		if(state)
			NVIC_EnableIRQ(UART_LL_UART1_IRQ);
		else
			NVIC_DisableIRQ(UART_LL_UART1_IRQ);

		port->enabled = state > 0;
		port->isUsb = 0;
	}
	else if(port->port == UART_LL_UART2_STRUCTURE)
	{
		UART_LL_UART2_INITIALIZE_PERIPHERAL(port->baudrate);

		if(state)
		{
			UART_LL_ENABLE(port->port);
		}
		else
		{
			UART_LL_DISABLE(port->port);
		}

		NVIC_SetPriority(UART_LL_UART2_IRQ, 2);
		if(state)
			NVIC_EnableIRQ(UART_LL_UART2_IRQ);
		else
			NVIC_DisableIRQ(UART_LL_UART2_IRQ);

		port->enabled = state > 0;
		port->isUsb = 0;
	}
	else
	{
		port->isUsb = 1;
		port->enabled = state > 0;
	}

}


void UartClearRx(Uart *port)
{
	__disable_irq();
	port->rxBufferHead = 0;
	port->rxType = DATA_NOTHING;
	__enable_irq();
}

