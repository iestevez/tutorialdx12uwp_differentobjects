//
// pch.h
// Header for standard system include files.
//

#pragma once
//Comment the following define for previous versions of SDK to 1941, for example SDK18362
#define _SDK19041

#define MYTRACE OutputDebugString


// Use the C++ standard templated min/max
#define NOMINMAX

#include <wrl/client.h>
#include <wrl/event.h>

// Headers DirectX 12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

#include <d2d1_3.h>
#include <dwrite.h>
#include <d3d11on12.h>

// Helper DirectX12
#ifndef _SDK19041
#include "d3dx12_18362.h"
#endif // !_SDK1941

#ifdef _SDK19041
#include "d3dx12.h"
#endif


// Cabeceras de la C++ STL
#include <algorithm>
#include <exception>
#include <future>
#include <memory>
#include <stdexcept>
#include <fstream>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

// Cabeceras WinRT

#include "winrt/base.h"
namespace winrt::impl
{
    template <typename Async>
    auto wait_for(Async const& async, Windows::Foundation::TimeSpan const& timeout);
}
#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Graphics.Display.h"
#include "winrt/Windows.System.h"
#include "winrt/Windows.UI.Core.h"
#include "winrt/Windows.UI.Input.h"
#include "winrt/Windows.UI.ViewManagement.h"
#include "winrt/Windows.Devices.Input.h"

// Windows Imaging Component (WIC) to load sprites
#include "wincodec.h"

#include "windows.h"

namespace DX
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::exception();
        }
    }
}