#include "pch.h"
#include "CampaignIdHelper.h"

using namespace WRLSample;
using namespace Platform;

#include <wrl\wrappers\corewrappers.h>
#include <wrl\client.h>
#include <wrl\event.h> 


using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

#include "14393\Windows.ApplicationModel.store.14393.h"
#include "14393\windows.services.store.14393.h"

#define ReturnIfFailedHresult(hr)   { if (FAILED(hr)) return hr; }  

#if DEBUG 
// #define DbgPrint(string) ::OutputDebugString(string );   
#endif 


#include <ppl.h>
#include "ppltasks.h"

#include <stdio.h>
#include <wchar.h>

CampaignIdHelper::CampaignIdHelper()
{
#if DEBUG 
	NetworkRequestTimeOutMills = 500000; 
#else 
	NetworkRequestTimeOutMills = 10000;
#endif 
}

 
Windows::Foundation::IAsyncOperation<String^>^ CampaignIdHelper::GetCampaignId81Async()
{
	return concurrency::create_async([=]() -> String^ {
		String^ resultCampaignId = nullptr ;		
		HRESULT hr = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
		HString campaignId; 
		if (SUCCEEDED(hr))
		{
			hr = GetCampaignIdFromStoreContextWithWait (campaignId.GetAddressOf() );
			if (FAILED(hr) || campaignId == nullptr)
			{ 
				hr = GetCampaignIdFromStoreLicenseWithWait(campaignId.GetAddressOf());
				if (FAILED(hr) || campaignId == nullptr)
				{
					hr = GetCampaignIdFromCurrentAppWithWait(campaignId.GetAddressOf());					 
				} 
			}
		}
		resultCampaignId = ref new String(campaignId.Get());  
		return resultCampaignId; 
	});
}

/// Note: ALl the code below is a little paranoid (Checking for nullptr on interfaces ) because store APIs do return null ptrs when the APIs are called against a sideloaded build.. 


HRESULT CampaignIdHelper::GetCampaignIdFromStoreContextWithWait( HSTRING *presultCampaignId)
{
	 
	ComPtr<ABI::Windows::Services::Store::IStoreContextStatics> storeContextStatics;
	HRESULT hr = RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Services_Store_StoreContext).Get(), __uuidof(storeContextStatics), &storeContextStatics);
	ReturnIfFailedHresult(hr);

	ComPtr<ABI::Windows::Services::Store::IStoreContext> storeContext;
    hr = storeContextStatics->GetDefault(&storeContext);
	 
	if (SUCCEEDED(hr) && storeContext != nullptr)
	{
		Event asyncFinished(CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS));
		ComPtr<IAsyncOperation<ABI::Windows::Services::Store::StoreProductResult*>> asyncOperation;

		hr = storeContext->GetStoreProductForCurrentAppAsync(&asyncOperation);

		if (SUCCEEDED(hr))
		{
			 
			auto callback = Microsoft::WRL::Callback<Implements<RuntimeClassFlags<ClassicCom>, IAsyncOperationCompletedHandler<ABI::Windows::Services::Store::StoreProductResult*>, FtmBase>>(
				[&](IAsyncOperation<ABI::Windows::Services::Store::StoreProductResult*>* operation, AsyncStatus status)
			{
				if (status == AsyncStatus::Completed)
				{
					ComPtr<ABI::Windows::Services::Store::IStoreProductResult> storeProductResult;
					hr = operation->GetResults(&storeProductResult);
					if (SUCCEEDED(hr) && storeProductResult != nullptr )
					{
						ComPtr<ABI::Windows::Services::Store::IStoreProduct> storeProduct;
						hr = storeProductResult->get_Product(&storeProduct);
						// &storeProduct returns null on sideloaded apps ... 
						if (SUCCEEDED(hr) && storeProduct != nullptr )
						{

							ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Services::Store::StoreSku*>* pSKUs;
							hr = storeProduct->get_Skus(&pSKUs);
							if (SUCCEEDED(hr) && pSKUs != nullptr  )
							{
								unsigned int totalSkus = 0;
								hr = pSKUs->get_Size(&totalSkus);								 
								for (unsigned int itemIndex = 0; itemIndex < totalSkus; itemIndex++)
								{									 
									boolean isInUserCollection = false;
									ComPtr<ABI::Windows::Services::Store::IStoreSku> sku;
									pSKUs->GetAt(itemIndex, &sku);
									sku->get_IsInUserCollection(&isInUserCollection);
									if (isInUserCollection)
									{
										ComPtr < ABI::Windows::Services::Store::IStoreCollectionData> collectionData;
										hr = sku->get_CollectionData(&collectionData);
										if (SUCCEEDED(hr) && collectionData != nullptr )
										{ 
											hr = collectionData->get_CampaignId( presultCampaignId );
											if (SUCCEEDED(hr))
											{ 
													break;
											}
										}

									}
								}
								 
							}
						}
					}
				}
				else
				{
					hr = E_FAIL;
					RoOriginateError(hr, HStringReference(L"GetStoreProductForCurrentAppAsync failed").Get());
				} 
 
				SetEvent(asyncFinished.Get());
				return S_OK;

			});
			asyncOperation->put_Completed(callback.Get());
			 
			DWORD waitResult = WaitForSingleObjectEx(asyncFinished.Get(), NetworkRequestTimeOutMills, false);


			if (waitResult != WAIT_OBJECT_0 )
			{
				hr = E_FAIL;  
				RoOriginateError(hr, HStringReference(L"GetStoreProductForCurrentAppAsync timed out").Get());
			}
		}
	}
	return hr;
}


HRESULT CampaignIdHelper::GetCampaignIdFromStoreLicenseWithWait (HSTRING* presultCampaignId )
{  
	ComPtr<ABI::Windows::Services::Store::IStoreContextStatics> storeContextStatics;
	HRESULT hr = RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Services_Store_StoreContext).Get(), __uuidof(storeContextStatics), &storeContextStatics);
	ReturnIfFailedHresult(hr);

	ComPtr<ABI::Windows::Services::Store::IStoreContext> storeContext;
	hr = storeContextStatics->GetDefault(&storeContext);
	if (SUCCEEDED(hr) && storeContext != nullptr ) 
	{
		ComPtr<IAsyncOperation<ABI::Windows::Services::Store::StoreAppLicense*>> asyncOperation;
		hr = storeContext->GetAppLicenseAsync(&asyncOperation);
		if (SUCCEEDED(hr))
		{
			Event asyncFinished(CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS));
			auto callback = Microsoft::WRL::Callback<Implements<RuntimeClassFlags<ClassicCom>, IAsyncOperationCompletedHandler<ABI::Windows::Services::Store::StoreAppLicense*>, FtmBase>>(
				[&](IAsyncOperation<ABI::Windows::Services::Store::StoreAppLicense*>* operation, AsyncStatus status)
			{
				 
				if (status == AsyncStatus::Completed)
				{
					ComPtr<ABI::Windows::Services::Store::IStoreAppLicense> storeAppLicense;
					hr = operation->GetResults(&storeAppLicense);
					if (SUCCEEDED(hr) && storeAppLicense != nullptr )
					{
						HString extendedData; 
						
						hr = storeAppLicense->get_ExtendedJsonData( extendedData.GetAddressOf());
						if (SUCCEEDED(hr))
						{
							String^ data = ref new String(extendedData.Get());							 
							String^ customPolicyField1 = ref new String(L"customPolicyField1");
							Windows::Data::Json::JsonObject ^json = ref new Windows::Data::Json::JsonObject();
							if (Windows::Data::Json::JsonObject::TryParse(data, &json))
							{
								if (json->HasKey(customPolicyField1))
								{
									String^ campaignId = json->GetNamedString(customPolicyField1);
									hr = WindowsCreateString ( campaignId->Data(), campaignId->Length (), presultCampaignId);  
									 
								} 
							}
						}
					}
				}
				else if ( status == AsyncStatus::Error )
				{
					hr = E_FAIL; 
					RoOriginateError(hr, HStringReference(L"GetAppLicenseAsync failed").Get());
				}
				SetEvent(asyncFinished.Get());
				return S_OK;
			});

			asyncOperation->put_Completed(callback.Get());			 
			DWORD waitResult = WaitForSingleObjectEx(asyncFinished.Get(), NetworkRequestTimeOutMills, false);
			if (waitResult != WAIT_OBJECT_0 )
			{
				hr = E_FAIL; 
				RoOriginateError(hr, HStringReference(L"GetAppLicenseAsync timed out").Get());
			}
		}
	}
	return hr; 
} 




HRESULT CampaignIdHelper::GetCampaignIdFromCurrentAppWithWait(HSTRING* presultCampaignId)
{ 
	 
	ComPtr<ABI::Windows::ApplicationModel::Store::ICurrentAppWithCampaignId> currentApp;
	HRESULT hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Store_CurrentApp).Get(), &currentApp);
	 
	if (SUCCEEDED(hr))
	{
		Event asyncFinished(CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS));
		ComPtr<IAsyncOperation<HSTRING>> asyncOperation;
		hr = currentApp->GetAppPurchaseCampaignIdAsync(&asyncOperation);
		if (SUCCEEDED(hr))
		{			 
			auto campaignIdCallback = Microsoft::WRL::Callback<Implements<RuntimeClassFlags<ClassicCom>, IAsyncOperationCompletedHandler<HSTRING>, FtmBase>>(
				[&](IAsyncOperation<HSTRING>* operation, AsyncStatus status)
			{
				if (status == AsyncStatus::Completed)
				{
					hr = operation->GetResults(presultCampaignId );
					 
				}
				else
				{
					hr = E_FAIL;
					RoOriginateError(hr, HStringReference(L"GetAppPurchaseCampaignIdAsync failed").Get());
				} 
				SetEvent(asyncFinished.Get());
				return hr;  

			});
			asyncOperation->put_Completed(campaignIdCallback.Get());			
			DWORD waitResult = WaitForSingleObjectEx(asyncFinished.Get(), NetworkRequestTimeOutMills, false);


			if (waitResult != WAIT_OBJECT_0)
			{
				hr = E_FAIL; 
				RoOriginateError(hr, HStringReference(L"GetAppPurchaseCampaignIdAsync timed out").Get());
			}
		}
	}
 
	return hr;
} 




 
