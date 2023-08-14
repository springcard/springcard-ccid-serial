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
 * @file scard_helpers.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Helpers functions of the PC/SC-Like library
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @addtogroup scard
 */

#include "scard_i.h"

static BOOL scard_valid;

/**
 * @internal
 * @brief Report an error
 */
void scard_raise_error(const char* msg)
{
	printf("\nError in PC/SC-Like stack: %s\n", msg);
	scard_valid = FALSE;
}

/**
 * @brief Initialize the PC/SC-Like stack (e.g. clear the state machine, clear all status variables)
 */
void SCARD_LIB(Init)(void)
{
	scard_valid = TRUE;
}

/**
 * @brief Are the PC/SC-like stack and device available?
 * This function returns TRUE if the device is up and running, and FALSE in the following situations:
 * - no device has been activated, ever
 * - a device has been activated, but a fatal error has been encountered
 * - an error has been encountered by the library
 * @return BOOL 
 */
BOOL SCARD_LIB(IsValidContext)(void)
{
	if (scard_valid)
	{
		/* Hook to be able to change the status (not used on all targets) */
		if (SCARD_LIB(IsCancelledHook)())
		{
			printf("PC/SC-Like context not valid, operation has been cancelled by the user\n");
			scard_valid = FALSE;
		}
		if (!CCID_LIB(IsValidDriver)())
		{
			printf("PC/SC-Like context not valid, the CCID driver has reported an error\n");
			scard_valid = FALSE;
		}		
		if (!CCID_LIB(SerialIsOpen)())
		{
			printf("PC/SC-Like context not valid, CCID serial port is not open\n");
			scard_valid = FALSE;
		}
	}
	return scard_valid;
}

/**
 * @brief Tells whether an error denotes a fatal communication error with the CCID device, or is only related to the card status
 */
BOOL SCARD_LIB(IsFatalError)(LONG rc)
{
	switch (rc)
	{
		case SCARD_ERR(S_SUCCESS):
		case SCARD_ERR(W_UNSUPPORTED_CARD):
		case SCARD_ERR(W_UNRESPONSIVE_CARD):
		case SCARD_ERR(W_UNPOWERED_CARD):
		case SCARD_ERR(W_RESET_CARD):
		case SCARD_ERR(W_REMOVED_CARD):
		case SCARD_ERR(W_INSERTED_CARD):
		case SCARD_ERR(E_NO_SMARTCARD):
		case SCARD_ERR(E_SHARING_VIOLATION):
		case SCARD_ERR(E_PROTO_MISMATCH):
		case SCARD_ERR(E_UNKNOWN_CARD):
		case SCARD_ERR(E_INVALID_ATR):
			return FALSE;

		default:
			return TRUE;
	}
}

