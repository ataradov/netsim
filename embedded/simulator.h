/*
 * Copyright (c) 2014-2017, Alex Taradov <alex@taradov.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>

/*- Definitions -------------------------------------------------------------*/
#define MMIO_REG(addr, type)   (*(volatile type *)(addr))

#define SYS_CTRL_UID           MMIO_REG(0x01000000, uint32_t)
#define SYS_CTRL_ID            MMIO_REG(0x01000004, uint32_t)
#define SYS_CTRL_RAND          MMIO_REG(0x01000008, uint32_t)
#define SYS_CTRL_LOG           MMIO_REG(0x0100000c, char *)

#define SYS_TIMER_CONTROL      MMIO_REG(0x02000000, uint32_t)
#define SYS_TIMER_PERIOD       MMIO_REG(0x02000004, uint32_t)
#define SYS_TIMER_COUNTER      MMIO_REG(0x02000008, uint32_t)
#define SYS_TIMER_INTENCLR     MMIO_REG(0x0200000c, uint32_t)
#define SYS_TIMER_INTENSET     MMIO_REG(0x02000010, uint32_t)
#define SYS_TIMER_INTMASK      MMIO_REG(0x02000014, uint32_t)
#define SYS_TIMER_INTFLAG      MMIO_REG(0x02000018, uint32_t)

#define SYS_TIMER_INTFLAG_COUNT    (1 << 0)

#define TRX_CONFIG             MMIO_REG(0x40000000, uint32_t)
#define TRX_PAN_ID             MMIO_REG(0x40000004, uint32_t)
#define TRX_SHORT_ADDR         MMIO_REG(0x40000008, uint32_t)
#define TRX_IEEE_ADDR_0        MMIO_REG(0x4000000c, uint32_t)
#define TRX_IEEE_ADDR_1        MMIO_REG(0x40000010, uint32_t)
#define TRX_TX_POWER           MMIO_REG(0x40000014, float)
#define TRX_RX_SENSITIVITY     MMIO_REG(0x40000018, float)
#define TRX_CHANNEL            MMIO_REG(0x4000001c, uint32_t)
#define TRX_SFD_VALUE          MMIO_REG(0x40000020, uint32_t)
#define TRX_STATE              MMIO_REG(0x40000024, uint32_t)
#define TRX_STATUS             MMIO_REG(0x40000028, uint32_t)
#define TRX_IRQ_MASK           MMIO_REG(0x4000002c, uint32_t)
#define TRX_IRQ_STATUS         MMIO_REG(0x40000030, uint32_t)
#define TRX_FRAME_RETRIES      MMIO_REG(0x40000034, uint32_t)
#define TRX_CSMA_RETRIES       MMIO_REG(0x40000038, uint32_t)
#define TRX_CSMA_MIN_BE        MMIO_REG(0x4000003c, uint32_t)
#define TRX_CSMA_MAX_BE        MMIO_REG(0x40000040, uint32_t)
#define TRX_CCA_MODE           MMIO_REG(0x40000044, uint32_t)
#define TRX_ED_THRESHOLD       MMIO_REG(0x40000048, float)
#define TRX_RSSI_LEVEL         MMIO_REG(0x4000004c, float)
#define TRX_FRAME_LQI          MMIO_REG(0x40000050, uint32_t)
#define TRX_FRAME_RSSI         MMIO_REG(0x40000054, float)
#define TRX_FRAME_BUFFER(i)    MMIO_REG(0x40001000 + (i), uint8_t)

#define BREAK                  MMIO_REG(0xff000000, uint32_t)

/*- Types -------------------------------------------------------------------*/
enum
{
  TRX_CONFIG_TX_AUTO_CRC       = 1 << 0,
  TRX_CONFIG_RX_AUTO_CRC       = 1 << 1,
  TRX_CONFIG_TX_EXTENDED       = 1 << 2,
  TRX_CONFIG_RX_EXTENDED       = 1 << 3,
  TRX_CONFIG_AACK_COORD        = 1 << 4,
  TRX_CONFIG_AACK_DISABLE_ACK  = 1 << 5,
  TRX_CONFIG_AACK_PENDING      = 1 << 6,
  TRX_CONFIG_AACK_FRAME_VER_0  = 1 << 7,
  TRX_CONFIG_AACK_FRAME_VER_1  = 1 << 8,
  TRX_CONFIG_AACK_FRAME_VER_2  = 1 << 9,
  TRX_CONFIG_AACK_FRAME_VER_3  = 1 << 10,
  TRX_CONFIG_AACK_PROM_MODE    = 1 << 11,
  TRX_CONFIG_AACK_ACK_TIME     = 1 << 12,
  TRX_CONFIG_AACK_RECV_RES     = 1 << 13,
  TRX_CONFIG_AACK_FLT_RES      = 1 << 14,
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

#endif // _SIMULATOR_H_

