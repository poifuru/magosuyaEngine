#include "Logger.h"
#include "function.h"

//ログを出力する関数
void Logger::Log (std::ofstream& os, const std::string& message) {
	os << message << std::endl;
	OutputDebugStringA (message.c_str ());
}
