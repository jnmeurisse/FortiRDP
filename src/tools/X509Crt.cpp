/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "X509Crt.h"

#include <windows.h>
#include <wincrypt.h>
#include <vector>
#include <mbedtls/base64.h>
#include <mbedtls/pem.h>



namespace tools {
	
	X509Crt::X509Crt()
	{
		mbedtls_x509_crt_init(&_crt);
	}

	
	X509Crt::~X509Crt()
	{
		mbedtls_x509_crt_free(&_crt);
	}


	mbed_err X509Crt::load(const char* filename)
	{
		return mbedtls_x509_crt_parse_file(&_crt, filename);
	}


	mbed_err X509Crt::get_info(char* buf, size_t size, const char* prefix) const
	{
		mbed_err errnum = mbedtls_x509_crt_info(buf, size, "   ", &_crt);

		return errnum >= 0 ? 0 : errnum;
	}



	bool X509crt_to_pem(const mbedtls_x509_crt* crt, std::string& pem)
	{
		constexpr auto pem_begin_crt{ "-----BEGIN CERTIFICATE-----\n" };
		constexpr auto pem_end_crt{ "-----END CERTIFICATE-----\n" };
		size_t buffer_size{ 1024 };

		int ret = 0;
		if (crt) {
			do {
				size_t written{};
				std::vector<unsigned char> buffer(buffer_size);

				ret = mbedtls_pem_write_buffer(
					pem_begin_crt,
					pem_end_crt,
					crt->raw.p,
					crt->raw.len,
					&buffer[0],
					buffer.size(),
					&written
				);

				if (ret == 0)
					pem = std::string((char*)&buffer[0]);
				else if (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
					buffer_size += 1024;
			} while (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL);
		}

		return crt && (ret == 0);
	}


	static DWORD WinVerifySslCert(PCCERT_CONTEXT certContext) {
		DWORD errorStatus = -1;

		static const LPCSTR usage[] = {
			szOID_PKIX_KP_SERVER_AUTH,
			szOID_SERVER_GATED_CRYPTO,
			szOID_SGC_NETSCAPE
		};

		CERT_CHAIN_PARA chainParameter = {0};
		chainParameter.cbSize = sizeof(CERT_CHAIN_PARA);
		chainParameter.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
		chainParameter.RequestedUsage.Usage.cUsageIdentifier = ARRAYSIZE(usage);
		chainParameter.RequestedUsage.Usage.rgpszUsageIdentifier = const_cast<LPSTR*>(usage);

		PCCERT_CHAIN_CONTEXT chainContext = NULL;
		if (::CertGetCertificateChain(NULL, certContext, NULL, NULL, &chainParameter, 0, NULL, &chainContext) &&
			chainContext) {
			errorStatus = chainContext->TrustStatus.dwErrorStatus;
			::CertFreeCertificateChain(chainContext);
		}

		return errorStatus;
	}


	bool x509crt_is_trusted(const mbedtls_x509_crt* crt)
	{
		bool status = false;

		// Create a certificate context from the DER certificate representation.
		PCCERT_CONTEXT pCertContext = ::CertCreateCertificateContext(
			X509_ASN_ENCODING,
			crt->raw.p,
			static_cast<DWORD>(crt->raw.len)
		);

		if (pCertContext) {
			// Verify if the certificate is signed by a trusted CA.
			status = WinVerifySslCert(pCertContext) == 0;
			::CertFreeCertificateContext(pCertContext);
		}

		return status;
	}

}
