#include "pch.h"
#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION 
#include "tiny_obj_loader.h"
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

	unsigned int sv = sizeof(Vertex);
	unsigned int nvertices = vertices.size();
	vsize = nvertices * sv;
	unsigned int sind = sizeof(unsigned int);
	unsigned int nindices = indices.size();
	isize = nindices * sind;
}

Mesh::Mesh(std::string const fileName) : defaultColor(XMFLOAT4(Colors::Coral)) {
	try {
		readObjFile(fileName);
	}
	catch (winrt::hresult_error error) {
		HRESULT code = error.code();
		winrt::hstring msg = error.message();
		std::wcerr << "Error message: " << static_cast<std::wstring>(msg);
		winrt::throw_hresult(code);
	}
}


Mesh::~Mesh()
{
}

UINT64 Mesh::GetVSize() const {
	return vsize;
}

UINT64 Mesh::GetISize() const {
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
	for (auto i = 0; i < nvertices; i++) {
		file >> vec[0] >> vec[1] >> vec[2];
		Vertex v;
		v.pos = XMFLOAT3(vec[0], vec[1], vec[2]);
		v.col = defaultColor;
		vertices.push_back(v);
	}

	auto cont = 0;
	for (auto i = 0; i < nvertices; i++) {
		file >> vec[0] >> vec[1] >> vec[2];
		vertices[cont++].normal = XMFLOAT3(vec[0], vec[1], vec[2]);
	}
	
	unsigned int nindices;
	file >> nindices;
	for (auto i = 0; i < nindices; i++) {
		unsigned int ind;
		file >> ind;
		indices.push_back(ind);
	}
	isize = indices.size()*sizeof(unsigned int);
	for (auto i = 0; i < nvertices; i++) {
		float u, v;
		file >> u >> v;
		XMFLOAT2 uv = XMFLOAT2(u, v);
		vertices[i].uvcoords = uv;
	}
	vsize = vertices.size() * sizeof(Vertex);
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
		winrt::hresult_error error{ E_FAIL, static_cast<std::wstring>(L"File not found") };
		throw error;
		//winrt::throw_last_error();
	}
	

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
				// Optional: vertex colors
				// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
				// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
				// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}
}