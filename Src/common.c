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

#include "common.h"
#include "ax25.h"
#include <math.h>
#include <stdlib.h>
#include "drivers/uart.h"
#include "usbd_cdc_if.h"

uint8_t call[6] = {'N' << 1, '0' << 1, 'C' << 1, 'A' << 1, 'L' << 1, 'L' << 1};
uint8_t callSsid = 0;

uint8_t dest[7] = {130, 160, 156, 172, 96, 98, 96}; //destination address: APNV01-0 by default. SSID MUST remain 0.

const uint8_t *versionString = (const uint8_t*)"VP-Digi v. 1.2.6\r\nThe open-source standalone APRS digipeater controller and KISS TNC\r\n";

uint8_t autoReset = 0;
uint32_t autoResetTimer = 0;
uint8_t kissMonitor = 0;


int64_t strToInt(uint8_t *str, uint8_t len)
{
	if(len == 0)
	{
		while(1)
		{
			if(((str[len] > 47) && (str[len] < 58)))
				len++;
			else
				break;
			if(len >= 100)
				return 0;
		}
	}
	int64_t tmp = 0;
	for(int16_t i = (len - 1); i >= 0; i--)
	{
		if((i == 0) && (str[i] == '-'))
			tmp = -tmp;
		else
			tmp += ((str[i] - 48) * pow(10, len - 1 - i));
	}
	return tmp;
}


int16_t rando(int16_t min, int16_t max)
{
    int16_t tmp;
    if (max>=min)
        max-= min;
    else
    {
        tmp= min - max;
        min= max;
        max= tmp;
    }
    return max ? (rand() % max + min) : min;
}



void common_toTNC2(uint8_t *from, uint16_t len, uint8_t *to)
{

	for(uint8_t i = 0; i < 6; i++) //source call
	{
		if((*(from + 7 + i) >> 1) != ' ') //skip spaces
		{
			*(to++) = *(from + 7 + i) >> 1;
		}
	}

	uint8_t ssid = ((*(from + 13) >> 1) & 0b00001111); //store ssid
	if(ssid > 0) //SSID >0
	{
		*(to++) = '-'; //add -
		if(ssid > 9)
			*(to++) = '1'; //ssid >9, so will be -1x
		*(to++) = (ssid % 10) + 48;
	}

	*(to++) = '>'; //first separator


	for(uint8_t i = 0; i < 6; i++) //destination call
	{
		if((*(from + i) >> 1) != ' ') //skip spaces
		{
			*(to++) = *(from + i) >> 1;
		}
	}
	ssid = ((*(from + 6) >> 1) & 0b00001111); //store ssid

	if(ssid > 0) //SSID >0
	{
		*(to++) = '-'; //add -
		if(ssid > 9)
			*(to++) = '1'; //ssid >9, so will be -1x
		*(to++) = (ssid % 10) + 48;
	}

	uint16_t nextPathEl = 14; //next path element index

    if(!(*(from + 13) & 1)) //no c-bit in source address, there is a digi path
    {

        do //analize all path elements
        {
            *(to++) = ','; //path separator

        	for(uint8_t i = 0; i < 6; i++) //copy element
        	{
        		if((*(from + nextPathEl + i) >> 1) != ' ') //skip spaces
        		{
        			*(to++) = *(from + nextPathEl + i) >> 1;
        		}
        	}

            ssid = ((*(from + nextPathEl + 6) >> 1) & 0b00001111); //store ssid
        	if(ssid > 0) //SSID >0
        	{
        		*(to++) = '-'; //add -
        		if(ssid > 9)
        			*(to++) = '1'; //ssid >9, so will be -1x
        		*(to++) = (ssid % 10) + 48;
        	}
            if((*(from + nextPathEl + 6) & 128)) //h-bit in ssid
            	*(to++) = '*'; //add *

            nextPathEl += 7; //next path element
            if(nextPathEl > 56) //too many path elements
            	break;
        }
        while((*(from + nextPathEl - 1) & 1) == 0); //loop until the c-bit is found

    }


    *(to++) = ':'; //separator

    nextPathEl += 2; //skip Control and PID

    for(; nextPathEl < len; nextPathEl++) //copy information field
    {
        *(to++) = *(from + nextPathEl);
    }


    *(to++) = 0; //terminate with NULL

}

uint32_t crc32(uint32_t crc0, uint8_t *s, uint64_t n)
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

void SendKiss(uint8_t *buf, uint16_t len)
{
	Uart *u = &uart1;

	for(uint8_t i = 0; i < 2; i++)
	{
		if(u->mode == MODE_KISS) //check if KISS mode
		{
			uart_sendByte(u, 0xc0); //send data in kiss format
			uart_sendByte(u, 0x00);
			for(uint16_t j = 0; j < len; j++)
			{
				uart_sendByte(u, buf[j]);
			}
			uart_sendByte(u, 0xc0);
			uart_transmitStart(u);
		}
		u = &uart2;
	}

	if(USBmode == MODE_KISS) //check if USB in KISS mode
	{
		uint8_t t[2] = {0xc0, 0};

		CDC_Transmit_FS(&t[0], 1);
		CDC_Transmit_FS(&t[1], 1);

		for(uint16_t i = 0; i < len; i++)
		{
			CDC_Transmit_FS(&buf[i], 1);

		}
		CDC_Transmit_FS(&t[0], 1);
	}
}
