/**
 * serialldma.c
 * 
 * Here descriptors are created for memory to UART transfer. Transfer is also
 * started/stopped through this module.
 * 
 * LDMA interrupts related to this LDMA channel (SERIAL_DMA_CHANNEL) are 
 * handled/routed through this module.
 * 
 *     Created: 18 Jul 2019
 *      Author: Johannes Ehala
 **/

#include "ldmaconfig.h"

LDMA_Descriptor_t memToUartLoopDsc[2]; //two descriptors are needed, first one with absolute source address and second for loop with relative address

/**
 * Here we handle LDMA interrupts related to our DMA channel (SERIAL_DMA_CHANNEL).
 * 
 **/
void serialLDMAIrq(void)
{
	if(serialLDMADone() != 0)
	{
		//serial write was late!
		LDMA_IntDisable(1 << SERIAL_DMA_CHANNEL);
		LDMA_StopTransfer(1 << SERIAL_DMA_CHANNEL);
	}
}

/**
 * This is a linked loop descriptor, looping to itself. No interrupts are generated
 * when a descriptor in the loop finishes and there are more iterations of the
 * loop left. An interrupt is generated only when the last descriptor in the
 * loop is finished. NB! LDMA must be configured for loop transfer, this is 
 * done elsewhere.
 * 
 * This descriptor is set up to transfer from memory to UART 2 bytes at a time. 
 * Notice, that destination address is USARTn->TXDOUBLE, which is a two byte 
 * FIFO in the UART transfer area. Destination address is not incremented.
 * 
 * Transfer unit size (.xfer.size) is half-word (because ADC data is 16 bits).
 *
 * Transfer block size (.xfer.blockSize) is 1, which means one unit of data is
 * transferred during a block. LDMA does not arbitrate during the transfer of 
 * a block of data, i.e. other LDMA channels can't use LDMA while block of data
 * is being serviced. Only after block transfer has completed will LDMA check 
 * if other LDMA channels need servicing.
 *
 * Transfer count (.xfer.xferCnt) is the number of units to be transferred 
 * within a descriptor.
 *
 * Byte swap is used, because ADC readings seem to be in little-endian order in
 * the gecko memory. 
 *
 * The first descriptor uses absolute adderssing mode for source address. This 
 * is a requirement. The second descriptor uses relative adderssing mode for 
 * source address. This means that the second descriptors source address is
 * the address of the first descriptors last transfer (eg. if first descriptor
 * source address was 0x00001111 and the descriptor transferred 40 half words
 * then the address of the last transfer would be something like 0x00001125, 
 * since memory area is 32bit and 2 half words occupy one memory slot so the 
 * initial address gets incremented 20 memory slots). Since source address
 * increment is used, the last address actually becomes the address of the
 * next data value to be transferred (0x00001127) and 
 * no offset is needed (.xfer.srcAddr = 0).
 **/
void serialLDMAStart(uint32_t* bufAddr)
{
	//create descriptor
	memToUartLoopDsc[0].xfer.structType	= ldmaCtrlStructTypeXfer;
	memToUartLoopDsc[0].xfer.structReq	= 0; //transfer started by USART signal, not descr. load
	memToUartLoopDsc[0].xfer.xferCnt	= 2047; //2047 cuz DMA xferCnt register is 11 bits
	memToUartLoopDsc[0].xfer.byteSwap	= 1; //ADC readings in memory seem to be in little-endian order, so swap bytes 
	memToUartLoopDsc[0].xfer.blockSize	= ldmaCtrlBlockSizeUnit1; //smallest block so as not to starve other DMA channels 
	memToUartLoopDsc[0].xfer.doneIfs	= 0; //no interrupt after descriptor is done
	memToUartLoopDsc[0].xfer.reqMode	= ldmaCtrlReqModeBlock;//recommended for peripheral transfer
	memToUartLoopDsc[0].xfer.decLoopCnt	= 0; //first descriptor is not looped
	memToUartLoopDsc[0].xfer.ignoreSrec	= 1; //page 519 efr32xg1 reference manual r1.1
	memToUartLoopDsc[0].xfer.srcInc		= ldmaCtrlSrcIncOne;
	memToUartLoopDsc[0].xfer.size		= ldmaCtrlSizeHalf; //transfer 2 bytes at a time (half-word), cuz we want to use byte swap
	memToUartLoopDsc[0].xfer.dstInc		= ldmaCtrlDstIncNone; //don't increment UART TX buffer
	memToUartLoopDsc[0].xfer.srcAddrMode	= ldmaCtrlSrcAddrModeAbs;
	memToUartLoopDsc[0].xfer.dstAddrMode	= ldmaCtrlDstAddrModeAbs;
	memToUartLoopDsc[0].xfer.srcAddr	= (uint32_t)bufAddr; //memory address
	memToUartLoopDsc[0].xfer.dstAddr	= (uint32_t)&USART0->TXDOUBLE; //UART TX buffer address for half-word data
	memToUartLoopDsc[0].xfer.linkAddr	= 4; //point to next
	memToUartLoopDsc[0].xfer.linkMode	= ldmaLinkModeRel;
	memToUartLoopDsc[0].xfer.link		= 1; //link to next

	memToUartLoopDsc[1].xfer.structType	= ldmaCtrlStructTypeXfer;
	memToUartLoopDsc[1].xfer.structReq	= 0; //transfer started by USART signal, not descr. load
	memToUartLoopDsc[1].xfer.xferCnt	= 2047; //2047 cuz DMA xferCnt register is 11 bits
	memToUartLoopDsc[1].xfer.byteSwap	= 1; //ADC readings in memory seem to be in little-endian order, so swap bytes 
	memToUartLoopDsc[1].xfer.blockSize	= ldmaCtrlBlockSizeUnit1; //smallest block so as not to starve other DMA channels 
	memToUartLoopDsc[1].xfer.doneIfs	= 0; //no interrupt after descriptor is done
	memToUartLoopDsc[1].xfer.reqMode	= ldmaCtrlReqModeBlock;//recommended for peripheral transfer
	memToUartLoopDsc[1].xfer.decLoopCnt	= 1; //cuz using looping
	memToUartLoopDsc[1].xfer.ignoreSrec	= 1; //page 519 efr32xg1 reference manual r1.1
	memToUartLoopDsc[1].xfer.srcInc		= ldmaCtrlSrcIncOne;
	memToUartLoopDsc[1].xfer.size		= ldmaCtrlSizeHalf; //transfer 2 bytes at a time (half-word), cuz we want to use byte swap
	memToUartLoopDsc[1].xfer.dstInc		= ldmaCtrlDstIncNone; //don't increment UART TX buffer
	memToUartLoopDsc[1].xfer.srcAddrMode	= ldmaCtrlSrcAddrModeRel;
	memToUartLoopDsc[1].xfer.dstAddrMode	= ldmaCtrlDstAddrModeAbs;
	memToUartLoopDsc[1].xfer.srcAddr	= 0; //offset from source address of previous transfer
	memToUartLoopDsc[1].xfer.dstAddr	= (uint32_t)&USART0->TXDOUBLE; //UART TX buffer address for half-word data
	memToUartLoopDsc[1].xfer.linkAddr	= 0; //point to ourself
	memToUartLoopDsc[1].xfer.linkMode	= ldmaLinkModeRel;
	memToUartLoopDsc[1].xfer.link		= 0; //stop after loop, generates an interrupt

	//start LDMA transfer
	uartLDMAstart(&memToUartLoopDsc[0]);
}

/**
 * NB! This won't work as intended because of absolute addressing mode for 
 * source adderss!! (but other parameters should be ok, so still saving this
 * as a template for future use)
 * 
 * This is a loop descriptor, looping to itself. No interrupts are generated
 * when a descriptor in the loop finishes and there are more iterations of the
 * loop left. An interrupt is generated only when the last descriptor in the
 * loop is finished. NB! LDMA must be configured for loop transfer, this is 
 * done elsewhere.
 *
 * This descriptor is set up to transfer from memory to UART 1 byte at a time. 
 * Notice, that destination address is USARTn->TXDATA, which is a single byte 
 * FIFO in the UART transfer area. 
 *
 * Byte swap does not work for transfer of 1 byte at a time.
 *
void serialLDMAStart(uint32_t* bufAddr)
{
	//create descriptor
	memToUartLoopDsc.xfer.structType	= ldmaCtrlStructTypeXfer;
	memToUartLoopDsc.xfer.structReq		= 0; //transfer started by USART signal, not descr. load
	memToUartLoopDsc.xfer.xferCnt		= 2047; //2047 cuz DMA xferCnt register is 11 bits
	memToUartLoopDsc.xfer.byteSwap		= 0;
	memToUartLoopDsc.xfer.blockSize		= ldmaCtrlBlockSizeUnit1; //smallest block so as not to starve other DMA channels 
	memToUartLoopDsc.xfer.doneIfs		= 0; //no interrupt after descriptor is done
	memToUartLoopDsc.xfer.reqMode		= ldmaCtrlReqModeBlock;//recommended for peripheral transfer
	memToUartLoopDsc.xfer.decLoopCnt	= 1; //cuz using looping
	memToUartLoopDsc.xfer.ignoreSrec	= 1; //page 519 efr32xg1 reference manual r1.1
	memToUartLoopDsc.xfer.srcInc		= ldmaCtrlSrcIncOne;
	memToUartLoopDsc.xfer.size		= ldmaCtrlSizeByte; //transfer 2 bytes at a time (half-word), cuz we want to use byte swap
	memToUartLoopDsc.xfer.dstInc		= ldmaCtrlDstIncNone; //don't increment UART TX buffer
	memToUartLoopDsc.xfer.srcAddrMode	= ldmaCtrlSrcAddrModeAbs;
	memToUartLoopDsc.xfer.dstAddrMode	= ldmaCtrlDstAddrModeAbs;
	memToUartLoopDsc.xfer.srcAddr		= (uint32_t)bufAddr; //memory address
	memToUartLoopDsc.xfer.dstAddr		= (uint32_t)&USART0->TXDATA; //UART TX buffer address
	memToUartLoopDsc.xfer.linkAddr		= 0; //point to ourself
	memToUartLoopDsc.xfer.linkMode		= ldmaLinkModeRel;
	memToUartLoopDsc.xfer.link		= 0; //stop after loop, generates an interrupt

	//start LDMA transfer
	uartLDMAstart(&memToUartLoopDsc);
}
*/

/**
 * NB! This won't work as intended because of absolute addressing mode for 
 * source adderss!! (but other parameters should be ok, so still saving this
 * as a template for future use)
 *
 * NB! This descriptor has never been used or tested. Just created a templet 
 * for future use, since everything conserning LDMA is still fresh in my mind.
 * 
 * This is a loop descriptor, looping to itself. No interrupts are generated
 * when a descriptor in the loop finishes and there are more iterations of the
 * loop left. An interrupt is generated only when the last descriptor in the
 * loop is finished. NB! LDMA must be configured for loop transfer, this is 
 * done elsewhere.
 * This descriptor is set up to transfer from memory to UART 4 bytes at a time. 
 * Notice, that destination address is USARTn->TXDOUBLE, which is a two byte 
 * FIFO in the UART transfer area. 
 *
 * This descriptor could be used to transfer float values, but I don't know how
 * the endianness will turn out in that case. Some tweaking may need to be done.
 *
void serialLDMAStart(uint32_t* bufAddr)
{
	//create descriptor
	memToUartLoopDsc.xfer.structType	= ldmaCtrlStructTypeXfer;
	memToUartLoopDsc.xfer.structReq		= 0; //transfer started by USART signal, not descr. load
	memToUartLoopDsc.xfer.xferCnt		= 2047; //2047 cuz DMA xferCnt register is 11 bits
	memToUartLoopDsc.xfer.byteSwap		= 1;
	memToUartLoopDsc.xfer.blockSize		= ldmaCtrlBlockSizeUnit1; //smallest block so as not to starve other DMA channels 
	memToUartLoopDsc.xfer.doneIfs		= 0; //no interrupt after descriptor is done
	memToUartLoopDsc.xfer.reqMode		= ldmaCtrlReqModeBlock;//recommended for peripheral transfer
	memToUartLoopDsc.xfer.decLoopCnt	= 1; //cuz using looping
	memToUartLoopDsc.xfer.ignoreSrec	= 1; //page 519 efr32xg1 reference manual r1.1
	memToUartLoopDsc.xfer.srcInc		= ldmaCtrlSrcIncOne;
	memToUartLoopDsc.xfer.size		= ldmaCtrlSizeHalf; //transfer 2 bytes at a time (half-word), cuz UART FIFO is only 2 bytes
	memToUartLoopDsc.xfer.dstInc		= ldmaCtrlDstIncNone; //don't increment UART TX buffer
	memToUartLoopDsc.xfer.srcAddrMode	= ldmaCtrlSrcAddrModeAbs;
	memToUartLoopDsc.xfer.dstAddrMode	= ldmaCtrlDstAddrModeAbs;
	memToUartLoopDsc.xfer.srcAddr		= (uint32_t)bufAddr; //memory address
	memToUartLoopDsc.xfer.dstAddr		= (uint32_t)&USART0->TXDOUBLE; //UART TX buffer address
	memToUartLoopDsc.xfer.linkAddr		= 0; //point to ourself
	memToUartLoopDsc.xfer.linkMode		= ldmaLinkModeRel;
	memToUartLoopDsc.xfer.link		= 0; //stop after loop, generates an interrupt

	//start LDMA transfer
	uartLDMAstart(&memToUartLoopDsc);
}
*/
