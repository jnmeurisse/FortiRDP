/**
* \file config.h
*
* \brief Configuration options (set of defines)
*
*  This set of compile-time options may be used to enable
*  or disable features selectively, and reduce the global
*  memory footprint.
*/

/* This file is customized for fortirdp
 * Some cipher are explicitly deactivated
 */

/*
*  Copyright (C) 2006-2018, ARM Limited, All Rights Reserved
*  SPDX-License-Identifier: Apache-2.0
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*  This file is part of mbed TLS (https://tls.mbed.org)
*/

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

/**
* \name SECTION: System support
*
* This section sets system specific settings.
* \{
*/

/**
* \def MBEDTLS_HAVE_ASM
*
* The compiler has support for asm().
*
* Requires support for asm() in compiler.
*
* Used in:
*      library/aria.c
*      library/timing.c
*      include/mbedtls/bn_mul.h
*
* Required by:
*      MBEDTLS_AESNI_C
*      MBEDTLS_PADLOCK_C
*
* Comment to disable the use of assembly code.
*/
#define MBEDTLS_HAVE_ASM


/**
* \def MBEDTLS_HAVE_SSE2
*
* CPU supports SSE2 instruction set.
*
* Uncomment if the CPU supports SSE2 (IA-32 specific).
*/
//#define MBEDTLS_HAVE_SSE2

/**
* \def MBEDTLS_HAVE_TIME
*
* System has time.h and time().
* The time does not need to be correct, only time differences are used,
* by contrast with MBEDTLS_HAVE_TIME_DATE
*
* Defining MBEDTLS_HAVE_TIME allows you to specify MBEDTLS_PLATFORM_TIME_ALT,
* MBEDTLS_PLATFORM_TIME_MACRO, MBEDTLS_PLATFORM_TIME_TYPE_MACRO and
* MBEDTLS_PLATFORM_STD_TIME.
*
* Comment if your system does not support time functions
*/
#define MBEDTLS_HAVE_TIME

/**
* \def MBEDTLS_HAVE_TIME_DATE
*
* System has time.h, time(), and an implementation for
* mbedtls_platform_gmtime_r() (see below).
* The time needs to be correct (not necesarily very accurate, but at least
* the date should be correct). This is used to verify the validity period of
* X.509 certificates.
*
* Comment if your system does not have a correct clock.
*
* \note mbedtls_platform_gmtime_r() is an abstraction in platform_util.h that
* behaves similarly to the gmtime_r() function from the C standard. Refer to
* the documentation for mbedtls_platform_gmtime_r() for more information.
*
* \note It is possible to configure an implementation for
* mbedtls_platform_gmtime_r() at compile-time by using the macro
* MBEDTLS_PLATFORM_GMTIME_R_ALT.
*/
#define MBEDTLS_HAVE_TIME_DATE


/**
* \def MBEDTLS_CHECK_PARAMS
*
* This configuration option controls whether the library validates more of
* the parameters passed to it.
*
* When this flag is not defined, the library only attempts to validate an
* input parameter if: (1) they may come from the outside world (such as the
* network, the filesystem, etc.) or (2) not validating them could result in
* internal memory errors such as overflowing a buffer controlled by the
* library. On the other hand, it doesn't attempt to validate parameters whose
* values are fully controlled by the application (such as pointers).
*
* When this flag is defined, the library additionally attempts to validate
* parameters that are fully controlled by the application, and should always
* be valid if the application code is fully correct and trusted.
*
* For example, when a function accepts as input a pointer to a buffer that may
* contain untrusted data, and its documentation mentions that this pointer
* must not be NULL:
* - the pointer is checked to be non-NULL only if this option is enabled
* - the content of the buffer is always validated
*
* When this flag is defined, if a library function receives a parameter that
* is invalid, it will:
* - invoke the macro MBEDTLS_PARAM_FAILED() which by default expands to a
*   call to the function mbedtls_param_failed()
* - immediately return (with a specific error code unless the function
*   returns void and can't communicate an error).
*
* When defining this flag, you also need to:
* - either provide a definition of the function mbedtls_param_failed() in
*   your application (see platform_util.h for its prototype) as the library
*   calls that function, but does not provide a default definition for it,
* - or provide a different definition of the macro MBEDTLS_PARAM_FAILED()
*   below if the above mechanism is not flexible enough to suit your needs.
*   See the documentation of this macro later in this file.
*
* Uncomment to enable validation of application-controlled parameters.
*/
#ifdef _DEBUG
#define MBEDTLS_CHECK_PARAMS
#endif // DEBUG


/* \} name SECTION: System support */

/**
* \name SECTION: mbed TLS feature support
*
* This section sets support for features that are or are not needed
* within the modules that are enabled.
* \{
*/



/**
* \def MBEDTLS_ENTROPY_HARDWARE_ALT
*
* Uncomment this macro to let mbed TLS use your own implementation of a
* hardware entropy collector.
*
* Your function must be called \c mbedtls_hardware_poll(), have the same
* prototype as declared in entropy_poll.h, and accept NULL as first argument.
*
* Uncomment to use your own hardware entropy collector.
*/
//#define MBEDTLS_ENTROPY_HARDWARE_ALT

/**
* \def MBEDTLS_CIPHER_MODE_CBC
*
* Enable Cipher Block Chaining mode (CBC) for symmetric ciphers.
*/
#define MBEDTLS_CIPHER_MODE_CBC

/**
* \def MBEDTLS_CIPHER_MODE_CFB
*
* Enable Cipher Feedback mode (CFB) for symmetric ciphers.
*/
#define MBEDTLS_CIPHER_MODE_CFB

/**
* \def MBEDTLS_CIPHER_MODE_CTR
*
* Enable Counter Block Cipher mode (CTR) for symmetric ciphers.
*/
#define MBEDTLS_CIPHER_MODE_CTR

/**
* \def MBEDTLS_CIPHER_MODE_OFB
*
* Enable Output Feedback mode (OFB) for symmetric ciphers.
*/
#define MBEDTLS_CIPHER_MODE_OFB

/**
* \def MBEDTLS_CIPHER_MODE_XTS
*
* Enable Xor-encrypt-xor with ciphertext stealing mode (XTS) for AES.
*/
#define MBEDTLS_CIPHER_MODE_XTS


/**
* \def MBEDTLS_CIPHER_PADDING_PKCS7
*
* MBEDTLS_CIPHER_PADDING_XXX: Uncomment or comment macros to add support for
* specific padding modes in the cipher layer with cipher modes that support
* padding (e.g. CBC)
*
* If you disable all padding modes, only full blocks can be used with CBC.
*
* Enable padding modes in the cipher layer.
*/
#define MBEDTLS_CIPHER_PADDING_PKCS7
#define MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS
#define MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN
#define MBEDTLS_CIPHER_PADDING_ZEROS

/**
* \def MBEDTLS_REMOVE_ARC4_CIPHERSUITES
*
* Remove RC4 ciphersuites by default in SSL / TLS.
* This flag removes the ciphersuites based on RC4 from the default list as
* returned by mbedtls_ssl_list_ciphersuites(). However, it is still possible to
* enable (some of) them with mbedtls_ssl_conf_ciphersuites() by including them
* explicitly.
*
* Uncomment this macro to remove RC4 ciphersuites by default.
*/
#define MBEDTLS_REMOVE_ARC4_CIPHERSUITES

/**
* \def MBEDTLS_ECP_DP_SECP192R1_ENABLED
*
* MBEDTLS_ECP_XXXX_ENABLED: Enables specific curves within the Elliptic Curve
* module.  By default all supported curves are enabled.
*
* Comment macros to disable the curve and functions for it
*/
#define MBEDTLS_ECP_DP_SECP192R1_ENABLED
#define MBEDTLS_ECP_DP_SECP224R1_ENABLED
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_SECP521R1_ENABLED
#define MBEDTLS_ECP_DP_SECP192K1_ENABLED
#define MBEDTLS_ECP_DP_SECP224K1_ENABLED
#define MBEDTLS_ECP_DP_SECP256K1_ENABLED
#define MBEDTLS_ECP_DP_BP256R1_ENABLED
#define MBEDTLS_ECP_DP_BP384R1_ENABLED
#define MBEDTLS_ECP_DP_BP512R1_ENABLED
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED
#define MBEDTLS_ECP_DP_CURVE448_ENABLED

/**
* \def MBEDTLS_ECP_NIST_OPTIM
*
* Enable specific 'modulo p' routines for each NIST prime.
* Depending on the prime and architecture, makes operations 4 to 8 times
* faster on the corresponding curve.
*
* Comment this macro to disable NIST curves optimisation.
*/
#define MBEDTLS_ECP_NIST_OPTIM


/**
* \def MBEDTLS_ECDSA_DETERMINISTIC
*
* Enable deterministic ECDSA (RFC 6979).
* Standard ECDSA is "fragile" in the sense that lack of entropy when signing
* may result in a compromise of the long-term signing key. This is avoided by
* the deterministic variant.
*
* Requires: MBEDTLS_HMAC_DRBG_C
*
* Comment this macro to disable deterministic ECDSA.
*/
#define MBEDTLS_ECDSA_DETERMINISTIC

/**
* \def MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
*
* Enable the PSK based ciphersuite modes in SSL / TLS.
*
* This enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_PSK_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_PSK_WITH_CAMELLIA_256_GCM_SHA384
*      MBEDTLS_TLS_PSK_WITH_CAMELLIA_256_CBC_SHA384
*      MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_PSK_WITH_CAMELLIA_128_GCM_SHA256
*      MBEDTLS_TLS_PSK_WITH_CAMELLIA_128_CBC_SHA256
*      MBEDTLS_TLS_PSK_WITH_3DES_EDE_CBC_SHA
*      MBEDTLS_TLS_PSK_WITH_RC4_128_SHA
*/
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED


/**
* \def MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
*
* Enable the ECDHE-PSK based ciphersuite modes in SSL / TLS.
*
* Requires: MBEDTLS_ECDH_C
*
* This enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384
*      MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA
*      MBEDTLS_TLS_ECDHE_PSK_WITH_RC4_128_SHA
*/
#define MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED

/**
* \def MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED
*
* Enable the RSA-PSK based ciphersuite modes in SSL / TLS.
*
* Requires: MBEDTLS_RSA_C, MBEDTLS_PKCS1_V15,
*           MBEDTLS_X509_CRT_PARSE_C
*
* This enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_256_GCM_SHA384
*      MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_128_GCM_SHA256
*      MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256
*      MBEDTLS_TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA
*      MBEDTLS_TLS_RSA_PSK_WITH_RC4_128_SHA
*/
#define MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED

/**
* \def MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
*
* Enable the RSA-only based ciphersuite modes in SSL / TLS.
*
* Requires: MBEDTLS_RSA_C, MBEDTLS_PKCS1_V15,
*           MBEDTLS_X509_CRT_PARSE_C
*
* This enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256
*      MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_GCM_SHA384
*      MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256
*      MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_CBC_SHA
*      MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_GCM_SHA256
*      MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256
*      MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_CBC_SHA
*      MBEDTLS_TLS_RSA_WITH_3DES_EDE_CBC_SHA
*      MBEDTLS_TLS_RSA_WITH_RC4_128_SHA
*      MBEDTLS_TLS_RSA_WITH_RC4_128_MD5
*/
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED


/**
* \def MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
*
* Enable the ECDHE-RSA based ciphersuite modes in SSL / TLS.
*
* Requires: MBEDTLS_ECDH_C, MBEDTLS_RSA_C, MBEDTLS_PKCS1_V15,
*           MBEDTLS_X509_CRT_PARSE_C
*
* This enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384
*      MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256
*      MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA
*      MBEDTLS_TLS_ECDHE_RSA_WITH_RC4_128_SHA
*/
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED

/**
* \def MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
*
* Enable the ECDHE-ECDSA based ciphersuite modes in SSL / TLS.
*
* Requires: MBEDTLS_ECDH_C, MBEDTLS_ECDSA_C, MBEDTLS_X509_CRT_PARSE_C,
*
* This enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_RC4_128_SHA
*/
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

/**
* \def MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
*
* Enable the ECDH-ECDSA based ciphersuite modes in SSL / TLS.
*
* Requires: MBEDTLS_ECDH_C, MBEDTLS_X509_CRT_PARSE_C
*
* This enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_RC4_128_SHA
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384
*/
#define MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED

/**
* \def MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED
*
* Enable the ECDH-RSA based ciphersuite modes in SSL / TLS.
*
* Requires: MBEDTLS_ECDH_C, MBEDTLS_X509_CRT_PARSE_C
*
* This enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_ECDH_RSA_WITH_RC4_128_SHA
*      MBEDTLS_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256
*      MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384
*      MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256
*      MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384
*/
#define MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED

/**
* \def MBEDTLS_PK_PARSE_EC_EXTENDED
*
* Enhance support for reading EC keys using variants of SEC1 not allowed by
* RFC 5915 and RFC 5480.
*
* Currently this means parsing the SpecifiedECDomain choice of EC
* parameters (only known groups are supported, not arbitrary domains, to
* avoid validation issues).
*
* Disable if you only need to support RFC 5915 + 5480 key formats.
*/
#define MBEDTLS_PK_PARSE_EC_EXTENDED

/**
* \def MBEDTLS_ERROR_STRERROR_DUMMY
*
* Enable a dummy error function to make use of mbedtls_strerror() in
* third party libraries easier when MBEDTLS_ERROR_C is disabled
* (no effect when MBEDTLS_ERROR_C is enabled).
*
* You can safely disable this if MBEDTLS_ERROR_C is enabled, or if you're
* not using mbedtls_strerror() or error_strerror() in your application.
*
* Disable if you run into name conflicts and want to really remove the
* mbedtls_strerror()
*/
#define MBEDTLS_ERROR_STRERROR_DUMMY

/**
* \def MBEDTLS_GENPRIME
*
* Enable the prime-number generation code.
*
* Requires: MBEDTLS_BIGNUM_C
*/
#define MBEDTLS_GENPRIME

/**
* \def MBEDTLS_FS_IO
*
* Enable functions that use the filesystem.
*/
#define MBEDTLS_FS_IO


/**
* \def MBEDTLS_PK_RSA_ALT_SUPPORT
*
* Support external private RSA keys (eg from a HSM) in the PK layer.
*
* Comment this macro to disable support for external private RSA keys.
*/
#define MBEDTLS_PK_RSA_ALT_SUPPORT

/**
* \def MBEDTLS_PKCS1_V15
*
* Enable support for PKCS#1 v1.5 encoding.
*
* Requires: MBEDTLS_RSA_C
*
* This enables support for PKCS#1 v1.5 operations.
*/
#define MBEDTLS_PKCS1_V15

/**
* \def MBEDTLS_PKCS1_V21
*
* Enable support for PKCS#1 v2.1 encoding.
*
* Requires: MBEDTLS_MD_C, MBEDTLS_RSA_C
*
* This enables support for RSAES-OAEP and RSASSA-PSS operations.
*/
#define MBEDTLS_PKCS1_V21


/**
* \def MBEDTLS_SSL_ALL_ALERT_MESSAGES
*
* Enable sending of alert messages in case of encountered errors as per RFC.
* If you choose not to send the alert messages, mbed TLS can still communicate
* with other servers, only debugging of failures is harder.
*
* The advantage of not sending alert messages, is that no information is given
* about reasons for failures thus preventing adversaries of gaining intel.
*
* Enable sending of all alert messages
*/
#define MBEDTLS_SSL_ALL_ALERT_MESSAGES

/** \def MBEDTLS_SSL_ENCRYPT_THEN_MAC
*
* Enable support for Encrypt-then-MAC, RFC 7366.
*
* This allows peers that both support it to use a more robust protection for
* ciphersuites using CBC, providing deep resistance against timing attacks
* on the padding or underlying cipher.
*
* This only affects CBC ciphersuites, and is useless if none is defined.
*
* Requires: MBEDTLS_SSL_PROTO_TLS1    or
*           MBEDTLS_SSL_PROTO_TLS1_1  or
*           MBEDTLS_SSL_PROTO_TLS1_2
*
* Comment this macro to disable support for Encrypt-then-MAC
*/
#define MBEDTLS_SSL_ENCRYPT_THEN_MAC

/** \def MBEDTLS_SSL_EXTENDED_MASTER_SECRET
*
* Enable support for Extended Master Secret, aka Session Hash
* (draft-ietf-tls-session-hash-02).
*
* This was introduced as "the proper fix" to the Triple Handshake familiy of
* attacks, but it is recommended to always use it (even if you disable
* renegotiation), since it actually fixes a more fundamental issue in the
* original SSL/TLS design, and has implications beyond Triple Handshake.
*
* Requires: MBEDTLS_SSL_PROTO_TLS1    or
*           MBEDTLS_SSL_PROTO_TLS1_1  or
*           MBEDTLS_SSL_PROTO_TLS1_2
*
* Comment this macro to disable support for Extended Master Secret.
*/
#define MBEDTLS_SSL_EXTENDED_MASTER_SECRET

/**
* \def MBEDTLS_SSL_CBC_RECORD_SPLITTING
*
* Enable 1/n-1 record splitting for CBC mode in SSLv3 and TLS 1.0.
*
* This is a countermeasure to the BEAST attack, which also minimizes the risk
* of interoperability issues compared to sending 0-length records.
*
* Comment this macro to disable 1/n-1 record splitting.
*/
// JNM: Disabled to keep compatibility with old FortiOS firmware
//#define MBEDTLS_SSL_CBC_RECORD_SPLITTING

/**
* \def MBEDTLS_SSL_RENEGOTIATION
*
* Enable support for TLS renegotiation.
*
* The two main uses of renegotiation are (1) refresh keys on long-lived
* connections and (2) client authentication after the initial handshake.
* If you don't need renegotiation, it's probably better to disable it, since
* it has been associated with security issues in the past and is easy to
* misuse/misunderstand.
*
* Comment this to disable support for renegotiation.
*
* \note   Even if this option is disabled, both client and server are aware
*         of the Renegotiation Indication Extension (RFC 5746) used to
*         prevent the SSL renegotiation attack (see RFC 5746 Sect. 1).
*         (See \c mbedtls_ssl_conf_legacy_renegotiation for the
*          configuration of this extension).
*
*/
#define MBEDTLS_SSL_RENEGOTIATION

/**
* \def MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
*
* Enable support for RFC 6066 max_fragment_length extension in SSL.
*
* Comment this macro to disable support for the max_fragment_length extension
*/
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH


/**
* \def MBEDTLS_SSL_PROTO_TLS1_1
*
* Enable support for TLS 1.1 (and DTLS 1.0 if DTLS is enabled).
*
* Requires: MBEDTLS_MD5_C
*           MBEDTLS_SHA1_C
*
* Comment this macro to disable support for TLS 1.1 / DTLS 1.0
*/
#define MBEDTLS_SSL_PROTO_TLS1_1

/**
* \def MBEDTLS_SSL_PROTO_TLS1_2
*
* Enable support for TLS 1.2 (and DTLS 1.2 if DTLS is enabled).
*
* Requires: MBEDTLS_SHA1_C or MBEDTLS_SHA256_C or MBEDTLS_SHA512_C
*           (Depends on ciphersuites)
*
* Comment this macro to disable support for TLS 1.2 / DTLS 1.2
*/
#define MBEDTLS_SSL_PROTO_TLS1_2


/**
* \def MBEDTLS_SSL_ALPN
*
* Enable support for RFC 7301 Application Layer Protocol Negotiation.
*
* Comment this macro to disable support for ALPN.
*/
#define MBEDTLS_SSL_ALPN

/**
* \def MBEDTLS_SSL_SESSION_TICKETS
*
* Enable support for RFC 5077 session tickets in SSL.
* Client-side, provides full support for session tickets (maintainance of a
* session store remains the responsibility of the application, though).
* Server-side, you also need to provide callbacks for writing and parsing
* tickets, including authenticated encryption and key management. Example
* callbacks are provided by MBEDTLS_SSL_TICKET_C.
*
* Comment this macro to disable support for SSL session tickets
*/
#define MBEDTLS_SSL_SESSION_TICKETS

/**
* \def MBEDTLS_SSL_EXPORT_KEYS
*
* Enable support for exporting key block and master secret.
* This is required for certain users of TLS, e.g. EAP-TLS.
*
* Comment this macro to disable support for key export
*/
#define MBEDTLS_SSL_EXPORT_KEYS

/**
* \def MBEDTLS_SSL_SERVER_NAME_INDICATION
*
* Enable support for RFC 6066 server name indication (SNI) in SSL.
*
* Requires: MBEDTLS_X509_CRT_PARSE_C
*
* Comment this macro to disable support for server name indication in SSL
*/
#define MBEDTLS_SSL_SERVER_NAME_INDICATION

/**
* \def MBEDTLS_SSL_TRUNCATED_HMAC
*
* Enable support for RFC 6066 truncated HMAC in SSL.
*
* Comment this macro to disable support for truncated HMAC in SSL
*/
#define MBEDTLS_SSL_TRUNCATED_HMAC

/**
* \def MBEDTLS_VERSION_FEATURES
*
* Allow run-time checking of compile-time enabled features. Thus allowing users
* to check at run-time if the library is for instance compiled with threading
* support via mbedtls_version_check_feature().
*
* Requires: MBEDTLS_VERSION_C
*
* Comment this to disable run-time checking and save ROM space
*/
#define MBEDTLS_VERSION_FEATURES


/**
* \def MBEDTLS_X509_CHECK_KEY_USAGE
*
* Enable verification of the keyUsage extension (CA and leaf certificates).
*
* Disabling this avoids problems with mis-issued and/or misused
* (intermediate) CA and leaf certificates.
*
* \warning Depending on your PKI use, disabling this can be a security risk!
*
* Comment to skip keyUsage checking for both CA and leaf certificates.
*/
#define MBEDTLS_X509_CHECK_KEY_USAGE

/**
* \def MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE
*
* Enable verification of the extendedKeyUsage extension (leaf certificates).
*
* Disabling this avoids problems with mis-issued and/or misused certificates.
*
* \warning Depending on your PKI use, disabling this can be a security risk!
*
* Comment to skip extendedKeyUsage checking for certificates.
*/
#define MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE

/**
* \def MBEDTLS_X509_RSASSA_PSS_SUPPORT
*
* Enable parsing and verification of X.509 certificates, CRLs and CSRS
* signed with RSASSA-PSS (aka PKCS#1 v2.1).
*
* Comment this macro to disallow using RSASSA-PSS in certificates.
*/
#define MBEDTLS_X509_RSASSA_PSS_SUPPORT

/* \} name SECTION: mbed TLS feature support */

/**
* \name SECTION: mbed TLS modules
*
* This section enables or disables entire modules in mbed TLS
* \{
*/

/**
* \def MBEDTLS_AESNI_C
*
* Enable AES-NI support on x86-64.
*
* Module:  library/aesni.c
* Caller:  library/aes.c
*
* Requires: MBEDTLS_HAVE_ASM
*
* This modules adds support for the AES-NI instructions on x86-64
*/
#define MBEDTLS_AESNI_C

/**
* \def MBEDTLS_AES_C
*
* Enable the AES block cipher.
*
* Module:  library/aes.c
* Caller:  library/cipher.c
*          library/pem.c
*          library/ctr_drbg.c
*
* This module enables the following ciphersuites (if other requisites are
* enabled as well):
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_DHE_RSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_DHE_PSK_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_DHE_PSK_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256
*      MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA
*      MBEDTLS_TLS_PSK_WITH_AES_256_GCM_SHA384
*      MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA384
*      MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA
*      MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256
*      MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA256
*      MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA
*
* PEM_PARSE uses AES for decrypting encrypted keys.
*/
#define MBEDTLS_AES_C

/**
* \def MBEDTLS_ASN1_PARSE_C
*
* Enable the generic ASN1 parser.
*
* Module:  library/asn1.c
* Caller:  library/x509.c
*          library/dhm.c
*          library/pkcs12.c
*          library/pkcs5.c
*          library/pkparse.c
*/
#define MBEDTLS_ASN1_PARSE_C

/**
* \def MBEDTLS_ASN1_WRITE_C
*
* Enable the generic ASN1 writer.
*
* Module:  library/asn1write.c
* Caller:  library/ecdsa.c
*          library/pkwrite.c
*          library/x509_create.c
*          library/x509write_crt.c
*          library/x509write_csr.c
*/
#define MBEDTLS_ASN1_WRITE_C

/**
* \def MBEDTLS_BASE64_C
*
* Enable the Base64 module.
*
* Module:  library/base64.c
* Caller:  library/pem.c
*
* This module is required for PEM support (required by X.509).
*/
#define MBEDTLS_BASE64_C

/**
* \def MBEDTLS_BIGNUM_C
*
* Enable the multi-precision integer library.
*
* Module:  library/bignum.c
* Caller:  library/dhm.c
*          library/ecp.c
*          library/ecdsa.c
*          library/rsa.c
*          library/rsa_internal.c
*          library/ssl_tls.c
*
* This module is required for RSA, DHM and ECC (ECDH, ECDSA) support.
*/
#define MBEDTLS_BIGNUM_C



/**
* \def MBEDTLS_CCM_C
*
* Enable the Counter with CBC-MAC (CCM) mode for 128-bit block cipher.
*
* Module:  library/ccm.c
*
* Requires: MBEDTLS_AES_C or MBEDTLS_CAMELLIA_C
*
* This module enables the AES-CCM ciphersuites, if other requisites are
* enabled as well.
*/
#define MBEDTLS_CCM_C

/**
* \def MBEDTLS_CHACHA20_C
*
* Enable the ChaCha20 stream cipher.
*
* Module:  library/chacha20.c
*/
#define MBEDTLS_CHACHA20_C

/**
* \def MBEDTLS_CHACHAPOLY_C
*
* Enable the ChaCha20-Poly1305 AEAD algorithm.
*
* Module:  library/chachapoly.c
*
* This module requires: MBEDTLS_CHACHA20_C, MBEDTLS_POLY1305_C
*/
#define MBEDTLS_CHACHAPOLY_C

/**
* \def MBEDTLS_CIPHER_C
*
* Enable the generic cipher layer.
*
* Module:  library/cipher.c
* Caller:  library/ssl_tls.c
*
* Uncomment to enable generic cipher wrappers.
*/
#define MBEDTLS_CIPHER_C


/**
* \def MBEDTLS_CTR_DRBG_C
*
* Enable the CTR_DRBG AES-based random generator.
* The CTR_DRBG generator uses AES-256 by default.
* To use AES-128 instead, enable MBEDTLS_CTR_DRBG_USE_128_BIT_KEY below.
*
* Module:  library/ctr_drbg.c
* Caller:
*
* Requires: MBEDTLS_AES_C
*
* This module provides the CTR_DRBG AES random number generator.
*/
#define MBEDTLS_CTR_DRBG_C

/**
* \def MBEDTLS_DEBUG_C
*
* Enable the debug functions.
*
* Module:  library/debug.c
* Caller:  library/ssl_cli.c
*          library/ssl_srv.c
*          library/ssl_tls.c
*
* This module provides debugging functions.
*/
#define MBEDTLS_DEBUG_C

/**
* \def MBEDTLS_ECDH_C
*
* Enable the elliptic curve Diffie-Hellman library.
*
* Module:  library/ecdh.c
* Caller:  library/ssl_cli.c
*          library/ssl_srv.c
*
* This module is used by the following key exchanges:
*      ECDHE-ECDSA, ECDHE-RSA, DHE-PSK
*
* Requires: MBEDTLS_ECP_C
*/
#define MBEDTLS_ECDH_C

/**
* \def MBEDTLS_ECDSA_C
*
* Enable the elliptic curve DSA library.
*
* Module:  library/ecdsa.c
* Caller:
*
* This module is used by the following key exchanges:
*      ECDHE-ECDSA
*
* Requires: MBEDTLS_ECP_C, MBEDTLS_ASN1_WRITE_C, MBEDTLS_ASN1_PARSE_C
*/
#define MBEDTLS_ECDSA_C


/**
* \def MBEDTLS_ECP_C
*
* Enable the elliptic curve over GF(p) library.
*
* Module:  library/ecp.c
* Caller:  library/ecdh.c
*          library/ecdsa.c
*          library/ecjpake.c
*
* Requires: MBEDTLS_BIGNUM_C and at least one MBEDTLS_ECP_DP_XXX_ENABLED
*/
#define MBEDTLS_ECP_C

/**
* \def MBEDTLS_ENTROPY_C
*
* Enable the platform-specific entropy code.
*
* Module:  library/entropy.c
* Caller:
*
* Requires: MBEDTLS_SHA512_C or MBEDTLS_SHA256_C
*
* This module provides a generic entropy pool
*/
#define MBEDTLS_ENTROPY_C

/**
* \def MBEDTLS_ERROR_C
*
* Enable error code to error string conversion.
*
* Module:  library/error.c
* Caller:
*
* This module enables mbedtls_strerror().
*/
#define MBEDTLS_ERROR_C

/**
* \def MBEDTLS_GCM_C
*
* Enable the Galois/Counter Mode (GCM) for AES.
*
* Module:  library/gcm.c
*
* Requires: MBEDTLS_AES_C or MBEDTLS_CAMELLIA_C
*
* This module enables the AES-GCM and CAMELLIA-GCM ciphersuites, if other
* requisites are enabled as well.
*/
#define MBEDTLS_GCM_C

/**
* \def MBEDTLS_HKDF_C
*
* Enable the HKDF algorithm (RFC 5869).
*
* Module:  library/hkdf.c
* Caller:
*
* Requires: MBEDTLS_MD_C
*
* This module adds support for the Hashed Message Authentication Code
* (HMAC)-based key derivation function (HKDF).
*/
#define MBEDTLS_HKDF_C

/**
* \def MBEDTLS_HMAC_DRBG_C
*
* Enable the HMAC_DRBG random generator.
*
* Module:  library/hmac_drbg.c
* Caller:
*
* Requires: MBEDTLS_MD_C
*
* Uncomment to enable the HMAC_DRBG random number geerator.
*/
#define MBEDTLS_HMAC_DRBG_C


/**
* \def MBEDTLS_MD_C
*
* Enable the generic message digest layer.
*
* Module:  library/md.c
* Caller:
*
* Uncomment to enable generic message digest wrappers.
*/
#define MBEDTLS_MD_C


/**
* \def MBEDTLS_MD5_C
*
* Enable the MD5 hash algorithm.
*
* Module:  library/md5.c
* Caller:  library/md.c
*          library/pem.c
*          library/ssl_tls.c
*
* This module is required for SSL/TLS up to version 1.1, and for TLS 1.2
* depending on the handshake parameters. Further, it is used for checking
* MD5-signed certificates, and for PBKDF1 when decrypting PEM-encoded
* encrypted keys.
*
* \warning   MD5 is considered a weak message digest and its use constitutes a
*            security risk. If possible, we recommend avoiding dependencies on
*            it, and considering stronger message digests instead.
*
*/
#define MBEDTLS_MD5_C


/**
* \def MBEDTLS_NET_C
*
* Enable the TCP and UDP over IPv6/IPv4 networking routines.
*
* \note This module only works on POSIX/Unix (including Linux, BSD and OS X)
* and Windows. For other platforms, you'll want to disable it, and write your
* own networking callbacks to be passed to \c mbedtls_ssl_set_bio().
*
* \note See also our Knowledge Base article about porting to a new
* environment:
* https://tls.mbed.org/kb/how-to/how-do-i-port-mbed-tls-to-a-new-environment-OS
*
* Module:  library/net_sockets.c
*
* This module provides networking routines.
*/
#define MBEDTLS_NET_C

/**
* \def MBEDTLS_OID_C
*
* Enable the OID database.
*
* Module:  library/oid.c
* Caller:  library/asn1write.c
*          library/pkcs5.c
*          library/pkparse.c
*          library/pkwrite.c
*          library/rsa.c
*          library/x509.c
*          library/x509_create.c
*          library/x509_crl.c
*          library/x509_crt.c
*          library/x509_csr.c
*          library/x509write_crt.c
*          library/x509write_csr.c
*
* This modules translates between OIDs and internal values.
*/
#define MBEDTLS_OID_C

/**
* \def MBEDTLS_PADLOCK_C
*
* Enable VIA Padlock support on x86.
*
* Module:  library/padlock.c
* Caller:  library/aes.c
*
* Requires: MBEDTLS_HAVE_ASM
*
* This modules adds support for the VIA PadLock on x86.
*/
#define MBEDTLS_PADLOCK_C

/**
* \def MBEDTLS_PEM_PARSE_C
*
* Enable PEM decoding / parsing.
*
* Module:  library/pem.c
* Caller:  library/dhm.c
*          library/pkparse.c
*          library/x509_crl.c
*          library/x509_crt.c
*          library/x509_csr.c
*
* Requires: MBEDTLS_BASE64_C
*
* This modules adds support for decoding / parsing PEM files.
*/
#define MBEDTLS_PEM_PARSE_C

/**
* \def MBEDTLS_PEM_WRITE_C
*
* Enable PEM encoding / writing.
*
* Module:  library/pem.c
* Caller:  library/pkwrite.c
*          library/x509write_crt.c
*          library/x509write_csr.c
*
* Requires: MBEDTLS_BASE64_C
*
* This modules adds support for encoding / writing PEM files.
*/
#define MBEDTLS_PEM_WRITE_C

/**
* \def MBEDTLS_PK_C
*
* Enable the generic public (asymetric) key layer.
*
* Module:  library/pk.c
* Caller:  library/ssl_tls.c
*          library/ssl_cli.c
*          library/ssl_srv.c
*
* Requires: MBEDTLS_RSA_C or MBEDTLS_ECP_C
*
* Uncomment to enable generic public key wrappers.
*/
#define MBEDTLS_PK_C

/**
* \def MBEDTLS_PK_PARSE_C
*
* Enable the generic public (asymetric) key parser.
*
* Module:  library/pkparse.c
* Caller:  library/x509_crt.c
*          library/x509_csr.c
*
* Requires: MBEDTLS_PK_C
*
* Uncomment to enable generic public key parse functions.
*/
#define MBEDTLS_PK_PARSE_C

/**
* \def MBEDTLS_PK_WRITE_C
*
* Enable the generic public (asymetric) key writer.
*
* Module:  library/pkwrite.c
* Caller:  library/x509write.c
*
* Requires: MBEDTLS_PK_C
*
* Uncomment to enable generic public key write functions.
*/
#define MBEDTLS_PK_WRITE_C

/**
* \def MBEDTLS_PKCS5_C
*
* Enable PKCS#5 functions.
*
* Module:  library/pkcs5.c
*
* Requires: MBEDTLS_MD_C
*
* This module adds support for the PKCS#5 functions.
*/
#define MBEDTLS_PKCS5_C

/**
* \def MBEDTLS_PKCS11_C
*
* Enable wrapper for PKCS#11 smartcard support.
*
* Module:  library/pkcs11.c
* Caller:  library/pk.c
*
* Requires: MBEDTLS_PK_C
*
* This module enables SSL/TLS PKCS #11 smartcard support.
* Requires the presence of the PKCS#11 helper library (libpkcs11-helper)
*/
//#define MBEDTLS_PKCS11_C

/**
* \def MBEDTLS_PKCS12_C
*
* Enable PKCS#12 PBE functions.
* Adds algorithms for parsing PKCS#8 encrypted private keys
*
* Module:  library/pkcs12.c
* Caller:  library/pkparse.c
*
* Requires: MBEDTLS_ASN1_PARSE_C, MBEDTLS_CIPHER_C, MBEDTLS_MD_C
* Can use:  MBEDTLS_ARC4_C
*
* This module enables PKCS#12 functions.
*/
#define MBEDTLS_PKCS12_C

/**
* \def MBEDTLS_PLATFORM_C
*
* Enable the platform abstraction layer that allows you to re-assign
* functions like calloc(), free(), snprintf(), printf(), fprintf(), exit().
*
* Enabling MBEDTLS_PLATFORM_C enables to use of MBEDTLS_PLATFORM_XXX_ALT
* or MBEDTLS_PLATFORM_XXX_MACRO directives, allowing the functions mentioned
* above to be specified at runtime or compile time respectively.
*
* \note This abstraction layer must be enabled on Windows (including MSYS2)
* as other module rely on it for a fixed snprintf implementation.
*
* Module:  library/platform.c
* Caller:  Most other .c files
*
* This module enables abstraction of common (libc) functions.
*/
#define MBEDTLS_PLATFORM_C

/**
* \def MBEDTLS_POLY1305_C
*
* Enable the Poly1305 MAC algorithm.
*
* Module:  library/poly1305.c
* Caller:  library/chachapoly.c
*/
#define MBEDTLS_POLY1305_C

/**
* \def MBEDTLS_RSA_C
*
* Enable the RSA public-key cryptosystem.
*
* Module:  library/rsa.c
*          library/rsa_internal.c
* Caller:  library/ssl_cli.c
*          library/ssl_srv.c
*          library/ssl_tls.c
*          library/x509.c
*
* This module is used by the following key exchanges:
*      RSA, DHE-RSA, ECDHE-RSA, RSA-PSK
*
* Requires: MBEDTLS_BIGNUM_C, MBEDTLS_OID_C
*/
#define MBEDTLS_RSA_C

/**
* \def MBEDTLS_SHA1_C
*
* Enable the SHA1 cryptographic hash algorithm.
*
* Module:  library/sha1.c
* Caller:  library/md.c
*          library/ssl_cli.c
*          library/ssl_srv.c
*          library/ssl_tls.c
*          library/x509write_crt.c
*
* This module is required for SSL/TLS up to version 1.1, for TLS 1.2
* depending on the handshake parameters, and for SHA1-signed certificates.
*
* \warning   SHA-1 is considered a weak message digest and its use constitutes
*            a security risk. If possible, we recommend avoiding dependencies
*            on it, and considering stronger message digests instead.
*
*/
#define MBEDTLS_SHA1_C

/**
* \def MBEDTLS_SHA256_C
*
* Enable the SHA-224 and SHA-256 cryptographic hash algorithms.
*
* Module:  library/sha256.c
* Caller:  library/entropy.c
*          library/md.c
*          library/ssl_cli.c
*          library/ssl_srv.c
*          library/ssl_tls.c
*
* This module adds support for SHA-224 and SHA-256.
* This module is required for the SSL/TLS 1.2 PRF function.
*/
#define MBEDTLS_SHA256_C

/**
* \def MBEDTLS_SHA512_C
*
* Enable the SHA-384 and SHA-512 cryptographic hash algorithms.
*
* Module:  library/sha512.c
* Caller:  library/entropy.c
*          library/md.c
*          library/ssl_cli.c
*          library/ssl_srv.c
*
* This module adds support for SHA-384 and SHA-512.
*/
#define MBEDTLS_SHA512_C

/**
* \def MBEDTLS_SSL_CACHE_C
*
* Enable simple SSL cache implementation.
*
* Module:  library/ssl_cache.c
* Caller:
*
* Requires: MBEDTLS_SSL_CACHE_C
*/
#define MBEDTLS_SSL_CACHE_C

/**
* \def MBEDTLS_SSL_COOKIE_C
*
* Enable basic implementation of DTLS cookies for hello verification.
*
* Module:  library/ssl_cookie.c
* Caller:
*/
#define MBEDTLS_SSL_COOKIE_C

/**
* \def MBEDTLS_SSL_TICKET_C
*
* Enable an implementation of TLS server-side callbacks for session tickets.
*
* Module:  library/ssl_ticket.c
* Caller:
*
* Requires: MBEDTLS_CIPHER_C
*/
#define MBEDTLS_SSL_TICKET_C

/**
* \def MBEDTLS_SSL_CLI_C
*
* Enable the SSL/TLS client code.
*
* Module:  library/ssl_cli.c
* Caller:
*
* Requires: MBEDTLS_SSL_TLS_C
*
* This module is required for SSL/TLS client support.
*/
#define MBEDTLS_SSL_CLI_C


/**
* \def MBEDTLS_SSL_TLS_C
*
* Enable the generic SSL/TLS code.
*
* Module:  library/ssl_tls.c
* Caller:  library/ssl_cli.c
*          library/ssl_srv.c
*
* Requires: MBEDTLS_CIPHER_C, MBEDTLS_MD_C
*           and at least one of the MBEDTLS_SSL_PROTO_XXX defines
*
* This module is required for SSL/TLS.
*/
#define MBEDTLS_SSL_TLS_C

/**
* \def MBEDTLS_TIMING_C
*
* Enable the semi-portable timing interface.
*
* \note The provided implementation only works on POSIX/Unix (including Linux,
* BSD and OS X) and Windows. On other platforms, you can either disable that
* module and provide your own implementations of the callbacks needed by
* \c mbedtls_ssl_set_timer_cb() for DTLS, or leave it enabled and provide
* your own implementation of the whole module by setting
* \c MBEDTLS_TIMING_ALT in the current file.
*
* \note See also our Knowledge Base article about porting to a new
* environment:
* https://tls.mbed.org/kb/how-to/how-do-i-port-mbed-tls-to-a-new-environment-OS
*
* Module:  library/timing.c
* Caller:  library/havege.c
*
* This module is used by the HAVEGE random number generator.
*/
#define MBEDTLS_TIMING_C

/**
* \def MBEDTLS_VERSION_C
*
* Enable run-time version information.
*
* Module:  library/version.c
*
* This module provides run-time version information.
*/
#define MBEDTLS_VERSION_C

/**
* \def MBEDTLS_X509_USE_C
*
* Enable X.509 core for using certificates.
*
* Module:  library/x509.c
* Caller:  library/x509_crl.c
*          library/x509_crt.c
*          library/x509_csr.c
*
* Requires: MBEDTLS_ASN1_PARSE_C, MBEDTLS_BIGNUM_C, MBEDTLS_OID_C,
*           MBEDTLS_PK_PARSE_C
*
* This module is required for the X.509 parsing modules.
*/
#define MBEDTLS_X509_USE_C

/**
* \def MBEDTLS_X509_CRT_PARSE_C
*
* Enable X.509 certificate parsing.
*
* Module:  library/x509_crt.c
* Caller:  library/ssl_cli.c
*          library/ssl_srv.c
*          library/ssl_tls.c
*
* Requires: MBEDTLS_X509_USE_C
*
* This module is required for X.509 certificate parsing.
*/
#define MBEDTLS_X509_CRT_PARSE_C

/**
* \def MBEDTLS_X509_CRL_PARSE_C
*
* Enable X.509 CRL parsing.
*
* Module:  library/x509_crl.c
* Caller:  library/x509_crt.c
*
* Requires: MBEDTLS_X509_USE_C
*
* This module is required for X.509 CRL parsing.
*/
#define MBEDTLS_X509_CRL_PARSE_C

/* \} name SECTION: mbed TLS modules */

/**
* \name SECTION: Module configuration options
*
* This section allows for the setting of module specific sizes and
* configuration options. The default values are already present in the
* relevant header files and should suffice for the regular use cases.
*
* Our advice is to enable options and change their values here
* only if you have a good reason and know the consequences.
*
* Please check the respective header file for documentation on these
* parameters (to prevent duplicate documentation).
* \{
*/


/**
* \brief       This macro is invoked by the library when an invalid parameter
*              is detected that is only checked with MBEDTLS_CHECK_PARAMS
*              (see the documentation of that option for context).
*
*              When you leave this undefined here, a default definition is
*              provided that invokes the function mbedtls_param_failed(),
*              which is declared in platform_util.h for the benefit of the
*              library, but that you need to define in your application.
*
*              When you define this here, this replaces the default
*              definition in platform_util.h (which no longer declares the
*              function mbedtls_param_failed()) and it is your responsibility
*              to make sure this macro expands to something suitable (in
*              particular, that all the necessary declarations are visible
*              from within the library - you can ensure that by providing
*              them in this file next to the macro definition).
*
*              Note that you may define this macro to expand to nothing, in
*              which case you don't have to worry about declarations or
*              definitions. However, you will then be notified about invalid
*              parameters only in non-void functions, and void function will
*              just silently return early on invalid parameters, which
*              partially negates the benefits of enabling
*              #MBEDTLS_CHECK_PARAMS in the first place, so is discouraged.
*
* \param cond  The expression that should evaluate to true, but doesn't.
*/
//#define MBEDTLS_PARAM_FAILED( cond )               assert( cond )


/* SSL options */


/**
* Allow SHA-1 in the default TLS configuration for TLS 1.2 handshake
* signature and ciphersuite selection. Without this build-time option, SHA-1
* support must be activated explicitly through mbedtls_ssl_conf_sig_hashes.
* The use of SHA-1 in TLS <= 1.1 and in HMAC-SHA-1 is always allowed by
* default. At the time of writing, there is no practical attack on the use
* of SHA-1 in handshake signatures, hence this option is turned on by default
* to preserve compatibility with existing peers, but the general
* warning applies nonetheless:
*
* \warning   SHA-1 is considered a weak message digest and its use constitutes
*            a security risk. If possible, we recommend avoiding dependencies
*            on it, and considering stronger message digests instead.
*
*/
#define MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_KEY_EXCHANGE


/* \} name SECTION: Customisation configuration options */


#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */
