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


#define DEBOUNCE_ENABLE
#define DEBOUNCE_TIME 200 // 1 us
#define PULSE_NUM_PER_PERIOD 1 // 1: every pulse will be update to ARM

#define WAITING 0 
#define DEBOUNCING 1 
#define CONFIRM 2 


#ifdef __GNUC__
#include <pru/io.h>
#endif

#ifndef __GNUC__
volatile register uint32_t __R31;
#endif

#define MAX_US 21474836 // 2^32/200 
#define TIME_SUB(x, y) ((x >= y)?(x - y):(0xFFFFFFFF - y + x))


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

uint32_t read_pin_ch(uint8_t chn_idx){
    return ((__R31&(1<<15)) != 0);
    // return ((read_r31()&(1<<15)) != 0);
}

#ifdef MULTI_PWM
    // treat all pins as pwm input
uint8_t state_ch[MAX_RCIN_NUM];
uint8_t last_pin_value_ch[MAX_RCIN_NUM];
uint8_t first_ch[MAX_RCIN_NUM];
uint32_t toggle_time_ch[MAX_RCIN_NUM];
uint32_t last_time_ch[MAX_RCIN_NUM];
void decode_multi_pwms()
{
    uint8_t chn_idx = 0;
    uint32_t v = 0;
    uint32_t delta_time_us_ch = 0;


    for(chn_idx  = 0; chn_idx < MAX_RCIN_NUM; chn_idx++)
    {
        switch(state_ch[chn_idx])
        {
            case WAITING:
                if((v=read_pin_ch(chn_idx)) == last_pin_value_ch[chn_idx]) {
                    continue; // next channel
                }
                // toggle_time = read_PIEP_COUNT()/200;
                toggle_time_ch[chn_idx] = read_PIEP_COUNT();
                state_ch[chn_idx] = DEBOUNCING;
                break;
            case DEBOUNCING:
                if((v=read_pin_ch(chn_idx)) != last_pin_value_ch[chn_idx]) {
                    if(DEBOUNCE_TIME <= TIME_SUB(read_PIEP_COUNT(), toggle_time_ch[chn_idx]))
                    {
                        // debounce done
                        state_ch[chn_idx] = CONFIRM;
                    }
                }
                else
                {
                    // invalid pulse
                    state_ch[chn_idx] = WAITING;
                }
                break;
            case CONFIRM:
                if(!first_ch[chn_idx])
                {
                    delta_time_us_ch = TIME_SUB(toggle_time_ch[chn_idx], last_time_ch[chn_idx])/200;
                }
                else
                {
                    delta_time_us_ch  = 0;
                    first_ch[chn_idx] = 0;
                }
                // uint32_t delta_time_us = 654; // now - last_time_us;
                last_time_ch[chn_idx] = toggle_time_ch[chn_idx];
                // add_to_ring_buffer(last_pin_value, delta_time_us);
                // 
                //
                if(last_pin_value_ch[chn_idx])
                {
                    RBUFF->multi_pwm_out[chn_idx].high = delta_time_us_ch;
                }
                else
                {
                    RBUFF->multi_pwm_out[chn_idx].low = delta_time_us_ch;
                }
                //
                v=read_pin_ch(chn_idx);
                last_pin_value_ch[chn_idx] = v;
                // back to wait next toggle
                state_ch[chn_idx] = WAITING;
                break;
        }
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

#ifdef DEBOUNCE_ENABLE


     uint32_t v = 0; 
     uint8_t state = WAITING;
     uint32_t toggle_time = 0;
     uint32_t delta_time_us = 0;
     uint8_t pass = 0;
     uint8_t first = 1;
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

#ifdef MULTI_PWM
    // treat all pins as pwm input
    for(ii = 0; ii < MAX_RCIN_NUM; ii++) 
    {
        state_ch[ii] = WAITING;
        last_pin_value_ch[ii] = 0;
        first_ch[ii] = 1;
        toggle_time_ch[ii] = 0;
    }
#endif
     

     while (1) {

#if defined(DEBOUNCE_ENABLE)

        switch(state)
        {
            case WAITING:
                while ((v=read_pin()) == last_pin_value) {
                }
                // toggle_time = read_PIEP_COUNT()/200;
                toggle_time = read_PIEP_COUNT();
                // state = DEBOUNCING;
                // state = CONFIRM;
                // break;
            case DEBOUNCING:
                if(read_pin() == v) 
                {
                    if(DEBOUNCE_TIME <= TIME_SUB(read_PIEP_COUNT(), toggle_time))
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
                    delta_time_us = TIME_SUB(toggle_time, last_time)/200;
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
                if(!(pass%PULSE_NUM_PER_PERIOD))
                {
                    pass = 0;
                    RBUFF->ring_tail = tail_local;
                }
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
#ifdef MULTI_PWM
        // treat all pins as pwm input
        decode_multi_pwms();
#endif
     }

	return 0;
}
