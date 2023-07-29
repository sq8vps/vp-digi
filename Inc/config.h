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
#define CONFIG_BC6 658	//668 ? it should be 658 e.t.c.
#define CONFIG_BC7 758	
#define CONFIG_BCP0 858 //beacon paths, 14 bytes each
#define CONFIG_BCP1 872
#define CONFIG_BCP2 886
#define CONFIG_BCP3 900
#define CONFIG_BCP4 914
#define CONFIG_BCP5 928
#define CONFIG_BCP6 942
#define CONFIG_BCP7 956
#define CONFIG_DIGION 970
#define CONFIG_DIGIEN 972
#define CONFIG_DIGIVISC 974 //viscous-delay settings in higher half, direct-only in lower half
#define CONFIG_DIGIAL0 976
#define CONFIG_DIGIAL1 982
#define CONFIG_DIGIAL2 988
#define CONFIG_DIGIAL3 994
#define CONFIG_DIGIAL4 1000
#define CONFIG_DIGIAL5 1008
#define CONFIG_DIGIAL6 1016
#define CONFIG_DIGIAL7 1024
#define CONFIG_DIGIMAX0 1032
#define CONFIG_DIGIMAX1 1034
#define CONFIG_DIGIMAX2 1036
#define CONFIG_DIGIMAX3 1038
#define CONFIG_DIGIREP0 1040
#define CONFIG_DIGIREP1 1042
#define CONFIG_DIGIREP2 1044
#define CONFIG_DIGIREP3 1046
#define CONFIG_DIGITRACE 1048
#define CONFIG_DIGIDEDUPE 1050
#define CONFIG_DIGICALLFILEN 1052
#define CONFIG_DIGIFILLIST 1054
#define CONFIG_DIGIFILTYPE 1194
#define CONFIG_AUTORST 1196
#define CONFIG_DIGISSID4 1198
#define CONFIG_DIGISSID5 1200
#define CONFIG_DIGISSID6 1202
#define CONFIG_DIGISSID7 1204
#define CONFIG_PWM_FLAT 1206
#define CONFIG_KISSMONITOR 1208
#define CONFIG_DEST 1210
#define CONFIG_ALLOWNONAPRS 1216
#define CONFIG_XXX 1218 //next address (not used)

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
