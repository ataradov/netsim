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
#include "utils.h"
#include "simulator.h"
#include "config.h"
#include "phy.h"
#include "nwk.h"

/*- Definitions ------------------------------------------------------------*/
#define APP_MAX_PAYLOAD_SIZE    32

/*- Types ------------------------------------------------------------------*/
typedef enum AppState_t
{
  APP_STATE_INITIAL,
  APP_STATE_IDLE,
  APP_STATE_WAIT_CONF,
} AppState_t;

typedef struct PACK
{
  uint32_t     counter;
  uint8_t      data[APP_MAX_PAYLOAD_SIZE];
} AppData_t;

/*- Prototypes -------------------------------------------------------------*/

/*- Variables --------------------------------------------------------------*/
static AppData_t appData;
static uint32_t counters[NWK_MAX_SLOTS];

/*- Implementations --------------------------------------------------------*/

//-----------------------------------------------------------------------------
void NWK_DataInd(NWK_DataInd_t *ind)
{
  AppData_t *data = (AppData_t *)ind->data;

  if (0 != SYS_CTRL_ID)
    return;

  if ((counters[ind->src] + 1) != data->counter)
    SYS_CTRL_LOG = "Counter mismatch";

  counters[ind->src] = data->counter;
}

//-----------------------------------------------------------------------------
static void appSendData(void)
{
  appData.counter++;
  NWK_DataReq(NWK_GATEWAY_ID, (uint8_t *)&appData, sizeof(AppData_t));
}

//-----------------------------------------------------------------------------
void NWK_DataConf(bool status)
{
  appSendData(); // Continuously send data
  (void)status;
}

//-----------------------------------------------------------------------------
void NWK_SyncConf(void)
{
  SYS_CTRL_LOG = "Sync";
  NWK_SyncReq();
}

//-----------------------------------------------------------------------------
static void appInit(void)
{
  PHY_SetChannel(APP_CHANNEL);
  PHY_SetRxState(true);

  if (0 == SYS_CTRL_ID)
  {
    SYS_CTRL_LOG = "Device role: gateway";

    for (int i = 0; i < NWK_MAX_SLOTS; i++)
      counters[i] = 0;

    NWK_SyncReq();
  }
  else
  {
    SYS_CTRL_LOG = "Device role: end node";

    appData.counter = 0;

    for (int i = 0; i < APP_MAX_PAYLOAD_SIZE; i++)
      appData.data[i] = i;

    appSendData();
  }
}

//-----------------------------------------------------------------------------
int main(void)
{
  SYS_CTRL_LOG = "--- main() ---";

  PHY_Init();
  NWK_Init(SYS_CTRL_ID);
  appInit();

  while (1)
  {
    asm("wfi");
  }
}

