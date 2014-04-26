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

#ifndef _CORE_H_
#define _CORE_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/*- Definitions -------------------------------------------------------------*/
#define SP     13
#define LR     14
#define PC     15

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  char         *name;

  uint32_t     r[16];
  bool         n;
  bool         z;
  bool         c;
  bool         v;

  uint16_t     opcode;

  uint16_t     *flash;
  void         *soc;
} core_t;

/*- Prototypes --------------------------------------------------------------*/
void core_setup(void);
void core_init(core_t *core);
void core_clk(core_t *core);

#endif // _CORE_H_

