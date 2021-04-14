#include <stdio.h> // sprintf
#include <assert.h>
#include <iostream>

#include "glad\glad.h"
#include "GLFW\glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glad.c"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include "ShaderProgram.h"
#include "Skybox.h"

#include "3DMath.h"

#define MEMORY_KB(x) ((x) * 1024ull)
#define MEMORY_MB(x) MEMORY_KB((x) * 1024ull)
#define MEMORY_GB(x) MEMORY_MB((x) * 1024ull)
#define TO_RADIANS(deg) ((PI / 180.0f) * deg)

struct Button
{
    int is_pressed;
    int was_pressed;
};

struct Game_input
{
    float aspect_ratio;
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
        };

        Button buttons[9];
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

#define ALIGN_UP(n, k) (((n) + (k) - 1) & ((~(k)) + 1))
#define ALIGN_PTR_UP(n, k) ALIGN_UP((uint64_t)(n), (k))
void *memory_arena_alloc(Memory_arena *arena, int64_t size)
{
    assert(size > 0);
    assert((uint64_t)arena->curr % 8 == 0);

    if (arena->end < (arena->curr + size))
    {
        return (0);
    }

    uint8_t *result = arena->curr;
    arena->curr = (uint8_t *)ALIGN_PTR_UP(result + size, 8);

    return (result);
}

inline void *memory_arena_get_cursor(Memory_arena *arena)
{
    return (arena->curr);
}

inline void memory_arena_set_cursor(Memory_arena *arena, void *cursor)
{
    assert(cursor <= (void *)arena->end);
    arena->curr = (uint8_t *)cursor;
}

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

struct Mesh
{
    int num_of_vs;
    GLuint vao;
    GLuint vbo;
};

#define CHUNK_DIM_LOG2 4
#define CHUNK_DIM (1 << 4)
#define BLOCKS_IN_CHUNK ((CHUNK_DIM) * (CHUNK_DIM) * (CHUNK_DIM))

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

#define REBUILD_STACK_SIZE 32
struct World
{
    Chunk *next;

    int rebuild_stack_top;
    Chunk *rebuild_stack[REBUILD_STACK_SIZE];
};

void world_push_chunk_for_rebuild(World *w, Chunk *c)
{
    assert(w->rebuild_stack_top < REBUILD_STACK_SIZE);
    w->rebuild_stack[w->rebuild_stack_top++] = c;
}

Chunk *world_pop_chunk_for_rebuild(World *w)
{
    assert(w->rebuild_stack_top > 0);
    return w->rebuild_stack[--w->rebuild_stack_top];
}

Chunk *world_add_chunk(World *world, Memory_arena *arena, int x, int y, int z)
{
    Chunk *result = (Chunk *)memory_arena_alloc(arena, sizeof(Chunk) + (CHUNK_DIM * CHUNK_DIM * CHUNK_DIM));

    if (result)
    {
        result->x = x;
        result->y = y;
        result->z = z;
        result->next = world->next;
        result->nblocks = 0;
        result->blocks = (uint8_t *)&result[1];

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

        world->next = result;
    }

    return (result);
}

//unsigned int skyboxVAO, skyboxVBO;
//ShaderProgram skyboxSP;
//Skybox skybox;

struct Game_state
{
    Memory_arena arena;

    ShaderProgram mesh_sp;

    ShaderProgram skyboxSP;
    Skybox skybox;
    GLuint skyboxVAO;
    GLuint skyboxVBO;

    Vec3f cam_pos;
    Vec3f cam_view_dir;
    Vec3f cam_move_dir;
    Vec3f cam_up;
    Vec3f cam_rot;

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

Raycast_result raycast(World *world, Vec3f pos, Vec3f view_dir)
{
    Raycast_result result = {};

    bool collision = false;
    float last_t = 0.0f;
    
    float tile_dim = 1.0f;
    float tolerance = 0.001f;

    float start_x = pos.x;
    float start_y = pos.y;
    float start_z = pos.z;

    float max_ray_len = 3.0f;
    Vec3f end_point = pos + max_ray_len * view_dir;
    float end_x = end_point.x;
    float end_y = end_point.y;
    float end_z = end_point.z;

    int i = (int)floorf(start_x / tile_dim);
    int j = (int)floorf(start_y / tile_dim);
    int k = (int)floorf(start_z / tile_dim);

    int i_end = (int)floorf(end_x / tile_dim);
    int j_end = (int)floorf(end_y / tile_dim);
    int k_end = (int)floorf(end_z / tile_dim);

    int di = (start_x < end_x) ? 1 : ((start_x > end_x) ? -1 : 0);
    int dj = (start_y < end_y) ? 1 : ((start_y > end_y) ? -1 : 0);
    int dk = (start_z < end_z) ? 1 : ((start_z > end_z) ? -1 : 0);

    float min_x = tile_dim * floorf(start_x / tile_dim);
    float max_x = min_x + tile_dim;
    float t_x = INFINITY;
    if (fabs(end_x - start_x) > tolerance)
    {
        t_x = ((start_x < end_x) ? (max_x - start_x) : (start_x - min_x)) / fabsf(end_x - start_x);
    }

    float min_y = tile_dim * floorf(start_y / tile_dim);
    float max_y = min_y + tile_dim;
    float t_y = INFINITY;
    if (fabs(end_y - start_y) > tolerance)
    {
        t_y = ((start_y < end_y) ? (max_y - start_y) : (start_y - min_y)) / fabsf(end_y - start_y);
    }

    float min_z = tile_dim * floorf(start_z / tile_dim);
    float max_z = min_z + tile_dim;
    float t_z = INFINITY;
    if (fabs(end_z - start_z) > tolerance)
    {
        t_z = ((start_z < end_z) ? (max_z - start_z) : (start_z - min_z)) / fabsf(end_z - start_z);
    }

    float dt_x = INFINITY;
    if (fabs(end_x - start_x) > tolerance)
    {
        dt_x = tile_dim / fabsf(end_x - start_x);
    }

    float dt_y = INFINITY;
    if (fabs(end_y - start_y) > tolerance)
    {
        dt_y = tile_dim / fabsf(end_y - start_y);
    }

    float dt_z = INFINITY;
    if (fabs(end_z - start_z) > tolerance)
    {
        dt_z = tile_dim / fabsf(end_z - start_z);
    }

    Chunk *chunk = 0;
    int last_di = 0;
    int last_dj = 0;
    int last_dk = 0;
    for (;;)
    {
        int chunk_x = i >> CHUNK_DIM_LOG2;
        int chunk_y = j >> CHUNK_DIM_LOG2;
        int chunk_z = k >> CHUNK_DIM_LOG2;
        Chunk *c = world->next;
        while (c != 0)
        {
            if ((chunk_x == c->x) && (chunk_y == c->y) && (chunk_z == c->z))
            {
                int mask = ~((~1) << (CHUNK_DIM_LOG2 - 1));
                int block_x = i & mask;
                int block_y = j & mask;
                int block_z = k & mask;

                if (c->blocks[CHUNK_DIM * CHUNK_DIM * block_y + CHUNK_DIM * block_z + block_x] != BLOCK_AIR)
                {
                    chunk = c;
                    collision = true;
                    goto end_loop;
                }
            }

            c = c->next;
        }

        if (i == i_end && j == j_end && k == k_end)
        {
            break;
        }

        if (t_x <= t_y && t_x <= t_z)
        {
            last_di = di;
            last_dj = 0;
            last_dk = 0;

            last_t = t_x;
            t_x += dt_x;
            i += di;
        }
        else if (t_y <= t_x && t_y <= t_z)
        {
            last_di = 0;
            last_dj = dj;
            last_dk = 0;

            last_t = t_y;
            t_y += dt_y;
            j += dj;
        }
        else
        {
            last_di = 0;
            last_dj = 0;
            last_dk = dk;

            last_t = t_z;
            t_z += dt_z;
            k += dk;
        }
    }

end_loop:
    float dx2 = (end_x - start_x) * (end_x - start_x);
    float dy2 = (end_y - start_y) * (end_y - start_y);
    float dz2 = (end_z - start_z) * (end_z - start_z);
    last_t *= sqrtf(dx2 + dy2 + dz2);

    result.collision = collision;
    result.i = i;
    result.j = j;
    result.k = k;
    result.last_i = i - last_di;
    result.last_j = j - last_dj;
    result.last_k = k - last_dk;
    result.last_t = last_t;
    result.chunk = chunk;

    return (result);
}

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

void gen_ranges_3d(uint8_t *blocks, Range3d *ranges, uint8_t *visited, int dim, int count, int *num_of_ranges)
{
    int ranges_count = 0;

    while (count > 0)
    {
        int start_z = 0;
        int end_z = 0;

        int start_y = 0;
        int end_y = 0;

        int start_x = 0;
        int end_x = 0;

        // skip all visited and empty blocks
        for (start_y = 0; start_y < dim; start_y++)
        {
            for (start_z = 0; start_z < dim; start_z++)
            {
                for (start_x = 0; start_x < dim; start_x++)
                {
                    if (visited[start_y * dim * dim + start_z * dim + start_x] == 0 &&
                        blocks[start_y * dim * dim + start_z * dim + start_x] != BLOCK_AIR)
                    {
                        goto break1;
                    }
                }
            }
        }
    break1:

        // If a block at (start_x, start_y, start_z) is in the grid (the grid is not empty), mark it as visited.
        // Also record block type.
        uint8_t block_type = BLOCK_AIR;
        if (start_x < dim && start_y < dim && start_z < dim)
        {
            visited[start_y * dim * dim + start_z * dim + start_x] = 1;
            block_type = blocks[start_y * dim * dim + start_z * dim + start_x];
            count--;
        }

        // try expand in x direction
        end_x = start_x;
        while ((end_x + 1 < dim) &&
            (blocks[start_y * dim * dim + start_z * dim + (end_x + 1)] == block_type) &&
            (visited[start_y * dim * dim + start_z * dim + (end_x + 1)] == 0))
        {
            visited[start_y * dim * dim + start_z * dim + (end_x + 1)] = 1;
            end_x++;
            count--;
        }

        // try expand in z direction
        end_z = start_z;
        while (end_z + 1 < dim)
        {
            bool can_expand = true;
            for (int x = start_x; x <= end_x; x++)
            {
                if (blocks[start_y * dim * dim + (end_z + 1) * dim + x] != block_type ||
                    visited[start_y * dim * dim + (end_z + 1) * dim + x] == 1)
                {
                    can_expand = false;
                    break;
                }
            }

            if (can_expand)
            {
                // mark expanded row of block as visited
                for (int x = start_x; x <= end_x; x++)
                {
                    visited[start_y * dim * dim + (end_z + 1) * dim + x] = 1;
                }
                end_z++;
                count -= end_x - start_x + 1;
            }
            else
            {
                break;
            }
        }

        // try expand in y direction
        end_y = start_y;
        while (end_y + 1 < dim)
        {
            bool can_expand = true;
            for (int z = start_z; z <= end_z; z++)
            {
                for (int x = start_x; x <= end_x; x++)
                {
                    if (blocks[(end_y + 1) * dim * dim + z * dim + x] != block_type ||
                        visited[(end_y + 1) * dim * dim + z * dim + x] == 1)
                    {
                        can_expand = false;
                        goto break2;
                    }
                }
            }
        break2:
            if (can_expand)
            {
                for (int z = start_z; z <= end_z; z++)
                {
                    for (int x = start_x; x <= end_x; x++)
                    {
                        visited[(end_y + 1) * dim * dim + z * dim + x] = 1;
                    }
                }
                end_y++;
                count -= (end_x - start_x + 1) * (end_z - start_z + 1);
            }
            else
            {
                break;
            }
        }

        assert(block_type != BLOCK_AIR);
        ranges[ranges_count++] = { block_type, start_x, start_y, start_z, end_x, end_y, end_z };
    }

    assert(count == 0);
    *num_of_ranges = ranges_count;
}

void game_state_and_memory_init(Game_memory *memory)
{
    assert(!memory->is_initialized);

    assert(sizeof(Game_state) <= memory->permanent_mem_size);
    Game_state *state = (Game_state *)memory->permanent_mem;

    memory->is_initialized = 1;

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    state->arena.curr = (uint8_t *)ALIGN_PTR_UP(&state[1], 8);
    state->arena.end = (uint8_t *)memory->permanent_mem + memory->permanent_mem_size;

    state->world.next = 0;
    state->world.rebuild_stack_top = 0;

    {
        Chunk *c = world_add_chunk(&state->world, &state->arena, 0, 0, 0);

        int i = 0;
        for (int y = 0; y < 4; y++)
        {
            for (int z = 0; z < CHUNK_DIM; z++)
            {
                for (int x = 0; x < CHUNK_DIM; x++)
                {
                    c->blocks[CHUNK_DIM * CHUNK_DIM * y + CHUNK_DIM * z + x] = BLOCK_DIRT;
                    c->nblocks++;
                    i++;
                }
            }
        }
        world_push_chunk_for_rebuild(&state->world, c);
    }

    {
        Chunk *c = world_add_chunk(&state->world, &state->arena, -1, 0, -1);

        int i = 0;
        for (int y = 0; y < 6; y++)
        {
            for (int z = 0; z < CHUNK_DIM; z++)
            {
                for (int x = 0; x < CHUNK_DIM; x++)
                {
                    if (i % 5 == 0)
                    {
                        c->blocks[CHUNK_DIM * CHUNK_DIM * y + CHUNK_DIM * z + x] = BLOCK_DIRT;
                        c->nblocks++;
                    }
                    i++;
                }
            }
        }
        world_push_chunk_for_rebuild(&state->world, c);
    }

    {
        Chunk *c = world_add_chunk(&state->world, &state->arena, -1, 0, 0);

        int i = 0;
        for (int y = 0; y < 2; y++)
        {
            for (int z = 0; z < CHUNK_DIM; z++)
            {
                for (int x = 0; x < CHUNK_DIM; x++)
                {
                    c->blocks[CHUNK_DIM * CHUNK_DIM * y + CHUNK_DIM * z + x] = BLOCK_DIRT;
                    c->nblocks++;
                    i++;
                }
            }
        }
        world_push_chunk_for_rebuild(&state->world, c);
    }

    {
        Chunk *c = world_add_chunk(&state->world, &state->arena, 1, -2, -3);

        int i = 0;
        for (int y = 0; y < 8; y++)
        {
            for (int z = 0; z < CHUNK_DIM; z++)
            {
                for (int x = 0; x < CHUNK_DIM; x++)
                {
                    uint8_t block_type;
                    if (y < 2)
                        block_type = BLOCK_STONE;
                    else if (y < 4)
                        block_type = BLOCK_DIRT;
                    else if (y < 6)
                        block_type = BLOCK_GRASS;
                    else
                        block_type = BLOCK_STONE;

                    c->blocks[CHUNK_DIM * CHUNK_DIM * y + CHUNK_DIM * z + x] = block_type;
                    c->nblocks++;
                    i++;
                }
            }
        }
        world_push_chunk_for_rebuild(&state->world, c);
    }

    state->cam_pos = Vec3f(0, 20, 0);
    state->cam_up = Vec3f(0, 1, 0);

    state->cam_rot.pitch = 0.0f;
    state->cam_rot.roll = 0.0f;
    state->cam_rot.yaw = -90.0f;

    state->cam_view_dir.x = cosf(TO_RADIANS(state->cam_rot.yaw)) * cosf(TO_RADIANS(state->cam_rot.pitch));
    state->cam_view_dir.y = sinf(TO_RADIANS(state->cam_rot.pitch));
    state->cam_view_dir.z = sinf(TO_RADIANS(state->cam_rot.yaw)) * cosf(TO_RADIANS(state->cam_rot.pitch));

    state->cam_move_dir.x = state->cam_view_dir.x;
    state->cam_move_dir.y = 0.0f;
    state->cam_move_dir.z = state->cam_view_dir.z;

    // NOTE(max): call constructors on existing memory
    new (&state->mesh_sp) ShaderProgram("mesh");
    new (&state->skyboxSP) ShaderProgram("skybox");
    new (&state->skybox) Skybox("Images/cubemap");

    // skybox VAO
    glGenVertexArrays(1, &state->skyboxVAO);
    glGenBuffers(1, &state->skyboxVBO);
    glBindVertexArray(state->skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state->skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void game_update_and_render(Game_input *input, Game_memory *memory)
{
    assert(memory->is_initialized);
    Game_state *state = (Game_state *)memory->permanent_mem;

    /* logic update */
    {
        state->cam_rot.pitch += input->mouse_dy;
        state->cam_rot.yaw += input->mouse_dx;

        if (state->cam_rot.pitch > 89.0f)
        {
            state->cam_rot.pitch = 89.0f;
        }
        if (state->cam_rot.pitch < -89.0f)
        {
            state->cam_rot.pitch = -89.0f;
        }

        Vec3f view_dir;
        view_dir.x = cosf(TO_RADIANS(state->cam_rot.yaw)) * cosf(TO_RADIANS(state->cam_rot.pitch));
        view_dir.y = sinf(TO_RADIANS(state->cam_rot.pitch));
        view_dir.z = sinf(TO_RADIANS(state->cam_rot.yaw)) * cosf(TO_RADIANS(state->cam_rot.pitch));
        state->cam_view_dir = normalize(view_dir);

        Vec3f move_dir;
        move_dir = state->cam_view_dir;
        move_dir.y = 0.0f;
        state->cam_move_dir = normalize(move_dir);

        float cam_speed = 10.0f * input->dt;
        if (input->w.is_pressed)
        {
            state->cam_pos = state->cam_pos + state->cam_move_dir * cam_speed;
        }
        if (input->s.is_pressed)
        {
            state->cam_pos = state->cam_pos + state->cam_move_dir * -cam_speed;
        }
        
        Vec3f right = cross(state->cam_move_dir, state->cam_up);
        if (input->d.is_pressed)
        {
            state->cam_pos = state->cam_pos + right * cam_speed;
        }
        if (input->a.is_pressed)
        {
            state->cam_pos = state->cam_pos + right * -cam_speed;
        }

        if (input->space.is_pressed)
        {
            state->cam_pos = state->cam_pos + Vec3f(0, cam_speed, 0);
        }
        if (input->lshift.is_pressed)
        {
            state->cam_pos = state->cam_pos + Vec3f(0, -cam_speed, 0);
        }

        // block removal
        if (input->mleft.is_pressed)
        {
            Raycast_result rc = raycast(&state->world, state->cam_pos, state->cam_view_dir);
            if (rc.collision == true)
            {
                int mask = ~((~1) << (CHUNK_DIM_LOG2 - 1));
                int block_x = rc.i & mask;
                int block_y = rc.j & mask;
                int block_z = rc.k & mask;

                int block_idx = CHUNK_DIM * CHUNK_DIM * block_y + CHUNK_DIM * block_z + block_x;
                if (rc.chunk->blocks[block_idx] != BLOCK_AIR)
                {
                    rc.chunk->blocks[block_idx] = BLOCK_AIR;
                    rc.chunk->nblocks--;
                    world_push_chunk_for_rebuild(&state->world, rc.chunk);
                }
            }
        }

        // block placement
        if (input->mright.is_pressed && !input->mright.was_pressed)
        {
            Raycast_result rc = raycast(&state->world, state->cam_pos, state->cam_view_dir);
            if (rc.collision)
            {
                int last_chunk_x = rc.last_i >> CHUNK_DIM_LOG2;
                int last_chunk_y = rc.last_j >> CHUNK_DIM_LOG2;
                int last_chunk_z = rc.last_k >> CHUNK_DIM_LOG2;

                Chunk *prev_chunk = 0;

                Chunk *c = state->world.next;
                while (c != 0)
                {
                    if ((c->x == last_chunk_x) &&
                        (c->y == last_chunk_y) &&
                        (c->z == last_chunk_z))
                    {
                        prev_chunk = c;
                        break;
                    }

                    c = c->next;
                }

                if (!prev_chunk)
                {
                    prev_chunk = world_add_chunk(&state->world, &state->arena, last_chunk_x, last_chunk_y, last_chunk_z);
                }

                if (prev_chunk)
                {
                    int mask = ~((~1) << (CHUNK_DIM_LOG2 - 1));
                    int block_x = rc.last_i & mask;
                    int block_y = rc.last_j & mask;
                    int block_z = rc.last_k & mask;

                    int block_idx = CHUNK_DIM * CHUNK_DIM * block_y + CHUNK_DIM * block_z + block_x;
                    if (prev_chunk->blocks[block_idx] == BLOCK_AIR)
                    {
                        // TODO(max): assing block type number
                        prev_chunk->blocks[block_idx] = BLOCK_GRASS;
                        prev_chunk->nblocks++;
                        world_push_chunk_for_rebuild(&state->world, prev_chunk);
                    }
                }
            }
        }

        Chunk *chunk_to_rebuild = 0;
        if (state->world.rebuild_stack_top > 0)
        {
            chunk_to_rebuild = world_pop_chunk_for_rebuild(&state->world);
        }

        if (chunk_to_rebuild && chunk_to_rebuild->nblocks)
        {
            Memory_arena arena = {};
            arena.curr = (uint8_t *)memory->transient_mem;
            arena.end  = (uint8_t *)memory->transient_mem + memory->transient_mem_size;
        
            Range3d *ranges  = (Range3d *)memory_arena_alloc(&arena, BLOCKS_IN_CHUNK * sizeof(Range3d));
            uint8_t *visited = (uint8_t *)memory_arena_alloc(&arena, BLOCKS_IN_CHUNK * sizeof(uint8_t));
            if (ranges && visited)
            {
                for (int i = 0; i < (BLOCKS_IN_CHUNK); i++) visited[i] = 0;
             
                int nranges = 0;
                gen_ranges_3d(chunk_to_rebuild->blocks, ranges, visited, CHUNK_DIM, chunk_to_rebuild->nblocks, &nranges);

                // NOTE(max): sort ranges by block type
                for (int i = 0; i < nranges - 1; i++)
                {
                    for (int j = 0; j < nranges - i - 1; j++)
                    {
                        if (ranges[j].type > ranges[j + 1].type)
                        {
                            Range3d temp = ranges[j + 1];
                            ranges[j + 1] = ranges[j];
                            ranges[j] = temp;
                        }
                    }
                }

                int rebuilded_mesh_types[BLOCK_TYPE_COUNT] = {};

                int ranges_left = nranges;
                int ranges_idx_start = 0;
                int ranges_idx_end  = 0;
                void *arena_cursor = memory_arena_get_cursor(&arena);
                while (ranges_left > 0)
                {
                    memory_arena_set_cursor(&arena, arena_cursor);

                    int ranges_count = 0;
                    uint8_t range_type = ranges[ranges_idx_start].type;
                    while ((ranges_idx_end < nranges) && ranges[ranges_idx_end].type == range_type)
                    {
                        ranges_count++;
                        ranges_idx_end++;
                    }
                    ranges_left -= ranges_count;

                    assert(range_type < BLOCK_TYPE_COUNT);
                    Mesh *mesh_to_rebuild = &chunk_to_rebuild->meshes[range_type];

                    int vs_arr_size = 3 * 12 * ranges_count * sizeof(Vec3f);
                    int ns_arr_size = 3 * 12 * ranges_count * sizeof(Vec3f);
                    Vec3f *vs = (Vec3f *)memory_arena_alloc(&arena, vs_arr_size + ns_arr_size);
                    printf("%p\n", vs);
                    Vec3f *ns = vs + (3 * 12 * ranges_count);
                    if (vs)
                    {
                        rebuilded_mesh_types[range_type] = 1;

                        int v_idx = 0;
                        mesh_to_rebuild->num_of_vs = 3 * 12 * ranges_count;
                        for (int i = ranges_idx_start; i < ranges_idx_end; i++)
                        {
                            Vec3f base((float)ranges[i].start_x, (float)ranges[i].start_y, (float)ranges[i].start_z);

                            float dim_x = (float)ranges[i].end_x - base.x + 1.0f;
                            float dim_y = (float)ranges[i].end_y - base.y + 1.0f;
                            float dim_z = (float)ranges[i].end_z - base.z + 1.0f;

                            // TODO(max): check for correct winding order

                            Vec3f bottom_n(0, -1, 0);
                            Vec3f top_n(0, 1, 0);
                            Vec3f north_n(0, 0, -1);
                            Vec3f south_n(0, 0, 1);
                            Vec3f west_n(-1, 0, 0);
                            Vec3f east_n(1, 0, 0);

                            // bottom tri 0
                            vs[v_idx + 0 * 3 + 0] = base;
                            vs[v_idx + 0 * 3 + 1] = base + Vec3f(dim_x, 0, 0);
                            vs[v_idx + 0 * 3 + 2] = base + Vec3f(0, 0, dim_z);

                            ns[v_idx + 0 * 3 + 0] = bottom_n;
                            ns[v_idx + 0 * 3 + 1] = bottom_n;
                            ns[v_idx + 0 * 3 + 2] = bottom_n;

                            // bottom tri 1
                            vs[v_idx + 1 * 3 + 0] = base + Vec3f(0, 0, dim_z);
                            vs[v_idx + 1 * 3 + 1] = base + Vec3f(dim_x, 0, 0);
                            vs[v_idx + 1 * 3 + 2] = base + Vec3f(dim_x, 0, dim_z);

                            ns[v_idx + 1 * 3 + 0] = bottom_n;
                            ns[v_idx + 1 * 3 + 1] = bottom_n;
                            ns[v_idx + 1 * 3 + 2] = bottom_n;

                            // top tri 0
                            vs[v_idx + 2 * 3 + 0] = base + Vec3f(0, dim_y, 0);
                            vs[v_idx + 2 * 3 + 1] = base + Vec3f(0, dim_y, dim_z);
                            vs[v_idx + 2 * 3 + 2] = base + Vec3f(dim_x, dim_y, 0);

                            ns[v_idx + 2 * 3 + 0] = top_n;
                            ns[v_idx + 2 * 3 + 1] = top_n;
                            ns[v_idx + 2 * 3 + 2] = top_n;

                            // top tri 1
                            vs[v_idx + 3 * 3 + 0] = base + Vec3f(0, dim_y, dim_z);
                            vs[v_idx + 3 * 3 + 1] = base + Vec3f(dim_x, dim_y, dim_z);
                            vs[v_idx + 3 * 3 + 2] = base + Vec3f(dim_x, dim_y, 0);

                            ns[v_idx + 3 * 3 + 0] = top_n;
                            ns[v_idx + 3 * 3 + 1] = top_n;
                            ns[v_idx + 3 * 3 + 2] = top_n;

                            // north tri 0
                            vs[v_idx + 4 * 3 + 0] = base;
                            vs[v_idx + 4 * 3 + 1] = base + Vec3f(0, dim_y, 0);
                            vs[v_idx + 4 * 3 + 2] = base + Vec3f(dim_x, dim_y, 0);

                            ns[v_idx + 4 * 3 + 0] = north_n;
                            ns[v_idx + 4 * 3 + 1] = north_n;
                            ns[v_idx + 4 * 3 + 2] = north_n;

                            // north tri 1
                            vs[v_idx + 5 * 3 + 0] = base;
                            vs[v_idx + 5 * 3 + 1] = base + Vec3f(dim_x, dim_y, 0);
                            vs[v_idx + 5 * 3 + 2] = base + Vec3f(dim_x, 0, 0);

                            ns[v_idx + 5 * 3 + 0] = north_n;
                            ns[v_idx + 5 * 3 + 1] = north_n;
                            ns[v_idx + 5 * 3 + 2] = north_n;

                            // south tri 0
                            vs[v_idx + 6 * 3 + 0] = base + Vec3f(0, 0, dim_z);
                            vs[v_idx + 6 * 3 + 1] = base + Vec3f(dim_x, dim_y, dim_z);
                            vs[v_idx + 6 * 3 + 2] = base + Vec3f(0, dim_y, dim_z);

                            ns[v_idx + 6 * 3 + 0] = south_n;
                            ns[v_idx + 6 * 3 + 1] = south_n;
                            ns[v_idx + 6 * 3 + 2] = south_n;

                            // south tri 1
                            vs[v_idx + 7 * 3 + 0] = base + Vec3f(0, 0, dim_z);
                            vs[v_idx + 7 * 3 + 1] = base + Vec3f(dim_x, 0, dim_z);
                            vs[v_idx + 7 * 3 + 2] = base + Vec3f(dim_x, dim_y, dim_z);

                            ns[v_idx + 7 * 3 + 0] = south_n;
                            ns[v_idx + 7 * 3 + 1] = south_n;
                            ns[v_idx + 7 * 3 + 2] = south_n;

                            // west tri 0
                            vs[v_idx + 8 * 3 + 0] = base;
                            vs[v_idx + 8 * 3 + 1] = base + Vec3f(0, dim_y, dim_z);
                            vs[v_idx + 8 * 3 + 2] = base + Vec3f(0, dim_y, 0);

                            ns[v_idx + 8 * 3 + 0] = west_n;
                            ns[v_idx + 8 * 3 + 1] = west_n;
                            ns[v_idx + 8 * 3 + 2] = west_n;

                            // west tri 1
                            vs[v_idx + 9 * 3 + 0] = base;
                            vs[v_idx + 9 * 3 + 1] = base + Vec3f(0, 0, dim_z);
                            vs[v_idx + 9 * 3 + 2] = base + Vec3f(0, dim_y, dim_z);

                            ns[v_idx + 9 * 3 + 0] = west_n;
                            ns[v_idx + 9 * 3 + 1] = west_n;
                            ns[v_idx + 9 * 3 + 2] = west_n;

                            // east tri 0
                            vs[v_idx + 10 * 3 + 0] = base + Vec3f(dim_x, 0, 0);
                            vs[v_idx + 10 * 3 + 1] = base + Vec3f(dim_x, dim_y, 0);
                            vs[v_idx + 10 * 3 + 2] = base + Vec3f(dim_x, dim_y, dim_z);

                            ns[v_idx + 10 * 3 + 0] = east_n;
                            ns[v_idx + 10 * 3 + 1] = east_n;
                            ns[v_idx + 10 * 3 + 2] = east_n;

                            // east tri 1
                            vs[v_idx + 11 * 3 + 0] = base + Vec3f(dim_x, 0, 0);
                            vs[v_idx + 11 * 3 + 1] = base + Vec3f(dim_x, dim_y, dim_z);
                            vs[v_idx + 11 * 3 + 2] = base + Vec3f(dim_x, 0, dim_z);

                            ns[v_idx + 11 * 3 + 0] = east_n;
                            ns[v_idx + 11 * 3 + 1] = east_n;
                            ns[v_idx + 11 * 3 + 2] = east_n;

                            v_idx += (3 * 12);
                        }

                        assert(v_idx == mesh_to_rebuild->num_of_vs);

                        if (mesh_to_rebuild->vao == 0)
                        {
                            assert((mesh_to_rebuild->vao == 0) && (mesh_to_rebuild->vbo == 0));
                            glGenVertexArrays(1, &mesh_to_rebuild->vao);
                            glGenBuffers(1, &mesh_to_rebuild->vbo);
                        }

                        glBindVertexArray(mesh_to_rebuild->vao);
                        glBindBuffer(GL_ARRAY_BUFFER, mesh_to_rebuild->vbo);

                        glBufferData(GL_ARRAY_BUFFER, vs_arr_size + ns_arr_size, vs, GL_STREAM_DRAW);
                        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)0);
                        glEnableVertexAttribArray(0);
                        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (char *)(0) + vs_arr_size);
                        glEnableVertexAttribArray(1);

                        glBindVertexArray(0);
                        glBindBuffer(GL_ARRAY_BUFFER, 0);
                    }

                    ranges_idx_start = ranges_idx_end;
                }

                for (int i = 0; i < BLOCK_TYPE_COUNT; i++)
                {
                    if (rebuilded_mesh_types[i] == 0)
                    {
                        Mesh *m = &chunk_to_rebuild->meshes[i];
                        if (m->vao != 0)
                        {
                            assert(m->vao && m->vbo);

                            glDeleteVertexArrays(1, &m->vao);
                            glDeleteBuffers(1, &m->vbo);

                            m->num_of_vs = 0;
                            m->vao = 0;
                            m->vbo = 0;
                        }
                    }
                }
            }
        }
        else if (chunk_to_rebuild && !chunk_to_rebuild->nblocks)
        {
            for (int i = 0; i < BLOCK_TYPE_COUNT; i++)
            {
                Mesh *m = &chunk_to_rebuild->meshes[i];
                if (m->vao != 0)
                {
                    assert(m->vao && m->vbo);

                    glDeleteVertexArrays(1, &m->vao);
                    glDeleteBuffers(1, &m->vbo);

                    m->num_of_vs = 0;
                    m->vao = 0;
                    m->vbo = 0;
                }
            }
        }
    }
    
    /* rendering */
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glEnable(GL_DEPTH_TEST);
        glClearColor(0.75f, 0.96f, 0.9f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        state->mesh_sp.use();

		Mat4x4f projection = mat4x4f_perspective(90.0f, input->aspect_ratio, 0.1f, 100.0f);
        state->mesh_sp.setMatrix4fv("u_projection", &projection.m[0][0]);

        Mat4x4f view = mat4x4f_lookat(state->cam_pos, state->cam_pos + state->cam_view_dir, state->cam_up);
        state->mesh_sp.setMatrix4fv("u_view", &view.m[0][0]);

        Chunk *c = state->world.next;
        while (c != 0)
        {
            if (c->nblocks)
            {
                Vec3f chunk_offset(
                    (float)(c->x * CHUNK_DIM),
                    (float)(c->y * CHUNK_DIM),
                    (float)(c->z * CHUNK_DIM));

                Mat4x4f model = mat4x4f_identity();
                model = mat4x4f_translate(model, chunk_offset);
                state->mesh_sp.setMatrix4fv("u_model", &model.m[0][0]);

                for (int m_idx = 0; m_idx < BLOCK_TYPE_COUNT; m_idx++)
                {
                    Mesh *mesh = &c->meshes[m_idx];
                    if (mesh->num_of_vs)
                    {
                        Vec3f colors[BLOCK_TYPE_COUNT] = {
                            Vec3f(0, 1, 0),
                            Vec3f(130 / 255.0f, 108 / 255.0f, 47 / 255.0f),
                            Vec3f(0.4f, 0.4f, 0.4f),
                            Vec3f(1, 1, 1),
                        };

                        Vec3f mesh_color = colors[m_idx];
                        glUniform3f(glGetUniformLocation(state->mesh_sp.get(), "u_color"), mesh_color.r, mesh_color.g, mesh_color.b);
                        glBindVertexArray(mesh->vao);
                        glDrawArrays(GL_TRIANGLES, 0, mesh->num_of_vs);
                        glBindVertexArray(0);
                    }
                }
            }
            c = c->next;
        }

		float ambient = 0.5f; //TODO

		//Skybox
		glm::mat4 viewMatrix;

		for (int x = 0; x < 4; ++x)
			for (int y = 0; y < 4; ++y)
				viewMatrix[x][y] = view.m[x][y];

		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_CLAMP);
	
		state->skyboxSP.use();
		glUniform1f(glGetUniformLocation(state->skyboxSP.get(), "ambientStrength"), ambient * 2);
		state->skyboxSP.setMatrix4fv("view", glm::mat4(glm::mat3(viewMatrix)));
		state->skyboxSP.setMatrix4fv("projection", &projection.m[0][0]);
		glBindVertexArray(state->skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, state->skybox.texture());
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);
		glDisable(GL_DEPTH_CLAMP);
    }
}

int main(void)
{

    if (glfwInit() == GLFW_FALSE)
    {
        return (-1);
    }

    int window_width  = 1280;
    int window_height = 720;
 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "This is awesome!", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return (-1);
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        return (-1);
    }
    glfwSwapInterval(1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

#define PERMANENT_MEM_SIZE MEMORY_MB(1)
#define TRANSIENT_MEM_SIZE MEMORY_GB(1)

    static uint8_t permanent_mem_blob[PERMANENT_MEM_SIZE];
    static uint8_t transient_mem_blob[TRANSIENT_MEM_SIZE];

    Game_memory game_memory = {};
    game_memory.permanent_mem_size = PERMANENT_MEM_SIZE;
    game_memory.permanent_mem = permanent_mem_blob;
    game_memory.transient_mem_size = TRANSIENT_MEM_SIZE;
    game_memory.transient_mem = transient_mem_blob;

    game_state_and_memory_init(&game_memory);

    Game_input inputs[2] = {};
    Game_input *game_input = &inputs[0];
    Game_input *prev_game_input = &inputs[1];

    double curr_time = glfwGetTime();
    double prev_time = curr_time;

    double prev_mx = (float)window_width / 2.0f;
    double prev_my = (float)window_height / 2.0f;

    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &window_width, &window_height);
        glViewport(0, 0, window_width, window_height);

        game_input->aspect_ratio = float(window_width) / float(window_height);
        game_input->dt = float(curr_time - prev_time);

        double curr_mx;
        double curr_my;
        glfwGetCursorPos(window, &curr_mx, &curr_my);
        double sensitivity = 0.1;
        game_input->mouse_dx = (float)((curr_mx - prev_mx) * sensitivity);
        game_input->mouse_dy = (float)((prev_my - curr_my) * sensitivity);
        prev_mx = curr_mx;
        prev_my = curr_my;

        int num_of_buttons = sizeof(inputs[0].buttons) / sizeof(inputs[0].buttons[0]);
        for (int i = 0; i < num_of_buttons; i++)
        {
            game_input->buttons[i].is_pressed  = 0;
            game_input->buttons[i].was_pressed = 0;
            game_input->buttons[i].was_pressed = prev_game_input->buttons[i].is_pressed;
        }

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            game_input->mleft.is_pressed = 1;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        {
            game_input->mright.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            game_input->w.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            game_input->a.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            game_input->s.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            game_input->d.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        {
            game_input->enter.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            game_input->space.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            game_input->lshift.is_pressed = 1;
        }

        game_update_and_render(game_input, &game_memory);

        prev_time = curr_time;
        curr_time = glfwGetTime();

        glfwSwapBuffers(window);
        glfwPollEvents();

        Game_input *temp_input = game_input;
        game_input = prev_game_input;
        prev_game_input = temp_input;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return (0);
}