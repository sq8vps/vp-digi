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

#ifndef AX25_H_
#define AX25_H_

#define FRAMELEN (150) //single frame max length
#define FRAMEBUFLEN (600) //circural frame buffer (multiple frames) length



#include <stdint.h>


typedef enum
{
	RX_STAGE_IDLE,
	RX_STAGE_FLAG,
	RX_STAGE_FRAME,
}  RxStage;

typedef struct
{
	uint16_t txDelayLength; //TXDelay length in ms
	uint16_t txTailLength; //TXTail length in ms
	uint16_t quietTime; //Quiet time in ms
	uint8_t allowNonAprs; //allow non-APRS packets

} Ax25_config;

Ax25_config ax25Cfg;


typedef struct
{
	uint8_t frameBuf[FRAMEBUFLEN]; //cirucal buffer for received frames, frames are separated with 0xFF
	uint16_t frameBufWr; //cirucal RX buffer write index
	uint16_t frameBufRd; //circural TX buffer read index
	uint8_t frameXmit[FRAMEBUFLEN];  //TX frame buffer
	uint16_t xmitIdx; //TX frame buffer index
	uint16_t sLvl; //RMS of the frame
	uint8_t frameReceived; //frame received flag, must be polled in main loop for >0. Bit 0 for frame received on decoder 1, bit 1 for decoder 2

} Ax25;

Ax25 ax25;

/**
 * @brief Get current RX stage
 * @param[in] modemNo Modem/decoder number (0 or 1)
 * @return RX_STATE_IDLE, RX_STATE_FLAG or RX_STATE_FRAME
 * @warning Only for internal use
 */
RxStage Ax25_getRxStage(uint8_t modemNo);

/**
 * @brief Parse incoming bit (not symbol!)
 * @details Handles bit-stuffing, header and CRC checking, stores received frame and sets "frame received flag", multiplexes both decoders
 * @param[in] bit Incoming bit
 * @param[in] *dem Modem state pointer
 * @warning Only for internal use
 */
void Ax25_bitParse(uint8_t bit, uint8_t modemNo);

/**
 * @brief Get next bit to be transmitted
 * @return Bit to be transmitted
 * @warning Only for internal use
 */
uint8_t Ax25_getTxBit(void);

/**
 * @brief Initialize transmission and start when possible
 */
void Ax25_transmitBuffer(void);

/**
 * @brief Start transmitting when possible
 * @attention Must be continuously polled in main loop
 */
void Ax25_transmitCheck(void);

/**
 * @brief Initialize AX25 module
 */
void Ax25_init(void);

#endif /* AX25_H_ */
