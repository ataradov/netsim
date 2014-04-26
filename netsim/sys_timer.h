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

#ifndef _SYS_TIMER_H_
#define _SYS_TIMER_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include "events.h"

/*- Types -------------------------------------------------------------------*/
enum
{
  SYS_TIMER_PERIOD     = 0x00,
  SYS_TIMER_COUNTER    = 0x04,
  SYS_TIMER_REG_MASK   = 0xff,
};

typedef struct
{
  void         *soc;
  event_t      event;

  struct
  {
    uint32_t   period;         // 0x00
    uint32_t   counter;        // 0x04
  } reg;
} sys_timer_t;

/*- Prototypes --------------------------------------------------------------*/
void sys_timer_init(sys_timer_t *sys_timer);
uint32_t sys_timer_read_w(sys_timer_t *sys_timer, uint32_t addr);
void sys_timer_write_w(sys_timer_t *sys_timer, uint32_t addr, uint32_t data);

#endif // _SYS_TIMER_H_

