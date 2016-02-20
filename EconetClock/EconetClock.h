// EconetClock.h
// Created on: Feb 19, 2016
//  Also elements Copyright (C) 2016 Simon R. Ellwood

//
// PWM control register
//
#define ARM_PWM_CTL_PWEN1 (1 << 0)
#define ARM_PWM_CTL_MODE1 (1 << 1)
#define ARM_PWM_CTL_RPTL1 (1 << 2)
#define ARM_PWM_CTL_SBIT1 (1 << 3)
#define ARM_PWM_CTL_POLA1 (1 << 4)
#define ARM_PWM_CTL_USEF1 (1 << 5)
#define ARM_PWM_CTL_CLRF1 (1 << 6)
#define ARM_PWM_CTL_MSEN1 (1 << 7)
#define ARM_PWM_CTL_PWEN2 (1 << 8)
#define ARM_PWM_CTL_MODE2 (1 << 9)
#define ARM_PWM_CTL_RPTL2 (1 << 10)
#define ARM_PWM_CTL_SBIT2 (1 << 11)
#define ARM_PWM_CTL_POLA2 (1 << 12)
#define ARM_PWM_CTL_USEF2 (1 << 13)
#define ARM_PWM_CTL_MSEN2 (1 << 14)

//
// PWM status register
//
#define ARM_PWM_STA_FULL1 (1 << 0)
#define ARM_PWM_STA_EMPT1 (1 << 1)
#define ARM_PWM_STA_WERR1 (1 << 2)
#define ARM_PWM_STA_RERR1 (1 << 3)
#define ARM_PWM_STA_GAPO1 (1 << 4)
#define ARM_PWM_STA_GAPO2 (1 << 5)
#define ARM_PWM_STA_GAPO3 (1 << 6)
#define ARM_PWM_STA_GAPO4 (1 << 7)
#define ARM_PWM_STA_BERR  (1 << 8)
#define ARM_PWM_STA_STA1  (1 << 9)
#define ARM_PWM_STA_STA2  (1 << 10)
#define ARM_PWM_STA_STA3  (1 << 11)
#define ARM_PWM_STA_STA4  (1 << 12)

#define ECONET_ENABLE_PIN RPI_GPIO17
#define ECONET_CLOCK_PIN  RPI_GPIO18

extern void EconetClock_Init(void);
extern void EconetClock_Start(uint32_t Speed, uint32_t Duty);
extern void EconetClock_Stop(void);

