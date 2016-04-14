/*
 * testpru
 *
 */

#define PRU1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "linux_types.h"
#include "pru_defs.h"
#include "prucomm.h"

#define SMT_PRU_PWM

#define MOTOR_CH_NUM 8
#define MOTOR_CH_MASK 0xFF

unsigned char ch_num_period = 0;
struct pwm_cmd_l cfg;

static void pwm_setup(void)
{
	u8 i;

	cfg.enmask = 0;
	for (i = 0; i < MAX_PWMS; i++)
		cfg.hilo[i][0] = cfg.hilo[i][1] = PRU_us(200);
}
unsigned int PIEP_COUNT = 100;
#define PIEP_COUNT_INTERVAL	100
static inline u32 read_PIEP_COUNT(void)
{
	unsigned int cyc_tmp = PIEP_COUNT;
	PIEP_COUNT += PIEP_COUNT_INTERVAL;
	return cyc_tmp;
}

enum control_flag {
	Init = 0,
	Min_out,
	En_all,
	Period_change1,
	Period_change2,
	Disable_ch1,
	Enable_ch1,
	Disable_all
};

enum bool_type {
	b_true,
	b_false
};

enum control_flag flag = Init;
enum bool_type full_period = b_true;
#define TIME_INTV 10000
unsigned int time_flow_ctr[8] = {2, 4, 8, 60, 108, 2110, 2620, 3230};
unsigned char time_flow_idx = 0;

#define SEED 0xbeef
unsigned int rand_period_change()
{

	unsigned int ret = 0;
	static unsigned int seed = SEED;
	srand(((unsigned)time(NULL)*SEED) - SEED*SEED);
	seed += rand()%0xfead;
	srand(((unsigned)time(NULL)*seed) - seed*seed);
	ret = rand()%1000;
	return ret;

}


void update_flag()
{
	static unsigned int ctr = 0;
	unsigned int ii = 0;
	unsigned int incr = 60;
	unsigned char first_prt = 1;

	//printf("ctr: %d\n", ctr);
	//printf("tf_ctr_idx: %d\n", time_flow_idx);
	//printf("time_flow_ctr[%d]: %d\n", time_flow_idx, time_flow_ctr[time_flow_idx++]);

	// ctl flag via time flow

	for(ii = 0; ii < Disable_all; ii++)
	{
		if(ctr == time_flow_ctr[ii])
		{
			flag++;
			flag %= (Disable_all+1);
			//flag = Min_out;
			first_prt = 1;
			break;
		}

	}

#if 0
	if(5 == ii)
	{
		ctr++;
		return;
	}
#endif

	switch(flag)
	{
	case Init:
		// nothing to do
		if(first_prt)
		{
			first_prt = 0;
			printf("=========================================================\n");
			printf("Enter Init state~!\n");
		}
		break;
	case Min_out:
		PWM_CMD->enmask = 0xFF;

		for(ii = 0; ii < MAX_PWMS; ii++)
		{
			incr += ii*incr;
			PWM_CMD->periodhi[ii][0] = PRU_us(2500);
			PWM_CMD->periodhi[ii][1] = PRU_us(1100);

		}
		PWM_CMD->magic = PWM_CMD_MAGIC;
		if(first_prt)
		{
			first_prt = 0;
			printf("=========================================================\n");
			printf("Enter Min_out state~!\n");
		}
		break;
	case En_all:
		PWM_CMD->enmask = 0xFF;
		if(first_prt)
		{
			first_prt = 0;
			printf("=========================================================\n");
			printf("Enter En_all state~!\n");
		}
		PWM_CMD->magic = PWM_CMD_MAGIC;
		break;
	case Period_change1:
		PWM_CMD->enmask = 0x0F;
		if(first_prt)
		{
			first_prt = 0;
			printf("=========================================================\n");
			printf("Enter Period_change[111] state~!\n");
		}
		for(ii = 0; ii < MOTOR_CH_NUM; ii++)
		{
			//incr += ii*incr;
			incr = rand_period_change();
			PWM_CMD->periodhi[ii][0] = PRU_us(2500);
			PWM_CMD->periodhi[ii][1] = PRU_us((1500 + incr)%2500);
			//printf("Period_change[111]: rand_period_change: [[%08x]]\n", incr);
			//printf("CH[%d]: hi: %08x lo: %08x \n", ii, PWM_CMD->periodhi[ii][1], PWM_CMD->periodhi[ii][0] - PWM_CMD->periodhi[ii][1]);

		}
		PWM_CMD->magic = PWM_CMD_MAGIC;

		break;
	case Period_change2:
		PWM_CMD->enmask = 0xFF;
		incr = 77;
		if(first_prt)
		{
			first_prt = 0;
			printf("=========================================================\n");
			printf("Enter Period_change[222] state~!\n");
		}
		for(ii = 0; ii < MOTOR_CH_NUM; ii++)
		{
			incr = rand_period_change();
			PWM_CMD->periodhi[ii][0] = PRU_us(2500);
			PWM_CMD->periodhi[ii][1] = PRU_us((1500 - incr)%2500);
			//printf("Period_change[222]: rand_period_change: [[%08x]]\n", incr);
			//printf("CH[%d]: hi: %08x lo: %08x \n", ii, PWM_CMD->periodhi[ii][1], PWM_CMD->periodhi[ii][0] - PWM_CMD->periodhi[ii][1]);
		}
		PWM_CMD->magic = PWM_CMD_MAGIC;
		break;
	case Disable_ch1:
		PWM_CMD->enmask = 0x2;
		if(first_prt)
		{
			first_prt = 0;
			printf("=========================================================\n");
			printf("Enter Disable_ch1[111] state~!\n");
		}
		for(ii = 0; ii < MOTOR_CH_NUM; ii++)
		{
			incr = rand_period_change();
			PWM_CMD->periodhi[ii][0] = PRU_us(2500);
			PWM_CMD->periodhi[ii][1] = PRU_us((1500 - incr)%2500);
			//printf("Period_change[222]: rand_period_change: [[%08x]]\n", incr);
			//printf("CH[%d]: hi: %08x lo: %08x \n", ii, PWM_CMD->periodhi[ii][1], PWM_CMD->periodhi[ii][0] - PWM_CMD->periodhi[ii][1]);
		}
		PWM_CMD->magic = PWM_CMD_MAGIC;
		break;
	case Enable_ch1:
		PWM_CMD->enmask = 0xF;
		if(first_prt)
		{
			first_prt = 0;
			printf("=========================================================\n");
			printf("Enter Enable_ch1[111] state~!\n");
		}
		for(ii = 0; ii < MOTOR_CH_NUM; ii++)
		{
			incr = rand_period_change();
			PWM_CMD->periodhi[ii][0] = PRU_us(2500);
			PWM_CMD->periodhi[ii][1] = PRU_us((1500 - incr)%2500);
			//printf("Period_change[222]: rand_period_change: [[%08x]]\n", incr);
			//printf("CH[%d]: hi: %08x lo: %08x \n", ii, PWM_CMD->periodhi[ii][1], PWM_CMD->periodhi[ii][0] - PWM_CMD->periodhi[ii][1]);
		}
		PWM_CMD->magic = PWM_CMD_MAGIC;
		break;
	case Disable_all:
		PWM_CMD->enmask = 0x0;
		break;
	default:
		printf("Hi, you just got the wrong flag!\n");
		break;
	}
	ctr++;
}
#ifndef SMT_PRU_PWM
int main(void) //(int argc, char *argv[])
{
	printf("Bingo, enter 1st simulator DSP APP~!\n");
       	u8 i, _i;
	u32 cnt, next;
	u32 msk, setmsk, clrmsk;
	u32 delta, deltamin, tnext, hi, lo;
	u32 *nextp;
	const u32 *hilop;
    u32 period;
	u32 enmask;	/* enable mask */
	u32 stmask;	/* state mask */
	static u32 next_hi_lo[MAX_PWMS][3];
	static struct cxt cxt;
#if 0
	/* enable OCP master port */
	PRUCFG_SYSCFG &= ~SYSCFG_STANDBY_INIT;
	PRUCFG_SYSCFG = (PRUCFG_SYSCFG &
			~(SYSCFG_IDLE_MODE_M | SYSCFG_STANDBY_MODE_M)) |
			SYSCFG_IDLE_MODE_NO | SYSCFG_STANDBY_MODE_NO;

	/* our PRU wins arbitration */
	PRUCFG_SPP |=  SPP_PRU1_PAD_HP_EN;
	pwm_setup();

	/* configure timer */
	PIEP_GLOBAL_CFG = GLOBAL_CFG_DEFAULT_INC(1) |
			  GLOBAL_CFG_CMP_INC(1);
	PIEP_CMP_STATUS = CMD_STATUS_CMP_HIT(1); /* clear the interrupt */
        PIEP_CMP_CMP1   = 0x0;
	PIEP_CMP_CFG |= CMP_CFG_CMP_EN(1);
        PIEP_GLOBAL_CFG |= GLOBAL_CFG_CNT_ENABLE;
#endif

	/* initialize */
	cnt = read_PIEP_COUNT();

	enmask = cfg.enmask;
	stmask = 0;		/* starting all low */

	clrmsk = 0;
	for (i = 0, msk = 1, nextp = &next_hi_lo[0][0], hilop = &cfg.hilo[0][0];
			i < MAX_PWMS;
			i++, msk <<= 1, nextp += 3, hilop += 2) {
		if ((enmask & msk) == 0) {
			nextp[1] = PRU_us(100);	/* default */
			nextp[2] = PRU_us(100);
			continue;
		}
		nextp[0] = cnt;		/* next */
		nextp[1] = 200000;	/* hi */
        nextp[2] = 208000;	/* lo */
        PWM_CMD->periodhi[i][0] = 408000;
        PWM_CMD->periodhi[i][1] = 180000;        
	}
    PWM_CMD->enmask = 0;
	clrmsk = enmask;
	setmsk = 0;
	/* guaranteed to be immediate */
	deltamin = 0;
	next = cnt + deltamin;
    PWM_CMD->magic = PWM_REPLY_MAGIC;
    
	while(1) {

		update_flag();

        if(PWM_CMD->magic == PWM_CMD_MAGIC)
        {
			msk = PWM_CMD->enmask;
            for(i=0, nextp = &next_hi_lo[0][0]; i<MAX_PWMS; 
                i++, nextp += 3){
                //Enable
                if ((PWM_EN_MASK & (msk&(1U<<i))) && (enmask & (msk&(1U<<i))) == 0) {
        		        enmask |= (msk&(1U<<i));

    				    __R30 |= (msk&(1U<<i));
    				    printf("Enable: R30 --- %08x\n", __R30);


                        // first enable
                        if (enmask == (msk&(1U<<i)))
            			    cnt = read_PIEP_COUNT();

                        nextp[0] = cnt;	//since we start high, wait this amount

                        deltamin = 0;
                        next = cnt;
                }
                //Disable
        		if ((PWM_EN_MASK & (msk&(1U<<i))) && ((msk & ~(1U<<i)) == 0)) {
        			enmask &= ~(1U<<i);
        			__R30 &= ~(1U<<i);
        		}
                
                //get and set pwm_vals
                if (PWM_EN_MASK & (msk&(1U<<i))) {
                	if(b_true == full_period)
                	{

            			//nextp = &next_hi_lo[i * 3];
                		nextp[1] = PWM_CMD->periodhi[i][1];
                        period = PWM_CMD->periodhi[i][0]; 
                		nextp[2] =period - nextp[1];

                	}
                }
                PWM_CMD->hilo_read[i][0] = nextp[0];
                PWM_CMD->hilo_read[i][1] = nextp[1];
                
                
            }
                    
			// guaranteed to be immediate 
			deltamin = 0;
        
            PWM_CMD->magic = PWM_REPLY_MAGIC;
		}
        PWM_CMD->enmask_read = enmask;
		/* if nothing is enabled just skip it all */
		if (enmask == 0)
			continue;

		setmsk = 0;
		clrmsk = (u32)-1;
		deltamin = PRU_ms(100); /* (1U << 31) - 1; */
		next = cnt + deltamin;


		for(_i = 0; _i< MAX_PWMS; _i++)
		{
			if (enmask & (1U << (_i))) { //
				nextp = &next_hi_lo[(_i)][0]; //
				tnext = nextp[0]; //
				hi = nextp[1]; //
				lo = nextp[2]; //
				/* avoid signed arithmetic */ //
				while (((delta = (tnext - cnt)) & (1U << 31)) != 0) { //
					/* toggle the state */ //
					if (stmask & (1U << (_i))) { //
						stmask &= ~(1U << (_i)); //
						clrmsk &= ~(1U << (_i)); //
						tnext += lo; //

						printf("CH %d: next hi: %08x\n", _i, tnext);
						// flag when all motor channel finish full period
						ch_num_period++;
						if(!((ch_num_period) %= MOTOR_CH_NUM))
						{
							full_period = b_true;
							printf("----------------full period--------------\n");
						}
					} else { //
						stmask |= (1U << (_i)); //
						setmsk |= (1U << (_i)); //
						tnext += hi; //
						full_period = b_false;

						//if(flag == Period_change)

					} //
				} //
				if (delta <= deltamin) { //
					deltamin = delta; //
					next = tnext; //
				} //
				nextp[0] = tnext; //
			} //
		}
#if 0
#define SINGLE_PWM(_i) \
	do { \
		if (enmask & (1U << (_i))) { \
			nextp = &next_hi_lo[(_i)][0]; \
			tnext = nextp[0]; \
			hi = nextp[1]; \
			lo = nextp[2]; \
			/* avoid signed arithmetic */ \
			while (((delta = (tnext - cnt)) & (1U << 31)) != 0) { \
				/* toggle the state */ \
				if (stmask & (1U << (_i))) { \
					stmask &= ~(1U << (_i)); \
					clrmsk &= ~(1U << (_i)); \
					tnext += lo; \
				} else { \
					stmask |= (1U << (_i)); \
					setmsk |= (1U << (_i)); \
					tnext += hi; \
				} \
			} \
			if (delta <= deltamin) { \
				deltamin = delta; \
				next = tnext; \
			} \
			nextp[0] = tnext; \
		} \
	} while (0)



#if MAX_PWMS > 0 && (PWM_EN_MASK & BIT(0))
		SINGLE_PWM(0);
#endif
#if MAX_PWMS > 1 && (PWM_EN_MASK & BIT(1))
		SINGLE_PWM(1);
#endif
#if MAX_PWMS > 2 && (PWM_EN_MASK & BIT(2))
		SINGLE_PWM(2);
#endif
#if MAX_PWMS > 3 && (PWM_EN_MASK & BIT(3))
		SINGLE_PWM(3);
#endif
#if MAX_PWMS > 4 && (PWM_EN_MASK & BIT(4))
		SINGLE_PWM(4);
#endif
#if MAX_PWMS > 5 && (PWM_EN_MASK & BIT(5))
		SINGLE_PWM(5);
#endif
#if MAX_PWMS > 6 && (PWM_EN_MASK & BIT(6))
		SINGLE_PWM(6);
#endif
#if MAX_PWMS > 7 && (PWM_EN_MASK & BIT(7))
		SINGLE_PWM(7);
#endif
#if MAX_PWMS > 8 && (PWM_EN_MASK & BIT(8))
		SINGLE_PWM(8);
#endif
#if MAX_PWMS > 9 && (PWM_EN_MASK & BIT(9))
		SINGLE_PWM(9);
#endif
#if MAX_PWMS > 10 && (PWM_EN_MASK & BIT(10))
		SINGLE_PWM(10);
#endif
#if MAX_PWMS > 11 && (PWM_EN_MASK & BIT(11))
		SINGLE_PWM(11);
#endif
#if MAX_PWMS > 12 && (PWM_EN_MASK & BIT(12))
		SINGLE_PWM(12);
#endif
#endif

		/* results in set bits where there are changes */

      __R30 = (__R30 & (clrmsk & 0xfff)) | (setmsk & 0xfff);
		
		/* loop while nothing changes */
		do {
			cnt = read_PIEP_COUNT();
			if(PWM_CMD->magic == PWM_CMD_MAGIC){
				break;
			}
		} while (((next - cnt) & (1U << 31)) == 0);
	}

	printf("Bingo, leave 1st simulator DSP APP~!\n");
}
#else

#define TIME_SUB(x, y) ((x > y)? (x - y) : (0xFFFFFFFF - y + x))



unsigned int chPWM[MAX_PWMS][2]; // 0: high, 1: low
unsigned int nextCnt[MAX_PWMS];

unsigned char chToggleIdx[MAX_PWMS]; // 0: high, 1: low
unsigned int chToggleDelta[MAX_PWMS]; //  store low length of motor channels


union channelMask{
	struct {
		// motor channels
		unsigned int ch0:1;
		unsigned int ch1:1;
		unsigned int ch2:1;
		unsigned int ch3:1;
		unsigned int ch4:1;
		unsigned int ch5:1;
		unsigned int ch6:1;
		unsigned int ch7:1;
		unsigned int ch8:1;

		// aux channels
		unsigned int ch9:1;
		unsigned int ch10:1;
		unsigned int ch11:1;
		unsigned int res:20;
	};

	unsigned int value;
}chMask;


unsigned int chFullPeriodMask = 0;

inline void initHW()
{
#if 0
	/* enable OCP master port */
	PRUCFG_SYSCFG &= ~SYSCFG_STANDBY_INIT;
	PRUCFG_SYSCFG = (PRUCFG_SYSCFG &
			~(SYSCFG_IDLE_MODE_M | SYSCFG_STANDBY_MODE_M)) |
			SYSCFG_IDLE_MODE_NO | SYSCFG_STANDBY_MODE_NO;

	/* our PRU wins arbitration */
	PRUCFG_SPP |=  SPP_PRU1_PAD_HP_EN;
	pwm_setup();

	/* configure timer */
	PIEP_GLOBAL_CFG = GLOBAL_CFG_DEFAULT_INC(1) |
			  GLOBAL_CFG_CMP_INC(1);
	PIEP_CMP_STATUS = CMD_STATUS_CMP_HIT(1); /* clear the interrupt */
        PIEP_CMP_CMP1   = 0x0;
	PIEP_CMP_CFG |= CMP_CFG_CMP_EN(1);
        PIEP_GLOBAL_CFG |= GLOBAL_CFG_CNT_ENABLE;
#endif
}


inline void initSW()
{
	unsigned int idx = 0;
	__R30 = 0;
	chMask.value = 0;
	chFullPeriodMask = 0;
	for(idx = 0; idx < MAX_PWMS; idx++)
	{
		chPWM[idx][0] = chPWM[idx][1] = 0;
		nextCnt[idx] = 0;
		chToggleIdx[idx] = 0;
		chToggleDelta[idx] = 0;

	}
}

inline void updateConfigs()
{
	static unsigned char first = 1;
	unsigned int idx = 0;

	chMask.value = PWM_CMD->enmask;

	for(idx = 0; idx < MAX_PWMS; idx++)
	{
		// ch disable then out
		if(!(chMask.value & (0x1 << idx)))
		{
			continue;
		}

		if(!first) // make sure full period
		{
			if(chFullPeriodMask & (0x1 << idx)) // full period then refresh configs
			{
				chPWM[idx][0] = PWM_CMD->periodhi[idx][1]; // high
				chPWM[idx][1] = PWM_CMD->periodhi[idx][0] - PWM_CMD->periodhi[idx][1]; // low
			}
		}
		else
		{
			first = 0;
			chPWM[idx][0] = PWM_CMD->periodhi[idx][1];
			chPWM[idx][1] = PWM_CMD->periodhi[idx][0] - PWM_CMD->periodhi[idx][1];
		}
	}

}

inline void waitForDeltaTime(unsigned int time)
{
	unsigned int curr = read_PIEP_COUNT();

	while(TIME_SUB(read_PIEP_COUNT(), curr) < time);

}

/*
 * main idea:
 * 	refresh configs: high, low
 * 	update next toggle time then pick out the min one (meanwhile record channel id)
 * 	toggle this channel when time is up, then mark this channel when full period to refresh configs
 */
void main()
{
	unsigned char firstStart = 1;
	unsigned int deltaMin = 0xFFFFFFFF;
	unsigned char idx = 0;
	unsigned char currChToggle = 0xFF;
	unsigned int currCnt = 0;

	// init pru HW
	initHW();

	// init SW: variables
	initSW();

	// main loop
	while(1)
	{
		// new configs arrive: update configs (make sure finish full period)
		if(PWM_CMD_MAGIC == PWM_CMD->magic)
		{
			updateConfigs();
		}


		// make sure channels enabled
		if(!chMask.value)
		{
			continue;
		}

		// update next toggle absolute time for all channels, update relative time to wait for enable channels
		if(!firstStart)
		{
			currCnt = read_PIEP_COUNT();
			for(idx = 0; idx < MAX_PWMS; idx++)
			{
				nextCnt[idx] += chPWM[idx][(chToggleIdx[idx]++)];

				// channel disable
				if(!(chMask.value & (0x1 << idx)))
				{
					__R30 &= ~(0x1 << idx);
					continue;
				}

				// channel enable
				chToggleDelta[idx] = TIME_SUB(nextCnt[idx], currCnt);
				if(deltaMin > chToggleDelta[idx])
				{
					deltaMin = chToggleDelta[idx];
					currChToggle = idx;
				}
			}
		}
		else
		{

			for(idx = 0; idx < MAX_PWMS; idx++)
			{
				// channel disable
				if(!(chMask.value & (0x1 << idx)))
				{
					__R30 &= ~(0x1 << idx);
					continue;
				}

				// channel enable
				firstStart = 0;

				chToggleDelta[idx] = chPWM[idx][(chToggleIdx[idx]++)];
				if(deltaMin > chToggleDelta[idx])
				{
					deltaMin = chToggleDelta[idx];
					currChToggle = idx;
				}
			}
		}


		// wait delta min to toggle
		waitForDeltaTime(deltaMin);
		// toggle channel PWM
		__R30 ^= (0x1 << currChToggle);
		// record toggle channel when high + low done: full period
		if(2 == chToggleIdx[currChToggle])
		{
			chToggleIdx[currChToggle] = 0;
			chFullPeriodMask |= (0x1 << currChToggle);
		}
		else
		{
			chFullPeriodMask &= ~(0x1 << currChToggle);
		}

	}

}

#endif
