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


#define VALID_CH 8
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
	Disable_ch,
	Disable_all
};

enum bool_type {
	b_true,
	b_false
};

enum control_flag flag = Init;
enum bool_type full_period = b_true;
unsigned char time_flow_ctr[5] = {2, 4, 600000, 1080000, 2110000};
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
	static unsigned char ctr = 0;
	unsigned int ii = 0;
	unsigned int incr = 60;
	unsigned char first_prt = 1;

	//printf("ctr: %d\n", ctr);
	//printf("tf_ctr_idx: %d\n", time_flow_idx);
	//printf("time_flow_ctr[%d]: %d\n", time_flow_idx, time_flow_ctr[time_flow_idx++]);

	// ctl flag via time flow

	for(ii = 0; ii < 5; ii++)
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
		break;
	case Period_change1:
		PWM_CMD->enmask = 0xFF;
		if(first_prt)
		{
			first_prt = 0;
			printf("=========================================================\n");
			printf("Enter Period_change[111] state~!\n");
		}
		for(ii = 0; ii < VALID_CH; ii++)
		{
			//incr += ii*incr;
			incr = rand_period_change();
			PWM_CMD->periodhi[ii][0] = PRU_us(2500);
			PWM_CMD->periodhi[ii][1] = PRU_us((1500 + incr)%2500);
			//printf("Period_change[111]: rand_period_change: [[%08x]]\n", incr);
			printf("CH[%d]: hi: %08x lo: %08x \n", ii, PWM_CMD->periodhi[ii][1], PWM_CMD->periodhi[ii][0] - PWM_CMD->periodhi[ii][1]);

		}

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
		for(ii = 0; ii < VALID_CH; ii++)
		{
			incr = rand_period_change();
			PWM_CMD->periodhi[ii][0] = PRU_us(2500);
			PWM_CMD->periodhi[ii][1] = PRU_us((1500 - incr)%2500);
			//printf("Period_change[222]: rand_period_change: [[%08x]]\n", incr);
			printf("CH[%d]: hi: %08x lo: %08x \n", ii, PWM_CMD->periodhi[ii][1], PWM_CMD->periodhi[ii][0] - PWM_CMD->periodhi[ii][1]);
		}

		break;
	case Disable_ch:
		PWM_CMD->enmask = 0x2;
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

        //if(PWM_CMD->magic == PWM_CMD_MAGIC) 
        {
			msk = PWM_CMD->enmask;
            for(i=0, nextp = &next_hi_lo[0][0]; i<MAX_PWMS; 
                i++, nextp += 3){
                //Enable
                if ((PWM_EN_MASK & (msk&(1U<<i))) && (enmask & (msk&(1U<<i))) == 0) {
        		        enmask |= (msk&(1U<<i));

    				    __R30 |= (msk&(1U<<i));


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
						if(!((ch_num_period) %= VALID_CH))
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
