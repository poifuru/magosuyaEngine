#include "SoundManager.h"
#pragma comment(lib, "xaudio2.lib")
#include <cassert>

SoundManager::SoundManager () {
	HRESULT hr;
	// XAudio2のエンジン作成
	hr = XAudio2Create (&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert (SUCCEEDED (hr));

	// マスターボイスの作成 (IXAudio2MasteringVoice* masterVoice_)
	hr = xAudio2_->CreateMasteringVoice (&masterVoice_);
	assert (SUCCEEDED (hr));
}

void SoundManager::Finalize () {
	if (masterVoice_) {
		masterVoice_->DestroyVoice ();
		masterVoice_ = nullptr;
	}
	for (auto& pair : map_) {
		delete[] pair.second.pBuffer;
	}
	map_.clear ();
	xAudio2_.Reset ();
}

SoundData* SoundManager::LoadSound (const std::string& filePath, const std::string& id) {
	if (!map_.count (id)) {
		//読み込むデータを入れる箱
		SoundData data = SoundLoadWave (filePath);

		map_[id] = data;
	}
	return &map_.at (id);
}

void SoundManager::UnloadSound (const std::string& id) {
	auto it = map_.find (id);

	// 存在するかチェック
	if (it != map_.end ()) {
		// メモリを解放
		SoundUnload (&it->second);

		// mapから要素を削除
		map_.erase (it);
	}
}

const SoundData& SoundManager::GetSound (const std::string& id) const {
	assert (map_.count (id));
	return map_.at (id);
}

SoundData SoundManager::SoundLoadWave (const std::string& filename) {
	/*1,ファイルを開く*/
	  //ファイルストリームのインスタンス
	std::ifstream file;
	//wavファイルをバイナリーモードで開く
	file.open (filename, std::ios_base::binary);
	//ファイルが開けなければassert
	assert (file.is_open ());

	/*2,wavデータ読み込み*/
	//RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read ((char*)&riff, sizeof (riff));
	//ファイルがRIFFかチェック
	if (strncmp (riff.chunk.id, "RIFF", 4) != 0) {
		assert (0);
	}
	//タイプがWAVEかチェック
	if (strncmp (riff.type, "WAVE", 4) != 0) {
		assert (0);
	}

	//Formatチャンクの読み込み
	FormatChunk format = {};
	//チャンクヘッダーの確認
	file.read ((char*)&format, sizeof (ChunkHeader));
	if (strncmp (format.chunk.id, "fmt ", 4) != 0) {
		assert (0);
	}

	//チャンク本体の読み込み
	assert (format.chunk.size <= sizeof (format.fmt));
	file.read ((char*)&format.fmt, format.chunk.size);

	//Dataチャンクの読み込み
	ChunkHeader data;
	file.read ((char*)&data, sizeof (data));
	//JUNKチャンクを検出した場合
	if (strncmp (data.id, "JUNK", 4) != 0) {
		//読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg (data.size, std::ios_base::cur);
		//再読み込み
		file.read ((char*)&data, sizeof (data));
	}

	if (strncmp (data.id, "data", 4) != 0) {
		assert (0);
	}

	//Dataチャンクのデータ部(波形データ)の読み込み
	char* pBuffer = new char[data.size];
	file.read (pBuffer, data.size);

	//waveファイルを閉じる
	file.close ();

	//returnするための音声データ
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

void SoundManager::SoundUnload (SoundData* soundData) {
	//バッファのメモリを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}
