/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "ui/ConnectDialog.h"
#include "ui/CredentialDialog.h"
#include "ui/AskCodeDialog.h"
#include "ui/AboutDialog.h"
#include "ui/OptionsDialog.h"
#include "ui/AsyncMessage.h"
#include "tools/StrUtil.h"
#include "tools/SysUtil.h"
#include "resources/resource.h"

// ID for the system menu
static const int SYSCMD_ABOUT = 1;
static const int SYSCMD_LAUNCH = 2;
static const int SYSCMD_OPTIONS = 3;

ConnectDialog::ConnectDialog(HINSTANCE hInstance, const CmdlineParams& params):
	ModelessDialog(hInstance, NULL, IDD_CONNECT_DIALOG),
	_params(params),
	_logger(tools::Logger::get_logger())
{
	// Assign application icons. Icons are automatically deleted when the 
	// application stops thanks to the LR_SHARED parameter.
	HICON hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_FORTIRDP), IMAGE_ICON, 16, 16, LR_SHARED);
	SendMessage(window_handle(), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_FORTIRDP), IMAGE_ICON, 128, 128, LR_SHARED);
	SendMessage(window_handle(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	// Configure the status text font. The color is assigned later when responding to WM_CTLCOLORSTATIC
	// message.
	_msg_font = create_font(10, L"Arial");
	set_control_font(IDC_STATUSTEXT, _msg_font);

	// create a black brush used to fill the IDC_STATUSTEXT control
	_bg_brush = CreateSolidBrush(RGB(0, 0, 0));

	_anim_font = create_font(10, L"Consolas");
	if (!_anim_font)
		_anim_font = create_font(10, L"Arial");
	set_control_font(IDC_ACTIVITY, _anim_font);

	// Add menu : about, options and launch submenu (and associated hot keys)
	HMENU hMenu = get_sys_menu(false);
	::AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
	if (params.is_mstsc())
		::AppendMenu(hMenu, MF_STRING, SYSCMD_OPTIONS, L"&Options...");

	if (params.multi_clients() && params.appname().length() > 0) {
		::AppendMenu(hMenu, MF_STRING, SYSCMD_LAUNCH, L"&Launch...\tCtlr+L");
		::RegisterHotKey(window_handle(), SYSCMD_LAUNCH, MOD_CONTROL | MOD_NOREPEAT, 0x4C);
	}

	::AppendMenu(hMenu, MF_STRING, SYSCMD_ABOUT, L"&About...");


	// link a new log writer to the logger. This log writer sends OutputInfoMessage
	// to this dialog window.
	_writer = new InfoLogWriter(window_handle());
	_logger->add_writer(_writer);


	// configure the maximum length for address text fields
	set_control_textlen(IDC_ADDR_FW, MAX_ADDR_LENGTH);
	set_control_textlen(IDC_ADDR_HOST, MAX_ADDR_LENGTH);

	// initialize addresses. Take values from command line and if
	// not specified, use the last values saved in the registry
	setFirewallAddress(!params.firewall_address().empty() 
			? params.firewall_address() 
			: _settings.get_firewall_address());
	setHostAddress(!params.host_address().empty()
		? params.host_address()
		: _settings.get_host_address());

	// initialize the user name
	if (!params.username().empty()) {
		_username = _params.username();
	}
	else {
		// get user name from last usage and if not available, we use the windows user name
		_username = _settings.get_username(tools::get_windows_username());
	}

	// allocate the communication controller
	_controller = std::make_unique<AsyncController>(window_handle(), _params.ca_filename());

	if (params.firewall_address().length() > 0 && params.host_address().length() > 0) {
		// auto connect if both addresses are specified
		PostMessage(window_handle(), WM_COMMAND, IDC_CONNECT, 0);
	}
}


ConnectDialog::~ConnectDialog()
{
	// delete all objects we created
	DeleteObject(_bg_brush);
	DeleteObject(_msg_font);
	DeleteObject(_anim_font);

	_controller->terminate();
	_controller->wait(1000);

	_logger->remove_writer(_writer);
	delete _writer;
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

	// add the new message and remove old one, we keep only the last 10 messages
	_msg_buffer.push_back(message);
	while (_msg_buffer.size() > 10) {
		_msg_buffer.erase(_msg_buffer.cbegin());
	}

	// render the messages buffer
	std::wstring output;
	for (auto const& str : _msg_buffer) {
		output.append(str);
		output.append(L"\n");
	}

	set_control_text(IDC_STATUSTEXT, output);
}


void ConnectDialog::connect()
{
	// Check if fw and host addresses are valid
	try {
		std::string fw_addr{ tools::trim(tools::wstr2str(getFirewallAddress())) };
		_firewall_endpoint = net::Endpoint(fw_addr, DEFAULT_FW_PORT);

	} catch (const std::invalid_argument) {
		set_focus(IDC_ADDR_FW);
		showErrorMessageDialog("Invalid firewall address");
		return;
	}

	try {
		std::string host_addr{ tools::trim(tools::wstr2str(getHostAddress())) };
		_host_endpoint = net::Endpoint(host_addr, DEFAULT_RDP_PORT);

	} catch (const std::invalid_argument) {
		set_focus(IDC_ADDR_HOST);
		showErrorMessageDialog("Invalid host address");
		return;
	}

	// Prepare the task exe name and parameters
	std::vector<std::wstring> task_params;
	std::wstring task_name;

	if (_params.is_mstsc()) {
		task_name = L"C:\\Windows\\system32\\mstsc.exe";

		if (!_params.rdp_filename().empty()) {
			task_params.push_back(_params.rdp_filename());
		} else if (_settings.get_rdpfile_mode() && !_settings.get_rdp_filename().empty()) {
			task_params.push_back(_settings.get_rdp_filename());
		}

		task_params.push_back(L"/v:${host}:${port}");
		
		if (_params.admin_console() || _settings.get_admin_console())
			task_params.push_back(L"/admin");

		if (_params.full_screen() || _settings.get_full_screen())
			task_params.push_back(L"/f");

		if (_params.span_mode() || _settings.get_span_mode())
			task_params.push_back(L"/span");

		if (_params.multimon_mode() || _settings.get_multimon_mode())
			task_params.push_back(L"/multimon");

	} else {
		std::vector<std::wstring> task_info;
		tools::split(_params.appname(), L';', task_info);

		if (task_info.size() > 0)
			task_name = tools::trim(task_info[0]);
		for (int i = 1; i < task_info.size(); i++)
			task_params.push_back(task_info[i]);
	}
	_task_info = std::make_unique<tools::TaskInfo>(task_name, task_params);

	// Check if the app executable exists
	if (!task_name.empty() && !tools::file_exists(task_name)) {
		std::string message{ "Application not found : " + tools::wstr2str(task_name) };
		showErrorMessageDialog(message.c_str());
		return;
	}

	// Check if the rdp file exists
	if (!_params.rdp_filename().empty() && !tools::file_exists(_params.rdp_filename())) {
		std::string message{ "RDP file not found : " + tools::wstr2str(_task_info->path()) };
		showErrorMessageDialog(message.c_str());
		return;
	}

	// Clear the information log
	clearInfo();

	// Disable all controls in this dialog and show the disconnect button
	set_control_enable(IDC_CONNECT, false);
	set_control_enable(IDC_DISCONNECT, false);
	set_control_enable(IDC_QUIT, false);
	set_control_enable(IDC_ADDR_FW, false);
	set_control_enable(IDC_ADDR_HOST, false);
	set_control_visible(IDC_CONNECT, false);
	set_control_visible(IDC_DISCONNECT, true);

	// start the client
	_controller->connect(_firewall_endpoint);
}


void ConnectDialog::disconnect()
{
	set_control_enable(IDC_DISCONNECT, false);
	_controller->disconnect();
}


void ConnectDialog::showAskCredentialDialog(fw::Credential* pCredential)
{
	bool modal_result;

	CredentialDialog credentialDialog(instance_handle(), window_handle());
	std::string message{ "Enter user name and password to access firewall " + _controller->portal()->host().hostname() };
	credentialDialog.setText(tools::str2wstr(message));
	credentialDialog.setUsername(_username);

	modal_result = credentialDialog.showModal() == TRUE;
	if (modal_result && pCredential) {
		// returns user name and password to caller
		_username = credentialDialog.getUsername();
		pCredential->username = _username;
		pCredential->password = credentialDialog.getPassword();

		// save user name in last usage registry but only if not specified on the command line
		if (_params.username().empty())
			_settings.set_username(_username);
	}

	::ReplyMessage(modal_result);
}


void ConnectDialog::showAskCodeDialog(fw::Code2FA* pCode)
{
	AskCodeDialog codeDialog{ instance_handle(), window_handle() };
	std::string message = (!pCode)
		? "Enter code to access firewall " + _controller->portal()->host().hostname()
		: pCode->info;
	codeDialog.setText(tools::str2wstr(message));

	bool modal_result = codeDialog.showModal() == TRUE;
	if (modal_result && pCode) {
		// returns code to caller
		pCode->code = tools::wstr2str(codeDialog.getCode());
	}

	::ReplyMessage(modal_result);
}


INT_PTR ConnectDialog::onDestroyDialogMessage(WPARAM wParam, LPARAM lParam)
{
	// Stop AsyncConnect if still running
	disconnect();

	// close this application
	::PostQuitMessage(0);

	return FALSE;
}


INT_PTR ConnectDialog::onCloseDialogMessage(WPARAM wParam, LPARAM lParam)
{
	return ::DestroyWindow(window_handle());
}


INT_PTR ConnectDialog::onButtonClick(int cid, LPARAM lParam)
{
	INT_PTR rc = FALSE;

	switch (cid) {
	case IDC_QUIT:
		::DestroyWindow(window_handle());
		break;

	case IDC_CONNECT:
		connect();
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
	if (lParam == (WPARAM)GetDlgItem(window_handle(), IDC_STATUSTEXT)) {
		HDC static_text = (HDC)wParam;
		::SetTextColor(static_text, RGB(0, 255, 255));
		::SetBkColor(static_text, RGB(0, 0, 0));

		return (INT_PTR)_bg_brush;

	} else {
		return 0;
	}
}



INT_PTR ConnectDialog::onTimerMessage(WPARAM wParam, LPARAM lParam)
{
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
				// show network activity
				const wchar_t activity_symbol[] = L"\u2190\u2191\u2192\u2193";
				_previous_counters = counters.total();
				_activity_loop = (_activity_loop + 1) % 4;
				_last_activity = now;

				const std::wstring symbol(1, activity_symbol[_activity_loop]);
				set_control_text(IDC_ACTIVITY, symbol);

			} else {
				// Clear display if no activity during a 1/4 second
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


void ConnectDialog::outputInfoMessage(const char* pText)
{
	if (pText) {
		writeInfo(tools::str2wstr(std::string(pText)));
		free((void *)pText);
	}
}


void ConnectDialog::showAboutDialog()
{
	AboutDialog aboutDialog(instance_handle(), window_handle());
	aboutDialog.showModal();
}


void ConnectDialog::showOptionsDialog()
{
	OptionsDialog optionsDialog{ instance_handle(), window_handle() };

	// options are not updatable in the gui when specified on the command line
	optionsDialog.full_screen = _params.full_screen() || _settings.get_full_screen();
	optionsDialog.full_screen_updatable = !_params.full_screen();

	optionsDialog.clear_rdp_username = _params.clear_rdp_username() || _settings.get_clear_rdp_username();
	optionsDialog.clear_rdp_username_updatable = ! _params.clear_rdp_username();

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

	if (optionsDialog.showModal()) {
		if (optionsDialog.full_screen_updatable)
			_settings.set_full_screen(optionsDialog.full_screen);
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
	}
}


void ConnectDialog::showErrorMessageDialog(const char * pText)
{
	::MessageBox(
		window_handle(),
		tools::str2wstr(pText).c_str(),
		get_title().c_str(),
		MB_ICONERROR | MB_OK);

	return;
}


void ConnectDialog::showInvalidCertificateDialog(const char* pText)
{
	const int rc = ::MessageBox(
		window_handle(),
		tools::str2wstr(pText).c_str(),
		get_title().c_str(),
		MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);

	ReplyMessage(rc == IDYES);

	return;
}


void ConnectDialog::disconnectFromFirewall(bool success)
{
	disconnect();
}



void ConnectDialog::startTask()
{
	// Starts the local task if a path has been specified. If a single client
	// is allowed to go through the tunnel, the application closes the tunnel
	// as soon as the local application is stopped. The AsyncController is
	// responsible to detect the end of the local application and post a message
	// to this window
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

		// Clear saved username
		std::string key = "Software\\Microsoft\\Terminal Server Client\\Servers\\" + endpoint.hostname();
		tools::RegKey rdp_server{ HKEY_CURRENT_USER, tools::str2wstr(key) };

		try {
			rdp_server.del_value(L"UsernameHint");
		}
		catch (std::system_error& err) {
			_logger->debug("ERROR : %s", err.what());
		}

		tools::RegKey rdp_default{
			HKEY_CURRENT_USER,
			L"Software\\Microsoft\\Terminal Server Client\\Default"
		};

		for (int i = 0; i < 10; i++) {
			try {
				std::wstring value = rdp_default.get_string(L"xx");
				if (value.compare(L"xx")) {
					rdp_default.del_value(L"");
				}
			}
			catch (std::system_error& err) {
				_logger->debug("ERROR : %s", err.what());
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
		// Save in the registry last valid address if not specified on the command line
		if (_params.firewall_address().empty())
			_settings.set_firewall_address(getFirewallAddress());
		if (_params.host_address().empty())
			_settings.set_host_address(getHostAddress());

		_logger->info(">> successfully logged in portal %s", _firewall_endpoint.to_string().c_str());

		// create the tunnel
		_controller->create_tunnel(
			_host_endpoint, 
			_params.local_port(), 
			_params.multi_clients(),
			_params.tcp_nodelay());

		// start network activity tracking
		_previous_counters = 0;
		_activity_loop = 0;

		// enable timers (in ms)
		::SetTimer(window_handle(), TIMER_COUNTERS, 500, nullptr);
		::SetTimer(window_handle(), TIMER_ACTIVITY, 250, nullptr);

		// show activity controls
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

	// Hide activity controls
	set_control_text(IDC_BYTES_SENT, L"");
	set_control_visible(IDC_BYTES_SENT, false);
	set_control_visible(IDC_ACTIVITY, false);

	// disable timers
	::KillTimer(window_handle(), TIMER_COUNTERS);
	::KillTimer(window_handle(), TIMER_ACTIVITY);
	
	if (success)
		writeInfo(L">> disconnected");

	// Enable all controls
	set_control_enable(IDC_ADDR_FW, true);
	set_control_enable(IDC_ADDR_HOST, true);
	set_control_enable(IDC_CONNECT, true);
	set_control_enable(IDC_DISCONNECT, true);
	set_control_enable(IDC_QUIT, true);
	set_control_visible(IDC_CONNECT, true);
	set_control_visible(IDC_DISCONNECT, false);	

	// Restore window visibility if currently minimized
	if (is_minimized())
		show_window(SW_RESTORE);
}


void ConnectDialog::onTunnelListeningEvent(bool success)
{
	set_control_enable(IDC_DISCONNECT, true);

	if (!success) {
		disconnect();

	} else {
		startTask();
	}
}


INT_PTR ConnectDialog::onUserEventMessage(UINT eventNumber, void* param)
{
	INT_PTR rc = TRUE;

	if (AsyncMessage::OutputInfoMessageRequest == eventNumber) {
		outputInfoMessage(static_cast<const char*>(param));

	}
	else if (AsyncMessage::ShowAskCredentialDialogRequest == eventNumber) {
		showAskCredentialDialog(static_cast<fw::Credential *>(param));

	}
	else if (AsyncMessage::ShowAskCodeDialogRequest == eventNumber) {
		showAskCodeDialog(static_cast<fw::Code2FA *>(param));

	}
	else if (AsyncMessage::ShowInvalidCertificateDialogRequest == eventNumber) {
		showInvalidCertificateDialog(static_cast<char *>(param));

	}
	else if (AsyncMessage::ShowErrorMessageDialogRequest == eventNumber) {
		showErrorMessageDialog(static_cast<char *>(param));

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