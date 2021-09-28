/*
 * ldmaconfig.h
 *
 *  Created on: 05 Jul 2019
 *      Author: Johannes Ehala
 */

#ifndef LDMA_CONFIGURE_H_
#define LDMA_CONFIGURE_H_

#include "em_ldma.h"

#define MIC_ADC_DMA_CHANNEL	0
#define SERIAL_DMA_CHANNEL	1

void ldmainit(void);
void micLDMAstart(LDMA_Descriptor_t* micDescriptor);
void uartLDMAstart(LDMA_Descriptor_t* uartDescriptor);

#endif /* LDMA_CONFIGURE_H_ */
