#pragma once
#include <Windows.h>
#include <cstdint>

class WinApp {
public:
	// ウィンドウプロシージャ
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
	// クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

public:
	void Initialize();
	void Update();
	void Finalize();

	HWND GetHwnd() const { return hwnd_; }
	HINSTANCE GetHInstance() const { return wc_.hInstance; }

private:
	HWND hwnd_ = nullptr;
	WNDCLASS wc_{};

	
	
};