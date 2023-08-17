/*
This file is part of VP-DigiConfig.

VP-Digi is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

VP-Digi is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VP-DigiConfig.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "terminal.h"
#include "common.h"
#include "beacon.h"
#include "digipeater.h"
#include "config.h"
#include "drivers/modem.h"
#include "ax25.h"


//uint16_t spLastIdx[3] = {0, 0, 0}; //index buffer was "special" terminal cases

/**
 * @brief Handle "special" terminal cases like backspace or local echo
 * @param[in] src Source: TERM_USB, TERM_UART1, TERM_UART2
 * @attention Must be called for every received data
 */
//void term_handleSpecial(Terminal_stream src)
//{
//	if(src == TERM_USB)
//	{
//		if(USBmode == MODE_KISS) //don't do anything in KISS mode
//		{
//			spLastIdx[0] = 0;
//			return;
//		}
//		if(spLastIdx[0] >= usbcdcidx) //USB RX buffer index was probably cleared
//			spLastIdx[0] = 0;
//
//		if(usbcdcdata[usbcdcidx - 1] == '\b') //user entered backspace
//		{
//			if(usbcdcidx > 1) //there was some data in buffer
//			{
//				usbcdcidx -= 2; //remove backspace and preceding character
//				CDC_Transmit_FS((uint8_t*)"\b \b", 3); //backspace (one character left), remove backspaced character (send space) and backspace again
//				if(spLastIdx[0] > 0)
//					spLastIdx[0]--; //1 character was removed
//			}
//			else //there was only a backspace
//				usbcdcidx = 0;
//		}
//		uint16_t t = usbcdcidx; //store last index
//		if(spLastIdx[0] < t) //local echo handling
//		{
//			CDC_Transmit_FS(&usbcdcdata[spLastIdx[0]], t - spLastIdx[0]); //echo characters entered by user
//			if((usbcdcdata[t - 1] == '\r') || (usbcdcdata[t - 1] == '\n'))
//				CDC_Transmit_FS((uint8_t*)"\r\n", 2);
//			spLastIdx[0] = t;
//		}
//	}
//	else if((src == TERM_UART1) || (src == TERM_UART2))
//	{
//		Uart *u = &uart1;
//		uint8_t nr = 1;
//		if(src == TERM_UART2)
//		{
//			u = &uart2;
//			nr = 2;
//		}
//
//
//		if(u->mode == MODE_KISS) //don't do anything in KISS mode
//		{
//			spLastIdx[nr] = 0;
//			return;
//		}
//		if(spLastIdx[nr] >= u->bufrxidx) //UART RX buffer index was probably cleared
//			spLastIdx[nr] = 0;
//
//		if(u->bufrx[u->bufrxidx - 1] == '\b') //user entered backspace
//		{
//			if(u->bufrxidx > 1) //there was some data in buffer
//			{
//				u->bufrxidx -= 2; //remove backspace and preceding character
//				uart_sendString(u, (uint8_t*)"\b \b", 3); //backspace (one character left), remove backspaced character (send space) and backspace again
//				if(spLastIdx[nr] > 0)
//					spLastIdx[nr]--; //1 character was removed
//			}
//			else //there was only a backspace
//				u->bufrxidx = 0;
//		}
//		uint16_t t = u->bufrxidx; //store last index
//		if(spLastIdx[nr] < t) //local echo handling
//		{
//			uart_sendString(u, &u->bufrx[spLastIdx[nr]], t - spLastIdx[nr]); //echo characters entered by user
//			if((u->bufrx[t - 1] == '\r') || (u->bufrx[t - 1] == '\n'))
//				uart_sendString(u, (uint8_t*)"\r\n", 2);
//			spLastIdx[nr] = t;
//		}
//		uart_transmitStart(u);
//	}
//
//}


static void sendKiss(Uart *port, uint8_t *buf, uint16_t len)
{
	if(port->mode == MODE_KISS) //check if KISS mode
	{
		UartSendByte(port, 0xc0); //send data in kiss format
		UartSendByte(port, 0x00);
		UartSendString(port, buf, len);
		UartSendByte(port, 0xc0);
	}
}

void TermSendToAll(enum UartMode mode, uint8_t *data, uint16_t size)
{
	if(MODE_KISS == mode)
	{
		sendKiss(&Uart1, data, size);
		sendKiss(&Uart2, data, size);
		sendKiss(&UartUsb, data, size);
	}
	else if(MODE_MONITOR == mode)
	{
		if(UartUsb.mode == MODE_MONITOR)
			UartSendString(&UartUsb, data, size);
		if(Uart1.mode == MODE_MONITOR)
			UartSendString(&Uart1, data, size);
		if(Uart2.mode == MODE_MONITOR)
			UartSendString(&Uart2, data, size);
	}

}

void TermSendNumberToAll(enum UartMode mode, int32_t n)
{
	if(MODE_MONITOR == mode)
	{
		if(UartUsb.mode == MODE_MONITOR)
			UartSendNumber(&UartUsb, n);
		if(Uart1.mode == MODE_MONITOR)
			UartSendNumber(&Uart1, n);
		if(Uart2.mode == MODE_MONITOR)
			UartSendNumber(&Uart2, n);
	}

}




static const char monitorHelp[] = "\r\nCommans available in monitor mode:\r\n"
		"help - shows this help page\r\n"
		"cal {low|high|alt|stop} - transmits/stops transmitter calibration pattern\r\n"
		"\tlow - transmits MARK tone, high - transmits SPACE tone, alt - transmits alternating tones (null bytes)\r\n"
		"beacon <beacon_number> - immediately transmits selected beacon (number from 0 to 7)\r\n"
		"kiss - switches to KISS mode\r\n"
		"config - switches to config mode\r\n"
		"reboot - reboots the device\r\n"
		"version - shows full firmware version info\r\n\r\n\r\n";

static const char configHelp[] = 	"\r\nCommands available in config mode:\r\n"
		"print - prints all configuration parameters\r\n"
		"list - prints callsign filter list\r\n"
		"save - saves configuration and reboots the device\r\n"
		"eraseall - erases all configurations and reboots the device\r\n"
		"help - shows this help page\r\n"
		"reboot - reboots the device\r\n"
		"version - shows full firmware version info\r\n\r\n"
		"call <callsign-SSID> - sets callsign with optional SSID\r\n"
		"dest <address> - sets destination address\r\n"
		"txdelay <50-2550> - sets TXDelay time (ms)\r\n"
		"txtail <10-2550> - sets TXTail time (ms)\r\n"
		"quiet <100-2550> - sets quiet time (ms)\r\n"
		"rs1baud/rs2baud <1200-115200> - sets UART1/UART2 baudrate\r\n"
		"pwm [on/off] - enables/disables PWM. If PWM is off, R2R will be used instead\r\n"
		"flat [on/off] - set to \"on\" if flat audio input is used\r\n"
		"beacon <0-7> [on/off] - enables/disables specified beacon\r\n"
		"beacon <0-7> [iv/dl] <0-720> - sets interval/delay for the specified beacon (min)\r\n"
		"beacon <0-7> path <el1,[el2]>/none - sets path for the specified beacon\r\n"
		"beacon <0-7> data <data> - sets information field for the specified beacon\r\n"
		"digi [on/off] - enables/disables whole digipeater\r\n"
		"digi <0-7> [on/off] - enables/disables specified slot\r\n"
		"digi <0-7> alias <alias> - sets alias for the specified slot\r\n"
		"digi <0-3> [max/rep] <0/1-7> - sets maximum/replacement N for the specified slot\r\n"
		"digi <0-7> trac [on/off] - enables/disables packet tracing for the specified slot\r\n"
		"digi <0-7> viscous [on/off] - enables/disables viscous-delay digipeating for the specified slot\r\n"
		"digi <0-7> direct [on/off] - enables/disables direct-only digipeating for the specified slot\r\n"\
		"digi <0-7> filter [on/off] - enables/disables packet filtering for the specified slot\r\n"
		"digi filter [black/white] - sets filtering type to blacklist/whitelist\r\n"
		"digi dupe <5-255> - sets anti-dupe buffer time (s)\r\n"
		"digi list <0-19> [set <call>/remove] - sets/clears specified callsign slot in filter list\r\n"
		"monkiss [on/off] - send own and digipeated frames to KISS ports\r\n"
		"nonaprs [on/off] - enable reception of non-APRS frames\r\n";



static void printConfig(Uart *src)
{
	UartSendString(src, "Callsign: ", 0);
	for(uint8_t i = 0; i < 6; i++)
	{
		if(GeneralConfig.call[i] != (' ' << 1))
			UartSendByte(src, GeneralConfig.call[i] >> 1);
	}
	UartSendByte(src, '-');
	UartSendNumber(src, GeneralConfig.callSsid);

	UartSendString(src, "\r\nDestination: ", 0);
	for(uint8_t i = 0; i < 6; i++)
	{
		if(GeneralConfig.dest[i] != (' ' << 1))
			UartSendByte(src, GeneralConfig.dest[i] >> 1);
	}

	UartSendString(src, "\r\nTXDelay (ms): ", 0);
	UartSendNumber(src, Ax25Config.txDelayLength);
	UartSendString(src, "\r\nTXTail (ms): ", 0);
	UartSendNumber(src, Ax25Config.txTailLength);
	UartSendString(src, "\r\nQuiet time (ms): ", 0);
	UartSendNumber(src, Ax25Config.quietTime);
	UartSendString(src, "\r\nUART1 baudrate: ", 0);
	UartSendNumber(src, Uart1.baudrate);
	UartSendString(src, "\r\nUART2 baudrate: ", 0);
	UartSendNumber(src, Uart2.baudrate);
	UartSendString(src, "\r\nDAC type: ", 0);
	if(ModemConfig.usePWM)
		UartSendString(src, "PWM", 0);
	else
		UartSendString(src, "R2R", 0);
	UartSendString(src, "\r\nFlat audio input: ", 0);
	if(ModemConfig.flatAudioIn)
		UartSendString(src, "yes", 0);
	else
		UartSendString(src, "no", 0);
	for(uint8_t i = 0; i < (sizeof(beacon) / sizeof(*beacon)); i++)
	{
		UartSendString(src, "\r\nBeacon ", 0);
		UartSendByte(src, i + '0');
		UartSendString(src, ": ", 0);
		if(beacon[i].enable)
			UartSendString(src, "On, Iv: ", 0);
		else
			UartSendString(src, "Off, Iv: ", 0);
		UartSendNumber(src, beacon[i].interval / 6000);
		UartSendString(src, ", Dl: ", 0);
		UartSendNumber(src, beacon[i].delay / 60);
		UartSendByte(src, ',');
		UartSendByte(src, ' ');
		if(beacon[i].path[0] != 0)
		{
			for(uint8_t j = 0; j < 6; j++)
			{
				if(beacon[i].path[j] != (' ' << 1))
					UartSendByte(src, beacon[i].path[j] >> 1);
			}
			UartSendByte(src, '-');
			UartSendNumber(src, beacon[i].path[6]);
			if(beacon[i].path[7] != 0)
			{
				UartSendByte(src, ',');
				for(uint8_t j = 7; j < 13; j++)
				{
					if(beacon[i].path[j] != (' ' << 1))
						UartSendByte(src, beacon[i].path[j] >> 1);
				}
				UartSendByte(src, '-');
				UartSendNumber(src, beacon[i].path[13]);
			}
		}
		else
			UartSendString(src, "no path", 0);
		UartSendByte(src, ',');
		UartSendByte(src, ' ');
		UartSendString(src, beacon[i].data, 0);
	}

	UartSendString(src, "\r\nDigipeater: ", 0);
	if(DigiConfig.enable)
		UartSendString(src, "On\r\n", 0);
	else
		UartSendString(src, "Off\r\n", 0);

	for(uint8_t i = 0; i < 4; i++) //n-N aliases
	{
		UartSendString(src, "Alias ", 0);
		if(DigiConfig.alias[i][0] != 0)
		{
			for(uint8_t j = 0; j < 5; j++)
			{
				if(DigiConfig.alias[i][j] != 0)
					UartSendByte(src, DigiConfig.alias[i][j] >> 1);
				else
					break;
			}
		}
		UartSendString(src, ": ", 0);
		if(DigiConfig.enableAlias & (1 << i))
			UartSendString(src, "On, max: ", 0);
		else
			UartSendString(src, "Off, max: ", 0);
		UartSendNumber(src, DigiConfig.max[i]);
		UartSendString(src, ", rep: ", 0);
		UartSendNumber(src, DigiConfig.rep[i]);
		if(DigiConfig.traced & (1 << i))
			UartSendString(src, ", traced, ", 0);
		else
			UartSendString(src, ", untraced, ", 0);
		if(DigiConfig.viscous & (1 << i))
			UartSendString(src, "viscous-delay, ", 0);
		else if(DigiConfig.directOnly & (1 << i))
			UartSendString(src, "direct-only, ", 0);
		if(DigiConfig.callFilterEnable & (1 << i))
			UartSendString(src, "filtered\r\n", 0);
		else
			UartSendString(src, "unfiltered\r\n", 0);
	}
	for(uint8_t i = 0; i < 4; i++) //simple aliases
	{
		UartSendString(src, "Alias ", 0);
		if(DigiConfig.alias[i + 4][0] != 0)
		{
			for(uint8_t j = 0; j < 6; j++)
			{
				if(DigiConfig.alias[i + 4][j] != 64)
					UartSendByte(src, DigiConfig.alias[i + 4][j] >> 1);
			}
		}
		UartSendByte(src, '-');
		UartSendNumber(src, DigiConfig.ssid[i]);
		UartSendString(src, ": ", 0);
		if(DigiConfig.enableAlias & (1 << (i + 4)))
			UartSendString(src, "On, ", 0);
		else
			UartSendString(src, "Off, ", 0);
		if(DigiConfig.traced & (1 << (i + 4)))
			UartSendString(src, "traced, ", 0);
		else
			UartSendString(src, "untraced, ", 0);
		if(DigiConfig.viscous & (1 << (i + 4)))
			UartSendString(src, "viscous-delay, ", 0);
		else if(DigiConfig.directOnly & (1 << (i + 4)))
			UartSendString(src, "direct-only, ", 0);
		if(DigiConfig.callFilterEnable & (1 << (i + 4)))
			UartSendString(src, "filtered\r\n", 0);
		else
			UartSendString(src, "unfiltered\r\n", 0);
	}
	UartSendString(src, "Anti-duplicate buffer hold time (s): ", 0);
	UartSendNumber(src, DigiConfig.dupeTime);
	UartSendString(src, "\r\nCallsign filter type: ", 0);
	if(DigiConfig.filterPolarity)
		UartSendString(src, "whitelist\r\n", 0);
	else
		UartSendString(src, "blacklist\r\n", 0);
	UartSendString(src, "Callsign filter list: ", 0);
	uint8_t entries = 0;
	for(uint8_t i = 0; i < 20; i++)
	{
		if(DigiConfig.callFilter[i][0] != 0)
			entries++;
	}
	UartSendNumber(src, entries);
	UartSendString(src, " entries\r\nKISS monitor: ", 0);
	if(GeneralConfig.kissMonitor == 1)
		UartSendString(src, "On\r\n", 0);
	else
		UartSendString(src, "Off\r\n", 0);
	UartSendString(src, "Allow non-APRS frames: ", 0);
	if(Ax25Config.allowNonAprs == 1)
		UartSendString(src, "On\r\n", 0);
	else
		UartSendString(src, "Off\r\n", 0);
}


void TermParse(Uart *src)
{
	const char *cmd = (char*)src->rxBuffer;
	uint16_t len = src->rxBufferHead;
	for(uint16_t i = 0; i < len; i++)
	{
		if((cmd[i] == '\r') || (cmd[i] == '\n'))
		{
			len = i;
			break;
		}
	}

	/*
	 * Terminal mode switching commands
	 */
	if(!strncmp(cmd, "kiss", 4))
	{
		UartSendString(src, (uint8_t*)"Switched to KISS mode\r\n", 0);
		src->mode = MODE_KISS;
		return;
	}
	else if(!strncmp(cmd, "config", 6))
	{
		UartSendString(src, (uint8_t*)"Switched to configuration mode\r\n"
				"Most settings will take effect immediately, but\r\n"
				"remember to save the configuration using \"save\"\r\n", 0);
		src->mode = MODE_TERM;
		return;
	}
	else if(!strncmp(cmd, "monitor", 7))
	{
		UartSendString(src, (uint8_t*)"USB switched to monitor mode\r\n", 0);
		src->mode = MODE_MONITOR;
		return;
	}
	/*
	 * KISS parsing
	 */
	else if((src->mode == MODE_KISS) && (src->rxType == DATA_KISS))
	{
		//Uart_txKiss(cmd, len); //if received KISS data, transmit KISS frame
		return;
	}
	/*
	 * Monitor mode handling
	 */
	else if((src->mode == MODE_MONITOR) && (src->rxType == DATA_TERM)) //monitor mode
	{
		if(!strncmp(cmd, "help", 4))
		{
			UartSendString(src, (char *)monitorHelp, 0);
		}
		else if(!strncmp(cmd, "version", 7))
		{
			UartSendString(src, (char *)versionString, 0);
		}
		else if(!strncmp(cmd, "reboot", 6))
		{
			NVIC_SystemReset();
		}
		else if(!strncmp(cmd, "beacon ", 7))
		{
			if((cmd[7] >= '0') && (cmd[7] <= '7'))
			{
				uint8_t number = cmd[7] - '0';
				if((beacon[number].interval != 0) && (beacon[number].enable != 0))
				{
					BeaconSend(number);
				}
				else
				{
					UartSendString(src, "Beacon " , 0);
					UartSendNumber(src, number);
					UartSendString(src, " is not enabled. Cannot transmit disabled beacons.\r\n", 0);
				}
			}
			else
			{
				UartSendString(src, "Beacon number must be in range of 0 to 7.\r\n", 0);
			}
		}
		else if(!strncmp(cmd, "cal", 3))
		{
			if(!strncmp(&cmd[4], "low", 3))
			{
				UartSendString(src, "Starting low tone calibration transmission\r\n", 0);
				ModemTxTestStart(TEST_MARK);
			}
			else if(!strncmp(&cmd[4], "high", 4))
			{
				UartSendString(src, "Starting high tone calibration transmission\r\n", 0);
				ModemTxTestStart(TEST_SPACE);
			}
			else if(!strncmp(&cmd[4], "alt", 3))
			{
				UartSendString(src, "Starting alternating tones calibration transmission\r\n", 0);
				ModemTxTestStart(TEST_ALTERNATING);
			}
			else if(!strncmp(&cmd[4], "stop", 4))
			{
				UartSendString(src, "Stopping calibration transmission\r\n", 0);
				ModemTxTestStop();
			}
			else
				UartSendString(src, "Usage: cal {low|high|alt|stop}\r\n", 0);
		}
		else
			UartSendString(src, "Unknown command. For command list type \"help\"\r\n", 0);

		return;
	}

	if(src->mode != MODE_TERM)
		return;

	/*
	 * Configuration mode handling
	 *
	 * General commands
	 */

	bool err = false;

	if(!strncmp(cmd, "help", 4))
	{
		UartSendString(src, (uint8_t*)configHelp, 0);
	}
	else if(!strncmp(cmd, "version", 7))
	{
		UartSendString(src, (uint8_t*)versionString, 0);
	}
	else if(!strncmp(cmd, "reboot", 6))
	{
		NVIC_SystemReset();
	}
	else if(!strncmp(cmd, "save", 4))
	{
		ConfigWrite();
		NVIC_SystemReset();
	}
	else if(!strncmp(cmd, "eraseall", 8))
	{
		ConfigErase();
		NVIC_SystemReset();
	}
	else if(!strncmp(cmd, "print", 5))
	{
		printConfig(src);
	}
	else if(!strncmp(cmd, "list", 4)) //list calls in call filter table
	{
		UartSendString(src, "Callsign filter list: \r\n", 0);
		for(uint8_t i = 0; i < 20; i++)
		{
			UartSendNumber(src, i);
			UartSendString(src, ". ", 0);
			if(DigiConfig.callFilter[i][0] != 0)
			{
                    uint8_t cl[10] = {0};
                    uint8_t clidx = 0;
                    for(uint8_t h = 0; h < 6; h++)
                    {
                        if(DigiConfig.callFilter[i][h] == 0xFF) //wildcard
                        {
                            uint8_t g = h;
                            uint8_t j = 0;
                            while(g < 6)
                            {
                                if(DigiConfig.callFilter[i][g] != 0xFF)
                                	j = 1;
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
                            if(DigiConfig.callFilter[i][h] != ' ')
                            	cl[clidx++] = DigiConfig.callFilter[i][h];
                        }
                    }
                    if(DigiConfig.callFilter[i][6] == 0xFF)
                    {
                    	cl[clidx++] = '-';
                    	cl[clidx++] = '*';
                    }
                    else if(DigiConfig.callFilter[i][6] != 0)
                    {
                    	cl[clidx++] = '-';
                        if(DigiConfig.callFilter[i][6] > 9)
                        {
                        	cl[clidx++] = (DigiConfig.callFilter[i][6] / 10) + 48;
                        	cl[clidx++] = (DigiConfig.callFilter[i][6] % 10) + 48;
                        }
                        else cl[clidx++] = DigiConfig.callFilter[i][6] + 48;
                    }
                    UartSendString(src, cl, 0);
			}
			else
				UartSendString(src, "empty", 0);
			UartSendString(src, "\r\n", 0);
			return;
		}
	}
	/*
	 * Settings insertion handling
	 */
	else if(!strncmp(cmd, "call", 4))
	{

		if(!ParseCallsignWithSsid(&cmd[5], len - 5, GeneralConfig.call, &GeneralConfig.callSsid))
		{
			UartSendString(src, "Incorrect callsign!\r\n", 0);
			return;
		}
		else
		{
			UartSendString(src, "OK\r\n", 0);
		}
		return;
	}
	else if(!strncmp(cmd, "dest", 4))
	{
		if(!ParseCallsign(&cmd[5], len - 5, GeneralConfig.dest))
		{
			UartSendString(src, "Incorrect address!\r\n", 0);
			return;
		}
		else
		{
			UartSendString(src, "OK\r\n", 0);
		}
	}
	else if(!strncmp(cmd, "txdelay", 7))
	{
		int64_t t = StrToInt(&cmd[8], len - 8);
		if((t > 2550) || (t < 50))
		{
			UartSendString(src, "Incorrect TXDelay!\r\n", 0);
			return;
		}
		else
		{
			Ax25Config.txDelayLength = (uint16_t)t;
			UartSendString(src, "OK\r\n", 0);
		}
	}
	else if(!strncmp(cmd, "txtail", 6))
	{
		int64_t t = StrToInt(&cmd[7], len - 7);
		if((t > 2550) || (t < 50))
		{
			UartSendString(src, "Incorrect TXTail!\r\n", 0);
			return;
		}
		else
		{
			Ax25Config.txTailLength = (uint16_t)t;
			UartSendString(src, "OK\r\n", 0);
		}
	}

	else if(!strncmp(cmd, "quiet", 5))
	{
		int64_t t = StrToInt(&cmd[6], len - 6);
		if((t > 2550) || (t < 100))
		{
			UartSendString(src, "Incorrect quiet time!\r\n", 0);
			return;
		}
		else
		{
			Ax25Config.quietTime = (uint16_t)t;
		}
	}
	else if(!strncmp(cmd, "rs1baud", 7) || !strncmp(cmd, "rs2baud", 7))
	{
		int64_t t = StrToInt(&cmd[8], len - 8);
		if((t > 115200) || (t < 1200))
		{
			UartSendString(src, "Incorrect baudrate!\r\n", 0);
		}
		else
		{
			if(cmd[2] == '1')
				Uart1.baudrate = (uint32_t)t;
			else if(cmd[2] == '2')
				Uart2.baudrate = (uint32_t)t;
			else
			{
				UartSendString(src, "Incorrect port number!\r\n", 0);
				return;
			}
		}
	}
	else if(!strncmp(cmd, "beacon", 6))
	{
		uint8_t bcno = 0;
		bcno = cmd[7] - 48;
		if(bcno > 7)
		{
			UartSendString(src, "Incorrect beacon number\r\n", 0);
			return;
		}
		if(!strncmp(&cmd[9], "on", 2))
			beacon[bcno].enable = 1;
		else if(!strncmp(&cmd[9], "off", 3))
			beacon[bcno].enable = 0;
		else if(!strncmp(&cmd[9], "iv", 2) || !strncmp(&cmd[9], "dl", 2)) //interval or delay
		{
			int64_t t = StrToInt(&cmd[12], len - 12);
			if(t > 720)
			{
				UartSendString(src, "Interval/delay must lesser or equal to 720 minutes\r\n", 0);
				return;
			}
			if(!strncmp(&cmd[9], "iv", 2))
				beacon[bcno].interval = t * 6000;
			else
				beacon[bcno].delay = t * 60;

		}
		else if(!strncmp(&cmd[9], "data", 4))
		{
			if((len - 14) > BEACON_MAX_PAYLOAD_SIZE)
			{
				UartSendString(src, "Data is too long\r\n", 0);
				return;
			}
			uint16_t i = 0;
			for(; i < (len - 14); i++)
				beacon[bcno].data[i] = cmd[14 + i];
			beacon[bcno].data[i] = 0;
		}
		else if(!strncmp(&cmd[9], "path", 4))
		{

			if((len - 14) < 0)
			{
				UartSendString(src, "Path cannot be empty. Use \"none\" for empty path. \r\n", 0);
				return;
			}
			if(((len - 14) == 4) && !strncmp(&cmd[14], "none", 4)) //"none" path
			{
				memset(beacon[bcno].path, 0, sizeof(beacon[bcno].path));

				UartSendString(src, "OK\r\n", 0);
				return;
			}

			uint8_t tmp[14];
			uint8_t tmpIdx = 0;
			uint16_t elementStart = 14;
			for(uint8_t i = 0; i < (len - 14); i++)
			{
				if((cmd[14 + i] == ',') || ((14 + i + 1) == len))
				{
					if((14 + i + 1) == len) //end of data
					{
						i++;
						tmp[7] = 0;
					}

					if((14 + i - elementStart) > 0)
					{
						if(!ParseCallsignWithSsid(&cmd[elementStart], 14 + i - elementStart, &tmp[tmpIdx], &tmp[tmpIdx + 6]))
						{
							err = true;
							break;
						}
						tmpIdx += 7;
						if(tmpIdx == 14)
							break;
					}
					elementStart = 14 + i + 1;
				}
			}
			if(err)
			{
				UartSendString(src, "Incorrect path!\r\n", 0);
				return;
			}

			memcpy(beacon[bcno].path, tmp, 14);
		}
		else
		{
			err = true;
		}

		UartSendString(src, "OK\r\n", 0);
	}
	else if(!strncmp(cmd, "digi", 4))
	{
		if(!strncmp(&cmd[5], "on", 2))
			DigiConfig.enable = 1;
		else if(!strncmp(&cmd[5], "off", 3))
			DigiConfig.enable = 0;
		else if(IS_NUMBER(cmd[5]))
		{
			uint8_t alno = 0;
			alno = cmd[5] - 48;
			if(alno > 7)
			{
				UartSendString(src, "Incorrect alias number\r\n", 0);
				return;
			}
			if(!strncmp(&cmd[7], "on", 2))
				DigiConfig.enableAlias |= (1 << alno);
			else if(!strncmp(&cmd[7], "off", 3))
				DigiConfig.enableAlias &= ~(1 << alno);
			else if(!strncmp(&cmd[7], "alias ", 6))
			{
				if(alno < 4) //New-N aliases
				{
					if((len - 13) <= 5)
					{
						if(false == (err = !ParseCallsign(&cmd[13], len - 13, DigiConfig.alias[alno])))
							DigiConfig.alias[alno][len - 13] = 0;
					}
					else
						err = true;
				}
				else  //simple aliases
				{
					if(!ParseCallsignWithSsid(&cmd[13], len - 13, DigiConfig.alias[alno], &DigiConfig.ssid[alno - 4]))
					{
						err = true;
					}
				}
				if(err)
				{
					UartSendString(src, "Incorrect alias!\r\n", 0);
					return;
				}
				UartSendString(src, "OK\r\n", 0);
			}
			else if((!strncmp(&cmd[7], "max ", 4) || !strncmp(&cmd[7], "rep ", 4)) && (alno < 4))
			{
				int64_t t = StrToInt(&cmd[11], len - 11);
				if(!strncmp(&cmd[7], "max ", 4) && (t >= 1 || t <= 7))
				{
					DigiConfig.max[alno] = t;
				}
				else if(!strncmp(&cmd[7], "rep ", 4) && (t <= 7))
				{
					DigiConfig.rep[alno] = t;
				}
				else
				{
					UartSendString(src, "Incorrect value!\r\n", 0);
					return;
				}
			}
			else if(!strncmp(&cmd[7], "trac ", 5))
			{
				if(!strncmp(&cmd[12], "on", 2))
					DigiConfig.traced |= 1 << alno;
				else if(!strncmp(&cmd[12], "off", 3))
					DigiConfig.traced &= ~(1 << alno);
				else
				{
					err = true;
				}
			}
			else if(!strncmp(&cmd[7], "filter ", 7))
			{
				if(!strncmp(&cmd[14], "on", 2))
					DigiConfig.callFilterEnable |= 1 << alno;
				else if(!strncmp(&cmd[14], "off", 3))
					DigiConfig.callFilterEnable &= ~(1 << alno);
				else
				{
					err = true;
				}
			}
			else if(!strncmp(&cmd[7], "viscous ", 8))
			{
				if(!strncmp(&cmd[15], "on", 2))
				{
					DigiConfig.viscous |= (1 << alno);
					DigiConfig.directOnly &= ~(1 << alno); //disable directonly mode
				}
				else if(!strncmp(&cmd[15], "off", 3))
					DigiConfig.viscous &= ~(1 << alno);
				else
				{
					err = true;
				}
			}
			else if(!strncmp(&cmd[7], "direct ", 7))
			{
				if(!strncmp(&cmd[14], "on", 2))
				{
					DigiConfig.directOnly |= (1 << alno);
					DigiConfig.viscous &= ~(1 << alno); //disable viscous delay mode
				}
				else if(!strncmp(&cmd[14], "off", 3))
					DigiConfig.directOnly &= ~(1 << alno);
				else
				{
					err = true;
				}
			}
			else
				err = true;


		}
		else if(!strncmp(&cmd[5], "filter ", 7))
		{
			if(!strncmp(&cmd[12], "white", 5))
				DigiConfig.filterPolarity = 1;
			else if(!strncmp(&cmd[12], "black", 5))
				DigiConfig.filterPolarity = 0;
			else
				err = true;
		}
		else if(!strncmp(&cmd[5], "dupe ", 5))
		{
			int64_t t = StrToInt(&cmd[10], len - 10);
			if((t > 255) || (t < 5))
			{
				UartSendString(src, "Incorrect anti-dupe time!\r\n", 0);
				return;
			}
			else
				DigiConfig.dupeTime = (uint8_t)t;
		}
		else if(!strncmp(&cmd[5], "list ", 5))
		{
			uint16_t shift = 10;

			while(shift < len)
			{
				if(cmd[shift] == ' ')
					break;

				shift++;
			}
			int64_t number = StrToInt(&cmd[10], shift - 10);
			if((number < 0) || (number >= (sizeof(DigiConfig.callFilter) / sizeof(*(DigiConfig.callFilter)))))
			{
				 UartSendString(src, "Incorrect filter slot!\r\n", 0);
				 return;
			}
			shift++;
			if((len - shift) < 4)
			{
				 UartSendString(src, "Incorrect format!\r\n", 0);
				 return;
			}

			if(!strncmp(&cmd[shift], "set ", 4))
			{
				shift += 4;

				uint8_t tmp[7] = {0};
				uint16_t tmpIdx = 0;

				for(uint16_t i = 0; i < len; i++)
                {
                      if(cmd[shift + i] == '-') //SSID separator
                      {
                          if((cmd[shift + i + 1] == '*') || (cmd[shift + i + 1] == '?')) //and there is any wildcard
                              tmp[6] = 0xFF;
                          else if(!ParseSsid(&cmd[shift + i + 1], len - (shift + i + 1), &tmp[6])) //otherwise it is a normal SSID
                        	  err = true;
                          break;
                      }
                      else if(cmd[shift + i] == '?')
                      {
                          tmp[tmpIdx++] = 0xFF;
                      }
                      else if(cmd[shift + i] == '*')
                      {
                          while(tmpIdx < 6)
                          {
                              tmp[tmpIdx++] = 0xFF;
                          }
                          continue;
                      }
                      else if(IS_UPPERCASE_ALPHANUMERIC(cmd[shift + i]))
                    	  tmp[tmpIdx++] = cmd[shift + i];
                      else
                      {
                    	  err = true;
                    	  break;
                      }
                 }

                 if(!err)
                 {
                	 while(tmpIdx < 6) //fill with spaces
                	 {
                		 tmp[tmpIdx++] = ' ';
                 	 }

                	 memcpy(DigiConfig.callFilter[number], tmp, 7);
                 }
                 else
                 {
                	 UartSendString(src, "Incorrect format!\r\n", 0);
                	 return;
                 }
			}
			else if(!strncmp(&cmd[shift], "remove", 6))
			{
				memset(DigiConfig.callFilter[number], 0, 7);
			}
			else
				err = true;
		}
		else
			err = true;
	}
	else if(!strncmp(cmd, "pwm ", 4))
	{
		if(!strncmp(&cmd[4], "on", 2))
			ModemConfig.usePWM = 1;
		else if(!strncmp(&cmd[4], "off", 3))
			ModemConfig.usePWM = 0;
		else
			err = true;
	}
	else if(!strncmp(cmd, "flat ", 5))
	{
		if(!strncmp(&cmd[5], "on", 2))
			ModemConfig.flatAudioIn = 1;
		else if(!strncmp(&cmd[5], "off", 3))
			ModemConfig.flatAudioIn = 0;
		else
			err = true;
	}
	else if(!strncmp(cmd, "monkiss ", 8))
	{
		if(!strncmp(&cmd[8], "on", 2))
			GeneralConfig.kissMonitor = 1;
		else if(!strncmp(&cmd[8], "off", 3))
			GeneralConfig.kissMonitor = 0;
		else
			err = true;
	}
	else if(!strncmp(cmd, "nonaprs ", 8))
	{
		if(!strncmp(&cmd[8], "on", 2))
			Ax25Config.allowNonAprs = 1;
		else if(!strncmp(&cmd[8], "off", 2))
			Ax25Config.allowNonAprs = 0;
		else
			err = true;
	}
	else
		UartSendString(src, "Unknown command. For command list type \"help\"\r\n", 0);


	if(err)
		UartSendString(src, "Incorrect command\r\n", 0);
	else
		UartSendString(src, "OK\r\n", 0);
}
