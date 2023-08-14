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
 * @file ccid_helpers.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Helpers and internal functions of the CCID driver
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 *
 * @addtogroup ccid
 */

#include "ccid_i.h"

static BOOL ccid_valid;

/**
 * @internal
 * @brief Report an error
 */
void ccid_raise_error(const char* msg)
{
	printf("\nError in CCID driver: %s\n", msg);
	ccid_valid = FALSE;
}

/**
 * @brief Return TRUE if the CCID driver is up and running (serial port OK, no error)
 */
BOOL CCID_LIB(IsValidDriver)(void)
{
	if (ccid_valid)
	{
		if (!CCID_LIB(SerialIsOpen)())
			ccid_valid = FALSE;
	}
	return ccid_valid;
}

/**
 * @brief Initialize the CCID driver (e.g. clear the state machine, clear all status variables)
 */
void CCID_LIB(Init)(void)
{
	ccid_reset_receiver();
	ccid_valid = TRUE;
}

/**
 * @brief Test/probe the communication with CCID compliant device
 */
LONG CCID_LIB(Ping)(void)
{
	CCID_PACKET_ST packet;
	LONG rc;

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_CONTROL_TO_RDR;
	packet.Header.p.bRequest = GET_STATUS;

	rc = CCID_LIB(Exchange)(&packet, CONTROL_TIMEOUT);

	return rc;
}

/**
 * @brief Start CCID (activate PC/SC operation in the device)
 */
LONG CCID_LIB(Start)(BOOL fUseNotifications)
{
	CCID_PACKET_ST packet;
	LONG rc;

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_CONTROL_TO_RDR;
	packet.Header.p.bRequest = SET_CONFIGURATION;
	packet.Header.p.Data.Control.Value.w = 1;
	packet.Header.p.Data.Control.Index.w = 0;
	packet.Header.p.Data.Control.InOut.bOutOption = 0;

	if (fUseNotifications)
	{
		packet.Header.p.Data.Control.InOut.bOutOption = 1;
	}

	rc = CCID_LIB(Exchange)(&packet, CONTROL_TIMEOUT);

	if (rc == SCARD_ERR(S_SUCCESS))
	{
		if (packet.Header.p.Data.Control.InOut.bInStatus != 0x01)
			rc = SCARD_ERR(E_UNEXPECTED);
	}

	/* Reset the sequence numbers */
	CCID_LIB(ResetSequences)();

	return rc;
}

/**
 * @brief Stop CCID (disable PC/SC operation in the device)
 */
LONG CCID_LIB(Stop)(void)
{
	CCID_PACKET_ST packet;
	LONG rc;

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_CONTROL_TO_RDR;
	packet.Header.p.bRequest = SET_CONFIGURATION;
	packet.Header.p.Data.Control.Value.w = 0;
	packet.Header.p.Data.Control.Index.w = 0;
	packet.Header.p.Data.Control.InOut.bOutOption = 0;

	rc = CCID_LIB(Exchange)(&packet, CONTROL_TIMEOUT);

	if (rc == SCARD_ERR(S_SUCCESS))
	{
		if (packet.Header.p.Data.Control.InOut.bInStatus != 0x00)
			rc = SCARD_ERR(E_UNEXPECTED);
	}

	return rc;	
}

/**
 * @brief Read the descriptor from the device
 */
LONG CCID_LIB(GetDescriptor)(BYTE bType, BYTE bIndex, BYTE abDescriptor[], DWORD *pdwDescriptorLength)
{
	CCID_PACKET_ST packet;
	LONG rc;

	if ((abDescriptor != NULL) && (pdwDescriptorLength == NULL))
		return SCARD_ERR(E_INVALID_PARAMETER);

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_CONTROL_TO_RDR;
	packet.Header.p.bRequest = GET_DESCRIPTOR;
	packet.Header.p.Data.Control.Value.ab[0] = bType;
	packet.Header.p.Data.Control.Value.ab[1] = bIndex;

	if (pdwDescriptorLength != NULL)
	{
		packet.abRecvPayload = abDescriptor;
		packet.dwRecvPayloadMaxLen = *pdwDescriptorLength;
	}

	rc = CCID_LIB(Exchange)(&packet, CONTROL_TIMEOUT);

	if (rc == SCARD_ERR(S_SUCCESS))
	{
		if (packet.Header.p.Data.Control.InOut.bInStatus != 0x00)
		{
			rc = SCARD_ERR(E_UNEXPECTED);
		}
		else
		{
			if (pdwDescriptorLength != NULL)
				*pdwDescriptorLength = packet.Header.p.Length.dw;
		}
	}

	return rc;	
}

/**
 * @brief Read the number of slots that a device has
 */
LONG CCID_LIB(GetSlotCount)(BYTE* bSlotCount)
{
	/*
	 * We use the upper layer (SCARD aka PC/SC) to answer a query regarding the low layer...
	 * I know this is not OSI compliant ;-)
	 * Why doing so?
	 * Just because the device is able to return the slot count at once, where reading and
	 * parsing its descriptor requires more memory and more communication time.
	 * */
	const BYTE abSendBuffer[] = { 0x58, 0x20, 0x80 };
	BYTE abRecvBuffer[2];
	DWORD dwRecvLength = sizeof(abRecvBuffer);
	LONG rc = SCARD_LIB(Control)(abSendBuffer, sizeof(abSendBuffer), abRecvBuffer, &dwRecvLength);

	if (rc == SCARD_ERR(S_SUCCESS))
	{
		if (dwRecvLength < 2)
		{
			rc = SCARD_ERR(E_READER_UNSUPPORTED);
		}
		else if (abRecvBuffer[0] != 0x00)
		{
			rc = SCARD_ERR(E_READER_UNSUPPORTED);
		}
		else
		{
			if (bSlotCount != NULL)
				*bSlotCount = abRecvBuffer[1];
		}
	}

	return rc;
}
