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
#include "i_common.h"

/*- Definitions -------------------------------------------------------------*/
#define GET32_IMM4(x)      ((x) & 0x0f)
#define GET32_IMM8(x)      ((x) & 0xff)
#define GET32_IMM16(x)     ((((x) >> 4) & 0xf000) | ((x) & 0xfff))
#define GET32_RD(x)        (((x) >> 8) & 0x0f)
#define GET32_RA(x)        (((x) >> 16) & 0x0f)
#define GET32_IMM10(x)     (((x) >> 16) & 0x3ff)
#define GET32_IMM11(x)     ((x) & 0x7ff)
#define GET32_S(x)         (((x) >> 26) & 1)
#define GET32_J1(x)        (((x) >> 13) & 1)
#define GET32_J2(x)        (((x) >> 11) & 1)

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void i_32bit(core_t *core)
{
  uint32_t opcode;

  opcode = ((uint32_t)core->opcode << 16) | core->flash[core->r[PC] >> 1];
  core->r[PC] += 2;

  if (0xf000d000 == (opcode & 0xf800d000)) // BL  #imm
  {
    uint32_t imm10 = GET32_IMM10(opcode);
    uint32_t imm11 = GET32_IMM11(opcode);
    uint32_t j1 = GET32_J1(opcode);
    uint32_t j2 = GET32_J2(opcode);
    uint32_t s = GET32_S(opcode);
    uint32_t i1, i2, target;

    i1 = ~(j1 ^ s) & 1;
    i2 = ~(j2 ^ s) & 1;

    target = (s << 24) | (i1 << 23) | (i2 << 22) | (imm10 << 12) | (imm11 << 1);
    target |= s ? 0xff000000 : 0x00000000;
    target += core->r[PC];

    CORE_DBG(core, "bl\t0x%08x", target);

    core->r[LR] = core->r[PC] | 1;
    core->r[PC] = target;
  }

  else if (0xf3ef8000 == (opcode & 0xfffff000)) // MRS  Rd, spec_reg
  {
    int rd = GET32_RD(opcode);
    uint32_t imm = GET32_IMM8(opcode);

    CORE_DBG(core, "mrs\tr%d, 0x%02x", rd, imm);
    // TODO: implement

    error("%s: mrs not implemented at 0x%08x", core->name, core->r[PC]-2);
  }

  else if (0xf3808800 == (opcode & 0xfff0ff00)) // MSR  spec_reg, Ra
  {
    int ra = GET32_RA(opcode);
    uint32_t imm = GET32_IMM8(opcode);

    CORE_DBG(core, "msr\t0x%02x, r%d", imm, ra);
    // TODO: implement

    error("%s: msr not implemented at 0x%08x", core->name, core->r[PC]-2);
  }

  else if (0xf7f0a000 == (opcode & 0xfff0f000)) // UDF.W  #imm16
  {
    uint32_t imm = GET32_IMM16(opcode);

    CORE_DBG(core, "udf.w\t0x%04x", imm);
    // TODO: interrupt

    error("%s: udf.w not implemented at 0x%08x", core->name, core->r[PC]-2);
  }

  else if (0xf3bf8f40 == (opcode & 0xfffffff0)) // DSB  #imm4
  {
    uint32_t imm = GET32_IMM4(opcode);

    CORE_DBG(core, "dsb\t%d", imm);
  }

  else if (0xf3bf8f50 == (opcode & 0xfffffff0)) // DMB  #imm4
  {
    uint32_t imm = GET32_IMM4(opcode);

    CORE_DBG(core, "dmb\t%d", imm);
  }

  else if (0xf3bf8f60 == (opcode & 0xfffffff0)) // ISB  #imm4
  {
    uint32_t imm = GET32_IMM4(opcode);

    CORE_DBG(core, "isb\t%d", imm);
  }

  else
  {
    i_undefined(core);
  }
}

