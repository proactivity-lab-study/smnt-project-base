#ifndef SERIAL_LDMA_H
#define SERIAL_LDMA_H

//public functions
void serialLDMAIrq(void);
void serialLDMAStart(uint32_t* bufAddr);
uint8_t serialLDMADone(); //callback to adc_print

#endif //SERIAL_LDMA_H
