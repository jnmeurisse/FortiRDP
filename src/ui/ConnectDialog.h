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
#include "util/Logger.h"
#include "util/Mutex.h"
#include "util/TaskInfo.h"
#include "ui/CmdlineParams.h"
#include "ui/RegistrySettings.h"
#include "ui/ModelessDialog.h"
#include "ui/AsyncController.h"
#include "ui/InfoLogWriter.h"


namespace ui {
	namespace chrono = std::chrono;

	constexpr int MAX_ADDR_LENGTH = 128;
	constexpr int MAX_TITLE_LENGTH = 128;

	class ConnectDialog final : public ModelessDialog
	{
	public:
		explicit ConnectDialog(HINSTANCE hInstance, const ui::CmdlineParams& cmdlineParams);
		~ConnectDialog() override;

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

		// The class name.
		static const char* __class__;

		// The application logger.
		utl::Logger* const _logger;

		// Command line parameters.
		const ui::CmdlineParams _params;

		// Registry parameters.
		ui::RegistrySettings _settings;

		// A writer that appends data in the InfoLog text box
		ui::InfoLogWriter _writer;

		// Connection parameters :
		// - Firewall endpoint
		const uint16_t DEFAULT_FW_PORT = 10443;
		net::Endpoint _firewall_endpoint;

		// - Firewall sslvpn realm
		std::wstring _realm;

		// - Host endpoint
		const uint16_t DEFAULT_RDP_PORT = 3389;
		net::Endpoint _host_endpoint;

		// - User name
		std::wstring _username;

		// - Task to launch and parameters
		std::unique_ptr<utl::TaskInfo> _task_info;

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
		utl::Mutex _msg_mutex;

		void connect(bool clear_log);
		void disconnect();
		void startTask();
		void clearRdpHistory();

		// Windows Event message handlers
		INT_PTR onDestroyDialogMessage(WPARAM wParam, LPARAM lParam) override;
		INT_PTR onCloseDialogMessage(WPARAM wParam, LPARAM lParam) override;
		INT_PTR onButtonClick(int control_id, LPARAM lParam) override;
		INT_PTR onSysCommandMessage(WPARAM wParam, LPARAM lParam) override;
		INT_PTR onCtlColorStaticMessage(WPARAM wParam, LPARAM lParam) override;
		INT_PTR onTimerMessage(WPARAM wParam, LPARAM lParam) override;
		INT_PTR onHotKey(WPARAM wParam, LPARAM lParam) override;
		INT_PTR onAsyncMessage(UINT eventId, void* param) override;

		// Async message request handlers
		void showAboutDialog();
		void showOptionsDialog();
		void showErrorMessageDialog(const std::wstring& text);
		void showCredentialsDialog(fw::AuthCredentials* pCredential);
		void showSamlDialog(fw::AuthSamlInfo* pSamlInfo);
		void showPinCodeDialog(fw::AuthCode* pCode);
		void showInvalidCertificateDialog(const std::wstring& text);
		void disconnectFromFirewall(bool success);

		// Async message event handlers
		void onConnectedEvent(bool success);
		void onDisconnectedEvent(bool success);
		void onTunnelListeningEvent(bool success);
		void onOutputInfoEvent(utl::LogQueue* pLogQueue);
	};

}
