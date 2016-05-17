/* Copyright (c) 2014, Dimitar Dimitrov
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. */

#include "resource_table_0.h"

#define PRU0

#include "linux_types.h"
#include "pru_defs.h"
#include "prucomm.h"


#ifdef __GNUC__
#include <pru/io.h>
#endif

#ifndef __GNUC__
volatile register uint32_t __R31;
#endif

#define TIME_SUB(x, y) ((x >= y)?(x - y):(0xFFFFFFFF - y + x))

#if 0
static void delay_us(unsigned int us)
{
	/* assume cpu frequency is 200MHz */
	__delay_cycles (us * (1000 / 5));
}
#endif

void add_to_ring_buffer(uint8_t v, uint16_t deltat)
{
    RBUFF->buffer[RBUFF->ring_tail].pin_value = v;
    RBUFF->buffer[RBUFF->ring_tail].delta_t = deltat;
    RBUFF->ring_tail = (RBUFF->ring_tail + 1) % NUM_RING_ENTRIES;
}

static inline u32 read_PIEP_COUNT(void)
{
    return PIEP_COUNT;
}

uint32_t read_pin(void){
    return ((__R31&(1<<15)) != 0);
    // return ((read_r31()&(1<<15)) != 0);
}

// const unsigned int period_us = 250 * 1000;

int main(void)
{
     uint32_t last_time = 0;
     uint32_t last_pin_value = 0x0;
     /*PRU Initialisation*/
     PRUCFG_SYSCFG &= ~SYSCFG_STANDBY_INIT;
     PRUCFG_SYSCFG = (PRUCFG_SYSCFG &
             ~(SYSCFG_IDLE_MODE_M | SYSCFG_STANDBY_MODE_M)) |
             SYSCFG_IDLE_MODE_NO | SYSCFG_STANDBY_MODE_NO;
     /* our PRU wins arbitration */
     PRUCFG_SPP |=  SPP_PRU1_PAD_HP_EN;
    
    /* configure timer */
    PIEP_GLOBAL_CFG = GLOBAL_CFG_DEFAULT_INC(1) |
              GLOBAL_CFG_CMP_INC(1);
    PIEP_CMP_STATUS = CMD_STATUS_CMP_HIT(1); /* clear the interrupt */
        PIEP_CMP_CMP1   = 0x0;
    PIEP_CMP_CFG |= CMP_CFG_CMP_EN(1);
        PIEP_GLOBAL_CFG |= GLOBAL_CFG_CNT_ENABLE;

     
     RBUFF->ring_tail = 20;
     while (1) {
        uint32_t v;
        while ((v=read_pin()) == last_pin_value) {
          // noop
        }
        uint32_t now = read_PIEP_COUNT()/200;
        uint32_t delta_time_us = TIME_SUB(now, last_time);
        // uint32_t delta_time_us = 654; // now - last_time_us;
        last_time = now;

        add_to_ring_buffer(last_pin_value, delta_time_us);
        last_pin_value = v;
     }

     /*
	for (c = 0; ; c++) {
		write_r30(c & 1 ? 0xffff : 0x0000);
		delay_us (period_us);
	}
    */

	return 0;
}
