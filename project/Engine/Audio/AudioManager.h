#pragma once
#include "VoiceCallback.h"

//チャンクヘッダ
struct ChunkHeader {
	char id[4];		//チャンク毎のID
	int32_t size;	//チャンクサイズ
};

//RIFFヘッダチャンク
struct RiffHeader {
	ChunkHeader chunk;	//"RIFF"
	char type[4];		//"WAVE"
};

//FMTチャンク
struct FormatChunk {
	ChunkHeader chunk;	//"fmt"
	WAVEFORMATEX fmt;		//波形フォーマット
};

//オーディオデータ構造体
struct AudioData {
	//波形フォーマット
	WAVEFORMATEX wfex;
	//バッファ
	std::vector<BYTE> buffer;
};

//音声の種類
enum AudioType {
	BGM,
	SE,
};

class AudioManager {
public:
	static AudioManager* GetInstance() {
		//初めて呼び出されたときに一回だけ初期化
		static AudioManager instance;
		return &instance;
	}

	//デストラクタ
	~AudioManager();

	//初期化
	void Initialize();

	//更新
	void Update();

	//終了処理
	void Finalize();

	//読み込み
	void Load(const std::string& filename, const std::string& name);

	//解放
	void Unload(const std::string& name);

	//全解放
	void UnloadAll();

	//再生
	uint32_t Play(const std::string& name, AudioType type, bool loop = false);

	//停止
	void Stop(uint32_t playID);

	//音声ごとの音量調整(0.0f ~ 1.0f)
	void SetVolume(uint32_t playID, float volume);

	//全体の音量調整
	void SetMasterVolume(float volume);

private:
	//コンストラクタを禁止
	AudioManager() = default;
	// コピーコンストラクタと代入演算子を禁止
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;
	AudioManager(AudioManager&&) = delete;
	AudioManager& operator=(AudioManager&&) = delete;

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;

	std::unordered_map<std::string, AudioData> audioResources_; // 読み込み済みデータ
	std::unordered_map<uint32_t, IXAudio2SourceVoice*> activeVoices_; // 再生中のボイス
	uint32_t nextPlayID_ = 0;

	VoiceCallback voiceCallback_;

	//終了処理
	bool isFinalized_ = false;

	//BGM・SE音量
	float volumeBGM_ = 0.5f;
	float volumeSE_ = 0.5f;
};