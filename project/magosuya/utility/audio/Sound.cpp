#include "Sound.h"
#include <cassert>
#include "MagosuyaEngine.h"

Sound::Sound (MagosuyaEngine* magosuya) {
	assert (magosuya);
	magosuya_ = magosuya;
}

Sound::~Sound () {
	if (sourceVoice_) {
		// 念のため停止とバッファクリア
		sourceVoice_->Stop ();
		sourceVoice_->FlushSourceBuffers ();

		// unique_ptrを明示的にリセットすることで、
		// SourceVoiceDeleterがここで呼ばれてDestroyVoiceが安全に実行されるでやんす。
		sourceVoice_.reset (nullptr);
	}
}

void Sound::Play () {
	HRESULT hr;
	// ボイスの停止（再生中に Play された場合のため）
	sourceVoice_->Stop ();
	sourceVoice_->FlushSourceBuffers (); // バッファクリア

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = data_->pBuffer;
	buf.AudioBytes = data_->bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	// 波形データの再生
	hr = sourceVoice_->SubmitSourceBuffer (&buf);
	hr = sourceVoice_->Start ();
}

void Sound::Stop () {
	// ボイスの停止
	sourceVoice_->Stop ();
	sourceVoice_->FlushSourceBuffers (); // バッファクリア
}

void Sound::SetSound (const std::string& id) {
	data_ = &magosuya_->GetSound (id);
	CreateFormat ();
}

void Sound::CreateFormat () {
	HRESULT hr;
	// ソースボイス作成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	hr = magosuya_->GetXAudio ()->CreateSourceVoice (&pSourceVoice, &data_->wfex);
	sourceVoice_.reset (pSourceVoice);
	assert (SUCCEEDED (hr));
}