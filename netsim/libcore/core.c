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

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void core_setup(void)
{
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

  core_hash_table[core->opcode](core);
}

