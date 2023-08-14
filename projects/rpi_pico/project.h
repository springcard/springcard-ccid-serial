#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"

#define UART_ID uart0

#define BAUD_RATE 38400
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define LED_PIN 25

typedef bool BOOL;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef long LONG;

#if (!defined(TRUE))
	#define TRUE 1
#endif

#if (!defined(FALSE))
	#define FALSE 0
#endif

#endif