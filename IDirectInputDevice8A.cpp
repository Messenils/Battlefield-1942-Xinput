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

bool combikeys[2];
HRESULT m_IDirectInputDevice8A::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	if ((riid == IID_IDirectInputDevice8A || riid == IID_IUnknown) && ppvObj)
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

ULONG m_IDirectInputDevice8A::AddRef()
{
	return ProxyInterface->AddRef();
}

ULONG m_IDirectInputDevice8A::Release()
{
	ULONG ref = ProxyInterface->Release();

	if (ref == 0)
	{
		delete this;
	}

	return ref;
}

HRESULT m_IDirectInputDevice8A::GetCapabilities(LPDIDEVCAPS lpDIDevCaps)
{
	return ProxyInterface->GetCapabilities(lpDIDevCaps);
}

HRESULT m_IDirectInputDevice8A::EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback, LPVOID pvRef, DWORD dwFlags)
{
	return ProxyInterface->EnumObjects(lpCallback, pvRef, dwFlags);
}

HRESULT m_IDirectInputDevice8A::GetProperty(REFGUID rguidProp, LPDIPROPHEADER pdiph)
{
	return ProxyInterface->GetProperty(rguidProp, pdiph);
}

HRESULT m_IDirectInputDevice8A::SetProperty(REFGUID rguidProp, LPCDIPROPHEADER pdiph)
{
	return ProxyInterface->SetProperty(rguidProp, pdiph);
}

HRESULT m_IDirectInputDevice8A::Acquire()
{
	return ProxyInterface->Acquire();
}

HRESULT m_IDirectInputDevice8A::Unacquire()
{
	return ProxyInterface->Unacquire();
}

HRESULT m_IDirectInputDevice8A::GetDeviceState(DWORD cbData, LPVOID lpvData)
{
	HRESULT hr = ProxyInterface->GetDeviceState(cbData, lpvData);
	EnterCriticalSection(&deltaLock);
	if (SUCCEEDED(hr) && lpvData && Xenabled)
	{
		
		// Check if this is a mouse device and the structure is the expected size
		if (cbData == sizeof(DIMOUSESTATE))
		{
			DIMOUSESTATE* pMouse = reinterpret_cast<DIMOUSESTATE*>(lpvData);
			// Example: scale movement by 50%
			
			pMouse->lX = delta.x;
			pMouse->lY = delta.y;

			//if (Dmousehilo[0] == true) pMouse->rgbButtons[0] = 0x80;
			//else pMouse->rgbButtons[0] = 0x00;
			//if (Dmousehilo[1] == true) pMouse->rgbButtons[1] = 0x80;
			//else pMouse->rgbButtons[1] = 0x00;

			if (Dmousehilo[0] == true) {
				pMouse->rgbButtons[0] = 0x80;
				combikeys[0] = true;
			}
			else {
				pMouse->rgbButtons[0] = 0x00;
				combikeys[0] = false;
			}
			if (Dmousehilo[1] == true) {
				pMouse->rgbButtons[1] = 0x80;
				combikeys[1] = true;
			}
			else {
				combikeys[1] = false;
				pMouse->rgbButtons[1] = 0x00;
			}

			if (Dmousehilo[2]) {
				pMouse->lZ = 120;
				Dmousehilo[2] = false;
			}
			else if (Dmousehilo[3]) {
				pMouse->lZ = -120;
				Dmousehilo[3] = false;
			}
			else pMouse->lZ = 0;
			//ZeroMemory(lpvData, cbData);
			// Optional: clamp or remap values here if needed
		}
		else if (cbData == sizeof(DIMOUSESTATE2))
		{
			DIMOUSESTATE2* pMouse = reinterpret_cast<DIMOUSESTATE2*>(lpvData);

			// Same logic for extended mouse state
			
			pMouse->lX = delta.x;
			pMouse->lY = delta.y;
			
			//if (Dmousehilo[0] == true) pMouse->rgbButtons[0] = 0x80;
			//else pMouse->rgbButtons[0] = 0x00;
			//if (Dmousehilo[1] == true) pMouse->rgbButtons[1] = 0x80;
			//else pMouse->rgbButtons[1] = 0x00;

			if (Dmousehilo[0] == true) {
				pMouse->rgbButtons[0] = 0x80;	
			}
			else {
				pMouse->rgbButtons[0] = 0x00;
			}
			if (Dmousehilo[1] == true) {
				pMouse->rgbButtons[1] = 0x80;
				combikeys[1] = true;
			}
			else {
				combikeys[1] = false;
				pMouse->rgbButtons[1] = 0x00;
			}


			if (Dmousehilo[2]) {
				pMouse->lZ = 120;
				Dmousehilo[2] = false;
			}
			else if (Dmousehilo[3]) {
				pMouse->lZ = -120;
				Dmousehilo[3] = false;
			}
			else pMouse->lZ = 0;
			//ZeroMemory(lpvData, cbData);

		}
		else if (cbData == 256) // Keyboard state buffer size
		{
			BYTE* pKeys = reinterpret_cast<BYTE*>(lpvData);
			
			for (int i = 0; i < 18; ++i)
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

			//parachute and jump common button
			if (pKeys[DIK_SPACE] == 0x80) 
			{ 
				pKeys[DIK_9] = 0x80;
			}
			else {
				pKeys[DIK_9] = 0;
			}

			//lshift and camera change common button
			if (pKeys[DIK_C] == 0x80)
			{
				pKeys[DIK_LSHIFT] = 0x80;
				combikeys[0] = true;
			}
			else {
				pKeys[DIK_LSHIFT] = 0;
				combikeys[0] = false;
			}

			if (combikeys[0])
			{
				if (pKeys[keytodinput[8]] == 0x80)
				{
					pKeys[keytodinput[8]] = 0;
					pKeys[DIK_F1] = 0x80;
				}
				else pKeys[DIK_F1] = 0;

				if (pKeys[keytodinput[9]] == 0x80)
				{
					pKeys[keytodinput[9]] = 0;
					pKeys[DIK_F2] = 0x80;
				}
				else pKeys[DIK_F2] = 0;

				if (pKeys[keytodinput[10]] == 0x80)
				{
					pKeys[keytodinput[10]] = 0;
					pKeys[DIK_F3] = 0x80;
				}
				else pKeys[DIK_F3] = 0;

				if (pKeys[keytodinput[11]] == 0x80)
				{
					pKeys[keytodinput[11]] = 0;
					pKeys[DIK_F4] = 0x80;
				}
				else pKeys[DIK_F4] = 0;

				if (pKeys[keytodinput[0]] == 0x80)
				{
					pKeys[keytodinput[0]] = 0;
					pKeys[DIK_F5] = 0x80;
				}
				else pKeys[DIK_F5] = 0;

				if (pKeys[keytodinput[1]] == 0x80)
				{
					pKeys[keytodinput[1]] = 0;
					pKeys[DIK_F6] = 0x80;
				}
				else pKeys[DIK_F6] = 0;

				if (pKeys[keytodinput[2]] == 0x80)
				{
					pKeys[keytodinput[2]] = 0;
					pKeys[DIK_F7] = 0x80;
				}
				else pKeys[DIK_F7] = 0;

				if (pKeys[keytodinput[3]] == 0x80)
				{
					pKeys[keytodinput[3]] = 0;
					pKeys[DIK_F8] = 0x80;
				}
				else pKeys[DIK_F8] = 0;
			}
			else {
				pKeys[DIK_F1] = 0;
				pKeys[DIK_F2] = 0;
				pKeys[DIK_F3] = 0;
				pKeys[DIK_F4] = 0;
				pKeys[DIK_F5] = 0;
				pKeys[DIK_F6] = 0;
				pKeys[DIK_F7] = 0;
				pKeys[DIK_F8] = 0;
			}

			if (combikeys[1]) 
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
				pKeys[DIK_5] = 0;
				pKeys[DIK_8] = 0;
				pKeys[DIK_7] = 0;
				pKeys[DIK_6] = 0;
			}

			//if (Dkeyhilo[0] == true)
			//	pKeys[0x1E] |= 0x80; // Set high bit to indicate key is pressed
			//else pKeys[0x1E] &= ~0x80; // Clear high bit to indicate key is released
			
			// Example: simulate pressing the 'A' key (DIK_A = 0x1E)
			

			// Example: simulate releasing the 'D' key (DIK_D = 0x20)
			//pKeys[0x20] &= ~0x80; // Clear high bit to indicate key is released

			// You can add more keys or logic here based on your input system
		}
		
	}
	LeaveCriticalSection(&deltaLock);
	return hr;
}


HRESULT m_IDirectInputDevice8A::GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags)
{
	return ProxyInterface->GetDeviceData(cbObjectData, rgdod, pdwInOut, dwFlags);
}

HRESULT m_IDirectInputDevice8A::SetDataFormat(LPCDIDATAFORMAT lpdf)
{
	return ProxyInterface->SetDataFormat(lpdf);
}

HRESULT m_IDirectInputDevice8A::SetEventNotification(HANDLE hEvent)
{
	return ProxyInterface->SetEventNotification(hEvent);
}

HRESULT m_IDirectInputDevice8A::SetCooperativeLevel(HWND hwnd, DWORD dwFlags)
{
	//return ProxyInterface->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE);
	return S_OK;
}

HRESULT m_IDirectInputDevice8A::GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj, DWORD dwHow)
{
	return ProxyInterface->GetObjectInfo(pdidoi, dwObj, dwHow);
}

HRESULT m_IDirectInputDevice8A::GetDeviceInfo(LPDIDEVICEINSTANCEA pdidi)
{
	return ProxyInterface->GetDeviceInfo(pdidi);
}

HRESULT m_IDirectInputDevice8A::RunControlPanel(HWND hwndOwner, DWORD dwFlags)
{
	return ProxyInterface->RunControlPanel(hwndOwner, dwFlags);
}

HRESULT m_IDirectInputDevice8A::Initialize(HINSTANCE hinst, DWORD dwVersion, REFGUID rguid)
{
	return ProxyInterface->Initialize(hinst, dwVersion, rguid);
}

HRESULT m_IDirectInputDevice8A::CreateEffect(REFGUID rguid, LPCDIEFFECT lpeff, LPDIRECTINPUTEFFECT * ppdeff, LPUNKNOWN punkOuter)
{
	HRESULT hr = ProxyInterface->CreateEffect(rguid, lpeff, ppdeff, punkOuter);

	if (SUCCEEDED(hr) && ppdeff)
	{
		*ppdeff = new m_IDirectInputEffect(*ppdeff);
	}

	return hr;
}

HRESULT m_IDirectInputDevice8A::EnumEffects(LPDIENUMEFFECTSCALLBACKA lpCallback, LPVOID pvRef, DWORD dwEffType)
{
	return ProxyInterface->EnumEffects(lpCallback, pvRef, dwEffType);
}

HRESULT m_IDirectInputDevice8A::GetEffectInfo(LPDIEFFECTINFOA pdei, REFGUID rguid)
{
	return ProxyInterface->GetEffectInfo(pdei, rguid);
}

HRESULT m_IDirectInputDevice8A::GetForceFeedbackState(LPDWORD pdwOut)
{
	return ProxyInterface->GetForceFeedbackState(pdwOut);
}

HRESULT m_IDirectInputDevice8A::SendForceFeedbackCommand(DWORD dwFlags)
{
	return ProxyInterface->SendForceFeedbackCommand(dwFlags);
}

HRESULT m_IDirectInputDevice8A::EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback, LPVOID pvRef, DWORD fl)
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

HRESULT m_IDirectInputDevice8A::Escape(LPDIEFFESCAPE pesc)
{
	return ProxyInterface->Escape(pesc);
}

HRESULT m_IDirectInputDevice8A::Poll()
{
	return ProxyInterface->Poll();
}

HRESULT m_IDirectInputDevice8A::SendDeviceData(DWORD cbObjectData, LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD fl)
{
	return ProxyInterface->SendDeviceData(cbObjectData, rgdod, pdwInOut, fl);
}

HRESULT m_IDirectInputDevice8A::EnumEffectsInFile(LPCSTR lpszFileName, LPDIENUMEFFECTSINFILECALLBACK pec, LPVOID pvRef, DWORD dwFlags)
{
	return ProxyInterface->EnumEffectsInFile(lpszFileName, pec, pvRef, dwFlags);
}

HRESULT m_IDirectInputDevice8A::WriteEffectToFile(LPCSTR lpszFileName, DWORD dwEntries, LPDIFILEEFFECT rgDiFileEft, DWORD dwFlags)
{
	return ProxyInterface->WriteEffectToFile(lpszFileName, dwEntries, rgDiFileEft, dwFlags);
}

HRESULT m_IDirectInputDevice8A::BuildActionMap(LPDIACTIONFORMATA lpdiaf, LPCSTR lpszUserName, DWORD dwFlags)
{
	return ProxyInterface->BuildActionMap(lpdiaf, lpszUserName, dwFlags);
}

HRESULT m_IDirectInputDevice8A::SetActionMap(LPDIACTIONFORMATA lpdiActionFormat, LPCSTR lptszUserName, DWORD dwFlags)
{
	return ProxyInterface->SetActionMap(lpdiActionFormat, lptszUserName, dwFlags);
}

HRESULT m_IDirectInputDevice8A::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERA lpdiDevImageInfoHeader)
{
	return ProxyInterface->GetImageInfo(lpdiDevImageInfoHeader);
}
