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

#include "modem.h"
#include "terminal.h"
#include "common.h"
#include "beacon.h"
#include "digipeater.h"
#include "config.h"
#include "ax25.h"
#include "systick.h"
#include "kiss.h"

void TermHandleSpecial(Uart *u)
{
	if(u->mode == MODE_KISS) //don't do anything in KISS mode
	{
		u->lastRxBufferHead = 0;
		return;
	}
	if(u->lastRxBufferHead >= u->rxBufferHead) //UART RX buffer index was probably cleared
		u->lastRxBufferHead = 0;

	if(u->rxBuffer[u->rxBufferHead - 1] == '\b') //user entered backspace
	{
		if(u->rxBufferHead > 1) //there was some data in buffer
		{
			u->rxBufferHead -= 2; //remove backspace and preceding character
			UartSendString(u, "\b \b", 3); //backspace (one character left), remove backspaced character (send space) and backspace again
			if(u->lastRxBufferHead > 0)
				u->lastRxBufferHead--; //1 character was removed
		}
		else //no preceding character
			u->rxBufferHead = 0;
	}
	uint16_t t = u->rxBufferHead; //store last index
	if(u->lastRxBufferHead < t) //local echo handling
	{
		UartSendString(u, (uint8_t*)&u->rxBuffer[u->lastRxBufferHead], t - u->lastRxBufferHead); //echo characters entered by user
		if((u->rxBuffer[t - 1] == '\r') || (u->rxBuffer[t - 1] == '\n'))
			UartSendString(u, "\r\n", 2);
		u->lastRxBufferHead = t;
	}

}


void TermSendToAll(enum UartMode mode, uint8_t *data, uint16_t size)
{
	if(MODE_KISS == mode)
	{
		KissSend(&Uart1, data, size);
		KissSend(&Uart2, data, size);
		KissSend(&UartUsb, data, size);
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

static const char monitorHelp[] = "\r\nCommands available in monitor mode:\r\n"
		"help - show this help page\r\n"
		"cal {low|high|alt|stop} - transmit/stop transmitter calibration pattern\r\n"
		"\tlow - transmit MARK tone, high - transmit SPACE tone, alt - transmit alternating tones (null bytes)\r\n"
		"beacon <beacon_number> - immediately transmit selected beacon (number from 0 to 7)\r\n"
		"kiss - switch to KISS mode\r\n"
		"config - switch to config mode\r\n"
		"reboot - reboot the device\r\n"
		"time - show time since boot\r\n"
		"version - show full firmware version info\r\n\r\n\r\n";

static const char configHelp[] = 	"\r\nCommands available in config mode:\r\n"
		"print - print all configuration parameters\r\n"
		"list - print callsign filter list\r\n"
		"save - save configuration and reboot the device\r\n"
		"eraseall - erase all configurations and reboot the device\r\n"
		"help - show this help page\r\n"
		"reboot - reboot the device\r\n"
		"time - show time since boot\r\n"
		"version - show full firmware version info\r\n\r\n"
		"modem <type> - set modem type: 1200, 1200_V23, 300 or 9600\r\n"
		"call <callsign-SSID> - set callsign with optional SSID\r\n"
		"dest <address> - set destination address\r\n"
		"txdelay <50-2550> - set TXDelay time (ms)\r\n"
		"txtail <10-2550> - set TXTail time (ms)\r\n"
		"quiet <100-2550> - set quiet time (ms)\r\n"
		"uart <1/2> baud <1200-115200> - set UART baud rate\r\n"
		"uart <0/1/2> mode [kiss/monitor/config] - set UART default mode (0 for USB)\r\n"
		"pwm [on/off] - enable/disable PWM. If PWM is off, R2R will be used instead\r\n"
		"flat [on/off] - set to \"on\" if flat audio input is used\r\n"
		"beacon <0-7> [on/off] - enable/disable specified beacon\r\n"
		"beacon <0-7> [iv/dl] <0-720> - set interval/delay for the specified beacon (min)\r\n"
		"beacon <0-7> path <el1,[el2]>/none - set path for the specified beacon\r\n"
		"beacon <0-7> data <data> - set information field for the specified beacon\r\n"
		"digi [on/off] - enable/disable whole digipeater\r\n"
		"digi <0-7> [on/off] - enable/disable specified slot\r\n"
		"digi <0-7> alias <alias> - set alias for the specified slot\r\n"
		"digi <0-3> [max/rep] <0/1-7> - set maximum/replacement N for the specified slot\r\n"
		"digi <0-7> trac [on/off] - enable/disable packet tracing for the specified slot\r\n"
		"digi <0-7> viscous [on/off] - enable/disable viscous-delay digipeating for the specified slot\r\n"
		"digi <0-7> direct [on/off] - enable/disable direct-only digipeating for the specified slot\r\n"\
		"digi <0-7> filter [on/off] - enable/disable packet filtering for the specified slot\r\n"
		"digi filter [black/white] - set filter type to blacklist/whitelist\r\n"
		"digi dupe <5-255> - set duplicate protection buffer time (s)\r\n"
		"digi list <0-19> [set <call>/remove] - set/clear given callsign slot in filter list\r\n"
		"monkiss [on/off] - send own and digipeated frames to KISS ports\r\n"
		"nonaprs [on/off] - enable reception of non-APRS frames\r\n"
		"fx25 [on/off] - enable FX.25 protocol (AX.25 + FEC)\r\n"
		"fx25tx [on/off] - enable TX in FX.25 mode\r\n";


static void sendUartParams(Uart *output, Uart *uart)
{
	if(!uart->isUsb)
	{
		UartSendNumber(output, uart->baudrate);
		UartSendString(output, " baud, ", 0);
	}
	UartSendString(output, "default mode: ", 0);
	switch(uart->defaultMode)
	{
		case MODE_KISS:
			UartSendString(output, "KISS", 0);
			break;
		case MODE_MONITOR:
			UartSendString(output, "monitor", 0);
			break;
		case MODE_TERM:
			UartSendString(output, "configuration", 0);
			break;
	}
}

static void printConfig(Uart *src)
{
	UartSendString(src, "Modem: ", 0);
	switch(ModemConfig.modem)
	{
		case MODEM_1200:
			UartSendString(src, "AFSK Bell 202 1200 Bd 1200/2200 Hz", 0);
			break;
		case MODEM_1200_V23:
			UartSendString(src, "AFSK V.23 1200 Bd 1300/2100 Hz", 0);
			break;
		case MODEM_300:
			UartSendString(src, "AFSK Bell 103 300 Bd 1600/1800 Hz", 0);
			break;
		case MODEM_9600:
			UartSendString(src, "GFSK G3RUH 9600 Bd", 0);
			break;
	}
	UartSendString(src, "\r\nCallsign: ", 0);
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
	UartSendString(src, "\r\nUSB: ", 0);
	sendUartParams(src, &UartUsb);
	UartSendString(src, "\r\nUART1: ", 0);
	sendUartParams(src, &Uart1);
	UartSendString(src, "\r\nUART2: ", 0);
	sendUartParams(src, &Uart2);
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
	UartSendString(src, "FX.25 protocol: ", 0);
	if(Ax25Config.fx25 == 1)
		UartSendString(src, "On\r\n", 0);
	else
		UartSendString(src, "Off\r\n", 0);
	UartSendString(src, "FX.25 TX: ", 0);
	if(Ax25Config.fx25Tx == 1)
		UartSendString(src, "On\r\n", 0);
	else
		UartSendString(src, "Off\r\n", 0);
}

static void sendTime(Uart *src)
{
	UartSendString(src, "Time since boot: ", 0);
	UartSendNumber(src, SysTickGet() * SYSTICK_INTERVAL / 60000); //convert from ms to minutes
	UartSendString(src, " minutes\r\n", 0);
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
				"Some settings require the device to be rebooted\r\n"
				"in order to behave correctly\r\n"
				"Always use \"save\" to save and reboot\r\n", 0);
		src->mode = MODE_TERM;
		return;
	}
	else if(!strncmp(cmd, "monitor", 7))
	{
		UartSendString(src, (uint8_t*)"Switched to monitor mode\r\n", 0);
		src->mode = MODE_MONITOR;
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
			return;
		}
		else if(!strncmp(cmd, "version", 7))
		{
			UartSendString(src, (char *)versionString, 0);
			return;
		}
		else if(!strncmp(cmd, "reboot", 6))
		{
			NVIC_SystemReset();
		}
		else if(!strncmp(cmd, "time", 4))
		{
			sendTime(src);
			return;
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
		return;
	}
	else if(!strncmp(cmd, "version", 7))
	{
		UartSendString(src, (uint8_t*)versionString, 0);
		return;
	}
	else if(!strncmp(cmd, "time", 4))
	{
		sendTime(src);
		return;
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
		return;
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
                    uint8_t clIdx = 0;
                    for(uint8_t j = 0; j < 6; j++)
                    {
                        if(DigiConfig.callFilter[i][j] == 0xFF) //wildcard
                        {
                            bool partialWildcard = false;
                            for(uint8_t k = j; k < 6; k++) //check if there are all characters wildcarded
                            {
                                if(DigiConfig.callFilter[i][k] != 0xFF)
                                {
                                	partialWildcard = true;
                                	break;
                                }
                            }
                            if(partialWildcard == false) //all characters wildcarded
                            {
                                cl[clIdx++] = '*';
                                break;
                            }
                            else
                            {
                            	cl[clIdx++] = '?';
                            }
                        }
                        else
                        {
                            if(DigiConfig.callFilter[i][j] != ' ')
                            	cl[clIdx++] = DigiConfig.callFilter[i][j];
                        }
                    }
                    if(DigiConfig.callFilter[i][6] == 0xFF) //wildcard on SSID
                    {
                    	cl[clIdx++] = '-';
                    	cl[clIdx++] = '*';
                    }
                    else if(DigiConfig.callFilter[i][6] != 0)
                    {
                    	cl[clIdx++] = '-';
                        if(DigiConfig.callFilter[i][6] > 9)
                        {
                        	cl[clIdx++] = (DigiConfig.callFilter[i][6] / 10) + '0';
                        	cl[clIdx++] = (DigiConfig.callFilter[i][6] % 10) + '0';
                        }
                        else
                        	cl[clIdx++] = DigiConfig.callFilter[i][6] + '0';
                    }
                    UartSendString(src, cl, 0);
			}
			else
				UartSendString(src, "empty", 0);

			UartSendString(src, "\r\n", 0);
		}
		return;
	}
	/*
	 * Settings insertion handling
	 */
	else if(!strncmp(cmd, "modem", 5))
	{
		if(!strncmp(&cmd[6], "1200_V23", 8))
			ModemConfig.modem = MODEM_1200_V23;
		else if(!strncmp(&cmd[6], "1200", 4))
			ModemConfig.modem = MODEM_1200;
		else if(!strncmp(&cmd[6], "300", 3))
			ModemConfig.modem = MODEM_300;
		else if(!strncmp(&cmd[6], "9600", 4))
			ModemConfig.modem = MODEM_9600;
		else
		{
			UartSendString(src, "Incorrect modem type!\r\n", 0);
			return;
		}
	}
	else if(!strncmp(cmd, "call", 4))
	{
		if(!ParseCallsignWithSsid(&cmd[5], len - 5, GeneralConfig.call, &GeneralConfig.callSsid))
		{
			UartSendString(src, "Incorrect callsign!\r\n", 0);
			return;
		}
	}
	else if(!strncmp(cmd, "dest", 4))
	{
		if(!ParseCallsign(&cmd[5], len - 5, GeneralConfig.dest))
		{
			UartSendString(src, "Incorrect address!\r\n", 0);
			return;
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
	else if(!strncmp(cmd, "uart", 4))
	{
		Uart *u = NULL;
		if((cmd[5] - '0') == 1)
			u = &Uart1;
		else if((cmd[5] - '0') == 2)
			u = &Uart2;
		else if((cmd[5] - '0') == 0)
			u = &UartUsb;
		else
		{
			UartSendString(src, "Incorrect UART number!\r\n", 0);
			return;
		}
		if(!strncmp(&cmd[7], "baud", 4))
		{
			int64_t t = StrToInt(&cmd[12], len - 12);
			if((t > 115200) || (t < 1200))
			{
				UartSendString(src, "Incorrect baud rate!\r\n", 0);
				return;
			}
			u->baudrate = (uint32_t)t;
		}
		else if(!strncmp(&cmd[7], "mode", 4))
		{
			if(!strncmp(&cmd[12], "kiss", 4))
			{
				u->defaultMode = MODE_KISS;
			}
			else if(!strncmp(&cmd[12], "monitor", 7))
			{
				u->defaultMode = MODE_MONITOR;
			}
			else if(!strncmp(&cmd[12], "config", 6))
			{
				u->defaultMode = MODE_TERM;
			}
			else
			{
				UartSendString(src, "Incorrect UART mode!\r\n", 0);
				return;
			}
		}
		else
		{
			UartSendString(src, "Incorrect option!\r\n", 0);
			return;
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

				for(uint16_t i = 0; i < (len - shift); i++)
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
	else if(!strncmp(cmd, "fx25 ", 5))
	{
		if(!strncmp(&cmd[5], "on", 2))
			Ax25Config.fx25 = 1;
		else if(!strncmp(&cmd[5], "off", 2))
			Ax25Config.fx25 = 0;
		else
			err = true;
	}
	else if(!strncmp(cmd, "fx25tx ", 7))
	{
		if(!strncmp(&cmd[7], "on", 2))
			Ax25Config.fx25Tx = 1;
		else if(!strncmp(&cmd[7], "off", 2))
			Ax25Config.fx25Tx = 0;
		else
			err = true;
	}
	else
	{
		UartSendString(src, "Unknown command. For command list type \"help\"\r\n", 0);
		return;
	}


	if(err)
		UartSendString(src, "Incorrect command\r\n", 0);
	else
		UartSendString(src, "OK\r\n", 0);
}
