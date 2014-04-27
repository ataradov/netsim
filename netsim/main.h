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

#ifndef _MAIN_H_
#define _MAIN_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/*- Definitions -------------------------------------------------------------*/
#define DEBUG_CORE         0
#define DEBUG_TRX          1
#define DEBUG_NOISE        0

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  uint32_t     seed;
  uint32_t     time;
  float        scale;

  uint32_t     uid;
  soc_t        *socs;
  trx_t        *trxs;
  noise_t      *noises;
  sniffer_t    *sniffers;
  uint64_t     cycle;
} sim_t;

/*- Variables ---------------------------------------------------------------*/
extern sim_t g_sim;

#endif // _MAIN_H_

