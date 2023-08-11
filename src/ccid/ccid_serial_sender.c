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
 * @file ccid_serial_sender.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Implementation of the CCID driver, TX part
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 *
 * @addtogroup ccid
 */

#include "ccid_i.h"

/**
 * @brief Send a CCID packet to the coupler
 */
LONG CCID_LIB(SerialSend)(CCID_PACKET_ST* packet)
{
	DWORD i, dwSendPayloadLength;
	BYTE bChecksum;

	if (packet == NULL)
		return SCARD_ERR(E_INVALID_PARAMETER);

	dwSendPayloadLength = packet->Header.p.Length.dw;
	htoul(packet->Header.p.Length.ab, dwSendPayloadLength);

	if (packet->bEndpoint == CCID_COMM_CONTROL_TO_RDR)
	{
		htous(packet->Header.p.Data.Control.Value.ab, packet->Header.p.Data.Control.Value.w);
		htous(packet->Header.p.Data.Control.Index.ab, packet->Header.p.Data.Control.Index.w);
	}

	if ((dwSendPayloadLength != 0) && (packet->abSendPayload == NULL))
		return SCARD_ERR(E_INVALID_PARAMETER);

	bChecksum = packet->bEndpoint;
	for (i = 0; i < CCID_HEADER_LENGTH; i++)
		bChecksum ^= packet->Header.u[i];

	if (packet->abSendPayload != NULL)
		for (i = 0; i < dwSendPayloadLength; i++)
			bChecksum ^= packet->abSendPayload[i];

	if (!CCID_LIB(SerialSendByte)(START_BYTE))
		return SCARD_ERR(F_COMM_ERROR);
	if (!CCID_LIB(SerialSendByte)(packet->bEndpoint))
		return SCARD_ERR(F_COMM_ERROR);
	if (!CCID_LIB(SerialSendBytes)(packet->Header.u, CCID_HEADER_LENGTH))
		return SCARD_ERR(F_COMM_ERROR);
	if (packet->abSendPayload != NULL)
		if (!CCID_LIB(SerialSendBytes)(packet->abSendPayload, dwSendPayloadLength))
			return SCARD_ERR(F_COMM_ERROR);
	if (!CCID_LIB(SerialSendByte)(bChecksum))
		return SCARD_ERR(F_COMM_ERROR);

	D(printf("\n"));
	return SCARD_ERR(S_SUCCESS);
}
