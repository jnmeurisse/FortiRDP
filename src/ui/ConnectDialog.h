/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <list>
#include <string>
#include <memory>
#include <chrono>
#include "fw/AuthTypes.h"
#include "net/Endpoint.h"
#include "tools/Mutex.h"
#include "tools/Logger.h"
#include "tools/TaskInfo.h"
#include "ui/CmdlineParams.h"
#include "ui/RegistrySettings.h"
#include "ui/ModelessDialog.h"
#include "ui/AsyncController.h"
#include "ui/InfoLogWriter.h"


namespace ui {
	namespace chrono = std::chrono;

	#define MAX_ADDR_LENGTH 128
	#define MAX_TITLE_LENGTH 128

	class ConnectDialog final : public ModelessDialog
	{
	public:
		explicit ConnectDialog(HINSTANCE hInstance, const CmdlineParams& cmdlineParams);
		virtual ~ConnectDialog() override;

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

		// Command line parameters.
		const CmdlineParams _params;

		// Registry parameters.
		RegistrySettings _settings;

		// The application logger.
		tools::Logger* const _logger;

		// A writer that appends data in the InfoLog text box
		InfoLogWriter* _writer;

		// Connection parameters :
		// - Firewall endpoint
		const uint16_t DEFAULT_FW_PORT = 10443;
		net::Endpoint _firewall_endpoint;

		// - Firewall domain
		std::string _firewall_domain;

		// - Host endpoint
		const uint16_t DEFAULT_RDP_PORT = 3389;
		net::Endpoint _host_endpoint;

		// - User name
		std::wstring _username;

		// - Task to launch and parameters
		std::unique_ptr<tools::TaskInfo> _task_info;

		// - Running controller
		std::unique_ptr<AsyncController> _controller;

		// - Activity animation
		HFONT _anim_font;
		int _activity_loop = 0;
		size_t _previous_counters;
		chrono::steady_clock::time_point _last_activity;

		// - Output window
		HBRUSH _bg_brush;
		HFONT _msg_font;
		std::list<std::wstring> _msg_buffer;
		tools::Mutex _msg_mutex;

		void connect(bool clear_log);
		void disconnect();
		void startTask();
		void clearRdpHistory();

		// Windows Event message handlers
		virtual INT_PTR onDestroyDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onCloseDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onButtonClick(int control_id, LPARAM lParam) override;
		virtual INT_PTR onSysCommandMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onCtlColorStaticMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onTimerMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onHotKey(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onUserEventMessage(UINT eventNumber, void* param) override;

		// Async message request handlers
		void outputInfoMessage(const char* pText);
		void showAboutDialog();
		void showOptionsDialog();
		void showErrorMessageDialog(const wchar_t* pText);
		void showCredentialsDialog(fw::AuthCredentials* pCredential);
		void showSamlDialog(fw::AuthSamlInfo* pSamlInfo);
		void showPinCodeDialog(fw::AuthCode* pCode);
		void showInvalidCertificateDialog(const char* pText);
		void disconnectFromFirewall(bool success);

		// Async message event handlers
		void onConnectedEvent(bool success);
		void onDisconnectedEvent(bool success);
		void onTunnelListeningEvent(bool success);
	};

}
