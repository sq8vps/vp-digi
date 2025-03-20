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

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include "uart.h"

#if defined(STM32F103xB) || defined(STM32F103x8)
#include "stm32f1xx_hal.h"
#elif defined(STM32F302xC)
#include "stm32f3xx_hal.h"
#endif

#define VPDIGI_HAL_TICK_FREQUENCY HAL_TICK_FREQ_100HZ /**< Systick frequency for HAL. Use this to change systick frequency */

#if VPDIGI_HAL_TICK_FREQUENCY == HAL_TICK_FREQ_100HZ
#define SYSTICK_INTERVAL (10)
#elif VPDIGI_HAL_TICK_FREQUENCY == HAL_TICK_FREQ_10HZ
#define SYSTICK_INERVAL (100)
#elif VPDIGI_HAL_TICK_FREQUENCY == HAL_TICK_FREQ_1KHZ
#define SYSTICK_INTERVAL (1)
#else
#error Wrong systick frequency!
#endif

#define SYSTICK_FREQUENCY (1000 / SYSTICK_INTERVAL)

#define IS_UPPERCASE_ALPHANUMERIC(x) ((((x) >= '0') && ((x) <= '9')) || (((x) >= 'A') && ((x) <= 'Z')))
#define IS_NUMBER(x) (((x) >= '0') && ((x) <= '9'))
#define ABS(x) (((x) > 0) ? (x) : (-x))

#define CRC32_INIT 0xFFFFFFFF

struct GeneralConfig
{
	uint8_t call[6]; //device callsign
	uint8_t callSsid; //device ssid
	uint8_t dest[7]; //destination address for own beacons. Should be APNV01-0 for Blue Pill VP-Digi and APNV02-0 for AIOC VP-Digi, but can be changed. SSID MUST remain 0.
	uint8_t kissMonitor;
};

extern struct GeneralConfig GeneralConfig;

extern const char versionString[]; //version string


/**
 * @brief Generate random number from min to max
 * @param[in] min Lower boundary
 * @param[in] max Higher boundary
 * @return Generated number
 */
int16_t Random(int16_t min, int16_t max);

/**
 * @brief Convert string to int
 * @param[in] *str Input string
 * @param[in] len String length or 0 to detect by strlen()
 * @return Converted int
 */
int64_t StrToInt(const char *str, uint16_t len);

///**
// * @brief Convert AX25 frame to TNC2 (readable) format
// * @param *from Input AX25 frame
// * @param len Input frame length
// * @param *to Destination buffer, will be NULL terminated
// * @param limit Destination buffer size limit
// */
//void ConvertToTNC2(uint8_t *from, uint16_t fromlen, uint8_t *to, uint16_t limit);

/**
 * @brief Convert AX25 frame to TNC2 (readable) format and send it through available ports
 * @param *from Input AX25 frame
 * @param len Input frame length
 */
void SendTNC2(uint8_t *from, uint16_t len);

/**
 * @brief Calculate CRC32
 * @param[in] crc0 Initial or current CRC value
 * @param[in] *s Input data
 * @param[in] n Input data length
 * @return Calculated CRC32
 */
uint32_t Crc32(uint32_t crc0, uint8_t *s, uint64_t n);

/**
 * @brief Check if callsign is correct and convert it to AX.25 format
 * @param *in Input ASCII callsign
 * @param size Input size, not bigger than 6
 * @param *out Output buffer, exactly 6 bytes
 * @return True if callsign is valid
 */
bool ParseCallsign(const char *in, uint16_t size, uint8_t *out);

/**
 * @brief Check if callsign with SSID is correct and convert it to AX.25 format
 * @param *in Input ASCII callsign with SSID
 * @param size Input size
 * @param *out Output buffer, exactly 6 bytes
 * @param *ssid Output SSID, exactly 1 byte
 * @return True if callsign is valid
 */
bool ParseCallsignWithSsid(const char *in, uint16_t size, uint8_t *out, uint8_t *ssid);

/**
 * @brief Check if SSID is correct and convert it to uint8_t
 * @param *in Input ASCII SSID
 * @param size Input size
 * @param *out Output buffer, exactly 1 byte
 * @return True if SSID is valid
 */
bool ParseSsid(const char *in, uint16_t size, uint8_t *out);

#endif /* COMMON_H_ */
