/**
 * 
 *
 * @copyright ProLab, TTÜ 2020
 */
#include <stdbool.h>
#include <stdio.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"

#include "retargetserial.h"
#include "adc_print.h"
#include "ldmaconfig.h"
#include "Microphones.h"
#include "inttypes.h"

#include "ustimer.h"

#include "incbin.h"
INCBIN(Header, "header.bin");
#include "SignatureArea.h"
#include "DeviceSignature.h"
/*

#include <stdint.h>
#include <string.h>

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



*/
void setupOscillators(void)
{
	CMU_ClockEnable(cmuClock_CORELE, true);
}

int main(void)
{
  uint8_t doloop, res;
  uint32_t cpu_clock;

  /* Chip errata */
  CHIP_Init();

  setupOscillators();

  USTIMER_Init();
  //Important delay, because we switch programming pins to UART pins with the next line and we want reprogramming functionality to remain, at least at the beginning so we can reprogramm by first doing a reset and then catching the programming pins during this delay
  if (USTIMER_Delay(2000 * 1000) != 0) while (1) ;

  RETARGET_SerialInit();
  RETARGET_SerialCrLf(0);

  //3V sensor power must be enabled to use microphones
  /* Enable 3V3_SW, by pulling PB12 pin to ground. This enables 3.3V voltage to the sensors. */
  GPIO_PinModeSet(gpioPortB, 12, gpioModePushPull, 0);

  //power mode must be set to PWM mode otherwise there will be power supply noise in the signal
  //set power mode to 'burst mode' (0) or PWM mode (1)
  GPIO_PinModeSet(gpioPortF, 4, gpioModePushPull, 1);

  //another 2sec delay to allow 3V3 power to settle. 
  //I have noticed a ~2sec settling time in the ADC output. According to datasheets this cannot be
  //from Gecko ADC module nor from microphone sensor nor from buck-boost itself. I'm guessing it 
  //is from the large capacitance on the 3V3 line (C29+C31=130uOhm). With a typical resistance of
  //10kOhms on 3V3 the charge time for these capacitors would be 1.3sec, so this seems to fit the 
  //settling time I'm seeing ...but I don't know if this is the real cause for the settling time.
  if (USTIMER_Delay(2000 * 1000) != 0) while (1) ;
  
  cpu_clock = CMU_ClockFreqGet(cmuClock_CORE);
//  printf("Booted, CPU clock at %"PRIi32" MHz\n", cpu_clock);
//  printf("Setting CPU clock rate to %"PRIi32" MHz\n", (int32_t)cmuHFRCOFreq_38M0Hz);
//  printf("This will increase serial baud rate %"PRIi32" times\n", (int32_t)(cmuHFRCOFreq_38M0Hz/cpu_clock));
  CMU_HFRCOBandSet(cmuHFRCOFreq_38M0Hz);
  cpu_clock = CMU_ClockFreqGet(cmuClock_CORE);
//  printf("CPU clock now at %"PRIi32" MHz\n", cpu_clock);
//  printf("\n");
  USTIMER_Init(); //needs to be initiated again if HFPERCLK and/or HFCORECLK frequency is changed
  
  //init LDMA
  ldmainit();

  //ADC init, also creates LDMA descriptor for ADC->memory transfers
  micInit();

  //start mic measures
  doloop = 1;
  res = 0;
  while(doloop)
  {
    res = adc_printout();
    //do something with res if necessary
    //maybe change doloop
  }

  return 0;
}
