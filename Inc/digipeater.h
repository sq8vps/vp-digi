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

#ifndef DIGIPEATER_H_
#define DIGIPEATER_H_

#include <stdint.h>




typedef struct
{
	uint8_t alias[8][6]; //digi alias list
	uint8_t ssid[4]; //ssid list for simple aliases
	uint8_t max[4]; //max n to digipeat
	uint8_t rep[4]; //min n to replace
	uint8_t traced; //tracing for each alias, 0 - untraced, 1 - traced
	uint8_t enableAlias; //enabling each alias, 0 - disabled, 1 - enabled
	uint8_t enable : 1; //enable whole digi
	uint8_t viscous; //visous-delay enable for each alias
	uint8_t directOnly; //direct-only enable for each alias
	uint8_t dupeTime; //duplicate filtering time in seconds
	uint8_t callFilter[20][7]; //callsign filter array
	uint8_t callFilterEnable; //enable filter by call for every alias
	uint8_t filterPolarity : 1; //filter polarity: 0 - blacklist, 1- whitelist
} Digi;

Digi digi; //digipeater state



/**
 * @brief Digipeater entry point
 * @param[in] *frame Pointer to frame buffer
 * @param[in] len Frame length
 * Decides whether the frame should be digipeated or not, processes it and pushes to TX buffer if needed
 */
void Digi_digipeat(uint8_t *frame, uint16_t len);

/**
 * @brief Store duplicate protection hash for the frame already pushed to TX buffer
 * @param[in] idx First frame byte index in TX buffer
 */
void Digi_storeDeDupeFromXmitBuf(uint16_t idx);

/**
 * @brief Refresh viscous-delay buffers and push frames to TX buffer if neccessary
 * @attention Should be called constantly
 */
void Digi_viscousRefresh(void);

#endif /* DIGIPEATER_H_ */
