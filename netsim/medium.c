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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "medium.h"
#include "trx.h"
#include "main.h"
#include "noise.h"
#include "utils.h"

/*- Definitions -------------------------------------------------------------*/
#define C               299792458.0f  // m/s
#define NOISE_FLOOR     (-120.0) // dBm
#define ADD_PATH_LOSS   6.0 // dB

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static inline float distance(float x1, float y1, float x2, float y2)
{
  return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

//-----------------------------------------------------------------------------
static inline float padd(float a, float b)
{
  return 10.0*log10f(powf(10.0, a/10.0) + powf(10.0, b/10.0));
}

//-----------------------------------------------------------------------------
static inline float psub(float a, float b)
{
  return 10.0*log10f(powf(10.0, a/10.0) - powf(10.0, b/10.0));
}

//-----------------------------------------------------------------------------
static inline float lqi_limit(float lqi)
{
  if (lqi < 0.0)
    return 0.0;
  else if (lqi > 1.0)
    return 1.0;
  else
    return lqi;
}

//-----------------------------------------------------------------------------
void medium_update_trx(trx_t *rx_trx)
{
  float noise, power, lambda, dist, loss, add_loss, freq;
  float lqi_carrier, lqi_noise, lqi_power;
  float carriers[3], dists[3];
  trx_t *trxs[3];

  noise = NOISE_FLOOR;
  freq = rx_trx->reg.channel * MHz;
  lambda = C / freq;

  for (int i = 0; i < 3; i++)
  {
    trxs[i] = NULL;
    carriers[i] = NOISE_FLOOR;
    dists[i] = 10000;
  }

  queue_foreach(trx_t, tx_trx, &g_sim.trxs)
  {
    if (tx_trx == rx_trx || !tx_trx->tx || tx_trx->reg.channel != rx_trx->reg.channel)
      continue;

    dist = distance(rx_trx->x, rx_trx->y, tx_trx->x, tx_trx->y);
    loss = 20.0*log10f(4.0*M_PI * dist / lambda);
    add_loss = rx_trx->loss_trx ? rx_trx->loss_trx[tx_trx->uid] : 0.0;
    power = tx_trx->reg.tx_power - loss - add_loss - ADD_PATH_LOSS;

    // Simulates random power loss due to fading and multipath propagation (-10 - 0 dB)
    power += -10.0 * randf_next();

    if (power < rx_trx->reg.rx_sensitivity)
      continue;

    if (power > carriers[0])
    {
      trxs[2] = trxs[1];
      trxs[1] = trxs[0];
      trxs[0] = tx_trx;

      dists[2] = dists[1];
      dists[1] = dists[0];
      dists[0] = dist;

      carriers[2] = carriers[1];
      carriers[1] = carriers[0];
      carriers[0] = power;
    }
    else if (power > carriers[1])
    {
      trxs[2] = trxs[1];
      trxs[1] = tx_trx;

      dists[2] = dists[1];
      dists[1] = dist;

      carriers[2] = carriers[1];
      carriers[1] = power;
    }
    else if (power > carriers[2])
    {
      trxs[2] = tx_trx;
      dists[2] = dist;
      carriers[2] = power;
    }

    noise = padd(noise, power);
  }

  queue_foreach(noise_t, tx_noise, &g_sim.noises)
  {
    if (!tx_noise->active || freq < tx_noise->freq_a || freq > tx_noise->freq_b)
      continue;

    dist = distance(rx_trx->x, rx_trx->y, tx_noise->x, tx_noise->y);
    loss = 20.0*log10f(4.0*M_PI * dist / lambda);
    add_loss = rx_trx->loss_noise ? rx_trx->loss_noise[tx_noise->uid] : 0.0;
    power = tx_noise->power - loss - add_loss;
    noise = padd(noise, power);
  }

  rx_trx->rx_rssi = noise;
  rx_trx->rx_carrier = carriers[0];
  rx_trx->rx_dist = dists[0];

  if (rx_trx->rx_trx != trxs[0])
    rx_trx->rx_crc_ok = false;

  if (!rx_trx->rx_trx_lock)
    rx_trx->rx_trx = trxs[0];

  if (NULL == trxs[0])
    return;

  // LQI drop due to correlated noise
  if (NULL != trxs[1])
    lqi_carrier = (carriers[0] - carriers[1]) / 3.0;
  else
    lqi_carrier = 1.0;

  lqi_carrier = lqi_limit(lqi_carrier);

  // LQI drop due to uncorrelated noise
  noise = psub(noise, carriers[0]);
  lqi_noise = (carriers[0] - noise) / 3.0;
  lqi_noise = lqi_limit(lqi_noise);

  // LQI drop due to RX power level
  lqi_power = 1.0 - expf(-0.2*(carriers[0] - NOISE_FLOOR));
  lqi_power = lqi_limit(lqi_power);

  rx_trx->rx_lqi *= lqi_carrier * lqi_noise * lqi_power;
}

//-----------------------------------------------------------------------------
void medium_tx_start(trx_t *trx)
{
  queue_foreach(trx_t, rx_trx, &g_sim.trxs)
  {
    if (rx_trx->rx)
      medium_update_trx(rx_trx);

    if (rx_trx->rx && rx_trx->rx_trx == trx && rx_trx->reg.sfd == trx->reg.sfd)
      trx_rx_start(rx_trx);
  }
}

//-----------------------------------------------------------------------------
void medium_tx_end(trx_t *trx, bool normal)
{
  queue_foreach(trx_t, rx_trx, &g_sim.trxs)
  {
    if (rx_trx->rx && rx_trx->rx_trx == trx && rx_trx->rx_trx_lock)
      trx_rx_end(rx_trx, normal);
  }

  if (normal)
  {
    float power, lambda, dist, loss, add_loss, freq;

    freq = trx->reg.channel * MHz;
    lambda = C / freq;

    queue_foreach(sniffer_t, sniffer, &g_sim.sniffers)
    {
      if (freq < sniffer->freq_a || freq > sniffer->freq_b)
        continue;

      dist = distance(sniffer->x, sniffer->y, trx->x, trx->y);
      loss = 20.0*log10f(4.0*M_PI * dist / lambda);
      add_loss = sniffer->loss_trx ? sniffer->loss_trx[trx->uid] : 0.0;
      power = trx->reg.tx_power - loss - add_loss;

      if (power < sniffer->sensitivity)
        continue;

      sniffer_write_frame(sniffer, trx->tx_data, power);
    }
  }
}

