#pragma once
#include <string>

namespace Logger {
	//ログを出力する関数
	void Log (std::ofstream& os, const std::string& message);

	//ログをテキストで出す関数
	std::ofstream& Logtext ();
}