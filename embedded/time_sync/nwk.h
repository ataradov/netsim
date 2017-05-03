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

#ifndef _NWK_H_
#define _NWK_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/*- Definitions -------------------------------------------------------------*/
#define NWK_GATEWAY_ID        0
#define NWK_BROADCAST_ID      0xff
#define NWK_MAX_PAYLOAD_SIZE  (127 - 16/*NwkHeader_t*/ - 2/*crc*/)

/*- Types -------------------------------------------------------------------*/
typedef struct NWK_DataInd_t
{
  uint8_t      src;
  uint8_t      dst;
  uint8_t      *data;
  uint8_t      size;
  uint8_t      lqi;
  int8_t       rssi;
} NWK_DataInd_t;

/*- Prototypes --------------------------------------------------------------*/
void NWK_Init(int id);
void NWK_SyncReq();
void NWK_DataReq(int dst, uint8_t *data, int size);

void NWK_DataConf(bool status);
void NWK_DataInd(NWK_DataInd_t *ind);
void NWK_SyncConf(void);

#endif // _NWK_H_

