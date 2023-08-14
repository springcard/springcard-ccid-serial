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
 * @file win32_hal.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Hardware abstraction layer for the CCID serial driver, Windows implementation
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 *
 * @addtogroup ccid
 */

#include <project.h>

#include "../../pcsc-serial.h"
#include "../../ccid/ccid_hal.h"
#include "../../scard/scard_errors.h"

static BOOL ccid_serial_configure(void);
static BOOL ccid_serial_flush(void);
static DWORD WINAPI ccid_serial_recv_task(void* unused);

static HANDLE hComm = INVALID_HANDLE_VALUE;
static HANDLE hThread = INVALID_HANDLE_VALUE;
static HANDLE hEvent = INVALID_HANDLE_VALUE;

static const char* ccid_comm_name;

/**
 * @brief Prepare the serial library, specifying the serial comm port
 * @note This function may have a different prototype, depending on the OS/target requirements
 */
void CCID_LIB(SerialInit)(const char* szCommName)
{
	ccid_comm_name = szCommName;
}

/**
 * @brief Open the serial comm port and activate the RX interrupt
 * @note This function must be implemented specifically for the OS/target
 */
BOOL CCID_LIB(SerialOpen)(void)
{
	CCID_LIB(SerialClose)();
	
	if (ccid_comm_name == NULL)
		return FALSE;
	
	if (hComm == INVALID_HANDLE_VALUE)
	{
		hComm = CreateFile(ccid_comm_name, GENERIC_READ | GENERIC_WRITE, 0,  // comm devices must be opened w/exclusive-access
			NULL,         // no security attributes
			OPEN_EXISTING,  // comm devices must use OPEN_EXISTING
			0,            // not overlapped I/O
			NULL          // hTemplate must be NULL for comm devices
		);

		if (hComm == INVALID_HANDLE_VALUE)
		{
			printf("Failed to open comm. port '%s' (%lu)\n", ccid_comm_name, GetLastError());
			CCID_LIB(SerialClose)();
			return FALSE;
		}
	}

	if (!ccid_serial_configure())
	{
		printf("Failed to configure the comm. port '%s' (%lu)\n", ccid_comm_name, GetLastError());
		CCID_LIB(SerialClose)();
		return FALSE;
	}

	if (!ccid_serial_flush())
	{
		printf("Failed to flush the comm. port '%s' (%lu)\n", ccid_comm_name, GetLastError());
		CCID_LIB(SerialClose)();
		return FALSE;
	}

	if (hEvent == INVALID_HANDLE_VALUE)
	{
		hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEvent == INVALID_HANDLE_VALUE)
		{
			CCID_LIB(SerialClose)();
			return FALSE;
		}
	}

	if (hThread == INVALID_HANDLE_VALUE)
	{
		DWORD dwId;

		hThread = CreateThread(NULL, 0, ccid_serial_recv_task, NULL, 0, &dwId);
		if (hThread == INVALID_HANDLE_VALUE)
		{
			printf("Failed to create a thread to simulate the RX ISR (%lu)\n", GetLastError());
			CCID_LIB(SerialClose)();
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * @brief Close the serial comm port
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(SerialClose)(void)
{
	if (hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
	}
	
	if (hThread != INVALID_HANDLE_VALUE)
	{
		TerminateThread(hThread, 0);
		CloseHandle(hThread);
		hThread = INVALID_HANDLE_VALUE;
	}

	if (hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEvent);
		hEvent = INVALID_HANDLE_VALUE;
	}	
}

/**
 * @brief Returns TRUE if the serial comm port is open, FALSE otherwise
 * @note This function must be implemented specifically for the OS/target
 */
BOOL CCID_LIB(SerialIsOpen)(void)
{
	if (hEvent == INVALID_HANDLE_VALUE)
		return FALSE;	
	if (hThread == INVALID_HANDLE_VALUE)
		return FALSE;
	if (hComm == INVALID_HANDLE_VALUE)
		return FALSE;
	return TRUE;
}

/**
 * @brief Send one byte to the device through the UART
 * @note This function must be implemented specifically for the OS/target. It is acceptable to block the caller.
 */
BOOL CCID_LIB(SerialSendByte)(BYTE bValue)
{
	DWORD dwWritten;

	if (!WriteFile(hComm, &bValue, 1, &dwWritten, NULL) || (dwWritten == 0))
	{
		printf("WriteFile failed (%lu)\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief Send a buffer to the device through the UART
 * @note This function must be implemented specifically for the OS/target. It is acceptable to block the caller.
 */
BOOL CCID_LIB(SerialSendBytes)(const BYTE* abValue, DWORD dwLength)
{
	DWORD dwWritten;

	if (!WriteFile(hComm, abValue, dwLength, &dwWritten, NULL) || (dwWritten < dwLength))
	{
		printf("WriteFile failed (%lu)\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief Notify the task/thread waiting over CCID_WaitWakeup that a message is available
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(WakeupFromISR)(void)
{
	SetEvent(hEvent);
}

/**
 * @brief Start waiting for a message
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(ClearWakeup)(void)
{
	ResetEvent(hEvent);
}

/**
 * @brief Wait until a message is available or a timeout occurs
 * @note This function must be implemented specifically for the OS/target
 */
BOOL CCID_LIB(WaitWakeup)(DWORD timeout_ms)
{
	DWORD rc;

	rc = WaitForSingleObject(hEvent, timeout_ms);
	if (rc == WAIT_OBJECT_0)
	{
		return TRUE;
	}

	return FALSE;
}

/**
 * @brief Receive bytes coming from the CCID device; call CCID_RecvByteFromISR every time a byte arrives
 * @note This function must be implemented specifically for the OS/target. It is namely the UART's RX ISR.
 */
static DWORD WINAPI ccid_serial_recv_task(void* unused)
{
	DWORD dwRead;
	BYTE bValue;
	(void)unused;

	for (;;)
	{
		if (!ReadFile(hComm, &bValue, 1, &dwRead, 0))
		{
			printf("ReadFile failed (%lu)\n", GetLastError());
			CloseHandle(hComm);
			hComm = INVALID_HANDLE_VALUE;
			break;
		}

		if (dwRead)
		{
			CCID_LIB(SerialRecvByteFromISR)(bValue);
		}
	}

	return 0;
}

/**
 * @brief Configure the UART for CCID operation
 * @note This function must be implemented specifically for the OS/target
 */
static BOOL ccid_serial_configure(void)
{
	DCB dcb;
	COMMTIMEOUTS tmo;

	if (!GetCommState(hComm, &dcb))
		return FALSE;

	dcb.BaudRate = CBR_38400;

	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fNull = FALSE;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.fRtsControl = RTS_CONTROL_ENABLE; /* New 1.73 : for compatibility with RL78 flash board */
	dcb.fDtrControl = DTR_CONTROL_ENABLE; /* New 1.73 : for compatibility with RL78 flash board */

	dcb.fAbortOnError = TRUE;
	dcb.fTXContinueOnXoff = TRUE;

	if (!SetCommState(hComm, &dcb))
		return FALSE;

	tmo.ReadIntervalTimeout = 0;
	tmo.ReadTotalTimeoutConstant = 10;
	tmo.ReadTotalTimeoutMultiplier = 1;
	tmo.WriteTotalTimeoutConstant = 0;
	tmo.WriteTotalTimeoutMultiplier = 0;

	if (!SetCommTimeouts(hComm, &tmo))
		return FALSE;

	return TRUE;
}

/**
 * @brief Flush the UART
 * @note This function must be implemented specifically for the OS/target
 */
static BOOL ccid_serial_flush(void)
{
	return PurgeComm(hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);
}

