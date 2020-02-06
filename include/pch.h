#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#if defined(_WIN32_WINNT)
#undef _WIN32_WINNT
#endif

#define COBJMACROS
#define _WIN32_WINNT 0x0601
#if defined(_MSC_VER)
#define _NO_CRT_STDIO_INLINE
#define _CRT_SECURE_NO_WARNINGS
#include <xmmintrin.h>
#endif
#include <windows.h>
#include <ntsecapi.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <fenv.h>
#define COM_NO_WINDOWS_H
#include <d3d11.h>

#if defined(_MSC_VER)
#define ALIGN(N)  __declspec(align(N))
#else
#define ALIGN(N)  __attribute__((aligned(N)))
#endif
