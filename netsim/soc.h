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

#ifndef _SOC_H_
#define _SOC_H_

/*- Includes ----------------------------------------------------------------*/
#include "trx.h"
#include "sys_ctrl.h"
#include "sys_timer.h"
#include "utils.h"
#include "core.h"

/*- Definitions -------------------------------------------------------------*/
#define SOC_PERIPHERALS_SIZE   256
#define SOC_PERIPHERAL_OFFSET  24
#define SOC_PERIPHERAL_MASK    0x00ffffff

#define SOC(x)                 ((soc_t *)((x)->soc))

/*- Types -------------------------------------------------------------------*/
typedef struct soc_t
{
  queue_t      queue;

  char         *name;
  float        x;
  float        y;
  long         id;
  char         *path;

  long         uid;
  core_t       core;
  sys_ctrl_t   sys_ctrl;
  sys_timer_t  sys_timer[4];
  trx_t        trx;

  void         *peripherals[SOC_PERIPHERALS_SIZE];
} soc_t;

enum
{
  SOC_ID_SYS_CTRL      = 0x01,
  SOC_ID_SYS_TIMER_0   = 0x02,
  SOC_ID_SYS_TIMER_1   = 0x03,
  SOC_ID_SYS_TIMER_2   = 0x04,
  SOC_ID_SYS_TIMER_3   = 0x05,
  SOC_ID_TRX           = 0x40,
};

enum
{
  SOC_IRQ_TRX          = 0,
  SOC_IRQ_SYS_TIMER_0  = 1,
  SOC_IRQ_SYS_TIMER_1  = 2,
  SOC_IRQ_SYS_TIMER_2  = 3,
  SOC_IRQ_SYS_TIMER_3  = 4,
};

/*- Prototypes --------------------------------------------------------------*/
void soc_setup(void);
void soc_init(soc_t *soc);
void soc_clk(soc_t *soc);
void soc_irq_set(soc_t *soc, int irq);
void soc_irq_clear(soc_t *soc, int irq);

uint8_t soc_read_b(soc_t *soc, uint32_t addr);
uint16_t soc_read_h(soc_t *soc, uint32_t addr);
uint32_t soc_read_w(soc_t *soc, uint32_t addr);
void soc_write_b(soc_t *soc, uint32_t addr, uint8_t data);
void soc_write_h(soc_t *soc, uint32_t addr, uint16_t data);
void soc_write_w(soc_t *soc, uint32_t addr, uint32_t data);

#endif // _SOC_H_

