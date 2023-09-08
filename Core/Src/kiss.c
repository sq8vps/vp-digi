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

#include "kiss.h"
#include "ax25.h"
#include "digipeater.h"

void KissSend(Uart *port, uint8_t *buf, uint16_t size)
{
	if(port->mode == MODE_KISS)
	{
		UartSendByte(port, 0xC0);
		UartSendByte(port, 0x00);
		for(uint16_t i = 0; i < size; i++)
		{
			if(buf[i] == 0xC0) //frame end in data
			{
				UartSendByte(port, 0xDB); //frame escape
				UartSendByte(port, 0xDC); //transposed frame end
			}
			else if(buf[i] == 0xDB) //frame escape in data
			{
				UartSendByte(port, 0xDB); //frame escape
				UartSendByte(port, 0xDD); //transposed frame escape
			}
			else
				UartSendByte(port, buf[i]);
		}
		UartSendByte(port, 0xC0);
	}
}


void KissParse(Uart *port, uint8_t data)
{
	if(data == 0xC0) //frame end marker
	{
		if(port->kissBufferHead < 16) //command+source+destination+Control=16
		{
			port->kissBufferHead = 0;
			return;
		}

		if((port->kissBuffer[0] & 0xF) != 0) //check if this is an actual frame
		{
			port->kissBufferHead = 0;
			return;
		}

		//simple sanity check
		//check if LSbits in the first 13 bytes are set to 0
		//they should always be in an AX.25 frame
		for(uint8_t i = 0; i < 13; i++)
		{
			if((port->kissBuffer[i + 1] & 1) != 0)
			{
				port->kissBufferHead = 0;
				return;
			}
		}

		Ax25WriteTxFrame((uint8_t*)&port->kissBuffer[1], port->kissBufferHead - 1);
		DigiStoreDeDupe((uint8_t*)&port->kissBuffer[1], port->kissBufferHead - 1);
		port->kissBufferHead = 0;
		return;
	}
	else if(port->kissBufferHead > 0)
	{
		if((data == 0xDC) && (port->kissBuffer[port->kissBufferHead - 1] == 0xDB)) //escape character with transposed frame end
		{
			port->kissBuffer[port->kissBufferHead - 1] = 0xC0;
			return;
		}
		else if((data == 0xDD) && (port->kissBuffer[port->kissBufferHead - 1] == 0xDB)) //escape character with transposed escape character
		{
			port->kissBuffer[port->kissBufferHead - 1] = 0xDB;
			return;
		}
	}
	port->kissBuffer[port->kissBufferHead++] = data;
	port->kissBufferHead %= sizeof(port->kissBuffer);
}
