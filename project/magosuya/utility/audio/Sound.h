#pragma once
#include <xaudio2.h>
#include <fstream>
#include <memory>
#include "struct.h"

class MagosuyaEngine;

//unique_ptr<IXAudio2SourceVoice>のデストラクタを置き換える
struct SourceVoiceDeleter {
	void operator()(IXAudio2SourceVoice* p) const {
		if (p) {
			// p->Release(); の代わりに XAudio2 の破棄関数を呼ぶでやんす
			p->DestroyVoice ();
		}
	}
};

class Sound{
public: // メンバ関数
	Sound (MagosuyaEngine* magosuya);
	~Sound ();

	void Play ();
	void Stop ();
	void SetSound (const std::string& id);

private:
	void CreateFormat ();

private:// メンバ変数
	//音声データ
	const SoundData* data_ = nullptr;
	//再生用ボイス
	std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter> sourceVoice_ = nullptr;

	//ポインタを借りる
	MagosuyaEngine* magosuya_ = nullptr;
};