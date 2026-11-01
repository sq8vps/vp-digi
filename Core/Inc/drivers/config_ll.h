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

#ifndef DRIVERS_CONFIG_LL_H_
#define DRIVERS_CONFIG_LL_H_

#include <stdint.h>

#if defined(STM32F103xB) || defined(STM32F103x8)

#include "stm32f1xx.h"

#define CONFIG_ADDRESS (uintptr_t)0x800F000 //64 KiB starting at 0x8000000 minus 4 kiB (two 1024-word pages)
#define CONFIG_PAGE_COUNT 2 //two 1024-word pages
#define CONFIG_PAGE_SIZE 1024 //1024 words per page (2048 bytes)

/**
 * @brief Write word to configuration part in flash
 * @param[in] address Relative address
 * @param[in] data Data to write
 * @warning Flash must be unlocked first
 */
static void write(uint32_t address, uint16_t data)
{
    FLASH->CR |= FLASH_CR_PG; //programming mode

    *((volatile uint16_t*)(address + CONFIG_ADDRESS)) = data; //store data

	while (FLASH->SR & FLASH_SR_BSY);; //wait for completion
	if(!(FLASH->SR & FLASH_SR_EOP)) //an error occurred
		FLASH->CR &= ~FLASH_CR_PG;
	else
		FLASH->SR |= FLASH_SR_EOP;
}

/**
 * @brief Read single word from configuration part in flash
 * @param[in] address Relative address
 * @return Data (word)
 */
static uint16_t read(uint32_t address)
{
	return *(volatile uint16_t*)((address + CONFIG_ADDRESS));
}

static void erase(void)
{
	while (FLASH->SR & FLASH_SR_BSY)
		;
	FLASH->CR |= FLASH_CR_PER; //erase mode
	for(uint8_t i = 0; i < CONFIG_PAGE_COUNT; i++)
	{
		FLASH->AR = CONFIG_ADDRESS + (CONFIG_PAGE_SIZE * i);
    	FLASH->CR |= FLASH_CR_STRT; //start erase
    	while (FLASH->SR & FLASH_SR_BSY)
    		;
    	if(!(FLASH->SR & FLASH_SR_EOP))
    	{
        	FLASH->CR &= ~FLASH_CR_PER;
    		return;
    	}
    	else
    		FLASH->SR |= FLASH_SR_EOP;
	}
    FLASH->CR &= ~FLASH_CR_PER;
}

static void unlock(void)
{
	FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
}

static void lock(void)
{
	FLASH->CR &= ~FLASH_CR_PG;
	FLASH->CR |= FLASH_CR_LOCK;
}

#elif defined(STM32F302xC)

#include "stm32f3xx.h"

#define CONFIG_ADDRESS (uintptr_t)0x801F000 //128 KiB starting at 0x8000000 minus 4 kiB (two 1024-word pages)
#define CONFIG_PAGE_COUNT 2 //two 1024-word pages
#define CONFIG_PAGE_SIZE 1024 //1024 words per page (2048 bytes)

/**
 * @brief Write word to configuration part in flash
 * @param[in] address Relative address
 * @param[in] data Data to write
 * @warning Flash must be unlocked first
 */
static void write(uint32_t address, uint16_t data)
{
    FLASH->CR |= FLASH_CR_PG; //programming mode

    *((volatile uint16_t*)(address + CONFIG_ADDRESS)) = data; //store data

	while (FLASH->SR & FLASH_SR_BSY);; //wait for completion
	if(!(FLASH->SR & FLASH_SR_EOP)) //an error occurred
		FLASH->CR &= ~FLASH_CR_PG;
	else
		FLASH->SR |= FLASH_SR_EOP;
}

/**
 * @brief Read single word from configuration part in flash
 * @param[in] address Relative address
 * @return Data (word)
 */
static uint16_t read(uint32_t address)
{
	return *(volatile uint16_t*)((address + CONFIG_ADDRESS));
}

static void erase(void)
{
	while (FLASH->SR & FLASH_SR_BSY)
		;
	FLASH->CR |= FLASH_CR_PER; //erase mode
	for(uint8_t i = 0; i < CONFIG_PAGE_COUNT; i++)
	{
		FLASH->AR = CONFIG_ADDRESS + (CONFIG_PAGE_SIZE * i);
    	FLASH->CR |= FLASH_CR_STRT; //start erase
    	while (FLASH->SR & FLASH_SR_BSY)
    		;
    	if(!(FLASH->SR & FLASH_SR_EOP))
    	{
        	FLASH->CR &= ~FLASH_CR_PER;
    		return;
    	}
    	else
    		FLASH->SR |= FLASH_SR_EOP;
	}
    FLASH->CR &= ~FLASH_CR_PER;
}

static void unlock(void)
{
	FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
}

static void lock(void)
{
	FLASH->CR &= ~FLASH_CR_PG;
	FLASH->CR |= FLASH_CR_LOCK;
}

#endif

#endif
