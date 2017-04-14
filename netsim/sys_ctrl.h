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

#ifndef _SYS_CTRL_H_
#define _SYS_CTRL_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include "io_ops.h"

/*- Types -------------------------------------------------------------------*/
enum
{
  SYS_CTRL_UID         = 0x00,
  SYS_CTRL_ID          = 0x04,
  SYS_CTRL_RAND        = 0x08,
  SYS_CTRL_LOG         = 0x0c,
  SYS_CTRL_INTENSET    = 0x10,
  SYS_CTRL_INTENCLR    = 0x14,
};

typedef struct
{
  void         *soc;
} sys_ctrl_t;

/*- Prototypes --------------------------------------------------------------*/
void sys_ctrl_init(sys_ctrl_t *sys_ctrl);

/*- Variables ---------------------------------------------------------------*/
extern io_ops_t sys_ctrl_ops;

#endif // _SYS_CTRL_H_

