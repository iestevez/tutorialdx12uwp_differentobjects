#include "pch.h"
#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION 
#include "tiny_obj_loader.h"
#include "Error.h"
#include <iostream>



Mesh::Mesh() : defaultColor (XMFLOAT4(Colors::White))
{
	vertices = {
		{XMFLOAT3(-1.0f,-1.0f,-1.0f),XMFLOAT4(Colors::Red)},
		{XMFLOAT3(-1.0f,+1.0f,-1.0f),XMFLOAT4(Colors::Red)},
		{XMFLOAT3(+1.0f,+1.0f,-1.0f),XMFLOAT4(Colors::Red)},
		{XMFLOAT3(+1.0f,-1.0f,-1.0f),XMFLOAT4(Colors::Red)},
		{XMFLOAT3(-1.0f,-1.0f,+1.0f),XMFLOAT4(Colors::Red)},
		{XMFLOAT3(-1.0f,+1.0f,+1.0f),XMFLOAT4(Colors::Red)},
		{XMFLOAT3(+1.0f,+1.0f,+1.0f),XMFLOAT4(Colors::Red)},
		{XMFLOAT3(+1.0f,-1.0f,+1.0f),XMFLOAT4(Colors::Red)}
	};

	indices = {
		0,1,2,
		0,2,3,
		4,6,5,
		5,7,6,
		4,5,1,
		4,1,0,
		3,2,6,
		3,6,7,
		1,5,6,
		1,6,2,
		4,0,3,
		4,3,7


	};

	size_t sv = sizeof(Vertex);
	size_t nvertices = vertices.size();
	vsize = static_cast<UINT>(nvertices * sv);
	size_t sind = sizeof(unsigned int);
	size_t nindices = indices.size();
	isize = static_cast<UINT>(nindices * sind);
	
}

Mesh::Mesh(std::string const fileName) : defaultColor(XMFLOAT4(Colors::Coral)) {
	
	try {
		readObjFile(fileName);

	}
	catch (winrt::hresult_error &error) {
		ShowWinRTError(error);
	}
	size_t sv = sizeof(Vertex);
	size_t nvertices = vertices.size();
	vsize = static_cast<UINT>(nvertices * sv);
	size_t sind = sizeof(unsigned int);
	size_t nindices = indices.size();
	isize = static_cast<UINT>(nindices * sind);
}


Mesh::~Mesh()
{
}

UINT Mesh::GetVSize() const {
	return vsize;
}

UINT Mesh::GetISize() const {
	return isize;
}

void Mesh::readFile(std::string const fileName) {
	std::ifstream file(fileName, std::fstream::in);
	if (!file.good())
		return;
	unsigned int nvertices;
	vertices.clear();
	indices.clear();
	file >> nvertices;
	float vec[3]{};
	for (unsigned int i = 0; i < nvertices; i++) {
		file >> vec[0] >> vec[1] >> vec[2];
		Vertex v;
		v.pos = XMFLOAT3(vec[0], vec[1], vec[2]);
		v.col = defaultColor;
		vertices.push_back(v);
	}

	auto cont = 0;
	for (unsigned int i = 0; i < nvertices; i++) {
		file >> vec[0] >> vec[1] >> vec[2];
		vertices[cont++].normal = XMFLOAT3(vec[0], vec[1], vec[2]);
	}
	
	unsigned int nindices;
	file >> nindices;
	for (unsigned int i = 0; i < nindices; i++) {
		unsigned int ind;
		file >> ind;
		indices.push_back(ind);
	}
	isize = static_cast<UINT>(indices.size()*sizeof(unsigned int));
	for (unsigned int i = 0; i < nvertices; i++) {
		float u, v;
		file >> u >> v;
		XMFLOAT2 uv = XMFLOAT2(u, v);
		vertices[i].uvcoords = uv;
	}
	vsize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
	file.close();

}

void Mesh::readObjFile(std::string const  inputfile) {
	
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = "./"; // Path to material files

	tinyobj::ObjReader reader;
	
	if (!reader.ParseFromFile(inputfile, reader_config)) {
		if (!reader.Error().empty()) {
				std::cerr << "TinyObjReader: " << reader.Error();
		}
		
		std::wstring inputfileName(inputfile.begin(),inputfile.end());
		std::wostringstream message;
		message << L"Object File" << inputfileName << "not found or wrong format" << std::endl;
	
		winrt::hresult_error error{ HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), message.str() };
		throw error;
		//winrt::throw_last_error();
	}
	

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	//auto& materials = reader.GetMaterials(); Materials will be programmed in the game

	// We don't loop over shapes, we are asuming one shape per file
	size_t s = 0;
	// Loop over faces(polygon) to get indexes
	size_t index_offset = 0;
	unsigned int countIndex = 0;
	std::map<unsigned int, tinyobj::index_t> mapTOVITOGI; // Map connecting TinyObject vertex index to TinyObject general index
	std::map<unsigned int, unsigned int>  mapTOVIIndex; // Map connecting Tiny Object Vertex index to out mesh vertices index
	// This code requires these mapping to avoid assumming consecutive vertex indexes in the obj file
	for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
		int fv = shapes[s].mesh.num_face_vertices[f];
		for (size_t v = 0; v < fv; v++) {
			unsigned int index;
			tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
			index = static_cast<unsigned int>(idx.vertex_index);
			
			mapTOVITOGI[index] = idx;
			if (mapTOVIIndex.find(index) == mapTOVIIndex.end()) { // if this Tinyobject vertex index has not been processed
				mapTOVIIndex[index] = countIndex++;
			}
			this->indices.push_back(mapTOVIIndex[index]);
		}
		index_offset += fv;
	}
	// Prepare vertice vector to allocate enough elements.
	vertices.resize(mapTOVIIndex.size());
	for (auto it = mapTOVIIndex.begin(); it != mapTOVIIndex.end(); it++) {
		unsigned int vid = it->first;
		unsigned int index = it->second;
		auto fresult = mapTOVITOGI.find(vid);
		if (fresult == mapTOVITOGI.end()) // The vertex index is not in the map
			break;
		// If everything is ok, we get the general index in tinyobj representation
		tinyobj::index_t idx = fresult->second;

		// With this general index we can compose the vertex structure
		Vertex vertex;

		vertex.pos.x = attrib.vertices[3 * idx.vertex_index + 0];
		vertex.pos.y = attrib.vertices[3 * idx.vertex_index + 1];
		vertex.pos.z = attrib.vertices[3 * idx.vertex_index + 2];
		vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
		vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
		vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
		vertex.uvcoords.x = attrib.texcoords[2 * idx.texcoord_index + 0];
		vertex.uvcoords.y = attrib.texcoords[2 * idx.texcoord_index + 1];
		vertex.col = defaultColor;
		vertex.material = XMUINT3(0,0,0);

		vertices[index] = vertex; //Copy vertex

	}

	



	
	
}