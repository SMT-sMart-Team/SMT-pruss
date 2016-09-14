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

// AB ZhaoYJ for multi-pwm to replace ppm-sum @2016-09-13
// GPI is not enough in PRU0, so move some of in PRU1
#define MULTI_PWM
/* maximum (PRU0 + PRU1) */
#define MAX_PWMS	12



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

#define PWM_CMD_KEEP_ALIVE 0xbeef
#define PWM_REPLY_KEEP_ALIVE 0x2152 // ~0xdead

struct pwm_cmd {
        u32 magic;
	u16 enmask;	/* enable mask */
	// u32 offmsk;	/* state when pwm is off */
	u32 periodhi[MAX_PWMS][2];
        u32 hilo_read[MAX_PWMS][2];
        // u32 enmask_read;
        u16 keep_alive; // flag, add By Roger
        u16 time_out; // second, add By Roger
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
#define PWM_CMD		((volatile struct pwm_cmd *)DPRAM_SHARED)


#ifdef MULTI_PWM
#define MAX_RCIN_NUM 4
uint8_t pwm_map[MAX_RCIN_NUM] = {3, 2, 1, 0}; // FIXME: need to confirm


struct rcin{
    volatile struct {
        volatile uint16_t high;
        volatile uint16_t low;
    }multi_pwm_out[MAX_RCIN_NUM];
};

// copy from rcinput_pru.h
struct rcin_ring_buffer {

    volatile uint16_t ring_head; // owned by ARM CPU
    volatile uint16_t ring_tail; // owned by the PRU
    struct {
        uint16_t pin_value;
        uint16_t delta_t;
    } buffer[300];
};

#define RCIN_PRUSS_SHAREDRAM_BASE   0x4a312000

#define RCIN ((volatile struct rcin*)(RCIN_PRUSS_SHAREDRAM_BASE + sizeof(rcin_ring_buffer)))

#endif

#endif
