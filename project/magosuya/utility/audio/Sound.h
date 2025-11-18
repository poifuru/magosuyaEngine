#pragma once
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <xaudio2.h>
#include <fstream>
#include "struct.h"

class Sound{
public: // メンバ関数
	void Play ();

private:// メンバ変数
	IXAudio2MasteringVoice* masterVoice_;
};