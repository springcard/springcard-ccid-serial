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
 * @file pcsc-serial-sample-main.c
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

#if (defined(WIN32))
/* Our reference reader uses COM5 */
const char* szCommDevice = "COM5";
#define sleep_ms(x) Sleep(x)
#endif
#if (defined(__linux))
/* Our reference reader uses /dev/ttyS5 */
const char* szCommDevice = "/dev/ttyS5";
#define sleep_ms(x) usleep(1000 * x)
#endif

static BOOL parse_args(int argc, char** argv);

int main(int argc, char** argv)
{
	LONG rc;

	printf("\n");
	printf("SpringCard SDK for PC/SC Serial : demo for Windows and Linux\n");
	printf("------------------------------------------------------------\n");
	printf("Copyright (c) 2015-2023 SPRINGCARD SAS, FRANCE - www.springcard.com\n");
	printf("See LICENSE.txt for disclaimer and license requirements\n");
	printf("\n");

	if (!parse_args(argc, argv))
	{
		printf("Usage:\n");
		printf("\tpcsc-serial-windows-demo [-d <COMM PORT>] [-i] [-c] [-t] [-v]\n");
		printf("\t\t-d <COM PORT>: select the comm. device (default is COM5)\n");
		printf("\t\t-i: use notifications (Interrupt endpoint)\n");
		printf("\t\t-c: run ECHO test over SCardControl\n");
		printf("\t\t-t: run ECHO test over SCardTransmit\n");
		printf("\t\t-v: verbose output\n");
		return -1;
	}

	printf("Using communication device: %s\n", szCommDevice);

	/* Prepare the underlying hardware and lower layer software */
	/* -------------------------------------------------------- */

	CCID_LIB(SerialInit)(szCommDevice);

	printf("\n\n***** Now running forever, hit Ctrl+C to exit ****\n\n");

	for (;;)
	{
		/* Try and open the serial communication layer */
		/* ------------------------------------------- */
		printf("Opening the serial port\n");
		if (!CCID_LIB(SerialOpen)())
		{
			printf("Failed to open serial port %s\n", szCommDevice);
			sleep_ms(1000);
			continue;
		}

		/* Initialize the CCID driver */
		/* -------------------------- */

		printf("Initializing the CCID driver\n");
		CCID_LIB(Init)();

		/* Connect or reconnect to the device */
		/* ---------------------------------- */

		printf("Pinging the device (if some)\n");
		rc = CCID_LIB(Ping)();
		if (rc != SCARD_ERR(S_SUCCESS))
		{
			printf("No device found on port %s (rc=%lX)\n", szCommDevice, rc);
			/* Close the serial port */
			CCID_LIB(SerialClose)();
			/* In case of a communication error, we shall wait at least 1200ms so the device may reset its state machine */
			sleep_ms(1200);
			/* Try again */
			continue;
		}

		printf("*** It seems that we've found a device... ***\n");

		/* Initialize the PC/SC-like stack */
		/* ------------------------------- */

		printf("Initializing the PC/SC-Like stack\n");
		SCARD_LIB(Init)();
		
		/* Run the sample */
		/* --------------- */

		sample();

		/* The sample is a loop, if we come back here, this means that we have lost the device */
		/* ----------------------------------------------------------------------------------- */
		printf("*** It seems that we've lost the device... ***\n");

		/* Close the serial port */
		printf("Closing the serial port\n");
		CCID_LIB(SerialClose)();
	}
	
	return 0;
}


static BOOL parse_args(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{ // Start at 1 because argv[0] is the program name
		if (argv[i][0] == '-')
		{ // This is an option
			if (!strcmp(argv[i], "-d") && i + 1 < argc)
			{
				szCommDevice = argv[i + 1];
				i++;  // Skip next item since we just processed it
			}
			else if (!strcmp(argv[i], "-i"))
			{
				fCcidUseNotifications = TRUE;
			}
			else if (!strcmp(argv[i], "-c"))
			{
				fTestEchoControl = TRUE;
			}			
			else if (!strcmp(argv[i], "-t"))
			{
				fTestEchoTransmit = TRUE;
			}
			else if (!strcmp(argv[i], "-v"))
			{
				fVerbose = TRUE;
			}
			else if (!strcmp(argv[i], "-h"))
			{
				/* Return FALSE to display the usage message */
				return FALSE;
			}			
			else
			{
				printf("Unknown option: %s\n", argv[i]);
				return FALSE;
			}
		}
		else
		{
			// It's not an option, maybe a standalone argument
			printf("Unsupported argument: %s\n", argv[i]);
			return FALSE;
		}
	}

	return TRUE;
}
