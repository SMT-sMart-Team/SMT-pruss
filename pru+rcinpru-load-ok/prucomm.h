/*
 * prucomm.h - structure definitions for communication
 *
 */
#ifndef PRUCOMM_H
#define PRUCOMM_H

#include "pru_defs.h"

// AB ZhaoYJ for trying to add PPMSUM decoding in PRU @2016-05-21
// #define PPMSUM_DECODE 
#define MAX_RCIN_NUM 16
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

#ifdef PPMSUM_DECODE 
    volatile struct {
           volatile uint16_t rcin_value[MAX_RCIN_NUM];
           volatile uint16_t new_rc_input;
           volatile uint16_t _num_channels;
    } ppm_decode_out;
#endif
    // 
};

/* the command is at the start of shared DPRAM */
#define RBUFF        ((volatile struct ring_buffer *)DPRAM_SHARED)

#endif
