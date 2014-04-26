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

#ifndef _I_COMMON_H_
#define _I_COMMON_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "core.h"

/*- Types -------------------------------------------------------------------*/
typedef void (handler_t)(core_t *);

/*- Prototypes --------------------------------------------------------------*/
void error(const char *fmt, ...);
void i_undefined(core_t *core);

uint8_t soc_read_b(void *soc, uint32_t addr);
uint16_t soc_read_h(void *soc, uint32_t addr);
uint32_t soc_read_w(void *soc, uint32_t addr);
void soc_write_b(void *soc, uint32_t addr, uint8_t data);
void soc_write_h(void *soc, uint32_t addr, uint16_t data);
void soc_write_w(void *soc, uint32_t addr, uint32_t data);

/*- Variables ---------------------------------------------------------------*/
handler_t *core_hash_table[0x10000];

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static inline bool overflow(uint32_t a, uint32_t b, uint32_t r)
{
  return ((a ^ b ^ 0x80000000UL) & (a ^ r)) >> 31;
}

//-----------------------------------------------------------------------------
static inline bool carry(uint32_t a, bool c, uint32_t r)
{
  return c ? r <= a : r < a;
}

//-----------------------------------------------------------------------------
static inline void CORE_DBG(core_t *core, char *fmt, ...)
{
  (void)core;
  (void)fmt;
}

#endif // _I_COMMON_H_

