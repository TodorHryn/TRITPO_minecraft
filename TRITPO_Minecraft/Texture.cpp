#include "Texture.h"

#include <iostream>
#include "stb_image.h"

Texture::Texture() {
}

Texture::Texture(std::string file, GLint format) {
	load(file, format);
}

Texture::~Texture() {
	glDeleteTextures(1, &m_texture);
}

void Texture::load(std::string file, GLint format) {
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, n;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *img = stbi_load(file.c_str(), &width, &height, &n, (format == GL_RGB) ? 3 : 4);
	if (!img) {
		std::cout << "Can't load image " << file << ": " << std::endl;
		std::cout << stbi_failure_reason() << std::endl;
	}
	stbi_set_flip_vertically_on_load(false);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, img);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(img);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind() {
	glBindTexture(GL_TEXTURE_2D, m_texture);
}
