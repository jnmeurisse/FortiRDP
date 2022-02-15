/*
* Copyright (c) 2001-2003 Swedish Institute of Computer Science.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
* SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
* This file is part of the lwIP TCP/IP stack.
*
* Author: Adam Dunkels <adam@sics.se>
*
*/
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

/* Disable some warnings */
#pragma warning (disable: 4103) /* structure packing changed by including file */
#pragma warning (disable: 4996) /* strncpy */

/* include MSC errno.h */
#undef	LWIP_PROVIDE_ERRNO
#define LWIP_ERRNO_STDINCLUDE		1


/* Define platform endianness (might already be defined) */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif /* BYTE_ORDER */

/* Use native compiler hton and ntoh functions */
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

typedef int sys_prot_t;

#ifdef _MSC_VER
/* define _INTPTR for Win32 MSVC stdint.h */
#define _INTPTR 2

/* Do not use lwIP default definitions for format strings
* because these do not work with MSVC 2010 compiler (no inttypes.h)
*/
#define LWIP_NO_INTTYPES_H 1

/* Define (sn)printf formatters for these lwIP types */
#define X8_F  "02x"
#define U16_F "hu"
#define U32_F "lu"
#define S32_F "ld"
#define X32_F "lx"

#define S16_F "hd"
#define X16_F "hx"
#define SZT_F "lu"
#endif /* _MSC_VER */

/* */
typedef void (*sys_logger_cb )(void *ctx, int level, const char* fmt, char* args);

#ifdef __cplusplus
extern "C" {
#endif
	extern void sys_set_logger(sys_logger_cb logger_cb, void *logger_ctx);
#ifdef __cplusplus
}
#endif

extern void sys_log_error(const char *format, ...);
extern void sys_log_diag(const char *format, ...);


/* Compiler hints for packing structures */
#define PACK_STRUCT_USE_INCLUDES


#define LWIP_ERROR(message, expression, handler) do { if (!(expression)) { \
  sys_log_error("ERROR: %s", message); \
  handler;} } while(0)


#ifndef LWIP_NORAND
extern unsigned int sys_win_rand(void);
#define LWIP_RAND() (sys_win_rand())
#endif

//#define PPP_INCLUDE_SETTINGS_HEADER
#define LWIP_PLATFORM_ASSERT(x) do {sys_log_error("Assertion \"%s\" failed at line %d in %s", x, __LINE__, __FILE__);} while(0)
#define LWIP_PLATFORM_DIAG(x) do {sys_log_diag x;} while(0)


#endif /* LWIP_ARCH_CC_H */
