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

#ifndef _SYS_CTRL_H_
#define _SYS_CTRL_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>

/*- Types -------------------------------------------------------------------*/
enum
{
  SYS_CTRL_UID         = 0x00,
  SYS_CTRL_ID          = 0x04,
  SYS_CTRL_RAND        = 0x08,
  SYS_CTRL_REG_MASK    = 0xff,
};

typedef struct
{
  void         *soc;
} sys_ctrl_t;

/*- Prototypes --------------------------------------------------------------*/
void sys_ctrl_init(sys_ctrl_t *sys_ctrl);
uint32_t sys_ctrl_read_w(sys_ctrl_t *sys_ctrl, uint32_t addr);

#endif // _SYS_CTRL_H_

