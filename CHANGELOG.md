# 2.0.0 (2023-09-09)
## New features
* New modems: AFSK Bell 103 (300 Bd, 1600/1800 Hz), GFSK G3RUH (9600 Bd), AFSK V.23 (1200 Bd, 1300/2100 Hz)
* FX.25 (AX.25 with Reed-Solomon FEC) support
* Default UART/USB modes can be configured
## Bug fixes
* none
## Other
* New signal level measurement method
* Full documentation (English and Polish) in Markdown
## Known bugs
* none
# 1.3.3 (2023-09-04)
## New features
* none
## Bug fixes
* RX buffer pointers bug fix
* AX.25 to TNC2 converter bug with non-UI frames
## Other
* New KISS handling method to support long and multiple frames
## Known bugs
* none
# 1.3.2 (2023-08-31)
## New features
* none
## Bug fixes
* Duplicate protection was not working properly
## Other
* none
## Known bugs
* none
# 1.3.1 (2023-08-30)
## New features
* none
## Bug fixes
* Non-APRS switch was not stored in memory
## Other
* PWM is now the default option
## Known bugs
* none
# 1.3.0 (2023-08-30)
## New features
* Callsign is now set together with SSID using ```call <call-SSID>```
* ```time``` command to show uptime
## Removed features
* ```ssid``` command is removed
* Auto-reset functionality and ```autoreset``` command is removed
## Bug fixes
* When beacon *n* delay hadn't passed yet, beacon *n+1*, *n+2*, ... were not sent regardless of their delay
* Bugs with line ending parsing
## Other
* Major code refactoring and rewrite
* Got rid of *uart_transmitStart()* routine
* USB sending is handled the same way as UART
* New way of TX and RX frame handling to improve non-APRS compatibility
* Much bigger frame buffer
* Minimized number of temporary buffers
* All *malloc()*s removed
* Added copyright notice as required by GNU GPL
## Known bugs
* none
# 1.2.6 (2023-07-29)
## New features
* Added ```nonaprs [on/off]``` command that enables reception of non-APRS frames, e.g. for full Packet Radio use
## Bug fixes
* Beacons not being send fixed
## Other
* none
## Known bugs
* none
# 1.2.5 (2022-11-05)
## New features
* Added ```dest <address>``` command that enables setting own destination address
## Bug fixes
* none
## Other
* PWM defaulting to 50%
## Known bugs
* none
# 1.2.4 (2022-08-30)
## New features
* Added ```monkiss [on/off]``` command that enables sending own and digipeated frames to KISS ports
## Bug fixes
* included .cproject so that the project can be imported and compiled in SW4STM32 without generating in CubeMX
## Other
* none
## Known bugs
* none
# 1.2.3 (2022-08-23)
## New features
* none
## Bug fixes
* KISS TX (UART and USB) buffer overrun, minor changes
## Other
* none
## Known bugs
* none
# 1.2.2 (2022-06-11)
## New features
* none
## Bug fixes
* Default de-dupe time was 0, backspace was sometimes stored in config, frame length was not checked in viscous delay mode
## Other
* none
## Known bugs
* USB in KISS mode has problem with TX frames
# 1.2.1 (2021-10-13)
## New features
* none
## Bug fixes
* Digi max and rep values could not be entered when only LF line ending was used.
## Other
* none
## Known bugs
* none
# 1.2.0 (2021-09-10)
This is the very first open-source VP-Digi release.
## New features
* Viscous-delay and direct-only modes are enabled separately for each digipeater alias: ```digi <0-7> viscous [on/off]``` and ``` digi <0-7> direct [on/off] ```.
**When updating from version <1.2.0 please erase configuration or set new settings carefully.**
* Selectable signal input type: normal (filtered) audio or flat (unfiltered) audio: ```flat [on/off]``` - switches modem settings - before v. 1.2.0 was hardcoded for normal audio
* Erase all settings command: ```eraseall```
## Bug fixes
* none
## Other
* Code was partially rewritten (especially digipeater, modem and AX.25 layer)
## Known bugs
* none