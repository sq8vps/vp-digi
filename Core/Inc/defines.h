#ifndef INC_DEFINES_H_
#define INC_DEFINES_H_

#define ENABLE_FX25

//guess hardware based on the selected MCU
#if defined(STM32F103xB) || defined(STM32F103x8)

#define BLUE_PILL 1

#elif defined(STM32F302xC)

#define AIOC 1

#endif

#endif
