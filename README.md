# VP-Digi
VP-Digi is a functional, cheap, easy to assemble and configure STM32-based APRS digipeater controller with built-in KISS modem.\
Full documentation can be found [here](doc/manual.md).\
Dokumentacja po polsku dostępna jest [tutaj](doc/manual_pl.md).

* Multiple modems:
  * 1200 Bd AFSK Bell 202 (VHF standard)
  * 300 Bd AFSK Bell 103 (HF standard)
  * 9600 Bd GFSK G3RUH (UHF standard)
  * 1200 Bd AFSK V.23
* PWM (or deprecated R2R) signal generation
* Analog-digital busy channel detection (data carrier detection)
* AX.25 coder/decoder
* FX.25 (AX.25 with error correction) coder/decoder, fully compatible with [Direwolf](https://github.com/wb2osz/direwolf) and [UZ7HO Soundmodem](http://uz7.ho.ua/packetradio.htm)
* Digipeater: 4 settable n-N aliases, 4 simple aliases, viscous delay (known from aprx) or direct only, black and white list
* 8 independent beacons
* KISS mode (can be used as an ordinary Packet Radio, Winlink, APRS etc. modem)
* USB and 2 UARTs: independent, running in KISS, monitor or configuration mode

## Updating to 2.0.0+ on older hardware
Since version 2.0.0 the component values have changed to provide support for faster modulations (9600 Bd). If you want to use these some components must be replaced. For more informations refer to the [manual](doc/manual.md) ([polska wersja](doc/manual_pl.md)).

## Description, schematic, instructions
The user manual and technical description is available [here](doc/manual.md) ([polska wersja](doc/manual_pl.md)).

## Source code
The firmware was written using System Workbench for STM32 (SW4STM32) and you should be able to import this repository directly to the IDE. You can get the source code using:
```bash
git clone https://github.com/sq8vps/vp-digi.git
```
Since version 2.0.0 you will also need to get appropriate submodule ([LwFEC](https://github.com/sq8vps/lwfec) for Reed-Solomon FEC):
```bash
git submodule init
git submodule update
```
Since version 2.0.0 there is also a possibility to build the firmware with or without FX.25 protocol support. The ```ENABLE_FX25``` symbol must be defined to enable FX.25 support. On SW4STM32 (and STM32CubeIDE probably) this can be done under *Project->Properties->C/C++ Build->Settings->Preprocessor->Defined symbols*.

## References
The project took a lot of time to finish, but now it's probably the most effective, publicly available, STM32-based modem and the most customizable microcontroller-based APRS digipeater. I would like to mention some resources I found really useful or inspiring:
* [multimon-ng](https://github.com/EliasOenal/multimon-ng) - general demodulator idea (correlation)
* [BeRTOS](https://github.com/develersrl/bertos) - AX.25 decoder
* [Forum thread by SP8EBC](http://forum.aprs.pl/index.php?topic=2086.0) - inspiration
* [DireWolf](https://github.com/wb2osz/direwolf) - PLL bit recovery idea, FX.25 references
* [A High-Performance Sound-Card AX.25 Modem](https://www.tau.ac.il/~stoledo/Bib/Pubs/QEX-JulAug-2012.pdf) - preemphasis and low pass filtering idea
* [UZ7HO Soundmodem](http://uz7.ho.ua/packetradio.htm) - PLL-based DCD idea

## Contributing
All contributions are appreciated.

## License
The project is licensed under the GNU GPL v3 license (see [LICENSE](LICENSE)).
