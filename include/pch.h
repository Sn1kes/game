#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0601
#include <sysinfoapi.h>
#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <fenv.h>

#ifdef __GNUC__
#define ALIGN(N)  __attribute__((aligned(N)))
#else
#define ALIGN(N)  __declspec(align(N))
#endif
