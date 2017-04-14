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
#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "main.h"
#include "utils.h"
#include "config.h"

/*- Definitions -------------------------------------------------------------*/
#define CONFIG_BUF_SIZE        8192
#define CONFIG_LINE_SIZE       1024
#define CONFIG_EOF             -1

/*- Variables ---------------------------------------------------------------*/
static const char *config_name;
static int config_line;
static int config_col;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static void skip_spaces(char **line)
{
  char *buf = *line;

  while (true)
  {
    if (' ' == buf[0])
    {
      buf++;
      config_col++;
    }
    else if ('\t' == buf[0])
    {
      buf++;
      config_col += 9 - (config_col % 8);
    }
    else
      break;
  }

  *line = buf;
}

//-----------------------------------------------------------------------------
static void skip_bytes(char **line, int bytes)
{
  *line += bytes;
  config_col += bytes;
}

//-----------------------------------------------------------------------------
static bool check_str(char **line, const char *str)
{
  int len = strlen(str);

  if (0 == strncmp(*line, str, len))
  {
    skip_bytes(line, len);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
static long get_long(char **line)
{
  long res;
  char *end;
  int len;

  skip_spaces(line);

  res = strtoul(*line, &end, 0);
  len = end - *line;
  skip_bytes(line, end - *line);

  if (0 == len)
    error("%s:%d:%d: integer expected", config_name, config_line, config_col);

  return res;
}

//-----------------------------------------------------------------------------
static long long get_long_long(char **line)
{
  long long res;
  char *end;
  int len;

  skip_spaces(line);

  res = strtoull(*line, &end, 0);
  len = end - *line;
  skip_bytes(line, end - *line);

  if (0 == len)
    error("%s:%d:%d: integer expected", config_name, config_line, config_col);

  return res;
}

//-----------------------------------------------------------------------------
static float get_float(char **line)
{
  float res;
  char *end;
  int len;

  skip_spaces(line);

  res = strtof(*line, &end);
  len = end - *line;
  skip_bytes(line, end - *line);

  if (0 == len)
    error("%s:%d:%d: floating point expected", config_name, config_line, config_col);

  return res;
}

//-----------------------------------------------------------------------------
static char *get_str(char **line)
{
  char *res, *start, *end;
  int len;

  skip_spaces(line);

  start = end = *line;

  while (0 != end[0] && ' ' != end[0] && '\t' != end[0])
    end++;

  len = end-start;

  if (0 == len)
    error("%s:%d:%d: string expected", config_name, config_line, config_col);

  skip_bytes(line, len);

  res = (char *)sim_malloc(len+1);
  memcpy(res, start, len);
  res[len] = 0;

  return res;
}

//-----------------------------------------------------------------------------
static void get_range(char **line, long *a, long *b)
{
  *a = get_long(line);

  if ('-' == *line[0])
  {
    skip_bytes(line, 1);
    *b = get_long(line);
  }
  else
  {
    *b = *a;
  }
}

//-----------------------------------------------------------------------------
static char *get_name(char **line)
{
  char *name = get_str(line);

  if (!isalpha(name[0]) && '_' != name[0])
    error("%s:%d: name must start with alphabetic character or '_', got '%s'", config_name, config_line, name);

  return name;
}

//-----------------------------------------------------------------------------
static trx_t *find_node(char *name)
{
  queue_foreach(trx_t, trx, &g_sim.trxs)
  {
    if (0 == strcmp(trx->name, name))
      return trx;
  }

  return NULL;
}

//-----------------------------------------------------------------------------
static noise_t *find_noise(char *name)
{
  queue_foreach(noise_t, noise, &g_sim.noises)
  {
    if (0 == strcmp(noise->name, name))
      return noise;
  }

  return NULL;
}

//-----------------------------------------------------------------------------
static sniffer_t *find_sniffer(char *name)
{
  queue_foreach(sniffer_t, sniffer, &g_sim.sniffers)
  {
    if (0 == strcmp(sniffer->name, name))
      return sniffer;
  }

  return NULL;
}

//-----------------------------------------------------------------------------
static void load_file(char *name, uint8_t *data, int size)
{
  int f, n;

  f = open(name, O_RDONLY);

  if (f < 0)
    error("cannot open firmware file %s", name);

  n = read(f, data, size);
  close(f);

  if (size == n)
    error("firmware file %s is too big", name);
}

//-----------------------------------------------------------------------------
static void process_line(char *line)
{
  skip_spaces(&line);

  if (0 == line[0] || '#' == line[0])
    return;

  if (check_str(&line, "seed"))
  {
    g_sim.seed = get_long(&line);
  }

  else if (check_str(&line, "time"))
  {
    g_sim.time = get_long_long(&line);
  }

  else if (check_str(&line, "scale"))
  {
    g_sim.scale = get_float(&line);
  }

  else if (check_str(&line, "node"))
  {
    soc_t *soc = (soc_t *)sim_malloc(sizeof(soc_t));

    soc->name = get_name(&line);
    soc->uid = g_sim.node_uid++;
    soc->x = get_float(&line) * g_sim.scale;
    soc->y = get_float(&line) * g_sim.scale;
    soc->id = get_long(&line);
    soc->path = get_str(&line);

    if (find_node(soc->name))
      error("%s:%d: node '%s' already exists", config_name, config_line, soc->name);

    load_file(soc->path, soc->core.ram, sizeof(soc->core.ram));

    soc_init(soc);
    queue_add(&g_sim.active, (queue_t *)soc);
  }

  else if (check_str(&line, "sniffer"))
  {
    sniffer_t *sniffer = (sniffer_t *)sim_malloc(sizeof(sniffer_t));
    long freq_a, freq_b;

    sniffer->name = get_name(&line);
    sniffer->uid = g_sim.sniffer_uid++;
    sniffer->x = get_float(&line) * g_sim.scale;
    sniffer->y = get_float(&line) * g_sim.scale;
    get_range(&line, &freq_a, &freq_b);
    sniffer->freq_a = freq_a * MHz;
    sniffer->freq_b = freq_b * MHz;
    sniffer->sensitivity = get_float(&line);
    sniffer->path = get_str(&line);

    if (find_sniffer(sniffer->name))
      error("%s:%d: sniffer '%s' already exists", config_name, config_line, sniffer->name);

    sniffer_init(sniffer);
    queue_add(&g_sim.sniffers, (queue_t *)sniffer);
  }

  else if (check_str(&line, "noise"))
  {
    noise_t *noise = (noise_t *)sim_malloc(sizeof(noise_t));
    long freq_a, freq_b;

    noise->name = get_name(&line);
    noise->uid = g_sim.noise_uid++;
    noise->x = get_float(&line) * g_sim.scale;
    noise->y = get_float(&line) * g_sim.scale;
    get_range(&line, &freq_a, &freq_b);
    noise->freq_a = freq_a * MHz;
    noise->freq_b = freq_b * MHz;
    noise->power = get_float(&line);
    noise->on = get_long(&line);
    noise->off = get_long(&line);

    if (find_noise(noise->name))
      error("%s:%d: noise '%s' already exists", config_name, config_line, noise->name);

    noise_init(noise);
    queue_add(&g_sim.noises, (queue_t *)noise);
  }

  else if (check_str(&line, "loss"))
  {
    char *node_name = get_name(&line);
    char *other_name = get_name(&line);
    float loss = get_float(&line);

    trx_t *node = find_node(node_name);
    sniffer_t *sniffer = find_sniffer(node_name);
    trx_t *other_node = find_node(other_name);
    noise_t *other_noise = find_noise(other_name);

    if (node)
    {
      if (other_node)
      {
        if (NULL == node->loss_trx)
          node->loss_trx = (float *)sim_malloc(sizeof(float) * g_sim.node_uid);

        if (NULL == other_node->loss_trx)
          other_node->loss_trx = (float *)sim_malloc(sizeof(float) * g_sim.node_uid);

        node->loss_trx[other_node->uid] = loss;
        other_node->loss_trx[node->uid] = loss;
      }
      else if (other_noise)
      {
        if (NULL == node->loss_noise)
          node->loss_noise = (float *)sim_malloc(sizeof(float) * g_sim.noise_uid);

        node->loss_noise[other_noise->uid] = loss;
      }
      else
        error("%s:%d: '%s' does not name a node or a noise", config_name, config_line, other_name);
    }
    else if (sniffer)
    {
      if (NULL == sniffer->loss_trx)
        sniffer->loss_trx = (float *)sim_malloc(sizeof(float) * g_sim.node_uid);

      sniffer->loss_trx[other_node->uid] = loss;
    }
    else
      error("%s:%d: '%s' does not name a node or a sniffer", config_name, config_line, node_name);
  }

  else
    error("%s:%d:%d: invalid command", config_name, config_line, config_col);

  skip_spaces(&line);

  if (0 != line[0])
    error("%s:%d:%d: extra junk at the end of the line: '%s'", config_name, config_line, config_col, line);
}

//-----------------------------------------------------------------------------
static int config_getc(int f)
{
  static char buf[CONFIG_BUF_SIZE];
  static int size = 0;
  static int ptr = 0;

  if (ptr == size)
  {
    ptr = 0;
    size = read(f, buf, sizeof(buf));
  }

  if (0 == size)
    return CONFIG_EOF;

  return buf[ptr++];
}

//-----------------------------------------------------------------------------
void config_read(const char *name)
{
  char line[CONFIG_LINE_SIZE];
  int f, c, ptr;

  f = open(name, O_RDONLY);

  if (f < 0)
    error("cannot open configuration file %s", name);

  ptr = 0;
  config_line = 1;
  config_name = name;

  while (1)
  {
    c = config_getc(f);

    if ('\n' == c || CONFIG_EOF == c)
    {
      if (ptr && '\r' == line[ptr-1])
        line[ptr-1] = 0;
      line[ptr] = 0;

      config_col = 1;

      process_line(line);

      if (CONFIG_EOF == c)
        break;

      ptr = 0;
      config_line++;
      continue;
    }
    else
    {
      if (ptr < (CONFIG_LINE_SIZE-1))
        line[ptr++] = c;
      else
        error("%s:%d: line too long", config_name, config_line);
    }
  }

  close(f);
}

