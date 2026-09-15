#include "PCH.h"
#include "AudioManager.h"
#include "ChangeString.h"

AudioManager::~AudioManager() {
}

void AudioManager::Initialize() {
	// 再初期化に備えてフラグをリセット
	isFinalized_ = false;

	HRESULT hr;
	//XAudio2を初期化
	hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));

	//マスターボイスの初期化
	hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(hr));

	//Windows Media Foundationの初期化
	hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(hr));
}

void AudioManager::Update() {
	// 終了処理済み、またはエンジンが動いていないなら再生しない
	if (isFinalized_ || !xAudio2_) {
		return;
	}

	//再生が終わった音声を削除する
	for (auto it = activeVoices_.begin(); it != activeVoices_.end();) {
		XAUDIO2_VOICE_STATE state;
		it->second->GetState(&state);

		//待機中のバッファが0になった = 再生完了
		if (state.BuffersQueued == 0) {
			it->second->Stop();
			it->second->DestroyVoice();
			it = activeVoices_.erase(it);	//リストから削除して次の要素へ
		}
		else {
			++it;
		}
	}
}

void AudioManager::Finalize() {
	if(isFinalized_) return;
	if(!xAudio2_) return; // すでにリセット済みなら何もしない

	//まず再生中のボイスをすべて破棄する
	for(auto& pair : activeVoices_) {
		pair.second->Stop();
		pair.second->DestroyVoice();
	}
	activeVoices_.clear();
	isFinalized_ = true;   // 終わったことを記録

	//読み込んだリソース（データ）をクリアする
	audioResources_.clear();

	// マスターボイスを明示的に破壊するでやんす
	if(masterVoice_) {
		masterVoice_->DestroyVoice();
		masterVoice_ = nullptr;
	}

	//XAudio2本体をリセットする
	xAudio2_.Reset();

	//最後にMedia Foundationをシャットダウンする
	MFShutdown();
}

void AudioManager::Load(const std::string& filename, const std::string& name) {
	if (audioResources_.count(name)) {
		//重複読み込み回避
		return;
	}

	HRESULT hr;

	//フルパスをワイド文字列に変換
	std::wstring filePathW = String::ConvertString(filename);

	//SourceReader作成
	Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
	hr = MFCreateSourceReaderFromURL(filePathW.c_str(), nullptr, &pReader);
	assert(SUCCEEDED(hr));

	//PCM形式にフォーマットを指定する
	Microsoft::WRL::ComPtr<IMFMediaType> pPCMType;
	MFCreateMediaType(&pPCMType);
	pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	hr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPCMType.Get());
	assert(SUCCEEDED(hr));

	//実際にセットされたメディアタイプを取得する
	Microsoft::WRL::ComPtr<IMFMediaType> pOutType;
	pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutType);

	//Waveフォーマットを取得する
	WAVEFORMATEX* waveFormat = nullptr;
	MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &waveFormat, nullptr);

	//returnするための音声データ
	AudioData audioData = {};
	audioData.wfex = *waveFormat;

	//生成したWaveフォーマットを解放
	CoTaskMemFree(waveFormat);

	//PCMデータのバッファを構築
	while (true) {
		Microsoft::WRL::ComPtr<IMFSample> pSample;
		DWORD streamIndex = 0, flags = 0;
		LONGLONG llTimeStamp = 0;
		//サンプルを読み込む
		hr = pReader->ReadSample(
			MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0,
			&streamIndex, &flags, &llTimeStamp, &pSample
		);
		//ストリームの末尾に達したら抜ける
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;

		if (pSample) {
			Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
			//サンプルに含まれているサウンドデータのバッファを一繋ぎにして取得
			pSample->ConvertToContiguousBuffer(&pBuffer);

			BYTE* pData = nullptr;	//データ読み取り用ポインタ
			DWORD maxLength = 0, currentLength = 0;
			//バッファ読み込み用にロック
			pBuffer->Lock(&pData, &maxLength, &currentLength);
			//バッファの末尾にデータを追加
			audioData.buffer.insert(audioData.buffer.end(), pData, pData + currentLength);
			pBuffer->Unlock();
		}
	}

	audioResources_[name] = audioData;
}

void AudioManager::Unload(const std::string& name) {
	// 指定した名前のデータがあるか確認
	auto it = audioResources_.find(name);
	if (it != audioResources_.end()) {
		// マップから削除
		audioResources_.erase(it);
	}
}

void AudioManager::UnloadAll() {
	// 再生中の音があればすべて止める
	for (auto& pair : activeVoices_) {
		pair.second->Stop();
		pair.second->DestroyVoice();
	}
	activeVoices_.clear();
	audioResources_.clear();
}

uint32_t AudioManager::Play(const std::string& name, AudioType type, bool loop) {
	// 終了処理済み、またはエンジンが動いていないなら再生しない
	if (isFinalized_ || !xAudio2_) {
		return 0;
	}

	// 読み込まれていない音声名なら再生せずに弾く
	auto it = audioResources_.find(name);

	if (it == audioResources_.end()) {
		return 0; // 無効なIDとして0を返して終了
	}

	// 存在するデータを取り出す
	const AudioData& data = it->second;

	IXAudio2SourceVoice* pSourceVoice = nullptr;

	//xAudio2_を使って生成
	xAudio2_->CreateSourceVoice(&pSourceVoice, &data.wfex, 0, 2.0f, &voiceCallback_);

	//PlayIDをコンテキストとして渡すためのメモリ確保
	void* pContext = reinterpret_cast<void*>(static_cast<uintptr_t>(nextPlayID_));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = data.buffer.data();
	buf.AudioBytes = static_cast<UINT32>(data.buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.pContext = pContext;	//IDを仕込む

	//フラグによってループさせる
	if (loop)buf.LoopCount = XAUDIO2_LOOP_INFINITE;

	pSourceVoice->SubmitSourceBuffer(&buf);

	switch(type) {
	case BGM:
		pSourceVoice->SetVolume(volumeBGM_);
		break;

	case SE:
		pSourceVoice->SetVolume(volumeSE_);
		break;
	}

	pSourceVoice->Start();

	//再生中音声リストに追加
	uint32_t playID = nextPlayID_++;
	activeVoices_[playID] = pSourceVoice;

	return playID;
}

void AudioManager::Stop(uint32_t playID) {
	// 終了処理済みなら何もしない
	if(isFinalized_) return;

	if (activeVoices_.count(playID)) {
		//ボイスを停止
		activeVoices_[playID]->Stop();
		//ボイスを破壊
		activeVoices_[playID]->DestroyVoice();
		//再生中音声リストから削除
		activeVoices_.erase(playID);
	}
}

void AudioManager::SetVolume(uint32_t playID, float volume) {
	// 指定したIDが再生中かチェック
	if(activeVoices_.count(playID)) {
		//IXAudio2SourceVoiceのSetVolumeを呼ぶ
		activeVoices_[playID]->SetVolume(volume);
	}
}

void AudioManager::SetMasterVolume(float volume) {
	if(masterVoice_) {
		masterVoice_->SetVolume(volume);
	}
}