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
#include <string.h>
#include "io_ops.h"
#include "soc.h"
#include "trx.h"
#include "main.h"
#include "utils.h"
#include "medium.h"
#include "sys_ctrl.h"
#include "sys_timer.h"

/*- Definitions -------------------------------------------------------------*/
#define GET_PC(soc)    (((soc)->core.r[15] & ~1) - 2)

/*- Prototypes --------------------------------------------------------------*/
static uint8_t soc_unhandled_read_b(soc_t *soc, uint32_t addr);
static uint16_t soc_unhandled_read_h(soc_t *soc, uint32_t addr);
static uint32_t soc_unhandled_read_w(soc_t *soc, uint32_t addr);
static void soc_unhandled_write_b(soc_t *soc, uint32_t addr, uint8_t data);
static void soc_unhandled_write_h(soc_t *soc, uint32_t addr, uint16_t data);
static void soc_unhandled_write_w(soc_t *soc, uint32_t addr, uint32_t data);

/*- Variables ---------------------------------------------------------------*/
static io_ops_t soc_peripherals[SOC_PERIPHERALS_SIZE];
static io_ops_t soc_unhandled_ops =
{
  .read_b  = (io_read_b_t)soc_unhandled_read_b,
  .read_h  = (io_read_h_t)soc_unhandled_read_h,
  .read_w  = (io_read_w_t)soc_unhandled_read_w,
  .write_b = (io_write_b_t)soc_unhandled_write_b,
  .write_h = (io_write_h_t)soc_unhandled_write_h,
  .write_w = (io_write_w_t)soc_unhandled_write_w,
};

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void soc_setup(void)
{
  core_setup();

  for (int i = 0; i < SOC_PERIPHERALS_SIZE; i++)
    soc_peripherals[i] = soc_unhandled_ops;

  soc_peripherals[SOC_ID_SYS_CTRL]    = sys_ctrl_ops;
  soc_peripherals[SOC_ID_SYS_TIMER_0] = sys_timer_ops;
  soc_peripherals[SOC_ID_SYS_TIMER_1] = sys_timer_ops;
  soc_peripherals[SOC_ID_SYS_TIMER_2] = sys_timer_ops;
  soc_peripherals[SOC_ID_SYS_TIMER_3] = sys_timer_ops;
  soc_peripherals[SOC_ID_TRX]         = trx_ops;
}

//-----------------------------------------------------------------------------
void soc_init(soc_t *soc)
{
  for (int i = 0; i < SOC_PERIPHERALS_SIZE; i++)
    soc->peripherals[i] = soc;

  soc->peripherals[SOC_ID_SYS_CTRL]    = &soc->sys_ctrl;
  soc->peripherals[SOC_ID_SYS_TIMER_0] = &soc->sys_timer[0];
  soc->peripherals[SOC_ID_SYS_TIMER_1] = &soc->sys_timer[1];
  soc->peripherals[SOC_ID_SYS_TIMER_2] = &soc->sys_timer[2];
  soc->peripherals[SOC_ID_SYS_TIMER_3] = &soc->sys_timer[3];
  soc->peripherals[SOC_ID_TRX]         = &soc->trx;

  soc->core.soc = soc;
  soc->core.name = soc->name;
  core_init(&soc->core);

  soc->trx.soc = soc;
  soc->trx.name = soc->name;
  soc->trx.uid = soc->uid;
  soc->trx.x = soc->x;
  soc->trx.y = soc->y;
  soc->trx.irq = SOC_IRQ_TRX;
  trx_init(&soc->trx);

  soc->sys_ctrl.soc = soc;
  sys_ctrl_init(&soc->sys_ctrl);

  for (int i = 0; i < 4; i++)
  {
    soc->sys_timer[i].soc = soc;
    soc->sys_timer[i].irq = SOC_IRQ_SYS_TIMER_0 + i;
    sys_timer_init(&soc->sys_timer[i]);
  }

  queue_add(&g_sim.trxs, &soc->trx);
}

//-----------------------------------------------------------------------------
void soc_clk(soc_t *soc)
{
  core_clk(&soc->core);
}

//-----------------------------------------------------------------------------
void soc_irq_set(soc_t *soc, int irq)
{
  core_irq_set(&soc->core, irq);
}

//-----------------------------------------------------------------------------
void soc_irq_clear(soc_t *soc, int irq)
{
  core_irq_clear(&soc->core, irq);
}

//-----------------------------------------------------------------------------
uint8_t soc_read_b(soc_t *soc, uint32_t addr)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  return soc_peripherals[id].read_b(soc->peripherals[id], addr & SOC_PERIPHERAL_MASK);
}

//-----------------------------------------------------------------------------
uint16_t soc_read_h(soc_t *soc, uint32_t addr)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  return soc_peripherals[id].read_h(soc->peripherals[id], addr & SOC_PERIPHERAL_MASK);
}

//-----------------------------------------------------------------------------
uint32_t soc_read_w(soc_t *soc, uint32_t addr)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  return soc_peripherals[id].read_w(soc->peripherals[id], addr & SOC_PERIPHERAL_MASK);
}

//-----------------------------------------------------------------------------
void soc_write_b(soc_t *soc, uint32_t addr, uint8_t data)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  soc_peripherals[id].write_b(soc->peripherals[id], addr & SOC_PERIPHERAL_MASK, data);
}

//-----------------------------------------------------------------------------
void soc_write_h(soc_t *soc, uint32_t addr, uint16_t data)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  soc_peripherals[id].write_h(soc->peripherals[id], addr & SOC_PERIPHERAL_MASK, data);
}

//-----------------------------------------------------------------------------
void soc_write_w(soc_t *soc, uint32_t addr, uint32_t data)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  soc_peripherals[id].write_w(soc->peripherals[id], addr & SOC_PERIPHERAL_MASK, data);
}

//-----------------------------------------------------------------------------
static uint8_t soc_unhandled_read_b(soc_t *soc, uint32_t addr)
{
  error("%s: 0x%08x: unhandled byte read @ 0x%08x",
      soc->name, GET_PC(soc), addr);
  return 0;
}

//-----------------------------------------------------------------------------
static uint16_t soc_unhandled_read_h(soc_t *soc, uint32_t addr)
{
  error("%s: 0x%08x: unhandled halfword read @ 0x%08x",
      soc->name, GET_PC(soc), addr);
  return 0;
}

//-----------------------------------------------------------------------------
static uint32_t soc_unhandled_read_w(soc_t *soc, uint32_t addr)
{
  error("%s: 0x%08x: unhandled word read @ 0x%08x",
      soc->name, GET_PC(soc), addr);
  return 0;
}

//-----------------------------------------------------------------------------
static void soc_unhandled_write_b(soc_t *soc, uint32_t addr, uint8_t data)
{
  error("%s: 0x%08x: unhandled byte write @ 0x%08x = 0x%02x",
      soc->name, GET_PC(soc), addr, data);
}

//-----------------------------------------------------------------------------
static void soc_unhandled_write_h(soc_t *soc, uint32_t addr, uint16_t data)
{
  error("%s: 0x%08x: unhandled halfword write @ 0x%08x = 0x%04x",
      soc->name, GET_PC(soc), addr, data);
}

//-----------------------------------------------------------------------------
static void soc_unhandled_write_w(soc_t *soc, uint32_t addr, uint32_t data)
{
  error("%s: 0x%08x: unhandled word write @ 0x%08x = 0x%08x",
      soc->name, GET_PC(soc), addr, data);
}


