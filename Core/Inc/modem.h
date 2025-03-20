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

//number of maximum parallel demodulators
//each demodulator must be explicitly configured in code
//currently used only for 1200 Bd modem
#define MODEM_MAX_DEMODULATOR_COUNT 2

enum ModemType
{
	MODEM_1200 = 0,
	MODEM_1200_V23,
	MODEM_300,
	MODEM_9600,
};

enum ModemTxTestMode
{
	TEST_DISABLED,
	TEST_MARK,
	TEST_SPACE,
	TEST_ALTERNATING,
};


struct ModemDemodConfig
{
	enum ModemType modem;
	uint32_t usePWM : 1; //PWM or DAC is always used, R2R support is dropped - this bit is unused since v.2.2.0
	uint32_t flatAudioIn : 1; //0 - normal (deemphasized) audio input, 1 - flat audio (unfiltered) input
	uint32_t pttOutput : 1; //AIOC version: 0 - PTT1 at PA1, 1 - PTT2 at PA0; non-AIOC version: don't care
	uint32_t attenuator : 1; //AIOC version: 0 - no output attenuation, 1 - 90% attenuation; non-AIOC version: don't care
	uint32_t gain : 3; //AIOC version: input gain = (1 << gain); non-AIOC version: don't care
	uint32_t txLevel : 7; //TX audio level in %
};

extern struct ModemDemodConfig ModemConfig;

enum ModemPrefilter
{
	PREFILTER_NONE = 0,
	PREFILTER_PREEMPHASIS,
	PREFILTER_DEEMPHASIS,
	PREFILTER_FLAT,
};

/**
 * @brief Get measured signal level
 * @param modem Modem number
 * @param *peak Output signal positive peak in %
 * @param *valley Output signal negative peak in %
 * @param *level Output signal level in %
 */
void ModemGetSignalLevel(uint8_t modem, int8_t *peak, int8_t *valley, uint8_t *level);

/**
 * @brief Get current modem baudrate
 * @return Baudrate
 */
float ModemGetBaudrate(void);

/**
 * @brief Get count of demodulators running in parallel
 * @return Count of demodulators
 */
uint8_t ModemGetDemodulatorCount(void);

/**
 * @brief Get prefilter type (preemphasis, deemphasis etc.) for given modem
 * @param modem Modem number
 * @return Filter type
 */
enum ModemPrefilter ModemGetFilterType(uint8_t modem);

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
 * @brief Apply RX gain setting (if applicable)
 */
void ModemApplyRxGain(void);

/**
 * @brief Apply TX attenuator (if applicable)
 */
void ModemApplyTxAttenuator(void);

/**
 * @brief Initialize modem module
 */
void ModemInit(void);

#endif
