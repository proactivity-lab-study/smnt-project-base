UART serial pins are defined/routed/retargeted in ../Drivers/retargetserialconfig.h
UART speed (baud rate) can be changed in ../Drivers/retargetserial.c (line 113); baud rate 460800 has been tested to work, be sure that the PC is also set at the higher baud rate

ADC configuration is done in ../Drivers/Microphones.c and ../Drivers/Microphones.h

Don't use printf functions together with LDMA UART operations, cuz this seems to mess things up sometimes (printf buffers transmissions and flushes them later and can interfere LDMA UART transmissions)

Definitely check out ./tests/README.txt for information on problems with LDMA ADC and UART.

There is noise at the beginning of every ADC measurment frame (~1sec, 10240 samlpes at 10kHz). There is no noise at the beginning of the very first frame right after boot. Noise length is ~200msec. Noise seems to be related to LDMA UART operation, because LDMA UART starts writing the first measured frame to UART at the beginning of the measurement of the second frame (and so on with every new frame). With current parameters (921600 baud rate) LDMA UART operation must take at least 178ms and oscilloscope measurements confirm this to take ~220ms including LDMA and UART overhead operations. 
Wall socket power does not seem to affect the volume of noise, however usage of the USB hub (together with programming board and read-out board) seemed to dampen the noise. This is peculiar.
Noise is audiable when playing back the measured signals through speakers.

At the moment my conclusion is that this is more of a hardware problem. UART usage at the beginning of every frame opens a channel for noise to affect the uC. A test with an electric decoupler-USB thingy is necessary. Alternative is that this is a gecko chip problem that the ADC gets affected when LDMA or UART are in use at the same time (but I think this is less likely). 
