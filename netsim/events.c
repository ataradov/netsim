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

/*- Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "events.h"

/*- Variables ---------------------------------------------------------------*/
static event_t *events = NULL;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void events_add(event_t *event)
{
  if (events)
  {
    event_t *next = events;
    event_t *prev = NULL;
    int time = 0;
    int diff;

    while (next && (time + next->counter) < event->timeout)
    {
      time += next->counter;
      prev = next;
      next = next->next;
    }

    diff = event->timeout - time;

    if (next)
    {
      event->next = next;
      event->counter = diff;
      next->counter -= diff;
      if (prev)
        prev->next = event;
      else
        events = event;
    }
    else
    {
      event->next = NULL;
      event->counter = diff;
      if (prev)
        prev->next = event;
      else
        events = event;
    }
  }
  else
  {
    event->next = NULL;
    event->counter = event->timeout;
    events = event;
  }
}

//-----------------------------------------------------------------------------
void events_remove(event_t *event)
{
  event_t *prev = NULL;

  for (event_t *e = events; e; e = e->next)
  {
    if (e == event)
    {
      if (prev)
        prev->next = e->next;
      else
        events = e->next;

      if (e->next)
        e->next->counter += e->counter;

      return;
    }
    prev = e;
  }
}

//-----------------------------------------------------------------------------
bool events_is_planned(event_t *event)
{
  for (event_t *e = events; e; e = e->next)
  {
    if (e == event)
      return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void events_tick(void)
{
  if (NULL == events)
    return;

  events->counter--;

  while (events && 0 == events->counter)
  {
    event_t *event = events;

    events = events->next;
    event->callback(event);
  }
}

