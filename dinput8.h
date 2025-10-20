#pragma once

#define INITGUID

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class m_IDirectInput8A;
class m_IDirectInput8W;
class m_IDirectInputDevice8A;
class m_IDirectInputDevice8W;
class m_IDirectInputEffect;

#include "AddressLookupTable.h"
#include "Logging.h"

typedef HRESULT(WINAPI *DirectInput8CreateProc)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
typedef HRESULT(WINAPI *DllCanUnloadNowProc)();
typedef	HRESULT(WINAPI *DllGetClassObjectProc)(REFCLSID, REFIID, LPVOID *);
typedef HRESULT(WINAPI *DllRegisterServerProc)();
typedef HRESULT(WINAPI *DllUnregisterServerProc)();
typedef	LPCDIDATAFORMAT(WINAPI *GetdfDIJoystickProc)();

void genericQueryInterface(REFIID CalledID, LPVOID * ppvObj);
extern AddressLookupTable<void> ProxyAddressLookupTable;

#include "IDirectInput8A.h"
#include "IDirectInput8W.h"
#include "IDirectInputDevice8A.h"
#include "IDirectInputDevice8W.h"
#include "IDirectInputEffect.h"

extern POINT delta;
extern CRITICAL_SECTION deltaLock;
extern bool Dmousehilo[4];
extern bool Xenabled;
extern bool Dkeyhilo[18];// byte[] keytodinput
extern unsigned char keytodinput[18];

//extern bool statehilo{ 8 }; //2: key hi or low 

//extern int keytodinput[8] = { 0, DIK_B, DIK_C, DIK_D, DIK_E, DIK_F, DIK_G, DIK_H };
//2: bytes set to correct keys on init 

