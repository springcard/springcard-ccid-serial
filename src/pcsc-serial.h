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
 * @file pcsc-serial.h
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Centralized configuration of the PC/SC-Like library and of the CCID driver
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

#ifndef __PCSC_SERIAL_H__
#define __PCSC_SERIAL_H__

#define CONCAT(prefix, name) prefix ## name

/* Static configuration of the PC/SC-Like stack and of the CCID driver */
/* ------------------------------------------------------------------- */

/**
 * @brief Macro to set the namespace (prefix) of the CCID driver.
 * Default prefix is "CCID_", but you may change it for any other
 */
#define CCID_LIB(name) CONCAT(CCID_, name)

/**
 * @brief Macro to set the namespace (prefix) of the PC/SC-Like stack library (functions).
 * Typically this would be "SCard", but when building a software that needs to use both this PC/SC-Like stack and an "official" PC/SC stack, this would lead to a collision with the system's includes and libraries.
 * Default is therefore is "SCARD_", but you may change it for any other
 */
#define SCARD_LIB(name) CONCAT(SCARD_, name)

/**
 * @brief Macro to set the namespace (prefix) of the PC/SC-Like stack library (error codes).
 * Typically this would be "SCARD_", but when building a software that needs to use both this PC/SC-Like stack and an "official" PC/SC stack, this would lead to a collision with the system's includes and libraries.
 * Default is therefore "SCARD_ERR_", but you may change it for any other prefix
 */
#define SCARD_ERR(name) CONCAT(SCARD_ERR_, name)

 /**
  * @brief Max number of slots supported by the library.
  * @note If only the contactless slot is used, 1 is enough
  */
#define CCID_MAX_SLOT_COUNT 6

/**
 * @brief Max payload size of the CCID buffers.
 * SpringCore devices support extended APDUs with up to 64kB of data. This makes CCID payloads up to 65545B (see ISO/IEC 7816-4).
 * Of course, when the host is a low-end MCU, it is impossible to provide that amount of memory.
 * Considering short APDUs (255B of data) is generally enough; this makes CCID payloads up to 261B.
 * You may even use smaller buffers, provided that your APDUs (command AND response) always fit.
 * In other words, if you do know exactly the transactions you'll be doing with the card(s), you may reduce the size required by the PC/SC-Like stack and the CCID driver.
 */
#define CCID_MAX_PAYLOAD_LENGTH 261

/**
 * @brief Max payload size of the CCID interrupt buffers.
 * This is 2 bits per slot. 4 will fit any device.
 */
#define CCID_MAX_INTERRUPT_PAYLOAD_LENGTH 4

/* Dynamic configuration of the PC/SC-Like stack and of the CCID driver */
/* -------------------------------------------------------------------- */

/**
 * @brief Does the CCID driver use the notifications? If no, the PC/SC-Like stack must poll the status of the slots
 */
extern BOOL fCcidUseNotifications;

/**
 * @brief Does the sample software test the communication using the ECHO Escape command (SCARD_Control)?
 */
extern BOOL fTestEchoControl;

/**
 * @brief Does the sample software test the communication using the ECHO APDU (SCARD_Transmit)?
 */
extern BOOL fTestEchoTransmit;

/**
 * @brief Does the library print debug messages?
 */
extern BOOL fVerbose;

 /* Debug messages */
/* -------------- */

/* To control debug messages */
#define D(x) if (fVerbose) do { x; } while(0)
/* To totally disable debug messages */
// #define D(x)

#endif
