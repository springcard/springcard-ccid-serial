# springcard-ccid-serial

PC/SC-Like library and CCID "driver" for PC/SC couplers over a Serial link

## Introduction

PC/SC is the de-facto standard to make an application running on a high-end computer (say, a PC) interact with a smart card (SC).

SpringCard offers numerous USB products that comes with a PC/SC driver for desktop and server operating systems.
SpringCard also offers products that use another communication channel as their primary interface: Bluetooth, Ethernet (network), or Serial.

Serial devices namely target low-end, OS-less systems, where USB is not available.
On such targets, there is of course no PC/SC stack, no support for plug'n'play drivers.
The goal of this project is to mimic the standard PC/SC stack (hence the name "PC/SC-Like") and provide a minimal, Serial-based CCID driver, to provide a seamless experience for developers who are familiar with PC/SC and/or with the development of a smart card-aware application, and are integrating a SpringCard Serial PC/SC coupler in their embedded system.

## Compliant products

### M519 family

The M519 and all its derivative products (for instance M519-SUV) are supported.

In particular, the M519-SRK (Starter Kit) is supported, and is our development and validation platform. Examples and documentations will refer to this SRK.

### K663 family

The K663 and all its derivative products (for instance K663-TTL) are supported, provided that they firmware version is >= 2.02. Earlier versions of K663 could be upgraded.

K531 and K632 are not supported.

## API Reference

The .html files within the /docs folder are your primary source of information.

## Sample

The /src/sample/pcsc-serial-sample.c has all you need.

## Porting the library to your MCU

Use the /src/hal/skel/hal_skel.c file as reference.

Your hardware-abstraction layer (HAL) must provide 

### Serial communication with the K663.

The serial_init function shall configure the UART (38400bps, 8 data bits, 1 stop bit, no parity, no flow control).

The serial_send_byte and serial_send_bytes are used to transmit (TX) from the MCU to the K663.

Receiving (RX) must be done in an ISR. The ISR shall call serial_recv_callback for every byte that comes from the K663 to the MCU.
  
### Wait for the end of the communication.

ccid_recv_wait will be called in the context of the main task, and shall block until the ISR calls ccid_recv_wakeupFromISR (this is done by serial_recv_callback).

If you don't have an OS, just use a volatile BOOL to do so.

Under a multitasking system, you must use a 'Event' or 'Semaphore' object to implement this correctly.

Under FreeRTOS (http://www.freertos.org) you will typically use a Binary Semaphore as follow:
- ccid_recv_wait blocks using xSemaphoreTake
- ccid_recv_wakeupFromISR calls xSemaphoreGiveFromISR
- you may create the semaphore in the platform_init function.

### Provide a mean to wait for a specified time, from 1 to 10000ms. This is the role of sleep_ms.

## License

This library is Copyright (c) 2015 SPRINGCARD SAS, FRANCE - http://www.springcard.com

Permission is given to embed it in your own MCU (or even PC-based) projects, provided that this project is always used in conjunction with a genuine SpringCard product.

Please read the LICENSE.txt file for details.

