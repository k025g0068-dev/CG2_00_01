#pragma once
#include "WinApp.h"

#include <Windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION 0x0800 
#include<dinput.h>

class Input{
public:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	void Initialize(WinApp*winApp);
	void Update();

	bool PushKey(BYTE Keynumber);
	bool TriggerKey(BYTE keyNumber);

private:
	WinApp* winApp = nullptr;

	ComPtr<IDirectInput8>directInput;
	ComPtr<IDirectInputDevice8> keyboard;

	BYTE key[256] = {};
	BYTE keyPre[256] = {};
};

