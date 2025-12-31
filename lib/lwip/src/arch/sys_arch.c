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
*         Simon Goldschmidt
*
*/

#define _CRT_RAND_S
#include <stdlib.h>
#include <stdio.h> /* sprintf() for task names */

#include <windows.h>
#include <time.h>

#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/stats.h>
#include <lwip/debug.h>
#include <lwip/sys.h>

/* These functions are used from NO_SYS also, for precise timer triggering */
static LARGE_INTEGER freq, sys_start_time;


static void sys_init_timing()
{
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&sys_start_time);
}

static LONGLONG sys_get_ms_longlong()
{
	LONGLONG ret;
	LARGE_INTEGER now;
#if NO_SYS
	if (freq.QuadPart == 0) {
		sys_init_timing();
	}
#endif /* NO_SYS */
	QueryPerformanceCounter(&now);
	ret = now.QuadPart - sys_start_time.QuadPart;
	return (u32_t)(((ret) * 1000) / freq.QuadPart);
}

u32_t sys_jiffies()
{
//	return (u32_t)sys_get_ms_longlong();
	return GetTickCount();
}

u32_t sys_now()
{
//	return (u32_t)sys_get_ms_longlong();
	return GetTickCount();
}


void sys_init()
{
	sys_init_timing();
}


sys_prot_t sys_arch_protect()
{
	return 0;
}


void sys_arch_unprotect(sys_prot_t pval)
{
	LWIP_UNUSED_ARG(pval);
}


u32_t sys_win_rand(void)
{
	u32_t random_value;

	rand_s(&random_value);
	return random_value;
}


static struct {
	sys_logger_cb cb;
	void* ctx;
} logger = { NULL, NULL };



void sys_set_logger(sys_logger_cb logger_cb, void *logger_ctx)
{
	logger.cb = logger_cb;
	logger.ctx = logger_ctx;
}


void sys_log_diag(const char* format, ...)
{
	if (logger.cb) {
		va_list args;
		va_start(args, format);
		logger.cb(logger.ctx, LWIP_DIAG_MESSAGE, format, args);
		va_end(args);
	}
}


void sys_log_error(const char* format, ...)
{
	if (logger.cb) {
		va_list args;
		va_start(args, format);
		logger.cb(logger.ctx, LWIP_ERROR_MESSAGE, format, args);
		va_end(args);
	}

}


