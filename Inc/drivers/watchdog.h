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

#ifndef DRIVERS_WATCHDOG_H_
#define DRIVERS_WATCHDOG_H_



/**
 * @brief Initialize watchdog
 */
void Wdog_init(void);

/**
 * @brief Restart watchdog
 * @attention Must be called continuously in main loop
 */
void Wdog_reset(void);

#endif /* DRIVERS_WATCHDOG_H_ */
