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
