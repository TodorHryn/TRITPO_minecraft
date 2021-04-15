#pragma once

#include <string>
#include "glad\glad.h"

class Texture {
	public:
		Texture();
		explicit Texture(std::string file, GLint format = GL_RGB);
		~Texture();

		void load(std::string file, GLint format = GL_RGB);

		void bind();

	private:
		GLuint m_texture;
};