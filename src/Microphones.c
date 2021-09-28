/*
 * Microphones.c
 *
 *  Created on: 21 Feb 2018
 *      Author: MS
 *      Editor: Johannes Ehala
 */

#include "em_chip.h"
#include "em_cmu.h"
#include "em_adc.h"
#include "em_prs.h"
#include "em_timer.h"

#include "Microphones.h"
#include "ldmaconfig.h"

/* Buffer to hold all of the samples. */
volatile uint16_t buffer_1[MIC_ADC_SAMPLES_PER_BATCH];
volatile uint16_t buffer_2[MIC_ADC_SAMPLES_PER_BATCH];

/* Descriptor linked list for LDMA transfer */
LDMA_Descriptor_t descLinkMic[MIC_DMA_NEEDED_NR_DESC*2];//we need descriptors for both buffers, hence *2

/* Callback is called from LDMA interrupt handler*/
volatile microphone_call_back mic_callback;
bool b1_pending = true;
bool upper_layer_done = true;

/***************************************************************************//**
 * @brief
 *   ADC LDMA IRQ handler.
 ******************************************************************************/

void adcLDMAIrq()
{
	if(upper_layer_done)
	{
		if(b1_pending)
		{
			b1_pending = false;
			mic_callback(buffer_1, false);
			upper_layer_done = false;
			//printf("b1\n");
		}
		else 
		{
			b1_pending = true;
			mic_callback(buffer_2, false);
			upper_layer_done = false;
			//printf("b2\n");
		}
	}
	else
	{
		//stop timer and ADC
		TIMER_Enable(TIMER0, false);
		mic_callback(buffer_2, true);
		LDMA_IntDisable(1 << MIC_ADC_DMA_CHANNEL);
		LDMA_StopTransfer(1 << MIC_ADC_DMA_CHANNEL);
	}
}

void micInit(void)
{
	/* Enable clocks */
	CMU_ClockEnable(cmuClock_ADC0, true);
	CMU_ClockEnable(cmuClock_TIMER0, true);
	CMU_ClockEnable(cmuClock_PRS, true);

	/* Configure DMA transfer from ADC to RAM */
	micLdmaSetup();

	/* Configure ADC Sampling and TIMER trigger through PRS */
	micAdcScanSetup();
}

void micStartSampling(microphone_call_back callback)
{
	//this upper_layer_done flag stuff is a bit of a hack..
	mic_callback = callback;
	if(upper_layer_done)
	{
		//Start LDMA for ADC to memory transfer
		micLDMAstart(&descLinkMic[0]);

		/* ADC is started by starting the timer*/
		TIMER_Enable(TIMER0, true);
	}
	upper_layer_done = true;
}

/***************************************************************************//**
 * @brief Initialize the LDMA controller for ADC transfers
 ******************************************************************************/

void micLdmaSetup(void)
{
	/*Create needed number of linked descriptors for buffer_1 and buffer_2. Descriptors are based on LDMA_DESCRIPTOR_LINKREL_P2M_BYTE*/
	uint16_t i = 0;
	for (i=0; i <= (MIC_DMA_NEEDED_NR_DESC*2 - 1); i++)
	{
		descLinkMic[i].xfer.structType   = ldmaCtrlStructTypeXfer;
		descLinkMic[i].xfer.structReq    = 0;
		descLinkMic[i].xfer.xferCnt      = MIC_DMA_MAX_TRANSFERS - 1;
		descLinkMic[i].xfer.byteSwap     = 0;
		descLinkMic[i].xfer.blockSize    = ldmaCtrlBlockSizeUnit4; /*Block size is 4 because of ADC FIFO has 4 samples*/
		descLinkMic[i].xfer.doneIfs      = 0;
		descLinkMic[i].xfer.reqMode      = ldmaCtrlReqModeBlock;
		descLinkMic[i].xfer.decLoopCnt   = 0;
		descLinkMic[i].xfer.ignoreSrec   = 1;//start transfer only when ADC FIFO is full
		descLinkMic[i].xfer.srcInc       = ldmaCtrlSrcIncNone;
		descLinkMic[i].xfer.size         = ldmaCtrlSizeHalf;	/*ADC sample fits to 16 bits*/
		descLinkMic[i].xfer.dstInc       = ldmaCtrlDstIncOne;
		descLinkMic[i].xfer.srcAddrMode  = ldmaCtrlSrcAddrModeAbs;
		descLinkMic[i].xfer.dstAddrMode  = ldmaCtrlDstAddrModeAbs;
		descLinkMic[i].xfer.srcAddr      = (uint32_t)(&ADC0->SCANDATA);
		descLinkMic[i].xfer.linkMode     = ldmaLinkModeRel;
		descLinkMic[i].xfer.link         = 1;
		
		//assigne destination address for this descriptor in either buffer_1 or buffer_2
		if(i <= (MIC_DMA_NEEDED_NR_DESC - 1))
		{		
			descLinkMic[i].xfer.dstAddr      = (uint32_t)(&buffer_1) + i * MIC_DMA_MAX_TRANSFERS * 2; /*increase the destination data pointer for uint16_t values*/
		}
		else
		{		
			descLinkMic[i].xfer.dstAddr      = (uint32_t)(&buffer_2) + (i - MIC_DMA_NEEDED_NR_DESC) * MIC_DMA_MAX_TRANSFERS * 2; /*increase the destination data pointer for uint16_t values*/
		}

		//interrupt should be set for last descriptor of buffer_1 and buffer_2
		if(i == (MIC_DMA_NEEDED_NR_DESC - 1))descLinkMic[i].xfer.doneIfs = 1;
		if(i == (MIC_DMA_NEEDED_NR_DESC*2 - 1))descLinkMic[i].xfer.doneIfs = 1;

		//link this discriptor to the beginning of linked list if it is the last or to the next otherwise
		if(i == (MIC_DMA_NEEDED_NR_DESC*2 - 1))
		{
			//last link should loop back to beginning of list, so we get a round-buffer
			descLinkMic[i].xfer.linkAddr     = -((MIC_DMA_NEEDED_NR_DESC*2)-1) * 4;/*Points to first descriptor in list*/
		}	
		else
		{
			descLinkMic[i].xfer.linkAddr     = (1) * 4;	/*Next descriptor pointer*/
		}
	}
}

/**************************************************************************//**
 * @brief Configure TIMER to trigger ADC through PRS at a set sample rate
 *****************************************************************************/
void micAdcScanSetup()
{
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitScan_TypeDef scanInit = ADC_INITSCAN_DEFAULT;

  /* Kostja poolne täiendus igaks juhuks */
  GPIO_PinModeSet(gpioPortD, 14, gpioModeInput, 1);
  GPIO_PinModeSet(gpioPortA, 4, gpioModeInput, 1);

  /* Setup scan channels, define DEBUG_EFM in debug build to identify invalid channel range */
  ADC_ScanSingleEndedInputAdd(&scanInit, adcScanInputGroup0, MIC_ADC_CH);

  /* Initialize for scan conversion */
  scanInit.prsSel = MIC_PRS_CHANNEL;
  scanInit.reference = adcRefVDD;
  scanInit.prsEnable = true;
  scanInit.fifoOverwrite = true;
  ADC_InitScan(ADC0, &scanInit);


  /* Set scan data valid level (DVL) to trigger */
  ADC0->SCANCTRLX |= (MIC_ADC_SCAN_DVL - 1) << _ADC_SCANCTRLX_DVL_SHIFT;

  /* Use HFPERCLK frequency to setup ADC if run on EM1 */
  init.prescale = ADC_PrescaleCalc(16000000, 0);

  /* Init common issues for both single conversion and scan mode */
  ADC_Init(ADC0, &init);

  /* Connect PRS channel 0 to TIMER overflow */
  PRS_SourceSignalSet(0, PRS_CH_CTRL_SOURCESEL_TIMER0, PRS_CH_CTRL_SIGSEL_TIMER0OF, prsEdgeOff);

  /* Clear the FIFOs and pending interrupt */
  ADC0->SCANFIFOCLEAR = ADC_SCANFIFOCLEAR_SCANFIFOCLEAR;

  /* Configure TIMER to trigger at sampling rate */
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.enable = false;                             /* Do not start counting when init complete */
  TIMER_Init(TIMER0, &timerInit);
  TIMER_TopSet(TIMER0,  CMU_ClockFreqGet(cmuClock_TIMER0)/MIC_ADCSAMPLESPERSEC);
  //TIMER_Enable(TIMER0, true);

}
