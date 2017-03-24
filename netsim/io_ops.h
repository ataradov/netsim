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

#ifndef _IO__H_
#define _IO__H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>

/*- Types -------------------------------------------------------------------*/
typedef uint8_t    (*io_read_b_t)(void *, uint32_t);
typedef uint16_t   (*io_read_h_t)(void *, uint32_t);
typedef uint32_t   (*io_read_w_t)(void *, uint32_t);
typedef void       (*io_write_b_t)(void *, uint32_t, uint8_t);
typedef void       (*io_write_h_t)(void *, uint32_t, uint16_t);
typedef void       (*io_write_w_t)(void *, uint32_t, uint32_t);

typedef struct
{
  io_read_b_t     read_b;
  io_read_h_t     read_h;
  io_read_w_t     read_w;
  io_write_b_t    write_b;
  io_write_h_t    write_h;
  io_write_w_t    write_w;
} io_ops_t;

#endif // _IO__H_

