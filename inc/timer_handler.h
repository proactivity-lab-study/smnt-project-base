/*
 * timer_handler.h
 *
 *  Created on: 15 April 2020
 *      Author: Johannes Ehala, ProLab
 */

#ifndef TIMER_HANDLER_H_
#define TIMER_HANDLER_H_

/* Route LED pins to TIMER0 CC for smnt-mb board */
#define LED0_LOC TIMER_ROUTELOC0_CC0LOC_LOC30 //PF6 - blue
#define LED1_LOC TIMER_ROUTELOC0_CC1LOC_LOC30 //PF7 - green

/* Which channel controls which LED */
#define LED0_CC_CHANNEL 0
#define LED1_CC_CHANNEL 1

#define TIMER0_TOP_VAL 100UL

/* These dim down LED0 and LED2, change value to 1 if not desired */
#define LED0_POWER_DIV 2UL

/* Maximum PWM duty cycle for each LED */
#define LED0_MAX_DC TIMER0_TOP_VAL/LED0_POWER_DIV
#define LED1_MAX_DC TIMER0_TOP_VAL

#define TIMER1_TOP_VAL 78

/* Public functions */
void timer1Init();
void timer0CCInit();
void startFadingLEDs();
uint8_t getLEDsPWM();
void setLEDsPWM(uint8_t val);

/* Private functions */
void changePWM_dutyCycle();

#endif /* TIMER_HANDLER_H_ */
