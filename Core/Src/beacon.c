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

#include "beacon.h"
#include "digipeater.h"
#include "common.h"
#include <string.h>
#include "ax25.h"
#include "terminal.h"

struct Beacon BeaconConfig[BEACON_COUNT];

static uint32_t beaconDelay[8] = {0};
static uint8_t buf[150]; //frame buffer

/**
 * @brief Send specified BeaconConfig
 * @param[in] no BeaconConfig number (0-7)
 */
void BeaconSend(uint8_t number)
{
	if(number >= BEACON_COUNT)
		return;

	if(BeaconConfig[number].enable == 0)
		return; //BeaconConfig disabled

	uint16_t idx = 0;

	for(uint8_t i = 0; i < sizeof(GeneralConfig.dest); i++) //add destination address
		buf[idx++] = GeneralConfig.dest[i];

	for(uint8_t i = 0; i < sizeof(GeneralConfig.call); i++) //add source address
		buf[idx++] = GeneralConfig.call[i];

	buf[idx++] = ((GeneralConfig.callSsid << 1) + 0b01100000); //add source ssid

	if(BeaconConfig[number].path[0] > 0) //this BeaconConfig has some path set
	{
		for(uint8_t i = 0; i < 14; i++) //loop through path
		{
			if((BeaconConfig[number].path[i] > 0) || (i == 6) || (i == 13)) //normal data, not a NULL symbol
			{
				buf[idx] = BeaconConfig[number].path[i]; //copy path
				if((i == 6) || (i == 13)) //it was and ssid
				{
					buf[idx] = ((buf[idx] << 1) + 0b01100000); //add appropriate bits for ssid
				}
				idx++;
			}
			else //NULL in path
				break; //end here
		}
	}
	buf[idx - 1] |= 1; //add c-bit on the last element
	buf[idx++] = 0x03; //control
	buf[idx++] = 0xF0; //pid
	for(uint8_t i = 0; i < strlen((char*)BeaconConfig[number].data); i++)
	{
		buf[idx++] = BeaconConfig[number].data[i]; //copy BeaconConfig comment
	}

	void *handle = NULL;
	if(NULL != (handle = Ax25WriteTxFrame(buf, idx))) //try to write frame to TX buffer
	{
        if(GeneralConfig.kissMonitor) //monitoring mode, send own frames to KISS ports
        {
        	TermSendToAll(MODE_KISS, buf, idx);
        }

		DigiStoreDeDupe(buf, idx); //store frame hash in duplicate protection buffer (to prevent from digipeating own packets)

		TermSendToAll(MODE_MONITOR, (uint8_t*)"(AX.25) Transmitting beacon ", 0);

		TermSendNumberToAll(MODE_MONITOR, number);
		TermSendToAll(MODE_MONITOR, (uint8_t*)": ", 0);
		SendTNC2(buf, idx);
		TermSendToAll(MODE_MONITOR, (uint8_t*)"\r\n", 0);
	}

}

/**
 * @brief Check if any BeaconConfig should be transmitted and transmit if necessary
 */
void BeaconCheck(void)
{
	for(uint8_t i = 0; i < BEACON_COUNT; i++)
	{
		if(BeaconConfig[i].enable == 0)
			continue;

		if((BeaconConfig[i].interval > 0) && ((HAL_GetTick() >= BeaconConfig[i].next) || (BeaconConfig[i].next == 0)))
		{
			if(beaconDelay[i] > HAL_GetTick()) //check for BeaconConfig delay (only for the very first transmission)
				continue;
			BeaconConfig[i].next = HAL_GetTick() + BeaconConfig[i].interval; //save next BeaconConfig timestamp
			beaconDelay[i] = 0;
			BeaconSend(i);
		}
	}
}


/**
 * @brief Initialize BeaconConfig module
 */
void BeaconInit(void)
{
	for(uint8_t i = 0; i < BEACON_COUNT; i++)
	{
		beaconDelay[i] = (BeaconConfig[i].delay * SYSTICK_FREQUENCY) + HAL_GetTick() + (30000 / SYSTICK_INTERVAL); //set delay for beacons and add constant 30 seconds of delay
		BeaconConfig[i].next = 0;
	}
}

