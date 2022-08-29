/*
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

#ifndef CONFIG_H_
#define CONFIG_H_


#include <stdint.h>


#define FLAG_CONFIG_WRITTEN 0x6B

#define MEM_CONFIG 0x800F000

//these are relative addresses, absolute address is calculated as relative address + MEM_CONFIG
//all fields are 16-bit or n*16-bit long, as data in flash is stored in 16-bit words
#define CONFIG_FLAG 0 //configuration written flag
#define CONFIG_CALL 2
#define CONFIG_SSID 8
#define CONFIG_TXDELAY 10
#define CONFIG_TXTAIL 12
#define CONFIG_TXQUIET 14
#define CONFIG_RS1BAUD 16
#define CONFIG_RS2BAUD 20
#define CONFIG_BEACONS 24
#define CONFIG_BCIV 26 //beacon intervals
#define CONFIG_BCDL 42 //beacon delays
#define CONFIG_BC0 58 //beacon information fields, null terminated
#define CONFIG_BC1 158
#define CONFIG_BC2 258
#define CONFIG_BC3 358
#define CONFIG_BC4 458
#define CONFIG_BC5 558
#define CONFIG_BC6 668
#define CONFIG_BC7 768
#define CONFIG_BCP0 868 //beacon paths, 14 bytes each
#define CONFIG_BCP1 882
#define CONFIG_BCP2 896
#define CONFIG_BCP3 910
#define CONFIG_BCP4 924
#define CONFIG_BCP5 938
#define CONFIG_BCP6 952
#define CONFIG_BCP7 966
#define CONFIG_DIGION 980
#define CONFIG_DIGIEN 982
#define CONFIG_DIGIVISC 984 //viscous-delay settings in higher half, direct-only in lower half
#define CONFIG_DIGIAL0 986
#define CONFIG_DIGIAL1 992
#define CONFIG_DIGIAL2 998
#define CONFIG_DIGIAL3 1004
#define CONFIG_DIGIAL4 1010
#define CONFIG_DIGIAL5 1018
#define CONFIG_DIGIAL6 1026
#define CONFIG_DIGIAL7 1034
#define CONFIG_DIGIMAX0 1042
#define CONFIG_DIGIMAX1 1044
#define CONFIG_DIGIMAX2 1046
#define CONFIG_DIGIMAX3 1048
#define CONFIG_DIGIREP0 1050
#define CONFIG_DIGIREP1 1052
#define CONFIG_DIGIREP2 1054
#define CONFIG_DIGIREP3 1056
#define CONFIG_DIGITRACE 1058
#define CONFIG_DIGIDEDUPE 1060
#define CONFIG_DIGICALLFILEN 1062
#define CONFIG_DIGIFILLIST 1064
#define CONFIG_DIGIFILTYPE 1204
#define CONFIG_AUTORST 1206
#define CONFIG_DIGISSID4 1208
#define CONFIG_DIGISSID5 1210
#define CONFIG_DIGISSID6 1212
#define CONFIG_DIGISSID7 1214
#define CONFIG_PWM_FLAT 1216
#define CONFIG_KISSMONITOR 1218
#define CONFIG_XXX 1220 //next address (not used)

/**
 * @brief Store configuration from RAM to Flash
 */
void Config_write(void);

/**
 * @brief Erase all configuration
 */
void Config_erase(void);

/**
 * @brief Read configuration from Flash to RAM
 * @return 1 if success, 0 if no configuration stored in Flash
 */
uint8_t Config_read(void);

#endif /* CONFIG_H_ */
