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
//#include <windowsx.h>

#include <Xinput.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include <cstdio>  // for swprintf
#include <psapi.h>
#include <string>
#include <cstdlib> // For strtoul
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")


#pragma comment(lib, "Xinput9_1_0.lib")

HMODULE g_hModule = nullptr;

typedef BOOL(WINAPI* GetCursorPos_t)(LPPOINT lpPoint);
typedef BOOL(WINAPI* SetCursorPos_t)(int X, int Y);

typedef SHORT(WINAPI* GetAsyncKeyState_t)(int vKey);
typedef SHORT(WINAPI* GetKeyState_t)(int nVirtKey);
typedef BOOL(WINAPI* ClipCursor_t)(const RECT*);
typedef HCURSOR(WINAPI* SetCursor_t)(HCURSOR hCursor);

typedef BOOL(WINAPI* SetRect_t)(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
typedef BOOL(WINAPI* AdjustWindowRect_t)(LPRECT lprc, DWORD  dwStyle, BOOL bMenu);





GetCursorPos_t fpGetCursorPos = nullptr;
GetCursorPos_t fpSetCursorPos = nullptr;
GetAsyncKeyState_t fpGetAsyncKeyState = nullptr;
GetKeyState_t fpGetKeyState = nullptr;
ClipCursor_t fpClipCursor = nullptr;
SetCursor_t fpSetCursor = nullptr;
SetRect_t fpSetRect = nullptr;
AdjustWindowRect_t fpAdjustWindowRect = nullptr;



POINT fakecursor;
POINT startdrag;
POINT activatewindow;
POINT scroll;
bool loop = true;
HWND hwnd;
int showmessage = 0; //0 = no message, 1 = initializing, 2 = bmp mode, 3 = bmp and cursor mode, 4 = edit mode   
int counter = 0;

//syncronization control
HANDLE hMutex;

int getmouseonkey = 0;
int message = 0;


//hooks
bool hooksinited = false;
int keystatesend = 0; //key to send
int clipcursorhook = 0;
int getkeystatehook = 0;
int getasynckeystatehook = 0;
int getcursorposhook = 0;
int setcursorposhook = 0;
int setcursorhook = 0;

int ignorerect = 0;
POINT rectignore = { 0,0 }; //for getcursorposhook
int setrecthook = 0;

int leftrect = 0;
int toprect = 0;
int rightrect = 0;
int bottomrect = 0;

int userealmouse = 0;
int atick = 0;


//fake cursor
int controllerID = 0;
int Xf = 20;
int Yf = 20;
int OldX = 0;
int OldY = 0;
int ydrag;
int xdrag;
int Xoffset = 0; //offset for cursor    
int Yoffset = 0;
bool scrollmap = false;
bool pausedraw = false;
bool gotcursoryet = false;
int drawfakecursor = 0;
int alwaysdrawcursor = 0; //always draw cursor even if setcursor set cursor NULL
HICON hCursor = 0;
DWORD lastClickTime;

WORD vibrator = 0;
//mousemove calc
// #define DEADZONE 8000
// #define MAX_SPEED 30.0f        // Maximum pixels per poll
// #define ACCELERATION 2.0f      // Controls non-linear ramp (higher = more acceleration)
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
bool Dmousehilo[4];

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
bool Dkeyhilo[17];// byte[] keytodinput
unsigned char keytodinput[17] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//CRITICAL_SECTION keytodinputLock;  // Definition and storage
//bool statehilo{ 8 }; //2: set to false on new keys. then getstate read them and set back to true 
//CRITICAL_SECTION statehiloLock;  // Definition and storage

//int keytodinput[8] = { DIK_A, DIK_B, DIK_C, DIK_D, DIK_E, DIK_F, DIK_G, DIK_H };
//2: bytes set to correct keys on init 






//bmp search
bool foundit = false;


//scroll type 3
int tick = 0;
bool doscrollyes = false;

// 
//beautiful cursor
int colorfulSword[20][20] = {
{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,1,1,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,1,0,0,1,2,2,2,2,1,0,0,0,0,0},
{1,2,2,2,2,1,0,0,0,0,1,2,2,2,1,0,0,0,0,0},
{1,1,2,2,1,0,0,0,0,0,0,1,2,2,2,1,0,0,0,0},
{1,2,2,1,0,0,0,0,0,0,0,1,2,2,2,1,0,0,0,0},
{1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0},
};
//temporary cursor on success

COLORREF colors[5] = {
    RGB(0, 0, 0),          // Transparent - won't be drawn
    RGB(140, 140, 140),    // Gray for blade
    RGB(255, 255, 255),    // White shine
    RGB(139, 69, 19),       // Brown handle
    RGB(50, 150, 255)     // Light blue accent

};


bool onoroff = true;

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

int startsearch = 0;
int startsearchA = 0;
int startsearchB = 0;
int startsearchX = 0;
int startsearchY = 0;
int startsearchC = 0;
int startsearchD = 0;
int startsearchE = 0;
int startsearchF = 0;

int righthanded = 0;


int Atype = 0;
int Btype = 0;
int Xtype = 0;
int Ytype = 0;
int Ctype = 0;
int Dtype = 0;
int Etype = 0;
int Ftype = 0;

int bmpAtype = 0;
int bmpBtype = 0;
int bmpXtype = 0;
int bmpYtype = 0;
int bmpCtype = 0;
int bmpDtype = 0;
int bmpEtype = 0;
int bmpFtype = 0;

int uptype = 0;
int downtype = 0;
int lefttype = 0;
int righttype = 0;


int x = 0;

HBITMAP hbm;

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
//std::string UGetExecutableFolder() {
    //TCHAR paths[MAX_PATH];
    //GetModuleFileName(NULL, paths, MAX_PATH);
    //std::string exePath(paths);

    // Remove the executable name to get the folder
  ///  size_t lastSlash = exePath.find_last_of("\\/");
 //   return exePath.substr(0, lastSlash);
//}



HCURSOR WINAPI HookedSetCursor(HCURSOR hcursor) {
    hCursor = hcursor; // Store the cursor handle  

    hcursor = fpSetCursor(hcursor);
    return hcursor;
}




////SetRect_t)(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
BOOL WINAPI HookedSetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
    xLeft = leftrect; // Set the left coordinate to Xrect  
    yTop = toprect; // Set the top coordinate to Yrect  

    xRight = rightrect; // Set the right coordinate to Xrect + 10 
    yBottom = bottomrect; // Set the bottom coordinate to Yrect + 10    


    bool result = fpSetRect(lprc, xLeft, yTop, xRight, yBottom);
    return result;
}

BOOL WINAPI HookedAdjustWindowRect(LPRECT lprc, DWORD  dwStyle, BOOL bMenu) {
    lprc->top = toprect; // Set the left coordinate to Xrect  
    lprc->bottom = bottomrect; // Set the left coordinate to Xrect  
    lprc->left = leftrect; // Set the left coordinate to Xrect  
    lprc->right = rightrect; // Set the left coordinate to Xrect  

    bool result = fpAdjustWindowRect(lprc, dwStyle, bMenu);
    return result;
}




SHORT WINAPI HookedGetAsyncKeyState(int vKey)
{

    if (samekeyA == vKey) {
        return 8001;
        //8001 on hold key. but this may not work
    }
    else samekeyA = 0;

    if (vKey == keystatesend)
    {
        samekeyA = vKey;
        return 8000; //8001 ?
    }
    else
    {
        SHORT result = fpGetAsyncKeyState(vKey);
        return result;
    }
}

// Hooked GetKeyState
SHORT WINAPI HookedGetKeyState(int nVirtKey) {
    if (samekey == nVirtKey) {
        return 8001;
        //8001 on hold key. but this may not work
    }
    else samekey = 0;

    if (nVirtKey == keystatesend)
    {
        samekey = nVirtKey;
        return 8000; //8001 ?
    }
    else
    {
        SHORT result = fpGetKeyState(nVirtKey);
        return result;
    }
}

BOOL WINAPI MyGetCursorPos(PPOINT lpPoint) {
    if (lpPoint)
    {
        POINT mpos;
        if (scrollmap == false)
        {

            if (ignorerect == 1) {
                mpos.x = Xf + rectignore.x; //hwnd coordinates 0-800 on a 800x600 window
                mpos.y = Yf + rectignore.y;//hwnd coordinate s0-600 on a 800x600 window
                lpPoint->x = mpos.x;
                lpPoint->y = mpos.y;
            }
            else {
                mpos.x = Xf; //hwnd coordinates 0-800 on a 800x600 window
                mpos.y = Yf;//hwnd coordinate s0-600 on a 800x600 window
                ClientToScreen(hwnd, &mpos);

                lpPoint->x = mpos.x; //desktop coordinates
                lpPoint->y = mpos.y;
                ScreenToClient(hwnd, &mpos); //revert so i am sure its done
            }
        }

        else
        {
            mpos.x = scroll.x;
            mpos.y = scroll.y;
            ClientToScreen(hwnd, &mpos);
            lpPoint->x = mpos.x;
            lpPoint->y = mpos.y;

            ScreenToClient(hwnd, &mpos);
        }
        return true;
    }
    return false;
}
POINT mpos;
BOOL WINAPI MySetCursorPos(int X, int Y) {
    POINT point;
    point.x = X;
    point.y = Y;
    char buffer[256];


    ScreenToClient(hwnd, &point);
    sprintf_s(buffer, "X: %d Y: %d", point.x, point.y);
    Xf = point.x; // Update the global X coordinate
    Yf = point.y; // Update the global Y coordinate

    //MessageBoxA(NULL, buffer, "Info", MB_OK | MB_ICONINFORMATION);
   // movedmouse = true;
    //crash fixme!
  //  Sleep(20);
    return true; //fpSetCursorPos(lpPoint); // Call the original SetCursorPos function
}
BOOL WINAPI HookedClipCursor(const RECT* lpRect) {
    return true; //nonzero bool or int
    //return originalClipCursor(nullptr);

}

bool Mutexlock(bool lock) {
    // Create a named mutex
    if (lock == true)
    {
        hMutex = CreateMutexA(
            NULL,      // Default security
            FALSE,     // Initially not owned
            "Global\\PuttingInputByMessenils" // Name of mutex
        );
        if (hMutex == NULL) {
            std::cerr << "CreateMutex failed: " << GetLastError() << std::endl;
            MessageBox(NULL, L"Error!", L"Failed to create mutex", MB_OK | MB_ICONINFORMATION);
            return false;
        }
        // Check if mutex already exists
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            Sleep(5);
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
            Mutexlock(true); //is this okay?

        }
    }
    if (lock == false)
    {
        ReleaseMutex(hMutex);

        CloseHandle(hMutex);
        // hMutex = nullptr; // Optional: Prevent dangling pointer

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

bool SendMouseClick(int x, int y, int z, int many) {
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

int CalculateStride(int width) {
    return ((width * 3 + 3) & ~3);
}

bool Save24BitBMP(const wchar_t* filename, const BYTE* pixels, int width, int height) { //for testing purposes
    int stride = ((width * 3 + 3) & ~3);
    int imageSize = stride * height;

    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;  // "BM"
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + imageSize;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height;  // bottom-up BMP (positive height)
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = imageSize;

    HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written;
    WriteFile(hFile, &bfh, sizeof(bfh), &written, NULL);
    WriteFile(hFile, &bih, sizeof(bih), &written, NULL);
    WriteFile(hFile, pixels, imageSize, &written, NULL);
    CloseHandle(hFile);

    return true;
}

bool IsTriggerPressed(BYTE triggerValue) {
    BYTE threshold = 175;
    return triggerValue > threshold;
}

HBITMAP CaptureWindow24Bit(HWND hwnd, SIZE& capturedwindow, std::vector<BYTE>& pixels, int& strideOut, bool draw) {
    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);


    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;
    capturedwindow.cx = width;
    capturedwindow.cy = height;

    int stride = ((width * 3 + 3) & ~3);
    strideOut = stride;
    pixels.resize(stride * height);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE* pBits = nullptr;



    HBITMAP hbm24 = CreateDIBSection(hdcWindow, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    if (hbm24 != 0)
    {

        HGDIOBJ oldBmp = SelectObject(hdcMem, hbm24);

        // Copy window contents to memory DC
        BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

        if (draw) {
            RECT rect = { 0, 0, 32, 32 }; //need bmp width height
            FillRect(hdcMem, &rect, (HBRUSH)(COLOR_WINDOW + 1));

            if (showmessage == 1)
            {
                TextOut(hdcWindow, Xf, Yf, TEXT("BMP MODE"), 8);
                TextOut(hdcWindow, Xf, Yf + 17, TEXT("only mapping searches"), 21);
            }
            else if (showmessage == 2)
            {
                TextOut(hdcWindow, Xf, Yf, TEXT("CURSOR MODE"), 11);
                TextOut(hdcWindow, Xf, Yf + 17, TEXT("mapping searches + cursor"), 25);
            }
            else if (showmessage == 3)
            {
                TextOut(hdcWindow, Xf, Yf, TEXT("EDIT MODE"), 9);
                TextOut(hdcWindow, Xf, Yf + 15, TEXT("tap a button to bind it to coordinate"), 37);
                TextOut(hdcWindow, Xf, Yf + 30, TEXT("A,B,X,Y,R2,R3,L2,L3 can be mapped"), 32);
            }
            else if (showmessage == 10)
            {
                TextOut(hdcWindow, Xf, Yf, TEXT("BUTTON MAPPED"), 13);
            }
            else if (showmessage == 11)
            {
                TextOut(hdcWindow, Xf, Yf, TEXT("WAIT FOR MESSAGE EXPIRE!"), 24);
            }
            else if (showmessage == 12)
            {
                TextOut(hdcWindow, 20, 20, TEXT("DISCONNECTED!"), 14); //14
            }
            else if (showmessage == 69)
            {
                TextOut(hdcWindow, Xf, Yf, TEXT("SHUTTING DOWN"), 13);
            }
            else if (showmessage == 70)
            {
                TextOut(hdcWindow, Xf, Yf, TEXT("STARTING!"), 10);
            }
            else if (hCursor != 0 && onoroff == true)
            {
                gotcursoryet = true;
                if (Xf - Xoffset < 0 || Yf - Yoffset < 0)
                    DrawIconEx(hdcWindow, 0 + Xf, 0 + Yf, hCursor, 32, 32, 0, NULL, DI_NORMAL);//need bmp width height
                else
                    DrawIconEx(hdcWindow, Xf - Xoffset, Yf - Yoffset, hCursor, 32, 32, 0, NULL, DI_NORMAL);//need bmp width height

            }
            else if (onoroff == true && (alwaysdrawcursor == 1 || gotcursoryet == false))
            {
                for (int y = 0; y < 20; y++)
                {
                    for (int x = 0; x < 20; x++)
                    {
                        int val = colorfulSword[y][x];
                        if (val != 0)
                        {
                            HBRUSH hBrush = CreateSolidBrush(colors[val]);
                            RECT rect = { Xf + x , Yf + y , Xf + x + 1, Yf + y + 1 };
                            FillRect(hdcWindow, &rect, hBrush);
                            DeleteObject(hBrush);
                        }
                    }
                }
            }


            GetDIBits(hdcMem, hbm24, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
            SelectObject(hdcMem, oldBmp);;
            if (hdcMem) DeleteDC(hdcMem);
            if (hdcWindow) ReleaseDC(hwnd, hdcWindow);
            if (hbm24) DeleteObject(hbm24);

            return 0;
        }

        // Copy bits out
        GetDIBits(hdcMem, hbm24, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
        SelectObject(hdcMem, oldBmp);
        if (hdcMem) DeleteDC(hdcMem);
        if (hdcWindow) ReleaseDC(hwnd, hdcWindow);
        if (hbm24) DeleteObject(hbm24);
        return hbm24 ? hbm24 : 0;
    } //hbm24 not null
    return 0; // Failed to create bitmap    
} //function end
// Helper: Get stick magnitude
float GetStickMagnitude(SHORT x, SHORT y) {
    return sqrtf(static_cast<float>(x) * x + static_cast<float>(y) * y);
}

// Helper: Clamp value to range [-1, 1]
float Clamp(float v) {
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
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
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
    Sleep(2000);
    wchar_t buffer[256];
	bool resized = false;

    // settings reporting
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

    Xoffset = GetPrivateProfileIntW(iniSettings.c_str(), L"Xoffset", 0, iniPath.c_str());
    Yoffset = GetPrivateProfileIntW(iniSettings.c_str(), L"Yoffset", 0, iniPath.c_str());

    //mode
    int InitialMode = GetPrivateProfileIntW(iniSettings.c_str(), L"Initial Mode", 1, iniPath.c_str());
    int Modechange = GetPrivateProfileIntW(iniSettings.c_str(), L"Allow modechange", 0, iniPath.c_str());

    int sendfocus = GetPrivateProfileIntW(iniSettings.c_str(), L"Sendfocus", 0, iniPath.c_str());
    int responsetime = GetPrivateProfileIntW(iniSettings.c_str(), L"Responsetime", 0, iniPath.c_str());
    int doubleclicks = GetPrivateProfileIntW(iniSettings.c_str(), L"Doubleclicks", 0, iniPath.c_str());
    int scrollenddelay = GetPrivateProfileIntW(iniSettings.c_str(), L"DelayEndScroll", 0, iniPath.c_str());
    int quickMW = GetPrivateProfileIntW(iniSettings.c_str(), L"MouseWheelContinous", 1, iniPath.c_str());

    //clicknotmove 2
    //movenotclick 1
    Atype = GetPrivateProfileIntW(iniSettings.c_str(), L"Ainputtype", 0, iniPath.c_str());
    Btype = GetPrivateProfileIntW(iniSettings.c_str(), L"Binputtype", 0, iniPath.c_str());
    Xtype = GetPrivateProfileIntW(iniSettings.c_str(), L"Xinputtype", 1, iniPath.c_str());
    Ytype = GetPrivateProfileIntW(iniSettings.c_str(), L"Yinputtype", 0, iniPath.c_str());
    Ctype = GetPrivateProfileIntW(iniSettings.c_str(), L"Cinputtype", 2, iniPath.c_str());
    Dtype = GetPrivateProfileIntW(iniSettings.c_str(), L"Dinputtype", 0, iniPath.c_str());
    Etype = GetPrivateProfileIntW(iniSettings.c_str(), L"Einputtype", 0, iniPath.c_str());
    Ftype = GetPrivateProfileIntW(iniSettings.c_str(), L"Finputtype", 0, iniPath.c_str());

    uptype = GetPrivateProfileIntW(iniSettings.c_str(), L"Upkey", -1, iniPath.c_str());
    downtype = GetPrivateProfileIntW(iniSettings.c_str(), L"Downkey", -2, iniPath.c_str());
    lefttype = GetPrivateProfileIntW(iniSettings.c_str(), L"Leftkey", -3, iniPath.c_str());
    righttype = GetPrivateProfileIntW(iniSettings.c_str(), L"Rightkey", -4, iniPath.c_str());

    //hooks
    drawfakecursor = GetPrivateProfileIntW(iniSettings.c_str(), L"DrawFakeCursor", 0, iniPath.c_str());
    alwaysdrawcursor = GetPrivateProfileIntW(iniSettings.c_str(), L"DrawFakeCursorAlways", 0, iniPath.c_str());
    userealmouse = GetPrivateProfileIntW(iniSettings.c_str(), L"UseRealMouse", 0, iniPath.c_str());   //scrolloutsidewindow
    ignorerect = GetPrivateProfileIntW(iniSettings.c_str(), L"IgnoreRect", 0, iniPath.c_str());

    int scrolloutsidewindow = GetPrivateProfileIntW(iniSettings.c_str(), L"Scrollmapfix", 4, iniPath.c_str());   //scrolloutsidewindow

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
    keytodinput[16] = DIKcodes(1);
    LeaveCriticalSection(&deltaLock);

    Sleep(1000);
    hwnd = GetMainWindowHandle(GetCurrentProcessId());
    int mode = InitialMode;
    int numphotoA = -1;
    int numphotoB = -1;
    int numphotoX = -1;
    int numphotoY = -1;
    int numphotoC = -1;
    int numphotoD = -1;
    int numphotoE = -1;
    int numphotoF = -1;

    if (controllerID == -9999)
    {
        MessageBoxA(NULL, "Warning. Settings file Xinput.ini not read. All settings standard. Or maybe ControllerID is missing from ini", "Error", MB_OK | MB_ICONERROR);
        controllerID = 0; //default controller  

    }
   // HBITMAP hbmdsktop = NULL;
    ///////////////////////////////////////////////////////////////////////////////////LLLLLLLOOOOOOOOOOOOOPPPPPPPPPPPPP
    bool Aprev = false;

    while (loop == true)
    {
        bool movedmouse = false; //reset
        int calcsleep = 0;
        if (hwnd == NULL)
        {
            hwnd = GetMainWindowHandle(GetCurrentProcessId());
        }
        RECT rect;
        GetClientRect(hwnd, &rect);
        if (ignorerect == 1)
        {
            RECT frameBounds;
            HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frameBounds, sizeof(frameBounds));
            if (SUCCEEDED(hr)) {
                // These are the actual visible edges of the window in client coordinates
                POINT upper;
                upper.x = frameBounds.left;
                upper.y = frameBounds.top;

                //used in getcursrorpos
                rectignore.x = upper.x;
                rectignore.y = upper.y;

                rect.right = frameBounds.right - frameBounds.left;
                rect.bottom = frameBounds.bottom - frameBounds.top;
                rect.left = 0;
                rect.top = 0;

            }
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

                fakecursor.x = Xf;
                fakecursor.y = Yf;
                ClientToScreen(hwnd, &fakecursor);
                // Controller is connected
                WORD buttons = state.Gamepad.wButtons;
                bool currA = (buttons & XINPUT_GAMEPAD_A) != 0;
                bool Apressed = (buttons & XINPUT_GAMEPAD_A);

                if (showmessage == 12) //was disconnected?
                {
                    showmessage = 0;
                }


                if (oldA == true)
                {
                    if (buttons & XINPUT_GAMEPAD_A && onoroff == true) //hold
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
                else if (buttons & XINPUT_GAMEPAD_A && onoroff == true)
                {
                    oldA = true;
                    PostKeyFunction(hwnd, Atype, true);
                    if (foundit == false)
                    {
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[0] = true;
                        LeaveCriticalSection(&deltaLock);
                    }
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoA++;
                        Sleep(500);
                    }
                }



                if (oldB == true)
                {
                    if (buttons & XINPUT_GAMEPAD_B && onoroff == true)
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
                else if (buttons & XINPUT_GAMEPAD_B && onoroff == true)
                {

                    PostKeyFunction(hwnd, Btype, true);
                    oldB = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[1] = true;
                    LeaveCriticalSection(&deltaLock);
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoB++;
                        Sleep(500);
                    }

                }



                if (oldX == true)
                {
                    if (buttons & XINPUT_GAMEPAD_X && onoroff == true)
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
                else if (buttons & XINPUT_GAMEPAD_X && onoroff == true)
                {
                    oldX = true;
                    PostKeyFunction(hwnd, Xtype, true);
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[2] = true;
                    LeaveCriticalSection(&deltaLock);
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoX++;
                        Sleep(500);
                    }
                }



                if (oldY == true)
                {
                    if (buttons & XINPUT_GAMEPAD_Y && onoroff == true)
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
                else if (buttons & XINPUT_GAMEPAD_Y && onoroff == true)
                {
                    oldY = true;
                    PostKeyFunction(hwnd, Ytype, true);
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[3] = true;
                    LeaveCriticalSection(&deltaLock);
                }



                if (oldC == true)
                {
                    if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        PostKeyFunction(hwnd, Ctype, false);
                        oldC = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[6] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER && onoroff == true)
                {
                    PostKeyFunction(hwnd, Ctype, true);
                    oldC = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[6] = true;
                    LeaveCriticalSection(&deltaLock);
                    if (mode == 2 && showmessage == 0)
                    {
                        numphotoC++;
                        Sleep(500);
                    }
                }



                if (oldD == true)
                {
                    if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldD = false;
                        PostKeyFunction(hwnd, Dtype, false);
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[7] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER && onoroff == true)
                {
                    PostKeyFunction(hwnd, Dtype, true);
                    oldD = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[7] = true;
                    LeaveCriticalSection(&deltaLock);
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoD++;
                        Sleep(500);
                    }
                }




                if (oldE == true)
                {
                    if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB && onoroff == true)
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
                else if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB && onoroff == true)
                {
                    PostKeyFunction(hwnd, Etype, true);
                    oldE = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[4] = true;
                    LeaveCriticalSection(&deltaLock);
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoE++;
                        Sleep(500);
                    }
                }



                if (oldF == true)
                {
                    if (buttons & XINPUT_GAMEPAD_LEFT_THUMB && onoroff == true)
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
                else if (buttons & XINPUT_GAMEPAD_LEFT_THUMB && onoroff == true)
                {
                    PostKeyFunction(hwnd, Ftype, true);
                    oldF = true;
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[5] = true;
                    LeaveCriticalSection(&deltaLock);
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoF++;
                        Sleep(500);
                    }
                }




               if (oldup)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_UP && onoroff == true)
                    {
                        //post keep?
                        if (scrolloutsidewindow >= 3 && quickMW == 1) {
                            ClientToScreen(hwnd, &fakecursor); //double
                            SendMouseClick(fakecursor.x, fakecursor.y, 20, 1);
                            ScreenToClient(hwnd, &fakecursor);
							//for sending mouse wheel. should work, but i went for combikey 5-8 instead
                            // all comination keys are in Inputdevice.cpp now
                           // atick++;
                           // if (atick > 400)
                           // {
                            //    EnterCriticalSection(&deltaLock);
                            //    if (Dmousehilo[1])
                            //    {
//

                            //        Dmousehilo[2] == true; //dinputdevice will clear
                            //        atick = 0;
                            //    }
                            //    LeaveCriticalSection(&deltaLock);

                           // }
                        }
                    }
                    else {
                        PostKeyFunction(hwnd, uptype, false);
                        oldup = false;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[8] = false;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_UP && onoroff == true)
                {
                    PostKeyFunction(hwnd, uptype, true);

                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[8] = true;
                    LeaveCriticalSection(&deltaLock);

                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    if (scrolloutsidewindow == 0)
                        scroll.y = rect.top + 1;
                    if (scrolloutsidewindow == 1)
                        scroll.y = rect.top - 1;
                    scrollmap = true;
                    if (scrolloutsidewindow == 2) {
                        oldup = true;
                    }
                    if (scrolloutsidewindow >= 3) {
                        oldup = true;
                        scrollmap = false;
                        ClientToScreen(hwnd, &fakecursor); //double
                        SendMouseClick(fakecursor.x, fakecursor.y, 20, 1);
                        ScreenToClient(hwnd, &fakecursor);

                    }
                }




                else if (olddown)
                {
                   
                    if (buttons & XINPUT_GAMEPAD_DPAD_DOWN && onoroff == true)
                    {
                        //post keep?
                        if (scrolloutsidewindow >= 3 && quickMW == 1) {
                            ClientToScreen(hwnd, &fakecursor); //double
                            SendMouseClick(fakecursor.x, fakecursor.y, 21, 1);
                            ScreenToClient(hwnd, &fakecursor);
                            //for sending mouse wheel. should work, but i went for combikey 5-8 instead
							// all comination keys are in Inputdevice.cpp now
                          //  atick++;
                         //   if (atick > 400) 
                         //   {
                          //      EnterCriticalSection(&deltaLock);
                         //       if (Dmousehilo[1])
                          //      {
                                
                                    
                          //          Dmousehilo[3] == true; //dinputdevice will clear
                           //         atick = 0;
                           //     }
                           //     LeaveCriticalSection(&deltaLock);
                                
                           // }
                        }
                    }
                    else {
                        PostKeyFunction(hwnd, downtype, false);
                        
                        Dkeyhilo[9] = false;
                        
                        olddown = false;
                    }
                    
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_DOWN && onoroff == true)
                {
                   PostKeyFunction(hwnd, downtype, true);

                   EnterCriticalSection(&deltaLock);
                    Dkeyhilo[9] = true;
                    LeaveCriticalSection(&deltaLock);

                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    if (scrolloutsidewindow == 0)
                        scroll.y = rect.bottom - 1;
                    if (scrolloutsidewindow == 1)
                        scroll.y = rect.bottom + 1;
                    scrollmap = true;
                    if (scrolloutsidewindow == 2) {
                        olddown = true;
                    }
                    if (scrolloutsidewindow >= 3) {
                        olddown = true;
                        scrollmap = false;
                        ClientToScreen(hwnd, &fakecursor); //double
                        SendMouseClick(fakecursor.x, fakecursor.y, 21, 1);
                        ScreenToClient(hwnd, &fakecursor);

                    }
                }





                else if (oldleft)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_LEFT && onoroff == true)
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
                else if (buttons & XINPUT_GAMEPAD_DPAD_LEFT && onoroff == true)
                {
                    PostKeyFunction(hwnd, lefttype, true);
                    if (scrolloutsidewindow == 0)
                        scroll.x = rect.left + 1;
                    if (scrolloutsidewindow == 1)
                        scroll.x = rect.left - 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                    scrollmap = true;
                    if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                        oldleft = true;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[10] = true;
                        LeaveCriticalSection(&deltaLock);
                    }

                }





                else if (oldright)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT && onoroff == true)
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
                else if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT && onoroff == true)
                {
                    PostKeyFunction(hwnd, righttype, true);
                    if (scrolloutsidewindow == 0)
                        scroll.x = rect.right - 1;
                    if (scrolloutsidewindow == 1)
                        scroll.x = rect.right + 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;
                    scrollmap = true;
                    if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                        oldright = true;
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[11] = true;
                        LeaveCriticalSection(&deltaLock);
                    }
                }
                else
                {
                    scrollmap = false;
                }




                if (buttons & XINPUT_GAMEPAD_START && showmessage == 0)
                {
                    Sleep(100);
                    if (onoroff == true && buttons & XINPUT_GAMEPAD_LEFT_SHOULDER && buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
                    {
                        //MessageBox(NULL, "Bmp mode", "shutdown", MB_OK | MB_ICONINFORMATION);
                        showmessage = 69;
                    }
                    else if (onoroff == false && buttons & XINPUT_GAMEPAD_LEFT_SHOULDER && buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
                    {
                        //MessageBox(NULL, "Bmp mode", "starting", MB_OK | MB_ICONINFORMATION);
                        showmessage = 70;
                    }
                    else if (mode == 0 && Modechange == 1 && onoroff == true)
                    {
                        mode = 1;

                        // MessageBox(NULL, "Bmp + Emulated cursor mode", "Move the flickering red dot and use right trigger for left click. left trigger for right click", MB_OK | MB_ICONINFORMATION);
                        showmessage = 2;
                    }
                    else if (mode == 1 && Modechange == 1 && onoroff == true)
                    {
                        mode = 2;
                        //MessageBox(NULL, "Edit Mode", "Button mapping. will map buttons you click with the flickering red dot as an input coordinate", MB_OK | MB_ICONINFORMATION);
                        showmessage = 3;


                    }
                    else if (mode == 2 && Modechange == 1 && onoroff == true)
                    {
                        // mode = 0;
                        // MessageBox(NULL, "Bmp mode", "only send input on bmp match", MB_OK | MB_ICONINFORMATION);
                        showmessage = 1;
                    }

                    else if (onoroff == true) { //assume modechange not allowed. send escape key instead
                        EnterCriticalSection(&deltaLock);
                        Dkeyhilo[16] = true;
                        LeaveCriticalSection(&deltaLock);
                    }
                    
                    // Sleep(1000); //have time to release button. this is no hurry anyway

                }
                else if (!(buttons & XINPUT_GAMEPAD_START) && Dkeyhilo[16] == true)
                {
                    EnterCriticalSection(&deltaLock);
                    Dkeyhilo[16] = false;
                    LeaveCriticalSection(&deltaLock);
				}
                if (mode > 0 && onoroff == true)
                {
                    //fake cursor poll
                    int Xaxis = 0;
                    int Yaxis = 0;
                    int scrollXaxis = 0;
                    int scrollYaxis = 0;
                    int width = rect.right - rect.left;
                    int height = rect.bottom - rect.top;
                    int Yscroll = 0;
                    int Xscroll = 0;
                    bool didscroll = false;




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


                    if (scrolloutsidewindow == 2 || scrolloutsidewindow == 3 || scrolloutsidewindow == 4)
                    {

                        if (oldscrollleftaxis)
                        {
                            if (scrollXaxis < AxisLeftsens) //left
                            {
                                if (scrolloutsidewindow == 3)
                                { //keep
                                    scrollXaxis = scrollXaxis - AxisLeftsens; //zero input
                                    doscrollyes = true;
                                    Xscroll = +scrollXaxis / scrollspeed3;
                                    didscroll = true;
                                }
                            }
                            else
                            { //stop
                                oldscrollleftaxis = false;
                                if (scrolloutsidewindow == 4)
                                { 
                                    EnterCriticalSection(&deltaLock);
                                    Dkeyhilo[14] = false;
                                    LeaveCriticalSection(&deltaLock);
                                }
                            }
                        }
                        else if (scrollXaxis < AxisLeftsens) //left
                        {
                            if (scrolloutsidewindow == 4) {
                                EnterCriticalSection(&deltaLock);
                                Dkeyhilo[14] = true;
                                LeaveCriticalSection(&deltaLock);
                            }
                            if (scrolloutsidewindow == 3 && doscrollyes == false)
                            {//start
                                tick = 0;
                                SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                                SendMouseClick(fakecursor.x, fakecursor.y, 5, 2); //4 skal vere 3 
                            }
                            oldscrollleftaxis = true;
                            //keystatesend = VK_LEFT;
                        }



                        if (oldscrollrightaxis)
                        {
                            if (scrollXaxis > AxisRightsens) //right
                            {
                                if (scrolloutsidewindow == 3)
                                { //keep
                                    doscrollyes = true;
                                    scrollXaxis = scrollXaxis - AxisRightsens; //zero input   
                                    Xscroll = scrollXaxis / scrollspeed3;
                                    didscroll = true;
                                }
                            }
                            else {
                                oldscrollrightaxis = false;
                                if (scrolloutsidewindow == 4)
                                { 
                                    EnterCriticalSection(&deltaLock);
                                    Dkeyhilo[15] = false;
                                    LeaveCriticalSection(&deltaLock);
                                }
                            }
                        }
                        else if (scrollXaxis > AxisRightsens) //right
                        {
                            if (scrolloutsidewindow == 4)
                            { 
                                EnterCriticalSection(&deltaLock);
                                Dkeyhilo[15] = true;
                                LeaveCriticalSection(&deltaLock);
                            }
                            if (scrolloutsidewindow == 3 && doscrollyes == false)
                            {//start
                                tick = 0;
                                SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                                SendMouseClick(fakecursor.x, fakecursor.y, 5, 2); //4 skal vere 3
                            }
                            oldscrollrightaxis = true;
                            //keystatesend = VK_RIGHT;

                        }



                        if (oldscrolldownaxis)
                        {
                            if (scrollYaxis < AxisDownsens)
                            {
                                if (scrolloutsidewindow == 3)
                                { //keep
                                    scrollYaxis = scrollYaxis - AxisDownsens; //zero input
                                    doscrollyes = true;
                                    Yscroll = scrollYaxis / scrollspeed3;
                                    didscroll = true;
                                }
                            }
                            else {
                                oldscrolldownaxis = false;
                                if (scrolloutsidewindow == 4)
                                { 
                                    EnterCriticalSection(&deltaLock);
                                    Dkeyhilo[13] = false;
                                    LeaveCriticalSection(&deltaLock);
                                }
                            }
                        }
                        else if (scrollYaxis < AxisDownsens) //down
                        { //start
                            if (scrolloutsidewindow == 4)
                            {
                                EnterCriticalSection(&deltaLock);
                                Dkeyhilo[13] = true;
                                LeaveCriticalSection(&deltaLock);
                            }
                            if (scrolloutsidewindow == 3 && doscrollyes == false)
                            {//start
                                tick = 0;
                                SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                                SendMouseClick(fakecursor.x, fakecursor.y, 5, 2); //4 skal vere 3
                            }
                            oldscrolldownaxis = true;
                        }





                        if (oldscrollupaxis)
                        {
                            if (scrollYaxis > AxisUpsens)
                            {
                                if (scrolloutsidewindow == 3)
                                { //keep
                                    scrollYaxis = scrollYaxis - AxisUpsens; //zero input
                                    doscrollyes = true;

                                    Yscroll = scrollYaxis / scrollspeed3; //150
                                    didscroll = true;
                                }
                            }
                            else {
                                oldscrollupaxis = false;
                                if (scrolloutsidewindow == 4)
                                {
                                    EnterCriticalSection(&deltaLock);
                                    Dkeyhilo[12] = false;
                                    LeaveCriticalSection(&deltaLock);
                                }
                            }
                        }
                        else if (scrollYaxis > AxisUpsens) //up
                        {
                            if (scrolloutsidewindow == 4) 
                            {
                                EnterCriticalSection(&deltaLock);
                                Dkeyhilo[12] = true;
                                LeaveCriticalSection(&deltaLock);
                            }
                            if (scrolloutsidewindow == 3 && doscrollyes == false)
                            {//start
                                tick = 0;
                                SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                                SendMouseClick(fakecursor.x, fakecursor.y, 5, 2); //4 skal vere 3
                            }
                            oldscrollupaxis = true;
                        }
                    }

                    //dinput WASD movement or thumbstick axis buttons

                        
                        
                  
                    //mouse left click and drag scrollfunction //scrolltype 3

                    if (doscrollyes) {
                        SendMouseClick(fakecursor.x + Xscroll, fakecursor.y - Yscroll, 8, 1); //4 skal vere 3

                        if (!didscroll && tick == scrollenddelay)
                        {
                            //MessageBox(NULL, "Scroll stopped", "Info", MB_OK | MB_ICONINFORMATION);
                            doscrollyes = false;
                            SendMouseClick(fakecursor.x, fakecursor.y, 6, 2); //4 skal vere 3
                        }
                    }



                    if (scrolloutsidewindow < 2 && scrollmap == false) //was 2
                    {
                        if (scrollXaxis < AxisLeftsens - 10000) //left
                        {
                            if (scrolloutsidewindow == 0)
                                scroll.x = rect.left + 1;
                            if (scrolloutsidewindow == 1)
                                scroll.x = rect.left - 1;
                            if (scrolloutsidewindow == 3)
                                scroll.x = (rect.left + (rect.right - rect.left) / 2) - 100;
                            scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                            scrollmap = true;

                        }
                        else if (scrollXaxis > AxisRightsens + 10000) //right
                        {
                            if (scrolloutsidewindow == 0)
                                scroll.x = rect.right - 1;
                            if (scrolloutsidewindow == 1)
                                scroll.x = rect.right + 1;
                            if (scrolloutsidewindow == 3)
                                scroll.x = (rect.left + (rect.right - rect.left) / 2) + 100;
                            scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                            scrollmap = true;

                        }
                        else if (scrollYaxis < AxisDownsens - 10000) //down
                        {
                            scroll.x = rect.left + (rect.right - rect.left) / 2;
                            if (scrolloutsidewindow == 0)
                                scroll.y = rect.bottom - 1;
                            if (scrolloutsidewindow == 1)
                                scroll.y = rect.bottom + 1;
                            if (scrolloutsidewindow == 3)
                                scroll.y = (rect.top + (rect.bottom - rect.top) / 2) + 100;
                            scrollmap = true;


                        }




                        else if (scrollYaxis > AxisUpsens + 10000) //up
                        {
                            scroll.x = rect.left + (rect.right - rect.left) / 2;
                            if (scrolloutsidewindow == 0)
                                scroll.y = rect.top + 1;
                            if (scrolloutsidewindow == 1)
                                scroll.y = rect.top - 1;
                            if (scrolloutsidewindow == 3)
                                scroll.y = (rect.top + (rect.bottom - rect.top) / 2) - 100;
                            scrollmap = true;
                        }

                        else {
                            scrollmap = false;
                        }
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

                    if (Xf < rect.left) Xf = rect.left;
                    if (Xf > rect.right) Xf = rect.right;
                    if (Yf < rect.top) Yf = rect.top;
                    if (Yf > rect.bottom) Yf = rect.bottom;

                    if (movedmouse == true) //fake cursor move message
                    {
                        if (userealmouse == 0)
                        {
                            //  if ( !leftPressed && !rightPressed)
                                  //fakecursor.x = Xf;
                                  //fakecursor.y = Yf;
                            SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                            //   else if (leftPressed && !rightPressed)
                             //      SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                        }
                    }

                    if (leftPressed)
                    {

                        if (leftPressedold == false)
                        {
                            //save coordinates
                            startdrag.x = Xf;
                            startdrag.y = Yf;
                            leftPressedold = true;
                            if (userealmouse == 0 && scrolloutsidewindow == 3)
                            {
                                SendMouseClick(fakecursor.x, fakecursor.y, 5, 2); //4 skal vere 3
                                SendMouseClick(fakecursor.x, fakecursor.y, 6, 2); //double click
                            }
                            else if (userealmouse == 0)
                                SendMouseClick(fakecursor.x, fakecursor.y, 5, 2); //4 skal vere 3
                        }
                    }
                    if (leftPressedold)
                    {
                        if (!leftPressed)
                        {
                            if (userealmouse == 0) {


                                SendMouseClick(fakecursor.x, fakecursor.y, 6, 2); //double click

                            }
                            else
                            {

                                if (abs(startdrag.x - fakecursor.x) <= 5)
                                {
                                    ClientToScreen(hwnd, &startdrag);
                                    SendMouseClick(startdrag.x, startdrag.y, 2, 3); //4 4 move //5 release
                                    ScreenToClient(hwnd, &startdrag);
                                }
                                else
                                {
                                    ClientToScreen(hwnd, &startdrag);
                                    SendMouseClick(startdrag.x, startdrag.y, 5, 2); //4 4 move //5 release
                                    Sleep(30);
                                    SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                                    Sleep(20);
                                    SendMouseClick(fakecursor.x, fakecursor.y, 6, 2);
                                    ScreenToClient(hwnd, &startdrag);

                                }
                            }
                            leftPressedold = false;
                        }
                    }
                    if (rightPressed)
                    {
                        if (rightPressedold == false)
                        {
                            //save coordinates
                            //start
                            startdrag.x = Xf;
                            startdrag.y = Yf;
                            rightPressedold = true;
                            if (userealmouse == 0)
                            {
                                DWORD currentTime = GetTickCount64();
                                if (currentTime - lastClickTime < GetDoubleClickTime() && movedmouse == false && doubleclicks == 1)
                                {
                                    SendMouseClick(fakecursor.x, fakecursor.y, 30, 2); //4 skal vere 3

                                }
                                else
                                {
                                    SendMouseClick(fakecursor.x, fakecursor.y, 3, 2); //4 skal vere 3

                                }
                                lastClickTime = currentTime;

                            }
                        }


                    }
                    if (rightPressedold)
					{
						if (vibrator < 20000)
                        { 
                            vibrator += 10;
                        }
                        vibrateController(0, vibrator);
                        if (!rightPressed)
                        {
                            vibrator = 0;
                            vibrateController(0, vibrator);
                            if (userealmouse == 0)
                                SendMouseClick(fakecursor.x, fakecursor.y, 4, 2);
                            else
                            {
                                if (abs(startdrag.x - fakecursor.x) <= 5)
                                {
                                    ClientToScreen(hwnd, &startdrag);
                                    SendMouseClick(startdrag.x, startdrag.y, 1, 3); //4 4 move //5 release
                                    ScreenToClient(hwnd, &startdrag);
                                }
                                else
                                {
                                    ClientToScreen(hwnd, &startdrag);
                                    SendMouseClick(startdrag.x, startdrag.y, 3, 2); //4 4 move //5 release
                                    Sleep(30);
                                    SendMouseClick(fakecursor.x, fakecursor.y, 8, 1); //4 skal vere 3
                                    Sleep(20);
                                    SendMouseClick(fakecursor.x, fakecursor.y, 4, 2);
                                    ScreenToClient(hwnd, &startdrag);
                                }
                            }
                            rightPressedold = false;
                        }
                    } //rightpress
                } // mode above 0
                ScreenToClient(hwnd, &fakecursor);
            } //no controller
            else {
                showmessage = 12;
                //MessageBoxA(NULL, "Controller not connected", "Error", MB_OK | MB_ICONERROR);
               // CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true); //draw message
            }
            if (drawfakecursor == 1 || showmessage != 0)
                CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true); //draw fake cursor
        } // no hwnd
        if (knappsovetid > 20)
        {
            //  sovetid = 20;
            //  knappsovetid = 100;

        }

        if (showmessage != 0 && showmessage != 12)
        {
            counter++;
            if (counter > 500)
            {
                if (showmessage == 1) {
                    mode = 0;
                }
                if (showmessage == 69) { //disabling dll
                    onoroff = false;
                }
                if (showmessage == 70) { //enabling dll
                    onoroff = true;
                }
                showmessage = 0;
                counter = 0;

            }
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
