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
 * @file scard_errors.h
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Error codes used by the PC/SC-Like stack. They are inspired by WinSCard.h.
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @addtogroup scard
 */

#ifndef __SCARD_ERRORS_H__
#define __SCARD_ERRORS_H__

/* Windows headers have an 'E_UNEXPECTED' define that collides with the following enum */
#if (defined(E_UNEXPECTED))
#undef E_UNEXPECTED
#endif

typedef enum
{
	SCARD_ERR(S_SUCCESS)               = 0x0000,
	SCARD_ERR(F_INTERNAL_ERROR)        = 0x0001,
	SCARD_ERR(E_CANCELLED)             = 0x0002,
	SCARD_ERR(E_INVALID_HANDLE)        = 0x0003,
	SCARD_ERR(E_INVALID_PARAMETER)     = 0x0004,
	SCARD_ERR(E_INVALID_TARGET)        = 0x0005,
	SCARD_ERR(E_NO_MEMORY)             = 0x0006,
	SCARD_ERR(F_WAITED_TOO_LONG)       = 0x0007,
	SCARD_ERR(E_INSUFFICIENT_BUFFER)   = 0x0008,
	SCARD_ERR(E_UNKNOWN_READER)        = 0x0009,
	SCARD_ERR(E_TIMEOUT)               = 0x000A,
	SCARD_ERR(E_SHARING_VIOLATION)     = 0x000B,
	SCARD_ERR(E_NO_SMARTCARD)          = 0x000C,
	SCARD_ERR(E_UNKNOWN_CARD)          = 0x000D,
	SCARD_ERR(E_CANT_DISPOSE)          = 0x000E,
	SCARD_ERR(E_PROTO_MISMATCH)        = 0x000F,
	SCARD_ERR(E_NOT_READY)             = 0x0010,
	SCARD_ERR(E_INVALID_VALUE)         = 0x0011,
	SCARD_ERR(E_SYSTEM_CANCELLED)      = 0x0012,
	SCARD_ERR(F_COMM_ERROR)            = 0x0013,
	SCARD_ERR(F_UNKNOWN_ERROR)         = 0x0014,
	SCARD_ERR(E_INVALID_ATR)           = 0x0015,
	SCARD_ERR(E_NOT_TRANSACTED)        = 0x0016,
	SCARD_ERR(E_READER_UNAVAILABLE)    = 0x0017,
	SCARD_ERR(E_P_SHUTDOWN)            = 0x0018,
	SCARD_ERR(E_PCI_TOO_SMALL)         = 0x0019,
	SCARD_ERR(E_READER_UNSUPPORTED)    = 0x001A,
	SCARD_ERR(E_DUPLICATE_READER)      = 0x001B,
	SCARD_ERR(E_CARD_UNSUPPORTED)      = 0x001C,
	SCARD_ERR(E_NO_SERVICE)            = 0x001D,
	SCARD_ERR(E_SERVICE_STOPPED)       = 0x001E,
	SCARD_ERR(E_UNEXPECTED)            = 0x001F,
	SCARD_ERR(E_UNSUPPORTED_FEATURE)   = 0x0022,
	SCARD_ERR(E_NO_READERS_AVAILABLE)  = 0x002E,
	SCARD_ERR(E_COMM_DATA_LOST)        = 0x002F,
	SCARD_ERR(W_UNSUPPORTED_CARD)      = 0x0065,
	SCARD_ERR(W_UNRESPONSIVE_CARD)     = 0x0066,
	SCARD_ERR(W_UNPOWERED_CARD)        = 0x0067,
	SCARD_ERR(W_RESET_CARD)            = 0x0068,
	SCARD_ERR(W_REMOVED_CARD)          = 0x0069,
	SCARD_ERR(W_INSERTED_CARD)         = 0x006A
} SCARD_LIB_RC;

#endif
