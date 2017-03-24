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
#include "io_ops.h"
#include "soc.h"
#include "events.h"
#include "sys_timer.h"

/*- Prototypes --------------------------------------------------------------*/
static void sys_timer_event_cb(event_t *event);

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void sys_timer_init(sys_timer_t *sys_timer)
{
  sys_timer->reg.control = 0;
  sys_timer->reg.period = 0;
  sys_timer->reg.counter = 0;
  sys_timer->reg.intenclr = 0;
  sys_timer->reg.intenset = 0;
  sys_timer->reg.intmask = 0;
  sys_timer->reg.intflag = 0;
}

//-----------------------------------------------------------------------------
static uint32_t sys_timer_read_w(sys_timer_t *sys_timer, uint32_t addr)
{
  uint32_t *m = (uint32_t *)&sys_timer->reg;
  return m[addr >> 2];
}

//-----------------------------------------------------------------------------
static void sys_timer_write_w(sys_timer_t *sys_timer, uint32_t addr, uint32_t data)
{
  switch (addr)
  {
    case SYS_TIMER_CONTROL:
    {
    } break;

    case SYS_TIMER_PERIOD:
    {
      sys_timer->reg.period = data;

      if (events_is_planned(&sys_timer->event))
        events_remove(&sys_timer->event);

      if (sys_timer->reg.period)
      {
        sys_timer->event.timeout = sys_timer->reg.period;
        sys_timer->event.callback = sys_timer_event_cb;
        sys_timer->event.data = (void *)sys_timer;
        events_add(&sys_timer->event);
      }
    } break;

    case SYS_TIMER_COUNTER:
    {
      sys_timer->reg.counter = data;
    } break;

    case SYS_TIMER_INTENCLR:
    {
      sys_timer->reg.intmask &= ~data;
      sys_timer->reg.intenclr = sys_timer->reg.intmask;
      sys_timer->reg.intenset = sys_timer->reg.intmask;
    } break;

    case SYS_TIMER_INTENSET:
    {
      sys_timer->reg.intmask |= data;
      sys_timer->reg.intenclr = sys_timer->reg.intmask;
      sys_timer->reg.intenset = sys_timer->reg.intmask;
    } break;

    case SYS_TIMER_INTMASK:
    {
      sys_timer->reg.intmask = data;
      sys_timer->reg.intenclr = sys_timer->reg.intmask;
      sys_timer->reg.intenset = sys_timer->reg.intmask;
    } break;

    case SYS_TIMER_INTFLAG:
    {
      sys_timer->reg.intflag &= ~data;

      if (0 == (sys_timer->reg.intflag & sys_timer->reg.intmask))
        soc_irq_clear(SOC(sys_timer), sys_timer->irq);
    } break;
  }
}

//-----------------------------------------------------------------------------
static void sys_timer_event_cb(event_t *event)
{
  sys_timer_t *sys_timer = (sys_timer_t *)event->data;

  sys_timer->reg.counter++;
  sys_timer->reg.intflag |= SYS_TIMER_INTFLAG_COUNT;

  if (sys_timer->reg.intflag & sys_timer->reg.intmask)
    soc_irq_set(SOC(sys_timer), sys_timer->irq);

  events_add(event);
}

//-----------------------------------------------------------------------------
io_ops_t sys_timer_ops =
{
  .read_b  = (io_read_b_t)NULL,
  .read_h  = (io_read_h_t)NULL,
  .read_w  = (io_read_w_t)sys_timer_read_w,
  .write_b = (io_write_b_t)NULL,
  .write_h = (io_write_h_t)NULL,
  .write_w = (io_write_w_t)sys_timer_write_w,
};

