/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SamlAuthDialog.h"

#include <objbase.h>
#include <stdexcept>
#include <wrl.h>
#include "resources/resource.h"
#include "tools/ErrUtil.h"
#include "tools/Strutil.h"


namespace ui {
	using namespace Microsoft::WRL;

	class webview2_error : public std::runtime_error
	{
	public:
		webview2_error(HRESULT error, const char* message) :
			std::runtime_error(message),
			error(error)
		{
		}

		webview2_error(HRESULT result, const std::string& message) :
			webview2_error(result, message.c_str())
		{
		}

		const HRESULT error;
	};



	SamlAuthDialog::SamlAuthDialog(HINSTANCE hInstance, HWND hParent) :
		ModalDialog(hInstance, hParent, IDD_SAMLAUTH_DIALOG),
		_logger(tools::Logger::get_logger()),
		_web_controller(),
		_can_close(false),
		_last_saml_error(saml_err::NONE),
		_service_provider_url(),
		_service_provider_cookies()
	{
		DEBUG_CTOR(_logger, "SamlAuthDialog");

		HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if (FAILED(hr)) {
			_logger->error("ERROR: CoInitializeEx error %x", hr);
			throw tools::win_err(hr);
		}
	}

	
	SamlAuthDialog::~SamlAuthDialog()
	{
		DEBUG_DTOR(_logger, "SamlAuthDialog");
		::CoUninitialize();
	}


	void SamlAuthDialog::set_service_provider_url(const http::Url& url)
	{
		_service_provider_url = url;
	}


	const http::Url& SamlAuthDialog::get_service_provider_url() const
	{
		return _service_provider_url;
	}


	void SamlAuthDialog::set_service_provider_crt(const std::string& crt)
	{
		_service_provider_crt = crt;
	}


	const std::string& SamlAuthDialog::get_service_provider_crt() const
	{
		return _service_provider_crt;
	}


	const http::Cookies& SamlAuthDialog::get_service_provider_cookies() const
	{
		return _service_provider_cookies;
	}


	ui::saml_err SamlAuthDialog::get_saml_error() const
	{
		return _last_saml_error;
	}

	
	INT_PTR SamlAuthDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		DEBUG_ENTER(_logger, "SamlAuthDialog", "onCreateDialogMessage");

		auto web_core_created = [this](HRESULT result, ICoreWebView2Controller* controller) {
			if (FAILED(result)) {
				_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
				_logger->error("ERROR: unable to create WebView2 controller hr=%x", result);

				close_dialog(false);
			}
			else {
				try {
					wil::com_ptr<ICoreWebView2> web_view;
					_web_controller = controller;
					HRESULT hr = _web_controller->get_CoreWebView2(&web_view);
					if (FAILED(hr))
						throw webview2_error(hr, "get_CoreWebView2");

					// Set up WebView bounds
					RECT webview_bounds = get_client_rect();
					RECT status_rect = get_control_rect(IDC_SAML_STATUS);
					webview_bounds.bottom -= (status_rect.bottom - status_rect.top);
					hr = _web_controller->put_Bounds(webview_bounds);
					if (FAILED(hr))
						throw webview2_error(hr, "put_Bounds");

					// Disable WebView functionalities
					wil::com_ptr<ICoreWebView2Settings> settings;
					hr = web_view->get_Settings(&settings);
					if (FAILED(hr))
						throw webview2_error(hr, "get_Settings");
					settings->put_AreDefaultContextMenusEnabled(false);
					settings->put_IsStatusBarEnabled(false);
					settings->put_AreDevToolsEnabled(false);
					settings->put_IsZoomControlEnabled(false);

					// Install a navigation callback
					EventRegistrationToken navCompletedToken;
					hr = web_view->add_NavigationCompleted(
						Callback<ICoreWebView2NavigationCompletedEventHandler>(
							this,
							&SamlAuthDialog::onWebViewNavigationCompleted
						).Get(),
						&navCompletedToken
					);
					if (FAILED(hr))
						throw webview2_error(hr, "add_NavigationCompleted");

					// Install a source changed callback
					EventRegistrationToken sourceChangedToken;
					hr = web_view->add_SourceChanged(
						Callback<ICoreWebView2SourceChangedEventHandler>(
							this,
							&SamlAuthDialog::onSourceChanged
						).Get(),
						&sourceChangedToken
					);
					if (FAILED(hr))
						throw webview2_error(hr, "add_SourceChanged");

					// Install a new window callback that blocks new windows
					EventRegistrationToken newWindowToken;
					hr = web_view->add_NewWindowRequested(
						Callback<ICoreWebView2NewWindowRequestedEventHandler>(
							[this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) {
						args->put_Handled(true);
						return S_OK;
					}
						).Get(),
						&newWindowToken
					);
					if (FAILED(hr))
						throw webview2_error(hr, "add_NewWindowRequested");

					// Get a reference to the cookie manager
					auto web_view2 = web_view.try_query<ICoreWebView2_2>();
					if (web_view2) {
						hr = web_view2->get_CookieManager(&_web_cookie_manager);
						if (FAILED(hr))
							throw webview2_error(hr, "get_CookieManager");
					}

					// Register a certificate error detection that accepts the firewall certificate
					// if it is not validated by webview2.
					auto web_view14 = web_view.try_query<ICoreWebView2_14>();
					if (web_view14) {
						EventRegistrationToken certRegistrationToken;
						hr = web_view14->add_ServerCertificateErrorDetected(
							Callback<ICoreWebView2ServerCertificateErrorDetectedEventHandler>(
								this,
								&SamlAuthDialog::onWebViewServerCertificateErrorDetected
							).Get(),
							&certRegistrationToken
						);

						if (FAILED(hr))
							throw webview2_error(hr, "add_ServerCertificateErrorDetected");
					}

					// Navigate to URL
					std::string status_message = tools::string_format("Connecting to %s://%s",
						_service_provider_url.get_scheme().c_str(),
						_service_provider_url.get_authority().c_str()
					);

					set_control_text(IDC_SAML_STATUS, tools::str2wstr(status_message));
					hr = web_view->Navigate(tools::str2wstr(_service_provider_url.to_string(false)).c_str());
					if (FAILED(hr))
						throw webview2_error(hr, "Navigate");

				}
				catch (webview2_error& e) {
					_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
					_logger->error(
						"ERROR: unable to create WebView2 SAML dialog hr=%x function=%s",
						e.error,
						e.what()
					);

					close_dialog(false);
				}
			}
			return S_OK;
		};

		auto web_env_created = [this, web_core_created](HRESULT result, ICoreWebView2Environment* env) {
			if (FAILED(result)) {
				_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
				_logger->error("ERROR: unable to create WebView2 environment hr=%x", result);

				close_dialog(false);
			}
			else {
				// Create WebView2 controller
				HRESULT hr = env->CreateCoreWebView2Controller(
					window_handle(),
					Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(web_core_created).Get()
				);

				if (FAILED(hr)) {
					_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
					_logger->error("ERROR: unable to create WebView2 SAML dialog hr=%x", hr);

					close_dialog(false);
				}
			}

			return S_OK;
		};


		set_control_text(IDC_SAML_STATUS, L"Initializing a web browser");
		HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
			nullptr,
			nullptr,
			nullptr,
			Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(web_env_created).Get()
		);

		if (FAILED(hr)) {
			_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
			_logger->error("ERROR: unable to create WebView2 SAML dialog hr=%x", hr);

			close_dialog(false);
		}

		return S_OK;
	}


	INT_PTR SamlAuthDialog::onSysCommandMessage(WPARAM wParam, LPARAM lParam)
	{
		INT_PTR rc = TRUE;

		switch (wParam) {
		case SC_CLOSE:
			if (_can_close) {
				// Close this dialog
				_last_saml_error = ui::saml_err::LOGIN_CANCELLED;
				_logger->error("ERROR: SAML login cancelled");

				close_dialog(false);
			}
			break;

		default:
			rc = FALSE;
		}

		return rc;
	}


	HRESULT SamlAuthDialog::onWebViewNavigationCompleted(
		ICoreWebView2* sender, 
		ICoreWebView2NavigationCompletedEventArgs* args
	)
	{
		TRACE_ENTER(_logger, "SamlAuthDialog", "onWebViewNavigationCompleted");

		// The user can decide to close the webview dialog
		_can_close = true;

		if (!sender || !args) {
			_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
			_logger->error("ERROR: invalid argument in onWebViewNavigationCompleted");

		}
		else {
			try {
				BOOL is_success = FALSE;
				HRESULT hr = args->get_IsSuccess(&is_success);
				if (FAILED(hr))
					throw webview2_error(hr, "get_IsSuccess");

				if (!is_success) {
					COREWEBVIEW2_WEB_ERROR_STATUS web_error_status;
					hr = args->get_WebErrorStatus(&web_error_status);
					if (FAILED(hr))
						throw webview2_error(hr, "get_WebErrorStatus");

					switch (web_error_status)
					{
					case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT:
					case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED:
					case COREWEBVIEW2_WEB_ERROR_STATUS_CLIENT_CERTIFICATE_CONTAINS_ERRORS:
					case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED:
					case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID:
						// error is delayed until the navigation completion event
						_last_saml_error = ui::saml_err::NONE;
						break;

					case COREWEBVIEW2_WEB_ERROR_STATUS_SERVER_UNREACHABLE:
					case COREWEBVIEW2_WEB_ERROR_STATUS_TIMEOUT:
					case COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED:
					case COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_RESET:
					case COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED:
					case COREWEBVIEW2_WEB_ERROR_STATUS_CANNOT_CONNECT:
					case COREWEBVIEW2_WEB_ERROR_STATUS_HOST_NAME_NOT_RESOLVED:
						_logger->error("ERROR: SAML connection failed, error=%d", web_error_status);
						_last_saml_error = ui::saml_err::COMM_ERROR;
						break;

					case COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED:
						_logger->error("ERROR: SAML connection cancelled, error=%d", web_error_status);
						_last_saml_error = ui::saml_err::LOGIN_CANCELLED;
						break;

					case COREWEBVIEW2_WEB_ERROR_STATUS_ERROR_HTTP_INVALID_SERVER_RESPONSE:
					case COREWEBVIEW2_WEB_ERROR_STATUS_REDIRECT_FAILED:
					case COREWEBVIEW2_WEB_ERROR_STATUS_UNEXPECTED_ERROR:
					case COREWEBVIEW2_WEB_ERROR_STATUS_VALID_AUTHENTICATION_CREDENTIALS_REQUIRED:
					case COREWEBVIEW2_WEB_ERROR_STATUS_VALID_PROXY_AUTHENTICATION_REQUIRED:
						_logger->error("ERROR: SAML http failure, error=%d", web_error_status);
						_last_saml_error = ui::saml_err::HTTP_ERROR;
						break;

					default:
						_logger->error("ERROR: SAML unexpected failure, error=%d", web_error_status);
						_last_saml_error = ui::saml_err::HTTP_ERROR;
						break;
					}
				}
				else {
					wil::unique_cotaskmem_string source_uri;

					hr = sender->get_Source(&source_uri);
					if (FAILED(hr))
						throw webview2_error(hr, "get_IsSuccess");

					const http::Url source_url{ tools::wstr2str(source_uri.get()) };
					if (tools::iequal(source_url.get_hostname(), _service_provider_url.get_hostname())) {
						hr = _web_cookie_manager->GetCookies(
							tools::str2wstr(_service_provider_url.get_hostname()).c_str(),
							Callback<ICoreWebView2GetCookiesCompletedHandler>(this, &SamlAuthDialog::onCookiesAvailable).Get()
						);
					}
				}
			}
			catch (webview2_error& e) {
				_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
				_logger->error(
					"ERROR: WebView2 Navigate callback error hr=%x function=%s",
					e.error,
					e.what()
				);
			}
		}

		return S_OK;
	}


	HRESULT SamlAuthDialog::onSourceChanged(ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args)
	{
		TRACE_ENTER(_logger, "SamlAuthDialog", "onSourceChanged");
		if (!sender || !args) {
			_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
			_logger->error("ERROR: invalid argument in onSourceChanged");

		}
		else {
			wil::unique_cotaskmem_string uri;
			HRESULT hr = sender->get_Source(&uri);
			if (FAILED(hr))
				return S_FALSE;
			set_control_text(IDC_SAML_STATUS, uri.get());
		}

		return S_OK;
	}

	
	HRESULT SamlAuthDialog::onWebViewServerCertificateErrorDetected(
		ICoreWebView2* sender, 
		ICoreWebView2ServerCertificateErrorDetectedEventArgs* args
	)
	{
		TRACE_ENTER(_logger, "SamlAuthDialog", "onWebViewServerCertificateErrorDetected");
		if (!sender || !args) {
			_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
			_logger->error("ERROR: invalid argument in onWebViewServerCertificateErrorDetected");
		}
		else {
			try {
				HRESULT hr;

				wil::com_ptr<ICoreWebView2Certificate> certificate = nullptr;
				hr = args->get_ServerCertificate(&certificate);
				if (FAILED(hr))
					throw webview2_error(hr, "get_ServerCertificate");

				wil::unique_cotaskmem_string pem;
				hr = certificate->ToPemEncoding(&pem);
				if (FAILED(hr))
					throw webview2_error(hr, "ToPemEncoding");

				if (tools::iequal(tools::wstr2str(std::wstring(pem.get())), _service_provider_crt)) {
					hr = args->put_Action(
						COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION::COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW);
					if (FAILED(hr))
						throw webview2_error(hr, "put_Action");
				}
			}
			catch (webview2_error& e) {
				_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
				_logger->error(
					"ERROR: WebView2 Certificate Error callback error hr=%x function=%s",
					e.error,
					e.what()
				);

			}
		}

		return S_OK;
	}



	static std::string FormatExpires(double expires) {
		if (expires == 0) {
			return "Session";
		}

		// Convert the Unix timestamp to time_t
		time_t rawTime = static_cast<time_t>(expires);

		// Convert to a tm structure
		struct tm timeInfo;
		gmtime_s(&timeInfo, &rawTime); // Use gmtime_s for thread-safe conversion

		// Format as a string
		char buffer[100];
		strftime(buffer, sizeof(buffer) / sizeof(wchar_t), "%Y-%m-%d %H:%M:%S UTC", &timeInfo);

		return std::string(buffer);
	}

	static http::Cookie WebviewCookie2Cookie(ICoreWebView2Cookie* webview2_cookie)
	{
		HRESULT hr;

		wil::unique_cotaskmem_string cookie_domain;
		hr = webview2_cookie->get_Domain(&cookie_domain);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Domain");

		wil::unique_cotaskmem_string cookie_value;
		hr = webview2_cookie->get_Value(&cookie_value);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Value");

		wil::unique_cotaskmem_string cookie_name;
		hr = webview2_cookie->get_Name(&cookie_name);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Name");

		wil::unique_cotaskmem_string cookie_path;
		hr = webview2_cookie->get_Path(&cookie_path);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Path");

		double expires_value = 0.0;
		hr = webview2_cookie->get_Expires(&expires_value);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Expires");

		BOOL isSecure = FALSE;
		hr = webview2_cookie->get_IsSecure(&isSecure);
		if (FAILED(hr))
			throw webview2_error(hr, "get_IsSecure");

		BOOL httpOnly = FALSE;
		hr = webview2_cookie->get_IsHttpOnly(&httpOnly);
		if (FAILED(hr))
			throw webview2_error(hr, "get_IsHttpOnly");

		return http::Cookie(
			tools::wstr2str(std::wstring(cookie_name.get())),
			tools::obfstring(tools::wstr2str(std::wstring(cookie_value.get()))),
			tools::wstr2str(std::wstring(cookie_domain.get())),
			tools::wstr2str(std::wstring(cookie_path.get())),
			FormatExpires(expires_value),
			isSecure != 0,
			httpOnly != 0
		);
	}

	HRESULT SamlAuthDialog::onCookiesAvailable(
		HRESULT result,
		ICoreWebView2CookieList* list
	)
	{
		TRACE_ENTER(_logger, "SamlAuthDialog", "onCookiesAvailable");
		if (FAILED(result)) {
			_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
			_logger->error("ERROR: unable to create WebView2 environment hr=%x", result);
		}
		else {
			try {
				HRESULT hr;
				UINT cookie_list_size;

				hr = list->get_Count(&cookie_list_size);

				for (UINT i = 0; i < cookie_list_size; ++i)
				{
					wil::com_ptr<ICoreWebView2Cookie> webview2_cookie;
					hr = list->GetValueAtIndex(i, &webview2_cookie);

					if (webview2_cookie)
					{
						http::Cookie cookie = WebviewCookie2Cookie(webview2_cookie.get());
						if (tools::iequal(cookie.get_domain(), _service_provider_url.get_hostname()) && 
							cookie.is_secure()) {
							_service_provider_cookies.add(cookie);
						}

							// We have the cookie, we can close the dialog
							//close_dialog(1);

							//_web_cookie_manager->DeleteCookie(webview2_cookie.get());
					}
				}
			}
			catch (webview2_error& e) {
				_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
				_logger->error(
					"ERROR: WebView2 onCookiesAvailable callback error hr=%x function=%s",
					e.error,
					e.what()
				);
			}
		}
		return S_OK;
	}

}
