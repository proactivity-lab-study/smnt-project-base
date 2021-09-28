/*
 * Microphones.h
 *
 *  Created on: 21 Feb 2018
 *      Author: MS
 */

#ifndef DRIVER_MICROPHONES_H_
#define DRIVER_MICROPHONES_H_

/* ADC measuring channel location. See Datasheet pg. 152 */
#define MIC_ADC_CH	adcPosSelAPORT4XCH5  //PD13

/* Number of samples to measure for each channel */
/* Has to be multiple of 2048 because DMA has maximum of 2048 of transfers */
#define MIC_ADC_SAMPLES_PER_BATCH              		10240 	//5 * 2048 = 10240 - ~1 second of data @ 10kHz sampling speed

/* Sampling rate */
#define MIC_ADCSAMPLESPERSEC				2000

/*PRS and DMA channels used*/
#define MIC_PRS_CHANNEL				adcPRSSELCh0

/* Calculated definitions for DMA transfers*/
#define MIC_DMA_MAX_TRANSFERS				2048	/*Limited by width of DMA count register (11 bits)*/
#define MIC_DMA_NEEDED_NR_DESC				MIC_ADC_SAMPLES_PER_BATCH / MIC_DMA_MAX_TRANSFERS /*Needed number of descriptors*/

/* How many samples in ADC FIFO before DMA is triggered (max. 4) */
#define MIC_ADC_SCAN_DVL   			4

/*!
 * @brief Callback for microphones sampling complete
 */
typedef void (*microphone_call_back) (volatile uint16_t *samples, volatile bool error);

void micInit(void);
void micStartSampling(microphone_call_back callback);
void micLdmaSetup(void);
void micAdcScanSetup();
void adcLDMAIrq();

#endif /* DRIVER_MICROPHONES_H_ */
