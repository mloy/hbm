#include <string>
#include <comdef.h>

// Define WIN32_LEAN_AND_MEAN because wbemidl.h includes windows.h. If WIN32_LEAN_AND_MEAN is not set, then windows.h includes winsock.h wich causes redefinition problems with winsock2.h.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN						// Exclude rarely-used stuff from Windows headers
#endif
#include <wbemidl.h>


#include "hbm/communication/wmi.h"


namespace hbm {
	namespace communication {

		IWbemLocator*	WMI::m_pLoc = NULL;
		IWbemServices*	WMI::m_pSvc = NULL;

		// Initialize WMI
		// Must be called before using any of the other methods
		long WMI::initWMI()
		{
			// Check if WMI has already been initialised
			if (m_pLoc && m_pSvc) {
				return true;
			}

			HRESULT hres = S_FALSE;
			
			// Initialize COM as it is needed by WMI
			hres = CoInitializeEx(0, COINIT_MULTITHREADED);
			if (FAILED(hres)) {
				return -1;
			}

			// Set general COM security levels --------------------------
			// Note: If you are using Windows 2000, you need to specify -
			// the default authentication credentials for a user by using
			// a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
			// parameter of CoInitializeSecurity ------------------------
			hres = CoInitializeSecurity(
			NULL,
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities
			NULL                         // Reserved
			);

			if (FAILED(hres)) {
				return -1;
			}
			

			// Obtain the initial locator to WMI -------------------------
			hres = CoCreateInstance(
				CLSID_WbemLocator,
				0,
				CLSCTX_INPROC_SERVER,
				IID_IWbemLocator,
				(LPVOID *)&m_pLoc);

			if (FAILED(hres)) {
				return -1;
			}


			// Connect to WMI through the IWbemLocator::ConnectServer method
			// Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
			hres = m_pLoc->ConnectServer(
				L"ROOT\\CIMV2",			// Object path of WMI namespace
				NULL,					// User name. NULL = current user
				NULL,                   // User password. NULL = current
				0,                      // Locale. NULL indicates current
				NULL,                   // Security flags.
				0,                      // Authority (for example, Kerberos)
				0,                      // Context object 
				&m_pSvc                   // pointer to IWbemServices proxy
			);

			if (FAILED(hres)) {
				m_pLoc->Release();
				m_pLoc = NULL;
				return -1;
			}


			// Set security levels on the proxy
			hres = CoSetProxyBlanket(
				m_pSvc,							// Indicates the proxy to set
				RPC_C_AUTHN_WINNT,				// RPC_C_AUTHN_xxx
				RPC_C_AUTHZ_NONE,				// RPC_C_AUTHZ_xxx
				NULL,							// Server principal name 
				RPC_C_AUTHN_LEVEL_CALL,			// RPC_C_AUTHN_LEVEL_xxx 
				RPC_C_IMP_LEVEL_IMPERSONATE,	// RPC_C_IMP_LEVEL_xxx
				NULL,							// client identity
				EOAC_NONE						// proxy capabilities 
			);

			if (FAILED(hres)) {
				m_pSvc->Release();
				m_pSvc = NULL;
				m_pLoc->Release();
				m_pLoc = NULL;
				return -1;
			}

			return 0;
		}

		// Clean resources
		void WMI::uninitWMI()
		{

			if (m_pSvc) {
				m_pSvc->Release();
				m_pSvc = NULL;
			}

			if (m_pLoc) {
				m_pLoc->Release();
				m_pLoc = NULL;
			}

			// Clean COM
			CoUninitialize();
		}

		// Check if the given adapter is a firewire adapter or not
		bool WMI::isFirewireAdapter(const communication::Netadapter &adapter)
		{
			// Make sure that WMI is initialised
			if (!m_pLoc || !m_pSvc) {
				return false;
			}

			HRESULT hres = S_FALSE;

			// Query the adapters having the index of this adapter and where the product name is 'HBM IEEE1394 IP Adapter'
			std::string query = "SELECT ProductName FROM Win32_NetworkAdapter WHERE ProductName='HBM IEEE1394 IP Adapter' AND InterfaceIndex=" + std::to_string(adapter.m_index);
			IEnumWbemClassObject* pEnumerator = NULL;
			hres = m_pSvc->ExecQuery(
				L"WQL",
				_bstr_t(query.c_str()),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
				NULL,
				&pEnumerator);
			if (FAILED(hres)) {
				return false;
			}

			// The query should have only one result or none. If there is one result, then this adapter is a firewire adapter, otherwise not.
			IWbemClassObject *pWMIAdapter = NULL;
			unsigned long ret;
			hres = pEnumerator->Next(WBEM_INFINITE, 1, &pWMIAdapter, &ret);
			pEnumerator->Release();
			if ((0 == ret) || (FAILED(hres))) {
				// This adapter is not a firewire adapter
				return false;
			}
			else {
				// This adapter is a firewire adapter
				pWMIAdapter->Release();
				return true;
			}
		}

		// Set the given adapter in DHCP mode
		long WMI::enableDHCP(const communication::Netadapter &adapter)
		{
			// Make sure that WMI is initialised
			if (!m_pLoc || !m_pSvc) {
				return -1;
			}

			HRESULT hres = S_FALSE;

			// Use the IWbemServices pointer to make a WQL query retrieving the right network adapter
			std::string query = "SELECT * FROM Win32_NetworkAdapterConfiguration WHERE InterfaceIndex=" + std::to_string(adapter.m_index);
			IEnumWbemClassObject* pEnumerator = NULL;
			hres = m_pSvc->ExecQuery(
				L"WQL",
				_bstr_t(query.c_str()),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
				NULL,
				&pEnumerator);

			if (FAILED(hres)) {
				return -1;
			}

			// The query should only have one result => this is our network adapter
			IWbemClassObject *pWMIAdapter = NULL;
			unsigned long ret;
			hres = pEnumerator->Next(WBEM_INFINITE, 1, &pWMIAdapter, &ret);
			pEnumerator->Release();
			if ((0 == ret) || (FAILED(hres))) {
				return -1;
			}

			// Get the WMI path to this network adapter
			VARIANT v;
			hres = pWMIAdapter->Get(L"__PATH", 0, &v, 0, 0);
			pWMIAdapter->Release();
			if (FAILED(hres)) {
				return -1;
			}

			// Execute the EnableDHCP() method of this network adapter using the IWbemServices pointer and the WMI path to the adapter
			IWbemClassObject *pOutInst = NULL;
			hres = m_pSvc->ExecMethod(V_BSTR(&v),
				L"EnableDHCP",
				0,
				NULL,
				NULL,
				&pOutInst,
				NULL);

			// Free memory
			VariantClear(&v);
			if (pOutInst) {
				pOutInst->Release();
			}

			if (WBEM_S_NO_ERROR != hres) {
				return -1;
			}

			return 0;
		}

		// Set the given adapter in manual mode and set the IPv4 address and subnet mask
		long WMI::setManualIpV4(const communication::Netadapter &adapter, const communication::Ipv4Address &manualConfig)
		{
			// Make sure that WMI is initialised
			if (!m_pLoc || !m_pSvc) {
				return -1;
			}

			HRESULT hres = S_FALSE;

			// Get the Win32_NetworkAdapterConfiguration class object so that we can retrieve information about the EnableStatic method
			IWbemClassObject *pClass = NULL;
			hres = m_pSvc->GetObject(L"Win32_NetworkAdapterConfiguration", 0, NULL, &pClass, NULL);
			if (WBEM_S_NO_ERROR != hres) {
				return -1;
			}

			// Get the EnableStatic method informations using the class pointer we just retrieved
			IWbemClassObject *pParamsIn = NULL;
			hres = pClass->GetMethod(L"EnableStatic", 0, &pParamsIn, NULL);
			pClass->Release();
			if (WBEM_S_NO_ERROR != hres) {
				return -1;
			}

			// Create an instance of the input parameter object (this will be used when calling the EnableStatic method)
			IWbemClassObject *pInInst = NULL;
			hres = pParamsIn->SpawnInstance(0, &pInInst);
			pParamsIn->Release();
			if (WBEM_S_NO_ERROR != hres) {
				return -1;
			}

			// Fill the input parameters
			// First fill the address
			SAFEARRAY *ip_list = SafeArrayCreateVector(VT_BSTR, 0, 1); // Create a safe array with one dimension containing one element
			long idx[] = { 0 };
			// Copy the address from multibyte string to wide char (into a BSTR)
			wchar_t sztmp[16];
			memset(sztmp, 0, sizeof(sztmp));
			mbstowcs(sztmp, manualConfig.address.c_str(), manualConfig.address.size());
			BSTR tmp = SysAllocString(sztmp);
			// Add the address in the dedicated array
			if (FAILED(SafeArrayPutElement(ip_list, idx, tmp)))
			{
				SysFreeString(tmp);
				pInInst->Release();
				SafeArrayDestroy(ip_list);
				return -1;
			}
			SysFreeString(tmp);

			// Then fill the network mask
			SAFEARRAY *netmask_list = SafeArrayCreateVector(VT_BSTR, 0, 1); // Create a safe array with one dimension containing one element
																			// Copy the address from multibyte string to wide char (into a BSTR)
			memset(sztmp, 0, sizeof(sztmp));
			mbstowcs(sztmp, manualConfig.netmask.c_str(), manualConfig.netmask.size());
			tmp = SysAllocString(sztmp);
			// Add the network mask in the dedicated array
			if (FAILED(SafeArrayPutElement(netmask_list, idx, tmp)))
			{
				SysFreeString(tmp);
				pInInst->Release();
				SafeArrayDestroy(ip_list);
				SafeArrayDestroy(netmask_list);
				return -1;
			}
			SysFreeString(tmp);

			// Now wrap each safe array in a VARIANT so that it can be passed to COM function
			VARIANT arg1_ES;
			VariantInit(&arg1_ES);
			arg1_ES.vt = VT_ARRAY | VT_BSTR;
			arg1_ES.parray = ip_list;

			VARIANT arg2_ES;
			VariantInit(&arg2_ES);
			arg2_ES.vt = VT_ARRAY | VT_BSTR;
			arg2_ES.parray = netmask_list;

			// Put the variants containing the address and network mask into our input parameters object
			if ((WBEM_S_NO_ERROR != pInInst->Put(L"IPAddress", 0, &arg1_ES, 0)) ||
				(WBEM_S_NO_ERROR != pInInst->Put(L"SubNetMask", 0, &arg2_ES, 0)))
			{
				pInInst->Release();
				SafeArrayDestroy(ip_list);
				SafeArrayDestroy(netmask_list);
			}

			// Use the IWbemServices pointer to make a WQL query retrieving the right network adapter
			std::string query = "SELECT * FROM Win32_NetworkAdapterConfiguration WHERE InterfaceIndex=" + std::to_string(adapter.m_index);
			IEnumWbemClassObject* pEnumerator = NULL;
			hres = m_pSvc->ExecQuery(
				L"WQL",
				_bstr_t(query.c_str()),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
				NULL,
				&pEnumerator);

			if (FAILED(hres)) {
				pInInst->Release();
				return -1;
			}

			// The query should only have one result => this is our network adapter
			IWbemClassObject *pWMIAdapter = NULL;
			unsigned long ret;
			hres = pEnumerator->Next(WBEM_INFINITE, 1, &pWMIAdapter, &ret);
			pEnumerator->Release();
			if ((0 == ret) || (FAILED(hres))) {
				pInInst->Release();
				return -1;
			}

			// Get the WMI path to this network adapter
			VARIANT v;
			hres = pWMIAdapter->Get(L"__PATH", 0, &v, 0, 0);
			pWMIAdapter->Release();
			if (FAILED(hres)) {
				pInInst->Release();
				return -1;
			}

			// Execute the EnableStatic() method of this network adapter using the IWbemServices pointer and the WMI path to the adapter.
			// The input parameters are given via the pInInst pointer.
			IWbemClassObject *pOutInst = NULL;
			hres = m_pSvc->ExecMethod(V_BSTR(&v),
				L"EnableStatic",
				0,
				NULL,
				pInInst,
				&pOutInst,
				NULL);

			// Free memory
			VariantClear(&v);
			if (pOutInst) {
				pOutInst->Release();
			}
			pInInst->Release();

			// Destroy safe arrays, which destroys the variant stored in them
			SafeArrayDestroy(ip_list);
			SafeArrayDestroy(netmask_list);

			if (WBEM_S_NO_ERROR != hres) {
				return -1;
			}

			return 0;
		}
	}
}