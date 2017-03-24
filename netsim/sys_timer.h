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

#ifndef _SYS_TIMER_H_
#define _SYS_TIMER_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include "io_ops.h"
#include "events.h"

/*- Types -------------------------------------------------------------------*/
enum
{
  SYS_TIMER_CONTROL    = 0x00,
  SYS_TIMER_PERIOD     = 0x04,
  SYS_TIMER_COUNTER    = 0x08,
  SYS_TIMER_INTENCLR   = 0x0c,
  SYS_TIMER_INTENSET   = 0x10,
  SYS_TIMER_INTMASK    = 0x14,
  SYS_TIMER_INTFLAG    = 0x18,
};

enum
{
  SYS_TIMER_INTFLAG_COUNT      = (1 << 0),
};

typedef struct
{
  void         *soc;
  event_t      event;
  int          irq;

  struct
  {
    uint32_t   control;
    uint32_t   period;
    uint32_t   counter;
    uint32_t   intenclr;
    uint32_t   intenset;
    uint32_t   intmask;
    uint32_t   intflag;
  } reg;
} sys_timer_t;

/*- Prototypes --------------------------------------------------------------*/
void sys_timer_init(sys_timer_t *sys_timer);

/*- Variables ---------------------------------------------------------------*/
extern io_ops_t sys_timer_ops;

#endif // _SYS_TIMER_H_

