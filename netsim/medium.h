/*
 * Copyright (c) 2014, Alex Taradov <taradov@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MEDIUM_H_
#define _MEDIUM_H_

/*- Includes ----------------------------------------------------------------*/
#include "trx.h"

/*- Definitions -------------------------------------------------------------*/
#define MEDIUM_NOISE_FLOOR     (-100) // dBm

/*- Prototypes --------------------------------------------------------------*/
void medium_update_trx(trx_t *rx_trx);

void medium_tx_start(trx_t *trx);
void medium_tx_end(trx_t *trx, bool normal);

#endif // _MEDIUM_H_

