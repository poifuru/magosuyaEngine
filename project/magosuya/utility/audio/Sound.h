#pragma once
#include <xaudio2.h>
#include <fstream>

class Sound{
public: //メンバ関数

private://メンバ変数
	IXAudio2 xAudio2_;
	IXAudio2MasteringVoice* masterVoice_;
};