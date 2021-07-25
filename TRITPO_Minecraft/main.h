#pragma once

#include <stdint.h>

#include "glad\glad.h"
#include "GLFW\glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glad.c"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"

#define TRANSIENT_MEM_SIZE MEMORY_GB(1)
#define PERMANENT_MEM_SIZE MEMORY_MB(32)
#define WORLD_RADIUS 8
#define CHUNK_DIM_LOG2 4
#define WORLD_SEED 0x7b447dc7

#define CHUNK_DIM (1 << CHUNK_DIM_LOG2)
#define BLOCKS_IN_CHUNK ((CHUNK_DIM) * (CHUNK_DIM) * (CHUNK_DIM))
#define MEMORY_KB(x) ((x) * 1024ull)
#define MEMORY_MB(x) MEMORY_KB((x) * 1024ull)
#define MEMORY_GB(x) MEMORY_MB((x) * 1024ull)
#define TO_RADIANS(deg) ((PI / 180.0f) * deg)
#define ALIGN_UP(n, k) (((n) + (k) - 1) & ((~(k)) + 1))
#define ALIGN_PTR_UP(n, k) ALIGN_UP((uint64_t)(n), (k))

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

struct Game_memory
{
    int is_initialized;

    uint64_t permanent_mem_size;
    void *permanent_mem;

    uint64_t transient_mem_size;
    void *transient_mem;
};

struct Memory_arena
{
    uint8_t *curr;
    uint8_t *end;
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
    Chunk *next;
    int nblocks;
    uint8_t *blocks;
    Mesh meshes[BLOCK_TYPE_COUNT];
};

struct World
{
    Chunk *next;

	std::stack<Chunk*> rebuild_stack;
};

struct Game_state
{
    Memory_arena arena;

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

void *memory_arena_alloc(Memory_arena *arena, int64_t size);
inline void *memory_arena_get_cursor(Memory_arena *arena);
inline void memory_arena_set_cursor(Memory_arena *arena, void *cursor);
void world_push_chunk_for_rebuild(World *w, Chunk *c);
Chunk* world_pop_chunk_for_rebuild(World *w);
Chunk *world_add_chunk(World *world, Memory_arena *arena, int x, int y, int z);
Raycast_result raycast(World *world, Vec3f pos, Vec3f view_dir);
void gen_ranges_3d(uint8_t *blocks, Range3d *ranges, uint8_t *visited, int dim, int count, int *num_of_ranges);

void game_state_and_memory_init(Game_memory *memory);
void renderWorld(Game_state *state, ShaderProgram &sp);
void drawText(Game_state *state, Game_input *input, std::string text, float x, float y, float scale);
void rebuild_chunk(Game_memory *memory, Chunk *chunk);
void game_update_and_render(Game_input *input, Game_memory *memory);
