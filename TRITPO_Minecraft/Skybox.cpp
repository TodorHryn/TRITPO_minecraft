#include "Skybox.h"
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include "stb_image.h"

Skybox::Skybox(std::string filename) {
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

	stbi_set_flip_vertically_on_load(true);
	int width, height, n;

	for (unsigned int i = 0; i < 6; ++i) {
		unsigned char *img = stbi_load((filename + "_" + std::to_string(i) + ".png").c_str(), &width, &height, &n, 3);
		if (!img) {
			std::cout << "Can't load image " << filename + "_" + std::to_string(i) + ".png" << ": " << std::endl;
			std::cout << stbi_failure_reason() << std::endl;
			continue;
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img);

		stbi_image_free(img);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

unsigned int Skybox::texture() {
	return textureId;
}
