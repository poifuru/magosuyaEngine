#pragma once
#include <string>

namespace String {
	//DirectX12が返してくる文字列を必要に応じて型変換してあげる
	//string->wstring
	std::wstring ConvertString (const std::string& str);
	//wstring->string
	std::string ConvertString (const std::wstring& str);
}