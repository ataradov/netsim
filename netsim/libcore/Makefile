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

.PHONY: all gen clean

CFLAGS += -W -Wall --std=gnu99 -O3
CFLAGS += -fdata-sections -ffunction-sections

LIB = libcore.a

SRCS = \
  core.c \
  i_32bit.c \
  i_undefined.c

SRCS_GEN = \
  hash_table.gen.c \
  i_adcs_reg.gen.c \
  i_add_reg4.gen.c \
  i_add_r_pc_imm.gen.c \
  i_add_r_sp_imm.gen.c \
  i_adds_imm3.gen.c \
  i_adds_imm8.gen.c \
  i_add_sp_imm.gen.c \
  i_adds_reg.gen.c \
  i_ands_reg.gen.c \
  i_asrs_imm.gen.c \
  i_asrs_reg.gen.c \
  i_b_c_imm.gen.c \
  i_bics_reg.gen.c \
  i_b_imm.gen.c \
  i_bkpt_imm.gen.c \
  i_blx_reg4.gen.c \
  i_bx_reg4.gen.c \
  i_cmn_reg.gen.c \
  i_cmp_imm.gen.c \
  i_cmp_reg.gen.c \
  i_cmp_reg4.gen.c \
  i_cps.gen.c \
  i_eors_reg.gen.c \
  i_ldm.gen.c \
  i_ldrb_imm.gen.c \
  i_ldrb_reg.gen.c \
  i_ldrh_imm.gen.c \
  i_ldrh_reg.gen.c \
  i_ldr_imm.gen.c \
  i_ldr_pc.gen.c \
  i_ldr_reg.gen.c \
  i_ldr_r_sp_imm.gen.c \
  i_ldrsb_reg.gen.c \
  i_ldrsh_reg.gen.c \
  i_lsls_imm.gen.c \
  i_lsls_reg.gen.c \
  i_lsrs_imm.gen.c \
  i_lsrs_reg.gen.c \
  i_mov_reg4.gen.c \
  i_movs_imm.gen.c \
  i_muls_reg.gen.c \
  i_mvns_reg.gen.c \
  i_nop.gen.c \
  i_orrs_reg.gen.c \
  i_pop.gen.c \
  i_push.gen.c \
  i_rev16_reg.gen.c \
  i_rev_reg.gen.c \
  i_revsh_reg.gen.c \
  i_rors_reg.gen.c \
  i_rsbs_imm.gen.c \
  i_sbcs_reg.gen.c \
  i_sev.gen.c \
  i_stm.gen.c \
  i_strb_imm.gen.c \
  i_strb_reg.gen.c \
  i_strh_imm.gen.c \
  i_strh_reg.gen.c \
  i_str_imm.gen.c \
  i_str_reg.gen.c \
  i_str_r_sp_imm.gen.c \
  i_subs_imm3.gen.c \
  i_subs_imm8.gen.c \
  i_sub_sp_imm.gen.c \
  i_subs_reg.gen.c \
  i_svc_imm.gen.c \
  i_sxtb_reg.gen.c \
  i_sxth_reg.gen.c \
  i_tst_reg.gen.c \
  i_udf_imm.gen.c \
  i_uxtb_reg.gen.c \
  i_uxth_reg.gen.c \
  i_wfe.gen.c \
  i_wfi.gen.c \
  i_yield.gen.c

SRCS += $(SRCS_GEN)

OBJS = $(notdir %/$(subst .c,.o, $(SRCS)))

all: $(LIB)

$(LIB): $(OBJS)
	@echo AR $@
	@ar cr $@ $(OBJS)

%.o:
	@echo CC $@
	@gcc $(CFLAGS) $(filter %$(subst .o,.c,$(notdir $@)), $(SRCS)) -c -o $@

gen:
	@echo GEN
	@python gen_core.py

clean:
	@echo CLEAN
	@-rm -rf $(SRCS_GEN) $(OBJS) $(LIB)

