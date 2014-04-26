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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trx.h"
#include "main.h"
#include "utils.h"
#include "events.h"
#include "medium.h"

/*- Definitions -------------------------------------------------------------*/
#define SYMBOLS_PER_OCTET      2
#define SYMBOL_DURATION        16 // us
#define UNIT_BACKOFF_PERIOD    20 // symbols
#define TURNAROUND_TIME        12 // symbols
#define PHY_SHR_DURATION       10 // symbols
#define PHY_PHR_DURATION       2  // symbols

#define ACK_WAIT_DURATION      ((UNIT_BACKOFF_PERIOD + TURNAROUND_TIME + PHY_SHR_DURATION + \
                                 6*SYMBOLS_PER_OCTET) * SYMBOL_DURATION)

#define PHY_PHR_OFFSET         0
#define PHY_PSDU_OFFSET        1
#define PHY_MAX_PSDU_SIZE      127
#define PHY_CRC_SIZE           2
#define MAC_ACK_SIZE           5
#define MAC_SEQ_OFFSET         3
#define MAC_BROADCAST_PANID    0xffff
#define MAC_BROADCAST_ADDR     0xffff

/*- Types -------------------------------------------------------------------*/
typedef union
{
  struct
  {
    uint16_t   frame_type    : 3;
    uint16_t   sec_enabled   : 1;
    uint16_t   frame_pending : 1;
    uint16_t   ack_request   : 1;
    uint16_t   pan_id_comp   : 1;
    uint16_t                 : 3;
    uint16_t   dst_addr_mode : 2;
    uint16_t   frame_version : 2;
    uint16_t   src_addr_mode : 2;
  };
  uint16_t     raw;
} mac_fcf_t;

typedef struct
{
  mac_fcf_t    fcf;
  bool         valid;
  uint8_t      seq_no;
  uint16_t     dst_pan_id;
  uint16_t     dst_short_addr;
  uint64_t     dst_ext_addr;
  uint16_t     src_pan_id;
  uint16_t     src_short_addr;
  uint64_t     src_ext_addr;
} mac_header_t;

enum
{
  MAC_FRAME_TYPE_BEACON    = 0,
  MAC_FRAME_TYPE_DATA      = 1,
  MAC_FRAME_TYPE_ACK       = 2,
  MAC_FRAME_TYPE_COMMAND   = 3,
};

enum
{
  MAC_ADDR_MODE_NO_ADDR    = 0,
  MAC_ADDR_MODE_SHORT_ADDR = 2,
  MAC_ADDR_MODE_EXT_ADDR   = 3,
};

/*- Prototypes --------------------------------------------------------------*/
static void trx_return_to_idle(trx_t *trx);
static void trx_send(trx_t *trx);
static void trx_receive(trx_t *trx);
static void trx_csma_backoff(trx_t *trx);
static void trx_backoff_period_cb(event_t *event);
static void trx_ack_wait_timeout_cb(event_t *event);
static void trx_rx_end_cb(event_t *event);
static void trx_transmit_frame(trx_t *trx);
static void trx_tx_end_cb(event_t *event);
static void trx_tx_ack_cb(event_t *event);
static bool trx_cca_ok(trx_t *trx);
static void trx_add_rx_event(trx_t *trx, int timeout, void (*callback)(event_t *));
static void trx_add_tx_event(trx_t *trx, int timeout, void (*callback)(event_t *));
static void trx_parse_mac_header(uint8_t *frame, mac_header_t *header);
static bool trx_filter_frame(trx_t *trx, mac_header_t *header);
static inline bool trx_config(trx_t *trx, uint32_t bit);
static inline void trx_interrupt(trx_t *trx, uint32_t status);
static void trx_insert_crc(uint8_t *data);
static bool trx_check_crc(uint8_t *data);

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void trx_init(trx_t *trx)
{
  trx->reg.config = 
      TRX_CONFIG_TX_AUTO_CRC |
      TRX_CONFIG_RX_AUTO_CRC |
      TRX_CONFIG_TX_EXTENDED |
      TRX_CONFIG_RX_EXTENDED |
      TRX_CONFIG_AACK_FRAME_VER_0 |
      TRX_CONFIG_AACK_FRAME_VER_1;
  trx->reg.pan_id          = 0;
  trx->reg.short_addr      = 0;
  trx->reg.ieee_addr_0     = 0;
  trx->reg.ieee_addr_1     = 0;
  trx->reg.tx_power        = 3.0f;
  trx->reg.channel         = 2425;
  trx->reg.sfd             = 0xa7;
  trx->reg.state           = TRX_STATE_IDLE;
  trx->reg.status          = TRX_STATUS_INVALID;
  trx->reg.irq_mask        = 0;
  trx->reg.irq_status      = 0;
  trx->reg.frame_retries   = 3;
  trx->reg.csma_retries    = 4;
  trx->reg.csma_min_be     = 3;
  trx->reg.csma_max_be     = 5;
  trx->reg.cca_mode        = 0; // ???
  trx->reg.ed_threshold    = -86.0f;

  trx->tx        = false;
  trx->rx        = false;
  trx->rx_lqi    = 1.0f;

  TRX_DBG(trx, "started (%.2f, %.2f)", trx->x, trx->y);
}

//-----------------------------------------------------------------------------
void trx_set_state(trx_t *trx, uint8_t state)
{
  if (state == trx->reg.state)
    return;

  if (TRX_STATE_IDLE == state)
  {
    trx_return_to_idle(trx);
    return;
  }

  else if (TRX_STATE_IDLE == trx->reg.state)
  {
    if (TRX_STATE_TX == state)
    {
      trx->reg.state = state;
      trx_send(trx);
      return;
    }
    else if (TRX_STATE_RX == state)
    {
      trx->reg.state = state;
      trx_receive(trx);
      return;
    }
  }

  else if (TRX_STATE_RX_DONE == trx->reg.state)
  {
    if (TRX_STATE_RX == state)
    {
      trx->reg.state = state;
      trx_receive(trx);
      return;
    }
  }

  error("%s: invalid state transition (%d -> %d)", trx->name, trx->reg.state, state);
}

//-----------------------------------------------------------------------------
static void trx_return_to_idle(trx_t *trx)
{
  if (trx->tx)
  {
    TRX_DBG(trx, "TX interrupted");
    trx->tx = false;
    medium_tx_end(trx, false);
  }

  if (trx->rx)
  {
    if (TRX_STATE_RX != trx->reg.state)
      TRX_DBG(trx, "RX interrupted");

    trx->rx = false;
    trx->rx_trx = NULL;
    trx->rx_trx_lock = false;
  }

  events_remove(&trx->rx_event);
  events_remove(&trx->tx_event);
  trx->reg.state = TRX_STATE_IDLE;
}

//-----------------------------------------------------------------------------
static void trx_send(trx_t *trx)
{
  TRX_DBG(trx, "TX");

  if (trx->buf[PHY_PHR_OFFSET] < PHY_CRC_SIZE ||
      trx->buf[PHY_PHR_OFFSET] > PHY_MAX_PSDU_SIZE)
    error("%s: invalid frame size in trx_send(): %d", trx->name, trx->buf[PHY_PHR_OFFSET]);

  memcpy(trx->tx_data, trx->buf, sizeof(trx->buf));

  if (trx_config(trx, TRX_CONFIG_TX_AUTO_CRC))
    trx_insert_crc(trx->tx_data);

  if (trx_config(trx, TRX_CONFIG_TX_EXTENDED))
  {
    trx->tx_csma_be = trx->reg.csma_min_be;
    trx->tx_csma_ret = 0;
    trx->tx_frame_ret = 0;
    trx_csma_backoff(trx);
  }
  else
  {
    trx->reg.state = TRX_STATE_TX_WAIT_END;
    trx_transmit_frame(trx);
  }
}

//-----------------------------------------------------------------------------
static void trx_receive(trx_t *trx)
{
  TRX_DBG(trx, "RX %s", (TRX_STATE_TX_WAIT_ACK == trx->reg.state) ? "(AACK)" : "");
  trx->rx = true;
}

//-----------------------------------------------------------------------------
static void trx_csma_backoff(trx_t *trx)
{
  int delay;

  trx->reg.state = TRX_STATE_TX_WAIT_BACKOFF;

  delay = rand_next() & ((1 << trx->tx_csma_be) - 1);
  delay = delay * UNIT_BACKOFF_PERIOD * SYMBOL_DURATION + 1;

  TRX_DBG(trx, "... backoff delay %d us", delay);

  trx_add_tx_event(trx, delay, trx_backoff_period_cb);
}

//-----------------------------------------------------------------------------
static void trx_backoff_period_cb(event_t *event)
{
  trx_t *trx = (trx_t *)event->data;

  if (TRX_STATE_TX_WAIT_BACKOFF != trx->reg.state)
    error("%s: invalid transceiver state in trx_backoff_period_cb():",
        trx->name, trx->reg.state);

  medium_update_trx(trx);

  if (trx_cca_ok(trx))
  {
    TRX_DBG(trx, "... CCA pass");

    trx->reg.state = TRX_STATE_TX_WAIT_END;
    trx_transmit_frame(trx);
  }
  else
  {
    TRX_DBG(trx, "... CCA fail");

    trx->tx_csma_ret++;
    trx->tx_csma_be = min(trx->tx_csma_be+1, (int)trx->reg.csma_max_be);

    if (trx->tx_csma_ret > (int)trx->reg.csma_retries)
    {
      trx->reg.state = TRX_STATE_TX_DONE;
      trx->reg.status = TRX_STATUS_CHANNEL_ACCESS_FAILURE;
      trx_interrupt(trx, TRX_IRQ_TX_END);
    }
    else
    {
      trx_csma_backoff(trx);
    }
  }
}

//-----------------------------------------------------------------------------
static void trx_transmit_frame(trx_t *trx)
{
  int size;

  trx->tx = true;
  size = PHY_SHR_DURATION + PHY_PHR_DURATION + trx->tx_data[PHY_PHR_OFFSET] * SYMBOLS_PER_OCTET;
  trx_add_tx_event(trx, size * SYMBOL_DURATION, trx_tx_end_cb);
  medium_tx_start(trx);
}

//-----------------------------------------------------------------------------
static void trx_tx_end_cb(event_t *event)
{
  trx_t *trx = (trx_t *)event->data;

  if (TRX_STATE_TX_WAIT_END != trx->reg.state &&
      TRX_STATE_TX_WAIT_END_AACK != trx->reg.state)
    error("%s: invalid transceiver state in trx_tx_end_cb(): %d",
        trx->name, trx->reg.state);

  trx->tx = false;
  medium_tx_end(trx, true);

  if (trx_config(trx, TRX_CONFIG_TX_EXTENDED))
  {
    TRX_DBG(trx, "... TX end extended");

    if (TRX_STATE_TX_WAIT_END_AACK == trx->reg.state)
    {
      TRX_DBG(trx, "ACK sent");
      trx->reg.state = TRX_STATE_RX_DONE;
    }
    else
    {
      mac_fcf_t *fcf = (mac_fcf_t *)&trx->tx_data[PHY_PSDU_OFFSET];

      if (fcf->ack_request)
      {
        TRX_DBG(trx, "... waiting for an ACK");
        trx->reg.state = TRX_STATE_TX_WAIT_ACK;
        trx_add_tx_event(trx, ACK_WAIT_DURATION, trx_ack_wait_timeout_cb);
        trx_receive(trx);
      }
      else
      {
        trx->reg.state = TRX_STATE_TX_DONE;
        trx->reg.status = TRX_STATUS_SUCCESS;
        trx_interrupt(trx, TRX_IRQ_TX_END);
      }
    }
  }
  else
  {
    TRX_DBG(trx, "... TX end basic");
    trx->reg.state = TRX_STATE_TX_DONE;
    trx->reg.status = TRX_STATUS_SUCCESS;
    trx_interrupt(trx, TRX_IRQ_TX_END);
  }
}

//-----------------------------------------------------------------------------
static void trx_ack_wait_timeout_cb(event_t *event)
{
  trx_t *trx = (trx_t *)event->data;

  if (TRX_STATE_TX_WAIT_ACK != trx->reg.state &&
      TRX_STATE_RX_WAIT_END_AACK != trx->reg.state)
    error("%s: invalid transceiver state in trx_ack_wait_timeout_cb(): %d",
        trx->name, trx->reg.state);

  TRX_DBG(trx, "... ACK wait timeout");

  trx->rx = false;
  trx->rx_trx_lock = false;
  events_remove(&trx->rx_event);

  trx->tx_frame_ret++;

  if (trx->tx_frame_ret > (int)trx->reg.frame_retries)
  {
    TRX_DBG(trx, "... no ACK received");

    trx->reg.state = TRX_STATE_TX_DONE;
    trx->reg.status = TRX_STATUS_NO_ACK;
    trx_interrupt(trx, TRX_IRQ_TX_END);
  }
  else
  {
    TRX_DBG(trx, "... frame retry");

    trx->tx_csma_be = trx->reg.csma_min_be;
    trx->tx_csma_ret = 0;
    trx_csma_backoff(trx);
  }
}

//-----------------------------------------------------------------------------
void trx_rx_start(trx_t *trx)
{
  uint8_t *data = trx->rx_trx->tx_data;
  int size;

  TRX_DBG(trx, "RX start from %s", trx->rx_trx->name);

  if (trx->reg.state == TRX_STATE_RX)
    trx->reg.state = TRX_STATE_RX_WAIT_END;
  else if (trx->reg.state == TRX_STATE_TX_WAIT_ACK)
    trx->reg.state = TRX_STATE_RX_WAIT_END_AACK;
  else
    error("%s: invalid transceiver state in trx_rx_start(): %d", trx->name, trx->reg.state);

  trx->rx_lqi = 1.0;
  trx->rx_crc_ok = true;
  trx->rx_trx_lock = true;
  size = PHY_SHR_DURATION + PHY_PHR_DURATION + data[PHY_PHR_OFFSET] * SYMBOLS_PER_OCTET;
  // Stop receiveing 1 us before transmission ends to give receiver a chance to update
  // RSSI and LQI values for the last time.
  trx_add_rx_event(trx, size * SYMBOL_DURATION - 1, trx_rx_end_cb);
  memcpy(trx->buf, data, sizeof(trx->buf));
  trx_interrupt(trx, TRX_IRQ_RX_START);
}

//-----------------------------------------------------------------------------
void trx_rx_end(trx_t *trx, bool normal)
{
  if (!normal)
  {
    // We are no longer receiving from this transmitter, but trx->rx_trx_lock should
    // remain true because we want to ignore any new transmissions from the same
    // transmitter before trx_rx_end_cb() is fired on our side.
    trx->rx_trx = NULL;
    trx->rx_crc_ok = false;
  }
}

//-----------------------------------------------------------------------------
static void trx_rx_end_cb(event_t *event)
{
  trx_t *trx = (trx_t *)event->data;
  float p_loss, random;

  medium_update_trx(trx);

  trx->rx = false;
  trx->rx_trx_lock = false;

  if (TRX_STATE_RX_WAIT_END != trx->reg.state &&
      TRX_STATE_RX_WAIT_END_AACK != trx->reg.state)
    error("%s: spurious trx_rx_end_cb()", trx->name);

  // This approximates the dependency from a real radio.
  p_loss = (tanhf((0.5 - trx->rx_lqi) * 5.5) + 1.0) / 2.0;
  random = randf_next();

  if (p_loss > random)
  {
    TRX_DBG(trx, "Frame is randomly lost due to LQI (P_loss = %.5f, random = %.5f)", p_loss, random);
    trx->rx_crc_ok = false;
  }

  if (!trx->rx_crc_ok)    // Damage the buffer so that even if application checks
    trx->buf[1] ^= 0xff;  // the CRC manually, it will fail.

  trx->rx_crc_ok = trx->rx_crc_ok && trx_check_crc(trx->buf);
  trx->reg.status = trx->rx_crc_ok ? TRX_STATUS_CRC_OK : TRX_STATUS_CRC_FAIL;
  trx->reg.frame_lqi = lround(trx->rx_lqi * 255);
  trx->reg.frame_rssi = trx->rx_rssi;

  TRX_DBG(trx, "RX end from %s, LQI = %.2f (%d), RSSI = %.2f, CRC = %s",
      trx->rx_trx ? trx->rx_trx->name : "<unknown>", trx->rx_lqi, trx->reg.frame_lqi,
      trx->rx_rssi, trx->rx_crc_ok ? "OK" : "Fail");

  if (trx_config(trx, TRX_CONFIG_TX_EXTENDED))
  {
    mac_header_t header;

    trx_parse_mac_header(trx->buf, &header);

    if (TRX_STATE_RX_WAIT_END_AACK == trx->reg.state)
    {
      if (header.valid && MAC_FRAME_TYPE_ACK == header.fcf.frame_type &&
          trx->tx_data[MAC_SEQ_OFFSET] == header.seq_no)
      {
        TRX_DBG(trx, "... valid ACK received");

        events_remove(&trx->tx_event);

        trx->reg.state = TRX_STATE_TX_DONE;
        trx->reg.status = TRX_STATUS_SUCCESS;
        trx_interrupt(trx, TRX_IRQ_TX_END);
      }
      else
      {
        trx->rx = true;
        trx->reg.state = TRX_STATE_TX_WAIT_ACK;
      }
    }

    else if (trx_filter_frame(trx, &header))
    {
      bool disable_ack = trx_config(trx, TRX_CONFIG_AACK_DISABLE_ACK);
      bool frame_pending = trx_config(trx, TRX_CONFIG_AACK_PENDING);

      if (header.fcf.ack_request && !disable_ack)
      {
        trx->tx_data[0] = MAC_ACK_SIZE;
        trx->tx_data[1] = frame_pending ? 0x12 : 0x02;
        trx->tx_data[2] = 0x00;
        trx->tx_data[3] = header.seq_no;
        trx->tx_data[4] = 0xff;  // CRC placeholder
        trx->tx_data[5] = 0xff;
        trx_insert_crc(trx->tx_data);

        trx->reg.state = TRX_STATE_RX_WAIT_ACK_TIMEOUT;
        trx_add_rx_event(trx, TURNAROUND_TIME * SYMBOL_DURATION, trx_tx_ack_cb);
        trx_interrupt(trx, TRX_IRQ_RX_END);
      }
      else
      {
        trx->reg.state = TRX_STATE_RX_DONE;
        trx_interrupt(trx, TRX_IRQ_RX_END);
      }
    }
    else
    {
      trx->reg.state = TRX_STATE_RX;
      trx_receive(trx);
    }
  }
  else
  {
    if (trx_config(trx, TRX_CONFIG_RX_AUTO_CRC))
    {
      if (trx->rx_crc_ok)
      {
        trx->reg.state = TRX_STATE_RX_DONE;
        trx_interrupt(trx, TRX_IRQ_RX_END);
      }
      else
      {
        trx->reg.state = TRX_STATE_RX;
        trx_receive(trx);
      }
    }
    else
    {
      trx->reg.state = TRX_STATE_RX_DONE;
      trx_interrupt(trx, TRX_IRQ_RX_END);
    }
  }
}

//-----------------------------------------------------------------------------
static void trx_tx_ack_cb(event_t *event)
{
  trx_t *trx = (trx_t *)event->data;

  if (TRX_STATE_RX_WAIT_ACK_TIMEOUT != trx->reg.state)
    error("%s: invalid transceiver state in trx_tx_ack_cb(): %d",
        trx->name, trx->reg.state);

  TRX_DBG(trx, "Sending an ACK");

  trx->reg.state = TRX_STATE_TX_WAIT_END_AACK;
  trx_transmit_frame(trx);
}

//-----------------------------------------------------------------------------
static bool trx_cca_ok(trx_t *trx)
{
  float thr = trx->reg.ed_threshold;
  int mode = trx->reg.cca_mode;

  switch (mode)
  {
    case 0: return trx->rx_rssi < thr;
    case 1: return trx->rx_carrier < thr;
    case 2: return trx->rx_carrier < thr && trx->rx_rssi < thr;
    case 3: return trx->rx_carrier < thr || trx->rx_rssi < thr;
    case 4: return true;
    default: error("%s: invalid CCA mode (%d)", trx->name, mode);
  }

  return false;
}

//-----------------------------------------------------------------------------
static void trx_add_rx_event(trx_t *trx, int timeout, void (*callback)(event_t *))
{
  if (events_is_planned(&trx->rx_event))
    error("%s: another RX event is already planned", trx->name);

  trx->rx_event.timeout = timeout;
  trx->rx_event.callback = callback;
  trx->rx_event.data = (void *)trx;
  events_add(&trx->rx_event);
}

//-----------------------------------------------------------------------------
static void trx_add_tx_event(trx_t *trx, int timeout, void (*callback)(event_t *))
{
  if (events_is_planned(&trx->tx_event))
    error("%s: another TX event is already planned", trx->name);

  trx->tx_event.timeout = timeout;
  trx->tx_event.callback = callback;
  trx->tx_event.data = (void *)trx;
  events_add(&trx->tx_event);
}

//-----------------------------------------------------------------------------
static void trx_parse_mac_header(uint8_t *frame, mac_header_t *header)
{
  int size;

  memset(header, 0, sizeof(mac_header_t));

  size = *frame;
  frame += sizeof(uint8_t);

  if (size < MAC_ACK_SIZE)
  {
    header->valid = false;
    return;
  }

  header->fcf.raw = *((uint16_t *)frame);
  frame += sizeof(uint16_t);
  size -= sizeof(uint16_t);

  header->seq_no = *frame;
  frame += sizeof(uint8_t);
  size -= sizeof(uint8_t);

  if (header->fcf.dst_addr_mode != MAC_ADDR_MODE_NO_ADDR)
  {
    header->dst_pan_id = *((uint16_t *)frame);
    frame += sizeof(uint16_t);
    size -= sizeof(uint16_t);

    if (header->fcf.dst_addr_mode == MAC_ADDR_MODE_SHORT_ADDR)
    {
      header->dst_short_addr = *((uint16_t *)frame);
      frame += sizeof(uint16_t);
      size -= sizeof(uint16_t);
    }
    else
    {
      header->dst_ext_addr = *((uint64_t *)frame);
      frame += sizeof(uint64_t);
      size -= sizeof(uint64_t);
    }
  }

  if (header->fcf.src_addr_mode != MAC_ADDR_MODE_NO_ADDR)
  {
    if (0 == header->fcf.pan_id_comp)
    {
      header->src_pan_id = *((uint16_t *)frame);
      frame += sizeof(uint16_t);
      size -= sizeof(uint16_t);
    }

    if (header->fcf.src_addr_mode == MAC_ADDR_MODE_SHORT_ADDR)
    {
      header->src_short_addr = *((uint16_t *)frame);
      frame += sizeof(uint16_t);
      size -= sizeof(uint16_t);
    }
    else
    {
      header->src_ext_addr = *((uint64_t *)frame);
      frame += sizeof(uint64_t);
      size -= sizeof(uint64_t);
    }
  }

  header->valid = (size >= 0);
}

//-----------------------------------------------------------------------------
static bool trx_filter_frame(trx_t *trx, mac_header_t *header)
{
  int frame_type      = header->fcf.frame_type;
  int frame_version   = header->fcf.frame_version;
  int dst_addr_mode   = header->fcf.dst_addr_mode;
  int src_addr_mode   = header->fcf.src_addr_mode;
  bool reserved       = frame_type > MAC_FRAME_TYPE_COMMAND;
  uint16_t pan_id     = trx->reg.pan_id;
  uint16_t short_addr = trx->reg.short_addr;
  uint64_t ieee_addr  = ((uint64_t)trx->reg.ieee_addr_1 << 32) | trx->reg.ieee_addr_0;
  bool is_coord       = trx_config(trx, TRX_CONFIG_AACK_COORD);

  if (!header->valid || !trx->rx_crc_ok)
    return false;

  if (reserved && !trx_config(trx, TRX_CONFIG_AACK_RECV_RES))
    return false;

  if (reserved && !trx_config(trx, TRX_CONFIG_AACK_FLT_RES))
    return true;

  if (!trx_config(trx, TRX_CONFIG_AACK_FRAME_VER_0 << frame_version))
    return false;

  if (MAC_ADDR_MODE_SHORT_ADDR == dst_addr_mode || MAC_ADDR_MODE_EXT_ADDR == dst_addr_mode)
  {
    if (pan_id != header->dst_pan_id && MAC_BROADCAST_PANID != header->dst_pan_id)
      return false;
  }

  if (MAC_ADDR_MODE_SHORT_ADDR == dst_addr_mode)
  {
    if (short_addr != header->dst_short_addr && MAC_BROADCAST_ADDR != header->dst_short_addr)
      return false;
  }
  else if (MAC_ADDR_MODE_EXT_ADDR == dst_addr_mode)
  {
    if (ieee_addr != header->dst_ext_addr)
      return false;
  }

  if (MAC_FRAME_TYPE_BEACON == frame_type)
  {
    if (MAC_BROADCAST_PANID != pan_id)
    {
      if (header->src_pan_id != pan_id)
        return false;
    }
  }

  if (MAC_FRAME_TYPE_DATA == frame_type || MAC_FRAME_TYPE_COMMAND == frame_type)
  {
    if ((MAC_ADDR_MODE_SHORT_ADDR == src_addr_mode || MAC_ADDR_MODE_EXT_ADDR == src_addr_mode) &&
         MAC_ADDR_MODE_NO_ADDR == dst_addr_mode)
    {
      if (!(is_coord && header->src_pan_id == pan_id))
        return false;
    }
  }

  if (MAC_FRAME_TYPE_ACK == frame_type)
    return false;

  if (MAC_ADDR_MODE_NO_ADDR == src_addr_mode && MAC_ADDR_MODE_NO_ADDR == dst_addr_mode)
    return false;

  return true;
}

//-----------------------------------------------------------------------------
static inline bool trx_config(trx_t *trx, uint32_t bit)
{
  return (trx->reg.config & bit) > 0;
}

//-----------------------------------------------------------------------------
static inline void trx_interrupt(trx_t *trx, uint32_t status)
{
  trx->reg.irq_status |= status & trx->reg.irq_mask;
  //  TODO: call interrupt handler
}

//-----------------------------------------------------------------------------
static inline uint16_t trx_update_crc_ccitt(uint16_t crc, uint8_t data)
{
  data ^= crc & 0xff;
  data ^= data << 4;
 
  return ((((uint16_t)data << 8) | ((crc & 0xff00) >> 8)) ^
            (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

//-----------------------------------------------------------------------------
static void trx_insert_crc(uint8_t *data)
{
  uint16_t crc = 0;
  int size = data[PHY_PHR_OFFSET] - PHY_CRC_SIZE;

  for (int i = 0; i < size; i++)
    crc = trx_update_crc_ccitt(crc, data[i+1]);

  data[size + 1] = crc & 0xff;
  data[size + 2] = crc >> 8;
}

//-----------------------------------------------------------------------------
static bool trx_check_crc(uint8_t *data)
{
  int size = data[PHY_PHR_OFFSET];
  uint16_t crc = 0;

  for (int i = 0; i < size; i++)
    crc = trx_update_crc_ccitt(crc, data[i+1]);

  return (0 == crc);
}

//-----------------------------------------------------------------------------
uint8_t trx_read_b(trx_t *trx, uint32_t addr)
{
  uint8_t *m = (uint8_t *)trx->buf;
  return m[addr & TRX_REG_MASK];
}

//-----------------------------------------------------------------------------
uint32_t trx_read_w(trx_t *trx, uint32_t addr)
{
  uint32_t *m = (uint32_t *)&trx->reg;
  return m[(addr & TRX_REG_MASK) >> 2];
}

//-----------------------------------------------------------------------------
void trx_write_b(trx_t *trx, uint32_t addr, uint8_t data)
{
  uint8_t *m = (uint8_t *)trx->buf;
  m[addr & TRX_REG_MASK] = data;
}

//-----------------------------------------------------------------------------
void trx_write_w(trx_t *trx, uint32_t addr, uint32_t data)
{
  uint32_t *m = (uint32_t *)&trx->reg;

  addr &= TRX_REG_MASK;

  if (TRX_STATE_REG == addr)
    trx_set_state(trx, data);
  else if (TRX_IRQ_STATUS_REG == addr)
    trx->reg.irq_status &= ~data;
  else
    m[addr >> 2] = data;
}

