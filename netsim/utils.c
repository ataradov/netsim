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

/*- Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include "main.h"
#include "utils.h"

// Random algorithm used here is Complementary Multiply With Carry
// Implementation is taken from http://en.wikipedia.org/wiki/Multiply-with-carry

/*- Definitions -------------------------------------------------------------*/
#define RAND_PHI   0x9e3779b9

/*- Variables ---------------------------------------------------------------*/
static uint32_t rand_state[4096];

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void rand_init(uint32_t state)
{
  rand_state[0] = state;
  rand_state[1] = state + RAND_PHI;
  rand_state[2] = state + RAND_PHI*2;
 
  for (int i = 3; i < 4096; i++)
    rand_state[i] = rand_state[i - 3] ^ rand_state[i - 2] ^ RAND_PHI ^ i;

  // This is not a part of the original implementation, but without this
  // first 4096 generated values are not random at all
  for (int i = 0; i < 4096; i++)
    rand_next();
}

//-----------------------------------------------------------------------------
uint32_t rand_next(void)
{
  static uint32_t i = 4095;
  static uint32_t c = 362436;
  uint64_t t;
 
  i = (i + 1) & 4095;
  t = (18705ULL * rand_state[i]) + c;
  c = t >> 32;
  rand_state[i] = 0xfffffffe - t;
 
  return rand_state[i];
}

//-----------------------------------------------------------------------------
float randf_next(void)
{
  return (float)rand_next() / (float)0xffffffff;
}

//-----------------------------------------------------------------------------
uint64_t get_sim_cycle(void)
{
  return g_sim.cycle;
}

//-----------------------------------------------------------------------------
void *sim_malloc(int size)
{
  void *ptr = malloc(size);

  if (NULL == ptr)
    error("out of memory");

  memset(ptr, 0, size);

  return ptr;
}

//-----------------------------------------------------------------------------
void sim_free(void *ptr)
{
  free(ptr);
}

//-----------------------------------------------------------------------------
void error(const char *fmt, ...)
{
  va_list arg;
  char buf[500];

  va_start(arg, fmt);
  vsnprintf(buf, sizeof(buf), fmt, arg);
  va_end(arg);

  fputs("Error: ", stderr);
  fputs(buf, stderr);
  fputs("\r\n", stderr);
  exit(1);
}

//-----------------------------------------------------------------------------
void queue_init(queue_t *queue)
{
  queue->next = queue;
  queue->prev = queue;
}

//-----------------------------------------------------------------------------
void queue_add(queue_t *queue, void *item)
{
  queue_t *it = (queue_t *)item;

  it->next = queue;
  it->prev = queue->prev;
  queue->prev->next = it;
  queue->prev = it;
}

//-----------------------------------------------------------------------------
void queue_remove(queue_t *queue, void *item)
{
  queue_t *it = (queue_t *)item;

  it->prev->next = it->next;
  it->next->prev = it->prev;

  (void)queue;
}

