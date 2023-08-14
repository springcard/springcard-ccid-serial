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
 * @file ccid_exchange.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Send/recv CCID packets
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @addtogroup ccid
 */

#include "ccid_i.h"

typedef struct
{
	BYTE bSequence;
} CCID_SLOT_ST;

static CCID_SLOT_ST ccid_slot[CCID_MAX_SLOT_COUNT];

/**
 * @brief Return the current sequence number for the given slot
 */
BYTE CCID_LIB(GetSequence)(BYTE bSlot)
{
	if (bSlot < CCID_MAX_SLOT_COUNT)
		return ccid_slot[bSlot].bSequence;
	return (BYTE)-1;
}

/**
 * @brief Increase the sequence number for the given slot
 */
void CCID_LIB(NextSequence)(BYTE bSlot)
{
	if (bSlot < CCID_MAX_SLOT_COUNT)
		ccid_slot[bSlot].bSequence++;
}

/**
 * @brief Reset all the sequence numbers
 */
void CCID_LIB(ResetSequences)(void)
{
	for (BYTE i = 0; i < CCID_MAX_SLOT_COUNT; i++)
		ccid_slot[i].bSequence = 0;
}

/**
 * @brief Initilize a CCID_PACKET_ST structure
 */
void CCID_LIB(PacketInit)(CCID_PACKET_ST* packet)
{
	if (packet != NULL)
		memset(packet, 0, sizeof(CCID_PACKET_ST));
}

/**
 * @internal
 * @brief Translate CCID slot status or error code into a PC/SC error code
 */
static LONG ccid_recv_to_slot_error(BYTE bSlotError)
{
	LONG rc;

	switch (bSlotError)
	{
		case CCID_ERR_UNKNOWN:
		case CCID_ERR_PARAMETERS:
		case CCID_ERR_CMD_NOT_SUPPORTED:
		case CCID_ERR_BAD_LENGTH:
		case CCID_ERR_BAD_SLOT:
		case CCID_ERR_BAD_POWERSELECT:
		case CCID_ERR_BAD_LEVELPARAMETER:
		case CCID_ERR_BAD_FIDI:
		case CCID_ERR_BAD_T01CONVCHECKSUM:
		case CCID_ERR_BAD_GUARDTIME:
		case CCID_ERR_BAD_WAITINGINTEGER:
		case CCID_ERR_BAD_CLOCKSTOP:
		case CCID_ERR_BAD_IFSC:
		case CCID_ERR_BAD_NAD:
		case CCID_ERR_CMD_ABORTED:
			rc = SCARD_ERR(E_UNEXPECTED);
		break;
		
		case CCID_ERR_ICC_MUTE:
		case CCID_ERR_XFR_PARITY_ERROR:
		case CCID_ERR_XFR_OVERRUN:
		case CCID_ERR_HW_ERROR:
			rc = SCARD_ERR(W_UNRESPONSIVE_CARD);
		break;
		
		case CCID_ERR_BAD_ATR_TS:
		case CCID_ERR_BAD_ATR_TCK:
		case CCID_ERR_ICC_PROTOCOL_NOT_SUPPORTED:
		case CCID_ERR_ICC_CLASS_NOT_SUPPORTED:
		case CCID_ERR_PROCEDURE_BYTE_CONFLICT:
		case CCID_ERR_DEACTIVATED_PROTOCOL:
			rc = SCARD_ERR(W_UNSUPPORTED_CARD);
		break;
		
		case CCID_ERR_BUSY_WITH_AUTO_SEQUENCE:
		case CCID_ERR_CMD_SLOT_BUSY:
			rc = SCARD_ERR(E_UNEXPECTED);
		break;
		
		case CCID_SUCCESS:
		default:
			rc = SCARD_ERR(S_SUCCESS);
			break;
	}

	return rc;
}

/**
 * @internal
 * @brief Translate CCID slot status to PC/SC slot status
 */
static LONG ccid_recv_to_slot_status(CCID_PACKET_ST* packet)
{
	switch (packet->Header.p.Data.BulkIn.bSlotStatus & 0xC0)
	{
		case 0x00:
			break;
		case 0x40:
			return ccid_recv_to_slot_error(packet->Header.p.Data.BulkIn.bSlotError);
		case 0x80:
			return SCARD_ERR(E_TIMEOUT);
		case 0xC0:
		default:
			return SCARD_ERR(E_READER_UNSUPPORTED);
	}

	switch (packet->Header.p.Data.BulkIn.bSlotStatus & 0x03)
	{
		case 0x00:
			return SCARD_ERR(S_SUCCESS);
		case 0x01:
			return SCARD_ERR(W_UNRESPONSIVE_CARD);
		case 0x02:
			return SCARD_ERR(W_REMOVED_CARD);
		case 0x03:
		default:
			return SCARD_ERR(E_READER_UNSUPPORTED);
	}
}

/**
 * @brief Send a packet to the device, and expect a packet in response, within the given timeout
 */
LONG CCID_LIB(Exchange)(CCID_PACKET_ST* packet, DWORD timeout_ms)
{
	LONG rc;
	BYTE bEndpoint;
	BYTE bSlot, bSequence;
	WORD wIndex, wValue;
	WORD wTimeExtension = 0;

	if (packet == NULL)
	{
		ccid_raise_error("NULL packet in CCID_Exchange");
		return SCARD_ERR(F_INTERNAL_ERROR);
	}

	bEndpoint = packet->bEndpoint;
	wIndex = packet->Header.p.Data.Control.Index.w;
	wValue = packet->Header.p.Data.Control.Value.w;
	bSlot = packet->Header.p.Data.BulkOut.bSlot;
	bSequence = packet->Header.p.Data.BulkOut.bSequence;

	rc = CCID_LIB(SerialSend)(packet);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		ccid_raise_error("Failed to send packet to device");
		return rc;
	}

again:

	rc = CCID_LIB(SerialRecv)(packet, timeout_ms);
	if (rc != SCARD_ERR(S_SUCCESS))
	{		
		ccid_raise_error("Failed to receive packet from device");
		return rc;
	}

	if (packet->bEndpoint == CCID_COMM_INTERRUPT_RDR_TO_PC)
	{
		/* This is not a response but an interrupt. We can discard it safely if we are in the middle of an exchange */
		D(printf("Incoming Interrupt\n"));
		goto again;
	}

	switch (bEndpoint)
	{
		case CCID_COMM_CONTROL_TO_RDR:
			if (packet->bEndpoint != CCID_COMM_CONTROL_TO_PC)
			{
				ccid_raise_error("Wrong endpoint in response");
				rc = SCARD_ERR(E_READER_UNSUPPORTED);
			}
			else if ((packet->Header.p.Data.Control.Index.w != wIndex) || (packet->Header.p.Data.Control.Value.w != wValue))
			{
				ccid_raise_error("Wrong index/value in response");
				rc = SCARD_ERR(E_READER_UNSUPPORTED);
			}
		break;

		case CCID_COMM_BULK_PC_TO_RDR:
			if (packet->bEndpoint != CCID_COMM_BULK_RDR_TO_PC)
			{
				ccid_raise_error("Wrong endpoint in response");
				rc = SCARD_ERR(E_READER_UNSUPPORTED);
			}
			else if ((packet->Header.p.Data.BulkIn.bSlot != bSlot) || (packet->Header.p.Data.BulkIn.bSequence != bSequence))
			{
				ccid_raise_error("Wrong slot in response");
				rc = SCARD_ERR(E_READER_UNSUPPORTED);
			}
			else
			{
				rc = ccid_recv_to_slot_status(packet);
				if (rc == SCARD_ERR(E_TIMEOUT))
				{
					/* This is a time extension */
					wTimeExtension++;
					D(printf("Time extension %d...\n", wTimeExtension));
					if (wTimeExtension <= 120)
						goto again;						
					/* More than 2 minutes seems too much... */
					rc = SCARD_ERR(F_WAITED_TOO_LONG);
				}
				CCID_LIB(NextSequence)(bSlot);				
			}
		break;

		case CCID_COMM_INTERRUPT_RDR_TO_PC:
			/* Interrupts should not arrive here */
			ccid_raise_error("Received an Interrupt instead of a Response");
			rc = SCARD_ERR(E_READER_UNSUPPORTED);
		break;

		default:
			ccid_raise_error("Unsupported Endpoint");
			rc = SCARD_ERR(E_READER_UNSUPPORTED);			
		break;
	}

	return rc;
}

/**
 * @brief Wait and receive an interrupt (notification) packet from the device, within the given timeout
 */
LONG CCID_LIB(WaitInterrupt)(CCID_PACKET_ST* packet, DWORD timeout_ms)
{
	LONG rc;

	if (packet == NULL)
	{
		ccid_raise_error("NULL packet in CCID_WaitInterrupt");
		return SCARD_ERR(F_INTERNAL_ERROR);
	}

	rc = CCID_LIB(SerialRecv)(packet, timeout_ms);
	if (rc != SCARD_ERR(S_SUCCESS))
	{
		ccid_raise_error("Failed to receive Interrupt packet from device");
		return rc;
	}

	if (packet->bEndpoint != CCID_COMM_INTERRUPT_RDR_TO_PC)
	{
		ccid_raise_error("Wrong endpoint for Interrupt");
		rc = SCARD_ERR(E_READER_UNSUPPORTED);
	}
	else if (packet->Header.p.bRequest != RDR_TO_PC_INTERRUPT)
	{
		ccid_raise_error("Wrong opcode for Interrupt");
		rc = SCARD_ERR(E_READER_UNSUPPORTED);
	}
	
	return rc;
}

