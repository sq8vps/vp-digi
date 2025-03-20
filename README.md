# VP-Digi
Polska wersja tego pliku dostępna jest [tutaj](README_pl.md).

**VP-Digi** is a functional, affordable, easy-to-assemble, and configure STM32-based APRS digipeater controller with a built-in KISS modem. VP-Digi can also run on [AIOC](https://github.com/skuep/AIOC)!

* Multiple modems:
  * 1200 Bd AFSK Bell 202 (VHF standard)
  * 300 Bd AFSK Bell 103 (HF standard)
  * 9600 Bd GFSK G3RUH (UHF standard)
  * 1200 Bd AFSK V.23
* DAC/PWM signal generation
* Analog-digital busy channel detection (data carrier detection)
* AX.25 coder/decoder
* FX.25 (AX.25 with error correction) coder/decoder, fully compatible with [Direwolf](https://github.com/wb2osz/direwolf) and [UZ7HO Soundmodem](http://uz7.ho.ua/packetradio.htm)
* Digipeater: 4 settable n-N aliases, 4 simple aliases, viscous delay (known from aprx) or direct only, black and white list
* 8 independent beacons
* KISS mode (can be used as an ordinary Packet Radio, Winlink, APRS, etc. modem)
* USB and 2 UARTs: independent, running in KISS, monitor, or configuration mode
  
## Download and setup
The latest compiled firmware can be downloaded [here](https://github.com/sq8vps/vp-digi/releases).\
Full documentation can be found [here](doc/manual.md).

## Updating to 2.0.0+ on older hardware

Since version 2.0.0, the component values have changed to provide support for faster modulations (9600 Bd). If you want to use these, some components must be replaced. For more information, refer to the [manual](doc/manual.md).

## Description, schematic, instructions

The user manual and technical description are available [here](doc/manual.md).

## Source code

You can get the source code using:

```bash
git clone https://github.com/sq8vps/vp-digi.git
```
Since version 2.0.0, you will also need to get the appropriate submodule ([LwFEC](https://github.com/sq8vps/lwfec) for Reed-Solomon FEC):

```bash
git submodule init
git submodule update
```

Since version 2.2.0, VP-Digi can also run on [AIOC](https://github.com/skuep/AIOC). The source code base is the same for the "Blue Pill" board and AIOC. 
However, there are two STM32CubeMX configuration files: `vp-digi.ioc` and `vp-digi_aioc.ioc`. In order to be able to compile and run the project, you need to:
1. Open `vp-digi.ioc` or `vp-digi_aioc.ioc` in STM32CubeMX, depending on your target platform.
2. Generate files for the IDE/toolchain of your choice. Make sure *Generate Under Root* is checked. Under the *Code Generator* tab make sure *Generate peripheral initialization as a pair of '.c/.h' files per peripheral* is checked.
3. Import the generated project to your IDE.
4. If you want to use FX.25, you need to include the `lwfec` directory in your build. In STM32CubeIDE, this can be done under *Project->Properties->C/C++ General->Paths and Symbols* in *Includes* and *Source Locations* tabs.
5. If you are targetting the "Blue Pill" board with STM32F103Cx, then you need to adjust the optimization level to `Optimize For Size (-Os)` for a release build or `Optimize for Debug (-Og)` for a debug build. 
On STM32CubeIDE, this can be done under *Project->Properties->C/C++ Build->Settings->MCU GCC Compiler->Optimization*. Otherwise, the program won't fit in the flash memory.

Since version 2.0.0, there is also a possibility to build the firmware with or without FX.25 protocol support. The `ENABLE_FX25` symbol must be defined to enable FX.25 support.
The easiest way is to define this symbol in `defines.h`. Alternatively, on STM32CubeIDE, this can be done under *Project->Properties->C/C++ Build->Settings->MCU GCC Compiler->Preprocessor->Defined symbols*.

When rebulding the project for different platform the code must be regenerated, as explained in the instructions above. 
However, since some files remain and some are not regenerated, you need to manually remove them beforehand. This includes removing the `Drivers`, `Middlewares`, `USB_DEVICE` except `USB_DEVICE/App/usbd_cdc_if.c`,
`Core/Startup` directories, the `stm32f*_hal_msp.c`, `syscalls.c`, `sysmem.c`, `gpio.c`, `system_stm32f*.c` files from the `Core/Src` directory, `stm32f*_it.h`, `stm32f*_hal_conf.h`, and `gpio.h` files from the `Core/Inc` directory,
and `.project`, `.cproject`, and `.mxproject` from the main directory.

## Contributing
All contributions are appreciated.

## License
The project is licensed under the GNU GPL v3 license (see [LICENSE](LICENSE)).
