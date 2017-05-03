/*
 * Copyright (c) 2017, Alex Taradov <alex@taradov.com>
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

/*- Includes ---------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "utils.h"
#include "simulator.h"
#include "phy.h"
#include "nwk.h"

/*- Definitions ------------------------------------------------------------*/
#define NWK_SLOT_TIME  10000 // 10 ms

enum
{
  NWK_COMMAND_DATA = 0x00,
  NWK_COMMAND_SYNC = 0x01,
};

/*- Types ------------------------------------------------------------------*/
typedef struct PACK NwkHeader_t
{
  uint8_t      src;
  uint8_t      dst;
  uint8_t      command;
} NwkHeader_t;

typedef struct PACK NwkSyncCmd_t
{
  NwkHeader_t header;
  uint32_t    ack;
} NwkSyncCmd_t;

typedef struct PACK NwkDataCmd_t
{
  NwkHeader_t header;
  uint8_t     data[NWK_MAX_PAYLOAD_SIZE];
} NwkDataCmd_t;

/*- Prototypes -------------------------------------------------------------*/
static void nwkDataReq(uint8_t *data, int size);
static void nwkStartTimer(int interval, void (*callback)(void));
static void nwkSlotTimerHandler(void);
static void nwkSyncTimerHandler(void);

/*- Variables --------------------------------------------------------------*/
static int nwkDeviceId = 0;
static uint8_t *nwkDataBuf = NULL;
static volatile int nwkDataSize = 0;
static void (*nwkTimerCallback)(void);
static uint32_t nwkSyncAck;
static NwkSyncCmd_t nwkSyncCmd;
static NwkDataCmd_t nwkDataCmd;

/*- Implementations --------------------------------------------------------*/

//-----------------------------------------------------------------------------
void NWK_Init(int id)
{
  nwkDataSize = 0;
  nwkDeviceId = id;
}

//-----------------------------------------------------------------------------
void NWK_DataReq(int dst, uint8_t *data, int size)
{
  nwkDataCmd.header.src = nwkDeviceId;
  nwkDataCmd.header.dst = dst;
  nwkDataCmd.header.command = NWK_COMMAND_DATA;

  memcpy(nwkDataCmd.data, data, size);

  nwkDataReq((uint8_t *)&nwkDataCmd, sizeof(NwkHeader_t) + size);
}

//-----------------------------------------------------------------------------
void NWK_SyncReq(void)
{
  nwkSyncCmd.header.src = nwkDeviceId;
  nwkSyncCmd.header.dst = NWK_BROADCAST_ID;
  nwkSyncCmd.header.command = NWK_COMMAND_SYNC;
  nwkSyncCmd.ack = nwkSyncAck;

  nwkSyncAck = 0;

  nwkStartTimer((NWK_MAX_SLOTS + 1) * NWK_SLOT_TIME, nwkSyncTimerHandler);

  nwkDataReq((uint8_t *)&nwkSyncCmd, sizeof(NwkSyncCmd_t));
}

//-----------------------------------------------------------------------------
static void nwkDataReq(uint8_t *data, int size)
{
  nwkDataBuf = data;

  ATOMIC_SECTION_ENTER
  nwkDataSize = size;
  ATOMIC_SECTION_LEAVE

  if (NWK_GATEWAY_ID == nwkDeviceId)
    PHY_DataReq(nwkDataBuf, nwkDataSize);
}

//-----------------------------------------------------------------------------
static void nwkStartTimer(int interval, void (*callback)(void))
{
  nwkTimerCallback = callback;

  SYS_TIMER3_COUNTER = 0;
  SYS_TIMER3_PERIOD = interval;
  SYS_TIMER3_INTFLAG = SYS_TIMER_INTFLAG_COUNT;
  SYS_TIMER3_INTENSET = SYS_TIMER_INTFLAG_COUNT;
  SYS_CTRL_INTENSET = SOC_IRQ_SYS_TIMER_3;
}

//-----------------------------------------------------------------------------
void irq_handler_timer3(void)
{
  SYS_TIMER3_PERIOD = 0;
  SYS_TIMER3_INTFLAG = SYS_TIMER_INTFLAG_COUNT;
  nwkTimerCallback();
}

//-----------------------------------------------------------------------------
static void nwkSlotTimerHandler(void)
{
  if (nwkDataSize)
    PHY_DataReq(nwkDataBuf, nwkDataSize);
}

//-----------------------------------------------------------------------------
static void nwkSyncTimerHandler(void)
{
  NWK_SyncConf();
}

//-----------------------------------------------------------------------------
void PHY_DataConf(uint8_t status)
{
  NwkHeader_t *header = (NwkHeader_t *)nwkDataBuf;

  if (NWK_COMMAND_DATA == header->command)
  {
    nwkDataSize = 0;
    NWK_DataConf(PHY_STATUS_SUCCESS == status);
  }
}

//-----------------------------------------------------------------------------
void PHY_DataInd(PHY_DataInd_t *ind)
{
  NwkHeader_t *header = (NwkHeader_t *)ind->data;

  if (ind->size < sizeof(NwkHeader_t))
    return; // Malformed frame

  if (nwkDeviceId != header->dst && NWK_BROADCAST_ID != header->dst)
    return; // This frame is not for us

  if (NWK_COMMAND_SYNC == header->command)
  {
    nwkStartTimer(nwkDeviceId * NWK_SLOT_TIME, nwkSlotTimerHandler);
  }
  else if (NWK_COMMAND_DATA == header->command)
  {
    NWK_DataInd_t nwkInd;

    nwkSyncAck |= (1 << header->src);

    nwkInd.src  = header->src;
    nwkInd.dst  = header->dst;
    nwkInd.data = ind->data + sizeof(NwkHeader_t);
    nwkInd.size = ind->size - sizeof(NwkHeader_t);
    nwkInd.lqi  = ind->lqi;
    nwkInd.rssi = ind->rssi;

    NWK_DataInd(&nwkInd);
  }
}

