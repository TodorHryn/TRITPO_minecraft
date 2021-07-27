#pragma once

#include <vector>
#include <stack>
#include "Chunk.h"
#include "PoolAllocator.hpp"

class Game_state;

class World {
	public:
		Chunk* add_chunk(int x, int y, int z);
		void load_chunk(int x, int y, int z);
		void unload_chunk(int chunk_id);
		void push_chunk_for_rebuild(Chunk *c);
		Chunk* pop_chunk_for_rebuild();

		std::vector<Chunk*> visible_chunks;
		std::vector<Chunk*> unloaded_chunks;
		std::stack<Chunk*> rebuild_stack;

		PoolAllocator<Chunk> *allocator;
};