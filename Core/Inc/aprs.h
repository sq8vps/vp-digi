/*
Copyright 2020-2026 Piotr Wilkon

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


#ifndef APRS_H_
#define APRS_H_

#define AX25_FLAG 0x7E /**< AX.25 flag */

#define APRS_CONTROL 0x03 /**< AX.25 control field value for APRS */
#define APRS_PID 0xF0 /**< AX.25 PID field value for APRS */

#define SSID_BITS 0b01100000 /**< Reserved SSID bits */
#define H_BIT 0x80 /**< H-bit - has been repeated */
#define C_BIT 0x01 /**< C-bit - end of path */
#define EXTRACT_SSID(b) (((b) >> 1) & 0xF) /**< Extract SSID from raw byte */
#define ENCODE_SSID(ssid) ((((ssid) & 0xF) << 1) | SSID_BITS) /**< Encode SSID to raw byte */


#endif
