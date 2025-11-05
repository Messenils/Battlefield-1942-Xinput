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

#include <cmath>
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <Xinput.h>
#include <psapi.h>
#include <string>


#pragma comment(lib, "Xinput9_1_0.lib")

HMODULE g_hModule = nullptr;



POINT fakecursor;
bool loop = true;  
int counter = 0;

int userealmouse = 0;
int atick = 0;


//fake cursor
int controllerID = 0;
int Xf = 20;
int Yf = 20;

WORD vibrator = 0;
float radial_deadzone = 0.10f; // Circular/Radial Deadzone (0.0 to 0.3)
float axial_deadzone = 0.00f; // Square/Axial Deadzone (0.0 to 0.3)
const float max_threshold = 0.03f; // Max Input Threshold, an "outer deadzone" (0.0 to 0.15)
const float curve_slope = 0.16f; // The linear portion of the response curve (0.0 to 1.0)
const float curve_exponent = 5.00f; // The exponential portion of the curve (1.0 to 10.0)
float sensitivity = 20.00f; // Base sensitivity / max speed (1.0 to 30.0)
float accel_multiplier = 1.90f; // Look Acceleration Multiplier (1.0 to 3.0)

///shares with dinput8 hooks
POINT delta = { 0, 0 }; //extern to ship to getdevicestate ccp
CRITICAL_SECTION deltaLock;  // Definition and storage

//dik codes
int DIKA;
int DIKB;
int DIKC;
int DIKD;
int DIKE;
int DIKF;
int DIKG;
int DIKH;
int DIKI;
int DIKJ;
int DIKK;
int DIKL;
int DIKM;
int DIKN;
int DIKO;
int DIKP;
int DIKQ;
int DIKR;
int DIKS;
int DIKT;
int DIKU;
int DIKV;
int DIKW;
bool Dmousehilo[4];
bool Dkeyhilo[18];// byte[] keytodinput
unsigned char keytodinput[18] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


COLORREF colors[5] = {
    RGB(0, 0, 0),          // Transparent - won't be drawn
    RGB(140, 140, 140),    // Gray for blade
    RGB(255, 255, 255),    // White shine
    RGB(139, 69, 19),       // Brown handle
    RGB(50, 150, 255)     // Light blue accent

};

int tick = 0;
HWND hwnd;
//remember old keystates
int oldscrollrightaxis = false; //reset 
int oldscrollleftaxis = false; //reset 
int oldscrollupaxis = false; //reset 
int oldscrolldownaxis = false; //reset 
bool Apressed = false;
bool Bpressed = false;
bool Xpressed = false;
bool Ypressed = false;
bool leftPressedold;
bool rightPressedold;
bool oldA = false;
bool oldB = false;
bool oldX = false;
bool oldY = false;
bool oldC = false;
bool oldD = false;
bool oldE = false;
bool oldF = false;
bool oldup = false;
bool olddown = false;
bool oldleft = false;
bool oldright = false;
bool oldback = false;
int startsearch = 0;
int startsearchA = 0;
int startsearchB = 0;
int startsearchX = 0;
int startsearchY = 0;
int startsearchC = 0;
int startsearchD = 0;
int startsearchE = 0;
int startsearchF = 0;

int righthanded = 0; //this will also disable dll if above 10


int Atype = 0;
int Btype = 0;
int Xtype = 0;
int Ytype = 0;
int Ctype = 0;
int Dtype = 0;
int Etype = 0;
int Ftype = 0;
int starttype = 0;
int backtype = 0;
int uptype = 0;
int downtype = 0;
int lefttype = 0;
int righttype = 0;


std::vector<BYTE> largePixels, smallPixels;
SIZE screenSize;
int strideLarge, strideSmall;
int smallW, smallH;

int mode = 0;
//int sovetid = 16;
int knappsovetid = 100;

int samekey = 0;
int samekeyA = 0;


std::ofstream Log::LOG("dinput8.log");
AddressLookupTable<void> ProxyAddressLookupTable = AddressLookupTable<void>();

DirectInput8CreateProc m_pDirectInput8Create;
DllCanUnloadNowProc m_pDllCanUnloadNow;
DllGetClassObjectProc m_pDllGetClassObject;
DllRegisterServerProc m_pDllRegisterServer;
DllUnregisterServerProc m_pDllUnregisterServer;
GetdfDIJoystickProc m_pGetdfDIJoystick;


bool WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    static HMODULE dinput8dll = nullptr;
    InitializeCriticalSection(&deltaLock);
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Load dll
        char path[MAX_PATH];
        GetSystemDirectoryA(path, MAX_PATH);
        strcat_s(path, "\\dinput8.dll");
        Log() << "Loading " << path;
        dinput8dll = LoadLibraryA(path);

        // Get function addresses
        m_pDirectInput8Create = (DirectInput8CreateProc)GetProcAddress(dinput8dll, "DirectInput8Create");
        m_pDllCanUnloadNow = (DllCanUnloadNowProc)GetProcAddress(dinput8dll, "DllCanUnloadNow");
        m_pDllGetClassObject = (DllGetClassObjectProc)GetProcAddress(dinput8dll, "DllGetClassObject");
        m_pDllRegisterServer = (DllRegisterServerProc)GetProcAddress(dinput8dll, "DllRegisterServer");
        m_pDllUnregisterServer = (DllUnregisterServerProc)GetProcAddress(dinput8dll, "DllUnregisterServer");
        m_pGetdfDIJoystick = (GetdfDIJoystickProc)GetProcAddress(dinput8dll, "GetdfDIJoystick");
        break;

    case DLL_PROCESS_DETACH:
        FreeLibrary(dinput8dll);
        break;
    }

    return true;
}

void vibrateController(int controllerId, WORD strength)
{
    XINPUT_VIBRATION vibration = {};
    vibration.wLeftMotorSpeed = strength;   // range: 0 - 65535
    vibration.wRightMotorSpeed = strength;

    XInputSetState(controllerId, &vibration);
}

bool SendMouseClick(int x, int y, int z, int many) 
{
    // Create a named mutex


        if (z == 1) {

            EnterCriticalSection(&deltaLock);
            Dmousehilo[0] = true;
            LeaveCriticalSection(&deltaLock);
            Sleep(1);
            EnterCriticalSection(&deltaLock);
            Dmousehilo[0] = false;
            LeaveCriticalSection(&deltaLock);
        }
        if (z == 2) {
            EnterCriticalSection(&deltaLock);
            Dmousehilo[1] = true;
            LeaveCriticalSection(&deltaLock);
            Sleep(1);
            EnterCriticalSection(&deltaLock);
            Dmousehilo[1] = false;
            LeaveCriticalSection(&deltaLock);
        }
        if (z == 3) {
            EnterCriticalSection(&deltaLock);
            Dmousehilo[0] = true;
            LeaveCriticalSection(&deltaLock);
        }
        if (z == 4)
        {
            EnterCriticalSection(&deltaLock);
            Dmousehilo[0] = false;
            LeaveCriticalSection(&deltaLock);

        }
        if (z == 5) {
            EnterCriticalSection(&deltaLock);
            Dmousehilo[1] = true;
            LeaveCriticalSection(&deltaLock);
        }
        if (z == 6)
        {
            EnterCriticalSection(&deltaLock);
            Dmousehilo[1] = false;
            LeaveCriticalSection(&deltaLock);

        }
        return true;
    
}

//hwnd for post message
HWND GetMainWindowHandle(DWORD targetPID) {
    HWND hwnd = nullptr;
    struct HandleData {
        DWORD pid;
        HWND hwnd;
    } data = { targetPID, nullptr };

    auto EnumWindowsCallback = [](HWND hWnd, LPARAM lParam) -> BOOL {
        HandleData* pData = reinterpret_cast<HandleData*>(lParam);
        DWORD windowPID = 0;
        GetWindowThreadProcessId(hWnd, &windowPID);
        if (windowPID == pData->pid && GetWindow(hWnd, GW_OWNER) == nullptr && IsWindowVisible(hWnd)) {
            pData->hwnd = hWnd;
            return FALSE; // Stop enumeration
        }
        return TRUE; // Continue
        };

    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

bool IsTriggerPressed(BYTE triggerValue) {
    BYTE threshold = 175;
    return triggerValue > threshold;
}

// Helper: Get stick magnitude
float GetStickMagnitude(SHORT x, SHORT y) {
    return sqrtf(static_cast<float>(x) * x + static_cast<float>(y) * y);
}

std::wstring WGetExecutableFolder() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exePath(path);
    size_t lastSlash = exePath.find_last_of(L"\\/");

    if (lastSlash == std::wstring::npos)
        return L"";

    return exePath.substr(0, lastSlash);
}
POINT CalculateUltimateCursorMove(
    SHORT stickX, SHORT stickY,
    float c_deadzone,
    float s_deadzone,
    float max_threshold,
    float curve_slope,
    float curve_exponent,
    float sensitivity,
    float accel_multiplier
) {
    static double mouseDeltaAccumulatorX = 0.0;
    static double mouseDeltaAccumulatorY = 0.0;

    double normX = static_cast<double>(stickX) / 32767.0;
    double normY = static_cast<double>(stickY) / 32767.0;

    double magnitude = std::sqrt(normX * normX + normY * normY);
    if (magnitude < c_deadzone) {
        return { 0, 0 }; // Inside circular deadzone
    }
    if (std::abs(normX) < s_deadzone) {
        normX = 0.0; // Inside axial deadzone for X
    }
    if (std::abs(normY) < s_deadzone) {
        normY = 0.0; // Inside axial deadzone for Y
    }
    magnitude = std::sqrt(normX * normX + normY * normY);
    if (magnitude < 1e-6) {
        return { 0, 0 };
    }

    double effectiveRange = 1.0 - max_threshold - c_deadzone;
    if (effectiveRange < 1e-6) effectiveRange = 1.0;

    double remappedMagnitude = (magnitude - c_deadzone) / effectiveRange;
    remappedMagnitude = (std::max)(0.0, (std::min)(1.0, remappedMagnitude));

    double curvedMagnitude = curve_slope * remappedMagnitude + (1.0 - curve_slope) * std::pow(remappedMagnitude, curve_exponent);

    double finalSpeed = sensitivity * accel_multiplier;

    double dirX = normX / magnitude;
    double dirY = normY / magnitude;
    double finalMouseDeltaX = dirX * curvedMagnitude * finalSpeed;
    double finalMouseDeltaY = dirY * curvedMagnitude * finalSpeed;

    mouseDeltaAccumulatorX += finalMouseDeltaX;
    mouseDeltaAccumulatorY += finalMouseDeltaY;
    LONG integerDeltaX = static_cast<LONG>(mouseDeltaAccumulatorX);
    LONG integerDeltaY = static_cast<LONG>(mouseDeltaAccumulatorY);

    mouseDeltaAccumulatorX -= integerDeltaX;
    mouseDeltaAccumulatorY -= integerDeltaY;

    return { integerDeltaX, -integerDeltaY };
}
void PostKeyFunction(HWND hwnd, int keytype, bool press) {
    DWORD mykey = 0;
    DWORD presskey = WM_KEYDOWN;

    UINT scanCode = MapVirtualKey(VK_LEFT, MAPVK_VK_TO_VSC);
    LPARAM lParam = (0x00000001 | (scanCode << 16));

    if (!press) {
        presskey = WM_KEYUP; // Key up event 
    }

    //standard keys for dpad
    if (keytype == -1)
        mykey = VK_UP;
    if (keytype == -2)
        mykey = VK_DOWN;
    if (keytype == -3)
        mykey = VK_LEFT;
    if (keytype == -4)
        mykey = VK_RIGHT;

    if (keytype == 3)
        mykey = VK_ESCAPE;
    if (keytype == 4)
        mykey = VK_RETURN;
    if (keytype == 5)
        mykey = VK_TAB;
    if (keytype == 6)
        mykey = VK_SHIFT;
    if (keytype == 7)
        mykey = VK_CONTROL;
    if (keytype == 8)
        mykey = VK_SPACE;

    if (keytype == 9)
        mykey = 0x4D; //M

    if (keytype == 10)
        mykey = 0x57; //W

    if (keytype == 11)
        mykey = 0x53; //S

    if (keytype == 12)
        mykey = 0x41; //A

    if (keytype == 13)
        mykey = 0x44; //D

    if (keytype == 14)
        mykey = 0x45; //E

    if (keytype == 15)
        mykey = 0x46; //F

    if (keytype == 16)
        mykey = 0x47; //G

    if (keytype == 17)
        mykey = 0x48; //H

    if (keytype == 18)
        mykey = 0x49; //I

    if (keytype == 19)
        mykey = 0x51; //Q

    if (keytype == 20)
        mykey = VK_OEM_PERIOD;

    if (keytype == 21)
        mykey = 0x52; //R

    if (keytype == 22)
        mykey = 0x54; //T

    if (keytype == 23)
        mykey = 0x42; //B

    if (keytype == 24)
        mykey = 0x43; //C

    if (keytype == 25)
        mykey = 0x4B; //K

    if (keytype == 26)
        mykey = 0x55; //U

    if (keytype == 27)
        mykey = 0x56; //V

    if (keytype == 28)
        mykey = 0x57; //W

    if (keytype == 30)
        mykey = 0x30; //0

    if (keytype == 31)
        mykey = 0x31; //1

    if (keytype == 32)
        mykey = 0x32; //2

    if (keytype == 33)
        mykey = 0x33; //3

    if (keytype == 34)
        mykey = 0x34; //4

    if (keytype == 35)
        mykey = 0x35; //5

    if (keytype == 36)
        mykey = 0x36; //6

    if (keytype == 37)
        mykey = 0x37; //7

    if (keytype == 38)
        mykey = 0x38; //8

    if (keytype == 39)
        mykey = 0x39; //9

    if (keytype == 40)
        mykey = VK_UP;

    if (keytype == 41)
        mykey = VK_DOWN;

    if (keytype == 42)
        mykey = VK_LEFT;

    if (keytype == 43)
        mykey = VK_RIGHT;

    if (keytype == 44)
        mykey = 0x58; //X

    if (keytype == 45)
        mykey = 0x5A; //Z

    if (keytype == 20)
        mykey = VK_OEM_PERIOD;

    if (keytype == 51)
        mykey = VK_F1;

    if (keytype == 52)
        mykey = VK_F2;

    if (keytype == 53)
        mykey = VK_F3;

    if (keytype == 54)
        mykey = VK_F4;

    if (keytype == 55)
        mykey = VK_F5;

    if (keytype == 56)
        mykey = VK_F6;

    if (keytype == 57)
        mykey = VK_F7;

    if (keytype == 58)
        mykey = VK_F8;
    if (keytype == 59)
        mykey = VK_F9;

    if (keytype == 60)
        mykey = VK_F10;

    if (keytype == 61)
        mykey = VK_F11;

    if (keytype == 62)
        mykey = VK_F12;

    if (keytype == 63) { //control+C
        mykey = VK_CONTROL;
    }
    if (keytype == 70)
        mykey = VK_NUMPAD0;

    if (keytype == 71)
        mykey = VK_NUMPAD1;

    if (keytype == 72)
        mykey = VK_NUMPAD2;

    if (keytype == 73)
        mykey = VK_NUMPAD3;

    if (keytype == 74)
        mykey = VK_NUMPAD4;

    if (keytype == 75)
        mykey = VK_NUMPAD5;

    if (keytype == 76)
        mykey = VK_NUMPAD6;

    if (keytype == 77)
        mykey = VK_NUMPAD7;

    if (keytype == 78)
        mykey = VK_NUMPAD8;

    if (keytype == 79)
        mykey = VK_NUMPAD9;

    if (keytype == 80)
        mykey = VK_SUBTRACT;

    if (keytype == 81)
        mykey = VK_ADD;
    PostMessage(hwnd, presskey, mykey, lParam);
    PostMessage(hwnd, WM_INPUT, VK_RIGHT, lParam);
    if (keytype == 63) {
        PostMessage(hwnd, presskey, 0x43, lParam);
    }
    return;

}
//int akkumulator = 0; 
byte DIKcodes(int DIK) {
    //system keys
    if (DIK == 1) return DIK_ESCAPE;
    if (DIK == 2) return DIK_RETURN;
    if (DIK == 3) return DIK_TAB;
    if (DIK == 4) return DIK_LSHIFT;
    if (DIK == 5) return DIK_RSHIFT;
    if (DIK == 6) return DIK_LSHIFT;
    if (DIK == 7) return DIK_LCONTROL;
    if (DIK == 8) return DIK_LCONTROL;
    if (DIK == 9) return DIK_RCONTROL;
    if (DIK == 10) return DIK_LALT;
    if (DIK == 11) return DIK_LALT;
    if (DIK == 12) return DIK_RALT;
    if (DIK == 13) return DIK_SPACE;
    if (DIK == 14) return DIK_UP;
    if (DIK == 15) return DIK_DOWN;
    if (DIK == 16) return DIK_LEFT;
    if (DIK == 17) return DIK_RIGHT;
    if (DIK == 18) return DIK_BACKSPACE;
    if (DIK == 19) return DIK_DELETE;
    if (DIK == 20) return DIK_INSERT;
    if (DIK == 21) return DIK_END;
    if (DIK == 22) return DIK_HOME;
    if (DIK == 23) return DIK_PGUP;
    if (DIK == 24) return DIK_PGDN;
    //alphenbet
    if (DIK == 25) return DIK_A;
    if (DIK == 26) return DIK_B;
    if (DIK == 27) return DIK_C;
    if (DIK == 28) return DIK_D;
    if (DIK == 29) return DIK_E;
    if (DIK == 30) return DIK_F;
    if (DIK == 31) return DIK_G;
    if (DIK == 32) return DIK_H;
    if (DIK == 33) return DIK_I;
    if (DIK == 34) return DIK_J;
    if (DIK == 35) return DIK_K;
    if (DIK == 36) return DIK_L;
    if (DIK == 37) return DIK_M;
    if (DIK == 38) return DIK_N;
    if (DIK == 39) return DIK_O;
    if (DIK == 40) return DIK_P;
    if (DIK == 41) return DIK_Q;
    if (DIK == 42) return DIK_R;
    if (DIK == 43) return DIK_S;
    if (DIK == 44) return DIK_T;
    if (DIK == 45) return DIK_U;
    if (DIK == 46) return DIK_V;
    if (DIK == 47) return DIK_W;
    if (DIK == 48) return DIK_X;
    if (DIK == 49) return DIK_Y;
    if (DIK == 50) return DIK_Z;
    //0-9
    if (DIK == 51) return DIK_0;
    if (DIK == 52) return DIK_1;
    if (DIK == 53) return DIK_2;
    if (DIK == 54) return DIK_3;
    if (DIK == 55) return DIK_4;
    if (DIK == 56) return DIK_5;
    if (DIK == 57) return DIK_6;
    if (DIK == 58) return DIK_7;
    if (DIK == 59) return DIK_8;
    if (DIK == 60) return DIK_9;
    //F function keys
    if (DIK == 61) return DIK_F1;
    if (DIK == 62) return DIK_F2;
    if (DIK == 63) return DIK_F3;
    if (DIK == 64) return DIK_F4;
    if (DIK == 65) return DIK_F5;
    if (DIK == 66) return DIK_F6;
    if (DIK == 67) return DIK_F7;
    if (DIK == 68) return DIK_F8;
    if (DIK == 69) return DIK_F9;
    if (DIK == 70) return DIK_F10;
    if (DIK == 71) return DIK_F11;
    if (DIK == 72) return DIK_F12;
    //numpad
    if (DIK == 73) return DIK_NUMPAD0;
    if (DIK == 74) return DIK_NUMPAD1;
    if (DIK == 75) return DIK_NUMPAD2;
    if (DIK == 76) return DIK_NUMPAD3;
    if (DIK == 77) return DIK_NUMPAD4;
    if (DIK == 78) return DIK_NUMPAD5;
    if (DIK == 79) return DIK_NUMPAD6;
    if (DIK == 80) return DIK_NUMPAD7;
    if (DIK == 81) return DIK_NUMPAD8;
    if (DIK == 82) return DIK_NUMPAD9;
    if (DIK == 83) return DIK_ADD;
    if (DIK == 84) return DIK_MINUS;
    if (DIK == 85) return DIK_MULTIPLY;
    if (DIK == 86) return DIK_DIVIDE;
    if (DIK == 87) return DIK_DECIMAL;
    if (DIK == 88) return DIK_NUMPADENTER;
    if (DIK == 91) return DIK_INSERT;
    if (DIK == 92) return DIK_END;
    if (DIK == 93) return DIK_NUMPAD2;//numpad down
    if (DIK == 94) return DIK_PGDN;
    if (DIK == 95) return DIK_NUMPAD4; //num left
    if (DIK == 96) return DIK_NUMPAD6; //numright
    if (DIK == 97) return DIK_HOME;
    if (DIK == 98) return DIK_NUMPAD8; //numup
    if (DIK == 99) return DIK_PGUP;
    if (DIK == 100) return DIK_DELETE;


    else return 0x00;
}
bool disabled = false; //to stop messagebox on controller disconnect
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
    Sleep(2000);
    wchar_t buffer[256];
	bool resized = false;

    std::wstring iniPath = WGetExecutableFolder() + L"\\Xinput.ini";
    std::wstring iniSettings = L"Settings";
    //windowpos and res
    int posX = GetPrivateProfileIntW(iniSettings.c_str(), L"posX", 0, iniPath.c_str());
    int posY = GetPrivateProfileIntW(iniSettings.c_str(), L"posY", 0, iniPath.c_str());
    int resX = GetPrivateProfileIntW(iniSettings.c_str(), L"resX", 0, iniPath.c_str());
    int resY = GetPrivateProfileIntW(iniSettings.c_str(), L"resY", 0, iniPath.c_str());
    //controller settings
    controllerID = GetPrivateProfileIntW(iniSettings.c_str(), L"Controllerid", -9999, iniPath.c_str()); //simple test if settings read but write it wont work.
    int AxisLeftsens = GetPrivateProfileIntW(iniSettings.c_str(), L"AxisLeftsens", -7849, iniPath.c_str());
    int AxisRightsens = GetPrivateProfileIntW(iniSettings.c_str(), L"AxisRightsens", 12000, iniPath.c_str());
    int AxisUpsens = GetPrivateProfileIntW(iniSettings.c_str(), L"AxisUpsens", 0, iniPath.c_str());
    int AxisDownsens = GetPrivateProfileIntW(iniSettings.c_str(), L"AxisDownsens", -16049, iniPath.c_str());
    int scrollspeed3 = GetPrivateProfileIntW(iniSettings.c_str(), L"Scrollspeed", 150, iniPath.c_str());
    righthanded = GetPrivateProfileIntW(iniSettings.c_str(), L"Righthanded", 0, iniPath.c_str());

    GetPrivateProfileStringW(iniSettings.c_str(), L"Radial_Deadzone", L"0.2", buffer, sizeof(buffer), iniPath.c_str());
    radial_deadzone = std::stof(buffer); //sensitivity

    GetPrivateProfileStringW(iniSettings.c_str(), L"Axial_Deadzone", L"0.0", buffer, sizeof(buffer), iniPath.c_str());
    axial_deadzone = std::stof(buffer); //sensitivity

    GetPrivateProfileStringW(iniSettings.c_str(), L"Sensitivity", L"20.0", buffer, sizeof(buffer), iniPath.c_str());
    sensitivity = std::stof(buffer); //sensitivity //accel_multiplier

    GetPrivateProfileStringW(iniSettings.c_str(), L"Accel_Multiplier", L"1.7", buffer, sizeof(buffer), iniPath.c_str());
    accel_multiplier = std::stof(buffer);

    //mode
    int responsetime = GetPrivateProfileIntW(iniSettings.c_str(), L"Responsetime", 0, iniPath.c_str());
    int scrollenddelay = GetPrivateProfileIntW(iniSettings.c_str(), L"Scrolldelay", 100, iniPath.c_str()); //repost scroll weapon

    //clicknotmove 2
    //movenotclick 1
    Atype = GetPrivateProfileIntW(iniSettings.c_str(), L"Ainputtype", 13, iniPath.c_str());
    Btype = GetPrivateProfileIntW(iniSettings.c_str(), L"Binputtype", 50, iniPath.c_str());
    Xtype = GetPrivateProfileIntW(iniSettings.c_str(), L"Xinputtype", 8, iniPath.c_str());
    Ytype = GetPrivateProfileIntW(iniSettings.c_str(), L"Yinputtype", 29, iniPath.c_str());
    Ctype = GetPrivateProfileIntW(iniSettings.c_str(), L"Cinputtype", 14, iniPath.c_str());
    Dtype = GetPrivateProfileIntW(iniSettings.c_str(), L"Dinputtype", 15, iniPath.c_str());
    Etype = GetPrivateProfileIntW(iniSettings.c_str(), L"Einputtype", 27, iniPath.c_str());
    Ftype = GetPrivateProfileIntW(iniSettings.c_str(), L"Finputtype", 5, iniPath.c_str());
    uptype = GetPrivateProfileIntW(iniSettings.c_str(), L"Upkey", 52, iniPath.c_str());
    downtype = GetPrivateProfileIntW(iniSettings.c_str(), L"Downkey", 53, iniPath.c_str());
    lefttype = GetPrivateProfileIntW(iniSettings.c_str(), L"Leftkey", 54, iniPath.c_str());
    righttype = GetPrivateProfileIntW(iniSettings.c_str(), L"Rightkey", 55, iniPath.c_str());
    starttype = GetPrivateProfileIntW(iniSettings.c_str(), L"Startkey", 2, iniPath.c_str());
    backtype = GetPrivateProfileIntW(iniSettings.c_str(), L"Backkey", 1, iniPath.c_str());

    EnterCriticalSection(&deltaLock);
    keytodinput[0] = DIKcodes(Atype);
    keytodinput[1] = DIKcodes(Btype);
    keytodinput[2] = DIKcodes(Xtype);
    keytodinput[3] = DIKcodes(Ytype);
    keytodinput[4] = DIKcodes(Etype);
    keytodinput[5] = DIKcodes(Ftype);
    keytodinput[6] = DIKcodes(Ctype);
    keytodinput[7] = DIKcodes(Dtype);
    keytodinput[8] = DIKcodes(uptype);
    keytodinput[9] = DIKcodes(downtype);
    keytodinput[10] = DIKcodes(lefttype);
    keytodinput[11] = DIKcodes(righttype);
    //wasd
    keytodinput[12] = DIKcodes(47);
    keytodinput[13] = DIKcodes(43);
    keytodinput[14] = DIKcodes(25);
    keytodinput[15] = DIKcodes(28);
    //escape
    keytodinput[16] = DIKcodes(starttype); //start
    keytodinput[17] = DIKcodes(backtype); //back
    LeaveCriticalSection(&deltaLock);

    Sleep(1000);
    hwnd = GetMainWindowHandle(GetCurrentProcessId());
   // int mode = InitialMode;

    if (controllerID == -9999)
    {
        MessageBoxA(NULL, "Warning. Settings file Xinput.ini not read. All settings standard. Or maybe ControllerID is missing from ini", "Error", MB_OK | MB_ICONERROR);
        controllerID = 0; //default controller  

    }
    MessageBeep(MB_OK);
    while (loop == true)
    {
		
        bool movedmouse = false; //reset
        int calcsleep = 0;
        if (hwnd == NULL)
        {
            hwnd = GetMainWindowHandle(GetCurrentProcessId());
        }
        if (hwnd != NULL)
        {
            if (!resized)
            {
                if (resX != 0)
                { 
                    SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

                    SetWindowPos(hwnd, NULL, posX, posY, resX, resY, SWP_NOZORDER | SWP_NOACTIVATE);
                }
                resized = true;
			}
            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(XINPUT_STATE));
            // Check controller 0
            DWORD dwResult = XInputGetState(controllerID, &state);
            bool leftPressed = IsTriggerPressed(state.Gamepad.bLeftTrigger);
            bool rightPressed = IsTriggerPressed(state.Gamepad.bRightTrigger);




            if (dwResult == ERROR_SUCCESS)
            {
                // Controller is connected
                disabled = false;
                //for post message
                fakecursor.x = Xf; 
                fakecursor.y = Yf;
                ClientToScreen(hwnd, &fakecursor);
                EnterCriticalSection(&deltaLock);
                LeaveCriticalSection(&deltaLock);
                WORD buttons = state.Gamepad.wButtons;
                bool currA = (buttons & XINPUT_GAMEPAD_A) != 0;
                bool Apressed = (buttons & XINPUT_GAMEPAD_A);


                if (oldA == true)
                {
                    if (buttons & XINPUT_GAMEPAD_A) //hold
                    {
                        // keep posting?
                    }
                    else { //release
                        PostKeyFunction(hwnd, Atype, false);
                        oldA = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[0] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_A)
                {
                    oldA = true;
                    PostKeyFunction(hwnd, Atype, true);
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[0] = true;
                    LeaveCriticalSection(&deltaLock);
                }



                if (oldB == true)
                {
                    if (buttons & XINPUT_GAMEPAD_B)
                    {
                        // keep posting?
                    }
                    else {
                        PostKeyFunction(hwnd, Btype, false);
                        oldB = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[1] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_B)
                {
                    PostKeyFunction(hwnd, Btype, true);
                    oldB = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[1] = true;
                    LeaveCriticalSection(&deltaLock);
                }



                if (oldX == true)
                {
                    if (buttons & XINPUT_GAMEPAD_X)
                    {
                        // keep posting?
                    }
                    else {
                        PostKeyFunction(hwnd, Xtype, false);
                        oldX = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[2] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_X)
                {
                    oldX = true;
                    PostKeyFunction(hwnd, Xtype, true);
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[2] = true;
                    LeaveCriticalSection(&deltaLock);
                }



                if (oldY == true)
                {
                    if (buttons & XINPUT_GAMEPAD_Y)
                    {
                        // keep posting?
                    }
                    else {
                        oldY = false;
                        PostKeyFunction(hwnd, Ytype, false);
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[3] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_Y)
                {
                    oldY = true;
                    PostKeyFunction(hwnd, Ytype, true);
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[3] = true;
                    LeaveCriticalSection(&deltaLock);
                }



                if (oldC == true)
                {
                    if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) //2
                    {
                        // keep posting?
                        atick++;
                        if (atick > scrollenddelay)
                        {
                            EnterCriticalSection(&deltaLock);
                            Dmousehilo[2] = true; //dinputdevice will clear
                            LeaveCriticalSection(&deltaLock);
                            atick = 0;
                        }
                    }
                    else {
                        PostKeyFunction(hwnd, Ctype, false);
                        oldC = false;
                        atick = 0;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[6] = false;
                       // Dmousehilo[2] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
                {
                    PostKeyFunction(hwnd, Ctype, true);
                    oldC = true;
                    atick = 0;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[6] = true;
                    Dmousehilo[2] = true;
                    LeaveCriticalSection(&deltaLock);
                }



                if (oldD == true)
                {
                    if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) //3
                    {
                        // keep posting?
                        // keep posting?
                        atick++;
                        if (atick > scrollenddelay)
                        {
                            EnterCriticalSection(&deltaLock);
                            Dmousehilo[3] = true; //dinputdevice will clear
                            LeaveCriticalSection(&deltaLock);
                            atick = 0;
                        }
                    }
                    else {
                        oldD = false;
                        PostKeyFunction(hwnd, Dtype, false);
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[7] = false;
                        atick = 0;
                      //  Dmousehilo[3] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER)
                {
                    PostKeyFunction(hwnd, Dtype, true);
                    oldD = true;
                    atick = 0;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[7] = true;
                    Dmousehilo[3] = true;
                    LeaveCriticalSection(&deltaLock);
                }




                if (oldE == true)
                {
                    if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB)
                    {
                        // keep posting?

                    }
                    else {
                        PostKeyFunction(hwnd, Etype, false);
                        oldE = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[4] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB)
                {
                    PostKeyFunction(hwnd, Etype, true);
                    oldE = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[4] = true;
                    LeaveCriticalSection(&deltaLock);
                }



                if (oldF == true)
                {
                    if (buttons & XINPUT_GAMEPAD_LEFT_THUMB)
                    {
                        // keep posting?
                    }
                    else {
                        PostKeyFunction(hwnd, Ftype, false);
                        oldF = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[5] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_LEFT_THUMB)
                {
                    PostKeyFunction(hwnd, Ftype, true);
                    oldF = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[5] = true;
                    LeaveCriticalSection(&deltaLock);
                }




               if (oldup)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_UP)
                    {
                    }
                    else {
                        PostKeyFunction(hwnd, uptype, false);
                        oldup = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[8] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_UP)
                {
                    PostKeyFunction(hwnd, uptype, true);
                    oldup = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[8] = true;
                    LeaveCriticalSection(&deltaLock);
                }




                else if (olddown)
                {
                   
                    if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
                    {
                    }
                    else {
                        PostKeyFunction(hwnd, downtype, false);
                        Dkeyhilo[9] = false;
                        olddown = false;
                    }
                    
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
                {
                   PostKeyFunction(hwnd, downtype, true);
                   olddown = true;
                   EnterCriticalSection(&deltaLock);
                   Dkeyhilo[9] = true;
                   LeaveCriticalSection(&deltaLock);
                }





                if (oldleft)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
                    {
                        //post keep?
                    }
                    else {
                        PostKeyFunction(hwnd, lefttype, false);
                        oldleft = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[10] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
                {
                    PostKeyFunction(hwnd, lefttype, true);
                    oldleft = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[10] = true;
                    LeaveCriticalSection(&deltaLock);
                }





                if (oldright)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
                    {
                        //post keep?
                    }
                    else {
                        PostKeyFunction(hwnd, righttype, false);
                        oldright = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[11] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
                {
                    PostKeyFunction(hwnd, righttype, true);
                    oldright = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[11] = true;
                    LeaveCriticalSection(&deltaLock);
                    
                }

                if (oldback) {
                    if (buttons & XINPUT_GAMEPAD_BACK) {
                        // hold?
                    }
                    else {
                        //release
                        oldback = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[17] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_BACK)
                { //on press
                    oldback = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[17] = true;
                    LeaveCriticalSection(&deltaLock);
				}

                if (buttons & XINPUT_GAMEPAD_START)
                {
                    Sleep(100);
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[16] = true;
                    LeaveCriticalSection(&deltaLock);
                    
                    // Sleep(1000); //have time to release button. this is no hurry anyway

                }
                else if (!(buttons & XINPUT_GAMEPAD_START) && Dkeyhilo[16] == true)
                {
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[16] = false;
                    LeaveCriticalSection(&deltaLock);
				}
                    //sticks
                    int Xaxis = 0;
                    int Yaxis = 0;
                    int scrollXaxis = 0;
                    int scrollYaxis = 0;

                    if (righthanded == 1) {
                        Xaxis = state.Gamepad.sThumbRX;
                        Yaxis = state.Gamepad.sThumbRY;
                        scrollXaxis = state.Gamepad.sThumbLX;
                        scrollYaxis = state.Gamepad.sThumbLY;
                    }
                    else
                    {
                        Xaxis = state.Gamepad.sThumbLX;
                        Yaxis = state.Gamepad.sThumbLY;
                        scrollXaxis = state.Gamepad.sThumbRX;
                        scrollYaxis = state.Gamepad.sThumbRY;
                    }

                        if (oldscrollleftaxis)
                        {
                            if (scrollXaxis < AxisLeftsens) //left
                            {
                            }
                            else
                            { //stop
                                oldscrollleftaxis = false;
                                    EnterCriticalSection(&deltaLock);
                                    Dkeyhilo[14] = false;
                                    LeaveCriticalSection(&deltaLock);
                            }
                        }
                        else if (scrollXaxis < AxisLeftsens) //left
                        {
                            EnterCriticalSection(&deltaLock);
                            Dkeyhilo[14] = true;
                            LeaveCriticalSection(&deltaLock);
                            oldscrollleftaxis = true;
                        }



                        if (oldscrollrightaxis)
                        {
                            if (scrollXaxis > AxisRightsens) //right
                            {
                            }
                            else {
                               oldscrollrightaxis = false;
                               EnterCriticalSection(&deltaLock);
                               Dkeyhilo[15] = false;
                               LeaveCriticalSection(&deltaLock);
                            }
                        }
                        else if (scrollXaxis > AxisRightsens) //right
                        {
                            EnterCriticalSection(&deltaLock);
                            Dkeyhilo[15] = true;
                            LeaveCriticalSection(&deltaLock);
                            oldscrollrightaxis = true;

                        }



                        if (oldscrolldownaxis)
                        {
                            if (scrollYaxis < AxisDownsens)
                            {
                            }
                            else {
                                oldscrolldownaxis = false;
                                EnterCriticalSection(&deltaLock);
                                Dkeyhilo[13] = false;
                                LeaveCriticalSection(&deltaLock);
                            }
                        }
                        else if (scrollYaxis < AxisDownsens) //down
                        { //start
                            EnterCriticalSection(&deltaLock);
                            Dkeyhilo[13] = true;
                            LeaveCriticalSection(&deltaLock);
                            oldscrolldownaxis = true;
                        }





                        if (oldscrollupaxis)
                        {
                            if (scrollYaxis > AxisUpsens)
                            {
                            }
                            else {
                                oldscrollupaxis = false;
                                    EnterCriticalSection(&deltaLock);
                                    Dkeyhilo[12] = false;
                                    LeaveCriticalSection(&deltaLock);
                            }
                        }
                        else if (scrollYaxis > AxisUpsens) //up
                        {
                                EnterCriticalSection(&deltaLock);
                                Dkeyhilo[12] = true;
                                LeaveCriticalSection(&deltaLock);
                            oldscrollupaxis = true;
                        }
                    

                    //sleeptime adjust. slow movement on low axis
                    if (Xaxis < 0) //negative
                    {
                        if (Xaxis < -22000) //-7849
                            calcsleep = 3;
                        else if (Xaxis < -15000)
                            calcsleep = 2;
                        else if (Xaxis < -10000)
                            calcsleep = 1;
                        else calcsleep = 0;
                    }
                    else if (Xaxis > 0) //positive
                    {
                        if (Xaxis > 24000) //12000
                            calcsleep = 3;
                        else if (Xaxis > 18000)
                            calcsleep = 2;
                        else if (Xaxis > 14000)
                            calcsleep = 1;
                        else calcsleep = 0;
                    }
                    if (Yaxis < 0) //negative
                    {
                        if (Yaxis < -24000) //-16000
                            calcsleep = 3;
                        else if (Yaxis < -22000)
                            calcsleep = 2;
                        else if (Yaxis < -17000)
                            calcsleep = 1;
                        else calcsleep = 0;
                    }
                    else if (Yaxis > 0) //positive
                    {
                        if (Yaxis > 24000) //0
                            calcsleep = 3;
                        else if (Yaxis > 20000)
                            calcsleep = 2;
                        else if (Yaxis > 16000)
                            calcsleep = 1;
                        else calcsleep = 0;
                    }
                    
                    EnterCriticalSection(&deltaLock);
                        delta = CalculateUltimateCursorMove(
                        Xaxis, Yaxis,
                        radial_deadzone, axial_deadzone, max_threshold,
                        curve_slope, curve_exponent,
                        sensitivity, accel_multiplier
                    );
                    

                    if (delta.x != 0 || delta.y != 0) {
                        Xf += delta.x;
                        Yf += delta.y;
                        movedmouse = true;
                    }
                    LeaveCriticalSection(&deltaLock);

                    if (leftPressed)
                    {

                        if (leftPressedold == false)
                        {
                            leftPressedold = true;
                            SendMouseClick(fakecursor.x, fakecursor.y, 5, 2); //4 skal vere 3
                        }
                    }
                    if (leftPressedold)
                    {
                        if (!leftPressed)
                        {
                            SendMouseClick(fakecursor.x, fakecursor.y, 6, 2); //double click
                            leftPressedold = false;
                        }
                    }
                    if (rightPressed)
                    {
                        if (rightPressedold == false)
                        {
                            //start
                            rightPressedold = true;
                            if (userealmouse == 0)
                            {
                                SendMouseClick(fakecursor.x, fakecursor.y, 3, 2); //4 skal vere 3

                            }
                        }


                    }
                    if (rightPressedold)
					{
						if (vibrator < 20000)
                        { 
                            vibrator += 10;
                        }
                        vibrateController(controllerID, vibrator);
                        if (!rightPressed)
                        {
                            vibrator = 0;
                            vibrateController(controllerID, vibrator);
                            SendMouseClick(fakecursor.x, fakecursor.y, 4, 2);
                            rightPressedold = false;
                        }
                    } //rightpress
                ScreenToClient(hwnd, &fakecursor);
            } //no controller
            else if (!disabled)
            {
                char message[256];
                sprintf(message, "Current ID: %d\nTRY AGAIN to retry ID\nCONTINUE to change ID\n\nCANCEL for automatic search", controllerID);
                int result = MessageBoxA(
                    NULL,
                    message,
                    "Error, controller Disconnected. Try again?",
                    MB_CANCELTRYCONTINUE | MB_ICONEXCLAMATION
                );
                    switch (result) {
                    case IDCANCEL:
                    {
						disabled = true;
                        break;
                    }
                    case IDTRYAGAIN: 
                    {
                        break;
                    }
                    case IDCONTINUE:
                    {
						if (controllerID < 4) controllerID++;  
						else controllerID = 0;
                        break;
                    }
                    default:
                    {
                        MessageBoxA(NULL, "Unexpected result.", "Error", MB_OK | MB_ICONERROR);
                        break;
                    }
                    }
               }
               if (disabled) {
                   //keep polling all IDs until one is found
                    if (controllerID < 4) controllerID++;
                    else controllerID = 0;
               }
            
        } // no hwnd
        if (knappsovetid > 20)
        {
            //  sovetid = 20;
            //  knappsovetid = 100;

        }
        //ticks for scroll end delay
        if (tick < scrollenddelay)
            tick++;
        if (mode == 0)
            Sleep(10);
        if (mode > 0) {
            // Sleep(sovetid); //15-80 //ini value
            if (movedmouse == true)
                Sleep(1 + responsetime); //max 3. 0-2 on slow movement - calcsleep
            else Sleep(2); //max 3. 0-2 on slow movement
        }
    } //loop end but endless
    return 0;
}

bool inither = false;
HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter)
{
	if (!m_pDirectInput8Create)
	{
		return E_FAIL;
	}
    if (!inither)
    { 
	    CreateThread(nullptr, 0,
		(LPTHREAD_START_ROUTINE)ThreadFunction, GetModuleHandle(0), 0, 0); //GetModuleHandle(0)
        inither = true;
    }
	HRESULT hr = m_pDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	if (SUCCEEDED(hr))
	{
		genericQueryInterface(riidltf, ppvOut);
	}

	return hr;
}

HRESULT WINAPI DllCanUnloadNow()
{
	if (!m_pDllCanUnloadNow)
	{
		return E_FAIL;
	}

	return m_pDllCanUnloadNow();
}

HRESULT WINAPI DllGetClassObject(IN REFCLSID rclsid, IN REFIID riid, OUT LPVOID FAR* ppv)
{
	if (!m_pDllGetClassObject)
	{
		return E_FAIL;
	}

	HRESULT hr = m_pDllGetClassObject(rclsid, riid, ppv);

	if (SUCCEEDED(hr))
	{
		genericQueryInterface(riid, ppv);
	}

	return hr;
}

HRESULT WINAPI DllRegisterServer()
{
	if (!m_pDllRegisterServer)
	{
		return E_FAIL;
	}

	return m_pDllRegisterServer();
}

HRESULT WINAPI DllUnregisterServer()
{
	if (!m_pDllUnregisterServer)
	{
		return E_FAIL;
	}

	return m_pDllUnregisterServer();
}

LPCDIDATAFORMAT WINAPI GetdfDIJoystick()
{
	if (!m_pGetdfDIJoystick)
	{
		return nullptr;
	}

	return m_pGetdfDIJoystick();
}
