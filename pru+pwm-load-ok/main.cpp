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


#define MOTOR_CH 8


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
    u32 chid;
    u32 enmask;
    time64 time_of_hi;
    time64 time_of_lo;
    time64 period_time;
} ChanelObj;

inline unsigned int time_greater(time64 x, time64 y)
{
    // reverse
    if(x.time_p2 > y.time_p2)
    {
        return 1;
    }
    else if(x.time_p2 < y.time_p2)
    {
        return 0;
    }
    else
    {
        if(x.time_p1 > y.time_p1)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

}

inline time64 time_add(time64 x, time64 y)
{

    time64 ret;
    u32 lowSum = x.time_p1 + y.time_p1;

    ret.time_p2 = x.time_p2 + y.time_p2;
    ret.time_p1 = lowSum;

    if((lowSum < x.time_p1) || lowSum < y.time_p1)
    {
        // reverse
        ret.time_p2 += 1;
    }

    return ret;

}

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


unsigned int chPWM[MAX_PWMS][2]; // 0: high, 1: low
inline void updateConfigs()
{
    unsigned char ii = 0;
    if(PWM_CMD->magic == PWM_CMD_MAGIC) 
    {
        PWM_CMD->magic = PWM_REPLY_MAGIC;
        for(ii = 0; ii < MAX_PWMS; ii++)
        {
            chPWM[ii][0] = PWM_CMD->periodhi[ii][0];
            chPWM[ii][1] = PWM_CMD->periodhi[ii][1];
        }
    }
    // while(1);
    // __R30 = BIT(10)|BIT(8);
}


static inline void initHW()
{
    // init
	/* enable OCP master port */
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

    __R30 = 0x0; // (~BIT(10))&(~BIT(8))&(~BIT(11))&(~BIT(9));
}


static inline void initSW()
{
}

int main(void) //(int argc, char *argv[])
{

    ChanelObj chnObj[MAX_PWMS];
    u32 temp = 0;
    u32 currTime = 0;
    time64 currTs64;
    u32 index = 0;

    initHW();


    //u32 sartRiseTime = 0;
    //u32 startFallTime = 0;
    for (index = 0; index < MAX_PWMS; index++)
    {
        chnObj[index].chid = index + 1;
        chnObj[index].enmask = 0;
        chnObj[index].time_of_hi.time_p2 = 0;
        chnObj[index].time_of_hi.time_p1 = 0;
        chnObj[index].time_of_lo.time_p2 = 0;
        chnObj[index].time_of_lo.time_p1 = 0;
        chnObj[index].period_time.time_p2 = 0;
        chnObj[index].period_time.time_p1 = 0;
        // default PWM
        chPWM[index][0] = PRU_us(2500);
        chPWM[index][1] = PRU_us(1100);
        PWM_CMD->periodhi[index][0] = PWM_CMD->periodhi[index][1] = 0; 
    }

    currTs64.time_p2 = 0;
    currTs64.time_p1 = 0;

    PWM_CMD->magic = 0xFFFFFFFF;
    PWM_CMD->enmask = 0x0;

    while (1)
    {

        //step 1 : update current time.
        currTime = read_PIEP_COUNT();
        // count overflow
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

        currTs64.time_p1 = currTime;



        //step 2: judge current if it is arrive at rising edge time
        for (index = 0; index < MAX_PWMS; index++)
        {
            chnObj[index].enmask = PWM_CMD->enmask & (1U << index);

            //it is time that arriving rising edge........
            if(time_greater(currTs64, chnObj[index].time_of_hi))
            {
                // update configs if have any
                updateConfigs();


                chnObj[index].period_time.time_p1 = chPWM[index][0]; // PWM_CMD->periodhi[index][0];


                // update time stamp
                chnObj[index].time_of_lo.time_p2 = 0;
                chnObj[index].time_of_lo.time_p1 = chPWM[index][1]; // PWM_CMD->periodhi[index][1]; // 
                chnObj[index].time_of_lo = time_add(chnObj[index].time_of_lo, currTs64);
                //rising_edge_time = current + period
                chnObj[index].time_of_hi = time_add(chnObj[index].period_time, currTs64);


                if (chnObj[index].enmask)
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
                continue;
            }

            //it is time that arriving falling edge........
            if(time_greater(currTs64, chnObj[index].time_of_lo))
            {

            	chnObj[index].time_of_lo = time_add(chnObj[index].period_time, currTs64);

                if (chnObj[index].enmask)
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

