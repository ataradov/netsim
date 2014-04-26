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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "soc.h"
#include "core.h"
#include "utils.h"

/*- Definitions -------------------------------------------------------------*/
#define HASH_TABLE_SIZE        0x10000  // 64k
#define ARRAY_SIZE(a)          (sizeof(a) / sizeof(a[0]))

/* Instruction fields bitmaps and macros that extract them:
  _____________rrr      r = GET_R1()
  __________rrr___      r = GET_R2()
  _______rrr______      r = GET_R3()
  _______iii______      i = GET_IMM3()
  _____iiiii______      i = GET_IMM5()
  _________iiiiiii      i = GET_IMM7()
  ________iiiiiiii      i = GET_IMM8()
  _____iiiiiiiiiii      i = GET_IMM11()
  _______i________      i = GET_EXTRA_REG()
  ________r____rrr      r = GET_R1_4()
  _________rrrr___      r = GET_R2_4()
  _____rrr________      r = GET_R_IMM8()
  ____cccc________      c = GET_COND()

  ________________ ____________iiii     i = GET32_IMM4()
  ________________ ________iiiiiiii     i = GET32_IMM8()
  ____________iiii ____iiiiiiiiiiii     i = GET32_IMM16()
  ________________ ____dddd________     i = GET32_RD()
  ____________aaaa ________________     i = GET32_RA()
  ______iiiiiiiiii ________________     i = GET32_IMM10()
  ________________ _____iiiiiiiiiii     i = GET32_IMM11()
  _____s__________ ________________     i = GET32_S()
  ________________ __j_____________     i = GET32_J1()
  ________________ ____j___________     i = GET32_J2()
*/

#define GET_R1(x)          ((x) & 0x07)
#define GET_R2(x)          (((x) >> 3) & 0x07)
#define GET_R3(x)          (((x) >> 6) & 0x07)
#define GET_IMM3(x)        (((x) >> 6) & 0x07)
#define GET_IMM5(x)        (((x) >> 6) & 0x1f)
#define GET_IMM7(x)        ((x) & 0x7f)
#define GET_IMM8(x)        ((x) & 0xff)
#define GET_IMM11(x)       ((x) & 0x7ff)
#define GET_EXTRA_REG(x)   (((x) >> 8) & 1)
#define GET_R1_4(x)        ((((x) >> 4) & 0x08) | ((x) & 0x07))
#define GET_R2_4(x)        (((x) >> 3) & 0x0f)
#define GET_R_IMM8(x)      (((x) >> 8) & 0x07)
#define GET_COND(x)        (((x) >> 8) & 0x0f)

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

/*- Types -------------------------------------------------------------------*/
typedef void (handler_t)(core_t *);

typedef struct
{
  handler_t    *handler;
  uint16_t     mask;
  uint16_t     value;
} instr_t;

/*- Variables ---------------------------------------------------------------*/
static handler_t *hash[HASH_TABLE_SIZE];

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
static void i_undefined(core_t *core)
{
  error("%s: undefined instruction 0x%04x at 0x%08x", core->name, core->opcode,
      core->r[PC]-2);
}

//-----------------------------------------------------------------------------
static void i_lsls_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "lsls\tr%d, r%d, %d", r1, r2, imm);

  if (imm == 0)
  {
    res = r2v;
  }
  else
  {
    res = r2v << imm;
    core->c = (r2v >> (32-imm)) & 1;
  }

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_lsrs_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t res;

  if (0 == imm)
    imm = 32;

  CORE_DBG(core, "lsrs\tr%d, r%d, %d", r1, r2, imm);

  if (imm < 32)
  {
    res = r2v >> imm;
    core->c = (r2v >> (imm-1)) & 1;
  }
  else
  {
    res = 0;
    core->c = (r2v >> 31) & 1;
  }

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_asrs_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t res;

  if (0 == imm)
    imm = 32;

  CORE_DBG(core, "asrs\tr%d, r%d, %d", r1, r2, imm);

  if (imm < 32)
  {
    res = (int32_t)r2v >> imm;
    core->c = (r2v >> (imm-1)) & 1;
  }
  else
  {
    if (r2v & 0x80000000)
    {
      res = 0xffffffff;
      core->c = true;
    }
    else
    {
      res = 0;
      core->c = false;
    }
  }

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_adds_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t r3v = core->r[r3];
  uint32_t res;

  CORE_DBG(core, "adds\tr%d, r%d, r%d", r1, r2, r3);

  res = r2v + r3v;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r2v, false, res);
  core->v = overflow(r2v, r3v, res);

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_subs_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t r3v = core->r[r3];
  uint32_t res;

  CORE_DBG(core, "subs\tr%d, r%d, r%d", r1, r2, r3);

  res = r2v + ~r3v + 1;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r2v, true, res);
  core->v = overflow(r2v, ~r3v, res);

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_adds_imm3(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM3(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "adds\tr%d, r%d, 0x%02x", r1, r2, imm);

  res = r2v + imm;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r2v, false, res);
  core->v = overflow(r2v, imm, res);

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_subs_imm3(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM3(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "subs\tr%d, r%d, 0x%02x", r1, r2, imm);

  res = r2v + ~imm + 1;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r2v, true, res);
  core->v = overflow(r2v, ~imm, res);

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_movs_imm(core_t *core)
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode);
  uint32_t res = imm;

  CORE_DBG(core, "movs\tr%d, 0x%02x", rd, imm);

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[rd] = res;
}

//-----------------------------------------------------------------------------
static void i_cmp_imm(core_t *core)
{
  int r = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode);
  uint32_t rv = core->r[r];
  uint32_t res;

  CORE_DBG(core, "cmp\tr%d, 0x%02x", r, imm);

  res = rv + ~imm + 1;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(rv, true, res);
  core->v = overflow(rv, ~imm, res);
}

//-----------------------------------------------------------------------------
static void i_adds_imm8(core_t *core)
{
  int r = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode);
  uint32_t rv = core->r[r];
  uint32_t res;

  CORE_DBG(core, "adds\tr%d, 0x%02x", r, imm);

  res = rv + imm;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(rv, false, res);
  core->v = overflow(rv, imm, res);

  core->r[r] = res;
}

//-----------------------------------------------------------------------------
static void i_subs_imm8(core_t *core)
{
  int r = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode);
  uint32_t rv = core->r[r];
  uint32_t res;

  CORE_DBG(core, "subs\tr%d, 0x%02x", r, imm);

  res = rv + ~imm + 1;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(rv, true, res);
  core->v = overflow(rv, ~imm, res);

  core->r[r] = res;
}

//-----------------------------------------------------------------------------
static void i_ands_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "ands\tr%d, r%d", r1, r2);

  res = r1v & r2v;

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_eors_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "eors\tr%d, r%d", r1, r2);

  res = r1v ^ r2v;

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_lsls_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2] & 0xff;
  uint32_t res;

  CORE_DBG(core, "lsls\tr%d, r%d", r1, r2);

  if (r2v == 0)
  {
    res = r1v;
  }
  else if (r2v < 32)
  {
    res = r1v << r2v;
    core->c = (r1v >> (32-r2v)) & 1;
  }
  else if (r2v == 32)
  {
    res = 0;
    core->c = r1v & 1;
  }
  else
  {
    res = 0;
    core->c = false;
  }

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_lsrs_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2] & 0xff;
  uint32_t res;

  CORE_DBG(core, "lsrs\tr%d, r%d", r1, r2);

  if (r2v == 0)
  {
    res = r1v;
  }
  else if (r2v < 32)
  {
    res = r1v >> r2v;
    core->c = (r1v >> (r2v-1)) & 1;
  }
  else if (r2v == 32)
  {
    res = 0;
    core->c = r1v >> 31;
  }
  else
  {
    res = 0;
    core->c = false;
  }

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_asrs_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2] & 0xff;
  uint32_t res;

  CORE_DBG(core, "asrs\tr%d, r%d", r1, r2);

  if (r2v == 0)
  {
    res = r1v;
  }
  else if (r2v < 32)
  {
    res = (int32_t)r1v >> r2v;
    core->c = (r1v >> (r2v-1)) & 1;
  }
  else
  {
    if (r1v & 0x80000000)
    {
      res = 0xffffffff;
      core->c = true;
    }
    else
    {
      res = 0;
      core->c = false;
    }
  }

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_adcs_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "adcs\tr%d, r%d", r1, r2);

  res = r1v + r2v + core->c;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r1v, core->c, res);
  core->v = overflow(r1v, r2v, res);

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_sbcs_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "sbcs\tr%d, r%d", r1, r2);

  res = r1v + ~r2v + core->c;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r1v, core->c, res);
  core->v = overflow(r1v, ~r2v, res);

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_rors_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2] & 0xff;
  uint32_t res = r1v;

  CORE_DBG(core, "rors\tr%d, r%d", r1, r2);

  if (r2v > 0)
  {
    r2v &= 0x1f;

    if (r2v > 0)
    {
      res = (r1v >> r2v) | (r1v << (32-r2v));
      core->c = (r1v >> (r2v-1)) & 1;
    }
    else
    {
      core->c = r1v >> 31;
    }
  }

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_tst_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "tst\tr%d, r%d", r1, r2);

  res = r1v & r2v;

  core->n = (res >> 31);
  core->z = res == 0;
}

//-----------------------------------------------------------------------------
static void i_rsbs_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "rsbs\tr%d, r%d", r1, r2);

  res = ~r2v + 0 + 1;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(~r2v, true, res);
  core->v = overflow(~r2v, 0, res);

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_cmp_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "cmp\tr%d, r%d", r1, r2);

  res = r1v + ~r2v + 1;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r1v, true, res);
  core->v = overflow(r1v, ~r2v, res);
}

//-----------------------------------------------------------------------------
static void i_cmn_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "cmn\tr%d, r%d", r1, r2);

  res = r1v + r2v;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r1v, false, res);
  core->v = overflow(r1v, r2v, res);
}

//-----------------------------------------------------------------------------
static void i_orrs_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "orrs\tr%d, r%d", r1, r2);

  res = r1v | r2v;

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_muls_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t res;

  CORE_DBG(core, "muls\tr%d, r%d, r%d", r1, r2, r1);

  res = core->r[r1] * core->r[r2];

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_bics_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "bics\tr%d, r%d", r1, r2);

  res = r1v & ~r2v;

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_mvns_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "mvns\tr%d, r%d", r1, r2);

  res = ~r2v;

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[r1] = res;
}

//-----------------------------------------------------------------------------
static void i_add_reg4(core_t *core)
{
  int r1 = GET_R1_4(core->opcode);
  int r2 = GET_R2_4(core->opcode);

  CORE_DBG(core, "add\tr%d, r%d", r1, r2);

  core->r[r1] = core->r[r1] + core->r[r2];

  if (PC == r1)
    core->r[PC] &= 0xfffffffe;
}

//-----------------------------------------------------------------------------
static void i_cmp_reg4(core_t *core)
{
  int r1 = GET_R1_4(core->opcode);
  int r2 = GET_R2_4(core->opcode);
  uint32_t r1v = core->r[r1];
  uint32_t r2v = core->r[r2];
  uint32_t res;

  CORE_DBG(core, "cmp\tr%d, r%d", r1, r2);

  res = r1v + ~r2v + 1;

  core->n = (res >> 31);
  core->z = res == 0;
  core->c = carry(r1v, true, res);
  core->v = overflow(r1v, ~r2v, res);
}

//-----------------------------------------------------------------------------
static void i_mov_reg4(core_t *core)
{
  int r1 = GET_R1_4(core->opcode);
  int r2 = GET_R2_4(core->opcode);

  CORE_DBG(core, "mov\tr%d, r%d", r1, r2);

  core->r[r1] = core->r[r2];

  if (PC == r1)
    core->r[PC] &= 0xfffffffe;
}

//-----------------------------------------------------------------------------
static void i_bx_reg4(core_t *core)
{
  int r = GET_R2_4(core->opcode);

  CORE_DBG(core, "bx\tr%d", r);

  core->r[PC] = core->r[r] & 0xfffffffe;
}

//-----------------------------------------------------------------------------
static void i_blx_reg4(core_t *core)
{
  int r = GET_R2_4(core->opcode);
  uint32_t addr;

  CORE_DBG(core, "blx\tr%d", r);

  addr = core->r[r] & 0xfffffffe;
  core->r[LR] = core->r[PC];
  core->r[PC] = addr;
}

//-----------------------------------------------------------------------------
static void i_ldr_pc(core_t *core)
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "ldr\tr%d, [PC, 0x%02x]", rd, imm);

  core->r[rd] = soc_read_w(core->soc, core->r[PC] + imm + 2);
}

//-----------------------------------------------------------------------------
static void i_str_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "str\tr%d, [r%d, r%d]", r1, r2, r3);

  soc_write_w(core->soc, core->r[r2] + core->r[r3], core->r[r1]);
}

//-----------------------------------------------------------------------------
static void i_strh_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "strh\tr%d, [r%d, r%d]", r1, r2, r3);

  soc_write_h(core->soc, core->r[r2] + core->r[r3], core->r[r1]);
}

//-----------------------------------------------------------------------------
static void i_strb_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "strb\tr%d, [r%d, r%d]", r1, r2, r3);

  soc_write_b(core->soc, core->r[r2] + core->r[r3], core->r[r1]);
}

//-----------------------------------------------------------------------------
static void i_ldrsb_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);
  uint32_t val;

  CORE_DBG(core, "ldrsb\tr%d, [r%d, r%d]", r1, r2, r3);

  val = soc_read_b(core->soc, core->r[r2] + core->r[r3]);
  core->r[r1] = (val & 0xff) | ((val & 0x80) ? 0xffffff00 : 0);
}

//-----------------------------------------------------------------------------
static void i_ldr_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "ldr\tr%d, [r%d, r%d]", r1, r2, r3);

  core->r[r1] = soc_read_w(core->soc, core->r[r2] + core->r[r3]);
}

//-----------------------------------------------------------------------------
static void i_ldrh_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "ldrh\tr%d, [r%d, r%d]", r1, r2, r3);

  core->r[r1] = soc_read_h(core->soc, core->r[r2] + core->r[r3]);
}

//-----------------------------------------------------------------------------
static void i_ldrb_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "ldrb\tr%d, [r%d, r%d]", r1, r2, r3);

  core->r[r1] = soc_read_b(core->soc, core->r[r2] + core->r[r3]);
}

//-----------------------------------------------------------------------------
static void i_ldrsh_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);
  uint32_t val;

  CORE_DBG(core, "ldrsh\tr%d, [r%d, r%d]", r1, r2, r3);

  val = soc_read_h(core->soc, core->r[r2] + core->r[r3]);
  core->r[r1] = val | ((val & 0x8000) ? 0xffff0000 : 0);
}

//-----------------------------------------------------------------------------
static void i_str_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode) * 4;

  CORE_DBG(core, "str\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  soc_write_w(core->soc, core->r[r2] + imm, core->r[r1]);
}

//-----------------------------------------------------------------------------
static void i_ldr_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode) * 4;

  CORE_DBG(core, "ldr\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  core->r[r1] = soc_read_w(core->soc, core->r[r2] + imm);
}

//-----------------------------------------------------------------------------
static void i_strb_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode);

  CORE_DBG(core, "strb\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  soc_write_b(core->soc, core->r[r2] + imm, core->r[r1]);
}

//-----------------------------------------------------------------------------
static void i_ldrb_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode);

  CORE_DBG(core, "ldrb\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  core->r[r1] = soc_read_b(core->soc, core->r[r2] + imm);
}

//-----------------------------------------------------------------------------
static void i_strh_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode) * 2;

  CORE_DBG(core, "strh\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  soc_write_h(core->soc, core->r[r2] + imm, core->r[r1]);
}

//-----------------------------------------------------------------------------
static void i_ldrh_imm(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode) * 2;

  CORE_DBG(core, "ldrh\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  core->r[r1] = soc_read_h(core->soc, core->r[r2] + imm);
}

//-----------------------------------------------------------------------------
static void i_str_r_sp_imm(core_t *core)
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "str\tr%d, [SP, 0x%02x]", rd, imm);

  soc_write_w(core->soc, core->r[SP] + imm, core->r[rd]);
}

//-----------------------------------------------------------------------------
static void i_ldr_r_sp_imm(core_t *core)
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "ldr\tr%d, [SP, 0x%02x]", rd, imm);

  core->r[rd] = soc_read_w(core->soc, core->r[SP] + imm);
}

//-----------------------------------------------------------------------------
static void i_add_r_pc_imm(core_t *core)
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "add\tr%d, PC, 0x%02x", rd, imm);

  core->r[rd] = (core->r[PC] & 0xfffffffc) + imm;
}

//-----------------------------------------------------------------------------
static void i_add_r_sp_imm(core_t *core)
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "add\tr%d, SP, 0x%02x", rd, imm);

  core->r[rd] = core->r[SP] + imm;
}

//-----------------------------------------------------------------------------
static void i_add_sp_imm(core_t *core)
{
  uint32_t imm = GET_IMM7(core->opcode) * 4;

  CORE_DBG(core, "add\tSP, SP, 0x%02x", imm);

  core->r[SP] += imm;
}

//-----------------------------------------------------------------------------
static void i_sub_sp_imm(core_t *core)
{
  uint32_t imm = GET_IMM7(core->opcode) * 4;

  CORE_DBG(core, "sub\tSP, SP, 0x%02x", imm);

  core->r[SP] -= imm;
}

//-----------------------------------------------------------------------------
static void i_sxth_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "sxth\tr%d, r%d", r1, r2);

  core->r[r1] = (r2v & 0xffff) | ((r2v & 0x8000) ? 0xffff0000 : 0);
}

//-----------------------------------------------------------------------------
static void i_sxtb_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "sxtb\tr%d, r%d", r1, r2);

  core->r[r1] = (r2v & 0xff) | ((r2v & 0x80) ? 0xffffff00 : 0);
}

//-----------------------------------------------------------------------------
static void i_uxth_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);

  CORE_DBG(core, "uxth\tr%d, r%d", r1, r2);

  core->r[r1] = core->r[r2] & 0xffff;
}

//-----------------------------------------------------------------------------
static void i_uxtb_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);

  CORE_DBG(core, "uxtb\tr%d, r%d", r1, r2);

  core->r[r1] = core->r[r2] & 0xff;
}

//-----------------------------------------------------------------------------
static void i_push(core_t *core)
{
  int list = GET_IMM8(core->opcode);
  int lr = GET_EXTRA_REG(core->opcode);
  uint32_t addr;

  CORE_DBG(core, "push\t{%d, 0x%02x}", lr, list);

  addr = core->r[SP];

  if (lr)
  {
    addr -= 4;
    soc_write_w(core->soc, addr, core->r[LR]);
  }

  for (int i = 7; i >= 0; i--)
  {
    if (list & (1 << i))
    {
      addr -= 4;
      soc_write_w(core->soc, addr, core->r[i]);
    }
  }

  core->r[SP] = addr;
}

//-----------------------------------------------------------------------------
static void i_pop(core_t *core)
{
  int list = GET_IMM8(core->opcode);
  int pc = GET_EXTRA_REG(core->opcode);
  uint32_t addr;

  CORE_DBG(core, "pop\t{%d, 0x%02x}", pc, list);

  addr = core->r[SP];

  for (int i = 0; i < 8; i++)
  {
    if (list & (1 << i))
    {
      core->r[i] = soc_read_w(core->soc, addr);
      addr += 4;
    }
  }

  if (pc)
  {
    core->r[PC] = soc_read_w(core->soc, addr) & 0xfffffffe;
    addr += 4;
  }

  core->r[SP] = addr;
}

//-----------------------------------------------------------------------------
static void i_cps(core_t *core)
{
  error("%s: cps not implemented at 0x%08x", core->name, core->r[PC]-2);
}

//-----------------------------------------------------------------------------
static void i_rev_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "rev\tr%d, r%d", r1, r2);

  core->r[r1] = ((r2v & 0x000000ff) << 24) | ((r2v & 0x0000ff00) << 8) |
                ((r2v & 0x00ff0000) >> 8) | ((r2v & 0xff000000) >> 24);
}

//-----------------------------------------------------------------------------
static void i_rev16_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "rev16\tr%d, r%d", r1, r2);

  core->r[r1] = ((r2v & 0x000000ff) << 8) | ((r2v & 0x0000ff00) >> 8) |
                ((r2v & 0x00ff0000) << 8) | ((r2v & 0xff000000) >> 8);
}

//-----------------------------------------------------------------------------
static void i_revsh_reg(core_t *core)
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "revsh\tr%d, r%d", r1, r2);

  r2v = ((r2v & 0x000000ff) << 8) | ((r2v & 0x0000ff00) >> 8);
  core->r[r1] = (r2v & 0xffff) | ((r2v & 0x8000) ? 0xffff0000 : 0);
}

//-----------------------------------------------------------------------------
static void i_bkpt_imm(core_t *core)
{
  uint32_t imm = GET_IMM8(core->opcode);

  CORE_DBG(core, "bkpt\t0x%02x", imm);
}

//-----------------------------------------------------------------------------
static void i_nop(core_t *core)
{
  CORE_DBG(core, "nop");
}

//-----------------------------------------------------------------------------
static void i_yield(core_t *core)
{
  CORE_DBG(core, "yield");
}

//-----------------------------------------------------------------------------
static void i_wfe(core_t *core)
{
  CORE_DBG(core, "wfe");
}

//-----------------------------------------------------------------------------
static void i_wfi(core_t *core)
{
  CORE_DBG(core, "wfi");
}

//-----------------------------------------------------------------------------
static void i_sev(core_t *core)
{
  CORE_DBG(core, "sev");
}

//-----------------------------------------------------------------------------
static void i_stm(core_t *core)
{
  int list = GET_IMM8(core->opcode);
  int r = GET_R_IMM8(core->opcode);
  uint32_t addr;

  CORE_DBG(core, "stm\tr%d, {0x%02x}", r, list);

  addr = core->r[r];

  for (int i = 0; i < 8; i++)
  {
    if (list & (1 << i))
    {
      soc_write_w(core->soc, addr, core->r[i]);
      addr += 4;
    }
  }

  core->r[r] = addr;
}

//-----------------------------------------------------------------------------
static void i_ldm(core_t *core)
{
  int list = GET_IMM8(core->opcode);
  int r = GET_R_IMM8(core->opcode);
  uint32_t addr;

  CORE_DBG(core, "ldm\tr%d, {0x%02x}", r, list);

  addr = core->r[r];

  for (int i = 0; i < 8; i++)
  {
    if (list & (1 << i))
    {
      core->r[i] = soc_read_w(core->soc, addr);
      addr += 4;
    }
  }

  if (0 == (list & (1 << r)))
    core->r[r] = addr;
}

//-----------------------------------------------------------------------------
static void i_b_c_imm(core_t *core)
{
  int cond = GET_COND(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 2;
  bool passed = false;

  switch (cond)
  {
    case 0x00: passed = core->z; break;
    case 0x01: passed = !core->z; break;
    case 0x02: passed = core->c; break;
    case 0x03: passed = !core->c; break;
    case 0x04: passed = core->n; break;
    case 0x05: passed = !core->n; break;
    case 0x06: passed = core->v; break;
    case 0x07: passed = !core->v; break;
    case 0x08: passed = core->c && !core->z; break;
    case 0x09: passed = !core->c || core->z; break;
    case 0x0a: passed = core->n == core->v; break;
    case 0x0b: passed = core->n != core->v; break;
    case 0x0c: passed = !core->z && (core->n == core->v); break;
    case 0x0d: passed = core->z || (core->n != core->v); break;
    default:
      error("%s: invalid condition code at 0x%08x", core->name, core->r[PC]-2);
  }

  imm |= (imm & 0x100) ? 0xfffffe00 : 0x00000000;

  char *conds[] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "?", "?"};
  CORE_DBG(core, "b%s\t0x%x [%s]", conds[cond], core->r[PC] + imm + 2, passed ? "taken" : "not taken");

  if (passed)
    core->r[PC] += imm + 2;
}

//-----------------------------------------------------------------------------
static void i_udf_imm(core_t *core)
{
  uint32_t imm = GET_IMM8(core->opcode);

  CORE_DBG(core, "udf\t0x%02x", imm);
  // TODO: interrupt

  error("%s: udf not implemented at 0x%08x", core->name, core->r[PC]-2);
}

//-----------------------------------------------------------------------------
static void i_svc_imm(core_t *core)
{
  uint32_t imm = GET_IMM8(core->opcode);

  CORE_DBG(core, "svc\t0x%02x", imm);
  // TODO: interrupt

  error("%s: svc not implemented at 0x%08x", core->name, core->r[PC]-2);
}

//-----------------------------------------------------------------------------
static void i_b_imm(core_t *core)
{
  uint32_t imm = GET_IMM11(core->opcode) * 2;

  imm |= (imm & 0x800) ? 0xfffff000 : 0x00000000;

  CORE_DBG(core, "b\t\t0x%x", core->r[PC] + imm + 2);

  core->r[PC] += imm + 2;
}

//-----------------------------------------------------------------------------
static void i_32bit(core_t *core)
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

//-----------------------------------------------------------------------------
static instr_t instructions[] =
{
  { i_lsls_imm,		0xf800, 0x0000 },
  { i_lsrs_imm,		0xf800, 0x0800 },
  { i_asrs_imm,		0xf800, 0x1000 },
  { i_adds_reg,		0xfe00, 0x1800 },
  { i_subs_reg,		0xfe00, 0x1a00 },
  { i_adds_imm3,	0xfe00, 0x1c00 },
  { i_subs_imm3,	0xfe00, 0x1e00 },
  { i_movs_imm,		0xf800, 0x2000 },
  { i_cmp_imm,		0xf800, 0x2800 },
  { i_adds_imm8,	0xf800, 0x3000 },
  { i_subs_imm8,	0xf800, 0x3800 },

  { i_ands_reg,		0xffc0, 0x4000 },
  { i_eors_reg,		0xffc0, 0x4040 },
  { i_lsls_reg,		0xffc0, 0x4080 },
  { i_lsrs_reg,		0xffc0, 0x40c0 },
  { i_asrs_reg,		0xffc0, 0x4100 },
  { i_adcs_reg,		0xffc0, 0x4140 },
  { i_sbcs_reg,		0xffc0, 0x4180 },
  { i_rors_reg,		0xffc0, 0x41c0 },
  { i_tst_reg,		0xffc0, 0x4200 },
  { i_rsbs_imm,		0xffc0, 0x4240 },
  { i_cmp_reg,		0xffc0, 0x4280 },
  { i_cmn_reg,		0xffc0, 0x42c0 },
  { i_orrs_reg,		0xffc0, 0x4300 },
  { i_muls_reg,		0xffc0, 0x4340 },
  { i_bics_reg,		0xffc0, 0x4380 },
  { i_mvns_reg,		0xffc0, 0x43c0 },

  { i_add_reg4,		0xff00, 0x4400 },
  { i_cmp_reg4,		0xff00, 0x4500 },
  { i_mov_reg4,		0xff00, 0x4600 },
  { i_bx_reg4,		0xff87, 0x4700 },
  { i_blx_reg4,		0xff87, 0x4780 },

  { i_ldr_pc,		0xf800, 0x4800 },

  { i_str_reg,		0xfe00, 0x5000 },
  { i_strh_reg,		0xfe00, 0x5200 },
  { i_strb_reg,		0xfe00, 0x5400 },
  { i_ldrsb_reg,	0xfe00, 0x5600 },
  { i_ldr_reg,		0xfe00, 0x5800 },
  { i_ldrh_reg,		0xfe00, 0x5a00 },
  { i_ldrb_reg,		0xfe00, 0x5c00 },
  { i_ldrsh_reg,	0xfe00, 0x5e00 },
  { i_str_imm,		0xf800, 0x6000 },
  { i_ldr_imm,		0xf800, 0x6800 },
  { i_strb_imm,		0xf800, 0x7000 },
  { i_ldrb_imm,		0xf800, 0x7800 },
  { i_strh_imm,		0xf800, 0x8000 },
  { i_ldrh_imm,		0xf800, 0x8800 },
  { i_str_r_sp_imm,	0xf800, 0x9000 },
  { i_ldr_r_sp_imm,	0xf800, 0x9800 },

  { i_add_r_pc_imm,	0xf800, 0xa000 },
  { i_add_r_sp_imm,	0xf800, 0xa800 },

  { i_add_sp_imm,	0xff80, 0xb000 },
  { i_sub_sp_imm,	0xff80, 0xb080 },
  { i_sxth_reg,		0xffc0, 0xb200 },
  { i_sxtb_reg,		0xffc0, 0xb240 },
  { i_uxth_reg,		0xffc0, 0xb280 },
  { i_uxtb_reg,		0xffc0, 0xb2c0 },
  { i_push,		0xfe00, 0xb400 },
  { i_pop,		0xfe00, 0xbc00 },
  { i_cps,		0xffef, 0xb662 },
  { i_rev_reg,		0xffc0, 0xba00 },
  { i_rev16_reg,	0xffc0, 0xba40 },
  { i_revsh_reg,	0xffc0, 0xbac0 },
  { i_bkpt_imm,		0xff00, 0xbe00 },
  { i_nop,		0xffff, 0xbf00 },
  { i_yield,		0xffff, 0xbf10 },
  { i_wfe,		0xffff, 0xbf20 },
  { i_wfi,		0xffff, 0xbf30 },
  { i_sev,		0xffff, 0xbf40 },

  { i_stm,		0xf800, 0xc000 },
  { i_ldm,		0xf800, 0xc800 },

  { i_b_c_imm,		0xf000, 0xd000 },
  { i_udf_imm,		0xff00, 0xde00 },
  { i_svc_imm,		0xff00, 0xdf00 },

  { i_b_imm,		0xf800, 0xe000 },

  { i_32bit,		0xf800, 0xf000 },
};

//-----------------------------------------------------------------------------
static bool is_more_specific(instr_t *i1, instr_t *i2)
{
  return ((i1->value & i2->mask) == i2->value);
}

//-----------------------------------------------------------------------------
static uint16_t distribute_bits(uint16_t d, uint16_t n)
{
  uint16_t res = 0;
  int in, id;

  for (id = 0, in = 0; id < 16; id++)
  {
    if (0 == (d & (1 << id)))
    {
      if (n & (1 << in))
        res |= 1 << id;
      in++;
    }
  }

  return res;
}

//-----------------------------------------------------------------------------
static int count_zeros(uint16_t d)
{
  int i, cnt = 0;

  for (i = 0; i < 16; i++)
  {
    if (0 == (d & (1 << i)))
      cnt++;
  }

  return cnt;
} 

//-----------------------------------------------------------------------------
void core_setup(void)
{
  int hash_index[HASH_TABLE_SIZE];
  int index, nindex, z;
  int free = 0;

  for (int i = 0; i < HASH_TABLE_SIZE; i++)
  {
    hash_index[i] = -1;
    hash[i] = i_undefined;
  }

  for (int i = 0; i < (int)ARRAY_SIZE(instructions); i++)
  {
    index = instructions[i].mask;

    z = (1 << count_zeros(index));
    for (int j = 0; j < z; j++)
    {
      nindex = instructions[i].value | distribute_bits(index, j);

      if (hash_index[nindex] == -1 || 
          is_more_specific(&instructions[i], &instructions[hash_index[nindex]]))
        hash_index[nindex] = i;
    }
  }

  for (int i = 0; i < HASH_TABLE_SIZE; i++)
  {
    if (hash_index[i] == -1)
      free++;
    else
      hash[i] = instructions[hash_index[i]].handler;
  }
}

//-----------------------------------------------------------------------------
void core_init(core_t *core)
{
  for (int i = 0; i < 16; i++)
    core->r[i] = 0;
}

//-----------------------------------------------------------------------------
void core_clk(core_t *core)
{
  core->opcode = core->flash[core->r[PC] >> 1];
  core->r[PC] += 2;

  hash[core->opcode](core);
}

