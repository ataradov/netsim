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

#ifndef _NOISE_H_
#define _NOISE_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdbool.h>
#include "utils.h"
#include "events.h"

/*- Types -------------------------------------------------------------------*/
typedef struct noise_t
{
  queue_t      queue;

  char         *name;
  int          uid;
  float        x;
  float        y;
  float        freq_a;
  float        freq_b;
  float        power;
  long         on;
  long         off;

  event_t      event;
  bool         active;
} noise_t;

/*- Prototypes --------------------------------------------------------------*/
void noise_init(noise_t *noise);

#endif // _NOISE_H_

