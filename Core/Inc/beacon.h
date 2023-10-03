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

#ifndef BEACON_H_
#define BEACON_H_


#include <stdint.h>

#define BEACON_MAX_PAYLOAD_SIZE 100

struct Beacon
{
	uint8_t enable; //enable beacon
	uint32_t interval; //interval in seconds
	uint32_t delay; //delay in seconds
	uint8_t data[BEACON_MAX_PAYLOAD_SIZE + 1]; //information field
	uint8_t path[15]; //path, 2 parts max, e.g. WIDE1<sp>1SP2<sp><sp><sp>2<NUL>, <NUL> can be at byte 0, 7 and 14
	uint32_t next; //next beacon timestamp
};

extern struct Beacon beacon[8];

/**
 * @brief Send specified beacon
 * @param number Beacon number (0-7)
 */
void BeaconSend(uint8_t number);


/**
 * @brief Check if any beacon should be transmitted and transmit if necessary
 */
void BeaconCheck(void);

/**
 * @brief Initialize beacon module
 */
void BeaconInit(void);

#endif /* BEACON_H_ */
