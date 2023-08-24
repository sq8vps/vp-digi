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

struct TxState
{
	uint8_t txByte; //current TX byte
	int8_t txBitIdx; //current bit index in txByte
	uint16_t txDelayElapsed; //counter of TXDelay bytes already sent
	uint8_t flagsElapsed; //counter of flag bytes already sent
	uint16_t xmitIdx; //current TX byte index in TX buffer
	uint8_t crcIdx; //currently transmitted byte of CRC
	uint8_t bitstuff; //bit-stuffing counter
	uint16_t txTailElapsed; //counter of TXTail bytes already sent
	uint16_t txDelay; //number of TXDelay bytes to send
	uint16_t txTail; //number of TXTail bytes to send
	uint16_t crc; //current CRC
	uint32_t txQuiet; //quit time + current tick value
	uint8_t txRetries; //number of TX retries
	enum TxInitStage tx; //current TX initialization stage
	enum TxStage txStage; //current TX stage
};

volatile struct TxState txState;


typedef struct
{
	uint16_t crc; //current CRC
	uint8_t frame[FRAMELEN]; //raw frame buffer
	uint16_t frameIdx; //index for raw frame buffer
	uint8_t recByte; //byte being currently received
	uint8_t rBitIdx; //bit index for recByte
	uint8_t rawData; //raw data being currently received
	RxStage rx; //current RX stage
	uint8_t frameReceived; //frame received flag
} RxState;

volatile RxState rxState1, rxState2;

uint16_t lastCrc = 0; //CRC of the last received frame. If not 0, a frame was successfully received
uint16_t rxMultiplexDelay = 0; //simple delay for decoder multiplexer to avoid receiving the same frame twice


Ax25_config ax25Cfg;
Ax25 ax25;

RxStage Ax25_getRxStage(uint8_t modemNo)
{
	if(modemNo == 0)
		return rxState1.rx;

	return rxState2.rx;
}



/**
 * @brief Recalculate CRC for one bit
 * @param[in] bit Input bit
 * @param *crc CRC pointer
 */
static void ax25_calcCRC(uint8_t bit, uint16_t *crc) {
    static uint16_t xor_result;
    xor_result = *crc ^ bit;
    *crc >>= 1;
    if (xor_result & 0x0001)
    {
    	*crc ^= 0x8408;
    }
    return;
}

void Ax25_bitParse(uint8_t bit, uint8_t modemNo)
{
	if(lastCrc > 0) //there was a frame received
	{
		rxMultiplexDelay++;
		if(rxMultiplexDelay > 4) //hold it for a while and wait for the other decoder to receive the frame
		{
			lastCrc = 0;
			rxMultiplexDelay = 0;
			ax25.frameReceived = (rxState1.frameReceived > 0) | ((rxState2.frameReceived > 0) << 1);
		}

	}


	RxState *rx = (RxState*)&rxState1;
	if(modemNo == 1)
		rx = (RxState*)&rxState2;

	rx->rawData <<= 1; //store incoming bit
	rx->rawData |= (bit > 0);


	if(rx->rawData == 0x7E) //HDLC flag received
	{
		if(rx->rx == RX_STAGE_FRAME) //we are in frame, so this is the end of the frame
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

				if(!ax25Cfg.allowNonAprs && ((rx->frame[i + 1] != 0x03) || (rx->frame[i + 2] != 0xf0)))
				{
					rx->recByte = 0;
					rx->rBitIdx = 0;
					rx->frameIdx = 0;
					rx->crc = 0xFFFF;

					return;
				}

    			if ((rx->frame[rx->frameIdx - 2] == ((rx->crc & 0xFF) ^ 0xFF)) && (rx->frame[rx->frameIdx - 1] == (((rx->crc >> 8) & 0xFF) ^ 0xFF))) //check CRC
	        	{
	        		rx->frameReceived = 1;
    				if(rx->crc != lastCrc) //the other decoder has not received this frame yet, so store it in main frame buffer
	        		{
	        			lastCrc = rx->crc; //store CRC of this frame
	        			ax25.sLvl = Afsk_getRMS(modemNo); //get RMS amplitude of the received frame
		        		uint16_t freebuf = 0;
		        		if(ax25.frameBufWr > ax25.frameBufRd) //check if there is enough free space in buffer
		        			freebuf = FRAMEBUFLEN - ax25.frameBufWr + ax25.frameBufRd - 3;
		        		else
		        			freebuf = ax25.frameBufRd - ax25.frameBufWr - 3;

		        		if((rx->frameIdx - 2) <= freebuf) //if enough space, store the frame
		        		{
		        			for(uint16_t i = 0; i < rx->frameIdx - 2; i++)
		        			{
		        				ax25.frameBuf[ax25.frameBufWr++] = rx->frame[i];
		        				ax25.frameBufWr %= (FRAMEBUFLEN);
		        			}
		        			ax25.frameBuf[ax25.frameBufWr++] = 0xFF; //add frame separator
		        			ax25.frameBufWr %= FRAMEBUFLEN;
		        		}
	        		}
	        	} else
	        	{
					Afsk_clearRMS(modemNo);
	        	}

    		}


			rx->recByte = 0;
			rx->rBitIdx = 0;
			rx->frameIdx = 0;
			rx->crc = 0xFFFF;
		}
		rx->rx = RX_STAGE_FLAG;
		rx->recByte = 0;
		rx->rBitIdx = 0;
		rx->frameIdx = 0;
		rx->crc = 0xFFFF;
		Afsk_clearRMS(modemNo);
		return;
	}


	if((rx->rawData & 0x7F) == 0x7F) //received 7 consecutive ones, this is an error (sometimes called "escape byte")
	{
		Afsk_clearRMS(modemNo);
		rx->rx = RX_STAGE_IDLE;
		rx->recByte = 0;
		rx->rBitIdx = 0;
		rx->frameIdx = 0;
		rx->crc = 0xFFFF;
		return;
	}



	if(rx->rx == RX_STAGE_IDLE) //not in a frame, don't go further
		return;


	if((rx->rawData & 0x3F) == 0x3E) //dismiss bit 0 added by bitstuffing
		return;

	if(rx->rawData & 0x01) //received bit 1
		rx->recByte |= 0x80; //store it

	if(++rx->rBitIdx >= 8) //received full byte
	{
		if(rx->frameIdx > FRAMELEN) //frame is too long
		{
			rx->rx = RX_STAGE_IDLE;
			rx->recByte = 0;
			rx->rBitIdx = 0;
			rx->frameIdx = 0;
			rx->crc = 0xFFFF;
			Afsk_clearRMS(modemNo);
			return;
		}
		if(rx->frameIdx >= 2) //more than 2 bytes received, calculate CRC
		{
			for(uint8_t i = 0; i < 8; i++)
			{
				ax25_calcCRC((rx->frame[rx->frameIdx - 2] >> i) & 0x01, &(rx->crc));
			}
		}
		rx->rx = RX_STAGE_FRAME;
		rx->frame[rx->frameIdx++] = rx->recByte; //store received byte
		rx->recByte = 0;
		rx->rBitIdx = 0;
	}
	else
		rx->recByte >>= 1;
}


uint8_t Ax25_getTxBit(void)
{
	if(txState.txBitIdx == 8)
	{
		txState.txBitIdx = 0;
		if(txState.txStage == TX_STAGE_PREAMBLE) //transmitting preamble (TXDelay)
		{
			if(txState.txDelayElapsed < txState.txDelay) //still transmitting
			{
				txState.txByte = 0x7E;
				txState.txDelayElapsed++;
			}
			else //now transmit initial flags
			{
				txState.txDelayElapsed = 0;
				txState.txStage = TX_STAGE_HEADER_FLAGS;
			}

		}
		if(txState.txStage == TX_STAGE_HEADER_FLAGS) //transmitting initial flags
		{
			if(txState.flagsElapsed < 4) //say we want to transmit 4 flags
			{
				txState.txByte = 0x7E;
				txState.flagsElapsed++;
			}
			else
			{
				txState.flagsElapsed = 0;
				txState.txStage = TX_STAGE_DATA; //transmit data
			}
		}
		if(txState.txStage == TX_STAGE_DATA) //transmitting normal data
		{
			if((ax25.xmitIdx > 10) && (txState.xmitIdx < ax25.xmitIdx)) //send buffer
			{
				if(ax25.frameXmit[txState.xmitIdx] == 0xFF) //frame separator found
				{
					txState.txStage = TX_STAGE_CRC; //transmit CRC
					txState.xmitIdx++;
				}
				else //normal bytes
				{
					txState.txByte = ax25.frameXmit[txState.xmitIdx];
					txState.xmitIdx++;
				}
			} else //end of buffer
			{
				ax25.xmitIdx = 0;
				txState.xmitIdx = 0;
				txState.txStage = TX_STAGE_TAIL;
			}
		}
		if(txState.txStage == TX_STAGE_CRC) //transmitting CRC
		{

			if(txState.crcIdx < 2)
			{
				txState.txByte = (txState.crc >> (txState.crcIdx * 8)) ^ 0xFF;
				txState.crcIdx++;
			}
			else
			{
				txState.crc = 0xFFFF;
				txState.txStage = TX_STAGE_FOOTER_FLAGS; //now transmit flags
				txState.crcIdx = 0;
			}
		}
		if(txState.txStage == TX_STAGE_FOOTER_FLAGS)
		{
			if(txState.flagsElapsed < 8) //say we want to transmit 8 flags
			{
				txState.txByte = 0x7E;
				txState.flagsElapsed++;
			} else
			{
				txState.flagsElapsed = 0;
				txState.txStage = TX_STAGE_DATA; //return to normal data transmission stage. There might be a next frame to transmit
				if((ax25.xmitIdx > 10) && (txState.xmitIdx < ax25.xmitIdx)) //send buffer
				{
					if(ax25.frameXmit[txState.xmitIdx] == 0xFF) //frame separator found
					{
						txState.txStage = TX_STAGE_CRC; //transmit CRC
						txState.xmitIdx++;
					}
					else //normal bytes
					{
						txState.txByte = ax25.frameXmit[txState.xmitIdx];
						txState.xmitIdx++;
					}
				} else //end of buffer
				{
					ax25.xmitIdx = 0;
					txState.xmitIdx = 0;
					txState.txStage = TX_STAGE_TAIL;
				}
			}
		}
		if(txState.txStage == TX_STAGE_TAIL) //transmitting tail
		{
			ax25.xmitIdx = 0;
			if(txState.txTailElapsed < txState.txTail)
			{
				txState.txByte = 0x7E;
				txState.txTailElapsed++;
			}
			else //tail transmitted, stop transmission
			{
				txState.txTailElapsed = 0;
				txState.txStage = TX_STAGE_IDLE;
				txState.crc = 0xFFFF;
				txState.bitstuff = 0;
				txState.txByte = 0;
				txState.tx = TX_INIT_OFF;
				Afsk_transmitStop();
			}


		}

	}

	uint8_t txBit = 0;

	if((txState.txStage == TX_STAGE_DATA) || (txState.txStage == TX_STAGE_CRC)) //transmitting normal data or CRC
	{
		if(txState.bitstuff == 5) //5 consecutive ones transmitted
		{
			txBit = 0; //transmit bit-stuffed 0
			txState.bitstuff = 0;
		}
		else
		{
			if(txState.txByte & 1) //1 being transmitted
			{
				txState.bitstuff++; //increment bit stuffing counter
				txBit = 1;
			} else
			{
				txBit = 0;
				txState.bitstuff = 0; //0 being transmitted, reset bit stuffing counter
			}
			if(txState.txStage == TX_STAGE_DATA) //calculate CRC only for normal data
				ax25_calcCRC(txState.txByte & 1, (uint16_t*)&(txState.crc));
			txState.txByte >>= 1;
			txState.txBitIdx++;
		}
	}
	else //transmitting preamble or flags, don't calculate CRC, don't use bit stuffing
	{
			txBit = txState.txByte & 1;
			txState.txByte >>= 1;
			txState.txBitIdx++;
	}
	return txBit;

}

/**
 * @brief Initialize transmission and start when possible
 */
void Ax25_transmitBuffer(void)
{
	if(txState.tx == TX_INIT_WAITING)
		return;
	if(txState.tx == TX_INIT_TRANSMITTING)
		return;

	if(ax25.xmitIdx > 10)
	{
		txState.txQuiet = (ticks + (ax25Cfg.quietTime / 10) + rando(0, 20)); //calculate required delay
		txState.tx = TX_INIT_WAITING;
	}
	else ax25.xmitIdx = 0;
}





/**
 * @brief Start transmission immediately
 * @warning Transmission should be initialized using Ax25_transmitBuffer
 */
static void ax25_transmitStart(void)
{
	txState.crc = 0xFFFF; //initial CRC value
	txState.txStage = TX_STAGE_PREAMBLE;
	txState.txByte = 0;
	txState.txBitIdx = 0;
	Afsk_transmitStart();
}


/**
 * @brief Start transmitting when possible
 * @attention Must be continuously polled in main loop
 */
void Ax25_transmitCheck(void)
{
	if(txState.tx == TX_INIT_OFF) //TX not initialized at all, nothing to transmit
		return;
	if(txState.tx == TX_INIT_TRANSMITTING) //already transmitting
		return;

	if(ax25.xmitIdx < 10)
	{
		ax25.xmitIdx = 0;
		return;
	}

	if(Afsk_isTxTestOngoing()) //TX test is enabled, wait for now
		return;

	if(txState.txQuiet < ticks) //quit time has elapsed
	{
		if(!Afsk_dcdState()) //channel is free
		{
			txState.tx = TX_INIT_TRANSMITTING; //transmit right now
			txState.txRetries = 0;
			ax25_transmitStart();
		}
		else //channel is busy
		{
			if(txState.txRetries == 8) //8th retry occurred, transmit immediately
			{
				txState.tx = TX_INIT_TRANSMITTING; //transmit right now
				txState.txRetries = 0;
				ax25_transmitStart();
			}
			else //still trying
			{
				txState.txQuiet = ticks + rando(10, 50); //try again after some random time
				txState.txRetries++;
			}
		}
	}
}

void Ax25_init(void)
{
	txState.crc = 0xFFFF;
	ax25.frameBufWr = 0;
	ax25.frameBufRd = 0;
	ax25.xmitIdx = 0;
	ax25.frameReceived = 0;

	rxState1.crc = 0xFFFF;
	rxState2.crc = 0xFFFF;

	txState.txDelay = ((float)ax25Cfg.txDelayLength / 6.66667f); //change milliseconds to byte count
	txState.txTail = ((float)ax25Cfg.txTailLength / 6.66667f);
}
