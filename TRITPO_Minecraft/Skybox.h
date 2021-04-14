#pragma once

#include <string>

class Skybox {
	public:
		Skybox();
		explicit Skybox(std::string filename);

		void load(std::string filename);

		unsigned int texture();

	private:
		unsigned int textureId;
};