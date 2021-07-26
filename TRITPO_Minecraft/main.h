#pragma once

#include <vector>
#include "3DMath.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include "Skybox.h"
#include "ShadowMap.h"
#include "PoolAllocator.hpp"

#define MEMORY_KB(x) ((x) * 1024ull)
#define MEMORY_MB(x) MEMORY_KB((x) * 1024ull)
#define MEMORY_GB(x) MEMORY_MB((x) * 1024ull)
#define TO_RADIANS(deg) ((PI / 180.0f) * deg)
#define ALIGN_UP(n, k) (((n) + (k) - 1) & ((~(k)) + 1))
#define ALIGN_PTR_UP(n, k) ALIGN_UP((uint64_t)(n), (k))

#define MAX_CHUNKS 2048
#define TRANSIENT_MEM_SIZE MEMORY_GB(1)

#define CHUNK_DIM_LOG2 4
#define WORLD_RADIUS 3
#define GENERATION_Y_RADIUS 4
#define WORLD_SEED 0x7b447dc7

#define CHUNK_DIM (1 << CHUNK_DIM_LOG2)
#define BLOCKS_IN_CHUNK ((CHUNK_DIM) * (CHUNK_DIM) * (CHUNK_DIM))

struct Range3d
{
    uint8_t type;

    int start_x;
    int start_y;
    int start_z;

    int end_x;
    int end_y;
    int end_z;
};

struct Button
{
    int is_pressed;
    int was_pressed;
};

struct Game_input
{
    float aspect_ratio;
	float window_width;
	float window_height;
    float dt;

    float mouse_dx;
    float mouse_dy;
    
    union
    {
        struct
        {
            Button mleft;
            Button mright;

            Button w;
            Button a;
            Button s;
            Button d;

            Button enter;
            Button space;
            Button lshift;

            Button n1;
            Button n2;
            Button n3;
            Button n4;
        };

        Button buttons[13];
    };
};

enum Block_type
{
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_SNOW,
    BLOCK_AIR,
   
    BLOCK_TYPE_COUNT = BLOCK_AIR,
};
static_assert(BLOCK_TYPE_COUNT < 255, "Block_type has more than 255 block types, this won't fit in uint8_t");

Vec3f Block_colors[BLOCK_TYPE_COUNT] =
{
	Vec3f(0, 1, 0),
	Vec3f(130 / 255.0f, 108 / 255.0f, 47 / 255.0f),
	Vec3f(0.4f, 0.4f, 0.4f),
	Vec3f(1, 1, 1)
};

struct Mesh
{
    int num_of_vs;
    GLuint vao;
    GLuint vbo;
};

struct Chunk
{
    int x;
    int y;
    int z;
    int nblocks;
    uint8_t blocks[CHUNK_DIM * CHUNK_DIM * CHUNK_DIM];
    Mesh meshes[BLOCK_TYPE_COUNT];
};

struct World
{
    std::vector<Chunk*> chunks;
	std::stack<Chunk*> rebuild_stack;
};

struct Game_state
{
	PoolAllocator<Chunk> *chunkAllocator;

	Skybox skybox;
    ShaderProgram skyboxSP;
	ShaderProgram sunSP;
	ShaderProgram imageSP;
	ShaderProgram inventoryBlockSP;
	ShaderProgram mesh_sp;
	ShaderProgram meshShadowMapSP;
	ShaderProgram fontCharacterSP;
	ShadowMap shadowMap1;
	ShadowMap shadowMap2;
	ShadowMap shadowMap3;
	ShadowMap shadowMap4;
	Texture sunTexture;
	Texture inventoryBarTexture;
	Texture crossTexture;
	Texture fontTexture;
	GLuint cubeVAO;
    GLuint cubeVBO;
	GLuint squareVAO;
	GLuint squareVBO;

    Vec3f cam_pos;
    Vec3f cam_view_dir;
    Vec3f cam_move_dir;
    Vec3f cam_up;
    Vec3f cam_rot;

    uint8_t block_to_place;

	int frameCount;
	float fpsCounterPrevTime;
	float fps;

    World world;
};

struct Game_memory
{
    int is_initialized;

    uint64_t transient_mem_size;
    void *transient_mem;

	Game_state *game_state;

	Range3d ranges[BLOCKS_IN_CHUNK];
	uint8_t visited[BLOCKS_IN_CHUNK];
	PoolAllocator<Chunk> *chunkAllocator;
};

struct Raycast_result
{
    bool collision;
    float last_t;
    Chunk *chunk;

    int i;
    int j;
    int k;
    
    int last_i;
    int last_j;
    int last_k;
};

