/*

 This code is Copyright (c) 2015-2023 SPRINGCARD SAS, FRANCE - www.springcard.com

 Redistribution and use in source (source code) and binary (object code)
  forms, with or without modification, are permitted provided that the
  following conditions are met :
  1. Redistributed source code or object code must be used only in conjunction
	 with a genuine SpringCard product,
  2. Redistributed source code must retain the above copyright notice, this
	 list of conditions and the disclaimer below,
  3. Redistributed object code must reproduce the above copyright notice,
	 this list of conditions and the disclaimer below in the documentation
	 and/or other materials provided with the distribution,
  4. The name of SpringCard may not be used to endorse or promote products
	 derived from this software or in any other form without specific prior
	 written permission from SpringCard,
  5. Redistribution of any modified code must be labeled
	"Code derived from original SpringCard copyrighted source code".

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
  PARTICULAR PURPOSE.
  SPRINGCARD SHALL NOT BE LIABLE FOR INFRINGEMENTS OF THIRD PARTIES RIGHTS
  BASED ON THIS SOFTWARE. ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. SPRINGCARD DOES NOT WARRANT THAT THE
  FUNCTIONS CONTAINED IN THIS SOFTWARE WILL MEET THE USER'S REQUIREMENTS OR
  THAT THE OPERATION OF IT WILL BE UNINTERRUPTED OR ERROR-FREE. IN NO EVENT,
  UNLESS REQUIRED BY APPLICABLE LAW, SHALL SPRINGCARD BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. ALSO, SPRINGCARD IS UNDER
  NO OBLIGATION TO MAINTAIN, CORRECT, UPDATE, CHANGE, MODIFY, OR OTHERWISE
  SUPPORT THIS SOFTWARE.
*/

/**
 * @file pcsc-serial-sample-main-rpi-pico.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Source code of the sample application (main function of the CLI program for Linux and Windows)
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @addtogroup sample
 */
 
#include <project.h> 

#include "../../pcsc-serial.h"
#include "../../scard/scard.h"
#include "../../ccid/ccid.h"
#include "../../ccid/ccid_hal.h"

void sample(void);

/* Are we on a Raspberry Pico W? */
bool has_wifi = false;
bool sample_running = false;

/* Local functions */
static void execute_command(char command);
static void print_banner(void);
static void print_menu(void);
static void platform_init(void);
static void set_led(bool onoff);

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"


/* This is the LED pin on Raspberry Pico */
/* On Raspberry Pico W, the LED pin is connected to the WiFi chip, not to the MCU itself */

int main(void)
{
	LONG rc;

	platform_init();

    while (1)
	{
		int ch = getchar_timeout_us(100);
		if (ch != PICO_ERROR_TIMEOUT)
		{
			execute_command(ch);
		}
		
		if (sample_running)
		{
			printf("Looking for CCID device through the UART...\n");
			CCID_LIB(Init)();
			rc = CCID_LIB(Ping)();
			if (rc != SCARD_ERR(S_SUCCESS))
			{
				printf("No device found on UART (rc=%lX), check hardware and try again\n", rc);
			}
			else
			{
				printf("CCID device found, starting the sample (use 's' to stop)\n");				
				SCARD_LIB(Init)();
				sample();
				if (sample_running)
				{
					/* The sample has exited spontaneously */
					printf("The sample has exited after a fatal error\n");
				}
				else
				{
					/* User has terminated the sample */
					printf("Sample stopped (use 'r' to restart)\n");
				}
			}
			sample_running = FALSE;		
		}
    }
}

BOOL SCARD_LIB(IsCancelledHook)(void)
{
	/* Keep receiving on USB to be able to stop the sample */
	int ch = getchar_timeout_us(0);
	if (ch == 's')
	{
		/* Stop --> Cancelled by user */
		sample_running = FALSE;
	}
	return !sample_running;
}

static const char *ENABLED_STRING = "ENABLED";
static const char *DISABLED_STRING = "DISABLED";

static void execute_command(char command)
{
	if (sample_running)
	{
		switch (command)
		{
			case 's' :
				sample_running = false;			
			break;
			default:
				printf("Sample is running (use 's' to stop)\n");
		}
	}
	else
	{
		switch (command)
		{
			case 'i' :
				fCcidUseNotifications = !fCcidUseNotifications;
				printf("Notifications (Interrupt endpoint) are %s\n", fCcidUseNotifications ? ENABLED_STRING : DISABLED_STRING);
			break;
			case 'c' :
				fTestEchoControl = !fTestEchoControl;
				printf("ECHO test over SCardControl is %s\n", fTestEchoControl ? ENABLED_STRING : DISABLED_STRING);
			break;
			case 't' :
				fTestEchoTransmit = !fTestEchoTransmit;
				printf("ECHO test over SCardTransmit is %s\n", fTestEchoTransmit ? ENABLED_STRING : DISABLED_STRING);
			break;
			case 'v' :
				fVerbose = !fVerbose;
				printf("Verbose output is %s\n", fVerbose ? ENABLED_STRING : DISABLED_STRING);
			break;				
			case 'r':
				sample_running = true;
			break;		
			case 's':
				printf("Sample is not running ('r' to start)\n");
			break;
			case 'h' :
			case '?' :
			default  :
				print_banner();
				print_menu();
		}	
	}
}

static void print_banner(void)
{
	printf("\n");
	printf("SpringCard SDK for PC/SC Serial : demo for Raspberry Pico\n");
	printf("---------------------------------------------------------\n");
	printf("Copyright (c) 2015-2023 SPRINGCARD SAS, FRANCE - www.springcard.com\n");
	printf("See LICENSE.txt for disclaimer and license requirements\n");
	printf("\n");	
}

static void print_menu(void)
{
	printf("\ti: enable/disable notifications (Interrupt endpoint) [%s]\n", fCcidUseNotifications ? ENABLED_STRING : DISABLED_STRING);
	printf("\tc: enable/disable ECHO test over SCardControl [%s]\n", fTestEchoControl ? ENABLED_STRING : DISABLED_STRING);
	printf("\tt: enable/disable ECHO test over SCardTransmit [%s]\n", fTestEchoTransmit ? ENABLED_STRING : DISABLED_STRING);
	printf("\tv: enable/disable verbose output [%s]\n", fVerbose ? ENABLED_STRING : DISABLED_STRING);	
	printf("\tr: start running the sample\n");
	printf("\ts: stop running the sample\n");
}

// RX interrupt handler
static void on_uart_rx()
{
    while (uart_is_readable(UART_ID))
	{		
        uint8_t ch = uart_getc(UART_ID);
		CCID_LIB(SerialRecvByteFromISR)(ch);
    }
}


static void platform_init(void)
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
}

static void set_led(bool onoff)
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

