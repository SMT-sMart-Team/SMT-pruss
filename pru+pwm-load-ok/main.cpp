#include "resource_table_1.h"
#define PRU1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>

#include "linux_types.h"
#include "pru_defs.h"
#include "prucomm.h"

#ifdef __GNUC__
#include <pru/io.h>
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
// reduce max PWM_OUT from 12 to 8
#define BYPASS_PWM_OUT_FOR_RCIN 

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


static const uint8_t chan_pru_map[MAX_PWMS]= {10,8,11,9,7,6,5,4,3,2,1,0};
unsigned int chPWM[MAX_PWMS][2]; // 0: period, 1: high
#define GAP 200 // us
#define UPDATE_CONFIGS() \
	do \
	{	\
		if(PWM_CMD->magic == PWM_CMD_MAGIC) \
		{	\
			PWM_CMD->magic = PWM_REPLY_MAGIC; \
            enmask = PWM_CMD->enmask; \
			for(ii = 0; ii < MAX_PWMS; ii++) \
			{ \
				chPWM[ii ][0] = PWM_CMD->periodhi[ii][0]; \
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
    u16 temp = 0;
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
        chPWM[index][0] = PRU_us(2500); // 400Hz 
        chPWM[index][1] = PRU_us(1100);
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

    // wait for host start 
    while(PWM_CMD_KEEP_ALIVE != PWM_CMD->keep_alive);
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

        }
#endif

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

        //step 2: judge current if it is arrive at rising edge time
#ifdef BYPASS_PWM_OUT_FOR_RCIN 
        // bypass last 4CH for multi-pwm: rcin used them
        for (index = 4; index < MAX_PWMS; index++)
#else
        for (index = 0; index < MAX_PWMS; index++)
#endif
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
                // update configs if have any
            	UPDATE_CONFIGS();

                // update time stamp
                //chnObj[index].time_of_lo.time_p2 = 0;
                //chnObj[index].time_of_lo.time_p1 = chPWM[index][1]; // PWM_CMD->periodhi[index][1]; //
                // falling_edge_time = current + high_len
                TIME_ADD(chnObj[index].time_of_lo, currTs64, chPWM[index][1]);
                //rising_edge_time = current + period
                TIME_ADD(chnObj[index].time_of_hi, currTs64, chPWM[index][0]);


            	if(enmask & (1U << index))
                {
                        temp = __R30;
    				    __R30 = temp | (1U<<index);

                }
                else // make sure low when disable
                {
                    temp = __R30;
    				__R30 = temp & (~(1u << index));
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
                        temp = __R30; 
    				    __R30 = temp & (~(1u << index));
                }

            }

        }
    }
}

