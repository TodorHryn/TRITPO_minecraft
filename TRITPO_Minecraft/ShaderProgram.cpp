#include "ShaderProgram.h"
#include <fstream>
#include <iostream>
#include "glad\glad.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include "glm\gtx\string_cast.hpp"

#define CHECK_SHADER(shader)																		\
	do {																							\
		GLint success;																				\
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);											\
		if (!success) {																				\
			std::cout << "Shader compilation failed (line " << __LINE__ << "):" << std::endl;		\
			GLchar info[512];																		\
			glGetShaderInfoLog(shader, 512, NULL, info);											\
			std::cout << info << std::endl;															\
		}																							\
	}																								\
	while(0)																						


#define CHECK_PROGRAM(program)																		\
	do {																							\
		GLint success;																				\
		glGetProgramiv(program, GL_LINK_STATUS, &success);											\
		if (!success) {																				\
			std::cout << "Program link failed (line " << __LINE__ << "):" << std::endl;				\
			GLchar info[512];																		\
			glGetProgramInfoLog(program, 512, NULL, info);											\
			std::cout << info << std::endl;															\
		}																							\
	}																								\
	while(0)	

ShaderProgram::ShaderProgram() {
}

ShaderProgram::ShaderProgram(std::string name) {
	load(name);
}

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(m_shaderProgram);
}

void ShaderProgram::load(std::string name) {
	std::ifstream vertexShaderFile(name  + ".vert");
	std::string vertexShaderString((std::istreambuf_iterator<char>(vertexShaderFile)), std::istreambuf_iterator<char>());
	const char *vertexShaderSource = vertexShaderString.c_str();

	std::ifstream fragmentShaderFile(name + ".frag");
	std::string fragmentShaderString((std::istreambuf_iterator<char>(fragmentShaderFile)), std::istreambuf_iterator<char>());
	const char *fragmentShaderSource = fragmentShaderString.c_str();

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	CHECK_SHADER(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	CHECK_SHADER(fragmentShader);

	m_shaderProgram = glCreateProgram();
	glAttachShader(m_shaderProgram, vertexShader);
	glAttachShader(m_shaderProgram, fragmentShader);
	glLinkProgram(m_shaderProgram);
	CHECK_PROGRAM(m_shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void ShaderProgram::use() {
	glUseProgram(m_shaderProgram);
}

GLuint ShaderProgram::get() {
	return m_shaderProgram;
}

void ShaderProgram::set1f(std::string name, float value) {
	glUniform1f(glGetUniformLocation(m_shaderProgram, name.c_str()), value);
}

void ShaderProgram::set3fv(std::string name, const glm::vec3 &vector) {
	glUniform3fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, glm::value_ptr(vector));
}

void ShaderProgram::setMatrix4fv(std::string name, const glm::mat4 &matrix) {
	glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void ShaderProgram::setMatrix4fv(std::string name, float * matrix) {
	glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, GL_FALSE, matrix);
}
