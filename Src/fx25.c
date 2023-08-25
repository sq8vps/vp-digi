
#ifdef ENABLE_FX25

#include "fx25.h"
#include <stddef.h>
#include "rs.h"

#define FX25_PREGENERATE_POLYS

const struct Fx25Mode Fx25ModeList[11] =
{
	{.tag =  0xB74DB7DF8A532F3E, .K = 239, .T = 16},
	{.tag =  0x26FF60A600CC8FDE, .K = 128, .T = 16},
	{.tag =  0xC7DC0508F3D9B09E, .K = 64, .T = 16},
	{.tag =  0x8F056EB4369660EE, .K = 32, .T = 16},
	{.tag =  0x6E260B1AC5835FAE, .K = 223, .T = 32},
	{.tag =  0xFF94DC634F1CFF4E, .K = 128, .T = 32},
	{.tag =  0x1EB7B9CDBC09C00E, .K = 64, .T = 32},
	{.tag =  0xDBF869BD2DBB1776, .K = 32, .T = 32},
	{.tag =  0x3ADB0C13DEAE2836, .K = 191, .T = 64},
	{.tag =  0xAB69DB6A543188D6, .K = 128, .T = 64},
	{.tag =  0x4A4ABEC4A724B796, .K = 64, .T = 64}
};

const struct Fx25Mode* Fx25GetMode(uint16_t size)
{
	if(size <= 32)
		return &Fx25ModeList[3];
	else if(size <= 64)
		return &Fx25ModeList[2];
	else if(size <= 128)
		return &Fx25ModeList[5];
	else if(size <= 191)
		return &Fx25ModeList[8];
	else if(size <= 223)
		return &Fx25ModeList[4];
	else if(size <= 239)
		return &Fx25ModeList[0];
	else
		return NULL; //frame too big, do not use FX.25
}

#ifdef FX25_PREGENERATE_POLYS
static uint8_t poly16[17], poly32[33], poly64[65];
#else
static uint8_t poly[65];
#endif

void Fx25AddParity(uint8_t *buffer, const struct Fx25Mode *mode)
{
#ifdef FX25_PREGENERATE_POLYS
	uint8_t *poly = NULL;
	switch(mode->T)
	{
		case 16:
			poly = poly16;
			break;
		case 32:
			poly = poly32;
			break;
		case 64:
			poly = poly64;
			break;
		default:
			poly = poly16;
	}
#else
	RsGeneratePolynomial(mode->T, poly);
#endif
	RsEncode(buffer, mode->K + mode->T, poly, mode->T);
}

void Fx25Init(void)
{
#ifdef FX25_PREGENERATE_POLYS
	RsGeneratePolynomial(16, poly16);
	RsGeneratePolynomial(32, poly32);
	RsGeneratePolynomial(64, poly64);
#else
#endif
}

#endif
