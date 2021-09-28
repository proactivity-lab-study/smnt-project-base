/**
 * adc_print.c
 * 
 * ADC sampling is started here. ADC callback returns with a pointer to 
 * the measured ADC data (ping-pong buffer one). (ADC itself continues 
 * measurement to the second ping-pong buffer.) The pointer is given to 
 * serialldma.c to flush the the ADC data to UART. Once serialldma.c 
 * finishes and returns, ADC is notified that the first ping-pong buffer 
 * is free again. This must happen before ADC finishes with buffer two, 
 * otherwise continues sampling is interrupted/broken.
 * 
 * 
 *     Created: some time ago
 *      Author: Johannes Ehala
 **/

#include "em_chip.h" //for GPIO

#include "inttypes.h" //for types used in printf
#include "math.h"
#include "retargetserial.h"
#include "adc_print.h"
#include "serialldma.h"
#include "Microphones.h"

//assuming reference voltage 3.3V and 12bit ADC readings
//there's a way to automatically check this but... maybe later
#define MREFVOL 3.3f
#define ADCBITS12 4095
#define MIC_SAMPLES_BUF_LEN MIC_ADC_SAMPLES_PER_BATCH

volatile uint8_t data_ready = 0;
volatile uint16_t *mic_samples;
volatile bool adc_ldma_errors, sampling_enabled = false;
uint32_t cnt;

void ADC_callback(volatile uint16_t *samples, volatile bool error)
{
	mic_samples = samples;
	adc_ldma_errors = error;
	data_ready = 1;
}

uint8_t serialLDMADone()
{
	uint8_t ret;

	if(!adc_ldma_errors && data_ready == 0)
	{
		//go again
		//check for errors again, because an adc interrupt might of occured 
		if(sampling_enabled)micStartSampling(ADC_callback);
		ret = 0;
	}
	else 
	{
		//serial write was late!
		ret = 1;
	}

	return ret;
}

uint8_t adc_printout()
{
	uint8_t res = 0;

	micStartSampling(ADC_callback);
	sampling_enabled = true;
	cnt = 0;
	//we should break this loop only when there is a problem with ADC sampling and buffer switching
	//let main() decide what to do, and if necessary restart this function
	//if there are no problems this loop will run forever
	while(1)
	{
		if(data_ready)
		{
			cnt++;
			if(adc_ldma_errors){res = 1;break;}
			//voltage_signal();
			//normalized_signal();

			//serial write
			serialLDMAStart((uint32_t*)mic_samples);
			data_ready = 0;
		}
	}
	sampling_enabled = false;
	return res;
}

//there is an architectural problem here - where do I store the float values of vol?
//I need a new memory area for them and in that case I will inevitably do MIC_SAMPLES_BUF_LEN
//number of writes. will I have time for all this???
//
//a solution might be to redo the ADC LDMA descriptor to write 16 bit ADC values to
//32 bit memory area, so I can later convert the ADC readout to voltage in place. 
//I would still do MIC_SAMPLES_BUF_LEN number of writes, but I would need less memory
void voltage_signal()
{
	uint32_t i;
	float vol;

	for(i=0;i<MIC_SAMPLES_BUF_LEN;i++)
	{
		vol = (float)mic_samples[i]/ADCBITS12*MREFVOL;
		//printf("%"PRIi32".%"PRIi32"\n", (int32_t)vol, abs((int32_t)(vol*1000) - (int32_t)vol*1000));
	}
}

//same architectural problem here
void normalized_signal()
{
	uint32_t i;
	double mic_bias = 0, vol;
	mic_bias = 0;
	
	GPIO_PinModeSet(gpioPortB, 11, gpioModePushPull, 1);

	for(i=0;i<MIC_SAMPLES_BUF_LEN;i++)
	{
		mic_bias += mic_samples[i];
	}
	mic_bias /= MIC_SAMPLES_BUF_LEN;

	for(i=0;i<MIC_SAMPLES_BUF_LEN;i++)
	{
		vol = (mic_samples[i]-mic_bias)/ADCBITS12;//subtract bias and normalize
		//printf("%"PRIi32".%"PRIi32"\n", (int32_t)vol, abs((int32_t)(vol*1000) - (int32_t)vol*1000));
	}

	GPIO_PinModeSet(gpioPortB, 11, gpioModePushPull, 0);
}
