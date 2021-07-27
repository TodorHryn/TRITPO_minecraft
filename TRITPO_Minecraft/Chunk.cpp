#include "Chunk.h"
#include "assert.h"

void Chunk::free_mesh() {
	for (Mesh &m : meshes) {
		if (m.vao != 0)
		{
			assert(m.vao && m.vbo);

			glDeleteVertexArrays(1, &m.vao);
			glDeleteBuffers(1, &m.vbo);

			m.num_of_vs = 0;
			m.vao = 0;
			m.vbo = 0;
		}
	}
}
