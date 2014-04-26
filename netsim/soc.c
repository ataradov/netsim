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
#include <string.h>
#include "soc.h"
#include "trx.h"
#include "mem.h"
#include "utils.h"
#include "medium.h"
#include "sys_ctrl.h"
#include "sys_timer.h"

/*- Types -------------------------------------------------------------------*/
typedef uint8_t    (soc_read_b_t)(void *, uint32_t);
typedef uint16_t   (soc_read_h_t)(void *, uint32_t);
typedef uint32_t   (soc_read_w_t)(void *, uint32_t);
typedef void       (soc_write_b_t)(void *, uint32_t, uint8_t);
typedef void       (soc_write_h_t)(void *, uint32_t, uint16_t);
typedef void       (soc_write_w_t)(void *, uint32_t, uint32_t);

typedef struct
{
  soc_read_b_t     *read_b;
  soc_read_h_t     *read_h;
  soc_read_w_t     *read_w;
  soc_write_b_t    *write_b;
  soc_write_h_t    *write_h;
  soc_write_w_t    *write_w;
} soc_peripheral_t;

/*- Prototypes --------------------------------------------------------------*/
static uint8_t soc_unhandled_read_b(void *per, uint32_t addr);
static uint16_t soc_unhandled_read_h(void *per, uint32_t addr);
static uint32_t soc_unhandled_read_w(void *per, uint32_t addr);
static void soc_unhandled_write_b(void *per, uint32_t addr, uint8_t data);
static void soc_unhandled_write_h(void *per, uint32_t addr, uint16_t data);
static void soc_unhandled_write_w(void *per, uint32_t addr, uint32_t data);

/*- Variables ---------------------------------------------------------------*/
static soc_peripheral_t soc_peripherals[SOC_PERIPHERALS_SIZE];

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void soc_setup(void)
{
  core_setup();

  for (int i = 0; i < SOC_PERIPHERALS_SIZE; i++)
  {
    soc_peripherals[i].read_b  = soc_unhandled_read_b;
    soc_peripherals[i].read_h  = soc_unhandled_read_h;
    soc_peripherals[i].read_w  = soc_unhandled_read_w;
    soc_peripherals[i].write_b = soc_unhandled_write_b;
    soc_peripherals[i].write_h = soc_unhandled_write_h;
    soc_peripherals[i].write_w = soc_unhandled_write_w;
  }

  soc_peripherals[SOC_ID_MEM].read_b        = (soc_read_b_t *)mem_read_b;
  soc_peripherals[SOC_ID_MEM].read_h        = (soc_read_h_t *)mem_read_h;
  soc_peripherals[SOC_ID_MEM].read_w        = (soc_read_w_t *)mem_read_w;
  soc_peripherals[SOC_ID_MEM].write_b       = (soc_write_b_t *)mem_write_b;
  soc_peripherals[SOC_ID_MEM].write_h       = (soc_write_h_t *)mem_write_h;
  soc_peripherals[SOC_ID_MEM].write_w       = (soc_write_w_t *)mem_write_w;

  soc_peripherals[SOC_ID_SYS_CTRL].read_w   = (soc_read_w_t *)sys_ctrl_read_w;

  soc_peripherals[SOC_ID_SYS_TIMER].read_w  = (soc_read_w_t *)sys_timer_read_w;
  soc_peripherals[SOC_ID_SYS_TIMER].write_w = (soc_write_w_t *)sys_timer_write_w;

  soc_peripherals[SOC_ID_TRX].read_b        = (soc_read_b_t *)trx_read_b;
  soc_peripherals[SOC_ID_TRX].read_w        = (soc_read_w_t *)trx_read_w;
  soc_peripherals[SOC_ID_TRX].write_b       = (soc_write_b_t *)trx_write_b;
  soc_peripherals[SOC_ID_TRX].write_w       = (soc_write_w_t *)trx_write_w;
}

//-----------------------------------------------------------------------------
void soc_init(soc_t *soc)
{
  soc->peripherals[SOC_ID_MEM]       = soc->mem;
  soc->peripherals[SOC_ID_SYS_CTRL]  = &soc->sys_ctrl;
  soc->peripherals[SOC_ID_SYS_TIMER] = &soc->sys_timer;
  soc->peripherals[SOC_ID_TRX]       = &soc->trx;

  soc->uid = g_sim.uid++;

  soc->core.soc = soc;
  soc->core.name = soc->name;
  soc->core.flash = (uint16_t *)soc->mem;
  core_init(&soc->core);

  soc->core.r[SP] = MEM_SIZE;

  soc->trx.name = soc->name;
  soc->trx.x = soc->x;
  soc->trx.y = soc->y;
  trx_init(&soc->trx);

  soc->sys_ctrl.soc = soc;
  sys_ctrl_init(&soc->sys_ctrl);

  soc->sys_timer.soc = soc;
  sys_timer_init(&soc->sys_timer);

  queue_add((queue_t **)&g_sim.trxs, (queue_t *)&soc->trx);
}

//-----------------------------------------------------------------------------
void soc_clk(soc_t *soc)
{
  core_clk(&soc->core);
}

//-----------------------------------------------------------------------------
uint8_t soc_read_b(soc_t *soc, uint32_t addr)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  return soc_peripherals[id].read_b(soc->peripherals[id], addr);
}

//-----------------------------------------------------------------------------
uint16_t soc_read_h(soc_t *soc, uint32_t addr)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  return soc_peripherals[id].read_h(soc->peripherals[id], addr);
}

//-----------------------------------------------------------------------------
uint32_t soc_read_w(soc_t *soc, uint32_t addr)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  return soc_peripherals[id].read_w(soc->peripherals[id], addr);
}

//-----------------------------------------------------------------------------
void soc_write_b(soc_t *soc, uint32_t addr, uint8_t data)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  soc_peripherals[id].write_b(soc->peripherals[id], addr, data);
}

//-----------------------------------------------------------------------------
void soc_write_h(soc_t *soc, uint32_t addr, uint16_t data)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  soc_peripherals[id].write_h(soc->peripherals[id], addr, data);
}

//-----------------------------------------------------------------------------
void soc_write_w(soc_t *soc, uint32_t addr, uint32_t data)
{
  uint8_t id = addr >> SOC_PERIPHERAL_OFFSET;
  soc_peripherals[id].write_w(soc->peripherals[id], addr, data);
}

//-----------------------------------------------------------------------------
static uint8_t soc_unhandled_read_b(void *per, uint32_t addr)
{
  error("unhandled byte read @ 0x%08x", addr);
  (void)per;
  return 0;
}

//-----------------------------------------------------------------------------
static uint16_t soc_unhandled_read_h(void *per, uint32_t addr)
{
  error("unhandled halfword read @ 0x%08x", addr);
  (void)per;
  return 0;
}

//-----------------------------------------------------------------------------
static uint32_t soc_unhandled_read_w(void *per, uint32_t addr)
{
  error("unhandled word read @ 0x%08x", addr);
  (void)per;
  return 0;
}

//-----------------------------------------------------------------------------
static void soc_unhandled_write_b(void *per, uint32_t addr, uint8_t data)
{
  error("unhandled byte write @ 0x%08x = 0x%02x", addr, data);
  (void)per;
}

//-----------------------------------------------------------------------------
static void soc_unhandled_write_h(void *per, uint32_t addr, uint16_t data)
{
  error("unhandled halfword write @ 0x%08x = 0x%04x", addr, data);
  (void)per;
}

//-----------------------------------------------------------------------------
static void soc_unhandled_write_w(void *per, uint32_t addr, uint32_t data)
{
  error("unhandled word write @ 0x%08x = 0x%08x", addr, data);
/*
  if (addr == 0xff000000)
    error("unhandled word write @ 0x%08x = 0x%08x", addr, data);
  else
    printf("%lu\r\n", (unsigned long)data);
*/
  (void)per;
}

