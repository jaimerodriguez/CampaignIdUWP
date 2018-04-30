#pragma once


#include <hstring.h>
namespace WRLSample
{
	public ref class CampaignIdHelper sealed
	{
	public:
		CampaignIdHelper();

		Windows::Foundation::IAsyncOperation<Platform::String^>^ GetCampaignId81Async();

		property int  NetworkRequestTimeOutMills; 

	private:
 
		HRESULT GetCampaignIdFromStoreContextWithWait(HSTRING* pcampaignId );
		HRESULT GetCampaignIdFromCurrentAppWithWait  (HSTRING* pcampaignId);
		HRESULT GetCampaignIdFromStoreLicenseWithWait(HSTRING* pcampaignId);
	}; 
} 
