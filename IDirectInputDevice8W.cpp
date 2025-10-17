/**
* Copyright (C) 2020 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#include "dinput8.h"

bool combikeysW[1];

HRESULT m_IDirectInputDevice8W::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	if ((riid == IID_IDirectInputDevice8W || riid == IID_IUnknown) && ppvObj)
	{
		AddRef();

		*ppvObj = this;

		return S_OK;
	}

	HRESULT hr = ProxyInterface->QueryInterface(riid, ppvObj);

	if (SUCCEEDED(hr))
	{
		genericQueryInterface(riid, ppvObj);
	}

	return hr;
}

ULONG m_IDirectInputDevice8W::AddRef()
{
	return ProxyInterface->AddRef();
}

ULONG m_IDirectInputDevice8W::Release()
{
	ULONG ref = ProxyInterface->Release();

	if (ref == 0)
	{
		delete this;
	}

	return ref;
}

HRESULT m_IDirectInputDevice8W::GetCapabilities(LPDIDEVCAPS lpDIDevCaps)
{
	return ProxyInterface->GetCapabilities(lpDIDevCaps);
}

HRESULT m_IDirectInputDevice8W::EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback, LPVOID pvRef, DWORD dwFlags)
{
	return ProxyInterface->EnumObjects(lpCallback, pvRef, dwFlags);
}

HRESULT m_IDirectInputDevice8W::GetProperty(REFGUID rguidProp, LPDIPROPHEADER pdiph)
{
	return ProxyInterface->GetProperty(rguidProp, pdiph);
}

HRESULT m_IDirectInputDevice8W::SetProperty(REFGUID rguidProp, LPCDIPROPHEADER pdiph)
{
	return ProxyInterface->SetProperty(rguidProp, pdiph);
}

HRESULT m_IDirectInputDevice8W::Acquire()
{
	return ProxyInterface->Acquire();
}

HRESULT m_IDirectInputDevice8W::Unacquire()
{
	return ProxyInterface->Unacquire();
}


HRESULT m_IDirectInputDevice8W::GetDeviceState(DWORD cbData, LPVOID lpvData)
{

	HRESULT hr = ProxyInterface->GetDeviceState(cbData, lpvData);

	if (SUCCEEDED(hr) && lpvData)
	{
		// Check if this is a mouse device and the structure is the expected size
		if (cbData == sizeof(DIMOUSESTATE))
		{
			DIMOUSESTATE* pMouse = reinterpret_cast<DIMOUSESTATE*>(lpvData);

			// Example: scale movement by 50%
			EnterCriticalSection(&deltaLock);
			pMouse->lX = delta.x;
			pMouse->lY = delta.y;

			//if (Dmousehilo[0] == true) pMouse->rgbButtons[0] = 0x80;
			//else pMouse->rgbButtons[0] = 0x00;
			//if (Dmousehilo[1] == true) pMouse->rgbButtons[1] = 0x80;
			//else pMouse->rgbButtons[1] = 0x00;

			if (Dmousehilo[0] == true) {
				pMouse->rgbButtons[0] = 0x80;
				combikeysW[0] = true;
			}
			else {
				pMouse->rgbButtons[0] = 0x00;
				combikeysW[0] = false;
			}
			if (Dmousehilo[1] == true) {
				pMouse->rgbButtons[1] = 0x80;
				combikeysW[1] = true;
			}
			else {
				combikeysW[1] = false;
				pMouse->rgbButtons[1] = 0x00;
			}
			LeaveCriticalSection(&deltaLock);
			//pMouse->lX = 0;
			//pMouse->lY = 0;
			// Optional: clamp or remap values here if needed
			//ZeroMemory(lpvData, cbData);
					// Example: simulate left and right button press
		// Set to 0x80 to indicate button is pressed, 0 to release
			//pMouse->rgbButtons[0] = 0x80; // Left button
			//pMouse->rgbButtons[1] = 0x00; // Right button released
			//pMouse->rgbButtons[2] = 0x00; // Middle button
			//pMouse->rgbButtons[3] = 0x00; // Extra button

		}
		else if (cbData == sizeof(DIMOUSESTATE2))
		{
			DIMOUSESTATE2* pMouse = reinterpret_cast<DIMOUSESTATE2*>(lpvData);
			EnterCriticalSection(&deltaLock);
			pMouse->lX = delta.x;
			pMouse->lY = delta.y;

			//if (Dmousehilo[0] == true) pMouse->rgbButtons[0] = 0x80;
			//else pMouse->rgbButtons[0] = 0x00;
			//if (Dmousehilo[1] == true) pMouse->rgbButtons[1] = 0x80;
			//else pMouse->rgbButtons[1] = 0x00;

			if (Dmousehilo[0] == true) {
				pMouse->rgbButtons[0] = 0x80;
				combikeysW[0] = true;
			}
			else {
				pMouse->rgbButtons[0] = 0x00;
				combikeysW[0] = false;
			}
			if (Dmousehilo[1] == true) {
				pMouse->rgbButtons[1] = 0x80;
				combikeysW[1] = true;
			}
			else {
				combikeysW[1] = false;
				pMouse->rgbButtons[1] = 0x00;
			}
			LeaveCriticalSection(&deltaLock);
			//pMouse->lX = 0;
			//pMouse->lY = 0;
			//ZeroMemory(lpvData, cbData);
		}
		else if (cbData == 256) // Keyboard state buffer size
		{
			BYTE* pKeys = reinterpret_cast<BYTE*>(lpvData);
			EnterCriticalSection(&deltaLock);
			for (int i = 0; i < 17; ++i)
			{
				if (Dkeyhilo[i] == true && keytodinput[i] != 0x00) {
					pKeys[keytodinput[i]] |= 0x80; //80 high?
					//MessageBoxA(nullptr, "Setting A key high", "Warning", MB_OK | MB_ICONWARNING);
				}
				if (Dkeyhilo[i] == false && keytodinput[i] != 0x00) {
					pKeys[keytodinput[i]] &= ~0x80; //0 low?
					//MessageBoxA(nullptr, "Setting A key low", "Warning", MB_OK | MB_ICONWARNING);
				}
			}
			if (pKeys[DIK_SPACE] == 0x80)
			{
				pKeys[DIK_9] = 0x80;
			}
			else {
				pKeys[DIK_9] = 0;
			}

			if (combikeysW[1])
			{ //aim and pick up kits common button
				pKeys[DIK_G] = 0x80;
				//select weapons range 5-8
				if (pKeys[DIK_1] == 0x80)
				{
					pKeys[DIK_1] = 0;
					pKeys[DIK_5] = 0x80;
				}
				else pKeys[DIK_5] = 0;

				if (pKeys[DIK_2] == 0x80)
				{
					pKeys[DIK_2] = 0;
					pKeys[DIK_6] = 0x80;
				}
				else pKeys[DIK_6] = 0;

				if (pKeys[DIK_3] == 0x80)
				{
					pKeys[DIK_3] = 0;
					pKeys[DIK_7] = 0x80;
				}
				else pKeys[DIK_7] = 0;

				if (pKeys[DIK_4] == 0x80)
				{
					pKeys[DIK_4] = 0;
					pKeys[DIK_8] = 0x80;
				}
				else pKeys[DIK_8] = 0;
			}
			else {
				pKeys[DIK_G] = 0;
			}
			//if (Dkeyhilo[0] == true)
			//	pKeys[0x1E] |= 0x80; // Set high bit to indicate key is pressed
			//else pKeys[0x1E] &= ~0x80; // Clear high bit to indicate key is released
			LeaveCriticalSection(&deltaLock);

			// Example: simulate pressing the 'A' key (DIK_A = 0x1E)
			

			// Example: simulate releasing the 'D' key (DIK_D = 0x20)
			//pKeys[0x20] &= ~0x80; // Clear high bit to indicate key is released

			// You can add more keys or logic here based on your input system
		}

	}

	return hr;
}

HRESULT m_IDirectInputDevice8W::GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags)
{
	return ProxyInterface->GetDeviceData(cbObjectData, rgdod, pdwInOut, dwFlags);
}

HRESULT m_IDirectInputDevice8W::SetDataFormat(LPCDIDATAFORMAT lpdf)
{
	return ProxyInterface->SetDataFormat(lpdf);
}

HRESULT m_IDirectInputDevice8W::SetEventNotification(HANDLE hEvent)
{
	return ProxyInterface->SetEventNotification(hEvent);
}

HRESULT m_IDirectInputDevice8W::SetCooperativeLevel(HWND hwnd, DWORD dwFlags)
{
	return ProxyInterface->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE);
	//return S_OK;
}

HRESULT m_IDirectInputDevice8W::GetObjectInfo(LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow)
{
	return ProxyInterface->GetObjectInfo(pdidoi, dwObj, dwHow);
}

HRESULT m_IDirectInputDevice8W::GetDeviceInfo(LPDIDEVICEINSTANCEW pdidi)
{
	return ProxyInterface->GetDeviceInfo(pdidi);
}

HRESULT m_IDirectInputDevice8W::RunControlPanel(HWND hwndOwner, DWORD dwFlags)
{
	return ProxyInterface->RunControlPanel(hwndOwner, dwFlags);
}

HRESULT m_IDirectInputDevice8W::Initialize(HINSTANCE hinst, DWORD dwVersion, REFGUID rguid)
{
	return ProxyInterface->Initialize(hinst, dwVersion, rguid);
}

HRESULT m_IDirectInputDevice8W::CreateEffect(REFGUID rguid, LPCDIEFFECT lpeff, LPDIRECTINPUTEFFECT * ppdeff, LPUNKNOWN punkOuter)
{
	HRESULT hr = ProxyInterface->CreateEffect(rguid, lpeff, ppdeff, punkOuter);

	if (SUCCEEDED(hr) && ppdeff)
	{
		*ppdeff = new m_IDirectInputEffect(*ppdeff);
	}

	return hr;
}

HRESULT m_IDirectInputDevice8W::EnumEffects(LPDIENUMEFFECTSCALLBACKW lpCallback, LPVOID pvRef, DWORD dwEffType)
{
	return ProxyInterface->EnumEffects(lpCallback, pvRef, dwEffType);
}

HRESULT m_IDirectInputDevice8W::GetEffectInfo(LPDIEFFECTINFOW pdei, REFGUID rguid)
{
	return ProxyInterface->GetEffectInfo(pdei, rguid);
}

HRESULT m_IDirectInputDevice8W::GetForceFeedbackState(LPDWORD pdwOut)
{
	return ProxyInterface->GetForceFeedbackState(pdwOut);
}

HRESULT m_IDirectInputDevice8W::SendForceFeedbackCommand(DWORD dwFlags)
{
	return ProxyInterface->SendForceFeedbackCommand(dwFlags);
}

HRESULT m_IDirectInputDevice8W::EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback, LPVOID pvRef, DWORD fl)
{
	if (!lpCallback)
	{
		return E_INVALIDARG;
	}

	struct EnumEffect
	{
		LPVOID pvRef;
		LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback;

		static BOOL CALLBACK EnumEffectCallback(LPDIRECTINPUTEFFECT pdeff, LPVOID pvRef)
		{
			EnumEffect *self = (EnumEffect*)pvRef;

			if (pdeff)
			{
				pdeff = ProxyAddressLookupTable.FindAddress<m_IDirectInputEffect>(pdeff);
			}

			return self->lpCallback(pdeff, self->pvRef);
		}
	} CallbackContext;
	CallbackContext.pvRef = pvRef;
	CallbackContext.lpCallback = lpCallback;

	return ProxyInterface->EnumCreatedEffectObjects(EnumEffect::EnumEffectCallback, &CallbackContext, fl);
}

HRESULT m_IDirectInputDevice8W::Escape(LPDIEFFESCAPE pesc)
{
	return ProxyInterface->Escape(pesc);
}

HRESULT m_IDirectInputDevice8W::Poll()
{
	return ProxyInterface->Poll();
}

HRESULT m_IDirectInputDevice8W::SendDeviceData(DWORD cbObjectData, LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD fl)
{
	return ProxyInterface->SendDeviceData(cbObjectData, rgdod, pdwInOut, fl);
}

HRESULT m_IDirectInputDevice8W::EnumEffectsInFile(LPCWSTR lpszFileName, LPDIENUMEFFECTSINFILECALLBACK pec, LPVOID pvRef, DWORD dwFlags)
{
	return ProxyInterface->EnumEffectsInFile(lpszFileName, pec, pvRef, dwFlags);
}

HRESULT m_IDirectInputDevice8W::WriteEffectToFile(LPCWSTR lpszFileName, DWORD dwEntries, LPDIFILEEFFECT rgDiFileEft, DWORD dwFlags)
{
	return ProxyInterface->WriteEffectToFile(lpszFileName, dwEntries, rgDiFileEft, dwFlags);
}

HRESULT m_IDirectInputDevice8W::BuildActionMap(LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUserName, DWORD dwFlags)
{
	return ProxyInterface->BuildActionMap(lpdiaf, lpszUserName, dwFlags);
}

HRESULT m_IDirectInputDevice8W::SetActionMap(LPDIACTIONFORMATW lpdiActionFormat, LPCWSTR lptszUserName, DWORD dwFlags)
{
	return ProxyInterface->SetActionMap(lpdiActionFormat, lptszUserName, dwFlags);
}

HRESULT m_IDirectInputDevice8W::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader)
{
	return ProxyInterface->GetImageInfo(lpdiDevImageInfoHeader);
}
