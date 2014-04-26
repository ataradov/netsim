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
#include <stdint.h>
#include "mem.h"

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
uint8_t mem_read_b(uint8_t *mem, uint32_t addr)
{
  return mem[addr & MEM_MASK];
}

//-----------------------------------------------------------------------------
uint16_t mem_read_h(uint16_t *mem, uint32_t addr)
{
  return mem[(addr & MEM_MASK) >> 1];
}

//-----------------------------------------------------------------------------
uint32_t mem_read_w(uint32_t *mem, uint32_t addr)
{
  return mem[(addr & MEM_MASK) >> 2];
}

//-----------------------------------------------------------------------------
void mem_write_b(uint8_t *mem, uint32_t addr, uint8_t data)
{
  mem[addr & MEM_MASK] = data;
}

//-----------------------------------------------------------------------------
void mem_write_h(uint16_t *mem, uint32_t addr, uint16_t data)
{
  mem[(addr & MEM_MASK) >> 1] = data;
}

//-----------------------------------------------------------------------------
void mem_write_w(uint32_t *mem, uint32_t addr, uint32_t data)
{
  mem[(addr & MEM_MASK) >> 2] = data;
}

