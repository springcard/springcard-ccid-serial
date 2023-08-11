#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>

typedef bool BOOL;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef long LONG;

#if (!defined(TRUE))
	#define TRUE 1
#endif

#if (!defined(FALSE))
	#define FALSE 0
#endif

#endif