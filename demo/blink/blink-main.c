/**
 * Blink example for smnt-mb devices.
 *
 * Uses PWM to fade LEDs on and off.
 *
 * @copyright ProLab, TTÜ 2020
 * @license MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include "em_chip.h"
#include "em_rmu.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_gpio.h"

#include "sleep.h" // emdrv sleep.h
#include "lptsleep.h"

#include "cmsis_os2_ext.h"

#include "basic_rtos_logger_setup.h"

#include "platform.h"

#include "incbin.h"
#include "SignatureArea.h"
#include "DeviceSignature.h"

#include "timer_handler.h"

#include "loglevels.h"
#define __MODUUL__ "main"
#define __LOG_LEVEL__ (LOG_LEVEL_main & BASE_LOG_LEVEL)
#include "log.h"

// Add the headeredit block
#include "incbin.h"
INCBIN(Header, "header.bin");

static osMutexId_t m_led_mutex;

static void led0_timer_cb(void* argument)
{
    osMutexAcquire(m_led_mutex, osWaitForever);
    debug1("led0 timer");
	setLEDsPWM(getLEDsPWM()^1);
    osMutexRelease(m_led_mutex);
}

static void led1_timer_cb(void* argument)
{
    osMutexAcquire(m_led_mutex, osWaitForever);
    debug1("led1 timer");
	setLEDsPWM(getLEDsPWM()^2);
    osMutexRelease(m_led_mutex);
}

// App loop - do setup and periodically print status
void app_loop ()
{
	// Switch to a thread-safe logger
 	basic_rtos_logger_setup();

	debug1("main_loop");

    m_led_mutex = osMutexNew(NULL);

    osTimerId_t led0_timer = osTimerNew(&led0_timer_cb, osTimerPeriodic, NULL, NULL);
    osTimerId_t led1_timer = osTimerNew(&led1_timer_cb, osTimerPeriodic, NULL, NULL);

	timer0CCInit();

    osTimerStart(led0_timer, 1000);
	osDelay(1000);
    osTimerStart(led1_timer, 1000);
   
	for (;;)
    {
        osMutexAcquire(m_led_mutex, osWaitForever);
        info1("leds %u", getLEDsPWM());
        osMutexRelease(m_led_mutex);
        osDelay(1000);
    }
}

//Use TIMER1 to regularly change PWM duty cycle and create LED fading effect
void dimmer_loop ()
{
	timer1Init();
	startFadingLEDs();
}

int main()
{
	PLATFORM_Init(); // Does CHIP_Init() and MSC_Init(), returns resetCause

	// LED
	PLATFORM_LedsInit();

	basic_noos_logger_setup();

    debug1("Blink-PWM "VERSION_STR" (%d.%d.%d)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	// Initialize node signature module
	sigInit();
	uint8_t eui[8];
	sigGetEui64(eui);
	infob1("EUI64:", eui, sizeof(eui));

	// Must initialize kernel to allow creation of threads/mutexes etc
    osKernelInitialize();

    // Initialize sleep management
    SLEEP_Init(NULL, NULL);
    vLowPowerSleepModeSetup(sleepEM3); // Must block EM3, as RTCC does not work in EM3
    vLowPowerSleepTimerSetup(cmuSelect_LFRCO); // Inaccurate, but always present

    // Most actual initialization can be performed once kernel has booted
    const osThreadAttr_t main_thread_attr = { .name = "main" };
    osThreadNew(app_loop, NULL, &main_thread_attr);

    const osThreadAttr_t timer1_thread_attr = { .name = "timer1" };
    osThreadNew(dimmer_loop, NULL, &timer1_thread_attr);

    if (osKernelReady == osKernelGetState())
    {
        osKernelStart();
    }
    else
    {
        err1("!osKernelReady");
    }

    for (;;);
}
