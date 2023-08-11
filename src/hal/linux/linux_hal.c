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
 * @file linux_hal.c
 * @author johann.d, SpringCard
 * @date 2023-08-11
 * @brief Hardware abstraction layer for the CCID serial driver, Linux implementation
 * 
 * @copyright Copyright (c) SpringCard SAS, France, 2015-2023
 *
 * @addtogroup ccid
 */

#include <project.h>

#include "../../pcsc-serial.h"
#include "../../ccid/ccid_hal.h"
#include "../../scard/scard_errors.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

static const char* ccid_comm_name;

static volatile BOOL ccid_comm_open;
static volatile BOOL ccid_wakeup_flag;

static int ccid_comm_handle = -1;
static int ccid_thread_handle = -1;
static pthread_t ccid_thread_id;
static pthread_mutex_t ccid_wakeup_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ccid_wakeup_cond = PTHREAD_COND_INITIALIZER;


static void* ccid_serial_recv_task(void* arg);
static BOOL ccid_serial_configure(void);
static BOOL ccid_serial_flush(void);

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

	ccid_comm_handle = open(ccid_comm_name, O_RDWR | O_NOCTTY);

	if (ccid_comm_handle < 0)
	{
		perror("open");
		CCID_LIB(SerialClose)();
		return FALSE;
	}
	
	/* Configure UART */
	if (!ccid_serial_configure())
	{
		CCID_LIB(SerialClose)();
		return FALSE;
	}
	
	/* Flush UART */
	if (!ccid_serial_flush())
	{
		CCID_LIB(SerialClose)();
		return FALSE;
	}
	
	/* Ready */
	ccid_comm_open = TRUE;
	
	/* Create the receiver thread */
	ccid_thread_handle = pthread_create(&ccid_thread_id, NULL, ccid_serial_recv_task, &ccid_comm_handle);
	if (ccid_thread_handle < 0)
	{
		perror("pthread_create");
		CCID_LIB(SerialClose)();
		return FALSE;
	}
	
	return TRUE;
}

/**
 * @brief Close the serial comm port
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(SerialClose)(void)
{
	ccid_comm_open = FALSE;
	
	if (ccid_comm_handle >= 0)
		close(ccid_comm_handle);
	
	ccid_comm_handle = -1;
	
	if (ccid_thread_handle >= 0)
	{
		pthread_join(ccid_thread_id, NULL);
		ccid_thread_handle = -1;
	}
}

/**
 * @brief Returns TRUE if the serial comm port is open, FALSE otherwise
 * @note This function must be implemented specifically for the OS/target
 */
BOOL CCID_LIB(SerialIsOpen)(void)
{
	return ccid_comm_open;
}

/**
 * @brief Send one byte to the device through the UART
 * @note This function must be implemented specifically for the OS/target. It is acceptable to block the caller.
 */
BOOL CCID_LIB(SerialSendByte)(BYTE bValue)
{
	int done;

	done = write(ccid_comm_handle, &bValue, 1);
	if (done <= 0)
	{
		perror("write");
		return FALSE;
	}

	D(printf("-%02X", bValue));
	return TRUE;
}

/**
 * @brief Send a buffer to the device through the UART
 * @note This function must be implemented specifically for the OS/target. It is acceptable to block the caller.
 */
BOOL CCID_LIB(SerialSendBytes)(const BYTE* abValue, DWORD dwLength)
{
	int done = 0;
	int tosend;
	int offset;

	tosend = dwLength;
	offset = 0;
	while (tosend)
	{
		done = write(ccid_comm_handle, &abValue[offset], tosend);
		if (done <= 0)
		{
			perror("write");
			return FALSE;
		}
		tosend -= done;
		offset += done;
	}

	D(for (DWORD i = 0; i < dwLength; i++) printf("-%02X", abValue[i]));

	return TRUE;
}

/**
 * @brief Notify the task/thread waiting over CCID_WaitWakeup that a message is available
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(WakeupFromISR)(void)
{
	pthread_mutex_lock(&ccid_wakeup_mutex);
	D(printf(" [WAKEUP]\n"));
	ccid_wakeup_flag = TRUE;
	pthread_cond_signal(&ccid_wakeup_cond);
	pthread_mutex_unlock(&ccid_wakeup_mutex);
}

/**
 * @brief Start waiting for a message
 * @note This function must be implemented specifically for the OS/target
 */
void CCID_LIB(ClearWakeup)(void)
{
	pthread_mutex_lock(&ccid_wakeup_mutex);
	D(printf("[CLEAR]\n"));
	ccid_wakeup_flag = FALSE;
	pthread_mutex_unlock(&ccid_wakeup_mutex);
}

/**
 * @brief Wait until a message is available or a timeout occurs
 * @note This function must be implemented specifically for the OS/target
 */
BOOL CCID_LIB(WaitWakeup)(DWORD timeout_ms)
{
	struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec  += 2; // timeout_ms / 1000;
	//ts.tv_nsec += (timeout_ms % 1000) * 1000 * 1000;
	(void) timeout_ms;

	pthread_mutex_lock(&ccid_wakeup_mutex);
	while (!ccid_wakeup_flag)
	{
		int rc;
		rc = pthread_cond_timedwait(&ccid_wakeup_cond, &ccid_wakeup_mutex, &ts);
		if (rc == ETIMEDOUT)
		{
			D(printf("[TIMEOUT]\n"));
			pthread_mutex_unlock(&ccid_wakeup_mutex);
			return FALSE;
		}
		if (rc < 0)
		{
			perror("pthread_cond_timedwait");
			return FALSE;
		}
	}
	D(printf("[READY]\n"));
	pthread_mutex_unlock(&ccid_wakeup_mutex);
	return TRUE;
}

/**
 * @brief Receive bytes coming from the CCID device; call CCID_RecvByteISR every time a byte arrives
 * @note This function must be implemented specifically for the OS/target. It is namely the UART's RX ISR.
 */
static void* ccid_serial_recv_task(void* arg)
{
	int fd = *(int *)arg;
	
    while (ccid_comm_open)
	{
		BYTE bValue;
        if (read(fd, &bValue, 1) > 0)
		{
			D(printf("+%02X", bValue));
			CCID_LIB(SerialRecvByteFromISR)(bValue);
        }
    }
    return NULL;
}	

/**
 * @brief Configure the UART for CCID operation
 * @note This function must be implemented specifically for the OS/target
 */
static BOOL ccid_serial_configure(void)
{
	struct termios newtio;
  
	bzero(&newtio, sizeof(newtio));
	// CS8  = 8n1 (8bit,no parity,1 stopbit
	// CLOCAL= local connection, no modem control
	// CREAD  = enable receiving characters
	newtio.c_cflag = CS8 | CLOCAL | CREAD;
	// 38400bps
	newtio.c_cflag |= B38400;
	// no parity, no flow control
	newtio.c_iflag = IGNPAR | IGNBRK;
	newtio.c_oflag = 0;

	// set input mode (non-canonical, no echo,...) 
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0;       // inter-character timer unused
	newtio.c_cc[VMIN]  = 1;        // blocking read until 1 chars received

	if (tcsetattr(ccid_comm_handle, TCSANOW, &newtio))
	{
		perror("tcsetattr");
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief Flush the UART
 * @note This function must be implemented specifically for the OS/target
 */
static BOOL ccid_serial_flush(void)
{
	if (tcflush(ccid_comm_handle, TCIFLUSH))
	{
		perror("tcflush");
		return FALSE;		
	}
	return TRUE;
}
