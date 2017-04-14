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

#ifndef _SNIFFER_H_
#define _SNIFFER_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdbool.h>
#include "utils.h"
#include "sniffer.h"

/*- Types -------------------------------------------------------------------*/
typedef struct sniffer_t
{
  queue_t      queue;

  char         *name;
  int          uid;
  float        x;
  float        y;
  float        freq_a;
  float        freq_b;
  float        sensitivity;
  char         *path;

  float        *loss_trx;

  int          fd;
  int          seq;
} sniffer_t;

/*- Prototypes --------------------------------------------------------------*/
void sniffer_init(sniffer_t *sniffer);
void sniffer_write_frame(sniffer_t *sniffer, uint8_t *data, float power);

#endif // _SNIFFER_H_

