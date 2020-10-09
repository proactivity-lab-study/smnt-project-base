/**
 * Project base for smnt-mb devices.
 *
 * @copyright ProLab 2020
 * @license <TBD>
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

#include "dmadrv.h"

#include "platform_io.h"
#include "platform_adc.h"

#include "sleep.h" // emdrv sleep.h
#include "lptsleep.h"

#include "cmsis_os2_ext.h"

#include "basic_rtos_logger_setup.h"

#include "platform.h"

#include "incbin.h"
#include "SignatureArea.h"
#include "DeviceSignature.h"

#include "loglevels.h"
#define __MODUUL__ "main"
#define __LOG_LEVEL__ (LOG_LEVEL_main & BASE_LOG_LEVEL)
#include "log.h"

// Add the headeredit block
#include "incbin.h"
INCBIN(Header, "header.bin");

static void main_loop (void * arg)
{
	bool annoy = false;

	debug1("main_loop");

	// Switch to a thread-safe logger
 	basic_rtos_logger_setup();

	for (;;)
	{
		osDelay(1000); //1 sec
		if(annoy)
		{
			debug1("/t/tNotice me");
			annoy = false;
		}
		else
		{
			debug1("Hello");
			annoy = true;
		}
	}
}

int main()
{
	PLATFORM_Init(); // Does CHIP_Init() and MSC_Init(), returns resetCause

	// LED
	PLATFORM_LedsInit();

	basic_noos_logger_setup();

    debug1("Sense MW "VERSION_STR" (%d.%d.%d)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	// Initialize node signature module
	sigInit();
	uint8_t eui[8];
	sigGetEui64(eui);
	infob1("EUI64:", eui, sizeof(eui));

    // Radio GPIO/PRS for LNA on some MGM12P
    PLATFORM_RadioInit();
	GPIO_PinModeSet(gpioPortF, 6, gpioModePushPull, 1);
    
	// Must initialize kernel to allow creation of threads/mutexes etc
    osKernelInitialize();

    // Initialize sleep management
    SLEEP_Init(NULL, NULL);
    vLowPowerSleepModeSetup(sleepEM3); // Must block EM3, as RTCC does not work in EM3
    vLowPowerSleepTimerSetup(cmuSelect_LFRCO); // Inaccurate, but always present

    // Most actual initialization can be performed once kernel has booted
    const osThreadAttr_t main_thread_attr = { .name = "main" };
    osThreadNew(main_loop, NULL, &main_thread_attr);

    if (osKernelReady == osKernelGetState())
    {
        osKernelStart();
    }
    else
    {
        err1("!osKernelReady");
    }

    while(true);
}
