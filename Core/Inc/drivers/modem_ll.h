/*
Copyright 2020-2025 Piotr Wilkon
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

/*
 * This file is kind of HAL for modem
 */

#ifndef DRIVERS_MODEM_LL_H_
#define DRIVERS_MODEM_LL_H_

#include <stdint.h>
#include "defines.h"

//Oversampling factor
//This is a helper value, not a setting that can be changed without further code modification!
#define MODEM_LL_OVERSAMPLING_FACTOR 4

#ifdef BLUE_PILL
#include "modem_ll_bluepill.h"
#elif defined(AIOC)
#include "modem_ll_aioc.h"
#endif

#endif /* DRIVERS_MODEM_LL_H_ */
