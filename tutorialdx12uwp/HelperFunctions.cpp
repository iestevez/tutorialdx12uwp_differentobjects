#pragma once
#include "pch.h"
#include "HelperFunctions.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// From Frank D. Luna: 3D Game Programming with DirectX12


	unsigned  int CalcConstantBufferByteSize(unsigned int bytesize) {


		return (bytesize + 255) & ~255;
	}
	

	void readfile(char const* fn, std::vector<char> &vbytes)
	{
		std::ifstream inputstream(fn, std::ios::binary);
		std::ios_base::iostate state = inputstream.rdstate();
		//bool isbad = inputstream.bad();
		//bool isgood = inputstream.good();
		//bool isfail = inputstream.fail();
		inputstream.seekg(0, std::ios::end);
		std::ifstream::pos_type pos = inputstream.tellg();
		vbytes.resize(pos);

		inputstream.seekg(0, std::ios::beg);
		inputstream.read((char*) &vbytes[0], pos);

		
	}

	// Debug related:
	void DebugLiveObjects() {
		// Debug live objects
#ifndef NDEBUG
// Configure debug device (if active).
		//ComPtr<ID3D12InfoQueue> d3dInfoQueue;
		{
			ComPtr<IDXGIDebug> dxgiDebug;
			HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
			if (SUCCEEDED(hr))
			{
#ifdef _DEBUG
				dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
#endif

			}
		}
#endif
	}
