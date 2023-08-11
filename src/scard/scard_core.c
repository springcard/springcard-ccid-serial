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
 * @file scard_core.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Core functions of the PC/SC-Like library
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @addtogroup scard
 */

#include "scard_i.h"

/**
 * @brief Retrieve the status of a slot:
 * - is there a card in the slot or not?
 * - is the card powered or not?
 * @note This is not exactly the same prototype as SCardStatus in the PC/SC standard, but it provides the same information
 * @param bSlot slot number (0 for the contactless slot, 1 for the 1st contact slot, etc)
 * @param pfCardPresent pointer to receive the 'present' flag
 * @param pfCardPowered pointer to receive the 'powered' flag
 * @return SCARD_ERR_S_SUCCESS success, flags are reliable
 * @return Other code if internal or communication error has occured
 * @note This function is based on CCID PC_TO_RDR_GetSlotStatus
 * @note
 *   There's no need to loop around SCARD_Status to wait for card insertion.
 *   Looping around SCARD_Connect is more efficient since when a card is
 *   inserted, the ATR will be returned at once.
 *   On the other hand, looping around SCARD_Status is useful to wait for
 *   card removal after a call to SCARD_Disconnect.
 * @see SCARD_Connect
 **/
LONG SCARD_LIB(Status)(BYTE bSlot, BOOL* pfCardPresent, BOOL* pfCardPowered)
{
	CCID_PACKET_ST packet;
	LONG rc;

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_BULK_PC_TO_RDR;
	packet.Header.p.bRequest = PC_TO_RDR_GETSLOTSTATUS;
	packet.Header.p.Data.BulkOut.bSlot = bSlot;
	packet.Header.p.Data.BulkOut.bSequence = CCID_LIB(GetSequence)(bSlot);

	rc = CCID_LIB(Exchange)(&packet, BULK_TIMEOUT);
	if (SCARD_LIB(IsFatalError)(rc))
		return rc; /* Fatal error encountered, no need to go further */

	if (packet.Header.p.bRequest != RDR_TO_PC_SLOTSTATUS)
	{
		scard_raise_error("Wrong opcode in response to GET SLOT STATUS");
		rc = SCARD_ERR(E_READER_UNSUPPORTED);
	}
	else
	{
		rc = SCARD_ERR(S_SUCCESS);
		switch (packet.Header.p.Data.BulkIn.bSlotStatus & 0x03)
		{
			case 0x00:
				if (pfCardPresent != NULL)
					*pfCardPresent = TRUE;
				if (pfCardPowered != NULL)
					*pfCardPowered = TRUE;
			break;
			case 0x01:
				if (pfCardPresent != NULL)
					*pfCardPresent = TRUE;
				if (pfCardPowered != NULL)
					*pfCardPowered = FALSE;
			break;
			case 0x02:
				if (pfCardPresent != NULL)
					*pfCardPresent = FALSE;
				if (pfCardPowered != NULL)
					*pfCardPowered = FALSE;
			break;
			case 0x03:
			default:
				scard_raise_error("Wrong STATUS value in response to GET SLOT STATUS");
				rc = SCARD_ERR(E_READER_UNSUPPORTED);
		}
	}

	return rc;
}

/**
 * @brief Connect to the card in the given slot (if some)
 * @note This is not exactly the same prototype as SCardConnect in the PC/SC standard, but it provides the same feature
 * @param bSlot slot number (0 for the contactless slot, 1 for the 1st contact slot, etc)
 * @param abAtr buffer to receive the ATR (must be at least 32-byte long)
 * @param pdwAtrLength IN: the size of the ATR buffer; OUT: the actual length of the ATR
 * @return SCARD_S_SUCCESS success, ATR is valid
 * @return SCARD_W_REMOVED_CARD: there's no card in the slot
 * @return SCARD_W_UNSUPPORTED_CARD: there's a card in the slot, but its ATR is invalid
 * @return SCARD_W_UNRESPONSIVE_CARD: there's a card in the slot, but it is mute
 * @return Other code if internal or communication error has occured
 * @note This function is based on CCID PC_TO_RDR_IccPowerOn
 * @see SCARD_Status
 * @see SCARD_Disconnect
 **/
LONG SCARD_LIB(Connect)(BYTE bSlot, BYTE abAtr[], DWORD* pdwAtrLength)
{
	CCID_PACKET_ST packet;
	LONG rc;

	if ((abAtr == NULL) || (pdwAtrLength == NULL))
		return SCARD_ERR(E_INVALID_PARAMETER);

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_BULK_PC_TO_RDR;
	packet.Header.p.bRequest = PC_TO_RDR_ICCPOWERON;
	packet.Header.p.Data.BulkOut.bSlot = bSlot;
	packet.Header.p.Data.BulkOut.bSequence = CCID_LIB(GetSequence)(bSlot);

	packet.abRecvPayload = abAtr;
	packet.dwRecvPayloadMaxLen = *pdwAtrLength;

	rc = CCID_LIB(Exchange)(&packet, BULK_TIMEOUT);

	if (rc == SCARD_ERR(S_SUCCESS))
	{
		if (packet.Header.p.bRequest != RDR_TO_PC_DATABLOCK)
		{
			rc = SCARD_ERR(E_READER_UNSUPPORTED);
		}
		else
		{
			*pdwAtrLength = packet.Header.p.Length.dw;
		}
	}

	return rc;
}

/**
 * @brief Disconnect from the card in the slot
 * @note This is not exactly the same prototype as SCardDisconnect in the PC/SC standard, but it provides the same feature
 * @param bSlot slot number (0 for the contactless slot, 1 for the 1st contact slot, etc)
 * @return SCARD_S_SUCCESS success
 * @return Other code if internal or communication error has occured
 * @note This function is based on CCID PC_TO_RDR_IccPowerOff
 * @see SCARD_Status
 * @see SCARD_Connect
 **/
LONG SCARD_LIB(Disconnect)(BYTE bSlot)
{
	CCID_PACKET_ST packet;
	LONG rc;

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_BULK_PC_TO_RDR;
	packet.Header.p.bRequest = PC_TO_RDR_ICCPOWEROFF;
	packet.Header.p.Data.BulkOut.bSlot = bSlot;
	packet.Header.p.Data.BulkOut.bSequence = CCID_LIB(GetSequence)(bSlot);

	rc = CCID_LIB(Exchange)(&packet, BULK_TIMEOUT);

	if ((rc == SCARD_ERR(W_UNSUPPORTED_CARD)) ||
		(rc == SCARD_ERR(W_UNRESPONSIVE_CARD)) ||
		(rc == SCARD_ERR(W_REMOVED_CARD)))
	{
		/* Not much an error... */
		rc = SCARD_ERR(S_SUCCESS);
	}

	return rc;
}

/**
 * @brief Send a command (C-APDU) to the card, and receive its response (R-APDU)
 * @note This is not exactly the same prototype as SCardTransmit in the PC/SC standard, but it provides the same feature
 * @param bSlot slot number (0 for the contactless slot, 1 for the 1st contact slot, etc)
 * @param abSendApdu the C-APDU
 * @param dwSendLength length of the C-APDU
 * @param abRecvApdu buffer to receive the R-APDU (required size depends on what the card will return...)
 * @param pdwRecvLength IN: the size of the R-APDU buffer; OUT: the actual length of the R-APDU
 * @return SCARD_S_SUCCESS success
 * @return SCARD_W_REMOVED_CARD the card has been removed during the exchange
 * @return Other code if internal or communication error has occured.
 * @note This function is based on CCID PC_TO_RDR_XfrBlock
 * @see SCARD_Connect
 * @see SCARD_Control
 **/
LONG SCARD_LIB(Transmit)(BYTE bSlot, const BYTE abSendApdu[], DWORD dwSendLength, BYTE abRecvApdu[], DWORD *pdwRecvLength)
{
	CCID_PACKET_ST packet;
	LONG rc;

	if (abSendApdu == NULL)
		return SCARD_ERR(E_INVALID_PARAMETER);
	if ((abRecvApdu != NULL) && (pdwRecvLength == NULL))
		return SCARD_ERR(E_INVALID_PARAMETER);
	if (dwSendLength > CCID_MAX_PAYLOAD_LENGTH)
		return SCARD_ERR(E_NO_MEMORY);	

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_BULK_PC_TO_RDR;
	packet.Header.p.bRequest = PC_TO_RDR_XFRBLOCK;
	packet.Header.p.Data.BulkOut.bSlot = bSlot;
	packet.Header.p.Data.BulkOut.bSequence = CCID_LIB(GetSequence)(bSlot);

	packet.abSendPayload = abSendApdu;
	packet.Header.p.Length.dw = dwSendLength;

	if (pdwRecvLength != NULL)
	{
		packet.abRecvPayload = abRecvApdu;
		packet.dwRecvPayloadMaxLen = *pdwRecvLength;
	}

	rc = CCID_LIB(Exchange)(&packet, BULK_TIMEOUT);

	if (rc == SCARD_ERR(S_SUCCESS))
		if (pdwRecvLength != NULL)
			*pdwRecvLength = packet.Header.p.Length.dw;

	if ((rc == SCARD_ERR(W_UNSUPPORTED_CARD)) ||
		(rc == SCARD_ERR(W_UNRESPONSIVE_CARD)) ||
		(rc == SCARD_ERR(W_UNPOWERED_CARD)) ||
		(rc == SCARD_ERR(W_RESET_CARD)))
	{
		rc = SCARD_ERR(W_REMOVED_CARD);
	}

	return rc;
}

/**
 * @brief Send a command to the coupler (PC/SC device), and receive its response
 * @note This is not exactly the same prototype as SCardControl in the PC/SC standard, but it provides the same feature
 * @param abSendBuffer the command
 * @param dwSendLength length of the command
 * @param abRecvBuffer buffer to receive the response (required size depends on what the device will return...)
 * @param pdwRecvLength IN: the size of the response buffer; OUT: the actual length of the response
 * @return SCARD_S_SUCCESS success
 * @return Other code if internal or communication error has occured.
 * @note This function is based on CCID PC_TO_RDR_Escape
 * @see SCARD_Transmit
 **/
LONG SCARD_LIB(Control)(const BYTE abSendBuffer[], DWORD dwSendLength, BYTE abRecvBuffer[], DWORD *pdwRecvLength)
{
	CCID_PACKET_ST packet;
	BYTE bDummyRecvByte;
	LONG rc;

	if (abSendBuffer == NULL)
		return SCARD_ERR(E_INVALID_PARAMETER);
	if ((abRecvBuffer != NULL) && (pdwRecvLength == NULL))
		return SCARD_ERR(E_INVALID_PARAMETER);
	if (dwSendLength > CCID_MAX_PAYLOAD_LENGTH)
		return SCARD_ERR(E_NO_MEMORY);

	CCID_LIB(PacketInit)(&packet);

	packet.bEndpoint = CCID_COMM_BULK_PC_TO_RDR;
	packet.Header.p.bRequest = PC_TO_RDR_ESCAPE;

	packet.abSendPayload = abSendBuffer;
	packet.Header.p.Length.dw = dwSendLength;

	if (pdwRecvLength != NULL)
	{
		packet.abRecvPayload = abRecvBuffer;
		packet.dwRecvPayloadMaxLen = *pdwRecvLength;
	}
	else
	{
		packet.abRecvPayload = &bDummyRecvByte;
		packet.dwRecvPayloadMaxLen = 1;
	}

	rc = CCID_LIB(Exchange)(&packet, BULK_TIMEOUT);

	if (!SCARD_LIB(IsFatalError)(rc))
	{
		if (packet.Header.p.bRequest != RDR_TO_PC_ESCAPE)
		{
			rc = SCARD_ERR(E_READER_UNSUPPORTED);
		}
		else
		{
			rc = SCARD_ERR(S_SUCCESS);
			if (pdwRecvLength != NULL)
			{
				*pdwRecvLength = packet.Header.p.Length.dw;
			}
			else if (bDummyRecvByte != 0)
			{
				/* The device has returned an error */
				return SCARD_ERR(F_UNKNOWN_ERROR);
			}
		}
	}

	return rc;
}

/**
 * @brief Wait for a notification (interrupt) from the device
 * @note This function shall not be used if the interrupts have not been enabled when calling CCID_Start
 * @note This is not exactly the same prototype as SCardGetStatusChange in the PC/SC standard, but it provides the same feature
 * @param dwTimeoutMs the timeout, in milliseconds
 * @param pdwPresentSlots OUT: the slots were a card is present (1 bit per slot, bit set if card present, unset if card absent)
 * @param pdwChangedSlots OUT: the slots were the status has changed (1 bit per slot, bit set if card has been inserted or removed, unset otherwise)
 * @return SCARD_S_SUCCESS success
 * @return SCARD_E_TIMEOUT no change and the timeout has occured
 * @return Other code if internal or communication error has occured
 * @note This function is based on CCID INTERRUPT endpoint
 * @see SCARD_Status
 * @see SCARD_GetStatusChange
 **/
LONG SCARD_LIB(GetStatusChangeEx)(DWORD dwTimeoutMs, DWORD* pdwPresentSlots, DWORD* pdwChangedSlots)
{
	CCID_PACKET_ST packet;
	BYTE abInterruptBuffer[CCID_MAX_INTERRUPT_PAYLOAD_LENGTH];
	LONG rc;
	DWORD dwPresentSlots = 0;
	DWORD dwChangedSlots = 0;

	CCID_LIB(PacketInit)(&packet);

	packet.abRecvPayload = abInterruptBuffer;
	packet.dwRecvPayloadMaxLen = sizeof(abInterruptBuffer);

	rc = CCID_LIB(WaitInterrupt)(&packet, dwTimeoutMs);

	if (rc == SCARD_ERR(S_SUCCESS))
	{
		if (packet.Header.p.Length.dw > 1)
		{
			if (abInterruptBuffer[0] & 0x01)
				dwPresentSlots |= 0x00000001;
			if (abInterruptBuffer[0] & 0x02)
				dwChangedSlots |= 0x00000001;
			if (abInterruptBuffer[0] & 0x04)
				dwPresentSlots |= 0x00000002;
			if (abInterruptBuffer[0] & 0x08)
				dwChangedSlots |= 0x00000002;
			if (abInterruptBuffer[0] & 0x10)
				dwPresentSlots |= 0x00000004;
			if (abInterruptBuffer[0] & 0x20)
				dwChangedSlots |= 0x00000004;
			if (abInterruptBuffer[0] & 0x40)
				dwPresentSlots |= 0x00000008;
			if (abInterruptBuffer[0] & 0x80)
				dwChangedSlots |= 0x00000008;
		}
		if (packet.Header.p.Length.dw > 2)
		{
			if (abInterruptBuffer[1] & 0x01)
				dwPresentSlots |= 0x00000010;
			if (abInterruptBuffer[1] & 0x02)
				dwChangedSlots |= 0x00000010;
			if (abInterruptBuffer[1] & 0x04)
				dwPresentSlots |= 0x00000020;
			if (abInterruptBuffer[1] & 0x08)
				dwChangedSlots |= 0x00000020;
			if (abInterruptBuffer[1] & 0x10)
				dwPresentSlots |= 0x00000040;
			if (abInterruptBuffer[1] & 0x20)
				dwChangedSlots |= 0x00000040;
			if (abInterruptBuffer[1] & 0x40)
				dwPresentSlots |= 0x00000080;
			if (abInterruptBuffer[1] & 0x80)
				dwChangedSlots |= 0x00000080;
		}

		D(printf("Interrupt, slots present: %08lX, slots changed: %08lX\n", dwPresentSlots, dwChangedSlots));

		if (pdwPresentSlots != NULL)
			*pdwPresentSlots = dwPresentSlots;
		if (pdwChangedSlots != NULL)
			*pdwChangedSlots = dwChangedSlots;		
	}
	
	return rc;
}

/**
 * @brief Wait for a notification (interrupt) from the device
 * @note This function shall not be used if the interrupts have not been enabled when calling CCID_Start
 * @note This is not exactly the same prototype as SCardGetStatusChange in the PC/SC standard, but it provides the same feature
 * @param dwTimeoutMs the timeout, in milliseconds
 * @return SCARD_S_SUCCESS success
 * @return SCARD_E_TIMEOUT no change and the timeout has occured
 * @return Other code if internal or communication error has occured
 * @note This function is based on CCID INTERRUPT endpoint
 * @see SCARD_Status
 * @see SCARD_GetStatusChangeEx
 **/
LONG SCARD_LIB(GetStatusChange)(DWORD dwTimeoutMs)
{
	return SCARD_LIB(GetStatusChangeEx)(dwTimeoutMs, NULL, NULL);
}

