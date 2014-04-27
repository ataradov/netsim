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
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "trx.h"
#include "soc.h"
#include "mem.h"
#include "utils.h"
#include "config.h"

/*- Variables ---------------------------------------------------------------*/
sim_t g_sim;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static void measure_time(void)
{
  static struct timeval tv_start;
  static bool first = true;

  if (first)
  {
    gettimeofday(&tv_start, NULL);
    first = false;
  }
  else
  {
    struct timeval tv_stop;
    unsigned int diff_msec;

    gettimeofday(&tv_stop, NULL);

    diff_msec = (tv_stop.tv_sec - tv_start.tv_sec)*1000;
    diff_msec += (tv_stop.tv_usec - tv_start.tv_usec)/1000;

    printf("%"PRId64" cycles in %u ms => %"PRId64" cycles/sec\n", g_sim.cycle,
        diff_msec, (g_sim.cycle*1000)/diff_msec);
  }
}

#ifdef __linux__
//-----------------------------------------------------------------------------
static void sig_handler(int signum)
{
  if (SIGINT == signum)
  {
    measure_time();
    exit(0);
  }
}

//-----------------------------------------------------------------------------
static void register_sigaction(void)
{
  static struct sigaction sigact;

  sigact.sa_handler = sig_handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
}
#endif

//-----------------------------------------------------------------------------
static void sim_init(void)
{
  g_sim.seed = 123456;
  g_sim.time = 1000000;
  g_sim.scale = 1.0f;

  g_sim.uid = 0;
  g_sim.socs = NULL;
  g_sim.trxs = NULL;
  g_sim.noises = NULL;
  g_sim.sniffers = NULL;
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  if (2 != argc)
    error("configuration file is not specified");

  sim_init();
  soc_setup();

  config_read(argv[1]);

  rand_init(g_sim.seed);

#ifdef __linux__
  register_sigaction();
#endif

  measure_time();

  for (g_sim.cycle = 0; g_sim.cycle < g_sim.time; g_sim.cycle++)
  {
    for (soc_t *soc = g_sim.socs; soc; soc = soc->next)
      soc_clk(soc);

    events_tick();
  }

  measure_time();

  return 0;
}

