#pragma once

#include <cstdint>
#include "Mesh.h"
#include "Blocks.h"

#define CHUNK_DIM_LOG2 4
#define CHUNK_DIM (1 << CHUNK_DIM_LOG2)
#define BLOCKS_IN_CHUNK ((CHUNK_DIM) * (CHUNK_DIM) * (CHUNK_DIM))

class Chunk {
	public:
		void free_mesh();

	    int x;
	    int y;
	    int z;
	    int nblocks;
		bool changed;
		uint8_t blocks[CHUNK_DIM * CHUNK_DIM * CHUNK_DIM];
		Mesh meshes[BLOCK_TYPE_COUNT];
};