#include "stdafx.h"
#include "Shader.h"
#include <fstream>
#include <iostream>

Shader::~Shader()
{
	glDeleteProgram(m_Program);
}

bool Shader::ReadFile(const char* filename, std::string* target)
{
	std::ifstream file(filename);
	if (file.fail())
	{
		std::cout << filename << " file loading failed..\n";
		return false;
	}

	std::string line;
	while (getline(file, line))
	{
		target->append(line.c_str());
		target->append("\n");
	}
	return true;
}

GLuint Shader::CompileStage(const char* source, GLenum stage)
{
	GLuint stageObj = glCreateShader(stage);
	if (stageObj == 0)
	{
		fprintf(stderr, "Error creating shader type %d\n", stage);
		return 0;
	}

	const GLchar* p[1] = { source };
	GLint lengths[1] = { (GLint)strlen(source) };

	glShaderSource(stageObj, 1, p, lengths);
	glCompileShader(stageObj);

	GLint success = 0;
	glGetShaderiv(stageObj, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[1024];
		glGetShaderInfoLog(stageObj, 1024, NULL, infoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", stage, infoLog);
	}

	return stageObj;
}

bool Shader::Load(const char* vsFilename, const char* fsFilename)
{
	std::string vs, fs;
	if (!ReadFile(vsFilename, &vs) || !ReadFile(fsFilename, &fs))
	{
		return false;
	}

	m_Program = glCreateProgram();
	if (m_Program == 0)
	{
		fprintf(stderr, "Error creating shader program\n");
		return false;
	}

	GLuint vsObj = CompileStage(vs.c_str(), GL_VERTEX_SHADER);
	GLuint fsObj = CompileStage(fs.c_str(), GL_FRAGMENT_SHADER);
	glAttachShader(m_Program, vsObj);
	glAttachShader(m_Program, fsObj);

	GLint success = 0;
	GLchar errorLog[1024] = { 0 };

	glLinkProgram(m_Program);

	// The individual stages are attached to the program now; it keeps them
	// alive as long as it needs them, so they do not need to live on their own.
	glDeleteShader(vsObj);
	glDeleteShader(fsObj);

	glGetProgramiv(m_Program, GL_LINK_STATUS, &success);
	if (success == 0)
	{
		glGetProgramInfoLog(m_Program, sizeof(errorLog), NULL, errorLog);
		std::cout << vsFilename << ", " << fsFilename << " error linking shader program\n" << errorLog;
		return false;
	}

	glValidateProgram(m_Program);
	glGetProgramiv(m_Program, GL_VALIDATE_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(m_Program, sizeof(errorLog), NULL, errorLog);
		std::cout << vsFilename << ", " << fsFilename << " error validating shader program\n" << errorLog;
		return false;
	}

	std::cout << vsFilename << ", " << fsFilename << " shader compiling is done.\n";
	return true;
}

void Shader::Use() const
{
	glUseProgram(m_Program);
}

GLint Shader::GetUniformLocation(const char* name) const
{
	return glGetUniformLocation(m_Program, name);
}

GLint Shader::GetAttribLocation(const char* name) const
{
	return glGetAttribLocation(m_Program, name);
}
