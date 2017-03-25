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

#ifdef PHY_SIMULATOR

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "phy.h"
#include "simulator.h"

/*- Definitions -------------------------------------------------------------*/
#define PHY_CRC_SIZE          2

#define CLEAR_IRQ_STATUS      (TRX_IRQ_RX_START | TRX_IRQ_RX_END | TRX_IRQ_TX_END)

/*- Types -------------------------------------------------------------------*/
typedef enum
{
  PHY_STATE_INITIAL,
  PHY_STATE_IDLE,
  PHY_STATE_SLEEP,
  PHY_STATE_TX_WAIT_END,
} PhyState_t;

/*- Prototypes --------------------------------------------------------------*/
static void phyTrxSetState(uint8_t state);
static void phySetRxState(void);

/*- Variables ---------------------------------------------------------------*/
static PhyState_t phyState = PHY_STATE_INITIAL;
static uint8_t phyRxBuffer[128];
static bool phyRxState;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void PHY_Init(void)
{
  phyRxState = false;
  phyState = PHY_STATE_IDLE;

  TRX_CONFIG |=
    TRX_CONFIG_TX_AUTO_CRC |
    TRX_CONFIG_RX_AUTO_CRC |
    TRX_CONFIG_TX_EXTENDED |
    TRX_CONFIG_RX_EXTENDED;

  TRX_IRQ_MASK = TRX_IRQ_RX_END | TRX_IRQ_TX_END;

  phyTrxSetState(TRX_STATE_IDLE);
}

//-----------------------------------------------------------------------------
void PHY_SetRxState(bool rx)
{
  phyRxState = rx;
  phySetRxState();
}

//-----------------------------------------------------------------------------
void PHY_SetChannel(uint8_t channel)
{
  TRX_CHANNEL = (470 + channel) * 5;
}

//-----------------------------------------------------------------------------
void PHY_SetPanId(uint16_t panId)
{
  TRX_PAN_ID = panId;
}

//-----------------------------------------------------------------------------
void PHY_SetShortAddr(uint16_t addr)
{
  TRX_SHORT_ADDR = addr;
}

//-----------------------------------------------------------------------------
void PHY_SetTxPower(uint8_t txPower)
{
  TRX_TX_POWER = txPower;
}

//-----------------------------------------------------------------------------
void PHY_Sleep(void)
{
  phyTrxSetState(TRX_STATE_SLEEP);
  phyState = PHY_STATE_SLEEP;
}

//-----------------------------------------------------------------------------
void PHY_Wakeup(void)
{
  phySetRxState();
  phyState = PHY_STATE_IDLE;
}

//-----------------------------------------------------------------------------
void PHY_DataReq(uint8_t *data, uint8_t size)
{
  phyTrxSetState(TRX_STATE_IDLE);

  TRX_IRQ_STATUS = CLEAR_IRQ_STATUS;

  TRX_FRAME_BUFFER(0) = size + PHY_CRC_SIZE;
  for (uint8_t i = 0; i < size; i++)
    TRX_FRAME_BUFFER(i+1) = data[i];

  phyState = PHY_STATE_TX_WAIT_END;
  TRX_STATE = TRX_STATE_TX;
}

//-----------------------------------------------------------------------------
static void phySetRxState(void)
{
  phyTrxSetState(TRX_STATE_IDLE);

  TRX_IRQ_STATUS = CLEAR_IRQ_STATUS;

  if (phyRxState)
    phyTrxSetState(TRX_STATE_RX);
}

//-----------------------------------------------------------------------------
static void phyTrxSetState(uint8_t state)
{
  TRX_STATE = TRX_STATE_IDLE;
  TRX_STATE = state;
}

//-----------------------------------------------------------------------------
void PHY_TaskHandler(void)
{
  if (PHY_STATE_SLEEP == phyState)
    return;

  if (TRX_IRQ_STATUS & TRX_IRQ_RX_END)
  {
    PHY_DataInd_t ind;
    uint8_t size = TRX_FRAME_BUFFER(0);

    for (uint8_t i = 0; i < size; i++)
      phyRxBuffer[i] = TRX_FRAME_BUFFER(i+1);

    ind.data = phyRxBuffer;
    ind.size = size - PHY_CRC_SIZE;
    ind.lqi  = TRX_FRAME_LQI;
    ind.rssi = (int8_t)TRX_FRAME_RSSI;
    PHY_DataInd(&ind);

    while (TRX_STATE_RX_DONE != TRX_STATE);

    TRX_IRQ_STATUS = TRX_IRQ_RX_END;
    TRX_STATE = TRX_STATE_RX;
  }

  else if (TRX_IRQ_STATUS & TRX_IRQ_TX_END)
  {
    uint8_t status = TRX_STATUS;

    if (TRX_STATUS_SUCCESS == status)
      status = PHY_STATUS_SUCCESS;
    else if (TRX_STATUS_CHANNEL_ACCESS_FAILURE == status)
      status = PHY_STATUS_CHANNEL_ACCESS_FAILURE;
    else if (TRX_STATUS_NO_ACK == status)
      status = PHY_STATUS_NO_ACK;
    else
      status = PHY_STATUS_ERROR;

    phySetRxState();
    phyState = PHY_STATE_IDLE;

    PHY_DataConf(status);

    TRX_IRQ_STATUS = TRX_IRQ_TX_END;
  }
}

#endif // PHY_SIMULATOR

