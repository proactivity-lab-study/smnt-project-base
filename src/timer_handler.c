/*
 * timer_handler.c
 *
 * TIMER0 PWM is used to control all LEDs
 * TIMER1 is used to trigger fading effect of LEDs
 * 
 *  Created on: 15 April 2020
 *      Author: Johannes Ehala, ProLab
 * 
 */

#include "em_cmu.h"
#include "em_timer.h"

#include "timer_handler.h"

volatile static uint8_t led_state = 0;
uint32_t led0_cnt, led1_cnt, led0_sd;
uint8_t led0_toggle, led1_toggle;

enum 
{
	FIRE_UP = 0,
	COOL_DOWN
};

/****************************************************************************
 * @brief Init TIMER1 to regulate PWM duty cycle. 
 *****************************************************************************/
void timer1Init(void)
{
	/* Enable clocks */
    CMU_ClockEnable(cmuClock_TIMER1, true);

	/* Set TIMER top value */
	TIMER_TopSet(TIMER1, TIMER1_TOP_VAL);

	/* TIMER general init */
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	timerInit.prescale = timerPrescale1024;
	timerInit.enable = false; //don't start timer after init

	/* LED0 and LED2 duty cycle count-down is slowed down; init counters */
	led0_sd = 1;

	TIMER_Init(TIMER1, &timerInit);
}

/****************************************************************************
 * @brief Init TIMER0 for PWM usage on three CC channels. Start TIMER0.
 *****************************************************************************/
void timer0CCInit(void)
{
	/* Enable clocks */
    CMU_ClockEnable(cmuClock_TIMER0, true);

	/* Init CC for PWM on GPIO pins */
	TIMER_InitCC_TypeDef ccInit = TIMER_INITCC_DEFAULT;
	ccInit.mode = timerCCModePWM;
	ccInit.cmoa = timerOutputActionToggle;

	/* Initilize a CC channels for each LED */
	TIMER_InitCC(TIMER0, LED0_CC_CHANNEL, &ccInit);
	TIMER_InitCC(TIMER0, LED1_CC_CHANNEL, &ccInit);

	/* Enable GPIO toggling by TIMER and set location of pins to be toggled */
	TIMER0->ROUTEPEN = (TIMER_ROUTEPEN_CC0PEN | TIMER_ROUTEPEN_CC1PEN);
	TIMER0->ROUTELOC0 = (LED0_LOC | LED1_LOC);

	/* Set TIMER0 top value, same for all CC channels */
	TIMER_TopSet(TIMER0, TIMER0_TOP_VAL);

	/* Set the PWM duty cycle, init all LEDs to zero */
	TIMER_CompareBufSet(TIMER0, LED0_CC_CHANNEL, 0);
	TIMER_CompareBufSet(TIMER0, LED1_CC_CHANNEL, 0);
	led0_cnt = led1_cnt = 0;
	led0_toggle = led1_toggle = COOL_DOWN;	

	/* TIMER general init */
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	timerInit.prescale = timerPrescale256;
	timerInit.enable = true; //start timer after init

	TIMER_Init(TIMER0, &timerInit);
}

/****************************************************************************
 * @brief Start TIMER1 and PWM duty cycle manipulation.
 *****************************************************************************/
void startFadingLEDs()
{
	TIMER_IntClear(TIMER1, TIMER_IFC_OF);
	TIMER_IntEnable(TIMER1, TIMER_IntGet(TIMER1) | TIMER_IEN_OF);
	TIMER_Enable(TIMER1, true);

	//TODO: Perhaps replace with an interrupt handler
	for(;;)
	{
		if(TIMER1->IF & _TIMER_IF_OF_MASK)//overflow has occurred
		{
			changePWM_dutyCycle();
			TIMER_IntClear(TIMER1, TIMER_IFC_OF);
		}
	}
}

/****************************************************************************
 * @brief Sets the state of LEDs. State change triggers gradual fading/- 
 * 		brightening of LED.
 *****************************************************************************/
void setLEDsPWM(uint8_t val)
{
	if(val&1)led0_toggle = FIRE_UP;
	else led0_toggle = COOL_DOWN;

	if(val&2)led1_toggle = FIRE_UP;
	else led1_toggle = COOL_DOWN;

	led_state = val;
}

/****************************************************************************
 * @brief Get current LED state. LEDs in the process of change are reported
 * 		as already changed state (ie if LED is fading from ON to OFF, it is 
 * 		reported as OFF already).
 *****************************************************************************/
uint8_t getLEDsPWM()
{
	return led_state;
}

/****************************************************************************
 * @brief Gradually fade/brighten LEDs. Also keep all LEDs at similar 
 * 		brightness. This means dimming LED0 (red) and LED2 (blue) to LED1 
 * 		(green) level.
 *****************************************************************************/
void changePWM_dutyCycle()
{
	if(led0_sd == LED0_POWER_DIV)
	{
		/* For LED 0 */
		if(led0_toggle == FIRE_UP)
		{
			if(led0_cnt <= LED0_MAX_DC)
			{
				led0_cnt++;
				TIMER_CompareBufSet(TIMER0, LED0_CC_CHANNEL, led0_cnt);
			}
		}
		else if(led0_toggle == COOL_DOWN)
		{
			if(led0_cnt > 0)
			{
				led0_cnt--;
				TIMER_CompareBufSet(TIMER0, LED0_CC_CHANNEL, led0_cnt);
			}
		}
		else ;
	}

	/* For LED 1 */
	if(led1_toggle == FIRE_UP)
	{
		if(led1_cnt <= LED1_MAX_DC)
		{
			led1_cnt++;
			TIMER_CompareBufSet(TIMER0, LED1_CC_CHANNEL, led1_cnt);
		}
	}
	else if(led1_toggle == COOL_DOWN)
	{
		if(led1_cnt > 0)
		{
			led1_cnt--;
			TIMER_CompareBufSet(TIMER0, LED1_CC_CHANNEL, led1_cnt);
		}
	}
	else ;

	/* LED0 and LED2 slow down - dims LED0 and LED2 to LED1 level */
	if(led0_sd >= LED0_POWER_DIV)led0_sd = 1;
	else led0_sd++;
}
