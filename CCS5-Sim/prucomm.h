/*
 * prucomm.h - structure definitions for communication
 *
 */
#ifndef PRUCOMM_H
#define PRUCOMM_H

#include "pru_defs.h"

struct pwm_config {
	u32 hi_cycles;
	u32 hi_err;
	u32 lo_cycles;
	u32 lo_err;
};

/* maximum (PRU0 + PRU1) */
#define MAX_PWMS	12

#ifndef BIT
#define BIT(x) (1U << (x))
#endif



unsigned int __R31;
unsigned int __R30;



/* mask of the possibly enabled PWMs (due to h/w) */
/* 14, 15 are not routed out for PRU1 */
#define PWM_EN_MASK	( \
	BIT( 0)|BIT( 1)|BIT( 2)|BIT( 3)|BIT( 4)|BIT( 5)|BIT( 6)|BIT( 7)| \
	BIT( 8)|BIT( 9)|BIT(10)|BIT(11)|BIT(12) \
	)

#define MIN_PWM_PULSE	PRU_us(4)

struct pwm_multi_config {
	u32 enmask;	/* enable mask */
	u32 offmsk;	/* state when pwm is off */
	u32 hilo[MAX_PWMS][2];
};

#define PWM_CMD_MAGIC	0xf00fbaaf
#define PWM_REPLY_MAGIC	0xbaaff00f
#define PWM_CMD_CONFIG	0	/* full configuration in one go */
#define PWM_CMD_ENABLE	1	/* enable a pwm */
#define PWM_CMD_DISABLE	2	/* disable a pwm */
#define PWM_CMD_MODIFY	3	/* modify a pwm */
#define PWM_CMD_SET	4	/* set a pwm output explicitly */
#define PWM_CMD_CLR	5	/* clr a pwm output explicitly */ 
#define PWM_CMD_TEST	6	/* various crap */

struct pwm_cmd {
        u32 magic;
	u32 enmask;	/* enable mask */
	u32 offmsk;	/* state when pwm is off */
	u32 periodhi[MAX_PWMS][2];
        u32 hilo_read[MAX_PWMS][2];
        u32 enmask_read;
};
struct pwm_cmd_l{
    u32 enmask;
    u32 offmsk;
    u32 hilo[MAX_PWMS][2];
};


struct cxt {
        u32 cnt;
        u32 next;
        u32 enmask;
        u32 stmask;
        u32 setmsk;
        u32 clrmsk;
        u32 deltamin;
        u32 *next_hi_lo;
};


/* the command is at the start of shared DPRAM */
struct pwm_cmd pcmd;

#define PWM_CMD		((volatile struct pwm_cmd *)&pcmd)

/* NOTE: Do no use it for larger than 5 secs */
#define PRU_200MHz_sec(x)	((u32)(((x) * 200000000)))
#define PRU_200MHz_ms(x)	((u32)(((x) * 200000)))
#define PRU_200MHz_ms_err(x)	0
#define PRU_200MHz_us(x)	((u32)(((x) * 200)))
#define PRU_200MHz_us_err(x)	0
#define PRU_200MHz_ns(x)	((u32)(((x) * 2) / 10))
#define PRU_200MHz_ns_err(x)	((u32)(((x) * 2) % 10))

#define PRU_CLK 200000000

#if PRU_CLK != 200000000
/* NOTE: Do no use it for larger than 5 secs */
#define PRU_sec(x)	((u32)(((u64)(x) * PRU_CLK)))
#define PRU_ms(x)	((u32)(((u64)(x) * PRU_CLK) / 1000))
#define PRU_ms_err(x)	((u32)(((u64)(x) * PRU_CLK) % 1000))
#define PRU_us(x)	((u32)(((u64)(x) * PRU_CLK) / 1000000))
#define PRU_us_err(x)	((u32)(((u64)(x) * PRU_CLK) % 1000000))
#define PRU_ns(x)	((u32)(((u64)(x) * PRU_CLK) / 1000000000))
#define PRU_ns_err(x)	((u32)(((u64)(x) * PRU_CLK) % 1000000000))
#else
/* NOTE: Do no use it for larger than 5 secs */
#define PRU_sec(x)	PRU_200MHz_sec(x)
#define PRU_ms(x)	PRU_200MHz_ms(x)
#define PRU_ms_err(x)	PRU_200MHz_ms_err(x)
#define PRU_us(x)	PRU_200MHz_us(x)
#define PRU_us_err(x)	PRU_200MHz_us_err(x)
#define PRU_ns(x)	PRU_200MHz_ns(x)
#define PRU_ns_err(x)	PRU_200MHz_ns_err(x)
#endif

#endif
