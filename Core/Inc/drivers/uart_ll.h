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

/*
 * This file is kind of HAL for UART
 */

#ifndef DRIVERS_UART_LL_H_
#define DRIVERS_UART_LL_H_

#if defined(STM32F103xB) || defined(STM32F103x8)

#include "stm32f1xx.h"

#define UART_LL_ENABLE(port) {port->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;}
#define UART_LL_DISABLE(port) {port->CR1 &= (~USART_CR1_RXNEIE) & (~USART_CR1_TE) & (~USART_CR1_RE) &  (~USART_CR1_UE) & (~USART_CR1_IDLEIE);}

#define UART_LL_CHECK_RX_NOT_EMPTY(port) (port->SR & USART_SR_RXNE)
#define UART_LL_CLEAR_RX_NOT_EMPTY(port) {port->SR &= ~USART_SR_RXNE;}

#define UART_LL_CHECK_TX_EMPTY(port) (port->SR & USART_SR_TXE)
#define UART_LL_ENABLE_TX_EMPTY_INTERRUPT(port) {port->CR1 |= USART_CR1_TXEIE;}
#define UART_LL_DISABLE_TX_EMPTY_INTERRUPT(port) {port->CR1 &= ~USART_CR1_TXEIE;}
#define UART_LL_CHECK_ENABLED_TX_EMPTY_INTERRUPT(port) (port->CR1 & USART_CR1_TXEIE)


#define UART_LL_CHECK_RX_IDLE(port) (port->SR & USART_SR_IDLE)

#define UART_LL_GET_DATA(port) (port->DR)
#define UART_LL_PUT_DATA(port, data) {port->DR = (data);}


#define UART_LL_UART1_INTERUPT_HANDLER USART1_IRQHandler
#define UART_LL_UART2_INTERUPT_HANDLER USART2_IRQHandler

#define UART_LL_UART1_STRUCTURE USART1
#define UART_LL_UART2_STRUCTURE USART2

#define UART_LL_UART1_IRQ USART1_IRQn
#define UART_LL_UART2_IRQ USART2_IRQn

#define UART_LL_UART1_INITIALIZE_PERIPHERAL(baudrate) { \
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; \
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN; \
	GPIOA->CRH |= GPIO_CRH_MODE9_1; \
	GPIOA->CRH &= ~GPIO_CRH_CNF9_0; \
	GPIOA->CRH |= GPIO_CRH_CNF9_1; \
	GPIOA->CRH |= GPIO_CRH_CNF10_0; \
	GPIOA->CRH &= ~GPIO_CRH_CNF10_1; \
	UART_LL_UART1_STRUCTURE->BRR = (SystemCoreClock / baudrate); \
} \

#define UART_LL_UART2_INITIALIZE_PERIPHERAL(baudrate) { \
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; \
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN; \
	GPIOA->CRL |= GPIO_CRL_MODE2_1; \
	GPIOA->CRL &= ~GPIO_CRL_CNF2_0; \
	GPIOA->CRL |= GPIO_CRL_CNF2_1; \
	GPIOA->CRL |= GPIO_CRL_CNF3_0; \
	GPIOA->CRL &= ~GPIO_CRL_CNF3_1; \
	UART_LL_UART2_STRUCTURE->BRR = (SystemCoreClock / (baudrate * 2)); \
} \

#endif

#endif /* INC_DRIVERS_UART_LL_H_ */
