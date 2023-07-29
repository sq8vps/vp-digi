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

#include "terminal.h"
#include "common.h"
#include "beacon.h"
#include "digipeater.h"
#include "config.h"
#include "drivers/modem.h"
#include "ax25.h"

uint8_t termBuf[TERMBUFLEN]; //terminal mode TX buffer
uint16_t termBufIdx = 0; //terminal mode TX buffer index


uint16_t spLastIdx[3] = {0, 0, 0}; //index buffer was "special" terminal cases

/**
 * @brief Handle "special" terminal cases like backspace or local echo
 * @param[in] src Source: TERM_USB, TERM_UART1, TERM_UART2
 * @attention Must be called for every received data
 */
void term_handleSpecial(Terminal_stream src)
{
	if(src == TERM_USB)
	{
		if(USBmode == MODE_KISS) //don't do anything in KISS mode
		{
			spLastIdx[0] = 0;
			return;
		}
		if(spLastIdx[0] >= usbcdcidx) //USB RX buffer index was probably cleared
			spLastIdx[0] = 0;

		if(usbcdcdata[usbcdcidx - 1] == '\b') //user entered backspace
		{
			if(usbcdcidx > 1) //there was some data in buffer
			{
				usbcdcidx -= 2; //remove backspace and preceding character
				CDC_Transmit_FS((uint8_t*)"\b \b", 3); //backspace (one character left), remove backspaced character (send space) and backspace again
				if(spLastIdx[0] > 0)
					spLastIdx[0]--; //1 character was removed
			}
			else //there was only a backspace
				usbcdcidx = 0;
		}
		uint16_t t = usbcdcidx; //store last index
		if(spLastIdx[0] < t) //local echo handling
		{
			CDC_Transmit_FS(&usbcdcdata[spLastIdx[0]], t - spLastIdx[0]); //echo characters entered by user
			if((usbcdcdata[t - 1] == '\r') || (usbcdcdata[t - 1] == '\n'))
				CDC_Transmit_FS((uint8_t*)"\r\n", 2);
			spLastIdx[0] = t;
		}
	}
	else if((src == TERM_UART1) || (src == TERM_UART2))
	{
		Uart *u = &uart1;
		uint8_t nr = 1;
		if(src == TERM_UART2)
		{
			u = &uart2;
			nr = 2;
		}


		if(u->mode == MODE_KISS) //don't do anything in KISS mode
		{
			spLastIdx[nr] = 0;
			return;
		}
		if(spLastIdx[nr] >= u->bufrxidx) //UART RX buffer index was probably cleared
			spLastIdx[nr] = 0;

		if(u->bufrx[u->bufrxidx - 1] == '\b') //user entered backspace
		{
			if(u->bufrxidx > 1) //there was some data in buffer
			{
				u->bufrxidx -= 2; //remove backspace and preceding character
				uart_sendString(u, (uint8_t*)"\b \b", 3); //backspace (one character left), remove backspaced character (send space) and backspace again
				if(spLastIdx[nr] > 0)
					spLastIdx[nr]--; //1 character was removed
			}
			else //there was only a backspace
				u->bufrxidx = 0;
		}
		uint16_t t = u->bufrxidx; //store last index
		if(spLastIdx[nr] < t) //local echo handling
		{
			uart_sendString(u, &u->bufrx[spLastIdx[nr]], t - spLastIdx[nr]); //echo characters entered by user
			if((u->bufrx[t - 1] == '\r') || (u->bufrx[t - 1] == '\n'))
				uart_sendString(u, (uint8_t*)"\r\n", 2);
			spLastIdx[nr] = t;
		}
		uart_transmitStart(u);
	}

}

/**
 * \brief Send data to all available monitor outputs
 * \param[in] *data Data to send
 * \param[in] len Data length or 0 for NULL-terminated data
 */
void term_sendMonitor(uint8_t *data, uint16_t len)
{
	if(USBmode == MODE_MONITOR)
	{
		uartUSB_sendString(data, len);
	}
	if((uart1.enabled) && (uart1.mode == MODE_MONITOR))
	{
		uart_sendString(&uart1, data, len);
		uart_transmitStart(&uart1);
	}
	if((uart2.enabled) && (uart2.mode == MODE_MONITOR))
	{
		uart_sendString(&uart2, data, len);
		uart_transmitStart(&uart2);
	}
}

/**
 * \brief Send number to all available monitor outputs
 * \param[in] data Number to send
 */
void term_sendMonitorNumber(int32_t data)
{
	if(USBmode == MODE_MONITOR)
	{
		uartUSB_sendNumber(data);
	}
	if((uart1.enabled) && (uart1.mode == MODE_MONITOR))
	{
		uart_sendNumber(&uart1, data);
		uart_transmitStart(&uart1);
	}
	if((uart2.enabled) && (uart2.mode == MODE_MONITOR))
	{
		uart_sendNumber(&uart2, data);
		uart_transmitStart(&uart2);
	}
}

/**
* \brief Send terminal buffer using specified stream
* \param[in] way Stream: TERM_ANY, TERM_USB, TERM_UART1, TERM_UART2
*/
void term_sendBuf(Terminal_stream way)
{
	if((way == TERM_USB) || (way == TERM_ANY))
	{
		uartUSB_sendString(termBuf, termBufIdx);
	}
	if((way == TERM_UART1) || (way == TERM_ANY))
	{
		for(uint16_t d = 0; d < termBufIdx; d++)
		{
			uart_sendByte(&uart1, termBuf[d]);
		}
		uart_transmitStart(&uart1);
	}
	if((way == TERM_UART2)  || (way == TERM_ANY))
	{
		for(uint16_t d = 0; d < termBufIdx; d++)
		{
			uart_sendByte(&uart2, termBuf[d]);
		}
		uart_transmitStart(&uart2);
	}
	termBufIdx = 0;
}

/**
 * \brief Push byte to terminal buffer
 * \param[in] data Byte to store
 */
void term_sendByte(uint8_t data)
{
	if(termBufIdx > TERMBUFLEN - 1)
		return;
	termBuf[termBufIdx++] = data;
}

/**
 * \brief Push string to terminal buffer
 * \param[in] *data String
 * \param[in] len String length or 0 for NULL-terminated string
 */
void term_sendString(uint8_t *data, uint16_t len)
{
	if(len != 0)
	{
		for(uint16_t y= 0; y < len; y++)
		{
			if(termBufIdx > TERMBUFLEN - 1) break;
			termBuf[termBufIdx++] = *data;
			data++;
		}
	} else
	{
		while(*data != 0)
		{
			if(termBufIdx > TERMBUFLEN - 1) break;
			termBuf[termBufIdx++] = *data;
			data++;
			if(termBufIdx > TERMBUFLEN) break;
		}
	}
	termBuf[termBufIdx] = 0;
}

/**
 * \brief Push number (in ASCII form) in terminal buffer
 * \param[in] n Number
 */
void term_sendNumber(int32_t n)
{
	if(n < 0)
	n = abs(n);
	if(n > 999999) term_sendByte((n / 1000000) + 48);
	if(n > 99999) term_sendByte(((n % 1000000) / 100000) + 48);
	if(n > 9999) term_sendByte(((n % 100000) / 10000) + 48);
	if(n > 999) term_sendByte(((n % 10000) / 1000) + 48);
	if(n > 99) term_sendByte(((n % 1000) / 100) + 48);
	if(n > 9) term_sendByte(((n % 100) / 10) + 48);
	term_sendByte((n % 10) + 48);
}

/**
 * \brief Chceck if received data and command are matching
 * \param[in] *data Received data
 * \param[in] dlen Data length
 * \param[in] *cmd Command
 * \return 1 if matching, 0 if not
 */
static uint8_t checkcmd(uint8_t *data, uint8_t dlen, uint8_t *cmd)
{
	for(uint8_t a = 0; a < dlen; a++)
	{
		if(*(data + a) != *(cmd + a))
			return 0;
	}
	return 1;
}

/**
 * \brief Parse and process received data
 * \param[in] *cmd Data
 * \param[in] len Data length
 * \param[in] src Source: TERM_USB, TERM_UART1, TERM_UART2
 * \param[in] type Data type: DATA_KISS, DATA_TERM
 * \param[in] mode Input mode: MODE_KISS, MODE_TERM, MODE_MONITOR
 */
void term_parse(uint8_t *cmd, uint16_t len, Terminal_stream src, Uart_data_type type, Uart_mode mode)
{
	if(src == TERM_ANY) //incorrect source
		return;

	if(checkcmd(cmd, 4, (uint8_t*)"kiss"))
	{
		if(src == TERM_USB)
		{
			term_sendString((uint8_t*)"USB switched to KISS mode\r\n", 0);
			term_sendBuf(TERM_USB);
			USBmode = MODE_KISS;
		}
		else if(src == TERM_UART1)
		{
			term_sendString((uint8_t*)"UART1 switched to KISS mode\r\n", 0);
			term_sendBuf(TERM_UART1);
			uart1.mode = MODE_KISS;
		}
		else if(src == TERM_UART2)
		{
			term_sendString((uint8_t*)"UART2 switched to KISS mode\r\n", 0);
			term_sendBuf(TERM_UART2);
			uart2.mode = MODE_KISS;
		}
		return;
	}

	if(checkcmd(cmd, 6, (uint8_t*)"config"))
	{
		term_sendString((uint8_t*)"Switched to configuration mode\r\n"
				"Most settings will take effect immidiately, but\r\n"
				"remember to save the configuration using \"save\"\r\n", 0);
		if(src == TERM_USB)
		{
			term_sendBuf(TERM_USB);
			USBmode = MODE_TERM;
		}
		else if(src == TERM_UART1)
		{
			term_sendBuf(TERM_UART1);
			uart1.mode = MODE_TERM;
		}
		else if(src == TERM_UART2)
		{
			term_sendBuf(TERM_UART2);
			uart2.mode = MODE_TERM;
		}
		return;
	}

	if(checkcmd(cmd, 7, (uint8_t*)"monitor"))
	{
		if(src == TERM_USB)
		{
			term_sendString((uint8_t*)"USB switched to monitor mode\r\n", 0);
			term_sendBuf(TERM_USB);
			USBmode = MODE_MONITOR;
		}
		else if(src == TERM_UART1)
		{
			term_sendString((uint8_t*)"UART1 switched to monitor mode\r\n", 0);
			term_sendBuf(TERM_UART1);
			uart1.mode = MODE_MONITOR;
		}
		else if(src == TERM_UART2)
		{
			term_sendString((uint8_t*)"UART2 switched to monitor mode\r\n", 0);
			term_sendBuf(TERM_UART2);
			uart2.mode = MODE_MONITOR;
		}
		return;
	}

	if((mode == MODE_KISS) && (type == DATA_KISS))
	{
		Uart_txKiss(cmd, len); //if received KISS data, transmit KISS frame
		return;
	}

	if((mode == MODE_MONITOR) && (type == DATA_TERM)) //monitor mode
	{
		if(checkcmd(cmd, 4, (uint8_t*)"help"))
		{
			term_sendString((uint8_t*)"\r\nCommans available in monitor mode:\r\n", 0);
			term_sendString((uint8_t*)"help - shows this help page\r\n", 0);
			term_sendString((uint8_t*)"cal {low|high|alt|stop} - transmits/stops transmitter calibration pattern\r\n", 0);
			term_sendBuf(src);
			term_sendString((uint8_t*)"\tlow - transmits MARK tone, high - transmits SPACE tone, alt - transmits alternating tones (null bytes)\r\n", 0);
			term_sendString((uint8_t*)"beacon <beacon_number> - immediately transmits selected beacon (number from 0 to 7)\r\n", 0);
			term_sendBuf(src);
			term_sendString((uint8_t*)"kiss - switches to KISS mode\r\n", 0);
			term_sendString((uint8_t*)"config - switches to config mode\r\n", 0);
			term_sendString((uint8_t*)"reboot - reboots the device\r\n", 0);
			term_sendString((uint8_t*)"version - shows full firmware version info\r\n\r\n\r\n", 0);
			term_sendBuf(src);
			return;
		}
		if(checkcmd(cmd, 7, (uint8_t*)"version"))
		{
			term_sendString((uint8_t*)versionString, 0);
			term_sendBuf(src);
			return;
		}
		if(checkcmd(cmd, 6, (uint8_t*)"reboot"))
		{
			NVIC_SystemReset();
			return;
		}
		if(checkcmd(cmd, 7, (uint8_t*)"beacon "))
		{
			if((cmd[7] > 47) && (cmd[7] < 56))
			{
				uint8_t bcno = cmd[7] - 48;
				if((beacon[bcno].interval != 0) && (beacon[bcno].enable != 0))
				{
					Beacon_send(bcno);
					return;
				}
				else
				{
					term_sendString((uint8_t*)"Beacon " , 0);
					term_sendNumber(bcno);
					term_sendString((uint8_t*)" not enabled. Cannot transmit disabled beacons.\r\n", 0);
					term_sendBuf(src);
				}
				return;
			}
			else
			{
				term_sendString((uint8_t*)"Beacon number must be within 0-7 range\r\n", 0);
				term_sendBuf(src);
				return;
			}
		}

		if(checkcmd(cmd, 3, (uint8_t*)"cal"))
		{
				if(checkcmd(&cmd[4], 3, (uint8_t*)"low"))
				{
					term_sendString((uint8_t*)"Starting low tone calibration transmission\r\n", 0);
					term_sendBuf(src);
					Afsk_txTestStart(TEST_MARK);
					return;
				}
				else if(checkcmd(&cmd[4], 4, (uint8_t*)"high"))
				{
					term_sendString((uint8_t*)"Starting high tone calibration transmission\r\n", 0);
					term_sendBuf(src);
					Afsk_txTestStart(TEST_SPACE);
					return;
				}
				else if(checkcmd(&cmd[4], 3, (uint8_t*)"alt"))
				{
					term_sendString((uint8_t*)"Starting alternating tones calibration pattern transmission\r\n", 0);
					term_sendBuf(src);
					Afsk_txTestStart(TEST_ALTERNATING);
					return;
				}
				else if(checkcmd(&cmd[4], 4, (uint8_t*)"stop"))
				{
					term_sendString((uint8_t*)"Stopping calibration transmission\r\n", 0);
					term_sendBuf(src);
					Afsk_txTestStop();
					return;
				}
			term_sendString((uint8_t*)"Usage: cal {low|high|alt|stop}\r\n", 0);
			term_sendBuf(src);
			return;
		}

		term_sendString((uint8_t*)"Unknown command. For command list type \"help\"\r\n", 0);
		term_sendBuf(src);
		return;
	}

	if((mode != MODE_TERM)) return;


	if(checkcmd(cmd, 4, (uint8_t*)"help"))
	{
		term_sendString((uint8_t*)"\r\nCommands available in config mode:\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"print - prints all configuration parameters\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"list - prints callsign filter list\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"save - saves configuration and reboots the device\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"eraseall - erases all configurations and reboots the device\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"help - shows this help page\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"reboot - reboots the device\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"version - shows full firmware version info\r\n\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"call <callsign> - sets callsign\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"ssid <0-15> - sets SSID\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"dest <address> - sets destination address\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"txdelay <50-2550> - sets TXDelay time (ms)\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"txtail <10-2550> - sets TXTail time (ms)\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"quiet <100-2550> - sets quiet time (ms)\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"rs1baud/rs2baud <1200-115200> - sets UART1/UART2 baudrate\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"pwm [on/off] - enables/disables PWM. If PWM is off, R2R will be used instead\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"flat [on/off] - set to \"on\" if flat audio input is used\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"beacon <0-7> [on/off] - enables/disables specified beacon\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"beacon <0-7> [iv/dl] <0-720> - sets interval/delay for the specified beacon (min)\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"beacon <0-7> path <el1,[el2]>/none - sets path for the specified beacon\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"beacon <0-7> data <data> - sets information field for the specified beacon\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi [on/off] - enables/disables whole digipeater\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi <0-7> [on/off] - enables/disables specified slot\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi <0-7> alias <alias> - sets alias for the specified slot\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi <0-3> [max/rep] <0/1-7> - sets maximum/replacement N for the specified slot\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi <0-7> trac [on/off] - enables/disables packet tracing for the specified slot\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi <0-7> viscous [on/off] - enables/disables viscous-delay digipeating for the specified slot\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi <0-7> direct [on/off] - enables/disables direct-only digipeating for the specified slot\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi <0-7> filter [on/off] - enables/disables packet filtering for the specified slot\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi filter [black/white] - sets filtering type to blacklist/whitelist\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi dupe <5-255> - sets anti-dupe buffer time (s)\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"digi list <0-19> [set <call>/remove] - sets/clears specified callsign slot in filter list\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"autoreset <0-255> - sets auto-reset interval (h) - 0 to disable\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"monkiss [on/off] - send own and digipeated frames to KISS ports\r\n", 0);
		term_sendBuf(src);
		term_sendString((uint8_t*)"nonaprs [on/off] - enable reception of non-APRS frames\r\n", 0);
		term_sendBuf(src);
		return;
	}
	if(checkcmd(cmd, 7, (uint8_t*)"version"))
	{
		term_sendString((uint8_t*)versionString, 0);
		term_sendBuf(src);
		return;
	}
	if(checkcmd(cmd, 6, (uint8_t*)"reboot"))
	{
		NVIC_SystemReset();
		return;
	}
	if(checkcmd(cmd, 4, (uint8_t*)"save"))
	{
		Config_write();
		NVIC_SystemReset();
		return;
	}
	if(checkcmd(cmd, 8, (uint8_t*)"eraseall"))
	{
		Config_erase();
		NVIC_SystemReset();
		return;
	}

	if(checkcmd(cmd, 5, (uint8_t*)"print"))
	{
		term_sendString((uint8_t*)"Callsign: ", 0);
		for(uint8_t i = 0; i < 6; i++)
		{
			if((call[i]) != 64)
				term_sendByte(call[i] >> 1);
		}
		term_sendByte('-');
		term_sendNumber(callSsid);

		term_sendString((uint8_t*)"\r\nDestination: ", 0);
		for(uint8_t i = 0; i < 6; i++)
		{
			if((dest[i]) != 64)
				term_sendByte(dest[i] >> 1);
		}

		term_sendString((uint8_t*)"\r\nTXDelay (ms): ", 0);
		term_sendNumber(ax25Cfg.txDelayLength);
		term_sendString((uint8_t*)"\r\nTXTail (ms): ", 0);
		term_sendNumber(ax25Cfg.txTailLength);
		term_sendString((uint8_t*)"\r\nQuiet time (ms): ", 0);
		term_sendNumber(ax25Cfg.quietTime);
		term_sendString((uint8_t*)"\r\nUART1 baudrate: ", 0);
		term_sendNumber(uart1.baudrate);
		term_sendString((uint8_t*)"\r\nUART2 baudrate: ", 0);
		term_sendNumber(uart2.baudrate);
		term_sendString((uint8_t*)"\r\nDAC type: ", 0);
		if(afskCfg.usePWM)
			term_sendString((uint8_t*)"PWM", 0);
		else
			term_sendString((uint8_t*)"R2R", 0);
		term_sendString((uint8_t*)"\r\nFlat audio input: ", 0);
		if(afskCfg.flatAudioIn)
			term_sendString((uint8_t*)"yes", 0);
		else
			term_sendString((uint8_t*)"no", 0);
		term_sendBuf(src);
		for(uint8_t i = 0; i < 8; i++)
		{
			term_sendString((uint8_t*)"\r\nBeacon ", 0);
			term_sendByte(i + 48);
			term_sendString((uint8_t*)": ", 0);
			if(beacon[i].enable)
				term_sendString((uint8_t*)"On, Iv: ", 0);
			else
				term_sendString((uint8_t*)"Off, Iv: ", 0);
			term_sendNumber(beacon[i].interval / 6000);
			term_sendString((uint8_t*)", Dl: ", 0);
			term_sendNumber(beacon[i].delay / 60);
			term_sendByte(',');
			term_sendByte(' ');
			if(beacon[i].path[0] != 0)
			{
				for(uint8_t j = 0; j < 6; j++)
				{
					if(beacon[i].path[j] != 32) term_sendByte(beacon[i].path[j]);
				}
				term_sendByte('-');
				term_sendNumber(beacon[i].path[6]);
				if(beacon[i].path[7] != 0)
				{
					term_sendByte(',');
					for(uint8_t j = 7; j < 13; j++)
					{
						if(beacon[i].path[j] != 32) term_sendByte(beacon[i].path[j]);
					}
					term_sendByte('-');
					term_sendNumber(beacon[i].path[13]);
				}
			}
			else term_sendString((uint8_t*)"no path", 0);
			term_sendBuf(src);
			term_sendByte(',');
			term_sendByte(' ');
			term_sendString(beacon[i].data, 0);
			term_sendBuf(src);
		}

		term_sendString((uint8_t*)"\r\nDigipeater: ", 0);
		if(digi.enable)
			term_sendString((uint8_t*)"On\r\n", 0);
		else
			term_sendString((uint8_t*)"Off\r\n", 0);
		term_sendBuf(src);

		for(uint8_t i = 0; i < 4; i++) //n-N aliases
		{
			term_sendString((uint8_t*)"Alias ", 0);
			if(digi.alias[i][0] != 0)
			{
				for(uint8_t j = 0; j < 5; j++)
				{
					if(digi.alias[i][j] != 0)
						term_sendByte(digi.alias[i][j] >> 1);
				}
			}
			term_sendString((uint8_t*)": ", 0);
			if(digi.enableAlias & (1 << i))
				term_sendString((uint8_t*)"On, max: ", 0);
			else
				term_sendString((uint8_t*)"Off, max: ", 0);
			term_sendNumber(digi.max[i]);
			term_sendString((uint8_t*)", rep: ", 0);
			term_sendNumber(digi.rep[i]);
			if(digi.traced & (1 << i))
				term_sendString((uint8_t*)", traced, ", 0);
			else
				term_sendString((uint8_t*)", untraced, ", 0);
			if(digi.viscous & (1 << i))
				term_sendString((uint8_t*)"viscous-delay, ", 0);
			else if(digi.directOnly & (1 << i))
				term_sendString((uint8_t*)"direct-only, ", 0);
			if(digi.callFilterEnable & (1 << i))
				term_sendString((uint8_t*)"filtered\r\n", 0);
			else
				term_sendString((uint8_t*)"unfiltered\r\n", 0);
		}
		term_sendBuf(src);
		for(uint8_t i = 0; i < 4; i++) //simple aliases
		{
			term_sendString((uint8_t*)"Alias ", 0);
			if(digi.alias[i + 4][0] != 0)
			{
				for(uint8_t j = 0; j < 6; j++)
				{
					if(digi.alias[i + 4][j] != 64)
						term_sendByte(digi.alias[i + 4][j] >> 1);
				}
			}
			term_sendByte('-');
			term_sendNumber(digi.ssid[i]);
			term_sendString((uint8_t*)": ", 0);
			if(digi.enableAlias & (1 << (i + 4)))
				term_sendString((uint8_t*)"On, ", 0);
			else
				term_sendString((uint8_t*)"Off, ", 0);
			if(digi.traced & (1 << (i + 4)))
				term_sendString((uint8_t*)"traced, ", 0);
			else
				term_sendString((uint8_t*)"untraced, ", 0);
			if(digi.viscous & (1 << (i + 4)))
				term_sendString((uint8_t*)"viscous-delay, ", 0);
			else if(digi.directOnly & (1 << (i + 4)))
				term_sendString((uint8_t*)"direct-only, ", 0);
			if(digi.callFilterEnable & (1 << (i + 4)))
				term_sendString((uint8_t*)"filtered\r\n", 0);
			else
				term_sendString((uint8_t*)"unfiltered\r\n", 0);
		}
		term_sendBuf(src);
		term_sendString((uint8_t*)"Anti-duplicate buffer hold time (s): ", 0);
		term_sendNumber(digi.dupeTime);
		term_sendString((uint8_t*)"\r\nCallsign filter type: ", 0);
		if(digi.filterPolarity)
			term_sendString((uint8_t*)"whitelist\r\n", 0);
		else
			term_sendString((uint8_t*)"blacklist\r\n", 0);
		term_sendString((uint8_t*)"Callsign filter list: ", 0);
		uint8_t callent = 0;
		for(uint8_t i = 0; i < 20; i++)
		{
			if(digi.callFilter[i][0] != 0)
				callent++;
		}
		if(callent > 9)
		{
			term_sendByte((callent / 10) + 48);
			term_sendByte((callent % 10) + 48);
		} else
			term_sendByte(callent + 48);
		term_sendString((uint8_t*)" entries\r\nAuto-reset every (h): ", 0);
		if(autoReset == 0)
			term_sendString((uint8_t*)"disabled", 0);
		else
			term_sendNumber(autoReset);
		term_sendString((uint8_t*)"\r\nKISS monitor: ", 0);
		if(kissMonitor == 1)
			term_sendString((uint8_t*)"On\r\n", 0);
		else
			term_sendString((uint8_t*)"Off\r\n", 0);
		term_sendString((uint8_t*)"Allow non-APRS frames: ", 0);
		if(ax25Cfg.allowNonAprs == 1)
			term_sendString((uint8_t*)"On\r\n", 0);
		else
			term_sendString((uint8_t*)"Off\r\n", 0);
		term_sendBuf(src);
		return;
	}


	if(checkcmd(cmd, 4, (uint8_t*)"list"))
	{
		term_sendString((uint8_t*)"Callsign filter list: \r\n", 0);
		for(uint8_t i = 0; i < 20; i++)
		{
			term_sendNumber(i);
			term_sendString((uint8_t*)". ", 0);
			if(digi.callFilter[i][0] != 0)
			{
                    uint8_t cl[10] = {0};
                    uint8_t clidx = 0;
                    for(uint8_t h = 0; h < 6; h++)
                    {
                        if(digi.callFilter[i][h] == 0xFF)
                        {
                            uint8_t g = h;
                            uint8_t j = 0;
                            while(g < 6)
                            {
                                if(digi.callFilter[i][g] != 0xFF) j = 1;
                                g++;
                            }
                            if(j == 0)
                            {
                                cl[clidx++] = '*';
                                break;
                            } else
                            {
                            	cl[clidx++] = '?';
                            }
                        }
                        else
                        {
                            if(digi.callFilter[i][h] != ' ')
                            	cl[clidx++] = digi.callFilter[i][h];
                        }
                    }
                    if(digi.callFilter[i][6] == 0xFF)
                    {
                    	cl[clidx++] = '-';
                    	cl[clidx++] = '*';
                    }
                    else if(digi.callFilter[i][6] != 0)
                    {
                    	cl[clidx++] = '-';
                        if(digi.callFilter[i][6] > 9)
                        {
                        	cl[clidx++] = (digi.callFilter[i][6] / 10) + 48;
                        	cl[clidx++] = (digi.callFilter[i][6] % 10) + 48;
                        }
                        else cl[clidx++] = digi.callFilter[i][6] + 48;
                    }
                    term_sendString(cl, 0);
			}
			else
				term_sendString((uint8_t*)"empty", 0);
			term_sendString((uint8_t*)"\r\n", 0);
			term_sendBuf(src);
		}
		return;
	}

	if(checkcmd(cmd, 4, (uint8_t*)"call"))
	{
		uint8_t tmp[6] = {0};
		uint8_t err = 0;
		for(uint8_t i = 0; i < 7; i++)
		{
			if((cmd[5 + i] == '\r') || (cmd[5 + i] == '\n'))
			{
				if((i == 0))
					err = 1;
				break;
			}
			if(!(((cmd[5 + i] > 47) && (cmd[5 + i] < 58)) || ((cmd[5 + i] > 64) && (cmd[5 + i] < 91)))) //only alphanumeric characters
			{
				err = 1;
				break;
			}
			if(i == 6) //call too long
			{
				err = 1;
				break;
			}
			tmp[i] = cmd[5 + i] << 1;
		}
		if(!err)
			for(uint8_t i = 0; i < 6; i++)
			{
				call[i] = 64; //fill with spaces
				if(tmp[i] != 0)
					call[i] = tmp[i];
			}
		if(err)
		{
			term_sendString((uint8_t*)"Incorrect callsign!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 4, (uint8_t*)"dest"))
	{
		uint8_t tmp[6] = {0};
		uint8_t err = 0;
		for(uint8_t i = 0; i < 7; i++)
		{
			if((cmd[5 + i] == '\r') || (cmd[5 + i] == '\n'))
			{
				if((i == 0))
					err = 1;
				break;
			}
			if(!(((cmd[5 + i] > 47) && (cmd[5 + i] < 58)) || ((cmd[5 + i] > 64) && (cmd[5 + i] < 91)))) //only alphanumeric characters
			{
				err = 1;
				break;
			}
			if(i == 6) //address too long
			{
				err = 1;
				break;
			}
			tmp[i] = cmd[5 + i] << 1;
		}
		if(!err)
			for(uint8_t i = 0; i < 6; i++)
			{
				dest[i] = 64; //fill with spaces
				if(tmp[i] != 0)
					dest[i] = tmp[i];
			}
		if(err)
		{
			term_sendString((uint8_t*)"Incorrect address!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 4, (uint8_t*)"ssid"))
	{
		uint8_t tmp[3] = {0};
		uint8_t err = 0;
		for(uint8_t i = 0; i < 3; i++)
		{
			if((cmd[5 + i] == '\r') || (cmd[5 + i] == '\n'))
			{
				if((i == 0))
					err = 1; //no ssid provided
				break;
			}
			if(!(((cmd[5 + i] > 47) && (cmd[5 + i] < 58)))) //only numbers
			{
				err = 1;
				break;
			}
			if(i == 2) //more than 2 digits in ssid
			{
				err = 1;
				break;
			}
			tmp[i] = cmd[5 + i];
		}
		if(!err)
		{
			uint8_t t = 0;
			t = (uint8_t)strToInt(tmp, 0);
			if(t > 15)
				err = 1;
			else
				callSsid = t;
		}
		if(err)
		{
			term_sendString((uint8_t*)"Incorrect SSID!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 7, (uint8_t*)"txdelay"))
	{
		uint8_t tmp[5] = {0};
		uint8_t err = 0;
		for(uint8_t i = 0; i < 5; i++)
		{
			if((cmd[8 + i] == '\r') || (cmd[8 + i] == '\n'))
			{
				if((i == 0))
					err = 1;
				break;
			}
			if(!(((cmd[8 + i] > 47) && (cmd[8 + i] < 58)))) //only digits
			{
				err = 1;
				break;
			}
			if(i == 4)
			{
				err = 1;
				break;
			}
			tmp[i] = cmd[8 + i];
		}
		if(!err)
		{
			uint16_t t = 0;
			t = (uint16_t)strToInt(tmp, 0);
			if((t > 2550) || (t < 50))
				err = 1;
			else
				ax25Cfg.txDelayLength = t;
		}
		if(err)
		{
			term_sendString((uint8_t*)"Incorrect TXDelay!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 6, (uint8_t*)"txtail"))
	{
		uint8_t tmp[5] = {0};
		uint8_t err = 0;
		for(uint8_t i = 0; i < 5; i++)
		{
			if((cmd[7 + i] == '\r') || (cmd[7 + i] == '\n'))
			{
				if((i == 0))
					err = 1;
				break;
			}
			if(!(((cmd[7 + i] > 47) && (cmd[7 + i] < 58)))) //only digits
			{
				err = 1;
				break;
			}
			if(i == 4)
			{
				err = 1;
				break;
			}
			tmp[i] = cmd[7 + i];
		}
		if(!err)
		{
			uint16_t t = 0;
			t = (uint16_t)strToInt(tmp, 0);
			if((t > 2550) || (t < 10))
				err = 1;
			else
				ax25Cfg.txTailLength = t;
		}
		if(err)
		{
			term_sendString((uint8_t*)"Incorrect TXTail!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 5, (uint8_t*)"quiet"))
	{
		uint8_t tmp[5] = {0};
		uint8_t err = 0;
		for(uint8_t i = 0; i < 5; i++)
		{
			if((cmd[6 + i] == '\r') || (cmd[6 + i] == '\n'))
			{
				if((i == 0))
					err = 1;
				break;
			}
			if(!(((cmd[6 + i] > 47) && (cmd[6 + i] < 58))))
			{
				err = 1;
				break;
			}
			if(i == 4)
			{
				err = 1;
				break;
			}
			tmp[i] = cmd[6 + i];
		}
		if(!err)
		{
			uint16_t t = 0;
			t = (uint16_t)strToInt(tmp, 0);
			if((t > 2550) || (t < 100))
				err = 1;
			else
				ax25Cfg.quietTime = t;
		}
		if(err)
		{
			term_sendString((uint8_t*)"Incorrect quiet time!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 7, (uint8_t*)"rs1baud") || checkcmd(cmd, 7, (uint8_t*)"rs2baud"))
	{
		uint8_t tmp[7] = {0};
		uint8_t err = 0;
		for(uint8_t i = 0; i < 7; i++)
		{
			if((cmd[8 + i] == '\r') || (cmd[8 + i] == '\n'))
			{
				if((i == 0))
					err = 1;
				break;
			}
			if(!(((cmd[8 + i] > 47) && (cmd[8 + i] < 58))))
			{
				err = 1;
				break;
			}
			if(i == 6)
			{
				err = 1;
				break;
			}
			tmp[i] = cmd[8 + i];
		}
		if(!err)
		{
			uint32_t t = 0;
			t = (uint32_t)strToInt(tmp, 0);
			if((t > 115200) || (t < 1200))
				err = 1;
			else
			{
				if(cmd[2] == '1')
					uart1.baudrate = t;
				else
					uart2.baudrate = t;
			}
		}
		if(err)
		{
			term_sendString((uint8_t*)"Incorrect baudrate!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 6, (uint8_t*)"beacon"))
	{
		uint8_t bcno = 0;
		bcno = cmd[7] - 48;
		if(bcno > 7)
		{
			term_sendString((uint8_t*)"Incorrect beacon number\r\n", 0);
			term_sendBuf(src);
		}
		uint8_t err = 0;
		if(cmd[9] == 'o' && cmd[10] == 'n')
			beacon[bcno].enable = 1;
		else if(cmd[9] == 'o' && cmd[10] == 'f' && cmd[11] == 'f')
			beacon[bcno].enable = 0;
		else if((cmd[9] == 'i' && cmd[10] == 'v') || (cmd[9] == 'd' && cmd[10] == 'l')) //interval or delay
		{
			uint8_t tmp[4] = {0};

			for(uint8_t i = 0; i < 4; i++)
			{
				if((cmd[12 + i] == '\r') || (cmd[12 + i] == '\n'))
				{
					if((i == 0))
						err = 1;
					break;
				}
				if(!(((cmd[12 + i] > 47) && (cmd[12 + i] < 58))))
				{
					err = 1;
					break;
				}
				if(i == 3)
				{
					err = 1;
					break;
				}
				tmp[i] = cmd[12 + i];
			}
			if(!err)
			{
				uint32_t t = 0;
				t = (uint32_t)strToInt(tmp, 0);
				if(t > 720) err = 1;
				else
				{
					if(cmd[9] == 'i')
						beacon[bcno].interval = t * 6000;
					else
						beacon[bcno].delay = t * 60;
				}
			}
		}
		else if((cmd[9] == 'd' && cmd[10] == 'a' && cmd[11] == 't' && cmd[12] == 'a'))
		{
			uint8_t dlen = 0;
			if((cmd[len - 1] == '\r') || (cmd[len - 1] == '\n'))
				dlen = len - 15;
			else
				dlen = len - 14;
			if(dlen > 99)
			{
				term_sendString((uint8_t*)"Data too long\r\n", 0);
				term_sendBuf(src);
				return;
			}
			uint8_t i = 0;
			for(; i < dlen; i++)
			{
				beacon[bcno].data[i] = cmd[14 + i];
			}
			beacon[bcno].data[i] = 0;
		}
		else if((cmd[9] == 'p' && cmd[10] == 'a' && cmd[11] == 't' && cmd[12] == 'h'))
		{
			uint8_t dlen = 0;
			if((cmd[len - 2] == '\r') || (cmd[len - 2] == '\n'))
				dlen = len - 16;
			else
				dlen = len - 15;

			if((dlen == 4) && (cmd[14] == 'n') && (cmd[15] == 'o') && (cmd[16] == 'n') && (cmd[17] == 'e')) //"none" path
			{
				for(uint8_t i = 0; i < 14; i++)
					beacon[bcno].path[i] = 0;

				term_sendString((uint8_t*)"OK\r\n", 0);
				term_sendBuf(src);
				return;
			}
			if(dlen == 0)
				err = 1;

			uint8_t tmp[14] = {0};
			uint8_t elemlen = 0;
			uint8_t tmpidx = 0;
			if(!err)
			for(uint8_t i = 0; i < dlen; i++)
			{
				if(elemlen > 6)
				{
					err = 1;
					break;
				}
				if(cmd[14 + i] == '-')
				{
					uint8_t ssid = 0;
					if(((cmd[15 + i] > 47) && (cmd[15 + i] < 58)))
					{
						ssid = cmd[15 + i] - 48;
					}
					else
					{
						err = 1;
						break;
					}
					if(((cmd[16 + i] > 47) && (cmd[16 + i] < 58)))
					{
						ssid *= 10;
						ssid += cmd[16 + i] - 48;
					}
					else if(!((cmd[16 + i] == ',')  || (cmd[16 + i] == '\r') || (cmd[16 + i] == '\n')))
					{
						err = 1;
						break;
					}
					if(ssid > 15)
					{
						err = 1;
						break;
					}
					else
					{
						while(tmpidx < ((tmpidx > 7) ? 13 : 6))
						{
							tmp[tmpidx++] = ' ';
						}
						tmp[tmpidx++] = ssid;
						if(ssid > 10)
							i += 2;
						else
							i++;
					}
				}
				else if(cmd[14 + i] == ',')
				{
					elemlen = 0;
				}
				else if(((cmd[14 + i] > 47) && (cmd[14 + i] < 58)) || ((cmd[14 + i] > 64) && (cmd[14 + i] < 91)))
				{
					elemlen++;
					tmp[tmpidx++] = cmd[14 + i];
				}
				else
				{
					err = 1;
					break;
				}
			}
			if(err)
			{
				term_sendString((uint8_t*)"Incorrect path!\r\n", 0);
				term_sendBuf(src);
				return;
			}

			for(uint8_t i = 0; i < 14; i++)
				beacon[bcno].path[i] = tmp[i];
			err = 0;
		}

		else err = 1;

		if(err == 1)
		{
			term_sendString((uint8_t*)"Incorrect command\r\n", 0);
		}
		else term_sendString((uint8_t*)"OK\r\n", 0);

		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 4, (uint8_t*)"digi"))
	{

		uint8_t err = 0;
		if(cmd[4] == ' ' && cmd[5] == 'o' && cmd[6] == 'n')
			digi.enable = 1;
		else if(cmd[4] == ' ' && cmd[5] == 'o' && cmd[6] == 'f' && cmd[7] == 'f')
			digi.enable = 0;
		else if((((cmd[5] > 47) && (cmd[5] < 58))) && (cmd[4] == ' '))
		{
			uint8_t alno = 0;
			alno = cmd[5] - 48;
			if(alno > 7)
			{
				term_sendString((uint8_t*)"Incorrect alias number\r\n", 0);
				term_sendBuf(src);
				return;
			}
			if(checkcmd(&cmd[7], 2, (uint8_t*)"on"))
				digi.enableAlias |= (1 << alno);
			else if(checkcmd(&cmd[7], 3, (uint8_t*)"off"))
				digi.enableAlias &= ~(1 << alno);
			else if(checkcmd(&cmd[7], 6, (uint8_t*)"alias "))
			{
				uint8_t tmp[5] = {0};
				uint8_t err = 0;

				if(alno < 4) //New-N aliases
				{
					for(uint8_t i = 0; i < 6; i++)
					{
						if((cmd[13 + i] == '\r') || (cmd[13 + i] == '\n'))
						{
							if((i == 0))
								err = 1;
							break;
						}
						if(!(((cmd[13 + i] > 47) && (cmd[13 + i] < 58)) || ((cmd[13 + i] > 64) && (cmd[13 + i] < 91))))
						{
							err = 1;
							break;
						}
						if(i == 5)
						{
							err = 1;
							break;
						}
						tmp[i] = cmd[13 + i] << 1;
					}
					if(!err) for(uint8_t i = 0; i < 5; i++)
					{
						digi.alias[alno][i] = tmp[i];
					}
					if(err)
					{
						term_sendString((uint8_t*)"Incorrect alias!\r\n", 0);
					}
					else
					{
						term_sendString((uint8_t*)"OK\r\n", 0);
					}
					term_sendBuf(src);
					return;
				}
				else  //simple aliases
				{
					uint8_t dlen = 0;
					if((cmd[len - 2] == '\r') || (cmd[len - 2] == '\n'))
						dlen = len - 16;
					else
						dlen = len - 14;

					if(dlen == 0)
						err = 1;

					uint8_t tmp[6] = {0};
					uint8_t tmpidx = 0;
					uint8_t elemlen = 0;
					uint8_t ssid = 0;
					if(!err)
					for(uint8_t i = 0; i < dlen; i++)
					{
						if(elemlen > 6)
						{
							err = 1;
							break;
						}
						if(cmd[13 + i] == '-')
						{
							if(((cmd[14 + i] > 47) && (cmd[14 + i] < 58)))
							{
								ssid = cmd[14 + i] - 48;
							}
							else
							{
								err = 1;
								break;
							}
							if(((cmd[15 + i] > 47) && (cmd[15 + i] < 58)))
							{
								ssid *= 10;
								ssid += cmd[15 + i] - 48;
							}
							else if(!((cmd[15 + i] == '\r') || (cmd[15 + i] == '\n')))
							{
								err = 1;
								break;
							}
							if(ssid > 15) err = 1;
							break;
						}
						else if(((cmd[13 + i] > 47) && (cmd[13 + i] < 58)) || ((cmd[13 + i] > 64) && (cmd[13 + i] < 91)))
						{
							elemlen++;
							tmp[tmpidx++] = cmd[13 + i];
						}
						else
						{
							err = 1;
							break;
						}
					}
					if(err)
					{
						term_sendString((uint8_t*)"Incorrect alias!\r\n", 0);
						term_sendBuf(src);
						return;
					}


					for(uint8_t i = 0; i < 6; i++)
					{
						digi.alias[alno][i] = 64;
						if(tmp[i] != 0)
							digi.alias[alno][i] = tmp[i] << 1;
					}
					digi.ssid[alno - 4] = ssid;
					err = 0;
				}

			}

			else if((checkcmd(&cmd[7], 4, (uint8_t*)"max ") || checkcmd(&cmd[7], 4, (uint8_t*)"rep ")) && (alno < 4))
			{

				uint8_t err = 0;

				if(!((cmd[12] == '\r') || (cmd[12] == '\n')))
				{
					err = 1;
				}
				if(!(((cmd[11] > 47) && (cmd[11] < 58))))
				{
					err = 1;
				}

				uint8_t val = cmd[11] - 48;
				if(cmd[7] == 'm' && (val < 1 || val > 7))
					err = 1;
				if(cmd[7] == 'r' && (val > 7))
					err = 1;

				if(err)
				{
					term_sendString((uint8_t*)"Incorrect value!\r\n", 0);
				}
				else
				{
					if(cmd[7] == 'm')
						digi.max[alno] = val;
					else digi.rep[alno] = val;
					term_sendString((uint8_t*)"OK\r\n", 0);
				}
				term_sendBuf(src);
				return;
			}
			else if(checkcmd(&cmd[7], 5, (uint8_t*)"trac "))
			{
				if(cmd[12] == 'o' && cmd[13] == 'n')
					digi.traced |= 1 << alno;
				else if(cmd[12] == 'o' && cmd[13] == 'f' && cmd[14] == 'f')
					digi.traced &= ~(1 << alno);
				else err = 1;
			}
			else if(checkcmd(&cmd[7], 7, (uint8_t*)"filter "))
			{
				if(cmd[14] == 'o' && cmd[15] == 'n')
					digi.callFilterEnable |= 1 << alno;
				else if(cmd[14] == 'o' && cmd[15] == 'f' && cmd[16] == 'f')
					digi.callFilterEnable &= ~(1 << alno);
				else err = 1;
			}
			else if(checkcmd(&cmd[7], 8, (uint8_t*)"viscous "))
			{
				if(cmd[15] == 'o' && cmd[16] == 'n')
				{
					digi.viscous |= (1 << alno);
					digi.directOnly &= ~(1 << alno); //disable directonly mode
				}
				else if(cmd[15] == 'o' && cmd[16] == 'f' && cmd[17] == 'f')
					digi.viscous &= ~(1 << alno);
				else
					err = 1;
			}
			else if(checkcmd(&cmd[7], 7, (uint8_t*)"direct "))
			{
				if(cmd[14] == 'o' && cmd[15] == 'n')
				{
					digi.directOnly |= (1 << alno);
					digi.viscous &= ~(1 << alno); //disable viscous delay mode
				}
				else if(cmd[14] == 'o' && cmd[15] == 'f' && cmd[16] == 'f')
					digi.directOnly &= ~(1 << alno);
				else err = 1;
			}


		}
		else if(checkcmd(&cmd[5], 7, (uint8_t*)"filter "))
		{
			if(checkcmd(&cmd[12], 5, (uint8_t*)"white"))
				digi.filterPolarity = 1;
			else if(checkcmd(&cmd[12], 5, (uint8_t*)"black"))
				digi.filterPolarity = 0;
			else
				err = 1;
		}
		else if(checkcmd(&cmd[5], 5, (uint8_t*)"dupe "))
		{
			uint8_t tmp[4] = {0};
			uint8_t err = 0;
			for(uint8_t i = 0; i < 4; i++)
			{
				if((cmd[10 + i] == '\r') || (cmd[10 + i] == '\n'))
				{
					if((i == 0))
						err = 1;
					break;
				}
				if(!(((cmd[10 + i] > 47) && (cmd[10 + i] < 58))))
				{
					err = 1;
					break;
				}
				if(i == 3)
				{
					err = 1;
					break;
				}
				tmp[i] = cmd[10 + i];
			}
			if(!err)
			{
				uint16_t t = 0;
				t = (uint16_t)strToInt(tmp, 0);
				if((t > 255) || (t < 5))
					err = 1;
				else
					digi.dupeTime = (uint8_t)t;
			}
			if(err)
			{
				term_sendString((uint8_t*)"Incorrect anti-dupe time!\r\n", 0);
			}
			else
			{
				term_sendString((uint8_t*)"OK\r\n", 0);
			}
			term_sendBuf(src);
			return;
		}
		else if(checkcmd(&cmd[5], 5, (uint8_t*)"list "))
		{
			uint8_t no = 0;
			if((((cmd[10] > 47) && (cmd[10] < 58))))
			{
				no = cmd[10] - 48;
			}
			else err = 1;

			if((((cmd[11] > 47) && (cmd[11] < 58))) && !err)
			{
				no *= 10;
				no += cmd[11] - 48;
			}

			if(no > 19) err = 1;

			uint8_t shift = 12;
			if(no > 9) shift = 13;

			if(checkcmd(&cmd[shift], 4, (uint8_t*)"set "))
			{
				uint8_t dlen = 0;
				if((cmd[len - 2] == '\r') || (cmd[len - 2] == '\n'))
					dlen = len - shift - 7;
				else
					dlen = len - shift - 5;

				if(dlen == 0)
					err = 1;
				uint8_t j = 0;
				uint8_t h = 0;

				volatile uint8_t tmp[7] = {0};

				if(!err) for(j = 0; j < dlen; j++)
                 {
                      if(cmd[shift + 4 + j] == '-')
                      {
                          if((cmd[shift + 5 + j] == '*') || (cmd[shift + 5 + j] == '?'))
                          {
                              tmp[6] = 0xFF;
                              break;
                          }
                          if((dlen - 1 - j) == 2)
                          {
                        	  if((cmd[shift + 6 + j] > 47) && (cmd[shift + 6 + j] < 58))
                        		  tmp[6] += cmd[shift + 6 + j] - 48;
                        	  else
                        		  err = 1;
                        	  if((cmd[shift + 5 + j] > 47) && (cmd[shift + 5 + j] < 58))
                        		  tmp[6] += (cmd[shift + 5 + j] - 48) * 10;
                        	  else
                        		  err = 1;
                        	  if(tmp[6] > 15)
                        		  err = 1;
                          }
                          else if((cmd[shift + 5 + j] > 47) && (cmd[shift + 5 + j] < 58))
                        	  tmp[6] += cmd[shift + 5 + j] - 48;
                          else
                        	  err = 1;
                          break;
                      }
                      if(cmd[shift + 4 + j] == '?')
                      {
                          tmp[j] = 0xFF;
                          continue;
                      } else if(cmd[shift + 4 + j] == '*')
                      {
                           h = j;
                          while(h < 6)
                          {
                              tmp[h] = 0xFF;
                               h++;
                          }
                          continue;
                      }
                      if(((cmd[shift + 4 + j] > 47) && (cmd[shift + 4 + j] < 58)) || ((cmd[shift + 4 + j] > 64) && (cmd[shift + 4 + j] < 91))) tmp[j] = cmd[shift + 4 + j];
                      else err = 1;
                  }

                 if(!err)
                 {
                	 while((j + h) < 6) //fill with spaces
                	 {
                		 tmp[j] = ' ';
                     	 j++;
                 	 }
                	 for(uint8_t i = 0; i < 7; i++)
                		 digi.callFilter[no][i] = tmp[i];
                 }
                 else
                 {
                	 term_sendString((uint8_t*)"Incorrect format!\r\n", 0);
                	 term_sendBuf(src);
                	 return;
                 }
			}
			else if(checkcmd(&cmd[shift], 6, (uint8_t*)"remove"))
			{
				for(uint8_t i = 0; i < 7; i++)
					digi.callFilter[no][i] = 0;
			}
			else
				err = 1;


			if(err)
			{
				term_sendString((uint8_t*)"Incorrect command!\r\n", 0);
			}
			else
			{
				term_sendString((uint8_t*)"OK\r\n", 0);
			}
			term_sendBuf(src);
			return;
		}
		else err = 1;

		if(err == 1)
		{
			term_sendString((uint8_t*)"Incorrect command\r\n", 0);
		}
		else term_sendString((uint8_t*)"OK\r\n", 0);

		term_sendBuf(src);
		return;
	}


	if(checkcmd(cmd, 10, (uint8_t*)"autoreset "))
	{
		uint8_t tmp[4] = {0};
		uint8_t err = 0;
		for(uint8_t i = 0; i < 4; i++)
		{
			if((cmd[10 + i] == '\r') || (cmd[10 + i] == '\n'))
			{
				if((i == 0))
					err = 1;
				break;
			}
			if(!(((cmd[10 + i] > 47) && (cmd[10 + i] < 58))))
			{
				err = 1;
				break;
			}
			if(i == 3)
			{
				err = 1;
				break;
			}
			tmp[i] = cmd[10 + i];
		}
		if(!err)
		{
			uint8_t t = 0;
			t = strToInt(tmp, 0);
			if(t > 255)
				err = 1;
			else
				autoReset = t;
		}
		if(err)
		{
			term_sendString((uint8_t*)"Incorrect time interval!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}
	if(checkcmd(cmd, 4, (uint8_t*)"pwm "))
	{
		uint8_t err = 0;
		if(checkcmd(&cmd[4], 2, (uint8_t*)"on"))
			afskCfg.usePWM = 1;
		else if(checkcmd(&cmd[4], 3, (uint8_t*)"off"))
			afskCfg.usePWM = 0;
		else
			err = 1;

		if(err)
		{
			term_sendString((uint8_t*)"Incorrect command!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}
	if(checkcmd(cmd, 5, (uint8_t*)"flat "))
	{
		uint8_t err = 0;
		if(checkcmd(&cmd[5], 2, (uint8_t*)"on"))
			afskCfg.flatAudioIn = 1;
		else if(checkcmd(&cmd[5], 3, (uint8_t*)"off"))
			afskCfg.flatAudioIn = 0;
		else
			err = 1;

		if(err)
		{
			term_sendString((uint8_t*)"Incorrect command!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	if(checkcmd(cmd, 8, (uint8_t*)"monkiss "))
	{
		uint8_t err = 0;
		if(checkcmd(&cmd[8], 2, (uint8_t*)"on"))
			kissMonitor = 1;
		else if(checkcmd(&cmd[8], 3, (uint8_t*)"off"))
			kissMonitor = 0;
		else
			err = 1;

		if(err)
		{
			term_sendString((uint8_t*)"Incorrect command!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}
	if(checkcmd(cmd, 8, (uint8_t*)"nonaprs "))
	{
		uint8_t err = 0;
		if(checkcmd(&cmd[8], 2, (uint8_t*)"on"))
			ax25Cfg.allowNonAprs = 1;
		else if(checkcmd(&cmd[8], 3, (uint8_t*)"off"))
			ax25Cfg.allowNonAprs = 0;
		else
			err = 1;

		if(err)
		{
			term_sendString((uint8_t*)"Incorrect command!\r\n", 0);
		}
		else
		{
			term_sendString((uint8_t*)"OK\r\n", 0);
		}
		term_sendBuf(src);
		return;
	}

	term_sendString((uint8_t*)"Unknown command. For command list type \"help\"\r\n", 0);
	term_sendBuf(src);


}
