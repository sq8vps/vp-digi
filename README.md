# VP-Digi
VP-Digi is a functional, cheap, easy to assemble and configure STM32-based APRS digipeater controller with built-in KISS modem.

* 1200 Bd Bell 202 modem (VHF APRS standard)
* R2R or PWM signal generation
* Analog-digital busy channel detection (data carrier detection)
* AX.25 coder/decoder designed for APRS frames
* digipeater: 4 settable n-N aliases, 4 simple aliases, viscous delay (known from aprx) or direct only, black and white list
* 8 independent beacons
* KISS mode (can be used as an ordinary modem with UI-View, APRSIS32, XASTIR etc.)
* USB and 2 UARTs: independent, running in KISS, monitor or configuration mode

## Description, schematic, instructions
If you are not interested in source code, this repository is not for you. You can find full project description, schematics, compiled firmware and instructions [on my website](https://sq8l.pzk.pl/index.php/vp-digi-cheap-and-functional-aprs-digipeater-controller-with-kiss-modem/).

## Source code usage
The firmware was written using System Workbench for STM32 (SW4STM32) and you should be able to import this repository directly to the IDE. The source code is publicly available since version 1.3.0.

## Technical description
The project was designed to be running on a Blue Pill board (STM32F103C8T6) with 8MHz crystal. The firmware was written using only register operations (with ST headers) and CMSIS libraries. The HAL is there only for USB. The code is (quite) extensively commented where needed, so it shouldn't be very hard to understand.

### Demodulator
There are two demodulators (and decoders) running in parallel to provide better efficiency. The signal is sampled at 38400Hz (32 samples per symbol) by DMA. The interrupt is generated after receiving 4 samples and the samples are decimated. Then they are passed through a preemphasis/deemphasis filter (if enabled) which equalizes tone amplitudes. Filtered samples are multiplied by locally generated mark and space tones (their I and Q parts - cosine and sine). This gives a correlation factor between the input signal and each tone. In the meanwhile an unfiltered symbol is produced to drive the data carrier detection PLL. The difference of correlation factors is the new sample and is passed through a low pass filter to eliminate noise. Filtered samples are compared to zero and the demodulated symbol is sent to bit recovery mechanism.
### Bit recovery (and NRZI decoder)
Bit recovery mechanism is based on a digital PLL. The PLL is nominally running at 1200Hz (=1200 Baud). The symbol change should occur near PLL counter zero, so that the counter overflows in the middle of the symbol and the symbol value is sampled. With every symbol change the counter is multiplied by a <1 factor to bring it closer to zero, keeping the PLL and incoming signal in phase. The DCD signal is multiplexed from both modems. Explained more in [modem.c](Src/drivers/modem.c). Sampled symbol is decoded by NRZI decoder and sent to AX.25 layer.
### AX.25 decoding
AX.25 decoder is quite standard. CRC, PID and Control Byte are checked and both modem paths are multiplexed to produce only one output frame.
### Data carrier detection
DCD uses an analog-digital approach: based on PLL, but working on a digital unfiltered symbol stream. The PLL works in the same way as bit recovery PLL, except that there is a special DCD pulse counting mechanism implemented, explained in [modem.c](Src/drivers/modem.c). This approach seems to be far more effective than a typical, digital, AX.25-based detection.
### AX.25 encoding
AX.25 encoder is also quite simple. It can handle multiple frames in a row (limited by buffer length). Every transmission starts with preamble flags, then header flags, actual data, CRC, footer/separating flags, actual data, CRC, footer/separating flags... and tail flags. Raw bits are requested by the modulator.
### Modulator (and NRZI encoder)
The NRZI encoder runs at exactly 1200Hz (=1200 Baud) and requests bits from the AX.25 encoder. Bits are encoded to symbols and the DAC sampling timer interval is set depending on symbol value. Because of that there is only one sine table used. For 1200Hz tone the timer interval is larger than for 2200 Hz tone - the sampling frequency is changed to change the output signal frequency. An array index is always kept so that the output signal phase is continuous.
## Using VP-Digi code in your project
I would love to hear about projects which implement VP-Digi source code. If you are making one, let me know at sq8vps(at)gmail.com.
## References
The project took a lot of time to finish, but now it's probably the most effective, publicly available, STM32-based modem and the most customizable microcontroller-based APRS digipeater. I would like to mention some resources I found really useful or inspiring:
* [multimon-ng](https://github.com/EliasOenal/multimon-ng) - general demodulator idea (correlation)
* [BeRTOS](https://github.com/develersrl/bertos) - AX.25 decoder
* [Forum thread by SP8EBC](http://forum.aprs.pl/index.php?topic=2086.0) - inspiration
* [DireWolf](https://github.com/wb2osz/direwolf) - PLL bit recovery idea
* [A High-Performance Sound-Card AX.25 Modem](https://www.tau.ac.il/~stoledo/Bib/Pubs/QEX-JulAug-2012.pdf) - preemphasis and low pass filtering idea
* [UZ7HO Soundmodem](http://uz7.ho.ua/packetradio.htm) - PLL-based DCD idea
## Contributing
All contributions are appreciated, but please keep the code reasonably clean. Also, please make sure that the firmware is working well before creating a pull request.

## License
The project is licensed under the GNU GPL v3 license (see [LICENSE](LICENSE)).
