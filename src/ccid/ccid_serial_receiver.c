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
 * @file ccid_serial_receiver.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Implementation of the CCID driver, RX part
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 *
 * @addtogroup ccid
 */

#include "ccid_i.h"

#define STATUS_IDLE 0
#define STATUS_RECV_ENDPOINT 1
#define STATUS_RECV_HEADER 2
#define STATUS_RECV_PAYLOAD 3
#define STATUS_RECV_CHECKSUM 4
#define STATUS_READY 5
#define STATUS_ERROR_PROTOCOL 6
#define STATUS_ERROR_OVERFLOW 7
#define STATUS_ERROR_CHECKSUM 8
#define STATUS_ERROR_OVERRUN 9
#define STATUS_ERROR_UNEXPECTED 10

typedef struct
{
	BYTE bStatus;
	BYTE bEndpoint;
	DWORD dwLength;
	DWORD dwOffset;
	BYTE bChecksum;
	BYTE abBuffer[CCID_HEADER_LENGTH + CCID_MAX_PAYLOAD_LENGTH];
} CCID_RECEIVER_ST;

static volatile BOOL ccid_receiver_error;
static volatile BYTE ccid_receiver_push_index;
static volatile BYTE ccid_receiver_pop_index;
static CCID_RECEIVER_ST ccid_receivers[2];

static void dump_receiver(CCID_RECEIVER_ST *receiver)
{
	printf("\tbStatus: %d\n", receiver->bStatus);
	printf("\tbEndpoint: %02X\n", receiver->bEndpoint);
	printf("\tabBuffer: ");
	for (DWORD i = 0; i < receiver->dwOffset; i++)
		printf("%02X", receiver->abBuffer[i]);
	printf("\n");
	if (receiver->dwOffset != receiver->dwLength)
		printf("\tOffset is %lu, expected length is %lu\n", receiver->dwOffset, receiver->dwLength);
}

static void dump(void)
{
	printf("fError: %d\n", ccid_receiver_error);
	printf("bPushIndex: %d\n", ccid_receiver_push_index);
	printf("bPopIndex: %d\n", ccid_receiver_pop_index);
	printf("Receiver 0:\n");
	dump_receiver(&ccid_receivers[0]);
	printf("Receiver 1:\n");
	dump_receiver(&ccid_receivers[1]);
}

/**
 * @brief Callback invoked by the UART interrupt when a byte has been received
 * @note As the name says, this function is executed in the context of an ISR
 */
void CCID_LIB(SerialRecvByteFromISR)(BYTE bValue)
{
	CCID_RECEIVER_ST* receiver;

	if (ccid_receiver_error)	
		return; /* Stop receiving until the error is cleared */

	/* Make sure we have a valid receiver */
	ccid_receiver_push_index %= 2;
	receiver = &ccid_receivers[ccid_receiver_push_index];

	switch (receiver->bStatus)
	{
		case STATUS_IDLE:
			if (bValue == START_BYTE)
			{
				/* This is the beginning of a serial CCID message */
				memset(receiver, 0, sizeof(CCID_RECEIVER_ST));
				receiver->bStatus = STATUS_RECV_ENDPOINT;
			}
			else
			{
				/* Invalid byte */
				ccid_receiver_error = TRUE;
				receiver->bStatus = STATUS_ERROR_PROTOCOL;
				CCID_LIB(WakeupFromISR)();				
			}
		break;

		case STATUS_RECV_ENDPOINT:
			/* Init the data */
			receiver->bEndpoint = bValue;
			receiver->bChecksum = bValue;
			/* Now ready to receive the header */
			receiver->dwLength = CCID_HEADER_LENGTH;
			receiver->dwOffset = 0;
			receiver->bStatus = STATUS_RECV_HEADER;
		break;

		case STATUS_RECV_HEADER:
			receiver->bChecksum ^= bValue;
			receiver->abBuffer[receiver->dwOffset++] = bValue;
			if (receiver->dwOffset >= CCID_HEADER_LENGTH)
			{
				DWORD dwLength = utohl(&receiver->abBuffer[CCID_POS_LENGTH]);
				
				if (dwLength > CCID_MAX_PAYLOAD_LENGTH)
				{
					/* Payload will not fit in our buffer */
					ccid_receiver_error = TRUE;
					receiver->bStatus = STATUS_ERROR_OVERFLOW;
					CCID_LIB(WakeupFromISR)();
				}
				else if (dwLength)
				{
					/* Ready to receive the payload */
					receiver->dwLength = CCID_HEADER_LENGTH + dwLength;
					receiver->bStatus = STATUS_RECV_PAYLOAD;						
				}
				else
				{
					/* No payload, ready to receive the checksum */
					receiver->bStatus = STATUS_RECV_CHECKSUM;
				}
			}
		break;

		case STATUS_RECV_PAYLOAD:
			receiver->bChecksum ^= bValue;
			receiver->abBuffer[receiver->dwOffset++] = bValue;
			if (receiver->dwOffset >= receiver->dwLength)
			{
				/* Done with the payload, ready to receive the checksum */
				receiver->bStatus = STATUS_RECV_CHECKSUM;
			}
		break;

		case STATUS_RECV_CHECKSUM:
			receiver->bChecksum ^= bValue;
			if (!receiver->bChecksum)
			{
				/* Checkum is OK */
				receiver->bStatus = STATUS_READY;
				/* Toggle */
				ccid_receiver_push_index = 1 - ccid_receiver_push_index;
				/* Wakeup the application */
				CCID_LIB(WakeupFromISR)();
			}
			else
			{
				ccid_receiver_error = TRUE;
				receiver->bStatus = STATUS_ERROR_CHECKSUM;
				CCID_LIB(WakeupFromISR)();
			}
		break;

		case STATUS_READY:
			ccid_receiver_error = TRUE;			
			dump();
			printf("ERROR OVERRUN  !!! %02X %d\n", bValue, receiver->bStatus);			
			receiver->bStatus = STATUS_ERROR_OVERRUN;
			CCID_LIB(WakeupFromISR)();
		break;

		default:
			receiver->bStatus = STATUS_ERROR_UNEXPECTED;
			CCID_LIB(WakeupFromISR)();
	}
}

/**
 * @brief Retrieve the last packet received from the coupler.
 * @note This function blocks until a message is available are a timeout occurs.
 */
LONG CCID_LIB(SerialRecv)(CCID_PACKET_ST* packet, DWORD timeout_ms)
{
	LONG rc = SCARD_ERR(S_SUCCESS);
	CCID_RECEIVER_ST* receiver;

	if (packet == NULL)
		return SCARD_ERR(E_INVALID_PARAMETER);

	ccid_receiver_pop_index %= 2;
	receiver = &ccid_receivers[ccid_receiver_pop_index];

	CCID_LIB(ClearWakeup)();
		
	if (receiver->bStatus != STATUS_READY)
	{
		/* Wait until a message arrives */
		if (!CCID_LIB(WaitWakeup)(timeout_ms))
			rc = SCARD_ERR(E_TIMEOUT);
	}

	if (rc == SCARD_ERR(S_SUCCESS))
	{
		switch (receiver->bStatus)
		{
			case STATUS_READY:
				/* This is the expected situation */
			break;
			case STATUS_IDLE:
				rc = SCARD_ERR(E_NOT_READY); /* Nothing received */
			break;
			case STATUS_ERROR_PROTOCOL:
				rc = SCARD_ERR(E_READER_UNSUPPORTED); /* Wrong protocol */
			break;
			case STATUS_ERROR_CHECKSUM:
				rc = SCARD_ERR(F_COMM_ERROR); /* Wrong checksum */
			break;
			case STATUS_ERROR_OVERFLOW:
				rc = SCARD_ERR(E_NO_MEMORY); /* Internal buffer is too short */
			break;
			case STATUS_ERROR_OVERRUN:
				rc = SCARD_ERR(F_INTERNAL_ERROR); /* Sequence was incoherent */
			break;
			default:
				rc = SCARD_ERR(E_UNEXPECTED); /* Oops? Whats going on */
		}
	}

	if (ccid_receiver_error)
	{
		/* Make sure the application knows there is an error */
		if (rc == SCARD_ERR(S_SUCCESS))
			rc = SCARD_ERR(F_UNKNOWN_ERROR);

		/* Cleanup */
		memset(ccid_receivers, 0, sizeof(ccid_receivers));
		ccid_receiver_push_index = 0;
		ccid_receiver_pop_index = 0;
		ccid_receiver_error = FALSE;
	}

	if (rc == SCARD_ERR(S_SUCCESS))
	{
		packet->bEndpoint = receiver->bEndpoint;
		memcpy(packet->Header.u, receiver->abBuffer, CCID_HEADER_LENGTH);
		packet->Header.p.Length.dw = utohl(&receiver->abBuffer[CCID_POS_LENGTH]);

		if (packet->bEndpoint == CCID_COMM_CONTROL_TO_PC)
		{
			packet->Header.p.Data.Control.Value.w = utohs(packet->Header.p.Data.Control.Value.ab);
			packet->Header.p.Data.Control.Index.w = utohs(packet->Header.p.Data.Control.Index.ab);
		}

		if (packet->Header.p.Length.dw)
		{
			if ((packet->abRecvPayload == NULL) || (packet->dwRecvPayloadMaxLen < packet->Header.p.Length.dw))
			{
				rc = SCARD_ERR(E_INSUFFICIENT_BUFFER);
			}
			else
			{
				memcpy(packet->abRecvPayload, &receiver->abBuffer[CCID_HEADER_LENGTH], packet->Header.p.Length.dw);
			}
		}
	}
	else
	{
		dump();
	}

	/* This receiver is ready to receive again */
	receiver->bStatus = STATUS_IDLE;
	/* And next time we'll read the other */
	ccid_receiver_pop_index = 1 - ccid_receiver_pop_index;

	D(printf("\n"));
	return rc;
}
