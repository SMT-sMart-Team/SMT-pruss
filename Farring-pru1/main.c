/*
 * LED Bilnk Pruss code for tl_am335x
 * Boch <zbq@tronlong.com>
 * Copyright (C) 2017 Tronlong, Inc.
 *
 */

#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include "pru_defs.h"
#include "prucomm.h"

 /* LED port address */
#define GPIO2_BASE_ADDR             0x481AC000
#define GPIO_OE_OFFSET              0x134
#define GPIO_CLRDATAOUT_OFFSET		0x190
#define GPIO_SETDATAOUT_OFFSET      0x194

#define TEST_TL_LED 0

#define TEST_TL_PRU1 1

//volatile register uint32_t __R30;

volatile pruCfg CT_CFG __attribute__((cregister("PRU_CFG", near), peripheral));


#ifdef __GNUC__
#include <pru/io.h>
#endif

#ifndef __GNUC__
volatile register uint32_t __R30;
volatile register uint32_t __R31;
#endif

#define REVERT_BY_SW

#define CHECK_PERIOD_MS 50 // 50ms
#define TIME_OUT_DEFAULT 20 // 20*50ms = 1s


static inline u32 read_PIEP_COUNT(void)
{
	return PIEP_COUNT;

}


// AB ZhaoYJ for dynamically change aux-rcout to rcin(CH1~CH4)
uint32_t read_pin_ch(uint8_t chn_idx){
    return ((__R31&(1<<pwm_map[chn_idx])) != 0);
    // return ((read_r31()&(1<<15)) != 0);
}

#ifdef MULTI_PWM
#define DEBOUNCE_ENABLE
#define DEBOUNCE_TIME 200 // 1 us
#define PULSE_NUM_PER_PERIOD 1 // 1: every pulse will be update to ARM
#define MAX_US 21474836 // 2^32/200
#define TIME_SUB(x, y) ((x >= y)?(x - y):(0xFFFFFFFF - y + x))

#define WAITING 0
#define DEBOUNCING 1
#define CONFIRM 2
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
                // break;
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
                    RCIN->multi_pwm_out[chn_idx].high = delta_time_us_ch;
                }
                else
                {
                    RCIN->multi_pwm_out[chn_idx].low = delta_time_us_ch;
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

typedef struct
{
    u32 time_p1;
    u32 time_p2;
}time64;

typedef struct {
    time64 time_of_hi;
    time64 time_of_lo;
} ChanelObj;

#define TIME_GREATER(x, y) ((x.time_p2 == y.time_p2)?((x.time_p1 >= y.time_p1)? 1 : 0):((x.time_p2 > y.time_p2)? 1 : 0))

#define TIME_ADD(out, in, delta) \
	do \
	{ \
		out.time_p2 = in.time_p2; \
		out.time_p1 = in.time_p1 + delta; \
		if((out.time_p1 < delta) || out.time_p1 < in.time_p1) \
		{ \
			out.time_p2 += 1; \
		} \
	} while(0)

#if 0
// only handle x > y condition
inline u32 time_sub(time64 x, time64 y)
{

    time64 ret;

    if(time_greater(x, y))
    {
        if(x.time_p2 == y.time_p2)
        {
            ret.time_p2 = 0;
            ret.time_p1 = x.time_p1 - y.time_p1;

        }
        else
        {
            if(x.time_p1 > y.time_p1)
            {
                ret.time_p2 = x.time_p2 - y.time_p2;
                ret.time_p1 = x.time_p1 - y.time_p1;
            }
            else
            {
                ret.time_p2 = x.time_p2 - 1 - y.time_p2;
                ret.time_p1 = 0xFFFFFFFF - y.time_p1 + x.time_p1;
            }
        }
    }
    return ret.time_p1; // we believe that the reverse should be just one time

}
#endif


unsigned int chPWM[MAX_PWMS][2]; // 0: period, 1: high
#define GAP 200 // us
#define UPDATE_CONFIGS() \
	do \
	{	\
        /*make sure just magic_sync motor_out 8ch*/ \
        /*here is just AUX channel, so no need to magic_sync*/ \
		for(ii = 0; ii < 4; ii++) \
		{ \
			chPWM[ii][0] = PWM_CMD->periodhi[ii][0]; \
			chPWM[ii][1] = PWM_CMD->periodhi[ii][1]; \
			if(chPWM[ii][0] <= chPWM[ii][1]) /*error configs*/ \
			{ \
				chPWM[ii][1] = chPWM[ii][0] - GAP; \
			} \
		} \
		if(PWM_CMD->magic == PWM_CMD_MAGIC) \
		{	\
			PWM_CMD->magic = PWM_REPLY_MAGIC; \
            enmask = PWM_CMD->enmask; \
            /*make sure just magic_sync motor_out 8ch*/ \
			for(ii = 4; ii < MAX_PWMS; ii++) \
			{ \
				chPWM[ii][0] = PWM_CMD->periodhi[ii][0]; \
				chPWM[ii][1] = PWM_CMD->periodhi[ii][1]; \
				if(chPWM[ii][0] <= chPWM[ii][1]) /*error configs*/ \
				{ \
					chPWM[ii][1] = chPWM[ii][0] - GAP; \
				} \
			} \
		} \
	} while(0)



#define INIT_HW() \
	do \
	{ \
	/* enable OCP master port */ \
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0; \
	PRUCFG_SYSCFG &= ~SYSCFG_STANDBY_INIT; \
	PRUCFG_SYSCFG = (PRUCFG_SYSCFG & \
			~(SYSCFG_IDLE_MODE_M | SYSCFG_STANDBY_MODE_M)) | \
			SYSCFG_IDLE_MODE_NO | SYSCFG_STANDBY_MODE_NO; \
	/* our PRU wins arbitration */ \
	PRUCFG_SPP |=  SPP_PRU1_PAD_HP_EN; \
	/* configure timer */ \
	PIEP_GLOBAL_CFG = GLOBAL_CFG_DEFAULT_INC(1) | \
			  GLOBAL_CFG_CMP_INC(1); \
	PIEP_CMP_STATUS = CMD_STATUS_CMP_HIT(1); /* clear the interrupt */ \
    PIEP_CMP_CMP1   = 0x0; \
	PIEP_CMP_CFG |= CMP_CFG_CMP_EN(1); \
    PIEP_GLOBAL_CFG |= GLOBAL_CFG_CNT_ENABLE; \
    __R30 = 0x0; /* (~BIT(10))&(~BIT(8))&(~BIT(11))&(~BIT(9)); */ \
	} while(0)



int main(void) //(int argc, char *argv[])
{

    ChanelObj chnObj[MAX_PWMS];
    u16 time_out = 0;
    u32 prevTime = 0;
    time64 currTs64;
    u8 index = 0;
    u8 ii = 0;
    u16 wbPeriod = 0;
    u16 enmask = 0;
    time64 keepAliveTs64;

    INIT_HW();


    //u32 sartRiseTime = 0;
    //u32 startFallTime = 0;
    for (index = 0; index < MAX_PWMS; index++)
    {
        chnObj[index].time_of_hi.time_p2 = 0;
        chnObj[index].time_of_hi.time_p1 = 0;
        chnObj[index].time_of_lo.time_p2 = 0;
        chnObj[index].time_of_lo.time_p1 = 0;
        // default PWM
        chPWM[index][0] = PRU_us(2500000); // 400Hz
        chPWM[index][1] = PRU_us(1100000);
        // chPWM[index][0] = PRU_us(2500); // 400Hz
        // chPWM[index][1] = PRU_us(1100);
        PWM_CMD->hilo_read[index][0] = 0;
        PWM_CMD->hilo_read[index][1] = 0;
        PWM_CMD->periodhi[index][0] = 0;
        PWM_CMD->periodhi[index][1] = 0;
    }

    currTs64.time_p2 = 0;
    currTs64.time_p1 = 0;

    keepAliveTs64.time_p2 = 0;
    keepAliveTs64.time_p1 = 0;

    PWM_CMD->magic = 0xFFFFFFFF;
    PWM_CMD->enmask = 0x0;
    PWM_CMD->keep_alive = 0xFFFF;
    PWM_CMD->time_out = TIME_OUT_DEFAULT; // default should be 10 second

#if TEST_TL_LED
	uint32_t i;
    uint32_t value;
    /* GPI Mode 0, GPO Mode 0 */
    CT_CFG.GPCFG0 = 0;
    /* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    uint32_t led_set_addr;
	uint32_t led_clr_addr;
//	uint32_t led_oe_addr;

	led_set_addr = GPIO2_BASE_ADDR + GPIO_SETDATAOUT_OFFSET;
	led_clr_addr = GPIO2_BASE_ADDR + GPIO_CLRDATAOUT_OFFSET;
//	led_oe_addr = GPIO2_BASE_ADDR + GPIO_OE_OFFSET;

	*(uint32_t*)led_set_addr |= 1 << 22;	//turn down LED0 (GPIO2_22)
//	*(uint32_t*)led_oe_addr |= 0xfe3fffff;	//gpio2_22~24 pin output enable

	while(1) {

			for (i = 0; i < 3; i++) {
				value = 1 << (i + 22);
				*(uint32_t*)led_set_addr |= value;
				__delay_cycles(100000000); // half-second delay
				*(uint32_t*)led_clr_addr |= value;
				__delay_cycles(100000000); // half-second delay
			}
	}
#endif

#if TEST_TL_PRU1
        // CT_CFG.GPCFG0 = 0;
        /* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
        // CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

        uint32_t led_set_addr;
    	uint32_t led_clr_addr;
        uint32_t value;
        value = 1 << (1 + 22);
    	led_set_addr = GPIO2_BASE_ADDR + GPIO_SETDATAOUT_OFFSET;
    	led_clr_addr = GPIO2_BASE_ADDR + GPIO_CLRDATAOUT_OFFSET;
#else
    // wait for host start
    while(PWM_CMD_KEEP_ALIVE != PWM_CMD->keep_alive);
#endif
    PWM_CMD->keep_alive = PWM_REPLY_KEEP_ALIVE;

    // record time stamp
    prevTime = currTs64.time_p1 = read_PIEP_COUNT();

    // update next keep alive ts
    TIME_ADD(keepAliveTs64, currTs64, PRU_ms(CHECK_PERIOD_MS));

    while (1)
    {

        // step 0: update rcin: CH1~CH4
        decode_multi_pwms();

        //step 1 : update current time.
        currTs64.time_p1 = read_PIEP_COUNT();
        wbPeriod++;
        // count overflow
#ifdef REVERT_BY_SW
        if(prevTime > currTs64.time_p1)
        {
            // reverse
            currTs64.time_p2++;

        }
        prevTime = currTs64.time_p1;
#else
        if(PIEP_GLOBAL_STATUS & GLOBAL_STATUS_CNT_OVF)
        {
            // clear
            PIEP_GLOBAL_STATUS |= GLOBAL_STATUS_CNT_OVF;

            // reverse
            currTs64.time_p2++;

#if 0
                        temp = __R30;
    				    temp |= (1U<<index);
                        if(temp & BIT(8))
                        {
    				        temp &= ~BIT(8);
                        }
                        else
                        {
    				        temp |= BIT(8);
                        }
                        __R30 = temp;
#endif
        }
#endif

#if TEST_TL_PRU1

#else
        // make sure host is alive when time is up to check
        if(TIME_GREATER(currTs64, keepAliveTs64))
        {
            // update next keep alive ts
            TIME_ADD(keepAliveTs64, currTs64, PRU_ms(CHECK_PERIOD_MS));

            // alive
            if(PWM_CMD_KEEP_ALIVE == PWM_CMD->keep_alive)
            {
                PWM_CMD->keep_alive = PWM_REPLY_KEEP_ALIVE;
                time_out = 0;
            }
            else
            {
                time_out++;
                if(time_out > PWM_CMD->time_out)
                {
                    // PWM_CMD->hilo_read[11][1] = PRU_us(777); // for debug
                    // host is dead, so sth very bad has happened, make sure no PWM out when this time and exit pru
                    __R30 = 0;
                    break;
                }
            }
        }
#endif

        //step 2: judge current if it is arrive at rising edge time
        for (index = 0; index < MAX_PWMS; index++)
        {
            // write back to HOST
            if(wbPeriod > 10000)
            {
                if(index == (MAX_PWMS - 1))
                {
                    wbPeriod = 0;
                }
				PWM_CMD->hilo_read[index][1] = PWM_CMD->periodhi[index][1];
            }
            if(TIME_GREATER(currTs64, chnObj[index].time_of_hi))
            {
#if TEST_TL_PRU1
            	enmask = 0xFFFF;
#else
                // update configs if have any
            	UPDATE_CONFIGS();
#endif

                // update time stamp
                //chnObj[index].time_of_lo.time_p2 = 0;
                //chnObj[index].time_of_lo.time_p1 = chPWM[index][1]; // PWM_CMD->periodhi[index][1]; //
                // falling_edge_time = current + high_len
                TIME_ADD(chnObj[index].time_of_lo, currTs64, chPWM[index][1]);
                //rising_edge_time = current + period
                TIME_ADD(chnObj[index].time_of_hi, currTs64, chPWM[index][0]);


            	if(enmask & (1U << index))
                {
#ifdef __GNUC__
                        temp = __R30;
    				    temp |= (1U<<index);
                        __R30 = temp;
#else

#if TEST_TL_PRU1
                        __R30 = 0xFFFFFFFF;
                        *(uint32_t*)led_set_addr |= value;
#else
    				    // __R30 |= (msk&(1U<<i));
                        __R30 |= (1U << index); //pull up
#endif

#endif
                }
                else // make sure low when disable
                {
#ifdef __GNUC__
                    temp = __R30;
    				temp &= ~(1u << index);
                    __R30 = temp;
#else
        			// __R30 &= ~(1U<<i);
                    __R30 &= ~(1U << index); //pull down
#endif
                }
                continue;
            }
            //it is time that arriving falling edge........
            if(TIME_GREATER(currTs64, chnObj[index].time_of_lo))
            {

            	//chnObj[index].time_of_lo = time_add(chPWM[index][0], currTs64);
            	TIME_ADD(chnObj[index].time_of_lo, currTs64, chPWM[index][0]);

            	if(enmask & (1U << index))
                {
#ifdef __GNUC__
                        temp = __R30;
    				    temp &= ~(1u << index);
                        __R30 = temp;
#else

#if TEST_TL_PRU1
                        __R30 = 0x0;
                        *(uint32_t*)led_clr_addr |= value;
#else
                        // __R30 &= ~(1U<<i);
                        __R30 &= ~(1U << index); //pull down
#endif

#endif
                }

            }

        }
    }
}


