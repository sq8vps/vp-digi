/*
Copyright 2020-2023 Piotr Wilkon
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



/**
 * @brief Store configuration from RAM to Flash
 */
void ConfigWrite(void);

/**
 * @brief Erase all configuration
 */
void ConfigErase(void);

/**
 * @brief Read configuration from Flash to RAM
 * @return 1 if success, 0 if no configuration stored in Flash
 */
uint8_t ConfigRead(void);

#endif /* CONFIG_H_ */
