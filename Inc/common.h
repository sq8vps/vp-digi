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
#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>


#define CRC32_INIT 0xFFFFFFFF

uint8_t call[6]; //device callsign
uint8_t callSsid; //device ssid

uint8_t dest[7]; //destination address for own beacons. Should be APNV01-0 for VP-Digi, but can be changed. SSID MUST remain 0.

const uint8_t *versionString; //version string

uint8_t autoReset;
uint32_t autoResetTimer;
uint8_t kissMonitor;

/**
 * @brief Generate random number from min to max
 * @param[in] min Lower boundary
 * @param[in] max Higher boundary
 * @return Generated number
 */
int16_t rando(int16_t min, int16_t max);

/**
 * @brief Convert string to int
 * @param[in] *str Input string
 * @param[in] len String length or 0 to detect by strlen()
 * @return Converted int
 */
int64_t strToInt(uint8_t *str, uint8_t len);

/**
 * @brief Convert AX25 frame to TNC2 (readable) format
 * @param[in] *from Input AX25 frame
 * @param[in] len Input frame length
 * @param[out] *to Destination buffer, will be NULL terminated
 */
void common_toTNC2(uint8_t *from, uint16_t fromlen, uint8_t *to);

/**
 * @brief Calculate CRC32
 * @param[in] crc0 Initial or current CRC value
 * @param[in] *s Input data
 * @param[in] n Input data length
 * @return Calculated CRC32
 */
uint32_t crc32(uint32_t crc0, uint8_t *s, uint64_t n);

/**
 * @brief Send frame to available UARTs and USB in KISS format
 * @param[in] *buf Frame buffer
 * @param[in] len Frame buffer length
 */
void SendKiss(uint8_t *buf, uint16_t len);

#endif /* COMMON_H_ */
