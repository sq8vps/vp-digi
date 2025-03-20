/*
Copyright 2020-2025 Piotr Wilkon

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


struct DigiConfig
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
};

extern struct DigiConfig DigiConfig; //digipeater state


/**
 * @brief Digipeater entry point
 * @param[in] *frame Pointer to frame buffer
 * @param[in] len Frame length
 * Decides whether the frame should be digipeated or not, processes it and pushes to TX buffer if needed
 */
void DigiDigipeat(uint8_t *frame, uint16_t len);

/**
 * @brief Store duplicate protection hash for frame
 * @param *buf Frame buffer
 * @param size Frame size
 */
void DigiStoreDeDupe(uint8_t *buf, uint16_t size);

/**
 * @brief Initialize digipeater
 */
void DigiInitialize(void);

/**
 * @brief Update internal digipeater state
 * @attention Should be called in main loop
 */
void DigiUpdateState(void);

#endif /* DIGIPEATER_H_ */
