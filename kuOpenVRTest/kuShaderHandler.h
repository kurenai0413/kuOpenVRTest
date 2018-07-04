#ifndef KUSHADERHANDLER_H
#define KUSHADERHANDLER_H

#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GLEW/glew.h>

#include "kuLogger.h"

enum ShaderType { VERTEX, FRAGMENT, TESSELLATION, GEOMETRY };

class kuShaderHandler
{
public:
	
	kuShaderHandler();
	kuShaderHandler(const char * VSPathName, const char * FSPathName);		// Vertex Shader, Fragment Shader
	~kuShaderHandler();

	bool		Load(const char * VSPathName, const char * FSPathName);
	bool		Use();
	GLuint		GetShaderProgramID() { return this->m_ShaderProgramID; };

private:

	GLuint		m_ShaderProgramID;
	bool		m_fShaderCreated;

	bool		CreateShader(GLuint &shaderID, ShaderType shaderType, const char * shaderPath);
	bool		CompileShader(GLuint shaderID, const GLchar * shaderCode);
};

#endif