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

#ifndef _EVENTS_H_
#define _EVENTS_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/*- Types -------------------------------------------------------------------*/
typedef struct event_t
{
  struct event_t *next;
  uint64_t     time;

  int          timeout;
  void         (*callback)(struct event_t *);
  void         *data;
} event_t;

/*- Prototypes --------------------------------------------------------------*/
void events_add(event_t *event);
void events_remove(event_t *event);
bool events_is_planned(event_t *event);
void events_tick(void);
uint64_t events_jump(void);

#endif // _EVENTS_H_

