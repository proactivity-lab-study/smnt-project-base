
# Project base for smnt-mb devices
This project provides the basic configuration (drivers, configuration logic, etc.) for programming the smnt-mb devices.

# Usage
To start development first get all the submodules and then create your own branch. To name your branch use the naming convention 'your-full-name-application-name' (but try to keep it short also). Then start development in your branch.

Directory structure: 
- all new source files go in /src
- all new header files go in /inc
- your test application goes in /demo/app-name
- all additionally required source code goes in /zoo

Before you can build your application you need to make changes to the Makefile. SILABS_SDKDIR must point to the location of your SiLabs SDK directory. Also add all new source files and headers to the make file, so they get built. Compile your binary with the command:

`make smnt-mb`

## Dependencies
The library runs on EFR32 Series 1 microcontrollers, uses the
[Thinnect node platform](https://github.com/thinnect/node-apps) and the
[adc_dmadrv software module](https://github.com/proactivity-lab/dmadrv-adc).


