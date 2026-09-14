#pragma once

#include <string>
#include <unordered_map>

#include "Dependencies\glew.h"

// Keeps one GPU vertex buffer per key. A mesh that has already been uploaded
// under a given key is handed back as-is instead of being created again.
class MeshCache
{
public:
	~MeshCache();

	GLuint GetOrCreate(const std::string& key, const float* vertexData, size_t byteSize);

private:
	std::unordered_map<std::string, GLuint> m_Buffers;
};
