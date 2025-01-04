
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/**
* @brief mbed TLS mutex type.
*
*/
typedef struct mbedtls_threading_mutex
{
	CRITICAL_SECTION critical_section;
} mbedtls_threading_mutex_t;


/* mbed TLS mutex functions. */
void mbedtls_platform_mutex_init(mbedtls_threading_mutex_t * pMutex);
void mbedtls_platform_mutex_free(mbedtls_threading_mutex_t * pMutex);
int mbedtls_platform_mutex_lock(mbedtls_threading_mutex_t * pMutex);
int mbedtls_platform_mutex_unlock(mbedtls_threading_mutex_t * pMutex);

