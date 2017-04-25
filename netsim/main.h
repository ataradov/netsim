/*
 * Copyright (c) 2014-2017, Alex Taradov <alex@taradov.com>
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
#include "utils.h"
#include "config.h"

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  uint32_t     seed;
  uint64_t     time;
  float        scale;
  int          node_uid;
  int          noise_uid;
  int          sniffer_uid;
  uint64_t     cycle;

  queue_t      active;
  queue_t      sleeping;
  queue_t      trxs;
  queue_t      noises;
  queue_t      sniffers;
} sim_t;

/*- Variables ---------------------------------------------------------------*/
extern sim_t g_sim;

#endif // _MAIN_H_

