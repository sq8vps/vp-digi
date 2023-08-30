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

#ifndef DRIVERS_MODEM_H_
#define DRIVERS_MODEM_H_

#include <stdint.h>

//number of parallel demodulators
//each demodulator must be explicitly configured in code
#define MODEM_DEMODULATOR_COUNT 2

#define MODEM_BAUDRATE 1200.f
#define MODEM_MARK_FREQUENCY 1200.f
#define MODEM_SPACE_FREQUENCY 2200.f

enum ModemTxTestMode
{
	TEST_DISABLED,
	TEST_MARK,
	TEST_SPACE,
	TEST_ALTERNATING,
};


struct ModemDemodConfig
{
	uint8_t usePWM : 1; //0 - use R2R, 1 - use PWM
	uint8_t flatAudioIn : 1; //0 - normal (deemphasized) audio input, 1 - flat audio (unfiltered) input
};

extern struct ModemDemodConfig ModemConfig;

enum ModemEmphasis
{
	PREEMPHASIS,
	DEEMPHASIS,
	EMPHASIS_NONE
};


/**
 * @brief Get filter type (preemphasis, deemphasis etc.) for given modem
 * @param modem Modem number
 * @return Filter type
 */
enum ModemEmphasis ModemGetFilterType(uint8_t modem);

/**
 * @brief Get current DCD state
 * @return 1 if channel busy, 0 if free
 */
uint8_t ModemDcdState(void);

/**
 * @brief Check if there is a TX test mode enabled
 * @return 1 if in TX test mode, 0 otherwise
 */
uint8_t ModemIsTxTestOngoing(void);

/**
 * @brief Clear modem RMS counter
 * @param number Modem number
 */
void ModemClearRMS(uint8_t number);

/**
 * @brief Get RMS value for modem
 * @param number Modem number
 * @return RMS value
 */
uint16_t ModemGetRMS(uint8_t number);

/**
 * @brief Start or restart TX test mode
 * @param type TX test type: TEST_MARK, TEST_SPACE or TEST_ALTERNATING
 */
void ModemTxTestStart(enum ModemTxTestMode type);


/**
 * @brief Stop TX test mode
 */
void ModemTxTestStop(void);

/**
 * @brief Configure and start TX
 * @info This function is used internally by protocol module.
 * @warning Use Ax25TransmitStart() to initialize transmission
 */
void ModemTransmitStart(void);


/**
 * @brief Stop TX and go back to RX
 */
void ModemTransmitStop(void);


/**
 * @brief Initialize modem module
 */
void ModemInit(void);


#if (MODEM_DEMODULATOR_COUNT > 8)
#error There may be at most 8 parallel demodulators/decoders
#endif

#endif
