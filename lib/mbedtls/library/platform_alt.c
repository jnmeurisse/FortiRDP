
#include "mbedtls/platform.h"
#include "mbedtls/threading.h"
#include "threading_alt.h"


int mbedtls_platform_setup(mbedtls_platform_context *ctx)
{
	mbedtls_threading_set_alt(
		mbedtls_platform_mutex_init,
		mbedtls_platform_mutex_free,
		mbedtls_platform_mutex_lock,
		mbedtls_platform_mutex_unlock
	);

	return 0;
}


void mbedtls_platform_teardown(mbedtls_platform_context *ctx)
{
	(void)ctx;
	mbedtls_threading_free_alt();
}

