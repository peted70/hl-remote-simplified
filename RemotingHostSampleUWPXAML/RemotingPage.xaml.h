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
// RemotingPage.xaml.h
// Declaration of the RemotingPage class.
//

#pragma once

#include "RemotingPage.g.h"

#include "Common\DeviceResourcesWindowed.h"
#include "AppMain.h"
#include "AppView.h"
#include <HolographicStreamerHelpers.h>
#include "HolographicRemoteConnection.h"

namespace RemotingHostSample
{
    /// <summary>
    /// A page that hosts a DirectX SwapChainPanel.
    /// </summary>
    public ref class RemotingPage sealed
    {
    public:
        RemotingPage();
        virtual ~RemotingPage();

        void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
        void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

    private:
		void OnInit(HolographicSpace ^ space, RemoteSpeech ^ speech);
		void OnConnected();
		void OnDisconnected(HolographicStreamerConnectionFailureReason reason);
		void OnPreviewFrame(const ComPtr<ID3D11Texture2D>& texture);

        // Window event handlers.
        void Key_Down(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
 
		void Tick();
        void StartRenderLoop();
        void StopRenderLoop();

        // Resources used to render the remoting frame preview in the XAML page background.
        Platform::String^                                   m_ipAddress;
        Microsoft::Holographic::HolographicStreamerHelpers^ m_streamerHelpers;

        int  m_width                = 0;
        int  m_height               = 0;
        bool m_connectedState       = false;
        bool m_showPreview          = true;
        bool m_priorPreviewState    = true;
        bool m_windowClosed         = false;
        bool m_windowVisible        = true;

        // The holographic space the app will use for rendering.
		Windows::Graphics::Holographic::HolographicSpace^   m_holographicSpace = nullptr;
		std::shared_ptr<DX::DeviceResources>                m_deviceResources;
		std::unique_ptr<AppMain> m_main;

		std::unique_ptr<HolographicRemoteConnection> m_connector;

        // XAML render loop.
        Windows::Foundation::IAsyncAction^  m_renderLoopWorker;
        Concurrency::critical_section       m_criticalSection;
	
		void TextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void Connect_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Disconnect_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}

