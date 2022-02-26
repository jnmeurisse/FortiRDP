/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <Windows.h>
#include <string>
#include <memory>
#include <chrono>
#include "fw/PortalClient.h"
#include "tools/Counters.h"
#include "tools/Mutex.h"
#include "tools/Logger.h"
#include "tools/TaskInfo.h"
#include "tools/Task.h"
#include "ui/CmdlineParams.h"
#include "ui/RegistrySettings.h"
#include "ui/ModelessDialog.h"
#include "ui/AsyncController.h"
#include "ui/InfoLogWriter.h"

namespace chrono = std::chrono;

#define MAX_ADDR_LENGTH 128
#define MAX_TITLE_LENGTH 128

class ConnectDialog final : public ModelessDialog
{
public:
	explicit ConnectDialog(HINSTANCE hInstance, const CmdlineParams& cmdlineParams);
	virtual ~ConnectDialog();

	std::wstring getFirewallAddress() const;
	void setFirewallAddress(const std::wstring& addr);

	std::wstring getHostAddress() const;
	void setHostAddress(const std::wstring& addr);

	void clearInfo();
	void writeInfo(const std::wstring& message);


private:
	enum TimersId
	{
		TIMER_COUNTERS = 2,
		TIMER_ACTIVITY = 3
	};

	// command line parameters
	const CmdlineParams _params;

	// registry parameters
	RegistrySettings _settings;

	// the application logger
	tools::Logger* const _logger;

	// a writer to append data in the InfoLog text box
	InfoLogWriter* _writer;

	// Connection parameters
	// - firewall endpoint
	const int DEFAULT_FW_PORT = 10443;
	net::Endpoint _firewall_endpoint;

	// - host endpoint
	const int DEFAULT_RDP_PORT = 3389;
	net::Endpoint _host_endpoint;

	// - username
	std::wstring _username;

	// - task to launch and parameters
	std::unique_ptr<tools::TaskInfo> _task_info;

	// - Running controller
	std::unique_ptr<AsyncController> _controller;

	// brush to fill the debug window
	HBRUSH _bg_brush;

	// used to show the activity animation
	int _activity_loop = 0;
	net::Counters _previous_counters;
	chrono::steady_clock::time_point _last_activity;

	// 
	HFONT _msg_font;
	std::list<std::wstring> _msg_buffer;
	tools::Mutex _msg_mutex;

	void connect();
	void disconnect();
	void startTask();

	// Event handlers
	virtual INT_PTR onDestroyDialogMessage(WPARAM wParam, LPARAM lParam) override;
	virtual INT_PTR onCloseDialogMessage(WPARAM wParam, LPARAM lParam) override;
	virtual INT_PTR onButtonClick(int cid, LPARAM lParam) override;
	virtual INT_PTR onSysCommandMessage(WPARAM wParam, LPARAM lParam) override;
	virtual INT_PTR onCtlColorStaticMessage(WPARAM wParam, LPARAM lParam) override;
	virtual INT_PTR onTimerMessage(WPARAM wParam, LPARAM lParam) override;
	virtual INT_PTR onHotKey(WPARAM wParam, LPARAM lParam) override;
	virtual INT_PTR onUserEventMessage(UINT eventNumber, void* param) override;

	// Async message request handlers
	void outputInfoMessage(const char* pText);
	void showAboutDialog();
	void showOptionsDialog();
	void showErrorMessageDialog(const char* pText);
	void showAskCredentialDialog(fw::Credential* pCredential);
	void showAskCodeDialog(fw::Code2FA* pCode);
	void showInvalidCertificateDialog(const char* pText);
	void disconnectFromFirewall(bool success);

	// Async message event handlers
	void onConnectedEvent(bool success);
	void onDisconnectedEvent(bool success);
	void onTunnelListeningEvent(bool success);
};

