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

#include "config.h"
#include "common.h"
#include "drivers/uart.h"
#include "usbd_cdc_if.h"
#include "digipeater.h"
#include "drivers/uart.h"
#include "ax25.h"
#include "beacon.h"
#include "stm32f1xx.h"
#include "drivers/modem.h"

/**
 * @brief Write word to configuration part in flash
 * @param[in] address Relative address
 * @param[in] data Data to write
 * @warning Flash must be unlocked first
 */
static void flash_write(uint32_t address, uint16_t data)
{
    FLASH->CR |= FLASH_CR_PG; //programming mode

    *(volatile uint16_t*)((address + MEM_CONFIG)) = data; //store data

	while (FLASH->SR & FLASH_SR_BSY);; //wait for ceompletion
	if(!(FLASH->SR & FLASH_SR_EOP)) //an error occurred
	{
		FLASH->CR &= ~FLASH_CR_PG;
		return;
	} else FLASH->SR |= FLASH_SR_EOP;

	//FLASH->CR &= ~FLASH_CR_PG;
}

/**
 * @brief Write data array to configuration part in flash
 * @param[in] address Relative address
 * @param[in] *data Data to write
 * @param[in] len Data length
 * @warning Flash must be unlocked first
 */
static void flash_writeString(uint32_t address, uint8_t *data, uint16_t len)
{
	uint16_t i = 0;
	for(; i < (len >> 1); i++)
	{
		flash_write(address + (i << 1), *(data + (i << 1)) | (*(data + 1 + (i << 1)) << 8)); //program memory
	}
	if((len % 2) > 0)
	{
		flash_write(address + (i << 1), *(data + (i << 1))); //store last byte if number of bytes is odd
	}
}

/**
 * @brief Read single word from configuration part in flash
 * @param[in] address Relative address
 * @return Data (word)
 */
static uint16_t flash_read(uint32_t address)
{
	return *(volatile uint16_t*)((address + MEM_CONFIG));
}

/**
* @brief Read array from configuration part in flash
* @param[in] address Relative address
* @param[out] *data Data
* @param[in] len Byte count
*/
static void flash_readString(uint32_t address, uint8_t *data, uint16_t len)
{
	uint16_t i = 0;
	for(; i < (len >> 1); i++)
	{
		*(data + (i << 1)) = (uint8_t)flash_read(address + (i << 1));
		*(data + 1 + (i << 1)) = (uint8_t)flash_read(address + 1 + (i << 1));
	}
	if((len % 2) > 0)
	{
		*(data + (i << 1)) = (uint8_t)flash_read(address + (i << 1));
	}
}

void Config_erase(void)
{
	FLASH->KEYR = 0x45670123; //unlock memory
    FLASH->KEYR = 0xCDEF89AB;
	while (FLASH->SR & FLASH_SR_BSY);;
	FLASH->CR |= FLASH_CR_PER; //erase mode
	for(uint8_t i = 0; i < 2; i++)
	{
		FLASH->AR = (MEM_CONFIG) + (1024 * i);
    	FLASH->CR |= FLASH_CR_STRT; //start erase
    	while (FLASH->SR & FLASH_SR_BSY);;
    	if(!(FLASH->SR & FLASH_SR_EOP))
    	{
        	FLASH->CR &= ~FLASH_CR_PER;

    		return;
    	} else FLASH->SR |= FLASH_SR_EOP;
	}
    FLASH->CR &= ~FLASH_CR_PER;
}

/**
 * @brief Store configuration from RAM to Flash
 */
void Config_write(void)
{
	Config_erase();

	flash_writeString(CONFIG_CALL, call, 6);
	flash_write(CONFIG_SSID, callSsid);
	flash_writeString(CONFIG_DEST, dest, 6);
	flash_write(CONFIG_TXDELAY, ax25Cfg.txDelayLength);
	flash_write(CONFIG_TXTAIL, ax25Cfg.txTailLength);
	flash_write(CONFIG_TXQUIET, ax25Cfg.quietTime);
	flash_writeString(CONFIG_RS1BAUD, (uint8_t*)&uart1.baudrate, 3);
	flash_writeString(CONFIG_RS2BAUD, (uint8_t*)&uart2.baudrate, 3);

	flash_write(CONFIG_BEACONS, (beacon[0].enable > 0) | ((beacon[1].enable > 0) << 1) | ((beacon[2].enable > 0) << 2) | ((beacon[3].enable > 0) << 3) | ((beacon[4].enable > 0) << 4) | ((beacon[5].enable > 0) << 5) | ((beacon[6].enable > 0) << 6) | ((beacon[7].enable > 0) << 7));
	for(uint8_t s = 0; s < 8; s++)
	{
		flash_write(CONFIG_BCIV + (2 * s), beacon[s].interval / 6000);
	}
	for(uint8_t s = 0; s < 8; s++)
	{
		flash_write(CONFIG_BCDL + (2 * s), beacon[s].delay / 60);
	}
	flash_writeString(CONFIG_BC0, beacon[0].data, 100);
	flash_writeString(CONFIG_BC1, beacon[1].data, 100);
	flash_writeString(CONFIG_BC2, beacon[2].data, 100);
	flash_writeString(CONFIG_BC3, beacon[3].data, 100);
	flash_writeString(CONFIG_BC4, beacon[4].data, 100);
	flash_writeString(CONFIG_BC5, beacon[5].data, 100);
	flash_writeString(CONFIG_BC6, beacon[6].data, 100);
	flash_writeString(CONFIG_BC7, beacon[7].data, 100);
	flash_writeString(CONFIG_BCP0, beacon[0].path, 14);
	flash_writeString(CONFIG_BCP1, beacon[1].path, 14);
	flash_writeString(CONFIG_BCP2, beacon[2].path, 14);
	flash_writeString(CONFIG_BCP3, beacon[3].path, 14);
	flash_writeString(CONFIG_BCP4, beacon[4].path, 14);
	flash_writeString(CONFIG_BCP5, beacon[5].path, 14);
	flash_writeString(CONFIG_BCP6, beacon[6].path, 14);
	flash_writeString(CONFIG_BCP7, beacon[7].path, 14);
	flash_write(CONFIG_DIGION, digi.enable);
	flash_write(CONFIG_DIGIEN, digi.enableAlias);
	flash_write(CONFIG_DIGIVISC, ((uint16_t)digi.viscous << 8) | (uint16_t)digi.directOnly);
	flash_writeString(CONFIG_DIGIAL0, digi.alias[0], 5);
	flash_writeString(CONFIG_DIGIAL1, digi.alias[1], 5);
	flash_writeString(CONFIG_DIGIAL2, digi.alias[2], 5);
	flash_writeString(CONFIG_DIGIAL3, digi.alias[3], 5);
	flash_writeString(CONFIG_DIGIAL4, digi.alias[4], 6);
	flash_writeString(CONFIG_DIGIAL5, digi.alias[5], 6);
	flash_writeString(CONFIG_DIGIAL6, digi.alias[6], 6);
	flash_writeString(CONFIG_DIGIAL7, digi.alias[7], 6);
	flash_write(CONFIG_DIGISSID4, digi.ssid[0]);
	flash_write(CONFIG_DIGISSID5, digi.ssid[1]);
	flash_write(CONFIG_DIGISSID6, digi.ssid[2]);
	flash_write(CONFIG_DIGISSID7, digi.ssid[3]);
	flash_write(CONFIG_DIGIMAX0, digi.max[0]);
	flash_write(CONFIG_DIGIMAX1, digi.max[1]);
	flash_write(CONFIG_DIGIMAX2, digi.max[2]);
	flash_write(CONFIG_DIGIMAX3, digi.max[3]);
	flash_write(CONFIG_DIGIREP0, digi.rep[0]);
	flash_write(CONFIG_DIGIREP1, digi.rep[1]);
	flash_write(CONFIG_DIGIREP2, digi.rep[2]);
	flash_write(CONFIG_DIGIREP3, digi.rep[3]);
	flash_write(CONFIG_DIGITRACE, digi.traced);
	flash_write(CONFIG_DIGIDEDUPE, digi.dupeTime);
	flash_write(CONFIG_DIGICALLFILEN, digi.callFilterEnable);
	flash_write(CONFIG_DIGIFILTYPE, digi.filterPolarity);
	flash_writeString(CONFIG_DIGIFILLIST, digi.callFilter[0], 140);
	flash_write(CONFIG_AUTORST, autoReset);
	flash_write(CONFIG_PWM_FLAT, afskCfg.usePWM | (afskCfg.flatAudioIn << 1));
	flash_write(CONFIG_KISSMONITOR, kissMonitor);
	flash_write(CONFIG_ALLOWNONAPRS, ax25Cfg.allowNonAprs);

	flash_write(CONFIG_FLAG, FLAG_CONFIG_WRITTEN);

	FLASH->CR &= ~FLASH_CR_PG;
	FLASH->CR |= FLASH_CR_LOCK;

}

uint8_t Config_read(void)
{
	if(flash_read(CONFIG_FLAG) != FLAG_CONFIG_WRITTEN) //no configuration stored
	{
		return 0;
	}
	flash_readString(CONFIG_CALL, call, 6);
	callSsid = (uint8_t)flash_read(CONFIG_SSID);
	uint8_t temp[6];
	flash_readString(CONFIG_DEST, temp, 6);
	if((temp[0] >= ('A' << 1)) && (temp[0] <= ('Z' << 1)) && ((temp[0] & 1) == 0)) //check if stored destination address is correct (we just assume it by reading the first byte)
	{
		memcpy(dest, temp, sizeof(uint8_t) * 6);
	}
	ax25Cfg.txDelayLength = flash_read(CONFIG_TXDELAY);
	ax25Cfg.txTailLength = flash_read(CONFIG_TXTAIL);
	ax25Cfg.quietTime = flash_read(CONFIG_TXQUIET);
	uart1.baudrate = 0;
	uart2.baudrate = 0;
	flash_readString(CONFIG_RS1BAUD, (uint8_t*)&uart1.baudrate, 3);
	flash_readString(CONFIG_RS2BAUD, (uint8_t*)&uart2.baudrate, 3);
	uint8_t bce = (uint8_t)flash_read(CONFIG_BEACONS);
	beacon[0].enable = (bce & 1) > 0;
	beacon[1].enable = (bce & 2) > 0;
	beacon[2].enable = (bce & 4) > 0;
	beacon[3].enable = (bce & 8) > 0;
	beacon[4].enable = (bce & 16) > 0;
	beacon[5].enable = (bce & 32) > 0;
	beacon[6].enable = (bce & 64) > 0;
	beacon[7].enable = (bce & 128) > 0;
	for(uint8_t s = 0; s < 8; s++)
	{
		 beacon[s].interval = flash_read(CONFIG_BCIV + (2 * s)) * 6000;
	}
	for(uint8_t s = 0; s < 8; s++)
	{
		 beacon[s].delay = flash_read(CONFIG_BCDL + (2 * s)) * 60;
	}

	for(uint8_t g = 0; g < 8; g++)
	{
		flash_readString(CONFIG_BC0 + (g * 100), beacon[g].data, 100);
	}
	for(uint8_t g = 0; g < 8; g++)
	{
		flash_readString(CONFIG_BCP0 + (g * 14), beacon[g].path, 14);
	}
	digi.enable = (uint8_t)flash_read(CONFIG_DIGION);
	digi.enableAlias = (uint8_t)flash_read(CONFIG_DIGIEN);
	uint16_t t = flash_read(CONFIG_DIGIVISC);
	digi.viscous = (t & 0xFF00) >> 8;
	digi.directOnly = t & 0xFF;
	flash_readString(CONFIG_DIGIAL0, digi.alias[0], 5);
	flash_readString(CONFIG_DIGIAL1, digi.alias[1], 5);
	flash_readString(CONFIG_DIGIAL2, digi.alias[2], 5);
	flash_readString(CONFIG_DIGIAL3, digi.alias[3], 5);
	flash_readString(CONFIG_DIGIAL4, digi.alias[4], 6);
	flash_readString(CONFIG_DIGIAL5, digi.alias[5], 6);
	flash_readString(CONFIG_DIGIAL6, digi.alias[6], 6);
	flash_readString(CONFIG_DIGIAL7, digi.alias[7], 6);
	digi.ssid[0] = (uint8_t)flash_read(CONFIG_DIGISSID4);
	digi.ssid[1] = (uint8_t)flash_read(CONFIG_DIGISSID5);
	digi.ssid[2] = (uint8_t)flash_read(CONFIG_DIGISSID6);
	digi.ssid[3] = (uint8_t)flash_read(CONFIG_DIGISSID7);
	digi.max[0] = (uint8_t)flash_read(CONFIG_DIGIMAX0);
	digi.max[1] = (uint8_t)flash_read(CONFIG_DIGIMAX1);
	digi.max[2] = (uint8_t)flash_read(CONFIG_DIGIMAX2);
	digi.max[3] = (uint8_t)flash_read(CONFIG_DIGIMAX3);
	digi.rep[0] = (uint8_t)flash_read(CONFIG_DIGIREP0);
	digi.rep[1] = (uint8_t)flash_read(CONFIG_DIGIREP1);
	digi.rep[2] = (uint8_t)flash_read(CONFIG_DIGIREP2);
	digi.rep[3] = (uint8_t)flash_read(CONFIG_DIGIREP3);
	digi.traced = (uint8_t)flash_read(CONFIG_DIGITRACE);
	digi.dupeTime = (uint8_t)flash_read(CONFIG_DIGIDEDUPE);
	digi.callFilterEnable = (uint8_t)flash_read(CONFIG_DIGICALLFILEN);
	digi.filterPolarity = (uint8_t)flash_read(CONFIG_DIGIFILTYPE);
	flash_readString(CONFIG_DIGIFILLIST, digi.callFilter[0], 140);
	autoReset = (uint8_t)flash_read(CONFIG_AUTORST);
	t = (uint8_t)flash_read(CONFIG_PWM_FLAT);
	afskCfg.usePWM = t & 1;
	afskCfg.flatAudioIn = (t & 2) > 0;
	kissMonitor = (flash_read(CONFIG_KISSMONITOR) == 1);
	ax25Cfg.allowNonAprs = (flash_read(CONFIG_ALLOWNONAPRS) == 1);

	return 1;
}
