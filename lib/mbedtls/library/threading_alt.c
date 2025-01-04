#include "mbedtls/threading_alt.h"
#include "mbedtls/threading.h"

void mbedtls_platform_mutex_init(mbedtls_threading_mutex_t* pMutex)
{
	if (pMutex) {
		InitializeCriticalSection(&pMutex->critical_section);
	}
}


void mbedtls_platform_mutex_free(mbedtls_threading_mutex_t* pMutex)
{
	if (pMutex) {
		DeleteCriticalSection(&pMutex->critical_section);
	}
}


int mbedtls_platform_mutex_lock(mbedtls_threading_mutex_t* pMutex)
{
	if (!pMutex)
		return MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;

	EnterCriticalSection(&pMutex->critical_section);
	return 0;
}


int mbedtls_platform_mutex_unlock(mbedtls_threading_mutex_t* pMutex)
{
	if (!pMutex)
		return MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;

	LeaveCriticalSection(&pMutex->critical_section);
	return 0;
}
