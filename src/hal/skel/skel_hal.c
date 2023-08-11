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
 * @file skel_hal.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Hardware abstraction layer for the CCID serial driver, abstract implementation to be completed by the integrator
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 *
 * @addtogroup ccid
 */

#include <project.h>

#include "../../pcsc-serial.h"
#include "../../ccid/ccid_hal.h"
#include "../../scard/scard_errors.h"

/**
 * @todo Write your UART RX interrupt handler so that CCID_LIB(SerialRecvByteFromISR)(bValue) is called everytime a byte is received
 */

/**
 * @brief Prepare the serial library
 * @note This function may have a different prototype, depending on the OS/target requirements
 */
void CCID_LIB(SerialInit)(void)
{

}

/**
 * @brief Open the serial comm port and activate the RX interrupt
 * @note This function must be implemented specifically for the OS/target
 */
BOOL CCID_LIB(SerialOpen)(void)
{

}

/**
 * @brief Close the serial comm port
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(SerialClose)(void)
{

}

/**
 * @brief Returns TRUE if the serial comm port is open, FALSE otherwise
 * @note This function must be implemented specifically for the OS/target
 */
BOOL CCID_LIB(SerialIsOpen)(void)
{

}

/**
 * @brief Send one byte to the device through the UART
 * @note This function must be implemented specifically for the OS/target. It is acceptable to block the caller.
 */
BOOL CCID_LIB(SerialSendByte)(BYTE bValue)
{

}

/**
 * @brief Send a buffer to the device through the UART
 * @note This function must be implemented specifically for the OS/target. It is acceptable to block the caller.
 */
BOOL CCID_LIB(SerialSendBytes)(const BYTE* abValue, DWORD dwLength)
{
	for (DWORD i = 0; i < dwLength; i++)
		if (!CCID_LIB(SerialSendByte)(abValue[i]))
			return FALSE;

	return TRUE;
}

/**
 * @todo Optimize this part if you have a kernel
 */

static volatile BOOL fWakeup;

/**
 * @brief Notify the task/thread waiting over CCID_WaitWakeup that a message is available
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(WakeupFromISR)(void)
{
	fWakeup = TRUE;
}

/**
 * @brief Start waiting for a message
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(ClearWakeup)(void)
{
	fWakeup = FALSE;
}

/**
 * @brief Wait until a message is available or a timeout occurs
 * @note This function must be implemented specifically for the OS/target
 */
BOOL CCID_LIB(WaitWakeup)(DWORD timeout_ms)
{
	while (!fWakeup)
	{
		if (timeout_ms != (DWORD) -1) /* (DWORD) -1 is INFINITE */
			if (timeout_ms-- == 0) return FALSE;
		sleep_ms(1);
	}
	return TRUE;
}

