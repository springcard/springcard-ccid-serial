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
 * @file ccid_constants.h
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Constants used by the CCID driver. Most values come from the the CCID or USB standards.
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @addtogroup ccid
 */

#ifndef __CCID_CONSTANTS_H__
#define __CCID_CONSTANTS_H__

#define CCID_COMM_CONTROL_TO_RDR      0x00
#define CCID_COMM_CONTROL_TO_PC       0x80
#define CCID_COMM_BULK_PC_TO_RDR      0x02
#define CCID_COMM_BULK_RDR_TO_PC      0x81
#define CCID_COMM_INTERRUPT_RDR_TO_PC 0x83

#define GET_STATUS               0x00
#define GET_DESCRIPTOR           0x06
#define SET_CONFIGURATION        0x09

#define PC_TO_RDR_ICCPOWERON     0x62
#define PC_TO_RDR_ICCPOWEROFF    0x63
#define PC_TO_RDR_GETSLOTSTATUS  0x65
#define PC_TO_RDR_ESCAPE         0x6B
#define PC_TO_RDR_XFRBLOCK       0x6F

#define RDR_TO_PC_INTERRUPT      0x50
#define RDR_TO_PC_DATABLOCK      0x80
#define RDR_TO_PC_SLOTSTATUS     0x81
#define RDR_TO_PC_ESCAPE         0x83

#define CONTROL_TIMEOUT 200
#define BULK_TIMEOUT    1200

#define START_BYTE 0xCD


#define CCID_HEADER_LENGTH 10
#define CCID_POS_LENGTH 1

#endif
