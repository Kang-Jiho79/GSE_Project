#pragma once

#include <string>

#include "Dependencies\glew.h"

// One compiled and linked GLSL program, loaded from a pair of .vs/.fs files.
// Owns the GL program object for its lifetime.
class Shader
{
public:
	~Shader();

	bool Load(const char* vsFilename, const char* fsFilename);

	void Use() const;
	GLint GetUniformLocation(const char* name) const;
	GLint GetAttribLocation(const char* name) const;

private:
	bool ReadFile(const char* filename, std::string* target);
	GLuint CompileStage(const char* source, GLenum stage);

	GLuint m_Program = 0;
};
