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
 * @file ccid_convert.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Serialize/deserialize WORD and DWORD values as per USB and CCID standards.
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 */

/**
 * @addtogroup ccid
 */

#include "ccid_i.h"

void htoul(BYTE abBuffer[], DWORD dwValue)
{
	abBuffer[0] = (BYTE)(dwValue & 0x000000FF); dwValue /= 0x00000100;
	abBuffer[1] = (BYTE)(dwValue & 0x000000FF); dwValue /= 0x00000100;
	abBuffer[2] = (BYTE)(dwValue & 0x000000FF); dwValue /= 0x00000100;
	abBuffer[3] = (BYTE)(dwValue & 0x000000FF);
}

void htous(BYTE abBuffer[], WORD wValue)
{
	abBuffer[0] = (BYTE)(wValue & 0x00FF); wValue /= 0x0100;
	abBuffer[1] = (BYTE)(wValue & 0x00FF);
}

DWORD utohl(const BYTE abBuffer[])
{
	DWORD r;
	r = abBuffer[3]; r *= 0x00000100;
	r += abBuffer[2]; r *= 0x00000100;
	r += abBuffer[1]; r *= 0x00000100;
	r += abBuffer[0];
	return r;
}

WORD utohs(const BYTE abBuffer[])
{
	WORD r;
	r = abBuffer[1]; r *= 0x0100;
	r += abBuffer[0];
	return r;
}

