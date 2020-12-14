#pragma once
#include "pch.h"

void ShowWinRTError(winrt::hresult_error &error) {
	HRESULT code = error.code();

	winrt::hstring msg = error.message();
	//std::wcout << "Error message: " << static_cast<std::wstring>(msg);
	wchar_t* msgbuff = new wchar_t[4096];
	swprintf(msgbuff, 4096, L"E===>Error code: %d, Error message: %s\n", code, msg.c_str());
	//TRACE("Error message: %s", static_cast<std::wstring>(msg));
	MYTRACE(msgbuff);
	delete[] msgbuff;
	winrt::throw_hresult(code);
}
