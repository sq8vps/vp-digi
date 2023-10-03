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

#include "digipeater.h"
#include "terminal.h"
#include <string.h>
#include "common.h"
#include "ax25.h"
#include <math.h>
#include <modem.h>
#include <systick.h>

struct _DigiConfig DigiConfig;

#define VISCOUS_MAX_FRAME_COUNT 10 //max frames in viscous-delay buffer
#define VISCOUS_MAX_FRAME_SIZE 150

struct ViscousData
{
	uint32_t hash;
	uint32_t timeLimit;
	uint8_t frame[VISCOUS_MAX_FRAME_SIZE];
	uint16_t size;
};
static struct ViscousData viscous[VISCOUS_MAX_FRAME_COUNT];
#define VISCOUS_HOLD_TIME 5000 //viscous-delay hold time in ms

struct DeDupeData
{
	uint32_t hash;
	uint32_t timeLimit;
};

#define DEDUPE_SIZE (50) //duplicate protection buffer size (number of hashes)
static struct DeDupeData deDupe[DEDUPE_SIZE]; //duplicate protection hash buffer
static uint8_t deDupeCount = 0; //duplicate protection buffer index


static uint8_t buf[AX25_FRAME_MAX_SIZE];

/**
 * @brief Check if frame with specified hash is already in viscous-delay buffer and delete it if so
 * @param[in] hash Frame hash
 * @return 0 if not in buffer, 1 if in buffer
 */
static uint8_t viscousCheckAndRemove(uint32_t hash)
{
    for(uint8_t i = 0; i < VISCOUS_MAX_FRAME_COUNT; i++)
    {
    	if(viscous[i].hash == hash) //matching hash
    	{
    		viscous[i].hash = 0; //clear slot
    		viscous[i].timeLimit = 0;
    		viscous[i].size = 0;
    		TermSendToAll(MODE_MONITOR, (uint8_t*)"Digipeated frame received, dropping old frame from viscous-delay buffer\r\n", 0);
    		return 1;
    	}
    }
    return 0;
}



void DigiViscousRefresh(void)
{
    if(DigiConfig.viscous == 0) //viscous digipeating disabled on every alias
    {
    	return;
    }

	for(uint8_t i = 0; i < VISCOUS_MAX_FRAME_COUNT; i++)
    {
    	if((viscous[i].timeLimit > 0) && (SysTickGet() >= viscous[i].timeLimit)) //it's time to transmit this frame
        {
            void *handle = NULL;
            if(NULL != (handle = Ax25WriteTxFrame(viscous[i].frame, viscous[i].size)))
            {
				if(GeneralConfig.kissMonitor) //monitoring mode, send own frames to KISS ports
				{
					TermSendToAll(MODE_KISS, viscous[i].frame, viscous[i].size);
				}

				TermSendToAll(MODE_MONITOR, (uint8_t*)"(AX.25) Transmitting viscous-delayed frame: ", 0);
				SendTNC2(viscous[i].frame, viscous[i].size);
				TermSendToAll(MODE_MONITOR, (uint8_t*)"\r\n", 0);
            }

    		viscous[i].hash = 0; //clear slot
    		viscous[i].timeLimit = 0;
    		viscous[i].size = 0;
        }
    }
}

/**
 * @brief Compare callsign with specified call in call filter table - helper function.
 * @param *call Callsign
 * @param index Callsign filter table index
 * @return 1 if matched, 0 otherwise
 */
static uint8_t compareFilterCall(uint8_t *call, uint8_t index)
{
	uint8_t err = 0;

	for(uint8_t i = 0; i < 6; i++)
	{
		if((DigiConfig.callFilter[index][i] < 0xff) && ((call[i] >> 1) != DigiConfig.callFilter[index][i]))
			err = 1;
	}
	if((DigiConfig.callFilter[index][6] < 0xff) && ((call[6] - 96) != DigiConfig.callFilter[index][6])) //special case for ssid
		err = 1;

	return (err == 0);
}

/**
 * @brief Check frame with call filter
 * @param[in] *call Callsign in incoming frame
 * @param[in] alias Digi alias index currently used
 * @return 1 if accepted, 0 if rejected
 */
static uint8_t filterFrameCheck(uint8_t *call, uint8_t alias)
{
	//filter by call
	if((DigiConfig.callFilterEnable >> alias) & 1) //check if enabled
	{
		for(uint8_t i = 0; i < (sizeof(DigiConfig.callFilter) / sizeof(DigiConfig.callFilter[0])); i++)
		{
			if(compareFilterCall(call, i)) //if callsigns match...
			{
				if(DigiConfig.filterPolarity == 0)
					return 0; //...and blacklist is enabled, drop the frame
				else
					return 1; //...and whitelist is enabled, accept the frame
			}
		}
		//if callsign is not on the list...
		if((DigiConfig.filterPolarity) == 0)
			return 1; //...and blacklist is enabled, accept the frame
		else
			return 0; //...and whitelist is enabled, drop the frame

	}
	//filter by call disabled
	return 1;
}


/**
 * @brief Produce and push digipeated frame to transmit buffer
 * @param[in] *frame Pointer to frame buffer
 * @param[in] elStart Index of the current path element very first byte
 * @param[in] len Frame length
 * @param[in] hash Frame hash
 * @param[in] alias Alias number: 0-3 - n-N aliases, 4-7 - simple aliases, 8 - own call
 * @param[in] simple If 1, it is a simple alias or should be treated as a simple alias
 * @param[in] n Number in n-N type alias, e.g. in WIDE2-1 n=2
 */
static void makeFrame(uint8_t *frame, uint16_t elStart, uint16_t len, uint32_t hash, uint8_t alias, uint8_t simple, uint8_t n)
{
    uint16_t _index = 0; //underlying index for buffer if not in viscous-delay mode
    uint8_t *buffer; //buffer to store frame being prepared
    uint16_t *index = &_index; //index in buffer
    uint8_t viscousSlot = 0; //viscous delay frame slot we will use

    if((alias < 8) && (DigiConfig.viscous & (1 << (alias)))) //viscous delay mode
    {
    	for(uint8_t i = 0; i < VISCOUS_MAX_FRAME_COUNT; i++)
        {
        	if(viscous[i].timeLimit == 0) //look for the first available slot
        	{
        		viscousSlot = i;
        		break;
        	}
        }

    	if((len + 7) > VISCOUS_MAX_FRAME_SIZE) //if frame length (+ 7 bytes for inserted call) is bigger than buffer size
    		return; //drop

    	buffer = viscous[viscousSlot].frame;
    	index = &(viscous[viscousSlot].size);
    	*index = 0;
    }
    else //normal mode
    {
    	if((uint16_t)sizeof(buf) < (len + 7))
    		return;
    	buffer = buf;
    }


    if(alias < 8)
    {
    	if(!filterFrameCheck(&frame[7], alias)) //push source callsign through the filter
    		return;
    }
    uint8_t ssid = (frame[elStart + 6] >> 1) - 48; //store SSID (N)


    if(alias < 8)
    {
    	if((DigiConfig.viscous & (1 << (alias))) || (DigiConfig.directOnly & (1 << alias))) //viscous-delay or direct-only enabled
    	{
    		if(elStart != 14)
    			return; //this is not the very first path element, frame not received directly
    		if((alias <= 3) && (ssid != n))
    			return; //n-N type alias, but n is not equal to N, frame not received directly
    	}
    }

    if(simple) //if this is a simple alias, our own call or we treat n-N as a simple alias
    {
    	while(*index < len) //copy whole frame
    	{
    		buffer[*index] = frame[*index];
    		(*index)++;
    	}

    	if((alias == 8) || ((DigiConfig.traced & (1 << alias)) == 0)) //own call or untraced
    	{
    		buffer[elStart + 6] += 128; //add h-bit
    	}
    	else //not our call, but treat it as a simple alias
    	{
    		for(uint8_t i = 0; i < sizeof(GeneralConfig.call); i++) //replace with own call
    			buffer[elStart + i] = GeneralConfig.call[i];

    		buffer[elStart + 6] &= 1; //clear everything but path end bit
    		buffer[elStart + 6] |= ((GeneralConfig.callSsid << 1) + 0b11100000); //insert ssid and h-bit
    	}
    }
    else //standard n-N alias
    {
    	while(*index < elStart) //copy all data before current path element
    	{
    		buffer[*index] = frame[*index];
    		(*index)++;
    	}

    	uint16_t shift = 0;

    	//insert own callsign to path if:
    	//1. this is a traced alias OR
    	//2. this is an untraced alias, but it is the very first hop (heard directly)
    	if((DigiConfig.traced & (1 << alias)) || ((ssid == n) && (elStart == 14)))
    	{
    		for(uint8_t i = 0; i < sizeof(GeneralConfig.call); i++) //insert own call
    			buffer[(*index)++] = GeneralConfig.call[i];

    		buffer[(*index)++] = ((GeneralConfig.callSsid << 1) + 0b11100000); //insert ssid and h-bit
    		shift = 7; //additional shift when own call is inserted
    	}

		while(*index < (len + shift)) //copy rest of the frame
    	{
    		buffer[*index] = frame[*index - shift];
    		(*index)++;
    	}

		buffer[elStart + shift + 6] -= 2; //decrement SSID in alias (2 because ssid is shifted left by 1)
		if((buffer[elStart + shift + 6] & 0b11110) == 0) //if SSID is 0
		{
			buffer[elStart + shift + 6] += 0x80; //add h-bit
		}

    }

	if((alias < 8) && (DigiConfig.viscous & (1 << alias)))
	{
		viscous[viscousSlot].hash = hash;
    	viscous[viscousSlot].timeLimit = SysTickGet() + (VISCOUS_HOLD_TIME / SYSTICK_INTERVAL);
		TermSendToAll(MODE_MONITOR, (uint8_t*)"Saving frame for viscous-delay digipeating\r\n", 0);
	}
	else
	{
		void *handle = NULL;
        if(NULL != (handle = Ax25WriteTxFrame(buffer, *index)))
        {
            DigiStoreDeDupe(buffer, *index);

			if(GeneralConfig.kissMonitor) //monitoring mode, send own frames to KISS ports
			{
				TermSendToAll(MODE_KISS, buffer, *index);
			}

			TermSendToAll(MODE_MONITOR, (uint8_t*)"(AX.25) Digipeating frame: ", 0);
			SendTNC2(buffer, *index);
			TermSendToAll(MODE_MONITOR, (uint8_t*)"\r\n", 0);
        }
	}
}



void DigiDigipeat(uint8_t *frame, uint16_t len)
{
	if(!DigiConfig.enable)
		return;

    uint16_t t = 13; //start from first byte that can contain path end bit
    while((frame[t] & 1) == 0) //look for path end
    {
    	if((t + 7) >= len)
    		return;
        t += 7;
    }

    //calculate frame "hash"
    uint32_t hash = Crc32(CRC32_INIT, frame, 14); //use destination and source address, skip path
    hash = Crc32(hash, &frame[t + 1], len - t - 1); //continue through all remaining data

    if(DigiConfig.viscous) //viscous-delay enabled on any slot
    {
    	if(viscousCheckAndRemove(hash)) //check if this frame was received twice
    		return; //if so, drop it
    }

    for(uint8_t i = 0; i < DEDUPE_SIZE; i++) //check if frame is already in duplicate filtering buffer
    {
        if(deDupe[i].hash == hash)
        {
            if(SysTickGet() < (deDupe[i].timeLimit))
            	return; //filter out duplicate frame
        }
    }


    if(t == 13) //path end bit in source address, no path in this frame
    {
        return; //drop it
    }


    while((frame[t] & 0x80) == 0) //look for h-bit
    {
        if(t == 13)
        {
            break; //no h-bit found and we are in source address, we can proceed with the first path element
        }
        t -= 7; //look backwards for h-bit
    }
    t++; //now t is the index for the first byte in path element we want to process

    uint8_t ssid = ((frame[t + 6] >> 1) - 0b00110000); //current path element SSID

    uint8_t err = 0;

    for(uint8_t i = 0; i < sizeof(GeneralConfig.call); i++) //compare with our call
    {
    	if(frame[t + i] != GeneralConfig.call[i])
    	{
    		err = 1;
    		break;
    	}
    }
    if(ssid != GeneralConfig.callSsid) //compare SSID also
    	err = 1;

    if(err == 0) //our callsign is in the path
    {
    	makeFrame(frame, t, len, hash, 8, 1, 0);
    	return;
    }

    for(uint8_t i = 0; i < 4; i++) //check for simple alias match
    {
        err = 0;
    	for(uint8_t j = 0; j < sizeof(DigiConfig.alias[0]); j++)
        {
        	if(frame[t + j] != DigiConfig.alias[i + 4][j])
        	{
            	err = 1;
            	break;
        	}
        }
        if(ssid != DigiConfig.ssid[i])
        	err = 1;

        if(err == 0) //no error
        {
        	makeFrame(frame, t, len, hash, i + 4, 1, 0);
        	return;
        }
    }

    //n-N style alias handling

    for(uint8_t i = 0; i < 4; i++)
    {
        err = 0;
        uint8_t j = 0;
    	for(; j < strlen((const char *)DigiConfig.alias[i]); j++)
        {
            if(frame[t + j] != DigiConfig.alias[i][j]) //check for matching alias
            {
                err = 1; //alias not matching
                break;
            }
        }

        if(err == 0) //alias matching, check further
        {
            uint8_t n = ((frame[t + j] >> 1) - 48); //get n from alias (e.g. WIDEn-N) - N is in ssid variable

            //every path must meet several requirements
            //say we have a WIDEn-N path. Then:
            //N <= n
            //0 < n < 8
            //0 < N < 8
            if(((ssid > 0) && (ssid < 8) && (n > 0) && (n < 8) && (ssid <= n)) == 0) //path is broken or already used (N=0)
            	return;

            //check if n and N <= digi max
            if((n <= DigiConfig.max[i]) && (ssid <= DigiConfig.max[i]))
            {
                if(DigiConfig.enableAlias & (1 << i))
                	makeFrame(frame, t, len, hash, i, 0, n); //process as a standard n-N frame
            }
            else if((DigiConfig.rep[i] > 0) && (n >= DigiConfig.rep[i])) //else check if n and N >= digi replace
            {
                if(DigiConfig.enableAlias & (1 << i))
                	makeFrame(frame, t, len, hash, i, 1, n);
            }
        }
    }


}



void DigiStoreDeDupe(uint8_t *buf, uint16_t size)
{
	uint32_t hash = Crc32(CRC32_INIT, buf, 14); //calculate for destination and source address

    uint16_t i = 13;

    while((buf[i] & 1) == 0) //look for path end bit (skip path)
    {
        i++;
    }
    i++;

    hash = Crc32(hash, &buf[i], size - i);

    deDupeCount %= DEDUPE_SIZE;

    deDupe[deDupeCount].hash = hash;
    deDupe[deDupeCount].timeLimit = SysTickGet() + (DigiConfig.dupeTime * 1000 / SYSTICK_INTERVAL);

    deDupeCount++;
}
