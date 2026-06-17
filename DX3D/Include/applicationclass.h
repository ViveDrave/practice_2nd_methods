////////////////////////////////////////////////////////////////////////////////
// Filename:  applicationclass.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "d3dclass.h"
#include "riskanalyzer.h"
#include "../../External/Imgui/imgui.h"
#include "../../External/Imgui/imgui_impl_win32.h"
#include "../../External/Imgui/imgui_impl_dx11.h"
#include "../../External/Imgui/implot.h"


class ApplicationClass
{
public:
	ApplicationClass();
	~ApplicationClass();
	
	bool Initialize(int, int, HWND);
	void Shutdown();
	bool Frame();

	void imgui_render();

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();

	void SaveReportCSV(const char* filename);

private:
	bool Render();
	D3DClass* m_Direct3D;

	RiskAnalyzer m_analyzer;
	RiskResults m_results;
	bool m_needsRecalc = true;
}; 

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/////////////////
/// GLOBALS /////
/////////////////
const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = false;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;