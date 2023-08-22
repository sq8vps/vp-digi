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

#include "ax25.h"
#include <stdlib.h>
#include "drivers/modem.h"
#include "common.h"
#include "drivers/systick.h"
#include <stdbool.h>
#include "digipeater.h"

struct Ax25ProtoConfig Ax25Config;

//values below must be kept consistent so that FRAME_BUFFER_SIZE >= FRAME_MAX_SIZE * FRAME_MAX_COUNT
#define FRAME_MAX_SIZE (308) //single frame max length for RX
//308 bytes is the theoretical max size assuming 2-byte Control, 256-byte info field and 5 digi address fields
#define FRAME_MAX_COUNT (10) //max count of frames in buffer
#define FRAME_BUFFER_SIZE (FRAME_MAX_COUNT * FRAME_MAX_SIZE) //circular frame buffer length

#define STATIC_HEADER_FLAG_COUNT 4 //number of flags sent before each frame
#define STATIC_FOOTER_FLAG_COUNT 8 //number of flags sent after each frame

#define MAX_TRANSMIT_RETRY_COUNT 8 //max number of retries if channel is busy

struct FrameHandle
{
	uint16_t start;
	uint16_t size;
	uint16_t signalLevel;
};

static uint8_t rxBuffer[FRAME_BUFFER_SIZE]; //circular buffer for received frames
static uint16_t rxBufferHead = 0; //circular RX buffer write index
static struct FrameHandle rxFrame[FRAME_MAX_COUNT];
static uint8_t rxFrameHead = 0;
static uint8_t rxFrameTail = 0;
static bool rxFrameBufferFull = false;

static uint8_t txBuffer[FRAME_BUFFER_SIZE];  //circular TX frame buffer
static uint16_t txBufferHead = 0; //circular TX buffer write index
static uint16_t txBufferTail = 0;
static struct FrameHandle txFrame[FRAME_MAX_COUNT];
static uint8_t txFrameHead = 0;
static uint8_t txFrameTail = 0;
static bool txFrameBufferFull = false;

static uint8_t frameReceived; //a bitmap of receivers that received the frame


enum TxStage
{
	TX_STAGE_IDLE,
	TX_STAGE_PREAMBLE,
	TX_STAGE_HEADER_FLAGS,
	TX_STAGE_DATA,
	TX_STAGE_CRC,
	TX_STAGE_FOOTER_FLAGS,
	TX_STAGE_TAIL,
};

enum TxInitStage
{
	TX_INIT_OFF,
	TX_INIT_WAITING,
	TX_INIT_TRANSMITTING
};

static uint8_t txByte = 0; //current TX byte
static uint16_t txByteIdx = 0; //current TX byte index
static int8_t txBitIdx = 0; //current bit index in txByte
static uint16_t txDelayElapsed = 0; //counter of TXDelay bytes already sent
static uint8_t txFlagsElapsed = 0; //counter of flag bytes already sent
static uint8_t txCrcByteIdx = 0; //currently transmitted byte of CRC
static uint8_t txBitstuff = 0; //bit-stuffing counter
static uint16_t txTailElapsed; //counter of TXTail bytes already sent
static uint16_t txCrc = 0xFFFF; //current CRC
static uint32_t txQuiet = 0; //quit time + current tick value
static uint8_t txRetries = 0; //number of TX retries
static enum TxInitStage txInitStage; //current TX initialization stage
static enum TxStage txStage; //current TX stage

struct RxState
{
	uint16_t crc; //current CRC
	uint8_t frame[FRAME_MAX_SIZE]; //raw frame buffer
	uint16_t frameIdx; //index for raw frame buffer
	uint8_t receivedByte; //byte being currently received
	uint8_t receivedBitIdx; //bit index for recByte
	uint8_t rawData; //raw data being currently received
	enum Ax25RxStage rx; //current RX stage
	uint8_t frameReceived; //frame received flag
};

static volatile struct RxState rxState[MODEM_MAX_DEMODULATOR_COUNT];

static uint16_t lastCrc = 0; //CRC of the last received frame. If not 0, a frame was successfully received
static uint16_t rxMultiplexDelay = 0; //simple delay for decoder multiplexer to avoid receiving the same frame twice

static uint16_t txDelay; //number of TXDelay bytes to send
static uint16_t txTail; //number of TXTail bytes to send

static uint8_t outputFrameBuffer[FRAME_MAX_SIZE];

#define GET_FREE_SIZE(max, head, tail) (((head) < (tail)) ? ((tail) - (head)) : ((max) - (head) + (tail)))
#define GET_USED_SIZE(max, head, tail) (max - GET_FREE_SIZE(max, head, tail))

/**
 * @brief Recalculate CRC for one bit
 * @param bit Input bit
 * @param *crc CRC pointer
 */
static void calculateCRC(uint8_t bit, uint16_t *crc)
{
    uint16_t xor_result;
    xor_result = *crc ^ bit;
    *crc >>= 1;
    if (xor_result & 0x0001)
    {
    	*crc ^= 0x8408;
    }
}

uint8_t Ax25GetReceivedFrameBitmap(void)
{
	return frameReceived;
}

void Ax25ClearReceivedFrameBitmap(void)
{
	frameReceived = 0;
}

void Ax25TxKiss(uint8_t *buf, uint16_t len)
{
	if(len < 18) //frame is too small
	{
		return;
	}
	for(uint16_t i = 0; i < len; i++)
	{
		if(buf[i] == 0xC0) //frame start marker
		{
			uint16_t end = i + 1;
			while(end < len)
			{
				if(buf[end] == 0xC0)
					break;
				end++;
			}
			if(end == len) //no frame end marker found
				return;
			Ax25WriteTxFrame(&buf[i + 2], end - (i + 2)); //skip modem number and send frame
			DigiStoreDeDupe(&buf[i + 2], end - (i + 2));
			i = end; //move pointer to the next byte if there are more consecutive frames
		}
	}
}

void *Ax25WriteTxFrame(uint8_t *data, uint16_t size)
{
	while(txStage != TX_STAGE_IDLE)
		;

	if((GET_FREE_SIZE(FRAME_BUFFER_SIZE, txBufferHead, txBufferTail) < size) || txFrameBufferFull)
	{
		return NULL;
	}


	txFrame[txFrameHead].size = size;
	txFrame[txFrameHead].start = txBufferHead;
	for(uint16_t i = 0; i < size; i++)
	{
		txBuffer[txBufferHead++] = data[i];
		txBufferHead %= FRAME_BUFFER_SIZE;
	}
	void *ret = &txFrame[txFrameHead];
	txFrameHead++;
	txFrameHead %= FRAME_MAX_COUNT;
	if(txFrameHead == txFrameTail)
		txFrameBufferFull = true;
	return ret;
}


bool Ax25ReadNextRxFrame(uint8_t **dst, uint16_t *size, uint16_t *signalLevel)
{
	if((rxFrameHead == rxFrameTail) && !rxFrameBufferFull)
		return false;

	*dst = outputFrameBuffer;

	for(uint16_t i = 0; i < rxFrame[rxFrameTail].size; i++)
	{
		(*dst)[i] = rxBuffer[(rxFrame[rxFrameTail].start + i) % FRAME_BUFFER_SIZE];
	}

	*signalLevel = rxFrame[rxFrameTail].signalLevel;
	*size = rxFrame[rxFrameTail].size;

	rxFrameBufferFull = false;
	rxFrameTail++;
	rxFrameTail %= FRAME_MAX_COUNT;
	return true;
}

enum Ax25RxStage Ax25GetRxStage(uint8_t modem)
{
	return rxState[modem].rx;
}


void Ax25BitParse(uint8_t bit, uint8_t modem)
{
	if(lastCrc != 0) //there was a frame received
	{
		rxMultiplexDelay++;
		if(rxMultiplexDelay > (4 * MODEM_MAX_DEMODULATOR_COUNT)) //hold it for a while and wait for other decoders to receive the frame
		{
			lastCrc = 0;
			rxMultiplexDelay = 0;
			for(uint8_t i = 0; i < MODEM_MAX_DEMODULATOR_COUNT; i++)
			{
				frameReceived |= ((rxState[i].frameReceived > 0) << i);
				rxState[i].frameReceived = 0;
			}
		}

	}


	struct RxState *rx = (struct RxState*)&(rxState[modem]);

	rx->rawData <<= 1; //store incoming bit
	rx->rawData |= (bit > 0);


	if(rx->rawData == 0x7E) //HDLC flag received
	{
		if(rx->rx == RX_STAGE_FRAME) //if we are in frame, this is the end of the frame
		{
    		if((rx->frameIdx > 15)) //correct frame must be at least 16 bytes long
    		{
        		uint16_t i = 0;
				for(; i < rx->frameIdx - 2; i++) //look for path end bit
        		{
        			if(rx->frame[i] & 1)
        				break;
        		}

				//if non-APRS frames are not allowed, check if this frame has control=0x03 and PID=0xF0

				if(Ax25Config.allowNonAprs || (((rx->frame[i + 1] == 0x03) && (rx->frame[i + 2] == 0xf0))))
				{
					if((rx->frame[rx->frameIdx - 2] == ((rx->crc & 0xFF) ^ 0xFF)) && (rx->frame[rx->frameIdx - 1] == (((rx->crc >> 8) & 0xFF) ^ 0xFF))) //check CRC
					{
						rx->frameReceived = 1;
						rx->frameIdx -= 2; //remove CRC
						if(rx->crc != lastCrc) //the other decoder has not received this frame yet, so store it in main frame buffer
						{
							lastCrc = rx->crc; //store CRC of this frame

							if(!rxFrameBufferFull) //if enough space, store the frame
							{
								rxFrame[rxFrameHead].start = rxBufferHead;
								rxFrame[rxFrameHead].signalLevel = ModemGetRMS(modem);
								rxFrame[rxFrameHead++].size = rx->frameIdx;
								rxFrameHead %= FRAME_MAX_COUNT;
								if(rxFrameHead == txFrameHead)
									rxFrameBufferFull = true;

								for(uint16_t i = 0; i < rx->frameIdx; i++)
								{
									rxBuffer[rxBufferHead++] = rx->frame[i];
									rxBufferHead %= FRAME_BUFFER_SIZE;
								}

							}
						}
					}
				}

    		}

		}
		rx->rx = RX_STAGE_FLAG;
		ModemClearRMS(modem);
		rx->receivedByte = 0;
		rx->receivedBitIdx = 0;
		rx->frameIdx = 0;
		rx->crc = 0xFFFF;
		return;
	}


	if((rx->rawData & 0x7F) == 0x7F) //received 7 consecutive ones, this is an error (sometimes called "escape byte")
	{
		rx->rx = RX_STAGE_FLAG;
		ModemClearRMS(modem);
		rx->receivedByte = 0;
		rx->receivedBitIdx = 0;
		rx->frameIdx = 0;
		rx->crc = 0xFFFF;
		return;
	}



	if(rx->rx == RX_STAGE_IDLE) //not in a frame, don't go further
		return;


	if((rx->rawData & 0x3F) == 0x3E) //dismiss bit 0 added by bit stuffing
		return;

	if(rx->rawData & 0x01) //received bit 1
		rx->receivedByte |= 0x80; //store it

	if(++rx->receivedBitIdx >= 8) //received full byte
	{
		if(rx->frameIdx > FRAME_MAX_SIZE) //frame is too long
		{
			rx->rx = RX_STAGE_IDLE;
			ModemClearRMS(modem);
			rx->receivedByte = 0;
			rx->receivedBitIdx = 0;
			rx->frameIdx = 0;
			rx->crc = 0xFFFF;
			return;
		}
		if(rx->frameIdx >= 2) //more than 2 bytes received, calculate CRC
		{
			for(uint8_t i = 0; i < 8; i++)
			{
				calculateCRC((rx->frame[rx->frameIdx - 2] >> i) & 1, &(rx->crc));
			}
		}
		rx->rx = RX_STAGE_FRAME;
		rx->frame[rx->frameIdx++] = rx->receivedByte; //store received byte
		rx->receivedByte = 0;
		rx->receivedBitIdx = 0;
	}
	else
		rx->receivedByte >>= 1;
}


uint8_t Ax25GetTxBit(void)
{
	if(txBitIdx == 8)
	{
		txBitIdx = 0;
		if(txStage == TX_STAGE_PREAMBLE) //transmitting preamble (TXDelay)
		{
			if(txDelayElapsed < txDelay) //still transmitting
			{
				txByte = 0x7E;
				txDelayElapsed++;
			}
			else //now transmit initial flags
			{
				txDelayElapsed = 0;
				txStage = TX_STAGE_HEADER_FLAGS;
			}

		}
		if(txStage == TX_STAGE_HEADER_FLAGS) //transmitting initial flags
		{
			if(txFlagsElapsed < STATIC_HEADER_FLAG_COUNT)
			{
				txByte = 0x7E;
				txFlagsElapsed++;
			}
			else
			{
				txFlagsElapsed = 0;
				txStage = TX_STAGE_DATA; //transmit data
			}
		}
		if(txStage == TX_STAGE_DATA) //transmitting normal data
		{
transmitNormalData:
			if((txFrameHead != txFrameTail) || txFrameBufferFull)
			{
				if(txByteIdx < txFrame[txFrameTail].size) //send buffer
				{
					txByte = txBuffer[(txFrame[txFrameTail].start + txByteIdx) % FRAME_BUFFER_SIZE];
					txByteIdx++;
				}
				else //end of buffer, send CRC
				{
					txStage = TX_STAGE_CRC; //transmit CRC
					txCrcByteIdx = 0;
				}
			}
			else //no more frames
			{
				txByteIdx = 0;
				txBitIdx = 0;
				txStage = TX_STAGE_TAIL;
			}
		}
		if(txStage == TX_STAGE_CRC) //transmitting CRC
		{
			if(txCrcByteIdx <= 1)
			{
				txByte = (txCrc & 0xFF) ^ 0xFF;
				txCrc >>= 8;
				txCrcByteIdx++;
			}
			else
			{
				txCrc = 0xFFFF;
				txStage = TX_STAGE_FOOTER_FLAGS; //now transmit flags
				txFlagsElapsed = 0;
			}

		}
		if(txStage == TX_STAGE_FOOTER_FLAGS)
		{
			if(txFlagsElapsed < STATIC_FOOTER_FLAG_COUNT)
			{
				txByte = 0x7E;
				txFlagsElapsed++;
			}
			else
			{
				txFlagsElapsed = 0;
				txStage = TX_STAGE_DATA; //return to normal data transmission stage. There might be a next frame to transmit
				txFrameBufferFull = false;
				txFrameTail++;
				txFrameTail %= FRAME_MAX_COUNT;
				goto transmitNormalData;
			}
		}
		if(txStage == TX_STAGE_TAIL) //transmitting tail
		{
			if(txTailElapsed < txTail)
			{
				txByte = 0x7E;
				txTailElapsed++;
			}
			else //tail transmitted, stop transmission
			{
				txTailElapsed = 0;
				txStage = TX_STAGE_IDLE;
				txCrc = 0xFFFF;
				txBitstuff = 0;
				txByte = 0;
				txInitStage = TX_INIT_OFF;
				txBufferTail = txBufferHead;
				ModemTransmitStop();
				return 0;
			}
		}

	}

	uint8_t txBit = 0;

	if((txStage == TX_STAGE_DATA) || (txStage == TX_STAGE_CRC)) //transmitting normal data or CRC
	{
		if(txBitstuff == 5) //5 consecutive ones transmitted
		{
			txBit = 0; //transmit bit-stuffed 0
			txBitstuff = 0;
		}
		else
		{
			if(txByte & 1) //1 being transmitted
			{
				txBitstuff++; //increment bit stuffing counter
				txBit = 1;
			}
			else
			{
				txBit = 0;
				txBitstuff = 0; //0 being transmitted, reset bit stuffing counter
			}
			if(txStage == TX_STAGE_DATA) //calculate CRC only for normal data
				calculateCRC(txByte & 1, &txCrc);

			txByte >>= 1;
			txBitIdx++;
		}
	}
	else //transmitting preamble or flags, don't calculate CRC, don't use bit stuffing
	{
		txBit = txByte & 1;
		txByte >>= 1;
		txBitIdx++;
	}
	return txBit;
}

/**
 * @brief Initialize transmission and start when possible
 */
void Ax25TransmitBuffer(void)
{
	if(txInitStage == TX_INIT_WAITING)
		return;
	if(txInitStage == TX_INIT_TRANSMITTING)
		return;

	if((txFrameHead != txFrameTail) || txFrameBufferFull)
	{
		txQuiet = (SysTickGet() + (Ax25Config.quietTime / SYSTICK_INTERVAL) + Random(0, 200 / SYSTICK_INTERVAL)); //calculate required delay
		txInitStage = TX_INIT_WAITING;
	}
}



/**
 * @brief Start transmission immediately
 * @warning Transmission should be initialized using Ax25_transmitBuffer
 */
static void transmitStart(void)
{
	txCrc = 0xFFFF; //initial CRC value
	txStage = TX_STAGE_PREAMBLE;
	txByte = 0;
	txBitIdx = 0;
	txFlagsElapsed = 0;
	ModemTransmitStart();
}


/**
 * @brief Start transmitting when possible
 * @attention Must be continuously polled in main loop
 */
void Ax25TransmitCheck(void)
{
	if(txInitStage == TX_INIT_OFF) //TX not initialized at all, nothing to transmit
		return;
	if(txInitStage == TX_INIT_TRANSMITTING) //already transmitting
		return;

	if(ModemIsTxTestOngoing()) //TX test is enabled, wait for now
		return;

	if(txQuiet < SysTickGet()) //quit time has elapsed
	{
		if(!ModemDcdState()) //channel is free
		{
			txInitStage = TX_INIT_TRANSMITTING; //transmit right now
			txRetries = 0;
			transmitStart();
		}
		else //channel is busy
		{
			if(txRetries == MAX_TRANSMIT_RETRY_COUNT) //timeout
			{
				txInitStage = TX_INIT_TRANSMITTING; //transmit right now
				txRetries = 0;
				transmitStart();
			}
			else //still trying
			{
				txQuiet = SysTickGet() + Random(100 / SYSTICK_INTERVAL, 500 / SYSTICK_INTERVAL); //try again after some random time
				txRetries++;
			}
		}
	}
}

void Ax25Init(void)
{
	txCrc = 0xFFFF;

	memset((void*)rxState, 0, sizeof(rxState));
	for(uint8_t i = 0; i < (sizeof(rxState) / sizeof(rxState[0])); i++)
		rxState[i].crc = 0xFFFF;

	txDelay = ((float)Ax25Config.txDelayLength / (8.f * 1000.f / ModemGetBaudrate())); //change milliseconds to byte count
	txTail = ((float)Ax25Config.txTailLength / (8.f * 1000.f / ModemGetBaudrate()));
}
