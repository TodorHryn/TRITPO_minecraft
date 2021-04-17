#pragma once
#include "glad\glad.h"

class ShadowMap {
	public:
		ShadowMap(unsigned int width, unsigned int height);

		void bind();
		void unbind();
		unsigned int get();

	private:
		unsigned int m_depthMapFBO;
		unsigned int m_depthMap;
		unsigned int m_width, m_height;

		GLint oldViewportDims[4];
};