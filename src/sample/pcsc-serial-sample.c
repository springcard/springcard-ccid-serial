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
 * @file pcsc-serial-sample.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Source code of the sample application (common part)
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @defgroup sample Sample application
 *
 * The detailed documentation is provided will be provided in an application note.
 */

#include <project.h>

#include "../pcsc-serial.h"
#include "../scard/scard.h"
#include "../ccid/ccid.h"

BOOL fCcidUseNotifications = FALSE;
BOOL fTestEchoControl = FALSE;
BOOL fTestEchoTransmit = FALSE;
BOOL fVerbose = FALSE;
BOOL fSampleMustExit = FALSE;

static BYTE abInOutBuffer[4 + 1 + 255 + 1]; /* Max size of a short APDU */

static const BYTE APDU_GET_UID[] = { 0xFF, 0xCA, 0x00, 0x00, 0x00 };

static void sample_on_slot(BYTE slot);
static WORD get_status_word(BYTE abRecvApdu[], DWORD wRecvLength);

static BOOL wait_status_change(DWORD timeout);

static BOOL start_pcsc(void);
static BOOL stop_pcsc(void);
static BOOL ping_device(void);
static void dump_device_descriptor(void);
static void dump_configuration_descriptor(void);
static void dump_interface_descriptor(void);
static void dump_string_descriptors(void);
static BYTE get_slot_count(void);
static BOOL card_is_present(BYTE bSlot);
static BOOL test_echo_control(void);
static BOOL test_echo_transmit(BYTE slot);

void sample(void)
{
	BYTE slotCount;
	BOOL cardPresent[CCID_MAX_SLOT_COUNT] = { 0 };

	printf("Running sample application with the found CCID device...\n");

	/* Verify we can ping the device */
	if (!ping_device())
		goto done;

	/* Retrieve and print the device descriptor */
	dump_device_descriptor();
	if (!SCARD_LIB(IsValidContext)())
		goto done;

	/* Retrieve and print the configuration descriptor */
	dump_configuration_descriptor();
	if (!SCARD_LIB(IsValidContext)())
		goto done;

	/* Retrieve and print all the string descriptors */
	dump_string_descriptors();
	if (!SCARD_LIB(IsValidContext)())
		goto done;

	/* Retrieve and print the interface descriptor */
	dump_interface_descriptor();
	if (!SCARD_LIB(IsValidContext)())
		goto done;
	
	/* Activate the configuration */
	printf("Starting PC/SC...\n");
	if (!start_pcsc())
		goto done;

	/* Verify we still can ping the device */
	if (!ping_device())
		goto done;
	
	/* Get the slot count */
	printf("Reading slot count\n");
	slotCount = get_slot_count();
	if (slotCount == 0)
		goto done;
	
	printf("Device has %d slot(s)\n", slotCount);
	if (slotCount > CCID_MAX_SLOT_COUNT)
	{
		printf("WARNING: device has %d card slots, but we can support only %d\n", slotCount, CCID_MAX_SLOT_COUNT);
		slotCount = CCID_MAX_SLOT_COUNT;		
	}

	if (fTestEchoControl && SCARD_LIB(IsValidContext)())
	{
		printf("Please wait during the test procedure (echo over SCardControl)...\n");
		if (!test_echo_control())
		{
			printf("Test failed!\n");
			goto done;
		}
	}

	printf("Insert a card in any slot...\n");

	/* Run while the device is up and running */
	while (SCARD_LIB(IsValidContext)())
	{
		BYTE slot;

		if (fCcidUseNotifications)
		{
			/* Wait until a notification arrives */
			printf("Waiting for a change");

			/* Wait forever */
			wait_status_change((DWORD)-1); /* Use -1 for INFINITE timeout */
			if (!SCARD_LIB(IsValidContext)())
				goto done;

			/* Verify we still can ping the device */
			if (!ping_device())
				goto done;
		}

		/* Find the slot(s) were a card has been inserted */
		for (slot = 0; slot < slotCount; slot++)
		{
			/* Ignore slots we can't support */
			if (slot >= CCID_MAX_SLOT_COUNT)
				continue;
			if (!SCARD_LIB(IsValidContext)())
				goto done;

			if (card_is_present(slot))
			{
				if (!cardPresent[slot])
				{
					/* A card has been inserted in this slot */
					cardPresent[slot] = TRUE;
					printf("Card inserted in slot %d\n", slot);

					/* Run sample over this card */
					sample_on_slot(slot);
					
					printf("Transaction with card in slot %d terminated\n", slot);
				}
			}
			else
			{
				if (cardPresent[slot])
				{
					/* The card has been removed from this slot */
					cardPresent[slot] = FALSE;
					printf("Card removed from slot %d\n", slot);
				}				
			}			
		}
	}

	/* In case we can exit... */
	printf("Stopping PC/SC...\n");
	stop_pcsc();

done:
	printf("Sample application terminated\n");
}

static void sample_on_slot(BYTE slot)
{
	LONG rc;
	DWORD dwRecvLength = sizeof(abInOutBuffer);

	/* Try and connect to the card in the slot */
	rc = SCARD_LIB(Connect)(slot, abInOutBuffer, &dwRecvLength);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to connect to the card (rc=%lX)\n", rc);
		return;
	}

	/* We've got a card */
	printf("Card connected, ATR=");
	for (DWORD i = 0; i < dwRecvLength; i++)
		printf("%02X", abInOutBuffer[i]);
	printf("\n");

	/* We use the GET DATA ( GET UID ) command APDU defined by PC/SC v2 chapter 3 for contactless cards */
	dwRecvLength = sizeof(abInOutBuffer);
	rc = SCARD_LIB(Transmit)(slot, APDU_GET_UID, sizeof(APDU_GET_UID), abInOutBuffer, &dwRecvLength);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to retrieve the UID (rc=%lX)\n", rc);
		/* This is not always a fatal error  */
		if (SCARD_LIB(IsFatalError)(rc))
			return;	
	}
	else
	{
		WORD sw = get_status_word(abInOutBuffer, dwRecvLength);

		if (sw != 0x9000)
		{
			printf("Failed to retrieve the UID (SW=%04X)\n", sw);
		}
		else
		{
			/* We got a success response */
			printf("Card UID=");
			for (DWORD i = 0; i < dwRecvLength - 2; i++)
				printf("%02X", abInOutBuffer[i]);
			printf("\n");			
		}
	}

	if (fTestEchoTransmit && SCARD_LIB(IsValidContext)())
	{
		printf("Please wait during the test (echo over SCardTransmit)...\n");
		if (!test_echo_transmit(slot))
		{
			printf("Test failed!\n");
			return;
		}
	}
		
	/* Done with the card */
	SCARD_LIB(Disconnect)(slot);
}

static WORD get_status_word(BYTE abRecvApdu[], DWORD dwRecvLength)
{
	if (abRecvApdu == NULL)
		return 0xFFFF;
	if (dwRecvLength < 2)
		return 0xFFFF;
	return (abRecvApdu[dwRecvLength - 2] * 0x0100) + abRecvApdu[dwRecvLength - 1];
}

static BOOL start_pcsc(void)
{
	LONG rc = CCID_LIB(Start)(fCcidUseNotifications);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to start PC/SC operation on the device (rc=%lX)\n", rc);
		return FALSE;
	}
	return TRUE;
}

static BOOL stop_pcsc(void)
{
	LONG rc = CCID_LIB(Stop)();
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to stop PC/SC operation on the device (rc=%lX)\n", rc);
		return FALSE;
	}
	return TRUE;
}

static BOOL wait_status_change(DWORD timeout)
{
	LONG rc = SCARD_LIB(GetStatusChange)(timeout);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		/* Timeout is a "normal" error */
		if (rc != SCARD_ERR(E_TIMEOUT))
			printf("Failed to wait for an event (rc=%lX)\n", rc);
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static BOOL ping_device(void)
{
	LONG rc = CCID_LIB(Ping)();
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to ping the device (rc=%lX)\n", rc);
		return FALSE;
	}
	return TRUE;
}

static void dump_device_descriptor(void)
{
	DWORD dwRecvLength = sizeof(abInOutBuffer);
	LONG rc = CCID_LIB(GetDescriptor)(1,0, abInOutBuffer, &dwRecvLength);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to read device's main descriptor (rc=%lX)\n", rc);		
		return;
	}

	if (dwRecvLength < 18)
	{
		printf("Wrong size for device descriptor (expected 18 bytes, got %lu)\n", dwRecvLength);		
		return;		
	}

	if ((abInOutBuffer[0] != 18) || (abInOutBuffer[1] != 1))
	{
		printf("Wrong format for device descriptor (expected header is %02X%02X, got %02X%02X)\n", 18, 1, abInOutBuffer[0], abInOutBuffer[1]);		
		return;		
	}
	
	printf("Device descriptor:\n");
    printf("\tbLength: %d\n", abInOutBuffer[0]);
    printf("\tbDescriptorType: %d\n", abInOutBuffer[1]);
    printf("\tbcdUSB: %X.%02X\n", abInOutBuffer[3], abInOutBuffer[2]);
    printf("\tbDeviceClass: %02X\n", abInOutBuffer[4]);
    printf("\tbDeviceSubClass: %02X\n", abInOutBuffer[5]);
    printf("\tbDeviceProtocol: %02X\n", abInOutBuffer[6]);
    printf("\tbMaxPacketSize0: %d\n", abInOutBuffer[7]);
    printf("\tidVendor: %02X%02X\n", abInOutBuffer[9], abInOutBuffer[8]);
    printf("\tidProduct: %02X%02X\n", abInOutBuffer[11], abInOutBuffer[10]);
    printf("\tbcdDevice: %X.%02X\n", abInOutBuffer[13], abInOutBuffer[12]);
    printf("\tiManufacturer: %d\n", abInOutBuffer[14]);
    printf("\tiProduct: %d\n", abInOutBuffer[15]);
    printf("\tiSerialNumber: %d\n", abInOutBuffer[16]);
    printf("\tbNumConfigurations: %d\n", abInOutBuffer[17]);	
	printf("\n");	
}

static void dump_configuration_descriptor(void)
{
	DWORD dwRecvLength = sizeof(abInOutBuffer);
	WORD wTotalLength;
	LONG rc = CCID_LIB(GetDescriptor)(2, 0, abInOutBuffer, &dwRecvLength);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to read device's configuration descriptor (rc=%lX)\n", rc);		
		return;
	}

	if (dwRecvLength < 9)
	{
		printf("Wrong size for configuration descriptor (expected at least 9 bytes, got %lu)\n", dwRecvLength);		
		return;		
	}

	if ((abInOutBuffer[0] != 9) || (abInOutBuffer[1] != 2))
	{
		printf("Wrong format for configuration descriptor (expected header is %02X%02X, got %02X%02X)\n", 9, 2, abInOutBuffer[0], abInOutBuffer[1]);		
		return;		
	}

	wTotalLength = (abInOutBuffer[3] << 8) | abInOutBuffer[2];

	printf("Configuration descriptor:\n");
    printf("\tbLength: %d\n", abInOutBuffer[0]);
    printf("\tbDescriptorType: %d\n", abInOutBuffer[1]);
    printf("\twTotalLength: %d\n", wTotalLength);
    printf("\tbNumInterfaces: %d\n", abInOutBuffer[4]);
    printf("\tbConfigurationValue: %d\n", abInOutBuffer[5]);
    printf("\tiConfiguration: %d\n", abInOutBuffer[6]);
    printf("\tbmAttributes: %02X\n", abInOutBuffer[7]);
    printf("\tbMaxPower: %d (2mA units, so %dmA)\n", abInOutBuffer[8], abInOutBuffer[8] * 2);	
	printf("\n");
}

static void dump_interface_descriptor(void)
{
	DWORD dwRecvLength = sizeof(abInOutBuffer);
	DWORD t;
	LONG rc = CCID_LIB(GetDescriptor)(4, 0, abInOutBuffer, &dwRecvLength);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to read device's interface descriptor (rc=%lX)\n", rc);		
		return;
	}

	if (dwRecvLength < 9)
	{
		printf("Wrong size for interface descriptor (expected at least 9 bytes, got %lu)\n", dwRecvLength);		
		return;		
	}

	if ((abInOutBuffer[0] != 9) || (abInOutBuffer[1] != 4))
	{
		printf("Wrong format for interface descriptor (expected header is %02X%02X, got %02X%02X)\n", 9, 4, abInOutBuffer[0], abInOutBuffer[1]);		
		return;		
	}

	printf("Interface descriptor:\n");
    printf("\tbLength: %d\n", abInOutBuffer[0]);
    printf("\tbDescriptorType: %d\n", abInOutBuffer[1]);
    printf("\tbInterfaceNumber: %d\n", abInOutBuffer[2]);
    printf("\tbAlternateSetting: %d\n", abInOutBuffer[3]);
	printf("\tbNumEndpoints: %d\n", abInOutBuffer[4]);
	printf("\tbInterfaceClass: %02X\n", abInOutBuffer[5]);
	printf("\tbInterfaceSubClass: %02X\n", abInOutBuffer[6]);
	printf("\tbInterfaceProtocol: %02X\n", abInOutBuffer[7]);
	printf("\tiInterface: %d\n", abInOutBuffer[8]);
	printf("\n");

	if (dwRecvLength < 9 + 54)
	{
		printf("Wrong size for CCID class descriptor (expected at least 54 bytes to follow, got %lu)\n", dwRecvLength - 9);
		return;				
	}

	if ((abInOutBuffer[9] != 54) || (abInOutBuffer[10] != 0x21))
	{
		printf("Wrong format for CCID class descriptor (expected header is %02X%02X, got %02X%02X)\n", 54, 0x21, abInOutBuffer[9], abInOutBuffer[10]);		
		return;		
	}
	
	printf("CCID class descriptor:\n");
    printf("\tbLength: %d\n", abInOutBuffer[9]);
    printf("\tbDescriptorType: %d\n", abInOutBuffer[10]);
    printf("\twVersion: %X.%02X\n", abInOutBuffer[12], abInOutBuffer[11]);
    printf("\tbMaxSlotIndex: %d\n", abInOutBuffer[13]);
	printf("\tbVoltageSupport: %02X\n", abInOutBuffer[14]);
	t = (abInOutBuffer[18] << 24) | (abInOutBuffer[17] << 16) | (abInOutBuffer[16] << 8) | abInOutBuffer[15];
	printf("\tdwProtocols: %08lX\n", t);
	t = (abInOutBuffer[22] << 24) | (abInOutBuffer[21] << 16) | (abInOutBuffer[20] << 8) | abInOutBuffer[19];
	printf("\tdwDefaultClock: %lu\n", t);
	t = (abInOutBuffer[26] << 24) | (abInOutBuffer[25] << 16) | (abInOutBuffer[24] << 8) | abInOutBuffer[23];
	printf("\tdwMaximumClocks: %lu\n", t);
	printf("\tbNumClockSupported: %d\n", abInOutBuffer[27]);
	t = (abInOutBuffer[31] << 24) | (abInOutBuffer[30] << 16) | (abInOutBuffer[29] << 8) | abInOutBuffer[28];
	printf("\tdwDataRate: %lu\n", t);
	t = (abInOutBuffer[35] << 24) | (abInOutBuffer[34] << 16) | (abInOutBuffer[33] << 8) | abInOutBuffer[32];
	printf("\tdwMaxDataRate: %lu\n", t);
	printf("\tbNumDataRatesSupported: %d\n", abInOutBuffer[36]);
	t = (abInOutBuffer[40] << 24) | (abInOutBuffer[39] << 16) | (abInOutBuffer[38] << 8) | abInOutBuffer[37];
	printf("\tdwMaxIFSD: %lu\n", t);
	t = (abInOutBuffer[44] << 24) | (abInOutBuffer[43] << 16) | (abInOutBuffer[42] << 8) | abInOutBuffer[41];
	printf("\tdwSynchProtocols: %08lX\n", t);
	t = (abInOutBuffer[48] << 24) | (abInOutBuffer[47] << 16) | (abInOutBuffer[46] << 8) | abInOutBuffer[45];
	printf("\tdwMechanical: %08lX\n", t);
	t = (abInOutBuffer[52] << 24) | (abInOutBuffer[51] << 16) | (abInOutBuffer[50] << 8) | abInOutBuffer[49];
	printf("\tdwFeatures: %08lX\n", t);
	t = (abInOutBuffer[56] << 24) | (abInOutBuffer[55] << 16) | (abInOutBuffer[54] << 8) | abInOutBuffer[53];
	printf("\tdwMaxCCIDMessageLength: %lu\n", t);
	printf("\tbClassGetResponse: %02X\n", abInOutBuffer[57]);
	printf("\tbClassEnvelope: %02X\n", abInOutBuffer[58]);
	printf("\twLcdLayout: %02X%02X\n", abInOutBuffer[60], abInOutBuffer[59]);
	printf("\tbPINSupport: %02X\n", abInOutBuffer[61]);
	printf("\tbMaxCCIDBusySlots: %02X\n", abInOutBuffer[62]);

	#if 0
	/* ---------------------------------------------------------------------- */
	/* Endpoint descriptors                                                   */
	/* ---------------------------------------------------------------------- */
  
	/* Endpoint 1 descriptor (Bulk In) */
	ENDPOINT_DESCRIPTOR_SZ,					/* bLength: Endpoint Descriptor size                */
	ENDPOINT_DESCRIPTOR,					/* bDescriptorType: Endpoint descriptor type        */
	0x81,		              				/* bEndpointAddress : EP1 IN                        */
	0x02,								/* bmAttributes: Bulk endpoint                      */
	0x40,0x00,							/* wMaxPacketSize : 64 char max (0x0040)            */
	0x00,								/* bInterval: Polling Interval (ignored)            */	

	/* Endpoint 2 descriptor (Bulk Out) */
	ENDPOINT_DESCRIPTOR_SZ,					/* bLength: Endpoint Descriptor size                */
	ENDPOINT_DESCRIPTOR,					/* bDescriptorType: Endpoint descriptor type        */
	0x02,								/* bEndpointAddress: EP2 OUT                        */
	0x02,								/* bmAttributes: Bulk endpoint                      */
	0x40,0x00,							/* wMaxPacketSize : 64 char max (0x0040)            */
	0x00,								/* bInterval: Polling Interval (ignored)            */	
	
	/* Endpoint 3 descriptor (Interrupt In)                                   */
	ENDPOINT_DESCRIPTOR_SZ,					/* bLength: Endpoint Descriptor size                */
	ENDPOINT_DESCRIPTOR,					/* bDescriptorType: Endpoint descriptor type        */
	0x83,								/* bEndpointAddress: EP3 IN                         */
	0x03,								/* bmAttributes: Interrupt endpoint                 */
	0x08,0x00,							/* wMaxPacketSize :  8 char max (0x0008)            */
		0x01									/* bInterval: Polling Interval (1ms)                */
#endif
		
}

/* The string descriptors are UNICODE strings, basically we show only one byte out of two (and skip the 'Lang' header) */
static void print_string_descriptor(BYTE abInOutBuffer[], DWORD dwRecvLength)
{
	for (DWORD i = 2; i < dwRecvLength; i += 2)
		printf("%c", abInOutBuffer[i]);
}

static void dump_string_descriptor(const char* stringName, BYTE stringIndex)
{
	DWORD dwRecvLength = sizeof(abInOutBuffer);
	LONG rc = CCID_LIB(GetDescriptor)(3, stringIndex, abInOutBuffer, &dwRecvLength);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Failed to read device string descriptor %02X (rc=%lX)\n", stringIndex, rc);		
		return;
	}

	printf("%s", stringName);
	print_string_descriptor(abInOutBuffer, dwRecvLength);
	printf("\n");
}

static void dump_string_descriptors(void)
{
	printf("Device's string descriptors:\n");
	dump_string_descriptor("\tVendorName:", 1);
	dump_string_descriptor("\tProductName:", 2);
	dump_string_descriptor("\tSerialNumber:", 3);
	dump_string_descriptor("\tProfileName:", 4);
}

static BYTE get_slot_count(void)
{
	BYTE bSlotCount;
	LONG rc = CCID_LIB(GetSlotCount)(&bSlotCount);

	if (rc != SCARD_ERR(S_SUCCESS))
	{
		D(printf("GetSlotCount returned %lX\n", rc));
		return 0;
	}

	return bSlotCount;
}

static BOOL card_is_present(BYTE bSlot)
{
	BOOL fCardPresent;
	LONG rc = SCARD_LIB(Status)(bSlot, &fCardPresent, NULL);

	if (rc != SCARD_ERR(S_SUCCESS))
	{
		D(printf("SCardStatus returned %lX\n", rc));
		return FALSE;
	}

	return fCardPresent;
}

static DWORD random_seed = 12345;
static void init_random(int some_entropy)
{
	random_seed = 12345 + some_entropy;
}
static BYTE get_random(void)
{
	random_seed = random_seed * 1664525 + 1013904223; // Parameters from Numerical Recipes
    return (BYTE) random_seed;
}

static BOOL test_echo_control_ex(unsigned delay, unsigned Lc, unsigned Le)
{
	LONG rc;
	DWORD dwSendLength = 0;
	DWORD dwRecvLength;

	printf("Control Test with Lc=%d, Le=%d, delay=%d...\n", Lc, Le, delay);

	init_random((Lc << 8) | Le);

	abInOutBuffer[dwSendLength++] = 0xFF; /* CLA */
	abInOutBuffer[dwSendLength++] = 0xFD; /* INS */
	abInOutBuffer[dwSendLength++] = 0x00; /* P1 */
	abInOutBuffer[dwSendLength++] = 0x80 | (delay & 0x3F); /* P2 */

	if ((Lc == 0) && (Le == 0))
	{
		/* Case 1 APDU */
	}
	else if ((Lc == 0) && (Le > 0))
	{
		/* Case 2 APDU */
		abInOutBuffer[dwSendLength++] = (BYTE) Le; /* 256 --> 0 is correct */
	}
	else if ((Lc > 0) && (Le == 0))
	{
		/* Case 3 APDU */
		abInOutBuffer[dwSendLength++] = (BYTE)Lc;
		/* Add some "random" data */
		for (unsigned i = 0; i < Lc; i++)
			abInOutBuffer[dwSendLength++] = get_random();
	}
	else /* ((Lc > 0) && (Le > 0)) */
	{
		/* Case 4 APDU */
		abInOutBuffer[dwSendLength++] = (BYTE)Lc;
		/* Add some "random" data */
		for (unsigned i = 0; i < Lc; i++)
			abInOutBuffer[dwSendLength++] = get_random();
		/* And terminate with Le */
		abInOutBuffer[dwSendLength++] = (BYTE)Le; /* 256 --> 0 is correct */
	}
	
	/* Transmit */
	dwRecvLength = sizeof(abInOutBuffer);
	rc = SCARD_LIB(Control)(abInOutBuffer, dwSendLength, abInOutBuffer, &dwRecvLength);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Test failed for Lc=%d, Le=%d, delay=%d (rc=%lX)\n", Lc, Le, delay, rc);
		return FALSE;
	}
	else
	{
		WORD sw = get_status_word(abInOutBuffer, dwRecvLength);
		if (sw != 0x9000)
		{
			printf("Test returned SW=%04X for Lc=%d, Le=%d\n", sw, Lc, Le);
			return FALSE;
		}
		if (dwRecvLength != (Le + 2))
		{
			printf("Test returned %lu bytes instead of %u for Lc=%u, Le=%u\n", dwRecvLength, Le + 2, Lc, Le);
			return FALSE;			
		}

		if (Lc == Le)
		{
			/* Test that echo is correct */
			init_random((Lc << 8) | Le);

			for (unsigned i = 0; i < Lc; i++)
			{
				if (abInOutBuffer[i] != get_random())
				{
					printf("Test failed, returned data is not an echo of sent data (at offset %d) for Lc=%d, Le=%d\n", i, Lc, Le);
					return FALSE;				
				}
			}
		}
	}

	return TRUE;	
}

static BOOL test_echo_control(void)
{
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;
	if (!test_echo_control_ex(0, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;
	if (!test_echo_control_ex(1, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;		
	if (!test_echo_control_ex(10, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_control_ex(0, 0, 256))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_control_ex(0, 255, 255))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_control_ex(30, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_control_ex(60, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	

	return TRUE;
}

/**
 * @brief Test the implementation using the ECHO instruction through SCardTransmit
 * See https://docs.springcard.com/books/SpringCore/PCSC_Operation/APDU_Interpreter/Vendor_instructions/ECHO for reference
 * @param slot The slot
 * @param delay Delay to be added by the coupler before answering (0 to 60 seconds)
 * @param Lc Length of data sent by the host application to the coupler
 * @param Le Length of data to be returned by the coupler to the host application
 * @return BOOL Success/Failure
 */
static BOOL test_echo_transmit_ex(BYTE slot, unsigned delay, unsigned Lc, unsigned Le)
{
	LONG rc;
	DWORD dwSendLength = 0;
	DWORD dwRecvLength;

	printf("Transmit Test with Lc=%d, Le=%d, delay=%d...\n", Lc, Le, delay);

	init_random((Lc << 8) | Le);

	abInOutBuffer[dwSendLength++] = 0xFF; /* CLA */
	abInOutBuffer[dwSendLength++] = 0xFD; /* INS */
	abInOutBuffer[dwSendLength++] = 0x00; /* P1 */
	abInOutBuffer[dwSendLength++] = 0x80 | (delay & 0x3F); /* P2 */

	if ((Lc == 0) && (Le == 0))
	{
		/* Case 1 APDU */
	}
	else if ((Lc == 0) && (Le > 0))
	{
		/* Case 2 APDU */
		abInOutBuffer[dwSendLength++] = (BYTE) Le; /* 256 --> 0 is correct */
	}
	else if ((Lc > 0) && (Le == 0))
	{
		/* Case 3 APDU */
		abInOutBuffer[dwSendLength++] = (BYTE)Lc;
		/* Add some "random" data */
		for (unsigned i = 0; i < Lc; i++)
			abInOutBuffer[dwSendLength++] = get_random();
	}
	else /* ((Lc > 0) && (Le > 0)) */
	{
		/* Case 4 APDU */
		abInOutBuffer[dwSendLength++] = (BYTE)Lc;
		/* Add some "random" data */
		for (unsigned i = 0; i < Lc; i++)
			abInOutBuffer[dwSendLength++] = get_random();
		/* And terminate with Le */
		abInOutBuffer[dwSendLength++] = (BYTE)Le; /* 256 --> 0 is correct */
	}
	
	/* Transmit */
	dwRecvLength = sizeof(abInOutBuffer);
	rc = SCARD_LIB(Transmit)(slot, abInOutBuffer, dwSendLength, abInOutBuffer, &dwRecvLength);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		printf("Test failed for Lc=%d, Le=%d, delay=%d (rc=%lX)\n", Lc, Le, delay, rc);
		return FALSE;
	}
	else
	{
		WORD sw = get_status_word(abInOutBuffer, dwRecvLength);
		if (sw != 0x9000)
		{
			printf("Test returned SW=%04X for Lc=%d, Le=%d\n", sw, Lc, Le);
			return FALSE;
		}
		if (dwRecvLength != (Le + 2))
		{
			printf("Test returned %lu bytes instead of %u for Lc=%u, Le=%u\n", dwRecvLength, Le + 2, Lc, Le);
			return FALSE;			
		}

		if (Lc == Le)
		{
			/* Test that echo is correct */
			init_random((Lc << 8) | Le);

			for (unsigned i = 0; i < Lc; i++)
			{
				if (abInOutBuffer[i] != get_random())
				{
					printf("Test failed, returned data is not an echo of sent data (at offset %d) for Lc=%d, Le=%d\n", i, Lc, Le);
					return FALSE;				
				}
			}
		}
	}

	return TRUE;
}

static BOOL test_echo_transmit(BYTE slot)
{
	/* Test the limits first - It is better to fail quickly if something is wrong */
	if (!test_echo_transmit_ex(slot, 0, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_transmit_ex(slot, 1, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_transmit_ex(slot, 10, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_transmit_ex(slot, 0, 0, 256))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_transmit_ex(slot, 0, 255, 255))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	
	if (!test_echo_transmit_ex(slot, 30, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;
	if (!test_echo_transmit_ex(slot, 60, 0, 0))
		return FALSE;
	if (!SCARD_LIB(IsValidContext)())
		return FALSE;	

	/* Now test every send length */
	for (int Lc = 1; Lc <= 255; Lc++)
	{
		if (!test_echo_transmit_ex(slot, 0, Lc, 0))
			return FALSE;
		if (!SCARD_LIB(IsValidContext)())
			return FALSE;		
	}
	/* The every recv length */
	for (int Le = 1; Le <= 256; Le++)
	{
		if (!test_echo_transmit_ex(slot, 0, 0, Le))
			return FALSE;
		if (!SCARD_LIB(IsValidContext)())
			return FALSE;		
	}
	/* And test actual echo (recv length = send length) */
	for (int Lc = 1; Lc <= 255; Lc++)
	{
		if (!test_echo_transmit_ex(slot, 0, Lc, Lc))
			return FALSE;
		if (!SCARD_LIB(IsValidContext)())
			return FALSE;		
	}

	return TRUE;
}
