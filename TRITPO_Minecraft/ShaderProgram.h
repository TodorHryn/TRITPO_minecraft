#pragma once

#include <string>
#include "glm\glm.hpp"
#include "glad\glad.h"

class ShaderProgram {
	public:
		ShaderProgram();
		explicit ShaderProgram(std::string name);
		~ShaderProgram();

		void load(std::string name);

		void use();
		GLuint get();

		void set1f(std::string name, float value);
		void set3fv(std::string name, const glm::vec3 &vector);
		void setMatrix4fv(std::string name, const glm::mat4 &matrix);
		void setMatrix4fv(std::string name, float *matrix);

	private:
		GLuint m_shaderProgram;
};