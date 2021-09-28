/*
 * ldmaconfig.c
 * 
 * Here LDMA is initialized and different LDMA channels are configured and started.
 * Also LDMA IRQ handler is here, because there is only one LDMA IRQ. Interrupts 
 * branch out from here based on the channels they apply to.
 *
 *     Created: 05 Jul 2019
 *      Author: Johannes Ehala
 */

#include "em_cmu.h" //for clocks
#include "Microphones.h"
#include "ldmaconfig.h"
#include "serialldma.h"

/******************************************************************************
 * @brief
 *   LDMA IRQ handler.
 ******************************************************************************/

void LDMA_IRQHandler(void)
{
	/* Get all pending and enabled interrupts. */
	uint32_t pending = LDMA_IntGetEnabled();

	/* Loop here on an LDMA error to enable debugging. */
	while (pending & LDMA_IF_ERROR) {}

	if(pending & (1 << MIC_ADC_DMA_CHANNEL))
	{
		/* Clear interrupt flag. */
		LDMA->IFC = (1 << MIC_ADC_DMA_CHANNEL);

		//notify mic controller-driver
		adcLDMAIrq();
	}

	if(pending & (1 << SERIAL_DMA_CHANNEL))
	{
		/* Clear interrupt flag. */
		LDMA->IFC = (1 << SERIAL_DMA_CHANNEL);

		//notify serialldma.c
		serialLDMAIrq();
	}
}

/******************************************************************************
 * @brief Initialize the LDMA controller
 ******************************************************************************/
void ldmainit(void)
{	
	/* Initialize the LDMA controller */
	LDMA_Init_t init = LDMA_INIT_DEFAULT;//only priority based arbitration, no round-robin
	LDMA_Init(&init);

	CMU_ClockEnable(cmuClock_LDMA, true);
	//CMU_ClockEnable(cmuClock_PRS, true);peripheral reflex system must be enabled
}

/******************************************************************************
 * @brief Start LDMA for ADC to memory transfer
 ******************************************************************************/
void micLDMAstart(LDMA_Descriptor_t* micDescriptor)
{
	/* Macro for scan mode ADC */
	LDMA_TransferCfg_t adcScanTx = LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_ADC0_SCAN);
	
	LDMA_IntEnable(1 << MIC_ADC_DMA_CHANNEL);
	NVIC_ClearPendingIRQ(LDMA_IRQn);
	NVIC_EnableIRQ(LDMA_IRQn);

	/* Start ADC LMDA transfer */
	LDMA_StartTransfer(MIC_ADC_DMA_CHANNEL, &adcScanTx, micDescriptor);
}


/******************************************************************************
 * @brief Start LDMA for memory to UART transfer
 ******************************************************************************/
void uartLDMAstart(LDMA_Descriptor_t* uartDescriptor)
{
	/* Macro for scan mode ADC */
	uint32_t loopCnt = MIC_DMA_NEEDED_NR_DESC - 2;//5 iterations, 1 done by first descriptor, 4 done by loop (so loop is repeated 3 times)
	LDMA_TransferCfg_t memToUartCfg = LDMA_TRANSFER_CFG_PERIPHERAL_LOOP(ldmaPeripheralSignal_USART0_TXBL, loopCnt);


	LDMA_IntEnable(1 << SERIAL_DMA_CHANNEL);
	NVIC_ClearPendingIRQ(LDMA_IRQn);
	NVIC_EnableIRQ(LDMA_IRQn);

	/* Start ADC LMDA transfer */
	LDMA_StartTransfer(SERIAL_DMA_CHANNEL, &memToUartCfg, uartDescriptor);
}
