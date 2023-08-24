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

#include "beacon.h"
#include "digipeater.h"
#include "common.h"
#include <string.h>
#include "ax25.h"
#include "terminal.h"
#include "drivers/systick.h"

uint32_t beaconDelay[8] = {0};

Beacon beacon[8];

/**
 * @brief Send specified beacon
 * @param[in] no Beacon number (0-7)
 */
void Beacon_send(uint8_t no)
{
	if(beacon[no].enable == 0)
		return; //beacon disabled

	uint8_t buf[150] = {0}; //frame buffer
	uint16_t idx = 0;

	for(uint8_t i = 0; i < 7; i++) //add destination address
		buf[idx++] = dest[i];

	for(uint8_t i = 0; i < 6; i++) //add source address
		buf[idx++] = call[i];

	buf[idx++] = ((callSsid << 1) + 0b01100000); //add source ssid

	if(beacon[no].path[0] > 0) //this beacon has some path set
	{
		for(uint8_t i = 0; i < 14; i++) //loop through path
		{
			if((beacon[no].path[i] > 0) || (i == 6) || (i == 13)) //normal data, not a NULL symbol
			{
				buf[idx] = (beacon[no].path[i] << 1); //copy path
				if((i == 6) || (i == 13)) //it was and ssid
				{
					buf[idx] += 0b01100000; //add appripriate bits for ssid
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
	for(uint8_t i = 0; i < strlen((char*)beacon[no].data); i++)
	{
		buf[idx++] = beacon[no].data[i]; //copy beacon comment
	}

	if((FRAMEBUFLEN - ax25.xmitIdx) > (idx + 2)) //check for free space in TX buffer
	{
		uint16_t frameStart = ax25.xmitIdx; //store index

		for(uint8_t i = 0; i < idx; i++)
		{
			ax25.frameXmit[ax25.xmitIdx++] = buf[i]; //copy frame to main TX buffer
		}

        if(kissMonitor) //monitoring mode, send own frames to KISS ports
        {
        	SendKiss(ax25.frameXmit, ax25.xmitIdx);
        }

		ax25.frameXmit[ax25.xmitIdx++] = 0xFF; //frame separator
		Digi_storeDeDupeFromXmitBuf(frameStart); //store frame hash in duplicate protection buffer (to prevent from digipeating own packets)

		uint8_t bufto[200];
		common_toTNC2((uint8_t *)&ax25.frameXmit[frameStart], ax25.xmitIdx - frameStart - 1, bufto);

		term_sendMonitor((uint8_t*)"(AX.25) Transmitting beacon ", 0);

		term_sendMonitorNumber(no);
		term_sendMonitor((uint8_t*)": ", 0);
		term_sendMonitor(bufto, 0);
		term_sendMonitor((uint8_t*)"\r\n", 0);
	}



}

/**
 * @brief Check if any beacon should be transmitted and transmit if neccessary
 */
void Beacon_check(void)
{
	for(uint8_t i = 0; i < 8; i++)
	{
		if(beacon[i].enable == 0)
			continue;

		if((beacon[i].interval > 0) && ((ticks >= beacon[i].next) || (beacon[i].next == 0)))
		{
			if(beaconDelay[i] > ticks) //check for beacon delay (only for the very first transmission)
				return;
			beacon[i].next = ticks + beacon[i].interval; //save next beacon timestamp
			beaconDelay[i] = 0;
			Beacon_send(i);
		}
	}
}


/**
 * @brief Initialize beacon module
 */
void Beacon_init(void)
{
	for(uint8_t i = 0; i < 8; i++)
	{
		beaconDelay[i] = (beacon[i].delay * 100) + ticks + 3000; //set delay for beacons and add constant 30 seconds of delay
		beacon[i].next = 0;
	}
}

