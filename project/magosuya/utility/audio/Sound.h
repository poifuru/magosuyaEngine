#pragma once
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.h")
#include <fstream>
#include "../utility/ComPtr.h"

class Sound{
public: //メンバ関数
	void 

private://メンバ変数
	ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_;
};