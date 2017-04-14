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

#ifndef _UTILS_H_
#define _UTILS_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

/*- Definitions -------------------------------------------------------------*/
#define DEBUG_CORE         0
#define DEBUG_TRX          0
#define DEBUG_NOISE        0
#define DEBUG_LOG          1

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
    printf("%9"PRId64" %-6s %-8s " _fmt "\r\n", g_sim.cycle, #_name, \
        (_mod)->name, ##__VA_ARGS__); \
  }

#define CORE_DBG(_mod, _fmt, ...)   DEBUG(CORE, _mod, _fmt, ##__VA_ARGS__)
#define TRX_DBG(_mod, _fmt, ...)    DEBUG(TRX, _mod, _fmt, ##__VA_ARGS__)
#define NOISE_DBG(_mod, _fmt, ...)  DEBUG(NOISE, _mod, _fmt, ##__VA_ARGS__)
#define LOG_DBG(_mod, _fmt, ...)    DEBUG(LOG, _mod, _fmt, ##__VA_ARGS__)

#define queue_foreach(t, v, q) \
  for (t *v = (t*)(q)->next, *__next = (t*)v->queue.next; (t*)(q) != v; v = __next, __next = (t*)v->queue.next)

/*- Types -------------------------------------------------------------------*/
typedef struct queue_t
{
  struct queue_t *next;
  struct queue_t *prev;
} queue_t;

/*- Prototypes --------------------------------------------------------------*/
void rand_init(uint32_t state);
uint32_t rand_next(void);
float randf_next(void);

uint64_t get_sim_cycle(void);

void *sim_malloc(int size);
void sim_free(void *ptr);

void error(const char *fmt, ...);

void queue_init(queue_t *queue);
void queue_add(queue_t *queue, void *item);
void queue_remove(queue_t *queue, void *item);

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static inline bool queue_is_empty(queue_t *queue)
{
  return (queue->next == queue);
}

#endif // _UTILS_H_

