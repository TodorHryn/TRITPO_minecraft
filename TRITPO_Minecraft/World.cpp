#include "World.h"
#include "main.h"
#include "assert.h"
#include "WorldGeneration.hpp"

Chunk* World::add_chunk(int x, int y, int z) {
	Chunk *result = allocator->malloc();

    if (result)
    {
        result->x = x;
        result->y = y;
        result->z = z;
		result->changed = false;
		result->render = true;
        result->nblocks = 0;
		
        for (int i = 0; i < BLOCKS_IN_CHUNK; i++)
        {
            result->blocks[i] = BLOCK_AIR;
        }

        for (int i = 0; i < BLOCK_TYPE_COUNT; i++)
        {
            result->meshes[i].num_of_vs = 0;
            result->meshes[i].vao = 0;
            result->meshes[i].vbo = 0;
        }

		visible_chunks.push_back(result);
    }

    return (result);
}

void World::load_chunk(int x, int y, int z) {
	for (int i = unloaded_chunks.size() - 1; i >= 0; --i) {
		Chunk *c = unloaded_chunks[i];

		if (c->x == x && c->y == y && c->z == z) {
			unloaded_chunks.erase(unloaded_chunks.begin() + i);
			visible_chunks.push_back(c);
			push_chunk_for_rebuild(c);
			return;
		}
	}

	generate_chunk(*this, x, y, z);
}

void World::unload_chunk(int chunk_id) {
	Chunk *c = visible_chunks[chunk_id];

	if (!c->changed) {
		c->free_mesh();
		visible_chunks.erase(visible_chunks.begin() + chunk_id);
		allocator->free(c);
	}
	else {
		c->free_mesh();
		visible_chunks.erase(visible_chunks.begin() + chunk_id);
		unloaded_chunks.push_back(c);
	}
}

void World::push_chunk_for_rebuild(Chunk *c) {
	rebuild_stack.push(c);
}

Chunk* World::pop_chunk_for_rebuild() {
	assert(!rebuild_stack.empty());
	Chunk *c = rebuild_stack.top();
	rebuild_stack.pop();

    return c;
}
