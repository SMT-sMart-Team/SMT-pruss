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

#ifndef __GNUC__
volatile register uint32_t __R31;
#endif


#define REVERT_BY_SW

#define CHECK_PERIOD_MS 500 // 500ms 
#define TIME_OUT_DEFAULT 20 // 20*500ms = 10s


static inline u32 read_PIEP_COUNT(void)
{
	return PIEP_COUNT;

}


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
    u32 currTime = 0;
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
        chPWM[index][0] = PRU_us(2500); // 50Hz
        chPWM[index][1] = PRU_us(1100);
        PWM_CMD->hilo_read[index][0] = PWM_CMD->hilo_read[index][1] = PWM_CMD->periodhi[index][0] = PWM_CMD->periodhi[index][1] = 0;
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
#ifdef __GNUC__
                        temp = __R30;
    				    temp |= (1U<<index);
                        __R30 = temp;
#else
    				    // __R30 |= (msk&(1U<<i));
                        __R30 |= (1U << index); //pull up
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
        			// __R30 &= ~(1U<<i);
                    __R30 &= ~(1U << index); //pull down
#endif
                }

            }

        }
    }
}

