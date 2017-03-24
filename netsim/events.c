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

/*- Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
#include "events.h"

/*- Variables ---------------------------------------------------------------*/
static event_t *events = NULL;
static event_t *last_event = NULL;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void events_add(event_t *event)
{
  event->time = get_sim_cycle() + event->timeout;

  if (NULL == events)
  {
    event->next = NULL;
    events = event;
    last_event = event;
  }
  else if (event->time >= last_event->time)
  {
    event->next = NULL;
    last_event->next = event;
    last_event = event;
  }
  else if (event->time <= events->time)
  {
    event->next = events;
    events = event;
  }
  else
  {
    event_t *next, *prev = NULL;

    for (next = events; next->time < event->time; next = next->next)
      prev = next;

    event->next = next;
    prev->next = event;
  }
}

//-----------------------------------------------------------------------------
void events_remove(event_t *event)
{
  event_t *ev, *prev = NULL;

  for (ev = events; ev && ev != event; ev = ev->next)
    prev = ev;

  if (NULL == prev)
  {
    events = event->next;
  }
  else if (ev)
  {
    prev->next = event->next;

    if (NULL == event->next)
      last_event = prev;
  }
}

//-----------------------------------------------------------------------------
bool events_is_planned(event_t *event)
{
  for (event_t *ev = events; ev; ev = ev->next)
  {
    if (ev == event)
      return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void events_tick(void)
{
  uint64_t cycle = get_sim_cycle();

  while (events && cycle == events->time)
  {
    event_t *event = events;
    events = events->next;
    event->callback(event);
  }
}

//-----------------------------------------------------------------------------
uint64_t events_jump(void)
{
  uint64_t delta = 0;

  if (events)
    delta = events->time - get_sim_cycle();

  return delta;
}


