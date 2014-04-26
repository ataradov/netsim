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
#include "sys_timer.h"
#include "events.h"

/*- Prototypes --------------------------------------------------------------*/
static void sys_timer_event_cb(event_t *event);

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void sys_timer_init(sys_timer_t *sys_timer)
{
  sys_timer->reg.period = 0;
  sys_timer->reg.counter = 0;
}

//-----------------------------------------------------------------------------
uint32_t sys_timer_read_w(sys_timer_t *sys_timer, uint32_t addr)
{
  uint32_t *m = (uint32_t *)&sys_timer->reg;
  return m[(addr & SYS_TIMER_REG_MASK) >> 2];
}

//-----------------------------------------------------------------------------
void sys_timer_write_w(sys_timer_t *sys_timer, uint32_t addr, uint32_t data)
{
  uint32_t *m = (uint32_t *)&sys_timer->reg;
  m[(addr & SYS_TIMER_REG_MASK) >> 2] = data;

  if (SYS_TIMER_PERIOD == (addr & SYS_TIMER_REG_MASK))
  {
    if (events_is_planned(&sys_timer->event))
      events_remove(&sys_timer->event);

    if (data)
    {
      sys_timer->event.timeout = data;
      sys_timer->event.callback = sys_timer_event_cb;
      sys_timer->event.data = (void *)sys_timer;
      events_add(&sys_timer->event);
    }
  }
}

//-----------------------------------------------------------------------------
static void sys_timer_event_cb(event_t *event)
{
  sys_timer_t *sys_timer = (sys_timer_t *)event->data;

  sys_timer->reg.counter++;

  events_add(event);
}

