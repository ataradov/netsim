#!/usr/bin/env python
#
# Copyright (c) 2014, Alex Taradov <taradov@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

#------------------------------------------------------------------------------
def GET_R1(x):
  return x & 0x07

def GET_R2(x):
  return (x >> 3) & 0x07

def GET_R3(x):
  return (x >> 6) & 0x07

def GET_IMM3(x):
  return (x >> 6) & 0x07

def GET_IMM5(x):
  return (x >> 6) & 0x1f

def GET_IMM7(x):
  return x & 0x7f

def GET_IMM8(x):
  return x & 0xff

def GET_IMM11(x):
  return x & 0x7ff

def GET_EXTRA_REG(x):
  return (x >> 8) & 1

def GET_R1_4(x):
  return ((x >> 4) & 0x08) | (x & 0x07)

def GET_R2_4(x):
  return (x >> 3) & 0x0f

def GET_R_IMM8(x):
  return (x >> 8) & 0x07

def GET_COND(x):
  return (x >> 8) & 0x0f

#------------------------------------------------------------------------------
def replace(text, opcode):
  fields = ['R1', 'R2', 'R3', 'IMM3', 'IMM5', 'IMM7', 'IMM8', 'IMM11',
      'EXTRA_REG', 'R1_4', 'R2_4', 'R_IMM8', 'COND']

  for field in fields:
    text = text.replace('GET_%s(core->opcode)' % field, str(eval('GET_' + field)(opcode)))

  return text

#------------------------------------------------------------------------------
i_lsls_imm = """
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
"""

#------------------------------------------------------------------------------
i_lsrs_imm = """
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
"""

#------------------------------------------------------------------------------
i_asrs_imm = """
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
"""

#------------------------------------------------------------------------------
i_adds_reg = """
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
"""

#------------------------------------------------------------------------------
i_subs_reg = """
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
"""

#------------------------------------------------------------------------------
i_adds_imm3 = """
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
"""

#------------------------------------------------------------------------------
i_subs_imm3 = """
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
"""

#------------------------------------------------------------------------------
i_movs_imm = """
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode);
  uint32_t res = imm;

  CORE_DBG(core, "movs\tr%d, 0x%02x", rd, imm);

  core->n = (res >> 31);
  core->z = res == 0;

  core->r[rd] = res;
}
"""

#------------------------------------------------------------------------------
i_cmp_imm = """
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
"""

#------------------------------------------------------------------------------
i_adds_imm8 = """
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
"""

#------------------------------------------------------------------------------
i_subs_imm8 = """
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
"""

#------------------------------------------------------------------------------
i_ands_reg = """
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
"""

#------------------------------------------------------------------------------
i_eors_reg = """
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
"""

#------------------------------------------------------------------------------
i_lsls_reg = """
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
"""

#------------------------------------------------------------------------------
i_lsrs_reg = """
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
"""

#------------------------------------------------------------------------------
i_asrs_reg = """
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
"""

#------------------------------------------------------------------------------
i_adcs_reg = """
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
"""

#------------------------------------------------------------------------------
i_sbcs_reg = """
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
"""

#------------------------------------------------------------------------------
i_rors_reg = """
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
"""

#------------------------------------------------------------------------------
i_tst_reg = """
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
"""

#------------------------------------------------------------------------------
i_rsbs_imm = """
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
"""

#------------------------------------------------------------------------------
i_cmp_reg = """
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
"""

#------------------------------------------------------------------------------
i_cmn_reg = """
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
"""

#------------------------------------------------------------------------------
i_orrs_reg = """
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
"""

#------------------------------------------------------------------------------
i_muls_reg = """
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
"""

#------------------------------------------------------------------------------
i_bics_reg = """
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
"""

#------------------------------------------------------------------------------
i_mvns_reg = """
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
"""

#------------------------------------------------------------------------------
i_add_reg4 = """
{
  int r1 = GET_R1_4(core->opcode);
  int r2 = GET_R2_4(core->opcode);

  CORE_DBG(core, "add\tr%d, r%d", r1, r2);

  core->r[r1] = core->r[r1] + core->r[r2];

  if (PC == r1)
    core->r[PC] &= 0xfffffffe;
}
"""

#------------------------------------------------------------------------------
i_cmp_reg4 = """
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
"""

#------------------------------------------------------------------------------
i_mov_reg4 = """
{
  int r1 = GET_R1_4(core->opcode);
  int r2 = GET_R2_4(core->opcode);

  CORE_DBG(core, "mov\tr%d, r%d", r1, r2);

  core->r[r1] = core->r[r2];

  if (PC == r1)
    core->r[PC] &= 0xfffffffe;
}
"""

#------------------------------------------------------------------------------
i_bx_reg4 = """
{
  int r = GET_R2_4(core->opcode);

  CORE_DBG(core, "bx\tr%d", r);

  core->r[PC] = core->r[r] & 0xfffffffe;
}
"""

#------------------------------------------------------------------------------
i_blx_reg4 = """
{
  int r = GET_R2_4(core->opcode);
  uint32_t addr;

  CORE_DBG(core, "blx\tr%d", r);

  addr = core->r[r] & 0xfffffffe;
  core->r[LR] = core->r[PC];
  core->r[PC] = addr;
}
"""

#------------------------------------------------------------------------------
i_ldr_pc = """
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "ldr\tr%d, [PC, 0x%02x]", rd, imm);

  core->r[rd] = soc_read_w(core->soc, core->r[PC] + imm + 2);
}
"""

#------------------------------------------------------------------------------
i_str_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "str\tr%d, [r%d, r%d]", r1, r2, r3);

  soc_write_w(core->soc, core->r[r2] + core->r[r3], core->r[r1]);
}
"""

#------------------------------------------------------------------------------
i_strh_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "strh\tr%d, [r%d, r%d]", r1, r2, r3);

  soc_write_h(core->soc, core->r[r2] + core->r[r3], core->r[r1]);
}
"""

#------------------------------------------------------------------------------
i_strb_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "strb\tr%d, [r%d, r%d]", r1, r2, r3);

  soc_write_b(core->soc, core->r[r2] + core->r[r3], core->r[r1]);
}
"""

#------------------------------------------------------------------------------
i_ldrsb_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);
  uint32_t val;

  CORE_DBG(core, "ldrsb\tr%d, [r%d, r%d]", r1, r2, r3);

  val = soc_read_b(core->soc, core->r[r2] + core->r[r3]);
  core->r[r1] = (val & 0xff) | ((val & 0x80) ? 0xffffff00 : 0);
}
"""

#------------------------------------------------------------------------------
i_ldr_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "ldr\tr%d, [r%d, r%d]", r1, r2, r3);

  core->r[r1] = soc_read_w(core->soc, core->r[r2] + core->r[r3]);
}
"""

#------------------------------------------------------------------------------
i_ldrh_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "ldrh\tr%d, [r%d, r%d]", r1, r2, r3);

  core->r[r1] = soc_read_h(core->soc, core->r[r2] + core->r[r3]);
}
"""

#------------------------------------------------------------------------------
i_ldrb_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);

  CORE_DBG(core, "ldrb\tr%d, [r%d, r%d]", r1, r2, r3);

  core->r[r1] = soc_read_b(core->soc, core->r[r2] + core->r[r3]);
}
"""

#------------------------------------------------------------------------------
i_ldrsh_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  int r3 = GET_R3(core->opcode);
  uint32_t val;

  CORE_DBG(core, "ldrsh\tr%d, [r%d, r%d]", r1, r2, r3);

  val = soc_read_h(core->soc, core->r[r2] + core->r[r3]);
  core->r[r1] = val | ((val & 0x8000) ? 0xffff0000 : 0);
}
"""

#------------------------------------------------------------------------------
i_str_imm = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode) * 4;

  CORE_DBG(core, "str\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  soc_write_w(core->soc, core->r[r2] + imm, core->r[r1]);
}
"""

#------------------------------------------------------------------------------
i_ldr_imm = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode) * 4;

  CORE_DBG(core, "ldr\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  core->r[r1] = soc_read_w(core->soc, core->r[r2] + imm);
}
"""

#------------------------------------------------------------------------------
i_strb_imm = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode);

  CORE_DBG(core, "strb\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  soc_write_b(core->soc, core->r[r2] + imm, core->r[r1]);
}
"""

#------------------------------------------------------------------------------
i_ldrb_imm = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode);

  CORE_DBG(core, "ldrb\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  core->r[r1] = soc_read_b(core->soc, core->r[r2] + imm);
}
"""

#------------------------------------------------------------------------------
i_strh_imm = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode) * 2;

  CORE_DBG(core, "strh\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  soc_write_h(core->soc, core->r[r2] + imm, core->r[r1]);
}
"""

#------------------------------------------------------------------------------
i_ldrh_imm = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t imm = GET_IMM5(core->opcode) * 2;

  CORE_DBG(core, "ldrh\tr%d, [r%d, 0x%02x]", r1, r2, imm);

  core->r[r1] = soc_read_h(core->soc, core->r[r2] + imm);
}
"""

#------------------------------------------------------------------------------
i_str_r_sp_imm = """
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "str\tr%d, [SP, 0x%02x]", rd, imm);

  soc_write_w(core->soc, core->r[SP] + imm, core->r[rd]);
}
"""

#------------------------------------------------------------------------------
i_ldr_r_sp_imm = """
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "ldr\tr%d, [SP, 0x%02x]", rd, imm);

  core->r[rd] = soc_read_w(core->soc, core->r[SP] + imm);
}
"""

#------------------------------------------------------------------------------
i_add_r_pc_imm = """
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "add\tr%d, PC, 0x%02x", rd, imm);

  core->r[rd] = (core->r[PC] & 0xfffffffc) + imm;
}
"""

#------------------------------------------------------------------------------
i_add_r_sp_imm = """
{
  int rd = GET_R_IMM8(core->opcode);
  uint32_t imm = GET_IMM8(core->opcode) * 4;

  CORE_DBG(core, "add\tr%d, SP, 0x%02x", rd, imm);

  core->r[rd] = core->r[SP] + imm;
}
"""

#------------------------------------------------------------------------------
i_add_sp_imm = """
{
  uint32_t imm = GET_IMM7(core->opcode) * 4;

  CORE_DBG(core, "add\tSP, SP, 0x%02x", imm);

  core->r[SP] += imm;
}
"""

#------------------------------------------------------------------------------
i_sub_sp_imm = """
{
  uint32_t imm = GET_IMM7(core->opcode) * 4;

  CORE_DBG(core, "sub\tSP, SP, 0x%02x", imm);

  core->r[SP] -= imm;
}
"""

#------------------------------------------------------------------------------
i_sxth_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "sxth\tr%d, r%d", r1, r2);

  core->r[r1] = (r2v & 0xffff) | ((r2v & 0x8000) ? 0xffff0000 : 0);
}
"""

#------------------------------------------------------------------------------
i_sxtb_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "sxtb\tr%d, r%d", r1, r2);

  core->r[r1] = (r2v & 0xff) | ((r2v & 0x80) ? 0xffffff00 : 0);
}
"""

#------------------------------------------------------------------------------
i_uxth_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);

  CORE_DBG(core, "uxth\tr%d, r%d", r1, r2);

  core->r[r1] = core->r[r2] & 0xffff;
}
"""

#------------------------------------------------------------------------------
i_uxtb_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);

  CORE_DBG(core, "uxtb\tr%d, r%d", r1, r2);

  core->r[r1] = core->r[r2] & 0xff;
}
"""

#------------------------------------------------------------------------------
i_push = """
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
"""

#------------------------------------------------------------------------------
i_pop = """
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
"""

#------------------------------------------------------------------------------
i_cps = """
{
  error("%s: cps not implemented at 0x%08x", core->name, core->r[PC]-2);
}
"""

#------------------------------------------------------------------------------
i_rev_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "rev\tr%d, r%d", r1, r2);

  core->r[r1] = ((r2v & 0x000000ff) << 24) | ((r2v & 0x0000ff00) << 8) |
                ((r2v & 0x00ff0000) >> 8) | ((r2v & 0xff000000) >> 24);
}
"""

#------------------------------------------------------------------------------
i_rev16_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "rev16\tr%d, r%d", r1, r2);

  core->r[r1] = ((r2v & 0x000000ff) << 8) | ((r2v & 0x0000ff00) >> 8) |
                ((r2v & 0x00ff0000) << 8) | ((r2v & 0xff000000) >> 8);
}
"""

#------------------------------------------------------------------------------
i_revsh_reg = """
{
  int r1 = GET_R1(core->opcode);
  int r2 = GET_R2(core->opcode);
  uint32_t r2v = core->r[r2];

  CORE_DBG(core, "revsh\tr%d, r%d", r1, r2);

  r2v = ((r2v & 0x000000ff) << 8) | ((r2v & 0x0000ff00) >> 8);
  core->r[r1] = (r2v & 0xffff) | ((r2v & 0x8000) ? 0xffff0000 : 0);
}
"""

#------------------------------------------------------------------------------
i_bkpt_imm = """
{
  uint32_t imm = GET_IMM8(core->opcode);

  CORE_DBG(core, "bkpt\t0x%02x", imm);
}
"""

#------------------------------------------------------------------------------
i_nop = """
{
  CORE_DBG(core, "nop");
}
"""

#------------------------------------------------------------------------------
i_yield = """
{
  CORE_DBG(core, "yield");
}
"""

#------------------------------------------------------------------------------
i_wfe = """
{
  CORE_DBG(core, "wfe");
}
"""

#------------------------------------------------------------------------------
i_wfi = """
{
  CORE_DBG(core, "wfi");
}
"""

#------------------------------------------------------------------------------
i_sev = """
{
  CORE_DBG(core, "sev");
}
"""

#------------------------------------------------------------------------------
i_stm = """
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
"""

#------------------------------------------------------------------------------
i_ldm = """
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
"""

#------------------------------------------------------------------------------
i_b_c_imm = """
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
"""

#------------------------------------------------------------------------------
i_udf_imm = """
{
  uint32_t imm = GET_IMM8(core->opcode);

  CORE_DBG(core, "udf\t0x%02x", imm);
  // TODO: interrupt

  error("%s: udf not implemented at 0x%08x", core->name, core->r[PC]-2);
}
"""

#------------------------------------------------------------------------------
i_svc_imm = """
{
  uint32_t imm = GET_IMM8(core->opcode);

  CORE_DBG(core, "svc\t0x%02x", imm);
  // TODO: interrupt

  error("%s: svc not implemented at 0x%08x", core->name, core->r[PC]-2);
}
"""

#------------------------------------------------------------------------------
i_b_imm = """
{
  uint32_t imm = GET_IMM11(core->opcode) * 2;

  imm |= (imm & 0x800) ? 0xfffff000 : 0x00000000;

  CORE_DBG(core, "b\t\t0x%x", core->r[PC] + imm + 2);

  core->r[PC] += imm + 2;
}
"""

#------------------------------------------------------------------------------
instructions = \
{
  'i_lsls_imm'		: (0xf800, 0x0000),
  'i_lsrs_imm'		: (0xf800, 0x0800),
  'i_asrs_imm'		: (0xf800, 0x1000),
  'i_adds_reg'		: (0xfe00, 0x1800),
  'i_subs_reg'		: (0xfe00, 0x1a00),
  'i_adds_imm3'		: (0xfe00, 0x1c00),
  'i_subs_imm3'		: (0xfe00, 0x1e00),
  'i_movs_imm'		: (0xf800, 0x2000),
  'i_cmp_imm'		: (0xf800, 0x2800),
  'i_adds_imm8'		: (0xf800, 0x3000),
  'i_subs_imm8'		: (0xf800, 0x3800),

  'i_ands_reg'		: (0xffc0, 0x4000),
  'i_eors_reg'		: (0xffc0, 0x4040),
  'i_lsls_reg'		: (0xffc0, 0x4080),
  'i_lsrs_reg'		: (0xffc0, 0x40c0),
  'i_asrs_reg'		: (0xffc0, 0x4100),
  'i_adcs_reg'		: (0xffc0, 0x4140),
  'i_sbcs_reg'		: (0xffc0, 0x4180),
  'i_rors_reg'		: (0xffc0, 0x41c0),
  'i_tst_reg'		: (0xffc0, 0x4200),
  'i_rsbs_imm'		: (0xffc0, 0x4240),
  'i_cmp_reg'		: (0xffc0, 0x4280),
  'i_cmn_reg'		: (0xffc0, 0x42c0),
  'i_orrs_reg'		: (0xffc0, 0x4300),
  'i_muls_reg'		: (0xffc0, 0x4340),
  'i_bics_reg'		: (0xffc0, 0x4380),
  'i_mvns_reg'		: (0xffc0, 0x43c0),

  'i_add_reg4'		: (0xff00, 0x4400),
  'i_cmp_reg4'		: (0xff00, 0x4500),
  'i_mov_reg4'		: (0xff00, 0x4600),
  'i_bx_reg4'		: (0xff87, 0x4700),
  'i_blx_reg4'		: (0xff87, 0x4780),

  'i_ldr_pc'		: (0xf800, 0x4800),

  'i_str_reg'		: (0xfe00, 0x5000),
  'i_strh_reg'		: (0xfe00, 0x5200),
  'i_strb_reg'		: (0xfe00, 0x5400),
  'i_ldrsb_reg'		: (0xfe00, 0x5600),
  'i_ldr_reg'		: (0xfe00, 0x5800),
  'i_ldrh_reg'		: (0xfe00, 0x5a00),
  'i_ldrb_reg'		: (0xfe00, 0x5c00),
  'i_ldrsh_reg'		: (0xfe00, 0x5e00),
  'i_str_imm'		: (0xf800, 0x6000),
  'i_ldr_imm'		: (0xf800, 0x6800),
  'i_strb_imm'		: (0xf800, 0x7000),
  'i_ldrb_imm'		: (0xf800, 0x7800),
  'i_strh_imm'		: (0xf800, 0x8000),
  'i_ldrh_imm'		: (0xf800, 0x8800),
  'i_str_r_sp_imm'	: (0xf800, 0x9000),
  'i_ldr_r_sp_imm'	: (0xf800, 0x9800),

  'i_add_r_pc_imm'	: (0xf800, 0xa000),
  'i_add_r_sp_imm'	: (0xf800, 0xa800),

  'i_add_sp_imm'	: (0xff80, 0xb000),
  'i_sub_sp_imm'	: (0xff80, 0xb080),
  'i_sxth_reg'		: (0xffc0, 0xb200),
  'i_sxtb_reg'		: (0xffc0, 0xb240),
  'i_uxth_reg'		: (0xffc0, 0xb280),
  'i_uxtb_reg'		: (0xffc0, 0xb2c0),
  'i_push'		: (0xfe00, 0xb400),
  'i_pop'		: (0xfe00, 0xbc00),
  'i_cps'		: (0xffef, 0xb662),
  'i_rev_reg'		: (0xffc0, 0xba00),
  'i_rev16_reg'		: (0xffc0, 0xba40),
  'i_revsh_reg'		: (0xffc0, 0xbac0),
  'i_bkpt_imm'		: (0xff00, 0xbe00),
  'i_nop'		: (0xffff, 0xbf00),
  'i_yield'		: (0xffff, 0xbf10),
  'i_wfe'		: (0xffff, 0xbf20),
  'i_wfi'		: (0xffff, 0xbf30),
  'i_sev'		: (0xffff, 0xbf40),

  'i_stm'		: (0xf800, 0xc000),
  'i_ldm'		: (0xf800, 0xc800),

  'i_b_c_imm'		: (0xf000, 0xd000),
  'i_udf_imm'		: (0xff00, 0xde00),
  'i_svc_imm'		: (0xff00, 0xdf00),

  'i_b_imm'		: (0xf800, 0xe000),

  'i_32bit'		: (0xf800, 0xf000),
}

#------------------------------------------------------------------------------
def is_more_specific(i1, i2):
  return (i1[1] & i2[0]) == i2[1]

#------------------------------------------------------------------------------
def generate_table():
  table = [None] * 0x10000

  for i in range(0x10000):
    for instr, mask_value in instructions.iteritems():
      if (i & mask_value[0]) == mask_value[1]:
        if table[i] is None or is_more_specific(mask_value, instructions[table[i]]):
          table[i] = instr

  for i in range(0x10000):
    if table[i] is None:
      table[i] = 'i_undefined'

  return table

#------------------------------------------------------------------------------
def generate_modules(table):
  modules = {}
  functions = [None] * 0x10000

  for opcode in range(0x10000):
    name = table[opcode]

    if name in ['i_undefined', 'i_32bit']:
      functions[opcode] = name
      continue

    body = replace(eval(name), opcode)
    text = 'void i%04x(core_t *core)%s' % (opcode, body)

    functions[opcode] = 'i%04x' % opcode

    if name in modules:
      modules[name] += text
    else:
      modules[name] = text

  return modules, functions

#------------------------------------------------------------------------------
def generate_hash(functions):
  fns = {}
  for name in functions:
    fns[name] = None

  text = ''
  for fn in fns:
    text += 'void %s(core_t *core);\n' % fn

  text += '\n'
  text += 'handler_t *core_hash_table[0x10000] = {\n'

  line = '  '
  for i in range(0x10000):
    next = '%s, ' % functions[i];

    if len(line + next) > 100:
      text += line + '\n'
      line = '  ' + next
    else:
      line += next

  text += '};'

  return text

#------------------------------------------------------------------------------
def main():
  table = generate_table()
  modules, functions = generate_modules(table)
  hash_table = generate_hash(functions)

  for name, text in modules.iteritems():
    f = file('%s.gen.c' % name, 'w')
    f.write('#include "i_common.h"\n\n')
    f.write(text)
    f.write('\n')
    f.close()

  f = file('hash_table.gen.c', 'w')
  f.write('#include "i_common.h"\n\n')
  f.write(hash_table)
  f.write('\n')
  f.close()

#------------------------------------------------------------------------------
main()


