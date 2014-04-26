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
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"
#include "config.h"

/*- Definitions -------------------------------------------------------------*/
#define CONFIG_BUF_SIZE        8192
#define CONFIG_LINE_SIZE       1024
#define CONFIG_EOF             -1

/*- Variables ---------------------------------------------------------------*/
static char *config_name;
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
static bool check_str(char **line, char *str)
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

  res = sim_malloc(len+1);
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

    soc->name = get_str(&line);
    soc->x = get_float(&line) * g_sim.scale;
    soc->y = get_float(&line) * g_sim.scale;
    soc->id = get_long(&line);
    soc->path = get_str(&line);

    load_file(soc->path, soc->mem, sizeof(soc->mem));

    soc_init(soc);
    queue_add((queue_t **)&g_sim.socs, (queue_t *)soc);
  }

  else if (check_str(&line, "sniffer"))
  {
    sniffer_t *sniffer = (sniffer_t *)sim_malloc(sizeof(sniffer_t));
    long freq_a, freq_b;

    sniffer->name = get_str(&line);
    sniffer->x = get_float(&line) * g_sim.scale;
    sniffer->y = get_float(&line) * g_sim.scale;
    get_range(&line, &freq_a, &freq_b);
    sniffer->freq_a = freq_a * MHz;
    sniffer->freq_b = freq_b * MHz;
    sniffer->sensitivity = get_float(&line);
    sniffer->path = get_str(&line);

    sniffer_init(sniffer);
    queue_add((queue_t **)&g_sim.sniffers, (queue_t *)sniffer);
  }

  else if (check_str(&line, "noise"))
  {
    noise_t *noise = (noise_t *)sim_malloc(sizeof(noise_t));
    long freq_a, freq_b;

    noise->name = get_str(&line);
    noise->x = get_float(&line) * g_sim.scale;
    noise->y = get_float(&line) * g_sim.scale;
    get_range(&line, &freq_a, &freq_b);
    noise->freq_a = freq_a * MHz;
    noise->freq_b = freq_b * MHz;
    noise->power = get_float(&line);
    noise->on = get_long(&line);
    noise->off = get_long(&line);

    noise_init(noise);
    queue_add((queue_t **)&g_sim.noises, (queue_t *)noise);
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
void config_read(char *name)
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

