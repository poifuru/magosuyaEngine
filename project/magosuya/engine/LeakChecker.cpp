#include "LeakChecker.h"
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <dxgidebug.h>
#include <d3d12.h>
#include <dxgi1_6.h>

LeakChecker::~LeakChecker () {
	ComPtr<IDXGIDebug> debug;
	if (SUCCEEDED (DXGIGetDebugInterface1 (0, IID_PPV_ARGS (debug.GetAddressOf ())))) {
		debug->ReportLiveObjects (DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects (DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects (DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
	}
}
