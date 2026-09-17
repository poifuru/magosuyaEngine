#pragma once

// WindowsAPI
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // 滅多に使わないWindowsヘッダーを除外して軽量化
#endif

#ifndef NOMINMAX
#define NOMINMAX            // std::min/max との衝突を防止
#endif

#include <Windows.h>
#include <wrl/client.h>     // Microsoft::WRL::ComPtr (DX12の生ポインタ管理に必須)

// DirectX
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")
#include <dxgi1_6.h>        // DXGIファクトリやスワップチェーン（最新の1.6を推奨）
#pragma comment(lib, "dxgi.lib")
#include <dxcapi.h>    // シェーダーのランタイムコンパイル用
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#ifdef _DEBUG
#include <dxgidebug.h>      // デバッグ層のメモリリーク検出用
#endif

#include <d3dx12.h>         // Microsoft公式のDX12補助構造体（※別途ダウンロードが必要な場合あり）

// PIX
#include <pix3.h>

// XInput
#include <Xinput.h>
#pragma comment(lib, "Xinput.lib")

// XAudio
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

// C++標準ライブラリ
#include <algorithm>        // std::min, std::max, std::sortなど
#include <array>
#include <cassert>
#include <chrono>           // 高精度タイマー用
#include <cmath>
#define _USE_MATH_DEFINES
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>           // std::unique_ptr, std::shared_ptr
#include <numbers>
#include <optional>
#include <queue>
#include <span>
#include <string>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

#include <hidusage.h>
#include <strsafe.h>
#include <DbgHelp.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Dbghelp.lib")

// 外部ライブラリ
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>
#include <imGuizmo.h>
#include <DirectXTex.h>
#include <json.hpp>

// 自作ヘッダー
#include "MathTypes.h"
#include "Transform.h"
#include "GameTime.h"