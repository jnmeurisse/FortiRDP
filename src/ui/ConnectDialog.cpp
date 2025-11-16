/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "ConnectDialog.h"

#include <malloc.h>
#include <stdexcept>
#include <system_error>
#include <vector>
#include "ui/AboutDialog.h"
#include "ui/AsyncMessage.h"
#include "ui/CredentialDialog.h"
#include "ui/OptionsDialog.h"
#include "ui/PinCodeDialog.h"
#include "ui/SamlAuthDialog.h"
#include "tools/Mutex.h"
#include "tools/StrUtil.h"
#include "tools/SysUtil.h"
#include "resources/resource.h"


namespace ui {

	// ID for the system menu
	static const int SYSCMD_ABOUT = 1;
	static const int SYSCMD_LAUNCH = 2;
	static const int SYSCMD_OPTIONS = 3;

	ConnectDialog::ConnectDialog(HINSTANCE hInstance, const CmdlineParams& params) :
		ModelessDialog(hInstance, NULL, IDD_CONNECT_DIALOG),
		_params(params),
		_writer(),
		_logger(tools::Logger::get_logger())
	{
		// Assign application icons. Icons are automatically deleted when the 
		// application stops thanks to the LR_SHARED parameter.
		HICON hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_FORTIRDP), IMAGE_ICON, 16, 16, LR_SHARED);
		::SendMessage(window_handle(), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_FORTIRDP), IMAGE_ICON, 128, 128, LR_SHARED);
		::SendMessage(window_handle(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		// Configure the status text font. The color is assigned later when responding
		// to WM_CTLCOLORSTATIC message.
		_msg_font = create_font(10, L"Tahoma");
		set_control_font(IDC_STATUSTEXT, _msg_font);

		// Create a black brush used to fill the IDC_STATUSTEXT control.
		_bg_brush = ::CreateSolidBrush(RGB(0, 0, 0));

		_anim_font = create_font(10, L"Consolas");
		if (!_anim_font)
			_anim_font = create_font(10, L"Arial");
		set_control_font(IDC_ACTIVITY, _anim_font);

		// Add menus : about, options and launch sub menu (and associated hot keys)
		HMENU hMenu = get_sys_menu(false);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
		if (params.is_mstsc())
			::AppendMenu(hMenu, MF_STRING, SYSCMD_OPTIONS, L"&Options...");

		if (params.multi_clients() && params.appname().length() > 0) {
			::AppendMenu(hMenu, MF_STRING, SYSCMD_LAUNCH, L"&Launch...\tCtlr+L");
			::RegisterHotKey(window_handle(), SYSCMD_LAUNCH, MOD_CONTROL | MOD_NOREPEAT, 0x4C);
		}

		::AppendMenu(hMenu, MF_STRING, SYSCMD_ABOUT, L"&About...");


		// Link a new log writer to the logger. This log writer sends OutputInfoMessage
		// to this dialog window.
		_writer = new InfoLogWriter(window_handle());
		_logger->add_writer(_writer);


		// Configure the maximum length for address text fields.
		set_control_textlen(IDC_ADDR_FW, MAX_ADDR_LENGTH);
		set_control_textlen(IDC_ADDR_HOST, MAX_ADDR_LENGTH);

		// Initialize address input fields. Take values from command line and if
		// not specified, use the last values saved in the registry.
		setFirewallAddress(!params.firewall_address().empty()
			? params.firewall_address()
			: _settings.get_firewall_address());
		setHostAddress(!params.host_address().empty()
			? params.host_address()
			: _settings.get_host_address());

		// Initialize the user name.
		if (!params.username().empty()) {
			_username = _params.username();
		}
		else {
			// Get user name from last usage and if not available, we use the Windows
			// user name.
			_username = _settings.get_username(tools::get_windows_username());
		}

		//  Load root Certificate Authority.
		tools::Path crt_ca_file;
		if (_params.ca_cert_filename().length() == 0) {
			// When no CA file is specified, we use the default file located in the
			// application folder.
			crt_ca_file = tools::Path(
				tools::Path::get_module_path().folder(),
				L"fortirdp.crt"
			);
		}
		else {
			// Use the CA file specified on the command line.
			crt_ca_file = tools::Path(_params.ca_cert_filename());

			// If no path is specified, assume the .crt file is located in the same
			// directory as the executable.
			if (crt_ca_file.folder().empty()) {
				crt_ca_file = tools::Path(
					tools::Path::get_module_path().folder(),
					crt_ca_file.filename()
				);
			}
		}

		// Allocate the communication controller.
		_controller = std::make_unique<AsyncController>(window_handle());
		_controller->load_ca_crt(crt_ca_file);

		if (params.firewall_address().length() > 0 && params.host_address().length() > 0) {
			// Auto connect if both addresses are specified
			::PostMessage(window_handle(), WM_COMMAND, IDC_CONNECT, 0);
		}
	}


	ConnectDialog::~ConnectDialog()
	{
		::DeleteObject(_bg_brush);
		::DeleteObject(_msg_font);
		::DeleteObject(_anim_font);

		_controller->terminate();
		_controller->wait(1000);

		if (_writer) {
			_logger->remove_writer(_writer);
			delete _writer;
		}
	}


	std::wstring ConnectDialog::getFirewallAddress() const
	{
		return get_control_text(IDC_ADDR_FW);
	}


	void ConnectDialog::setFirewallAddress(const std::wstring& addr)
	{
		set_control_text(IDC_ADDR_FW, addr);
	}


	std::wstring ConnectDialog::getHostAddress() const
	{
		return get_control_text(IDC_ADDR_HOST);
	}


	void ConnectDialog::setHostAddress(const std::wstring& addr)
	{
		set_control_text(IDC_ADDR_HOST, addr);
	}


	void ConnectDialog::clearInfo()
	{
		tools::Mutex::Lock lock{ _msg_mutex };

		_msg_buffer.clear();
		set_control_text(IDC_STATUSTEXT, L"");
	}


	void ConnectDialog::writeInfo(const std::wstring& message)
	{
		tools::Mutex::Lock lock{ _msg_mutex };

		// Add the new message and remove old one, we keep only the last 10 messages
		_msg_buffer.push_back(message);
		while (_msg_buffer.size() > 10) {
			_msg_buffer.erase(_msg_buffer.cbegin());
		}

		// Render the messages buffer
		std::wstring output;
		for (auto const& str : _msg_buffer) {
			output.append(str);
			output.append(L"\n");
		}

		set_control_text(IDC_STATUSTEXT, output);
	}


	static bool in_range(int value, int min, int max)
	{
		return (value >= min) && (value <= max);
	}


	void ConnectDialog::connect(bool clear_log)
	{
		// Check if fw and host addresses are valid.
		try {
			// Split the address and the domain if specified
			std::vector<std::wstring> address_parts;
			if (!in_range(tools::split(tools::trim(getFirewallAddress()), '/', address_parts), 1, 2))
				throw std::invalid_argument("invalid syntax");

			std::string fw_addr{ tools::trim(tools::wstr2str(address_parts[0])) };
			_firewall_endpoint = net::Endpoint(fw_addr, DEFAULT_FW_PORT);

			_firewall_domain = "";
			if (address_parts.size() == 2)
				_firewall_domain = tools::trim(tools::wstr2str(address_parts[1]));

		}
		catch (const std::invalid_argument&) {
			set_focus(IDC_ADDR_FW);
			showErrorMessageDialog(L"Invalid firewall address");
			return;
		}

		try {
			std::string host_addr{ tools::trim(tools::wstr2str(getHostAddress())) };
			_host_endpoint = net::Endpoint(host_addr, DEFAULT_RDP_PORT);

		}
		catch (const std::invalid_argument&) {
			set_focus(IDC_ADDR_HOST);
			showErrorMessageDialog(L"Invalid host address");
			return;
		}

		// Prepare the task exe name and parameters.
		std::vector<std::wstring> task_params;
		std::wstring task_name;

		if (_params.is_mstsc()) {
			task_name = L"C:\\Windows\\system32\\mstsc.exe";

			if (!_params.rdp_filename().empty()) {
				task_params.push_back(_params.rdp_filename());
			}
			else if (_settings.get_rdpfile_mode() && !_settings.get_rdp_filename().empty()) {
				task_params.push_back(_settings.get_rdp_filename());
			}

			task_params.push_back(L"/v:${host}:${port}");

			if (_params.admin_console() || _settings.get_admin_console())
				task_params.push_back(L"/admin");

			if (_params.full_screen() || _settings.get_full_screen())
				task_params.push_back(L"/f");

			else {
				const bool size_from_parameters = _params.screen_size().height > 0 || _params.screen_size().width > 0;
				const ScreenSize screen_size = size_from_parameters ? _params.screen_size() : _settings.get_screen_size();
				if (screen_size.height > 0)
					task_params.push_back(L"/h:" + std::to_wstring(screen_size.height));
				if (screen_size.width > 0)
					task_params.push_back(L"/w:" + std::to_wstring(screen_size.width));
			}

			if (_params.span_mode() || _settings.get_span_mode())
				task_params.push_back(L"/span");

			if (_params.multimon_mode() || _settings.get_multimon_mode())
				task_params.push_back(L"/multimon");

		}
		else {
			std::vector<std::wstring> task_info;
			tools::split(_params.appname(), L';', task_info);

			if (task_info.size() > 0)
				task_name = tools::trim(task_info[0]);
			for (int i = 1; i < task_info.size(); i++)
				task_params.push_back(task_info[i]);
		}
		_task_info = std::make_unique<tools::TaskInfo>(task_name, task_params);

		// Check if the app executable exists.
		if (!task_name.empty() && !tools::file_exists(task_name)) {
			const std::wstring message{ L"Application not found : " + task_name };
			showErrorMessageDialog(message.c_str());
			return;
		}

		// Check if the RDP file exists.
		if (!_params.rdp_filename().empty() && !tools::file_exists(_params.rdp_filename())) {
			const std::wstring message{ L"RDP file not found : " + _task_info->path() };
			showErrorMessageDialog(message.c_str());
			return;
		}

		// Configure the authentication method.
		fw::AuthMethod auth_method = _params.auth_method();
		if (auth_method == fw::AuthMethod::DEFAULT) {
			auth_method = _settings.get_auth_method();
		}
		_controller->set_auth_method(auth_method);

		//  Load user certificate file.
		if (_params.us_cert_filename().length() > 0) {
			tools::Path user_crt;

			// Use the user certificate file specified in the command line.
			user_crt = tools::Path(_params.us_cert_filename());

			// If no path specified, we assume that the .crt file is located next to the .exe
			if (user_crt.folder().empty()) {
				user_crt = tools::Path(
					tools::Path::get_module_path().folder(),
					user_crt.filename()
				);
			}

			if (!tools::file_exists(user_crt)) {
				std::wstring message{ L"User certificate file not found : " + user_crt.to_string() };
				showErrorMessageDialog(message.c_str());
				return;
			}

			auto ask_password = [this](std::string& passcode) {
				PinCodeDialog codeDialog{ instance_handle(), window_handle() };
				const std::string message = "Enter your user certificate password";

				const bool modal_result = codeDialog.show_modal() == TRUE;
				if (modal_result) {
					// Returns code to caller
					passcode = tools::wstr2str(codeDialog.getCode());
				}

				return modal_result;
			};

			if (!_controller->load_user_crt(user_crt, ask_password)) {
				std::wstring message{ L"User certificate file not loaded" };
				showErrorMessageDialog(message.c_str());
				return;
			}

			_controller->set_auth_method(fw::AuthMethod::CERTIFICATE);
		}

		// Clear the information log.
		if (clear_log)
			clearInfo();

		// Disable all controls on this dialog and enable the disconnect button.
		set_control_enable(IDC_CONNECT, false);
		set_control_enable(IDC_DISCONNECT, false);
		set_control_enable(IDC_QUIT, false);
		set_control_enable(IDC_ADDR_FW, false);
		set_control_enable(IDC_ADDR_HOST, false);
		set_control_visible(IDC_CONNECT, false);
		set_control_visible(IDC_DISCONNECT, true);
		if (_params.is_mstsc())
			::EnableMenuItem(get_sys_menu(false), SYSCMD_OPTIONS, MF_BYCOMMAND | MF_DISABLED);

		// Start the client
		_controller->connect(_firewall_endpoint, _firewall_domain);
	}


	void ConnectDialog::disconnect()
	{
		set_control_enable(IDC_DISCONNECT, false);
		_controller->disconnect();
	}


	void ConnectDialog::showCredentialsDialog(fw::AuthCredentials* pCredentials)
	{
		CredentialDialog credentialDialog(instance_handle(), window_handle());
		const std::string message{
			"Enter user name and password to access firewall " +
			_controller->portal_client()->host().hostname()
		};
		credentialDialog.setText(tools::str2wstr(message));
		credentialDialog.setUsername(_username);

		const bool modal_result = credentialDialog.show_modal() == TRUE;
		if (modal_result && pCredentials) {
			// Returns user name and password to caller
			_username = credentialDialog.getUsername();
			pCredentials->username = _username;
			pCredentials->password = credentialDialog.getPassword();

			// Save user name in last usage registry but only if not specified on
			// the command line.
			if (_params.username().empty())
				_settings.set_username(_username);
		}

		::ReplyMessage(modal_result);
	}


	void ConnectDialog::showSamlDialog(fw::AuthSamlInfo* pSamlInfo)
	{
		SamlAuthDialog samlDialog{ instance_handle(), window_handle(), pSamlInfo };

		const bool modal_result = samlDialog.show_modal() == TRUE;

		::ReplyMessage(modal_result);
	}


	void ConnectDialog::showPinCodeDialog(fw::AuthCode* pCode)
	{
		PinCodeDialog codeDialog{ instance_handle(), window_handle() };
		const std::string message = (!pCode)
			? "Enter code to access firewall " + _controller->portal_client()->host().hostname()
			: pCode->prompt;
		codeDialog.setText(tools::str2wstr(message));

		const bool modal_result = codeDialog.show_modal() == TRUE;
		if (modal_result && pCode) {
			// Returns code to caller.
			pCode->code = tools::wstr2str(codeDialog.getCode());
		}

		::ReplyMessage(modal_result);
	}


	INT_PTR ConnectDialog::onDestroyDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();

		// Stop AsyncConnect if still running.
		disconnect();

		// Close this application.
		::PostQuitMessage(0);

		return FALSE;
	}


	INT_PTR ConnectDialog::onCloseDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();

		return ::DestroyWindow(window_handle());
	}


	INT_PTR ConnectDialog::onButtonClick(int control_id, LPARAM lParam)
	{
		INT_PTR rc = FALSE;

		switch (control_id) {
		case IDC_QUIT:
			::DestroyWindow(window_handle());
			break;

		case IDC_CONNECT:
			connect(lParam != 0);
			break;

		case IDC_DISCONNECT:
			disconnect();
			break;

		default:
			rc = TRUE;
			break;
		}

		return rc;
	}


	INT_PTR ConnectDialog::onSysCommandMessage(WPARAM wParam, LPARAM lParam)
	{
		LPARAM_UNUSED();
		INT_PTR rc = TRUE;

		switch (wParam) {
		case SC_CLOSE:
			if (is_control_enabled(IDC_QUIT)) {
				::SendMessage(window_handle(), WM_COMMAND, IDC_QUIT, 0);
			}
			break;

		case SYSCMD_ABOUT:
			showAboutDialog();
			break;

		case SYSCMD_OPTIONS:
			showOptionsDialog();
			break;

		case SYSCMD_LAUNCH:
			startTask();
			break;

		default:
			rc = FALSE;
		}

		return rc;

	}


	INT_PTR ConnectDialog::onCtlColorStaticMessage(WPARAM wParam, LPARAM lParam)
	{
		if (lParam == (LPARAM)control_handle(IDC_STATUSTEXT)) {
			HDC static_text = (HDC)wParam;
			::SetTextColor(static_text, RGB(0, 255, 255));
			::SetBkColor(static_text, RGB(0, 0, 0));

			return (INT_PTR)_bg_brush;

		}
		else {
			return 0;
		}
	}


	INT_PTR ConnectDialog::onTimerMessage(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();

		net::Tunneler* tunneler = nullptr;
		if (_controller)
			tunneler = _controller->tunnel();

		switch (wParam) {
		case TIMER_COUNTERS:
			if (tunneler) {
				const tools::Counters& counters = tunneler->counters();
				const std::string message = tools::string_format(
					"KBytes sent/received : %.1f/%.1f",
					counters.sent / 1024.0,
					counters.received / 1024.0);
				set_control_text(IDC_BYTES_SENT, tools::str2wstr(message));
			}
			break;

		case TIMER_ACTIVITY:
			if (tunneler) {
				const tools::Counters& counters = tunneler->counters();
				const chrono::steady_clock::time_point now = chrono::steady_clock::now();
				if (counters.total() > _previous_counters) {
					// Show network activity
					const wchar_t activity_symbol[] = L"\u2190\u2191\u2192\u2193";
					_previous_counters = counters.total();
					_activity_loop = (_activity_loop + 1) % 4;
					_last_activity = now;

					const std::wstring symbol(1, activity_symbol[_activity_loop]);
					set_control_text(IDC_ACTIVITY, symbol);

				}
				else {
					// Clear display if no activity during a 1/4 second.
					const auto inactive_duration = chrono::duration_cast<chrono::milliseconds>(now - _last_activity);
					if (inactive_duration.count() >= 250)
						set_control_text(IDC_ACTIVITY, L" ");
				}
			}
			break;

		default:
			break;
		}

		return 0;
	}

	INT_PTR ConnectDialog::onHotKey(WPARAM wParam, LPARAM lParam)
	{
		LPARAM_UNUSED();
		INT_PTR rc = TRUE;

		switch (wParam) {
		case SYSCMD_LAUNCH:
			startTask();
			break;

		default:
			rc = FALSE;
		}

		return rc;
	}


	void ConnectDialog::outputInfoMessage(char* pText)
	{
		if (pText) {
			writeInfo(tools::str2wstr(std::string(pText)));
			free(pText);
		}
	}


	void ConnectDialog::showAboutDialog()
	{
		AboutDialog aboutDialog(instance_handle(), window_handle());
		aboutDialog.show_modal();
	}


	void ConnectDialog::showOptionsDialog()
	{
		OptionsDialog optionsDialog{ instance_handle(), window_handle() };

		// Options are not modifiable in the GUI when specified on the command line.
		optionsDialog.full_screen = _params.full_screen() || _settings.get_full_screen();
		optionsDialog.full_screen_updatable = !_params.full_screen();

		optionsDialog.screen_size_updatable = _params.screen_size().height == 0 && _params.screen_size().width == 0;
		if (optionsDialog.screen_size_updatable)
			optionsDialog.screen_size = _settings.get_screen_size();
		else
			optionsDialog.screen_size = _params.screen_size();

		optionsDialog.clear_rdp_username = _params.clear_rdp_username() || _settings.get_clear_rdp_username();
		optionsDialog.clear_rdp_username_updatable = !_params.clear_rdp_username();

		optionsDialog.span_mode = _params.span_mode() || _settings.get_span_mode();
		optionsDialog.span_mode_updatable = !_params.span_mode();

		optionsDialog.multimon_mode = _params.multimon_mode() || _settings.get_multimon_mode();
		optionsDialog.multimon_mode_updatable = !_params.multimon_mode();

		optionsDialog.admin_console = _params.admin_console() || _settings.get_admin_console();
		optionsDialog.admin_console_updatable = !_params.admin_console();

		optionsDialog.rdpfile_mode = (_params.rdp_filename().length() > 0) || _settings.get_rdpfile_mode();
		optionsDialog.rdpfile_updatable = (_params.rdp_filename().length() == 0);
		if (_params.rdp_filename().length() > 0) {
			optionsDialog.rdp_filename = _params.rdp_filename();
		}
		else {
			optionsDialog.rdp_filename = _settings.get_rdp_filename();
		}

		optionsDialog.auth_method = _settings.get_auth_method();


		if (optionsDialog.show_modal()) {
			if (optionsDialog.full_screen_updatable)
				_settings.set_full_screen(optionsDialog.full_screen);
			if (optionsDialog.screen_size_updatable)
				_settings.set_screen_size(optionsDialog.screen_size);
			if (optionsDialog.clear_rdp_username_updatable)
				_settings.set_clear_username(optionsDialog.clear_rdp_username);
			if (optionsDialog.span_mode_updatable)
				_settings.set_span_mode(optionsDialog.span_mode);
			if (optionsDialog.multimon_mode_updatable)
				_settings.set_multimon_mode(optionsDialog.multimon_mode);
			if (optionsDialog.admin_console_updatable)
				_settings.set_admin_console(optionsDialog.admin_console);
			if (optionsDialog.rdpfile_updatable) {
				_settings.set_rdpfile_mode(optionsDialog.rdpfile_mode);
				_settings.set_rdp_filename(optionsDialog.rdp_filename);
			}
			_settings.set_auth_method(optionsDialog.auth_method);
		}
	}


	void ConnectDialog::showErrorMessageDialog(const wchar_t* pText)
	{
		show_message_box(pText, MB_ICONERROR | MB_OK);
		return;
	}


	void ConnectDialog::showInvalidCertificateDialog(const char* pText)
	{
		const int rc = show_message_box(tools::str2wstr(pText), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
		ReplyMessage(rc == IDYES);

		return;
	}


	void ConnectDialog::disconnectFromFirewall(bool success)
	{
		UNREFERENCED_PARAMETER(success);

		disconnect();
	}


	void ConnectDialog::startTask()
	{
		// Starts the local task if a path has been specified. If a single client
		// is allowed to go through the tunnel, the application closes the tunnel
		// as soon as the local application is stopped. The AsyncController is
		// responsible to detect the end of the local application and post a message
		// to this window.
		if (!_task_info->path().empty()) {
			_controller->start_task(*_task_info, !_params.multi_clients());
		}
	}


	void ConnectDialog::clearRdpHistory()
	{
		if (_params.is_mstsc() && (_params.clear_rdp_username() || _settings.get_clear_rdp_username())) {
			// see http://woshub.com/how-to-clear-rdp-connections-history/

			net::Endpoint endpoint{ "127.0.0.1", 0 };
			if (_controller && _controller->tunnel()) {
				endpoint = _controller->tunnel()->local_endpoint();
			}

			// Clear saved user name.
			std::string key = "Software\\Microsoft\\Terminal Server Client\\Servers\\" + endpoint.hostname();
			tools::RegKey rdp_server{ HKEY_CURRENT_USER, tools::str2wstr(key) };

			try {
				rdp_server.del_value(L"UsernameHint");
			}
			catch (std::system_error& err) {
				_logger->debug("ERROR: ClearRdpHistory %s", err.what());
			}

			tools::RegKey rdp_default{
				HKEY_CURRENT_USER,
				L"Software\\Microsoft\\Terminal Server Client\\Default"
			};

			// Remove entry in the MRU list.
			const std::wstring mru_entry = tools::str2wstr(endpoint.to_string());

			for (int i = 0; i < 10; i++) {
				try {
					wchar_t value_name[5]{ 0 };
					wsprintf(value_name, L"MRU%d", i);

					std::wstring value = rdp_default.get_string(value_name, L"");
					if (value.compare(mru_entry) == 0) {
						rdp_default.del_value(value_name);
					}
				}
				catch (std::system_error& err) {
					_logger->debug("ERROR: ClearRdpHistory %s", err.what());
				}
			}
		}
	}


	void ConnectDialog::onConnectedEvent(bool success)
	{
		if (!success) {
			disconnect();
		}
		else {
			// Save in the registry last valid address if not specified
			// on the command line.
			if (_params.firewall_address().empty())
				_settings.set_firewall_address(getFirewallAddress());
			if (_params.host_address().empty())
				_settings.set_host_address(getHostAddress());

			_logger->info(">> successfully logged in portal %s", _firewall_endpoint.to_string().c_str());

			// create the tunnel.
			_controller->create_tunnel(
				_host_endpoint,
				_params.local_port(),
				_params.multi_clients(),
				_params.tcp_nodelay());

			// Start network activity tracking.
			_previous_counters = 0;
			_activity_loop = 0;

			// Enable timers (in ms).
			::SetTimer(window_handle(), TIMER_COUNTERS, 500, nullptr);
			::SetTimer(window_handle(), TIMER_ACTIVITY, 250, nullptr);

			// Show activity controls.
			set_control_text(IDC_BYTES_SENT, L"");
			set_control_visible(IDC_BYTES_SENT, true);
			set_control_visible(IDC_ACTIVITY, true);
		}
	}


	void ConnectDialog::onDisconnectedEvent(bool success)
	{
		if (!_task_info->path().empty()) {
			clearRdpHistory();
		}

		// Hide activity controls.
		set_control_text(IDC_BYTES_SENT, L"");
		set_control_visible(IDC_BYTES_SENT, false);
		set_control_visible(IDC_ACTIVITY, false);

		// Disable timers.
		::KillTimer(window_handle(), TIMER_COUNTERS);
		::KillTimer(window_handle(), TIMER_ACTIVITY);

		if (success)
			writeInfo(L">> disconnected");

		// Reset controls status.
		set_control_enable(IDC_ADDR_FW, true);
		set_control_enable(IDC_ADDR_HOST, true);
		set_control_enable(IDC_CONNECT, true);
		set_control_enable(IDC_DISCONNECT, true);
		set_control_enable(IDC_QUIT, true);
		set_control_visible(IDC_CONNECT, true);
		set_control_visible(IDC_DISCONNECT, false);
		if (_params.is_mstsc())
			::EnableMenuItem(get_sys_menu(false), SYSCMD_OPTIONS, MF_BYCOMMAND | MF_ENABLED);

		// Restore window visibility if currently minimized
		if (is_minimized())
			show_window(SW_RESTORE);
	}


	void ConnectDialog::onTunnelListeningEvent(bool success)
	{
		set_control_enable(IDC_DISCONNECT, true);

		if (!success) {
			disconnect();

		}
		else {
			startTask();
		}
	}


	INT_PTR ConnectDialog::onUserEventMessage(UINT eventNumber, void* param)
	{
		INT_PTR rc = TRUE;

		if (AsyncMessage::OutputInfoMessageRequest == eventNumber) {
			outputInfoMessage(reinterpret_cast<char*>(param));

		}
		else if (AsyncMessage::ShowCredentialsDialogRequest == eventNumber) {
			showCredentialsDialog(reinterpret_cast<fw::AuthCredentials*>(param));

		}
		else if (AsyncMessage::ShowPinCodeDialogRequest == eventNumber) {
			showPinCodeDialog(reinterpret_cast<fw::AuthCode*>(param));

		}
		else if (AsyncMessage::ShowSamlAuthDialogRequest == eventNumber) {
			showSamlDialog(reinterpret_cast<fw::AuthSamlInfo*>(param));

		}
		else if (AsyncMessage::ShowInvalidCertificateDialogRequest == eventNumber) {
			showInvalidCertificateDialog(reinterpret_cast<char*>(param));

		}
		else if (AsyncMessage::ShowErrorMessageDialogRequest == eventNumber) {
			showErrorMessageDialog(reinterpret_cast<wchar_t*>(param));

		}
		else if (AsyncMessage::DisconnectFromFirewallRequest == eventNumber) {
			disconnectFromFirewall(param != 0);

		}
		else if (AsyncMessage::ConnectedEvent == eventNumber) {
			onConnectedEvent(param != 0);
		}
		else if (AsyncMessage::DisconnectedEvent == eventNumber) {
			onDisconnectedEvent(param != 0);

		}
		else if (AsyncMessage::TunnelListeningEvent == eventNumber) {
			onTunnelListeningEvent(param != 0);

		}
		else {
			rc = FALSE;
		}

		return rc;
	}

}
