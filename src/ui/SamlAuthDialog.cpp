/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SamlAuthDialog.h"

#include <cmath>
#include <objbase.h>
#include <stdexcept>
#include <wrl.h>
#include "resources/resource.h"
#include "util/ErrUtil.h"
#include "util/Path.h"
#include "util/Strutil.h"


namespace ui {
	using namespace Microsoft::WRL;
	using namespace utl;

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


	static const char* web_error_status_message(COREWEBVIEW2_WEB_ERROR_STATUS error_status)
	{
		switch (error_status) {
		case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT:
			return "Certificate common name is incorrect";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED:
			return "Certificate expired";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CLIENT_CERTIFICATE_CONTAINS_ERRORS:
			return "Client certificate contains errors";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED:
			return "Certificate was revoked";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID:
			return "Certificate is invalid";
		case COREWEBVIEW2_WEB_ERROR_STATUS_SERVER_UNREACHABLE:
			return "Server unreachable";
		case COREWEBVIEW2_WEB_ERROR_STATUS_TIMEOUT:
			return "Server timeout";
		case COREWEBVIEW2_WEB_ERROR_STATUS_ERROR_HTTP_INVALID_SERVER_RESPONSE:
			return "Invalid server HTTP response";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED:
			return "Connection aborted";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_RESET:
			return "Connection reset";
		case COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED:
			return "Disconnected";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CANNOT_CONNECT:
			return "Connection error";
		case COREWEBVIEW2_WEB_ERROR_STATUS_HOST_NAME_NOT_RESOLVED:
			return "hostname not resolved";
		case COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED:
			return "Operation cancelled";
		case COREWEBVIEW2_WEB_ERROR_STATUS_REDIRECT_FAILED:
			return "Redirect failed";
		case COREWEBVIEW2_WEB_ERROR_STATUS_VALID_AUTHENTICATION_CREDENTIALS_REQUIRED:
			return "Authentication credentials required";
		case COREWEBVIEW2_WEB_ERROR_STATUS_VALID_PROXY_AUTHENTICATION_REQUIRED:
			return "Proxy authentication required";
		case COREWEBVIEW2_WEB_ERROR_STATUS_UNEXPECTED_ERROR:
		default:
			return "Unexpected error";

		}
	}


	SamlAuthDialog::SamlAuthDialog(HINSTANCE hInstance, HWND hParent, fw::AuthSamlInfo* pSamlInfo) :
		ModalDialog(hInstance, hParent, IDD_SAMLAUTH_DIALOG),
		_logger(utl::Logger::get_logger()),
		_web_controller(),
		_can_close(false),
		_last_saml_error(saml_err::NONE),
		_saml_auth_info{ *pSamlInfo }
	{
		DEBUG_CTOR(_logger);

		HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(hr)) {
			_logger->error("ERROR: CoInitializeEx error=%x", hr);
			throw utl::win_err(hr);
		}
	}

	
	SamlAuthDialog::~SamlAuthDialog()
	{
		DEBUG_DTOR(_logger);
		::CoUninitialize();
	}


	ui::saml_err SamlAuthDialog::get_saml_error() const
	{
		return _last_saml_error;
	}

	
	INT_PTR SamlAuthDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		DEBUG_ENTER(_logger);

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

					EventRegistrationToken navStartingToken;
					hr = web_view->add_NavigationStarting(
						Callback<ICoreWebView2NavigationStartingEventHandler>(
							this,
							&SamlAuthDialog::onWebViewNavigationStarting
						).Get(),
						&navStartingToken
					);
					if (FAILED(hr))
						throw webview2_error(hr, "add_NavigationStarting");

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
					std::string status_message = str::string_format("Connecting to %s://%s",
						_saml_auth_info.service_provider_url.get_scheme().c_str(),
						_saml_auth_info.service_provider_url.get_authority().c_str()
					);

					set_control_text(IDC_SAML_STATUS, str::str2wstr(status_message));
					hr = web_view->Navigate(str::str2wstr(_saml_auth_info.service_provider_url.to_string(false)).c_str());
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


		std::wstring app_data_path = utl::Path::get_appdata_path().folder() + L"FortiRDP\\";


		set_control_text(IDC_SAML_STATUS, L"Initializing a web browser");
		HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
			nullptr,
			app_data_path.c_str(),
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


	HRESULT SamlAuthDialog::onWebViewNavigationStarting(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
	{
		TRACE_ENTER(_logger);
		if (!sender || !args) {
			_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
			_logger->error("ERROR: invalid argument in onWebViewNavigationStarting");

		}
		else {
			wil::unique_cotaskmem_string uri;
			HRESULT hr = args->get_Uri(&uri);
			if (FAILED(hr))
				return S_FALSE;
			set_control_text(IDC_SAML_STATUS, uri.get());
		}

		return S_OK;
	}

	HRESULT SamlAuthDialog::onWebViewNavigationCompleted(
		ICoreWebView2* sender, 
		ICoreWebView2NavigationCompletedEventArgs* args
	)
	{
		TRACE_ENTER(_logger);

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
						_logger->error("ERROR: SAML connection failed. %s (%d).",
							web_error_status_message(web_error_status),
							web_error_status);
						_last_saml_error = ui::saml_err::COMM_ERROR;
						break;

					case COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED:
						_logger->error("ERROR: SAML connection failed. %s (%d).",
							web_error_status_message(web_error_status),
							web_error_status);
						_last_saml_error = ui::saml_err::LOGIN_CANCELLED;
						break;

					case COREWEBVIEW2_WEB_ERROR_STATUS_ERROR_HTTP_INVALID_SERVER_RESPONSE:
					case COREWEBVIEW2_WEB_ERROR_STATUS_REDIRECT_FAILED:
					case COREWEBVIEW2_WEB_ERROR_STATUS_UNEXPECTED_ERROR:
					case COREWEBVIEW2_WEB_ERROR_STATUS_VALID_AUTHENTICATION_CREDENTIALS_REQUIRED:
					case COREWEBVIEW2_WEB_ERROR_STATUS_VALID_PROXY_AUTHENTICATION_REQUIRED:
						_logger->error("ERROR: SAML connection failed. %s (%d).",
							web_error_status_message(web_error_status),
							web_error_status);
						_last_saml_error = ui::saml_err::HTTP_ERROR;
						break;

					default:
						_logger->error("ERROR: SAML connection failed. %s.", web_error_status_message(web_error_status));
						_last_saml_error = ui::saml_err::HTTP_ERROR;
						break;
					}
				}
				else {
					wil::unique_cotaskmem_string source_uri;

					hr = sender->get_Source(&source_uri);
					if (FAILED(hr))
						throw webview2_error(hr, "get_IsSuccess");

					const http::Url source_url{ str::wstr2str(source_uri.get()) };
					if (str::iequal(source_url.get_hostname(), _saml_auth_info.service_provider_url.get_hostname())) {
						hr = _web_cookie_manager->GetCookies(
							str::str2wstr(_saml_auth_info.service_provider_url.to_string(false)).c_str(),
							Callback<ICoreWebView2GetCookiesCompletedHandler>(this, &SamlAuthDialog::onCookiesAvailable).Get()
						);
						if (FAILED(hr))
							throw webview2_error(hr, "GetCookies");
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
		TRACE_ENTER(_logger);
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
		TRACE_ENTER(_logger);
		if (!sender || !args) {
			_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
			_logger->error("ERROR: invalid argument in %s", __func__);
		}
		else {
			try {
				HRESULT hr;

				// Is this error detected on the SAML service provide URL ?
				// If yes, we compare the certificate obtained during the initial connection and
				// stored in the class field _service_provider_crt with the certificate provided
				// by the web server generating the certificate detection error.  At this step, 
				// send->get_Source returns about:blank because the connection is not yet validated.
				// The OnSource event registered the target URI in the IDC_SAML_STATUS control.

				http::Url web_server_url{ str::wstr2str(get_control_text(IDC_SAML_STATUS)) };
				if (str::iequal(web_server_url.get_authority(), _saml_auth_info.service_provider_url.get_authority())) {
					// Get a reference to the web server certificate
					wil::com_ptr<ICoreWebView2Certificate> certificate_ptr = nullptr;
					hr = args->get_ServerCertificate(&certificate_ptr);
					if (FAILED(hr))
						throw webview2_error(hr, "get_ServerCertificate");

					// convert the web server certificate to PEM format.
					wil::unique_cotaskmem_string certificate_pem;
					hr = certificate_ptr->ToPemEncoding(&certificate_pem);
					if (FAILED(hr))
						throw webview2_error(hr, "ToPemEncoding");

					// Compare the web server certificate and the SAML service provider certificate.
					// If both matches, we do not warn the user as it has been previously accepted during
					// the initial connection.
					if (str::iequal(str::wstr2str(certificate_pem.get()), _saml_auth_info.service_provider_crt)) {
						hr = args->put_Action(
							COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION::COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW);
						if (FAILED(hr))
							throw webview2_error(hr, "put_Action");
					}
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


	static std::time_t dcom_time_to_time_t(double time)
	{
		constexpr double unix_start_time = 25569.0f; 
		return llround(time - unix_start_time) * 86400;
	}


	static http::Cookie WebviewCookie2Cookie(ICoreWebView2Cookie& webview2_cookie)
	{
		HRESULT hr;

		wil::unique_cotaskmem_string cookie_domain;
		hr = webview2_cookie.get_Domain(&cookie_domain);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Domain");

		wil::unique_cotaskmem_string cookie_value;
		hr = webview2_cookie.get_Value(&cookie_value);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Value");

		wil::unique_cotaskmem_string cookie_name;
		hr = webview2_cookie.get_Name(&cookie_name);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Name");

		wil::unique_cotaskmem_string cookie_path;
		hr = webview2_cookie.get_Path(&cookie_path);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Path");

		double expires_value = 0.0;
		hr = webview2_cookie.get_Expires(&expires_value);
		if (FAILED(hr))
			throw webview2_error(hr, "get_Expires");

		BOOL isSecure = FALSE;
		hr = webview2_cookie.get_IsSecure(&isSecure);
		if (FAILED(hr))
			throw webview2_error(hr, "get_IsSecure");

		BOOL httpOnly = FALSE;
		hr = webview2_cookie.get_IsHttpOnly(&httpOnly);
		if (FAILED(hr))
			throw webview2_error(hr, "get_IsHttpOnly");

		BOOL isSession = FALSE;
		hr = webview2_cookie.get_IsSession(&isSession);
		if (FAILED(hr))
			throw webview2_error(hr, "get_IsSession");

		return http::Cookie(
			str::wstr2str(cookie_name.get()),
			utl::obfstring(str::wstr2str(cookie_value.get())),
			str::wstr2str(cookie_domain.get()),
			str::wstr2str(cookie_path.get()),
			isSession ? -1 : dcom_time_to_time_t(expires_value),
			isSecure != 0,
			httpOnly != 0
		);
	}

	HRESULT SamlAuthDialog::onCookiesAvailable(
		HRESULT result,
		ICoreWebView2CookieList* list
	)
	{
		TRACE_ENTER(_logger);

		if (FAILED(result)) {
			_last_saml_error = ui::saml_err::WEBVIEW_ERROR;
			_logger->error("ERROR: unable to create WebView2 environment hr=%x", result);
		}
		else {
			try {
				HRESULT hr;
				UINT cookie_list_size =0;

				hr = list->get_Count(&cookie_list_size);
				if (FAILED(hr))
					throw webview2_error(hr, "get_Count");

				for (UINT i = 0; i < cookie_list_size; ++i)
				{
					wil::com_ptr<ICoreWebView2Cookie> webview2_cookie;
					hr = list->GetValueAtIndex(i, &webview2_cookie);
					if (FAILED(hr))
						throw webview2_error(hr, "GetValueAtIndex");

					if (webview2_cookie.get())
					{
						const http::Cookie cookie{ WebviewCookie2Cookie(*webview2_cookie.get()) };

						if (cookie.same_domain(_saml_auth_info.service_provider_url.get_hostname()) &&
							cookie.is_http_only() &&
							cookie.is_secure()) 
						{
							if (cookie.is_expired())
								_saml_auth_info.cookie_jar.remove(cookie.get_name());
							else
								_saml_auth_info.cookie_jar.add(cookie);
							_web_cookie_manager->DeleteCookie(webview2_cookie.get());
						}
					}
				}

				if (_saml_auth_info.is_authenticated()) {
					// We are authenticated on the firewall portal
					close_dialog(TRUE);
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

	const char* SamlAuthDialog::__class__ = "SamlAuthDialog";

}
