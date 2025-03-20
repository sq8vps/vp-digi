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

#include "modem.h"
#include "uart.h"
#include "config.h"
#include "common.h"
#include "usbd_cdc_if.h"
#include "digipeater.h"
#include "ax25.h"
#include "beacon.h"
#include "drivers/config_ll.h"

/**
* @brief Read string from configuration space
* @param address Relative address
* @param *data Output string pointer
* @param len Number of bytes
*/
static void readString(uint32_t address, void *data, uint16_t len)
{
	uint8_t *d = data;
	uint16_t i = 0;
	for(; i < len; i += 2)
	{
		uint16_t t = read(address + i);
		d[i] = t & 0xFF;
		d[i + 1] = t >> 8;
	}
	if(len & 1)
	{
		d[i] = read(address + i);
	}
}

/**
* @brief Write string to configuration space
* @param address Relative address
* @param *data Input string pointer
* @param len Number of bytes
*/
static void writeString(uint32_t address, const void *data, uint16_t len)
{
	const uint8_t *d = data;
	uint16_t i = 0;
	for(; i < len; i += 2)
	{
		write(address + i, (uint16_t)d[i] | ((uint16_t)d[i + 1] << 8));
	}
	if(len & 1)
	{
		write(address + i, d[i]);
	}
}

void ConfigErase(void)
{
	unlock();
	erase();
	lock();
}


#ifdef BLUE_PILL

#define CONFIG_FLAG_WRITTEN 0x6B

//these are relative addresses, absolute address is calculated as relative address + MEM_CONFIG
//all fields are 16-bit or n*16-bit long, as data in flash is stored in 16-bit words
#define CONFIG_FLAG 0 //configuration written flag
#define CONFIG_CALL 2
#define CONFIG_SSID 8
#define CONFIG_TXDELAY 10
#define CONFIG_TXTAIL 12
#define CONFIG_TXQUIET 14
#define CONFIG_RS1BAUD 16
#define CONFIG_RS2BAUD 20
#define CONFIG_BEACONS 24
#define CONFIG_BCIV 26 //beacon intervals
#define CONFIG_BCDL 42 //beacon delays
#define CONFIG_BC0 58 //beacon information fields, null terminated
#define CONFIG_BC1 158
#define CONFIG_BC2 258
#define CONFIG_BC3 358
#define CONFIG_BC4 458
#define CONFIG_BC5 558
#define CONFIG_BC6 658
#define CONFIG_BC7 758
#define CONFIG_BCP0 858 //beacon paths, 14 bytes each
#define CONFIG_BCP1 872
#define CONFIG_BCP2 886
#define CONFIG_BCP3 900
#define CONFIG_BCP4 914
#define CONFIG_BCP5 928
#define CONFIG_BCP6 942
#define CONFIG_BCP7 956
#define CONFIG_DIGION 970
#define CONFIG_DIGIEN 972
#define CONFIG_DIGIVISC 974 //viscous-delay settings in higher half, direct-only in lower half
#define CONFIG_DIGIAL0 976
#define CONFIG_DIGIAL1 982
#define CONFIG_DIGIAL2 988
#define CONFIG_DIGIAL3 994
#define CONFIG_DIGIAL4 1000
#define CONFIG_DIGIAL5 1008
#define CONFIG_DIGIAL6 1016
#define CONFIG_DIGIAL7 1024
#define CONFIG_DIGIMAX0 1032
#define CONFIG_DIGIMAX1 1034
#define CONFIG_DIGIMAX2 1036
#define CONFIG_DIGIMAX3 1038
#define CONFIG_DIGIREP0 1040
#define CONFIG_DIGIREP1 1042
#define CONFIG_DIGIREP2 1044
#define CONFIG_DIGIREP3 1046
#define CONFIG_DIGITRACE 1048
#define CONFIG_DIGIDEDUPE 1050
#define CONFIG_DIGICALLFILEN 1052
#define CONFIG_DIGIFILLIST 1054
#define CONFIG_DIGIFILTYPE 1194
#define CONFIG_DIGISSID4 1196
#define CONFIG_DIGISSID5 1198
#define CONFIG_DIGISSID6 1200
#define CONFIG_DIGISSID7 1202
#define CONFIG_PWM_FLAT_PTT 1204
#define CONFIG_KISSMONITOR 1206
#define CONFIG_DEST 1208
#define CONFIG_ALLOWNONAPRS 1214
#define CONFIG_FX25 1216
#define CONFIG_MODEM 1218
#define CONFIG_MODE_USB 1220
#define CONFIG_MODE_UART1 1222
#define CONFIG_MODE_UART2 1224
#define CONFIG_TX_LEVEL 1226
#define CONFIG_XXX 1228 //next address (not used)

void ConfigWrite(void)
{
	ConfigErase();

	unlock();

	writeString(CONFIG_CALL, GeneralConfig.call, 6);
	write(CONFIG_SSID, GeneralConfig.callSsid);
	writeString(CONFIG_DEST, GeneralConfig.dest, 6);
	write(CONFIG_TXDELAY, Ax25Config.txDelayLength);
	write(CONFIG_TXTAIL, Ax25Config.txTailLength);
	write(CONFIG_TXQUIET, Ax25Config.quietTime);
#ifdef BLUE_PILL
	writeString(CONFIG_RS1BAUD, (uint8_t*)&Uart1.config.baudrate, 4);
	write(CONFIG_MODE_UART1, Uart1.config.defaultMode);
	writeString(CONFIG_RS2BAUD, (uint8_t*)&Uart2.config.baudrate, 4);
	write(CONFIG_MODE_UART2, Uart2.config.defaultMode);
#endif
	write(CONFIG_BEACONS, (BeaconConfig[0].enable > 0) | ((BeaconConfig[1].enable > 0) << 1) | ((BeaconConfig[2].enable > 0) << 2) | ((BeaconConfig[3].enable > 0) << 3) | ((BeaconConfig[4].enable > 0) << 4) | ((BeaconConfig[5].enable > 0) << 5) | ((BeaconConfig[6].enable > 0) << 6) | ((BeaconConfig[7].enable > 0) << 7));
	for(uint8_t s = 0; s < 8; s++)
	{
		write(CONFIG_BCIV + (2 * s), BeaconConfig[s].interval / 6000);
	}
	for(uint8_t s = 0; s < 8; s++)
	{
		write(CONFIG_BCDL + (2 * s), BeaconConfig[s].delay / 60);
	}
	writeString(CONFIG_BC0, BeaconConfig[0].data, 100);
	writeString(CONFIG_BC1, BeaconConfig[1].data, 100);
	writeString(CONFIG_BC2, BeaconConfig[2].data, 100);
	writeString(CONFIG_BC3, BeaconConfig[3].data, 100);
	writeString(CONFIG_BC4, BeaconConfig[4].data, 100);
	writeString(CONFIG_BC5, BeaconConfig[5].data, 100);
	writeString(CONFIG_BC6, BeaconConfig[6].data, 100);
	writeString(CONFIG_BC7, BeaconConfig[7].data, 100);
	writeString(CONFIG_BCP0, BeaconConfig[0].path, 14);
	writeString(CONFIG_BCP1, BeaconConfig[1].path, 14);
	writeString(CONFIG_BCP2, BeaconConfig[2].path, 14);
	writeString(CONFIG_BCP3, BeaconConfig[3].path, 14);
	writeString(CONFIG_BCP4, BeaconConfig[4].path, 14);
	writeString(CONFIG_BCP5, BeaconConfig[5].path, 14);
	writeString(CONFIG_BCP6, BeaconConfig[6].path, 14);
	writeString(CONFIG_BCP7, BeaconConfig[7].path, 14);
	write(CONFIG_DIGION, DigiConfig.enable);
	write(CONFIG_DIGIEN, DigiConfig.enableAlias);
	write(CONFIG_DIGIVISC, ((uint16_t)DigiConfig.viscous << 8) | (uint16_t)DigiConfig.directOnly);
	writeString(CONFIG_DIGIAL0, DigiConfig.alias[0], 5);
	writeString(CONFIG_DIGIAL1, DigiConfig.alias[1], 5);
	writeString(CONFIG_DIGIAL2, DigiConfig.alias[2], 5);
	writeString(CONFIG_DIGIAL3, DigiConfig.alias[3], 5);
	writeString(CONFIG_DIGIAL4, DigiConfig.alias[4], 6);
	writeString(CONFIG_DIGIAL5, DigiConfig.alias[5], 6);
	writeString(CONFIG_DIGIAL6, DigiConfig.alias[6], 6);
	writeString(CONFIG_DIGIAL7, DigiConfig.alias[7], 6);
	write(CONFIG_DIGISSID4, DigiConfig.ssid[0]);
	write(CONFIG_DIGISSID5, DigiConfig.ssid[1]);
	write(CONFIG_DIGISSID6, DigiConfig.ssid[2]);
	write(CONFIG_DIGISSID7, DigiConfig.ssid[3]);
	write(CONFIG_DIGIMAX0, DigiConfig.max[0]);
	write(CONFIG_DIGIMAX1, DigiConfig.max[1]);
	write(CONFIG_DIGIMAX2, DigiConfig.max[2]);
	write(CONFIG_DIGIMAX3, DigiConfig.max[3]);
	write(CONFIG_DIGIREP0, DigiConfig.rep[0]);
	write(CONFIG_DIGIREP1, DigiConfig.rep[1]);
	write(CONFIG_DIGIREP2, DigiConfig.rep[2]);
	write(CONFIG_DIGIREP3, DigiConfig.rep[3]);
	write(CONFIG_DIGITRACE, DigiConfig.traced);
	write(CONFIG_DIGIDEDUPE, DigiConfig.dupeTime);
	write(CONFIG_DIGICALLFILEN, DigiConfig.callFilterEnable);
	write(CONFIG_DIGIFILTYPE, DigiConfig.filterPolarity);
	writeString(CONFIG_DIGIFILLIST, DigiConfig.callFilter[0], sizeof(DigiConfig.callFilter));
	write(CONFIG_PWM_FLAT_PTT, ModemConfig.usePWM | (ModemConfig.flatAudioIn << 1) | (ModemConfig.pttOutput << 2));
	write(CONFIG_TX_LEVEL, ModemConfig.txLevel);
	write(CONFIG_KISSMONITOR, GeneralConfig.kissMonitor);
	write(CONFIG_ALLOWNONAPRS, Ax25Config.allowNonAprs);
	write(CONFIG_FX25, Ax25Config.fx25 | (Ax25Config.fx25Tx << 1));
	write(CONFIG_MODEM, ModemConfig.modem);
	write(CONFIG_MODE_USB, UartUsb.config.defaultMode);

	write(CONFIG_FLAG, CONFIG_FLAG_WRITTEN);

	lock();

}

uint8_t ConfigRead(void)
{
	if(read(CONFIG_FLAG) != CONFIG_FLAG_WRITTEN) //no configuration stored
	{
		return 0;
	}
	readString(CONFIG_CALL, GeneralConfig.call, sizeof(GeneralConfig.call));
	GeneralConfig.callSsid = (uint8_t)read(CONFIG_SSID);
	uint8_t temp[6];
	readString(CONFIG_DEST, temp, 6);
	if((temp[0] >= ('A' << 1)) && (temp[0] <= ('Z' << 1)) && ((temp[0] & 1) == 0)) //check if stored destination address is correct (we just assume it by reading the first byte)
	{
		memcpy(GeneralConfig.dest, temp, 6);
	}
	Ax25Config.txDelayLength = read(CONFIG_TXDELAY);
	Ax25Config.txTailLength = read(CONFIG_TXTAIL);
	Ax25Config.quietTime = read(CONFIG_TXQUIET);
#ifdef BLUE_PILL
	readString(CONFIG_RS1BAUD, (uint8_t*)&Uart1.config.baudrate, 4);
	Uart1.config.defaultMode = read(CONFIG_MODE_UART1);
	readString(CONFIG_RS2BAUD, (uint8_t*)&Uart2.config.baudrate, 4);
	Uart2.config.defaultMode = read(CONFIG_MODE_UART2);
#endif
	uint8_t bce = (uint8_t)read(CONFIG_BEACONS);
	BeaconConfig[0].enable = (bce & 1) > 0;
	BeaconConfig[1].enable = (bce & 2) > 0;
	BeaconConfig[2].enable = (bce & 4) > 0;
	BeaconConfig[3].enable = (bce & 8) > 0;
	BeaconConfig[4].enable = (bce & 16) > 0;
	BeaconConfig[5].enable = (bce & 32) > 0;
	BeaconConfig[6].enable = (bce & 64) > 0;
	BeaconConfig[7].enable = (bce & 128) > 0;
	for(uint8_t s = 0; s < BEACON_COUNT; s++)
	{
		 BeaconConfig[s].interval = read(CONFIG_BCIV + (2 * s)) * 6000;
	}
	for(uint8_t s = 0; s < BEACON_COUNT; s++)
	{
		 BeaconConfig[s].delay = read(CONFIG_BCDL + (2 * s)) * 60;
	}

	for(uint8_t g = 0; g < BEACON_COUNT; g++)
	{
		readString(CONFIG_BC0 + (g * 100), BeaconConfig[g].data, 100);
	}
	for(uint8_t g = 0; g < BEACON_COUNT; g++)
	{
		readString(CONFIG_BCP0 + (g * 14), BeaconConfig[g].path, 14);
	}
	DigiConfig.enable = (uint8_t)read(CONFIG_DIGION);
	DigiConfig.enableAlias = (uint8_t)read(CONFIG_DIGIEN);
	uint16_t t = read(CONFIG_DIGIVISC);
	DigiConfig.viscous = (t & 0xFF00) >> 8;
	DigiConfig.directOnly = t & 0xFF;
	readString(CONFIG_DIGIAL0, DigiConfig.alias[0], 5);
	readString(CONFIG_DIGIAL1, DigiConfig.alias[1], 5);
	readString(CONFIG_DIGIAL2, DigiConfig.alias[2], 5);
	readString(CONFIG_DIGIAL3, DigiConfig.alias[3], 5);
	readString(CONFIG_DIGIAL4, DigiConfig.alias[4], 6);
	readString(CONFIG_DIGIAL5, DigiConfig.alias[5], 6);
	readString(CONFIG_DIGIAL6, DigiConfig.alias[6], 6);
	readString(CONFIG_DIGIAL7, DigiConfig.alias[7], 6);
	DigiConfig.ssid[0] = (uint8_t)read(CONFIG_DIGISSID4);
	DigiConfig.ssid[1] = (uint8_t)read(CONFIG_DIGISSID5);
	DigiConfig.ssid[2] = (uint8_t)read(CONFIG_DIGISSID6);
	DigiConfig.ssid[3] = (uint8_t)read(CONFIG_DIGISSID7);
	DigiConfig.max[0] = (uint8_t)read(CONFIG_DIGIMAX0);
	DigiConfig.max[1] = (uint8_t)read(CONFIG_DIGIMAX1);
	DigiConfig.max[2] = (uint8_t)read(CONFIG_DIGIMAX2);
	DigiConfig.max[3] = (uint8_t)read(CONFIG_DIGIMAX3);
	DigiConfig.rep[0] = (uint8_t)read(CONFIG_DIGIREP0);
	DigiConfig.rep[1] = (uint8_t)read(CONFIG_DIGIREP1);
	DigiConfig.rep[2] = (uint8_t)read(CONFIG_DIGIREP2);
	DigiConfig.rep[3] = (uint8_t)read(CONFIG_DIGIREP3);
	DigiConfig.traced = (uint8_t)read(CONFIG_DIGITRACE);
	DigiConfig.dupeTime = (uint8_t)read(CONFIG_DIGIDEDUPE);
	DigiConfig.callFilterEnable = (uint8_t)read(CONFIG_DIGICALLFILEN);
	DigiConfig.filterPolarity = (uint8_t)read(CONFIG_DIGIFILTYPE);
	readString(CONFIG_DIGIFILLIST, DigiConfig.callFilter[0], 140);
	t = (uint8_t)read(CONFIG_PWM_FLAT_PTT);
	ModemConfig.usePWM = t & 1;
	ModemConfig.flatAudioIn = (t & 2) > 0;
	ModemConfig.pttOutput = (t & 4) > 0;
	ModemConfig.txLevel = read(CONFIG_TX_LEVEL);
	GeneralConfig.kissMonitor = (read(CONFIG_KISSMONITOR) == 1);
	Ax25Config.allowNonAprs = (read(CONFIG_ALLOWNONAPRS) == 1);
	t = (uint8_t)read(CONFIG_FX25);
	Ax25Config.fx25 = t & 1;
	Ax25Config.fx25Tx = (t & 2) > 0;
	ModemConfig.modem = read(CONFIG_MODEM);
	UartUsb.config.defaultMode = read(CONFIG_MODE_USB);


	return 1;
}

#elif defined(AIOC)

#define CONFIG_FLAG_WRITTEN 0xA10C

#define CONFIG_FLAG 0 //configuration written flag
#define CONFIG_GENERAL 8 //GeneralConfig
#define CONFIG_AX25 (CONFIG_GENERAL + 64) //Ax25Config
#define CONFIG_BEACON (CONFIG_AX25 + 64) //BeaconConfig
#define CONFIG_MODEM (CONFIG_BEACON + 144 * 8) //ModemConfig
#define CONFIG_DIGI (CONFIG_MODEM + 256) //DigiConfig
#define CONFIG_USB (CONFIG_DIGI + 64) //UartUsb.config


void ConfigWrite(void)
{
	ConfigErase();

	unlock();

	writeString(CONFIG_GENERAL, &GeneralConfig, sizeof(GeneralConfig));
	writeString(CONFIG_AX25, &Ax25Config, sizeof(Ax25Config));
	writeString(CONFIG_BEACON, BeaconConfig, sizeof(DigiConfig));
	writeString(CONFIG_MODEM, &ModemConfig, sizeof(ModemConfig));
	writeString(CONFIG_DIGI, &DigiConfig, sizeof(DigiConfig));
	writeString(CONFIG_USB, &(UartUsb.config), sizeof(UartUsb.config));
	write(CONFIG_FLAG, CONFIG_FLAG_WRITTEN);

	lock();
}

uint8_t ConfigRead(void)
{
	if(read(CONFIG_FLAG) != CONFIG_FLAG_WRITTEN) //no configuration stored
	{
		return 0;
	}

	readString(CONFIG_GENERAL, &GeneralConfig, sizeof(GeneralConfig));
	readString(CONFIG_AX25, &Ax25Config, sizeof(Ax25Config));
	readString(CONFIG_BEACON, BeaconConfig, sizeof(DigiConfig));
	readString(CONFIG_MODEM, &ModemConfig, sizeof(ModemConfig));
	readString(CONFIG_DIGI, &DigiConfig, sizeof(DigiConfig));
	readString(CONFIG_USB, &(UartUsb.config), sizeof(UartUsb.config));

	return 1;
}

#endif
