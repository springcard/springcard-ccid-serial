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

The `.html` files within the `/docs` folder are your primary source of information.

## Sample

The `/src/sample/pcsc-serial-sample.c` has all you need.

## Porting the library to your MCU

Use the `/src/hal/skel/hal_skel.c` file as reference.

Your hardware-abstraction layer (HAL) must provide the following features:

### Serial communication with the module.

The `CCID_SerialOpen` function shall configure the UART (38400bps, 8 data bits, 1 stop bit, no parity, no flow control).

The `CCID_SerialSendByte` and `CCID_SerialSendBytes` are used to transmit (TX) from the MCU to the module.

Receiving (RX) must be done in an ISR. The ISR shall call `CCID_SerialRecvByteFromISR` for every byte that comes from the module to the MCU.

### Wait for the end of the communication.

`CCID_WaitWakeup` will be called in the context of the main task, and shall block until `CCID_SerialRecvByteFromISR` has called `CCID_WakeupFromISR`.

You must implement `CCID_WaitWakeup` and `CCID_WakeupFromISR` to provide this behaviour.

If you don't have an OS, a simple flag (`volatile BOOL`) does the job easily.

Under a multi-task/multi-thread kernel, you must use a 'Event' or 'Semaphore' object to implement this correctly.

Under FreeRTOS (http://www.freertos.org) you could typically use a Binary Semaphore as follow:
- `CCID_WaitWakeup` blocks using `xSemaphoreTake`
- `CCID_WakeupFromISR` calls `xSemaphoreGiveFromISR` to unblock.

### Manage delays

The libraries expects to have a function named `sleep_ms` to wait for the specified number of milliseconds.

## License

This library is Copyright (c) 2015-2023 SPRINGCARD SAS, FRANCE - http://www.springcard.com

Permission is given to embed it in your own MCU (or even PC-based) projects, provided that this project is always used in conjunction with a genuine SpringCard product.

Please read the LICENSE.txt file for details.

