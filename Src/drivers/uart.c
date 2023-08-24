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

#include "drivers/uart.h"
#include "drivers/systick.h"
#include "terminal.h"
#include "ax25.h"
#include "common.h"
#include <string.h>

#include "digipeater.h"

Uart uart1, uart2;


uint8_t Uart_txKiss(uint8_t *buf, uint16_t len)
{
	if(len < 10) //frame is too small
	{
		return 1;
	}

	uint16_t framebegin = 0;
	uint8_t framestatus = 0; //0 - frame not started, 1 - frame start found, 2 - in a frame, 3 - frame end found

	for(uint16_t i = 0; i < len; i++)
	{
		if(*(buf + i) == 0xc0) //found KISS frame delimiter
		{
			if((i > 2) && (framestatus == 2)) //we are already in frame, this is the ending marker
			{
				framestatus = 3;
				ax25.frameXmit[ax25.xmitIdx++] = 0xFF; //write frame separator
				Digi_storeDeDupeFromXmitBuf(framebegin); //store duplicate protection hash
		        if((FRAMEBUFLEN - ax25.xmitIdx) < (len - i + 2)) //there might be next frame in input buffer, but if there is no space for it, drop it
		        	break;
			}
		}
		else if((*(buf + i) == 0x00) && (*(buf + i - 1) == 0xC0) && ((framestatus == 0) || (framestatus == 3))) //found frame delimiter, modem number (0x00) and we are not in a frame yet or preceding frame has been processed
		{
			framestatus = 1; //copy next frame
			framebegin = ax25.xmitIdx;
		}
		else if((framestatus == 1) || (framestatus == 2)) //we are in a frame
		{
			ax25.frameXmit[ax25.xmitIdx++] = *(buf + i); //copy data
			framestatus = 2;
		}
	}

	return 0;
}

static volatile void uart_handleInterrupt(Uart *port)
{
	if(port->port->SR & USART_SR_RXNE) //byte received
	{
		port->port->SR &= ~USART_SR_RXNE;
		port->bufrx[port->bufrxidx] = port->port->DR; //store it
		port->bufrxidx++;
		if(port->port == USART1) //handle special functions and characters
			term_handleSpecial(TERM_UART1);
		else if(port->port == USART2)
			term_handleSpecial(TERM_UART2);

		port->bufrxidx %= UARTBUFLEN;

		if(port->mode == MODE_KISS)
			port->kissTimer = ticks + 500; //set timeout to 5s in KISS mode
	}
	if(port->port->SR & USART_SR_IDLE) //line is idle, end of data reception
	{
		port->port->DR; //reset idle flag by dummy read
		if(port->bufrxidx == 0)
			return; //no data, stop

		if((port->bufrx[0] == 0xc0) && (port->bufrx[port->bufrxidx - 1] == 0xc0))   //data starts with 0xc0 and ends with 0xc0 - this is a KISS frame
		{
			port->rxflag = DATA_KISS;
			port->kissTimer = 0;
		}

		if(((port->bufrx[port->bufrxidx - 1] == '\r') || (port->bufrx[port->bufrxidx - 1] == '\n'))) //data ends with \r or \n, process as data
		{
			port->rxflag = DATA_TERM;
			port->kissTimer = 0;
		}

	}
	if(port->port->SR & USART_SR_TXE) //TX buffer empty
	{
		if(port->buftxrd != port->buftxwr) //if there is anything to transmit
		{
			port->port->DR = port->buftx[port->buftxrd++]; //push it to the refister
			port->buftxrd %= UARTBUFLEN;
		} else //nothing more to be transmitted
		{
			port->txflag = 0; //stop transmission
			port->port->CR1 &= ~USART_CR1_TXEIE;
		}
	}

	if((port->kissTimer > 0) && (ticks >= port->kissTimer)) //KISS timer timeout
	{
		port->kissTimer = 0;
		port->bufrxidx = 0;
		memset(port->bufrx, 0, UARTBUFLEN);
	}
}

void USART1_IRQHandler(void) __attribute__ ((interrupt));
void USART1_IRQHandler(void)
{
	uart_handleInterrupt(&uart1);
}

void USART2_IRQHandler(void) __attribute__ ((interrupt));
void USART2_IRQHandler(void)
{
	uart_handleInterrupt(&uart2);
}




void uart_transmitStart(Uart *port)
{
	if(port->enabled == 0)
	{
		port->buftxrd = port->buftxwr;
		return;
	}
	port->txflag = 1;
	port->port->CR1 |= USART_CR1_TXEIE;
}


void uart_sendByte(Uart *port, uint8_t data)
{
	while(port->txflag == 1);;
	port->buftx[port->buftxwr++] = data;
	port->buftxwr %= (UARTBUFLEN);
}


void uart_sendString(Uart *port, uint8_t *data, uint16_t len)
{

	if(len == 0)
	{
		while(*(data + len) != 0)
		{
			len++;
			if(len == UARTBUFLEN)
				break;
		}
	}

	while(port->txflag == 1);;
	uint16_t i = 0;
	while(i < len)
	{
		port->buftx[port->buftxwr++] = *(data + i);
		port->buftxwr %= (UARTBUFLEN);
		i++;
	}

}


void uart_sendNumber(Uart *port, int32_t n)
{
	if(n < 0) uart_sendByte(port, '-');
	n = abs(n);
	if(n > 999999) uart_sendByte(port, (n / 1000000) + 48);
	if(n > 99999) uart_sendByte(port, ((n % 1000000) / 100000) + 48);
	if(n > 9999) uart_sendByte(port, ((n % 100000) / 10000) + 48);
	if(n > 999) uart_sendByte(port, ((n % 10000) / 1000) + 48);
	if(n > 99) uart_sendByte(port, ((n % 1000) / 100) + 48);
	if(n > 9) uart_sendByte(port, ((n % 100) / 10) + 48);
	uart_sendByte(port, (n % 10) + 48);
}

void uart_init(Uart *port, USART_TypeDef *uart, uint32_t baud)
{
	port->port = uart;
	port->baudrate = baud;
	port->rxflag = DATA_NOTHING;
	port->txflag = 0;
	port->bufrxidx = 0;
	port->buftxrd = 0;
	port->buftxwr = 0;
	port->mode = MODE_KISS;
	port->enabled = 0;
	port->kissTimer = 0;
	memset(port->bufrx, 0, UARTBUFLEN);
	memset(port->buftx, 0, UARTBUFLEN);
}


void uart_config(Uart *port, uint8_t state)
{
	if(port->port == USART1)
	{
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
		GPIOA->CRH |= GPIO_CRH_MODE9_1;
		GPIOA->CRH &= ~GPIO_CRH_CNF9_0;
		GPIOA->CRH |= GPIO_CRH_CNF9_1;
		GPIOA->CRH |= GPIO_CRH_CNF10_0;
		GPIOA->CRH &= ~GPIO_CRH_CNF10_1;

		USART1->BRR = (SystemCoreClock / (port->baudrate));

		if(state)
			USART1->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
		else
			USART1->CR1 &= (~USART_CR1_RXNEIE) & (~USART_CR1_TE) & (~USART_CR1_RE) &  (~USART_CR1_UE) & (~USART_CR1_IDLEIE);

		NVIC_SetPriority(USART1_IRQn, 2);
		if(state)
			NVIC_EnableIRQ(USART1_IRQn);
		else
			NVIC_DisableIRQ(USART1_IRQn);

		port->enabled = state > 0;
	}
	else if(port->port == USART2)
	{
		RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
		GPIOA->CRL |= GPIO_CRL_MODE2_1;
		GPIOA->CRL &= ~GPIO_CRL_CNF2_0;
		GPIOA->CRL |= GPIO_CRL_CNF2_1;
		GPIOA->CRL |= GPIO_CRL_CNF3_0;
		GPIOA->CRL &= ~GPIO_CRL_CNF3_1;

		USART2->BRR = (SystemCoreClock / (port->baudrate * 2)); // clk/2, APB1 runs at clk/2 (apb1clk/2)
		if(state)
			USART2->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
		else
			USART2->CR1 &= (~USART_CR1_RXNEIE) & (~USART_CR1_TE) & (~USART_CR1_RE) &  (~USART_CR1_UE) & (~USART_CR1_IDLEIE);

		NVIC_SetPriority(USART2_IRQn, 2);
		if(state)
			NVIC_EnableIRQ(USART2_IRQn);
		else
			NVIC_DisableIRQ(USART2_IRQn);

		port->enabled = state > 0;
	}

}


void uart_clearRx(Uart *port)
{
	port->bufrxidx = 0;
	port->rxflag = 0;
}

