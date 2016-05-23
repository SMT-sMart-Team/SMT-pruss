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

// #define FAKE_PPM
#ifdef FAKE_PPM
uint32_t fake_deltaT[18] = { /*CH1, CH2, CH3, CH4, CH5, CH6, CH7, CH8, END*/
                            350, 351, 450, 451, 400, 401, 500, 501, 
                            600, 601, 700, 701, 800, 801, 900, 901, 3310, 3310  
                        };
#endif

#define DEBOUNCE_ENABLE
#define DEBOUNCE_TIME 1 // us

#ifdef PPMSUM_DECODE
// #define TEST_OUT
struct {
    uint8_t _channel_counter;
    uint16_t _pulse_capt[MAX_RCIN_NUM];
} ppm_state;

#ifdef TEST_OUT
uint16_t fake_deltaT[9] = { /*CH1, CH2, CH3, CH4, CH5, CH6, CH7, CH8, END*/
                            400, 410, 420, 430, 440, 450, 460, 470, 3310  
                        };
#endif
#endif

#ifdef __GNUC__
#include <pru/io.h>
#endif

#ifndef __GNUC__
volatile register uint32_t __R31;
#endif

#define TIME_SUB(x, y) ((x >= y)?(x - y):(0xFFFFFFFF - y + x))

#if defined(FAKE_PPM) || defined(TEST_OUT)
static void delay_us(unsigned int us)
{
	/* assume cpu frequency is 200MHz */
	__delay_cycles (us * (1000 / 5));
}
#endif

#ifndef DEBOUNCE_ENABLE
void add_to_ring_buffer(uint8_t v, uint16_t deltat)
{
    RBUFF->buffer[tail_local].pin_value = v;
    RBUFF->buffer[tail_local].delta_t = deltat;
    tail_local = (tail_local + 1) % NUM_RING_ENTRIES;
    // RBUFF->buffer[RBUFF->ring_tail].pin_value = v;
    // RBUFF->buffer[RBUFF->ring_tail].delta_t = deltat;
    // update to Host
    RBUFF->ring_tail = tail_local;
}
#endif

static inline u32 read_PIEP_COUNT(void)
{
    return PIEP_COUNT;
}

uint32_t read_pin(void){
    return ((__R31&(1<<15)) != 0);
    // return ((read_r31()&(1<<15)) != 0);
}


#ifdef PPMSUM_DECODE
void process_ppmsum_pulse(uint16_t width_usec)
{
    if(width_usec >= 2700) {
        // a long pulse indicates the end of a frame. Reset the
        // channel counter so next pulse is channel 0
        if ((ppm_state._channel_counter != 0xFF) && (ppm_state._channel_counter >= 5)) {
            for (uint8_t i=0; i<ppm_state._channel_counter; i++) {
                RBUFF->ppm_decode_out.rcin_value[i] = ppm_state._pulse_capt[i];
            }
            RBUFF->ppm_decode_out._num_channels = ppm_state._channel_counter;
            RBUFF->ppm_decode_out.new_rc_input = OK;
        }
        ppm_state._channel_counter = 0;
        return;
    }
    if(ppm_state._channel_counter == 0xFF) {
        // we are not synchronised
        return;
    }

    /*
      we limit inputs to between 700usec and 2300usec. This allows us
      to decode SBUS on the same pin, as SBUS will have a maximum
      pulse width of 100usec
     */
    if (width_usec > 700 && width_usec < 2300) {
        // take a reading for the current channel
        // buffer these
        ppm_state._pulse_capt[ppm_state._channel_counter] = width_usec;

        // move to next channel
        ppm_state._channel_counter++;
    }

    // if we have reached the maximum supported channels then
    // mark as unsynchronised, so we wait for a wide pulse
    if ((ppm_state._channel_counter != 0xFF) && (ppm_state._channel_counter >= MAX_RCIN_NUM)) {
        for (uint8_t i=0; i<ppm_state._channel_counter; i++) {
            RBUFF->ppm_decode_out.rcin_value[i] = ppm_state._pulse_capt[i];
        }
        RBUFF->ppm_decode_out._num_channels = ppm_state._channel_counter;
        RBUFF->ppm_decode_out.new_rc_input = OK;
        ppm_state._channel_counter = 0xFF;
    }
}
#endif

// const unsigned int period_us = 250 * 1000;

int main(void)
{
     uint32_t last_time = 0;
     uint32_t last_pin_value = 0x0;
     uint16_t tail_local = 0x0;
     uint16_t ii = 0;
#ifdef PPMSUM_DECODE
     ppm_state._channel_counter = 0xFF;
     uint16_t _s0_time = 0;
#endif

#ifdef DEBOUNCE_ENABLE

#define WAITING 0 
#define DEBOUNCING 1 
#define CONFIRM 2 

     uint32_t v = 0; 
     uint8_t state = WAITING;
     uint32_t toggle_time = 0;
     uint32_t delta_time_us = 0;
     uint8_t pass = 0;
     uint8_t first = 1;
#ifdef TEST_OUT
     uint8_t id = 0;
     v = 1; // for test
#endif
#endif

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

     
     RBUFF->ring_tail = 0x0;
     RBUFF->ring_head = 0x0;
     for(ii = 0; ii < NUM_RING_ENTRIES; ii++) 
     {
         RBUFF->buffer[ii].pin_value = 0;
         RBUFF->buffer[ii].delta_t = 0;
     }
     
#ifdef PPMSUM_DECODE
     RBUFF->ppm_decode_out.new_rc_input = KO;
     RBUFF->ppm_decode_out._num_channels = 0;
     ppm_state._channel_counter = 0xFF;
     for(ii = 0; ii < MAX_RCIN_NUM; ii++) 
     {
         RBUFF->ppm_decode_out.rcin_value[ii] = 0;
         ppm_state._pulse_capt[ii] = 0;
     }
#endif

#ifdef FAKE_PPM
     uint8_t fake_idx = 0;
     uint8_t idx = 0;
     uint16_t dt = 0;
#endif
     while (1) {
#ifdef FAKE_PPM
         delay_us(20*1000);
         uint8_t ii = 0;
         for(ii = 0; ii < 9; ii++)
         {
             idx = ii*2+fake_idx;
             dt = fake_deltaT[idx];
             add_to_ring_buffer(1, dt);
             add_to_ring_buffer(0, dt);
         }

         fake_idx++;
        fake_idx %= 2;



#elif defined(DEBOUNCE_ENABLE)
#ifdef  TEST_OUT
        state = CONFIRM;
#endif

        switch(state)
        {
            case WAITING:
                while ((v=read_pin()) == last_pin_value) {
                }
                toggle_time = read_PIEP_COUNT()/200;
                state = DEBOUNCING;
            case DEBOUNCING:
                if(read_pin() == v) 
                {
                    if(DEBOUNCE_TIME <= TIME_SUB(read_PIEP_COUNT()/200, toggle_time))
                    {
                        // debounce done
                        state = CONFIRM;
                    }
                }
                else
                {
                    // invalid pulse
                    state = WAITING;
                }
                break;
            case CONFIRM:
                if(!first)
                {
                    delta_time_us = TIME_SUB(toggle_time, last_time);
                }
                else
                {
                    delta_time_us  = 0;
                    first  = 0;
                }
                // uint32_t delta_time_us = 654; // now - last_time_us;
                last_time = toggle_time;
                // add_to_ring_buffer(last_pin_value, delta_time_us);
                //
                RBUFF->buffer[tail_local].pin_value = last_pin_value;
                RBUFF->buffer[tail_local].delta_t = delta_time_us;
                tail_local = (tail_local + 1) % NUM_RING_ENTRIES;
                // RBUFF->buffer[RBUFF->ring_tail].pin_value = v;
                // RBUFF->buffer[RBUFF->ring_tail].delta_t = deltat;
                // update to Host
                pass++;
                if(!(pass%16))
                {
                    pass = 0;
                    RBUFF->ring_tail = tail_local;
                }
#ifdef PPMSUM_DECODE

#ifdef TEST_OUT
                delay_us(20*1000);
                v = v?0:1;
                delta_time_us = fake_deltaT[id/2];
                // delta_time_us = delta_time_us*2;
                id = id+1;
                if(id == 18)
                {
                    id = 0;
                }
#endif

                if (last_pin_value == 1) {
                    // remember the time we spent in the low state
                    _s0_time = delta_time_us;
                } else {
                    // the pulse value is the sum of the time spent in the low
                    // and high states
                    _s0_time = _s0_time + delta_time_us;
                    process_ppmsum_pulse(_s0_time);
                }
#endif
                //
                //
                last_pin_value = v;
                // back to wait next toggle
                state = WAITING;
                break;
        }

#else
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
#endif
     }

	return 0;
}
