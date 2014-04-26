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

#ifndef _UTILS_H_
#define _UTILS_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "main.h"

/*- Definitions -------------------------------------------------------------*/
#define MHz    1000000.0f

#define max(a, b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a, b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define DEBUG(_name, _mod, _fmt, ...) \
  if (DEBUG_##_name) { \
    printf("%9lld %-6s %-8s " _fmt "\r\n", g_sim.cycle, #_name, \
        (_mod)->name, ##__VA_ARGS__); \
  }

#define CORE_DBG(_mod, _fmt, ...)   DEBUG(CORE, _mod, _fmt, ##__VA_ARGS__)
#define TRX_DBG(_mod, _fmt, ...)    DEBUG(TRX, _mod, _fmt, ##__VA_ARGS__)
#define NOISE_DBG(_mod, _fmt, ...)  DEBUG(NOISE, _mod, _fmt, ##__VA_ARGS__)

/*- Types -------------------------------------------------------------------*/
typedef struct queue_t
{
  struct queue_t *next;
} queue_t;

/*- Prototypes --------------------------------------------------------------*/
void rand_init(uint32_t state);
uint32_t rand_next(void);
float randf_next(void);

void *sim_malloc(int size);
void sim_free(void *ptr);

void queue_add(queue_t **queue, queue_t *item);
void queue_remove(queue_t **queue, queue_t *item);

void error(const char *fmt, ...);

#endif // _UTILS_H_

