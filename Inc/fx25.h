#ifndef FX25_H_
#define FX25_H_

#ifdef ENABLE_FX25

#include <stdint.h>
#include <stdbool.h>

#define FX25_MAX_BLOCK_SIZE 255

struct Fx25Mode
{
	uint64_t tag; //correlation tag
	uint16_t K; //data size
	uint8_t T; //parity check size
};

extern const struct Fx25Mode Fx25ModeList[11];

/**
 * @brief Get FX.25 mode for given correlation tag
 * @param tag FX.25 correlation tag
 * @return FX.25 mode structure pointer or NULL if not a FX.25 tag
 */
const struct Fx25Mode* Fx25GetModeForTag(uint64_t tag);

/**
 * @brief Get FX.25 mode for given payload size
 * @param size Payload size including flags and CRC
 * @return FX.25 mode structure pointer or NULL if standard AX.25 must be used
 */
const struct Fx25Mode* Fx25GetModeForSize(uint16_t size);

/**
 * @brief Encode AX.25 packet using FX.25
 * @param *buffer Input full AX.25 packets with flags and output FX.25 packet
 * @param *mode FEC mode to use
 */
void Fx25Encode(uint8_t *buffer, const struct Fx25Mode *mode);

/**
 * @brief Decode FX.25 packet to AX.25
 * @param *buffer Input FX.25 packet and output AX.25 packet
 * @param *mode FEC mode to use
 * @param *fixed Number of bytes corrected
 * @return True on success, false on failure
 */
bool Fx25Decode(uint8_t *buffer, const struct Fx25Mode *mode, uint8_t *fixed);

/**
 * @brief Initialize FX.25 module
 */
void Fx25Init(void);

#endif

#endif /* FX25_H_ */
