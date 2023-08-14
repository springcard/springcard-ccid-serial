#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"

/* From https://github.com/raspberrypi/pico-examples/blob/master/uart/uart_advanced/uart_advanced.c */

#define UART_ID uart0
#define BAUD_RATE 38400
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

/* Are we on a Raspberry Pico W? */
bool has_wifi = false;

uint8_t uart_rx_queue[8];
volatile int uart_rx_queue_push;
volatile int uart_rx_queue_pop;

// RX interrupt handler
void on_uart_rx()
{
    while (uart_is_readable(UART_ID))
	{		
        uint8_t ch = uart_getc(UART_ID);
		if (uart_rx_queue_push >= sizeof(uart_rx_queue))
			uart_rx_queue_push = 0;
		uart_rx_queue[uart_rx_queue_push++] = ch;
    }
}

/* This is the LED pin on Raspberry Pico */
/* On Raspberry Pico W, the LED pin is connected to the WiFi chip, not to the MCU itself */
const uint LED_PIN = 25;

void set_led(bool onoff)
{
	if (onoff)
	{
		if (has_wifi)
			cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
		else
			gpio_put(LED_PIN, 0);
	}
	else
	{
		if (has_wifi)
			cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
		else
			gpio_put(LED_PIN, 1);
	}
}

int main(void)
{

	
	/* Initialize the libraries */
    stdio_init_all();

	/* Say hello */
	printf("This is SpringCard ccid-serial for Raspberry Pico / Raspberry Pico W\n");
	
	/* For Raspberry Pico W: find the WiFi chip */
	if (cyw43_arch_init() == 0)
	{
        printf("WiFi init success, this must be a Raspberry Pico W\n");
		has_wifi = true;
    }
	else
	{
        printf("WiFi init failed, this must be a Raspberry Pico\n");		
	}

	if (!has_wifi)
	{
		/* Initialize the hardware for the Raspberry Pico */
		gpio_init(LED_PIN);
		gpio_set_dir(LED_PIN, GPIO_OUT);
	}
	else
	{
		/* TODO: Initialize the hardware for the Raspberry Pico W */
	}
	
	/* Preparing... */
	set_led(true);
	
    /* Set up the UART */
    uart_init(UART_ID, BAUD_RATE);	
	
    /* Set the TX and RX pins by using the function select on the GPIO */
    /* Set datasheet for more information on function select */
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

	/* Confirm the baudrate */
	uart_set_baudrate(UART_ID, BAUD_RATE);
	
	/* Set UART flow control CTS/RTS, we don't want these, so turn them off */
    uart_set_hw_flow(UART_ID, false, false);

    /* Set our data format */
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    /* Turn off FIFO's - we want to do this character by character */
    uart_set_fifo_enabled(UART_ID, false);
	
    /* Set up a RX interrupt */
    /* We need to set up the handler first */
    /* Select correct interrupt for the UART we are using */
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

	/* And set up and enable the interrupt handlers */
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
	
    /* Now enable the UART to send interrupts - RX only */
    uart_set_irq_enables(UART_ID, true, false);	

	/* Ready... */
	set_led(false);

    while (1)
	{
		/* Simple test: copy flow fropm UART to USB */
		while (uart_rx_queue_pop != uart_rx_queue_push)
		{
			uint8_t ch;
			set_led(true);
			if (uart_rx_queue_pop >= sizeof(uart_rx_queue_pop))
				uart_rx_queue_pop = 0;
			ch = uart_rx_queue[uart_rx_queue_pop++];
			printf("%c", ch);
			set_led(false);
		}
    }
}