#pragma once
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <xaudio2.h>
#include <unordered_map>
#include <fstream>
#include <string>
#include "struct.h"

class SoundManager {
public:		//メンバ関数
	SoundManager ();

	void Finalize ();

	//サウンドロード関数
	SoundData* LoadSound (const std::string& filePath, const std::string& id);
	//サウンドアンロード関数
	void UnloadSound (const std::string& id);
	//登録した音を取得
	const SoundData& GetSound (const std::string& id) const;
	//オーディオエンジン
	IXAudio2* GetXAudio () { return xAudio2_.Get (); }
	//マスターボイス
	IXAudio2MasteringVoice* GetMasterVoice () { return masterVoice_; }

private:	//ヘルパー関数
	SoundData SoundLoadWave (const std::string& filename);
	void SoundUnload (SoundData* soundData);

private:	//メンバ変数
	//音声データマップ
	std::unordered_map<std::string, SoundData> map_;
	//オーディオエンジン
	ComPtr<IXAudio2> xAudio2_;
	//マスターボイス
	IXAudio2MasteringVoice* masterVoice_;
};