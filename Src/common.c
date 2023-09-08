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

#include <math.h>
#include <stdlib.h>
#include "common.h"
#include "ax25.h"
#include "usbd_cdc_if.h"

struct _GeneralConfig GeneralConfig =
{
		.call = {'N' << 1, '0' << 1, 'C' << 1, 'A' << 1, 'L' << 1, 'L' << 1},
		.callSsid = 0,
		.dest = {130, 160, 156, 172, 96, 98, 96}, //destination address: APNV01-0 by default. SSID MUST remain 0.
		.kissMonitor = 0,
};

const char versionString[] = "VP-Digi v. 2.0.0\r\nThe open-source standalone APRS digipeater controller and KISS TNC\r\n"
#ifdef ENABLE_FX25
		"With FX.25 support compiled-in\r\n"
#endif
		;

static uint64_t pow10i(uint16_t exp)
{
	if(exp == 0)
		return 1;
	uint64_t n = 1;
	while(exp--)
		n *= 10;
	return n;
}

int64_t StrToInt(const char *str, uint16_t len)
{
	if(len == 0)
		len = strlen(str);

	int64_t tmp = 0;
	for(int32_t i = (len - 1); i >= 0; i--)
	{
		if((i == 0) && (str[0] == '-'))
		{
			return -tmp;
		}
		else if(IS_NUMBER(str[i]))
			tmp += ((str[i] - '0') * pow10i(len - 1 - i));
		else
			return 0;
	}
	return tmp;
}


int16_t Random(int16_t min, int16_t max)
{
    int16_t tmp;
    if (max >= min)
        max -= min;
    else
    {
        tmp = min - max;
        min = max;
        max = tmp;
    }
    return max ? (rand() % max + min) : min;
}

static void sendTNC2ToUart(Uart *uart, uint8_t *from, uint16_t len)
{
	for(uint8_t i = 0; i < 6; i++) //source call
	{
		if((from[7 + i] >> 1) != ' ') //skip spaces
		{
			UartSendByte(uart, from[7 + i] >> 1);
		}
	}

	uint8_t ssid = ((from[13] >> 1) & 0b00001111); //store ssid
	if(ssid > 0) //SSID >0
	{
		UartSendByte(uart, '-'); //add -
		UartSendNumber(uart, ssid);
	}

	UartSendByte(uart, '>'); //first separator

	for(uint8_t i = 0; i < 6; i++) //destination call
	{
		if((from[i] >> 1) != ' ') //skip spaces
		{
			UartSendByte(uart, from[i] >> 1);
		}
	}
	ssid = ((from[6] >> 1) & 0b00001111); //store ssid

	if(ssid > 0) //SSID >0
	{
		UartSendByte(uart, '-'); //add -
		UartSendNumber(uart, ssid);
	}

	uint16_t nextPathEl = 14; //next path element index

    if(!(from[13] & 1)) //no c-bit in source address, there is a digi path
    {
        do //analyze all path elements
        {
            UartSendByte(uart, ','); //path separator

        	for(uint8_t i = 0; i < 6; i++) //copy element
        	{
        		if((from[nextPathEl + i] >> 1) != ' ') //skip spaces
        		{
        			UartSendByte(uart, from[nextPathEl + i] >> 1);
        		}
        	}

            ssid = ((from[nextPathEl + 6] >> 1) & 0b00001111); //store ssid
        	if(ssid > 0) //SSID >0
        	{
        		UartSendByte(uart, '-'); //add -
        		UartSendNumber(uart, ssid);
        	}
            if((from[nextPathEl + 6] & 0x80)) //h-bit in ssid
            	UartSendByte(uart, '*'); //add *

            nextPathEl += 7; //next path element
            if(nextPathEl > 56) //too many path elements
            	break;
        }
        while((from[nextPathEl - 1] & 1) == 0); //loop until the c-bit is found

    }

	UartSendByte(uart, ':'); //separator

    if((from[nextPathEl] & 0b11101111) == 0b00000011) //check if UI packet
    {
		nextPathEl += 2; //skip Control and PID

		UartSendString(uart, &(from[nextPathEl]), len - nextPathEl); //send information field
    }
    else
    	UartSendString(uart, "<not UI packet>", 0);

    UartSendByte(uart, 0); //terminate with NULL
}

void SendTNC2(uint8_t *from, uint16_t len)
{
	if(UartUsb.mode == MODE_MONITOR)
		sendTNC2ToUart(&UartUsb, from, len);
	if(Uart1.mode == MODE_MONITOR)
		sendTNC2ToUart(&Uart1, from, len);
	if(Uart2.mode == MODE_MONITOR)
		sendTNC2ToUart(&Uart2, from, len);
}

uint32_t Crc32(uint32_t crc0, uint8_t *s, uint64_t n)
{
	uint32_t crc = ~crc0;

	for(uint64_t i = 0; i < n; i++)
    {
		uint8_t ch = s[i];
		for(uint8_t j = 0; j < 8; j++) {
			uint32_t b = (ch ^ crc) & 1;
			crc >>= 1;
			if(b) crc ^= 0xEDB88320;
			ch >>= 1;
		}
	}

	return ~crc;
}

bool ParseCallsign(const char *in, uint16_t size, uint8_t *out)
{
	if(size > 6)
		return false;

	uint8_t tmp[6];

	uint8_t i = 0;
	for(; i < size; i++)
	{
		if(!IS_UPPERCASE_ALPHANUMERIC(in[i]))
			return false;

		tmp[i] = in[i] << 1;
	}

	for(uint8_t k = 0; k < i; k++)
		out[k] = tmp[k];

	for(; i < 6; i++)
		out[i] = ' ' << 1;


	return true;
}

bool ParseCallsignWithSsid(const char *in, uint16_t size, uint8_t *out, uint8_t *ssid)
{
	uint16_t ssidPosition = size;
	for(uint16_t i = 0; i < size; i++)
	{
		if(in[i] == '-')
		{
			ssidPosition = i;
			break;
		}
	}
	ssidPosition++;
	if(!ParseCallsign(in, ssidPosition - 1, out))
		return false;

	if(ssidPosition == size)
	{
		*ssid = 0;
		return true;
	}

	if(!ParseSsid(&in[ssidPosition], size - ssidPosition, ssid))
		return false;
	return true;
}

bool ParseSsid(const char *in, uint16_t size, uint8_t *out)
{
	int64_t ssid = StrToInt(in, size);
	if((ssid >= 0) && (ssid <= 15))
	{
		*out = (uint8_t)ssid;
		return true;
	}
	return false;
}


