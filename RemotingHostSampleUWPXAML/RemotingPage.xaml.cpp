//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

//
// RemotingPage.xaml.cpp
// Implementation of the RemotingPage class.
//

#include "pch.h"
#include "Common\DbgLog.h"
#include <ppltasks.h>

#include "RemotingPage.xaml.h"

#define STREAMER_WIDTH          1280
#define STREAMER_HEIGHT         720

#define INITIAL_WINDOW_WIDTH    STREAMER_WIDTH
#define INITIAL_WINDOW_HEIGHT   STREAMER_HEIGHT

static void ConsoleLog(Windows::UI::Xaml::Controls::TextBlock^ Console, _In_z_ LPCWSTR format, ...);

using namespace RemotingHostSample;

using namespace concurrency;
using namespace Microsoft::Holographic;
using namespace Windows::Graphics::Holographic;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

RemotingPage::RemotingPage()
{
    InitializeComponent();

	m_deviceResources = std::make_shared<DX::DeviceResources>();
	m_connector = std::make_unique<HolographicRemoteConnection>(m_deviceResources);

	m_main = std::make_unique<AppMain>(m_deviceResources);

	Console->Text += "\n";
}

RemotingPage::~RemotingPage()
{
    critical_section::scoped_lock lock(m_criticalSection);

    // Stop rendering and processing events on destruction.
    StopRenderLoop();
}

// Saves the current state of the app for suspend and terminate events.
void RemotingPage::SaveInternalState(IPropertySet^ state)
{
    m_deviceResources->Trim();

    // Stop rendering when the app is suspended.
    StopRenderLoop();
}

// Loads the current state of the app for resume events.
void RemotingPage::LoadInternalState(IPropertySet^ state)
{

}

void RemotingPage::Tick()
{
	if (m_main)
	{
		// When running on Windows Holographic, we can use the holographic rendering system.
		HolographicFrame^ holographicFrame = m_main->Update();

		if (holographicFrame && m_main->Render(holographicFrame))
		{
			// The holographic frame has an API that presents the swap chain for each
			// holographic camera.
			m_deviceResources->Present(holographicFrame);
		}
	}
}

void RemotingPage::Key_Down(Platform::Object ^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs ^ e)
{
    if (e->Key == VirtualKey::Enter)
    {
		Connect_Click(sender, e);
    }
}

//void RemotingPage::Start_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
//{
//    {
//        critical_section::scoped_lock lock(m_criticalSection);
//
//        if (m_connectedState == true)
//        {
//            // Already trying to connect - return "true" to break out of loop.
//            return;
//        }
//        else
//        {
//            m_connectedState = true;
//        }
//    }
//
//    if (!m_ipAddress)
//    {
//        ConsoleLog(Console, L"Error: Please set an IP address.");
//    }
//    else
//    {
//        Start->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
//        ipAddress->IsEnabled = false;
//
//        while (!ConnectToRemoteDevice());
//        StartRenderLoop();
//        
//        Stop->Visibility = Windows::UI::Xaml::Visibility::Visible;
//    }
//}
//
//void RemotingPage::Stop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
//{
//    {
//        critical_section::scoped_lock lock(m_criticalSection);
//        if (m_connectedState == false)
//        {
//            return;
//        }
//        else
//        {
//            m_connectedState = false;
//        }
//    }
//
//    Stop->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
//    
//    DisconnectFromRemoteDevice();
//    StopRenderLoop();
//    
//    Start->Visibility = Windows::UI::Xaml::Visibility::Visible;
//    ipAddress->IsEnabled = true;
//}

void RemotingPage::StartRenderLoop()
{
    // If the animation render loop is already running then do not start another thread.
    if ((m_renderLoopWorker != nullptr) && (m_renderLoopWorker->Status == AsyncStatus::Started))
    {
        return;
    }

    // Create a task that will be run on a background thread.
    auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
    {
        // Calculate the updated frame and render once per vertical blanking interval.
        while (action->Status == AsyncStatus::Started)
        {
            critical_section::scoped_lock lock(m_criticalSection);

            Tick();
        }
    });

    // Run task on a dedicated high priority background thread.
    m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void RemotingPage::StopRenderLoop()
{
    if (m_renderLoopWorker != nullptr)
    {
        m_renderLoopWorker->Cancel();
    }
}

#include <Ws2tcpip.h>

bool validateIpAddress(const wchar_t& ipAddress)
{
	struct sockaddr_in sa;
	int result = InetPton(AF_INET, &ipAddress, &(sa.sin_addr));
	return result != 0;
}

void RemotingPage::TextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (IPAddressText->Text->Length() <= 0 || !validateIpAddress(*IPAddressText->Text->Data()))
	{
		ConnectButton->IsEnabled = false;
		return;
	}

	ConnectButton->IsEnabled = true;
}

void RemotingPage::OnInit(HolographicSpace ^ space, RemoteSpeech ^ speech)
{
	m_holographicSpace = space;
	m_deviceResources->SetHolographicSpace(space);
	m_main->SetHolographicSpace(space);

	if (speech)
	{
		m_main->SetSpeechWrapper(speech);
	}
}

void RemotingPage::OnConnected()
{
	StartRenderLoop();

	this->Dispatcher->RunAsync(
		Windows::UI::Core::CoreDispatcherPriority::Normal,
		ref new Windows::UI::Core::DispatchedHandler([this]()
	{
		StatusText->Text = "Connected";
		ProgressControl->IsActive = false;
		DisconnectButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}));
}

void RemotingPage::OnDisconnected(HolographicStreamerConnectionFailureReason reason)
{
	this->Dispatcher->RunAsync(
		Windows::UI::Core::CoreDispatcherPriority::Normal,
		ref new Windows::UI::Core::DispatchedHandler([this]()
	{
		StatusText->Text = "";
		ProgressControl->IsActive = false;
		DisconnectButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}));
}

void RemotingPage::OnPreviewFrame(const ComPtr<ID3D11Texture2D>& texture)
{
}

void RemotingPage::Connect_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	ProgressControl->IsActive = true;
	m_connector->Connect(IPAddressText->Text,
		[this](HolographicSpace^ space, RemoteSpeech^ speech) { this->OnInit(space, speech); },
		[this]() { this->OnConnected(); },
		[this](HolographicStreamerConnectionFailureReason reason) { this->OnDisconnected(reason); },
		[this](const ComPtr<ID3D11Texture2D>& texture) { this->OnPreviewFrame(texture); });
}

void RemotingPage::Disconnect_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	StatusText->Text = "Disconnecting...";
	ProgressControl->IsActive = true;
	m_connector->Disconnect();

	// Call here as we never get called when we disconnect..
	OnDisconnected(HolographicStreamerConnectionFailureReason::None);
}

static void ConsoleLog(Windows::UI::Xaml::Controls::TextBlock^ Console, _In_z_ LPCWSTR format, ...)
{
    wchar_t buffer[1024];
    LPWSTR bufEnd = nullptr;

    va_list args;
    va_start(args, format);
    HRESULT hr = StringCchVPrintfExW(buffer, _countof(buffer), &bufEnd, nullptr, STRSAFE_FILL_BEHIND_NULL | STRSAFE_FILL_ON_FAILURE, format, args);

    if (SUCCEEDED(hr))
    {
        if (*bufEnd != L'\n')
        {
            StringCchCatW(buffer, _countof(buffer), L"\r\n");
        }

        OutputDebugStringW(buffer);
    }

    va_end(args);

    if (Console!= nullptr)
    {
        auto dispatcher = Console->Dispatcher;
        if (dispatcher != nullptr)
        {
            auto stringToSend = ref new Platform::String(buffer);
            dispatcher->RunAsync(
                Windows::UI::Core::CoreDispatcherPriority::Normal,
                ref new Windows::UI::Core::DispatchedHandler(
                    [Console, stringToSend]()
                    {
                        Console->Text += stringToSend;
                    }));
        }
    }
}