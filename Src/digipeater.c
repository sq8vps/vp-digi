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

#include "digipeater.h"
#include "terminal.h"
#include "drivers/modem.h"
#include <string.h>
#include "common.h"
#include "ax25.h"
#include <math.h>
#include "drivers/systick.h"

#define VISCOUS_DATA_LEN (6) //max frames in viscous-delay buffer
uint8_t viscousBuf[VISCOUS_DATA_LEN][FRAMELEN]; //viscous-delay frames buffer
uint32_t viscousData[VISCOUS_DATA_LEN][2]; //viscous-delay hash and timestamp buffer
#define VISCOUS_HOLD_TIME 500 //viscous-delay hold time in 10 ms units


#define DEDUPE_LEN (40) //duplicate protection buffer size (number of hashes)
uint32_t deDupeBuf[DEDUPE_LEN][2]; //duplicate protection hash buffer
uint8_t deDupeIndex = 0; //duplicate protection buffer index


/**
 * @brief Check if frame with specified hash is alread in viscous-delay buffer and delete it if so
 * @param[in] hash Frame hash
 * @return 0 if not in buffer, 1 if in buffer
 */
static uint8_t digi_viscousCheckAndRemove(uint32_t hash)
{
    for(uint8_t i = 0; i < VISCOUS_DATA_LEN; i++)
    {
    	if(viscousData[i][0] == hash) //matching hash
    	{
    		viscousData[i][0] = 0; //clear slot
    		viscousData[i][1] = 0;
    		term_sendMonitor((uint8_t*)"Digipeated frame received, dropping old frame from viscous-delay buffer\r\n", 0);
    		return 1;
    	}
    }
    return 0;
}



void Digi_viscousRefresh(void)
{
    if(digi.viscous == 0) //viscous-digipeating disabled on every alias
    {
    	return;
    }

	for(uint8_t i = 0; i < VISCOUS_DATA_LEN; i++)
    {
    	if((viscousData[i][0] > 0) && ((ticks - viscousData[i][1]) > VISCOUS_HOLD_TIME)) //it's time to transmit this frame
        {
            uint8_t len = strlen((char*)viscousBuf[i]);
            if((len + 2) > (FRAMEBUFLEN - ax25.xmitIdx)) //frame won't fit in tx buffer
            {
            	return;
            }

    		uint16_t begin = ax25.xmitIdx;

    		for(uint8_t j = 0; j < len; j++) //copy frame to tx buffer
            {
                ax25.frameXmit[ax25.xmitIdx] = viscousBuf[i][j];
                ax25.xmitIdx++;
            }
            ax25.frameXmit[ax25.xmitIdx] = 0xFF;
            ax25.xmitIdx++;

            if(kissMonitor) //monitoring mode, send own frames to KISS ports
            {
            	SendKiss(ax25.frameXmit, ax25.xmitIdx - 1);
            }

        	uint8_t buf[200];
        	common_toTNC2((uint8_t*)&ax25.frameXmit[begin], ax25.xmitIdx - begin - 1, buf);

            term_sendMonitor((uint8_t*)"(AX.25) Transmitting viscous-delayed frame: ", 0);
            term_sendMonitor(buf, 0);
            term_sendMonitor((uint8_t*)"\r\n", 0);

            viscousData[i][0] = 0;
            viscousData[i][1] = 0;
        }
    }
}

/**
 * @brief Compare callsign with specified call in call filter table - helper function.
 * @param[in] *call Callsign
 * @param[in] no Callsign filter table index
 * @return 0 if matched with call filter, 1 if not
 */
static uint8_t compareFilterCall(uint8_t *call, uint8_t no)
{
	uint8_t err = 0;

	for(uint8_t i = 0; i < 6; i++)
	{
		if((digi.callFilter[no][i] < 0xff) && ((call[i] >> 1) != digi.callFilter[no][i]))
			err = 1;
	}
	if((digi.callFilter[no][6] < 0xff) && ((call[6] - 96) != digi.callFilter[no][6])) //special case for ssid
		err = 1;
	return err;
}

/**
 * @brief Check frame with call filter
 * @param[in] *call Callsign in incoming frame
 * @param[in] alias Digi alias index currently used
 * @return 0 if accepted, 1 if rejected
 */
static uint8_t filterFrameCheck(uint8_t *call, uint8_t alias)
{

	//filter by call
	if((digi.callFilterEnable >> alias) & 1) //check if enabled
	{
		for(uint8_t i = 0; i < 20; i++)
		{
			if(!compareFilterCall(call, i)) //if callsigns match...
			{
				if((digi.filterPolarity) == 0)
					return 1; //...and blacklist is enabled, drop the frame
				else
					return 0; //...and whitelist is enabled, accept the frame
			}
		}
		//if callsign is not on the list...
		if((digi.filterPolarity) == 0)
			return 0; //...and blacklist is enabled, accept the frame
		else
			return 1; //...and whitelist is enabled, drop the frame

	}
	//filter by call disabled
	return 0;
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
    uint8_t *buf;
    uint16_t bufidx = 0;
    uint8_t viscousSlot = 0; //viscous delay frame slot we will use

    if((alias < 8) && (digi.viscous & (1 << (alias))))
    {
    	for(uint8_t i = 0; i < VISCOUS_DATA_LEN; i++)
        {
        	if(viscousData[i][0] == 0) //look for the first available slot
        	{
        		viscousSlot = i;
        		break;
        	}
        }

    	if((len + 7) > FRAMELEN) //if frame length (+ 7 bytes for inserted call) is bigger than buffer size
    		return; //drop

    	buf = viscousBuf[viscousSlot];
    }
    else //normal mode
    {
    	buf = malloc(FRAMELEN);
    	if(buf == NULL)
    		return;
    }



    if(alias < 8)
    {
    	if(filterFrameCheck(&frame[7], alias)) //push source callsign though the filter
    		return;
    }
    uint8_t ssid = (frame[elStart + 6] >> 1) - 48; //store SSID (N)


    if(alias < 8)
    {
    	if((digi.viscous & (1 << (alias))) || (digi.directOnly & (1 << alias))) //viscous-delay or direct-only enabled
    	{
    		if(elStart != 14)
    			return; //this is not the very first path element, frame not received directly
    		if((alias >= 0) && (alias <= 3) && (ssid != n))
    			return; //n-N type alias, but n is not equal to N, frame not received directly
    	}
    }

    if(simple) //if this is a simple alias, our own call or we treat n-N as a simple alias
    {
    	while(bufidx < (len)) //copy whole frame
    	{
    		buf[bufidx] = frame[bufidx];
    		bufidx++;
    		if(bufidx >= FRAMELEN)
    		{
    			if(buf != NULL)
    				free(buf);
    			return;
    		}
    	}
    	if((alias == 8) || ((digi.traced & (1 << alias)) == 0)) //own call or untraced
    	{
    		buf[elStart + 6] += 128; //add h-bit
    	}
    	else //not our call, but treat it as a simple alias
    	{
    		for(uint8_t i = 0; i < 6; i++) //replace with own call
    			buf[elStart + i] = call[i];

    		buf[elStart + 6] &= 1; //clear everything but path end bit
    		buf[elStart + 6] |= ((callSsid << 1) + 0b11100000); //inset ssid and h-bit
    	}
    }
    else //standard n-N alias
    {
    	while(bufidx < elStart) //copy all data before current path element
    	{
    		buf[bufidx] = frame[bufidx];
    		bufidx++;
    		if(bufidx >= FRAMELEN)
    		{
    			if(buf != NULL)
    				free(buf);
    			return;
    		}
    	}

    	uint16_t shift = 0;

    	if((digi.traced & (1 << alias)) || ((ssid == n) && (elStart == 14))) //if this is a traced alias OR it's not, but this is the very first hop for this packet, insert own call
    	{
    		for(uint8_t i = 0; i < 6; i++) //insert own call
    			buf[bufidx++] = call[i];

    		buf[bufidx++] = ((callSsid << 1) + 0b11100000); //insert ssid and h-bit
    		shift = 7; //additional shift when own call is inserted
    	}

		while(bufidx < (len + shift)) //copy rest of the frame
    	{
    		buf[bufidx] = frame[bufidx - shift];
    		bufidx++;
    		if(bufidx >= FRAMELEN)
    		{
    			if(buf != NULL)
    				free(buf);
    			return;
    		}
    	}

		buf[elStart + shift + 6] -= 2; //decrement SSID in alias (2 beacuse ssid is shifted left by 1)
		if((buf[elStart + shift + 6] & 0b11110) == 0) //if SSID is 0
		{
			buf[elStart + shift + 6] += 128; //add h-bit
		}

    }

	if((alias < 8) && (digi.viscous & (1 << alias)))
	{
    	buf[bufidx++] = 0x00;
		viscousData[viscousSlot][0] = hash;
    	viscousData[viscousSlot][1] = ticks;
		term_sendMonitor((uint8_t*)"Saving frame for viscous-delay digipeating\r\n", 0);
	}
	else
	{
        if((FRAMEBUFLEN - ax25.xmitIdx) > (bufidx + 2))
        {
            deDupeIndex %= DEDUPE_LEN;

            deDupeBuf[deDupeIndex][0] = hash; //store duplicate protection hash
            deDupeBuf[deDupeIndex][1] = ticks; //store timestamp

            deDupeIndex++;

            uint16_t begin = ax25.xmitIdx; //store frame beginning in tx buffer

            for(uint16_t i = 0; i < bufidx; i++) //copy frame to tx buffer
            {
            	ax25.frameXmit[ax25.xmitIdx++] = buf[i];
            }
            ax25.frameXmit[ax25.xmitIdx++] = 0xFF;

            if(kissMonitor) //monitoring mode, send own frames to KISS ports
            {
            	SendKiss(ax25.frameXmit, ax25.xmitIdx - 1);
            }

    		common_toTNC2((uint8_t *)&ax25.frameXmit[begin], ax25.xmitIdx - begin - 1, buf);
    		term_sendMonitor((uint8_t*)"(AX.25) Digipeating frame: ", 0);
    		term_sendMonitor(buf, 0);
    		term_sendMonitor((uint8_t*)"\r\n", 0);
        }
		free(buf);
	}
}



void Digi_digipeat(uint8_t *frame, uint16_t len)
{

    uint16_t t = 13; //path length for now
    while((frame[t] & 1) == 0) //look for path end
    {
        t += 7;
        if(t > 150) return;
    }

    //calculate frame "hash"
    uint32_t hash = crc32(CRC32_INIT, frame, 14); //use destination and source adddress, skip path
    hash = crc32(hash, &frame[t + 1], len - t - 1); //continue through all remaining data

    if(digi.viscous) //viscous-delay enabled on any slot
    {
    	if(digi_viscousCheckAndRemove(hash)) //check if this frame was received twice
    		return; //if so, drop it
    }

    for(uint8_t i = 0; i < DEDUPE_LEN; i++) //check if frame is already in duplicate filtering buffer
    {
        if(deDupeBuf[i][0] == hash)
        {
            if((ticks - deDupeBuf[i][1]) <= (digi.dupeTime * 100))
            	return; //filter duplicate frame
        }
    }


    if(t == 13) //path end bit in source address, no path in this frame
    {
        return; //drop it
    }


    while((frame[t] & 128) == 0) //look for h-bit
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

    for(uint8_t i = 0; i < 6; i++) //compare with our call
    {
    	if(frame[t + i] != call[i])
    		err = 1;
    }
    if(ssid != callSsid) //compare SSID also
    	err = 1;

    if(err == 0) //our callsign is in the path
    {
    	makeFrame(frame, t, len, hash, 8, 1, 0);
    	return;
    }



    for(uint8_t i = 0; i < 4; i++) //check for simple alias match
    {
        err = 0;
    	for(uint8_t j = 0; j < 6; j++)
        {
        	if(frame[t + j] != digi.alias[i + 4][j])
        	{
            	err = 1;
        	}
        }
        if(ssid != digi.ssid[i])
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
    	for(; j < strlen((const char *)digi.alias[i]); j++)
        {
            if(frame[t + j] != digi.alias[i][j]) //check for matching alias
            {
                err = 1; //alias not matching
            }
        }

        if(err == 0) //alias matching, check further
        {
            uint8_t n = ((frame[t + j] >> 1) - 48); //get n from alias (e.g. WIDEn-N) - N is in ssid variable

            //every path must meet several requirements
            //say we have WIDEn-N path
            //N <= n
            //0 < n < 8
            //0 < N < 8
            if(((ssid > 0) && (ssid < 8) && (n > 0) && (n < 8) && (ssid <= n)) == 0) //path is broken or already used (N=0)
            	return;

            //check if n and N <= digi max
            if((n <= digi.max[i]) && (ssid <= digi.max[i]))
            {
                if(digi.enableAlias & (1 << i))
                	makeFrame(frame, t, len, hash, i, 0, n); //process as a standard n-N frame
            }
            else if((digi.rep[i] > 0) && (n >= digi.rep[i])) //else check if n and N >= digi replace
            {
                if(digi.enableAlias & (1 << i))
                	makeFrame(frame, t, len, hash, i, 1, n);
            }
        }
    }


}



void Digi_storeDeDupeFromXmitBuf(uint16_t idx)
{
	uint32_t hash = crc32(CRC32_INIT, &ax25.frameXmit[idx], 14); //calculate for destination and source address


    uint16_t i = 13;

    while((ax25.frameXmit[i] & 1) == 0) //look for path end bit (skip path)
    {
        i++;
    }
    i++;

    while(ax25.frameXmit[i] != 0xFF)
    {
        hash = crc32(hash, &ax25.frameXmit[i++], 1);
    }

    deDupeIndex %= DEDUPE_LEN;

    deDupeBuf[deDupeIndex][0] = hash;
    deDupeBuf[deDupeIndex][1] = ticks;

    deDupeIndex++;
}
