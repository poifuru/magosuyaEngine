#pragma once
#include <chrono>
#include <format>
#include <string>
#include "Logger.h"

/// <summary>
/// スコープを抜けるまでの経過時間を計測してログに出すクラス
/// 使い方: { SCOPE_TIMER("シェーダーコンパイル"); ...計測したい処理... }
/// </summary>
class ScopeTimer {
public:
	explicit ScopeTimer(std::string name)
		: name_(std::move(name)), start_(std::chrono::high_resolution_clock::now()) {
		// ネストの深さを記録しておき、ログをインデントして親子関係を見やすくする
		depth_ = sDepth_++;
	}

	~ScopeTimer() {
		auto end = std::chrono::high_resolution_clock::now();
		float ms = std::chrono::duration<float, std::milli>(end - start_).count();
		sDepth_--;

		std::string indent(static_cast<size_t>(depth_) * 2, ' ');
		std::string message = std::format("[Timer] {}{}: {:.2f} ms\n", indent, name_, ms);

		// ログファイルとVSの出力ウィンドウの両方に出す（出力ウィンドウへはLogger側で出している）
		Logger::GetInstance()->Log(message);
	}

	// コピー・ムーブは禁止（計測が二重にならないように）
	ScopeTimer(const ScopeTimer&) = delete;
	ScopeTimer& operator=(const ScopeTimer&) = delete;

private:
	std::string name_;
	std::chrono::high_resolution_clock::time_point start_;
	int depth_ = 0;

	// 現在のネストの深さ（メインスレッドでの使用を想定）
	static inline int sDepth_ = 0;
};

// 変数名を考えずに1行で置けるようにするマクロ
#define SCOPE_TIMER_CONCAT_INNER(a, b) a##b
#define SCOPE_TIMER_CONCAT(a, b) SCOPE_TIMER_CONCAT_INNER(a, b)
#define SCOPE_TIMER(name) ScopeTimer SCOPE_TIMER_CONCAT(scopeTimer_, __LINE__)(name)
