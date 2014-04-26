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
#include "utils.h"
#include "sys_ctrl.h"

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void sys_ctrl_init(sys_ctrl_t *sys_ctrl)
{
  (void)sys_ctrl;
}

//-----------------------------------------------------------------------------
uint32_t sys_ctrl_read_w(sys_ctrl_t *sys_ctrl, uint32_t addr)
{
  switch (addr & SYS_CTRL_REG_MASK)
  {
    case SYS_CTRL_UID:
      return SOC(sys_ctrl)->uid;

    case SYS_CTRL_ID:
      return SOC(sys_ctrl)->id;

    case SYS_CTRL_RAND:
      return rand_next();
  }

  return 0;
}

