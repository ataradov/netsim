/*
 * Copyright (c) 2014, Alex Taradov <taradov@gmail.com>
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

/*- Includes ----------------------------------------------------------------*/
#include <unistd.h>
#include "simulator.h"

/*- Prototypes --------------------------------------------------------------*/
extern int main(void);

/*- Variables ---------------------------------------------------------------*/
extern unsigned int _etext;
extern unsigned int _data;
extern unsigned int _edata;
extern unsigned int _bss;
extern unsigned int _ebss;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void _entry(void)
{
  unsigned int *src, *dst;
  int delay = SYS_CTRL_RAND & 0xffff;

  src = &_etext;
  dst = &_data;
  while (dst < &_edata)
    *dst++ = *src++;

  dst = &_bss;
  while (dst < &_ebss)
    *dst++ = 0;

  // Random delay to spread out start time
  for (int i = 0; i < delay; i++)
    asm("nop");

  main();
  while (1);
}

//-----------------------------------------------------------------------------
void _exit(int status)
{
  (void)status;
  while (1);
}

