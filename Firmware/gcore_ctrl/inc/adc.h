/*
 * adc.h
 *
 * Header for ADC Module
 *
 * Copyright (c) 2021 danjuliodesigns, LLC.  All rights reserved.
 *
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "SI_EFM8SB2_Register_Enums.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Measurement indices (first 4 must be measured ADC values)
#define ADC_NUM_MEASUREMENTS 5
#define ADC_MEAS_VU_INDEX    0
#define ADC_MEAS_IU_INDEX    1
#define ADC_MEAS_VB_INDEX    2
#define ADC_MEAS_IL_INDEX    3
#define ADC_MEAS_T_INDEX     4

//
// Low-pass digital filter parameters
//  From: https://www.edn.com/design/systems-design/4320010/A-simple-software-lowpass-filter-suits-embedded-system-applications
//
//  K        Bandwidth (normalized to 1Hz)     Rise Time (Samples)
//  1        0.1197                            3
//  2        0.0466                            8
//  3        0.0217                            16
//  4        0.0104                            34
//  5        0.0051                            69
//  6        0.0026                            140
//  7        0.0012                            280
//  8        0.0007                            561
//
#define ADC_V_FILTER_SHIFT  5
#define ADC_I_FILTER_SHIFT  8

// Temperature averaging - must be a power-of-two
#define ADC_NUM_TEMP_SMPLS  8
#define ADC_TEMP_SHIFT      3
#define ADC_TEMP_RND_MASK   0x04

// External voltage reference value
#define ADC_VREF_MV         1250

// Temperature measurement evaluation interval (one temp sensor every 500 mSec)
#define ADC_EVALS_PER_MSEC  4
#define ADC_TEMP_EVAL_MSEC  500
#define ADC_TEMP_EVAL_COUNT (ADC_EVALS_PER_MSEC*ADC_TEMP_EVAL_MSEC)

// Timer0 Interrupt control macros
#define ADC_DIS_TMR0() IE_ET0 = 0
#define ADC_EN_TMR0()  IE_ET0 = 1

// ADC Interrupt control macros
#define ADC_DIS_INT() EIE1 &= ~EIE1_EADC0__BMASK
#define ADC_EN_INT()  EIE1 |= EIE1_EADC0__BMASK



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void ADC_Init();

#endif /* INC_ADC_H_ */
