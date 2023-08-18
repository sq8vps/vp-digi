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

#include <stdint.h>
#include <stdbool.h>




enum Ax25RxStage
{
	RX_STAGE_IDLE = 0,
	RX_STAGE_FLAG,
	RX_STAGE_FRAME,
};

struct Ax25ProtoConfig
{
	uint16_t txDelayLength; //TXDelay length in ms
	uint16_t txTailLength; //TXTail length in ms
	uint16_t quietTime; //Quiet time in ms
	uint8_t allowNonAprs; //allow non-APRS packets
};

extern struct Ax25ProtoConfig Ax25Config;

/**
 * @brief Transmit one or more frames encoded in KISS format
 * @param *buf Inout buffer
 * @param len Buffer size
 */
void Ax25TxKiss(uint8_t *buf, uint16_t len);

/**
 * @brief Write frame to transmit buffer
 * @param *data Data to transmit
 * @param size Data size
 * @return Pointer to internal frame handle or NULL on failure
 * @attention This function will block if transmission is already in progress
 */
void *Ax25WriteTxFrame(uint8_t *data, uint16_t size);

/**
 * @brief Get bitmap of "frame received" flags for each decoder. A non-zero value means that a frame was received
 * @return Bitmap of decoder that received the frame
 */
uint8_t Ax25GetReceivedFrameBitmap(void);

/**
 * @brief Clear bitmap of "frame received" flags
 */
void Ax25ClearReceivedFrameBitmap(void);

/**
 * @brief Get next received frame (if available)
 * @param **dst Pointer to internal buffer
 * @param *size Actual frame size
 * @param *signalLevel Frame signal level (RMS)
 * @return True if frame was read, false if no more frames to read
 */
bool Ax25ReadNextRxFrame(uint8_t **dst, uint16_t *size, uint16_t *signalLevel);

/**
 * @brief Get current RX stage
 * @param[in] modemNo Modem/decoder number (0 or 1)
 * @return RX_STATE_IDLE, RX_STATE_FLAG or RX_STATE_FRAME
 * @warning Only for internal use
 */
enum Ax25RxStage Ax25GetRxStage(uint8_t modemNo);

/**
 * @brief Parse incoming bit (not symbol!)
 * @details Handles bit-stuffing, header and CRC checking, stores received frame and sets "frame received flag", multiplexes both decoders
 * @param[in] bit Incoming bit
 * @param[in] *dem Modem state pointer
 * @warning Only for internal use
 */
void Ax25BitParse(uint8_t bit, uint8_t modemNo);

/**
 * @brief Get next bit to be transmitted
 * @return Bit to be transmitted
 * @warning Only for internal use
 */
uint8_t Ax25GetTxBit(void);

/**
 * @brief Initialize transmission and start when possible
 */
void Ax25TransmitBuffer(void);

/**
 * @brief Start transmitting when possible
 * @attention Must be continuously polled in main loop
 */
void Ax25TransmitCheck(void);

/**
 * @brief Initialize AX25 module
 */
void Ax25Init(void);

#endif /* AX25_H_ */
