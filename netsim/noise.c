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
#include "main.h"
#include "utils.h"
#include "noise.h"
#include "events.h"

/*- Prototypes --------------------------------------------------------------*/
static void noise_change_state(noise_t *noise);

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void noise_init(noise_t *noise)
{
  if (0 == noise->off)
  {
    noise->active = true;
  }
  else if (0 == noise->on)
  {
    NOISE_DBG(noise, "warning: noise source is always off");
    noise->active = false;
  }
  else
  {
    noise->active = false;
    noise_change_state(noise);
  }

  NOISE_DBG(noise, "started (%.2f, %.2f), on = %ld, off = %ld",
      noise->x, noise->y, noise->on, noise->off);
}

//-----------------------------------------------------------------------------
static void noise_event_cb(event_t *event)
{
  noise_t *noise = (noise_t *)event->data;

  noise_change_state(noise);
}

//-----------------------------------------------------------------------------
static void noise_change_state(noise_t *noise)
{
  if (noise->active)
  {
    noise->active = false;
    noise->event.timeout = noise->off;
    NOISE_DBG(noise, "now off for %ld us", noise->off);
  }
  else
  {
    noise->active = true;
    noise->event.timeout = noise->on;
    NOISE_DBG(noise, "now on for %ld us", noise->on);
  }

  noise->event.callback = noise_event_cb;
  noise->event.data = (void *)noise;
  events_add(&noise->event);
}

