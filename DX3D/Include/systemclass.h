////////////////////////////////////////////////////////////////////////////////
// Filename: systemclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <windows.h>
//#include "inputclass.h"
//#include "applicationclass.h"

#include "../../External/Imgui/imgui.h"
#include "../../External/Imgui/imgui_impl_win32.h"
#include "../../External/Imgui/imgui_impl_dx11.h"

class SystemClass
{
public:
	explicit SystemClass();
	~SystemClass();

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	bool Frame();
	void InitializeWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCWSTR m_applicationName;
	HINSTANCE m_hinstance;
	HWND m_hwnd;
	// InputClass* m_Input;
	//ApplicationClass* m_Application;

};

/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/////////////
// GLOBALS //
/////////////
static SystemClass* ApplicationHandle = 0;
