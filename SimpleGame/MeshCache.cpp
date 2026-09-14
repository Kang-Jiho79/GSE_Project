#include "stdafx.h"
#include "MeshCache.h"

MeshCache::~MeshCache()
{
	for (auto it = m_Buffers.begin(); it != m_Buffers.end(); ++it)
	{
		GLuint vbo = it->second;
		glDeleteBuffers(1, &vbo);
	}
}

GLuint MeshCache::GetOrCreate(const std::string& key, const float* vertexData, size_t byteSize)
{
	auto found = m_Buffers.find(key);
	if (found != m_Buffers.end())
	{
		return found->second;
	}

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, byteSize, vertexData, GL_STATIC_DRAW);

	m_Buffers[key] = vbo;
	return vbo;
}
