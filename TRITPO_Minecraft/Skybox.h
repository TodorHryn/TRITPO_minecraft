#pragma once

#include <string>

class Skybox {
	public:
		explicit Skybox(std::string filename);

		unsigned int texture();

	private:
		unsigned int textureId;
};