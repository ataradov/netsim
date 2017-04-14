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

#ifndef _TRX_H_
#define _TRX_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "io_ops.h"
#include "utils.h"
#include "events.h"

/*- Types -------------------------------------------------------------------*/
enum
{
  TRX_CONFIG_REG        = 0x00,
  TRX_PAN_ID_REG        = 0x04,
  TRX_SHORT_ADDR_REG    = 0x08,
  TRX_IEEE_ADDR_0_REG   = 0x0c,
  TRX_IEEE_ADDR_1_REG   = 0x10,
  TRX_TX_POWER_REG      = 0x14,
  TRX_RX_SENSITIVITY    = 0x18,
  TRX_CHANNEL_REG       = 0x1c,
  TRX_SFD_VALUE_REG     = 0x20,
  TRX_STATE_REG         = 0x24,
  TRX_STATUS_REG        = 0x28,
  TRX_IRQ_MASK_REG      = 0x2c,
  TRX_IRQ_STATUS_REG    = 0x30,
  TRX_FRAME_RETRIES_REG = 0x34,
  TRX_CSMA_RETRIES_REG  = 0x38,
  TRX_CSMA_MIN_BE_REG   = 0x3c,
  TRX_CSMA_MAX_BE_REG   = 0x40,
  TRX_CCA_MODE_REG      = 0x44,
  TRX_ED_THRESHOLD_REG  = 0x48,
  TRX_RSSI_LEVEL_REG    = 0x4c,
  TRX_FRAME_LQI_REG     = 0x50,
  TRX_FRAME_RSSI_REG    = 0x54,
  TRX_FRAME_START_REG   = 0x1000,
};

enum
{
  TRX_CONFIG_TX_AUTO_CRC      = 1 << 0,
  TRX_CONFIG_RX_AUTO_CRC      = 1 << 1,
  TRX_CONFIG_TX_EXTENDED      = 1 << 2,
  TRX_CONFIG_RX_EXTENDED      = 1 << 3,
  TRX_CONFIG_AACK_COORD       = 1 << 4,
  TRX_CONFIG_AACK_DISABLE_ACK = 1 << 5,
  TRX_CONFIG_AACK_PENDING     = 1 << 6,
  TRX_CONFIG_AACK_FRAME_VER_0 = 1 << 7,
  TRX_CONFIG_AACK_FRAME_VER_1 = 1 << 8,
  TRX_CONFIG_AACK_FRAME_VER_2 = 1 << 9,
  TRX_CONFIG_AACK_FRAME_VER_3 = 1 << 10,
  TRX_CONFIG_AACK_PROM_MODE   = 1 << 11,
  TRX_CONFIG_AACK_ACK_TIME    = 1 << 12,
  TRX_CONFIG_AACK_RECV_RES    = 1 << 13,
  TRX_CONFIG_AACK_FLT_RES     = 1 << 14,
};

enum
{
  TRX_STATE_IDLE                = 0,
  TRX_STATE_SLEEP               = 1,
  TRX_STATE_TX                  = 2,
  TRX_STATE_TX_WAIT_BACKOFF     = 3,
  TRX_STATE_TX_WAIT_END         = 4,
  TRX_STATE_TX_WAIT_END_AACK    = 5,
  TRX_STATE_TX_WAIT_ACK         = 6,
  TRX_STATE_TX_DONE             = 7,
  TRX_STATE_RX                  = 8,
  TRX_STATE_RX_WAIT_END         = 9,
  TRX_STATE_RX_WAIT_END_AACK    = 10,
  TRX_STATE_RX_WAIT_ACK_TIMEOUT = 11,
  TRX_STATE_RX_DONE             = 12,
};

enum
{
  TRX_STATUS_INVALID                 = 0,
  TRX_STATUS_SUCCESS                 = 1,
  TRX_STATUS_SUCCESS_DATA_PENDING    = 2,
  TRX_STATUS_CHANNEL_ACCESS_FAILURE  = 3,
  TRX_STATUS_NO_ACK                  = 4,
  TRX_STATUS_CRC_OK                  = 5,
  TRX_STATUS_CRC_FAIL                = 6,
};

enum
{
  TRX_IRQ_RX_START = 1 << 0,
  TRX_IRQ_RX_END   = 1 << 1,
  TRX_IRQ_TX_END   = 1 << 2,
};

typedef struct trx_t
{
  queue_t      queue;

  void         *soc;
  char         *name;
  int          uid;
  int          irq;
  float        x;
  float        y;

  float        *loss_trx;
  float        *loss_noise;

  bool         tx;
  event_t      tx_event;
  int          tx_csma_be;
  int          tx_csma_ret;
  int          tx_frame_ret;
  uint8_t      tx_data[128];

  bool         rx;
  event_t      rx_event;
  struct trx_t *rx_trx;
  bool         rx_trx_lock;
  float        rx_lqi;
  float        rx_rssi;
  float        rx_carrier;
  float        rx_dist;
  bool         rx_crc_ok;

  struct
  {
    uint32_t   config;         // 0x00
    uint32_t   pan_id;         // 0x04
    uint32_t   short_addr;     // 0x08
    uint32_t   ieee_addr_0;    // 0x0c
    uint32_t   ieee_addr_1;    // 0x10
    float      tx_power;       // 0x14
    float      rx_sensitivity; // 0x18
    uint32_t   channel;        // 0x1c
    uint32_t   sfd;            // 0x20
    uint32_t   state;          // 0x24
    uint32_t   status;         // 0x28
    uint32_t   irq_mask;       // 0x2c
    uint32_t   irq_status;     // 0x30
    uint32_t   frame_retries;  // 0x34
    uint32_t   csma_retries;   // 0x38
    uint32_t   csma_min_be;    // 0x3c
    uint32_t   csma_max_be;    // 0x40
    uint32_t   cca_mode;       // 0x44
    float      ed_threshold;   // 0x48
    float      rssi_level;     // 0x4c
    uint32_t   frame_lqi;      // 0x50
    float      frame_rssi;     // 0x54
  } reg;

  uint8_t      buf[128];       // 0x1000
} trx_t;

/*- Prototypes --------------------------------------------------------------*/
void trx_init(trx_t *trx);
void trx_set_state(trx_t *trx, uint8_t state);
void trx_rx_start(trx_t *trx);
void trx_rx_end(trx_t *trx, bool normal);

/*- Variables ---------------------------------------------------------------*/
extern io_ops_t trx_ops;

#endif // _TRX_H_

