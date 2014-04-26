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
#include <math.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "main.h"
#include "utils.h"
#include "sniffer.h"

/*- Prototypes --------------------------------------------------------------*/
static void sniffer_write(sniffer_t *sniffer, char *str, int size);

/*- Variables ---------------------------------------------------------------*/
static char hex[] = "0123456789abcdef";

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void sniffer_init(sniffer_t *sniffer)
{
  sniffer->fd = open(sniffer->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (sniffer->fd < 0)
    error("cannot create sniffer output file %s", sniffer->path);

  sniffer_write(sniffer, "#Format=4\r\n", -1);
  sniffer_write(sniffer, "# SNA v5.5.5.5 SUS:20140418 ACT:000000\r\n", -1);

  sniffer->seq = 1;
}

//-----------------------------------------------------------------------------
void sniffer_write_frame(sniffer_t *sniffer, uint8_t *data, float power)
{
  char data_str[256];
  char str[512];
  int len, size = data[0];

  for (int i = 0; i < size-2; i++)
  {
    data_str[i*2 + 0] = hex[data[i + 1] >> 4];
    data_str[i*2 + 1] = hex[data[i + 1] & 0x0f];
  }

  data_str[size*2 - 4] = 'f';
  data_str[size*2 - 3] = 'f';
  data_str[size*2 - 2] = 'f';
  data_str[size*2 - 1] = 'f';
  data_str[size*2] = 0;

  // seq, time, size, data, lqi, crc_ok, power, channel, ?, duplicate, timestamp_sync, device_id
  len = sprintf(str, "%d %.6f %d %s 255 1 %d 15 0 0 1 32767\r\n",
      sniffer->seq++, g_sim.cycle / 1000000.0, size, data_str, (int)lround(power));

  sniffer_write(sniffer, str, len);
}

//-----------------------------------------------------------------------------
static void sniffer_write(sniffer_t *sniffer, char *str, int size)
{
  int n;

  if (-1 == size)
    size = strlen(str);

  n = write(sniffer->fd, str, size);

  if (n < 0)
    error("cannot write to sniffer output file %s", sniffer->path);
}

