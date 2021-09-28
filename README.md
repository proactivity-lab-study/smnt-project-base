
# Mic-stream-to-serial

SmENeTe SM4 sensor microphone is sampled continuously by ADC, the data stream is temporarily bufferd in uC memory and then sent to serial (UART). LDMA handles data transfer from ADC to memory and from memory to UART. 

This application should also work on SmENeTe SM2 platform (mic-array) out of the box. Only one microphone (right) is sampled by the ADC though. 

# Usage

Before you can build your application you need to make changes to the Makefile. SILABS_SDKDIR must point to the location of your SiLabs SDK directory. Also add all new source files and headers to the make file, so they get built. Compile your binary with the command:

`make smnt-mb`

