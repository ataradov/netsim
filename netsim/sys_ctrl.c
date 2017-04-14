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
#include "main.h"
#include "soc.h"
#include "core.h"
#include "utils.h"
#include "sys_ctrl.h"

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void sys_ctrl_init(sys_ctrl_t *sys_ctrl)
{
  (void)sys_ctrl;
}

//-----------------------------------------------------------------------------
static uint32_t sys_ctrl_read_w(sys_ctrl_t *sys_ctrl, uint32_t addr)
{
  soc_t *soc = SOC(sys_ctrl);

  switch (addr)
  {
    case SYS_CTRL_UID:
      return SOC(sys_ctrl)->uid;

    case SYS_CTRL_ID:
      return SOC(sys_ctrl)->id;

    case SYS_CTRL_RAND:
      return rand_next();

    case SYS_CTRL_INTENSET:
    case SYS_CTRL_INTENCLR:
      return soc->core.irq_en;
  }

  return 0;
}

//-----------------------------------------------------------------------------
static void sys_ctrl_write_w(sys_ctrl_t *sys_ctrl, uint32_t addr, uint32_t data)
{
  soc_t *soc = SOC(sys_ctrl);

  switch (addr)
  {
    case SYS_CTRL_LOG:
    {
      if (data < CORE_RAM_SIZE)
      {
        char *str = (char *)&soc->core.ram[data];
        LOG_DBG(soc, "%s", str);
      }
    } break;

    case SYS_CTRL_INTENSET:
    {
      soc->core.irq_en |= data;
    } break;

    case SYS_CTRL_INTENCLR:
    {
      soc->core.irq_en &= ~data;
    } break;
  }
}

//-----------------------------------------------------------------------------
io_ops_t sys_ctrl_ops =
{
  .read_b  = (io_read_b_t)NULL,
  .read_h  = (io_read_h_t)NULL,
  .read_w  = (io_read_w_t)sys_ctrl_read_w,
  .write_b = (io_write_b_t)NULL,
  .write_h = (io_write_h_t)NULL,
  .write_w = (io_write_w_t)sys_ctrl_write_w,
};

