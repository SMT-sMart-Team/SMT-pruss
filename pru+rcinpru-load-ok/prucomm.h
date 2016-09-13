/*
 * prucomm.h - structure definitions for communication
 *
 */
#ifndef PRUCOMM_H
#define PRUCOMM_H

#include "pru_defs.h"

// AB ZhaoYJ for trying to add PPMSUM decoding in PRU @2016-05-21
// #define PPMSUM_DECODE 
//
// AB ZhaoYJ for multi-pwm to replace ppm-sum @2016-09-13
#define MULTI_PWM

#ifdef MULTI_PWM
#define MAX_RCIN_NUM 8
#define NUM_RCIN_BUFF 64
uint8_t pwm_map[MAX_RCIN_NUM] = {15, 2, 3, 0, 1, 6, 7, 8}; // FIXME: need to confirm
#else
#define MAX_RCIN_NUM 16
#endif

#define OK 0xbeef
#define KO 0x4110 /// !beef

// #define NUM_RING_ENTRIES 256
#define NUM_RING_ENTRIES 300
    
#define PWM_CMD_MAGIC    0xf00fbaaf
#define PWM_REPLY_MAGIC  0xbaaff00f


struct ring_buffer{
    volatile uint16_t ring_head;
    volatile uint16_t ring_tail;
    volatile struct {
           volatile uint16_t pin_value;
           volatile uint16_t delta_t;
    } buffer[NUM_RING_ENTRIES];


#ifdef MULTI_PWM 
    volatile struct {
        volatile uint16_t high;
        volatile uint16_t low;
    }multi_pwm_out[MAX_RCIN_NUM];
#endif
    // 
};

/* the command is at the start of shared DPRAM */
#define RBUFF        ((volatile struct ring_buffer *)DPRAM_SHARED)

#endif
