#ifndef FX25_H_
#define FX25_H_

#ifdef ENABLE_FX25

#include <stdint.h>

#define FX25_MAX_BLOCK_SIZE 255

struct Fx25Mode
{
	uint64_t tag; //correlation tag
	uint16_t K; //data size
	uint8_t T; //parity check size
};

extern const struct Fx25Mode Fx25ModeList[11];

/**
 * @brief Get FX.25 mode for given payload size
 * @param size Payload size including flags and CRC
 * @return FX.25 mode structure pointer or NULL if standard AX.25 must be used
 */
const struct Fx25Mode* Fx25GetMode(uint16_t size);

void Fx25AddParity(uint8_t *buffer, const struct Fx25Mode *mode);

void Fx25Init(void);

#endif

#endif /* FX25_H_ */
