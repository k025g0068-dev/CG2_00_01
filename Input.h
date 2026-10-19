#pragma once
#include <Windows.h>
#include <wrl.h>

#define DIRECYINOUT_VERSION 0x0800
#include<dinput.h>

class Input{
public:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	void Initialize(HINSTANCE hInsrance,HWND hwnd);
	void Update();

	bool PushKey(BYTE Keynumber);
	bool TriggerKey(BYTE keyNumber);

private:
	ComPtr<IDirectInput8>directInput;
	ComPtr<IDirectInputDevice8> keyboard;

	BYTE key[256] = {};
	BYTE keyPre[256] = {};
};

