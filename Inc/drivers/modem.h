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

#ifndef DRIVERS_MODEM_H_
#define DRIVERS_MODEM_H_

#include <stdint.h>


typedef enum
{
	TEST_DISABLED,
	TEST_MARK,
	TEST_SPACE,
	TEST_ALTERNATING,
} TxTestMode;


typedef struct
{
	uint8_t usePWM : 1; //0 - use R2R, 1 - use PWM
	uint8_t flatAudioIn : 1; //0 - normal (deemphasized) audio input, 1 - flat audio (unfiltered) input
} Afsk_config;

extern Afsk_config afskCfg;

typedef enum
{
	PREEMPHASIS,
	DEEMPHASIS,
	EMPHASIS_NONE
}  Emphasis;

/**
 * @brief Get current DCD (channel busy) state
 * @return 0 if channel free, 1 if busy
 */
uint8_t Afsk_dcdState(void);

/**
 * @brief Check if there is an ongoing TX test
 * @return 0 if not, 1 if any TX test is enabled
 */
uint8_t Afsk_isTxTestOngoing(void);

/**
 * @brief Clear RMS meter state for specified modem
 * @param[in] modemNo Modem number: 0 or 1
 */
void Afsk_clearRMS(uint8_t modemNo);

/**
 * @brief Clear RMS value from specified modem
 * @param[in] modemNo Modem number: 0 or 1
 * @return RMS
 */
uint16_t Afsk_getRMS(uint8_t modemNo);

/**
 * @brief Current DCD state
 * @return 0 if no DCD (channel free), 1 if DCD
 */
uint8_t Afsk_dcdState(void);

/**
 * @brief Start or restart TX test mode
 * @param[in] type TX test type: TEST_MARK, TEST_SPACE or TEST_ALTERNATING
 */
void Afsk_txTestStart(TxTestMode type);


/**
 * @brief Stop TX test mode
 */
void Afsk_txTestStop(void);


/**
 * @brief Configure and start TX
 * @warning Transmission should be started with Ax25_transmitBuffer
 */
void Afsk_transmitStart(void);


/**
 * @brief Stop TX and go back to RX
 */
void Afsk_transmitStop(void);


/**
 * @brief Initialize AFSK module
 */
void Afsk_init(void);



void DMA1_Channel2_IRQHandler(void) __attribute__ ((interrupt));
void TIM3_IRQHandler(void) __attribute__ ((interrupt));
void TIM2_IRQHandler(void) __attribute__ ((interrupt));


#endif
