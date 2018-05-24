//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "ppltasks.h"

using namespace SampleCallerCPP;
using namespace Concurrency;


using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
	this->Loaded += ref new RoutedEventHandler( this, &SampleCallerCPP::MainPage::OnLoaded); 
}

void MainPage::OnLoaded(Object ^unused, RoutedEventArgs^ unused2 )
{

	WRLSample::CampaignIdHelper^ helper = ref new WRLSample::CampaignIdHelper();
	create_task(helper->GetCampaignId81Async()).then([this](task<String^> t)
	{
		String^ cid;
		try
		{
			cid = t.get();
		}
		catch (Platform::COMException^ cex)
		{
			std::wstring str = L"COM Exception handled: "; 
			str += cex->Message->Data();
			::OutputDebugString( str.c_str()); 
		}
		catch (std::exception stdex)
		{
			std::wstring str = L"std Exception handled";		 
			::OutputDebugString(str.c_str());
		}
		catch (...)
		{
			::OutputDebugString(L"Some other exception"); 
		}

		String^ message = ref new String( L"CID is: "); 

		if (cid != nullptr && cid->Length() > 0)
			message += cid;
		else
			message += "'(empty)'"; 

		this->Result->Text = message; 
	}, task_continuation_context::use_current());

	

}


