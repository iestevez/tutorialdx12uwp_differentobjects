#pragma once
#include "pch.h"

using namespace DirectX;

struct Vertex {

	XMFLOAT3 pos;
	XMFLOAT4 col;
	XMFLOAT3 normal;
	XMFLOAT2 uvcoords;
	XMUINT3 material;

};

class Mesh
{
public:
	Mesh();
	Mesh(std::string const fileName);
	~Mesh();

	UINT GetVSize() const;
	UINT GetISize() const;
	void readFile(std::string const fileName);
	void readObjFile(std::string const fileName);
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	struct Texture {
		std::string Name;
		std::wstring Filename;
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
	};

	std::unique_ptr<Texture> meshTexture;
private:
	UINT vsize;
	UINT isize;
	XMFLOAT4 defaultColor;
};

