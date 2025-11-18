#pragma once
#include <unordered_map>
#include <fstream>
#include <string>
#include "struct.h"

class SoundManager {
public:		//メンバ関数


	void LoadSound (const std::string& filePath, const std::string& id);
	void UnloadSound (const std::string& id);

private:	//メンバ変数
	std::unordered_map<std::string, SoundData> map_;
};