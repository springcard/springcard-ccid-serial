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
 * @file scard.h
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Public functions of the PC/SC-Like library
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @defgroup scard Pseudo PC/SC stack for embedded systems, to be used with a CCID over serial device
 *
 * The detailed documentation is provided will be provided in an application note.
 */

#ifndef __SCARD_H__
#define __SCARD_H__

/* Core functions, counterpart to PC/SC standard functions */
/* ------------------------------------------------------- */

LONG SCARD_LIB(Status)(BYTE bSlot, BOOL* pfCardPresent, BOOL* pfCardPowered);
LONG SCARD_LIB(Connect)(BYTE bSlot, BYTE abAtr[], DWORD *pdwAtrLength);
LONG SCARD_LIB(Disconnect)(BYTE bSlot);
LONG SCARD_LIB(Transmit)(BYTE bSlot, const BYTE abSendApdu[], DWORD dwSendLength, BYTE abRecvApdu[], DWORD *pwRecvLength);
LONG SCARD_LIB(Control)(const BYTE abSendBuffer[], DWORD dwSendLength, BYTE abRecvBuffer[], DWORD* pdwRecvLength);

LONG SCARD_LIB(GetStatusChange)(DWORD dwTimeoutMs);
LONG SCARD_LIB(GetStatusChangeEx)(DWORD dwTimeoutMs, DWORD* pdwPresentSlots, DWORD* pdwChangedSlots);

/* Functions specific to the PC/SC-Like library */
/* -------------------------------------------- */

void SCARD_LIB(Init)(void);
LONG SCARD_LIB(IsValid)(void);
BOOL SCARD_LIB(IsFatalError)(LONG rc);

/* Error codes */
/* ----------- */

#include "scard_errors.h"

#endif
