#pragma once

#include <HolographicStreamerHelpers.h>
#include "./Common/DeviceResources.h"

using namespace Microsoft::Holographic;
using namespace Windows::Graphics::Holographic;
using namespace std;
using namespace Platform;
using namespace Microsoft::WRL;
using namespace DX;

// Helper class to wrap up the details of making a connection to a HoloLens device
// listening at the IP address supplied. Must be used from the main render thread.
// Device Resources should be those used for the main user interface..
class HolographicRemoteConnection
{
public:
	HolographicRemoteConnection(const std::shared_ptr<DeviceResources>& deviceResources);

	void Connect(String^ ipAddress,
		function<void(HolographicSpace^, RemoteSpeech^)> Init,
		function<void()> OnConnected,
		function<void(HolographicStreamerConnectionFailureReason)> OnDisconnected,
		function<void(const ComPtr<ID3D11Texture2D>&)> OnPreviewFrame);

	void Disconnect();

private:
	void ConnectHandlers(String^ ipAddress,
		HolographicStreamerHelpers ^ helper,
		function<void()> OnConnected, 
		function<void(HolographicStreamerConnectionFailureReason)> OnDisconnected,
		function<void(const ComPtr<ID3D11Texture2D>&)> OnPreviewFrame);

	HolographicStreamerHelpers^ m_streamerHelpers;
	shared_ptr<DeviceResources> m_deviceResources;
};

