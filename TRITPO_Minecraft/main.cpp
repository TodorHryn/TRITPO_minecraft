#include <stdio.h> // sprintf
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <stack>

#include "glad\glad.h"
#include "GLFW\glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glad.c"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include "main.h"

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

    float max_ray_len = 10.0f;
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
        
        for (Chunk *c : world->visible_chunks)
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

    Game_state *state =	memory->game_state;

    memory->is_initialized = 1;

    float cubeVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,	 0.0f,  0.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,	 0.0f,  0.0f, -1.0f,
         1.0f, -1.0f, -1.0f,	 0.0f,  0.0f, -1.0f,
         1.0f, -1.0f, -1.0f,	 0.0f,  0.0f, -1.0f,
         1.0f,  1.0f, -1.0f,	 0.0f,  0.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,	 0.0f,  0.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,	-1.0f,  0.0f,  0.0f,
        -1.0f, -1.0f, -1.0f,	-1.0f,  0.0f,  0.0f,
        -1.0f,  1.0f, -1.0f,	-1.0f,  0.0f,  0.0f,
        -1.0f,  1.0f, -1.0f,	-1.0f,  0.0f,  0.0f,
        -1.0f,  1.0f,  1.0f,	-1.0f,  0.0f,  0.0f,
        -1.0f, -1.0f,  1.0f,	-1.0f,  0.0f,  0.0f,

         1.0f, -1.0f, -1.0f,	 1.0f,  0.0f,  0.0f,
         1.0f, -1.0f,  1.0f,	 1.0f,  0.0f,  0.0f,
         1.0f,  1.0f,  1.0f,	 1.0f,  0.0f,  0.0f,
         1.0f,  1.0f,  1.0f,	 1.0f,  0.0f,  0.0f,
         1.0f,  1.0f, -1.0f,	 1.0f,  0.0f,  0.0f,
         1.0f, -1.0f, -1.0f,	 1.0f,  0.0f,  0.0f,

        -1.0f, -1.0f,  1.0f,	 0.0f,  0.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,	 0.0f,  0.0f,  1.0f,
         1.0f,  1.0f,  1.0f,	 0.0f,  0.0f,  1.0f,
         1.0f,  1.0f,  1.0f,	 0.0f,  0.0f,  1.0f,
         1.0f, -1.0f,  1.0f,	 0.0f,  0.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,	 0.0f,  0.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,	 0.0f,  1.0f,  0.0f,
         1.0f,  1.0f, -1.0f,	 0.0f,  1.0f,  0.0f,
         1.0f,  1.0f,  1.0f,	 0.0f,  1.0f,  0.0f,
         1.0f,  1.0f,  1.0f,	 0.0f,  1.0f,  0.0f,
        -1.0f,  1.0f,  1.0f,	 0.0f,  1.0f,  0.0f,
        -1.0f,  1.0f, -1.0f,	 0.0f,  1.0f,  0.0f,

        -1.0f, -1.0f, -1.0f,	 0.0f, -1.0f,  0.0f,
        -1.0f, -1.0f,  1.0f,	 0.0f, -1.0f,  0.0f,
         1.0f, -1.0f, -1.0f,	 0.0f, -1.0f,  0.0f,
         1.0f, -1.0f, -1.0f,	 0.0f, -1.0f,  0.0f,
        -1.0f, -1.0f,  1.0f,	 0.0f, -1.0f,  0.0f,
         1.0f, -1.0f,  1.0f, 	 0.0f, -1.0f,  0.0f
    };

	float squareVertices[] = {
		-1.0f,  1.0f,  0.0f,
        -1.0f, -1.0f,  0.0f,
         1.0f, -1.0f,  0.0f,
         1.0f, -1.0f,  0.0f,
         1.0f,  1.0f,  0.0f,
        -1.0f,  1.0f,  0.0f
	};

	state->chunkAllocator = memory->chunkAllocator;
	state->world.allocator = memory->chunkAllocator;

	new (&state->world.rebuild_stack) std::stack<Chunk*>();
	new (&state->world.visible_chunks) std::vector<Chunk*>();
	new (&state->world.unloaded_chunks) std::vector<Chunk*>();

    state->cam_pos = Vec3f(0, 120, 0);
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

    state->block_to_place = BLOCK_GRASS;

	state->frameCount = 0;
	state->fpsCounterPrevTime = glfwGetTime();
	state->fps = 0;

    // NOTE(max): call constructors on existing memory
    new (&state->mesh_sp) ShaderProgram("mesh");
    new (&state->skyboxSP) ShaderProgram("skybox");
	new (&state->sunSP) ShaderProgram("sun");
	new (&state->imageSP) ShaderProgram("image");
	new (&state->inventoryBlockSP) ShaderProgram("inventoryBlock");
	new (&state->meshShadowMapSP) ShaderProgram("meshShadowMap");
	new (&state->fontCharacterSP) ShaderProgram("fontchar");
	new (&state->shadowMap1) ShadowMap(2048, 2048);
	new (&state->shadowMap2) ShadowMap(2048, 2048);
	new (&state->shadowMap3) ShadowMap(2048, 2048);
	new (&state->shadowMap4) ShadowMap(2048, 2048);
	new (&state->sunTexture) Texture("Images/sun.png", GL_RGBA);
	new (&state->inventoryBarTexture) Texture("Images/inventoryBar.png");
	new (&state->crossTexture) Texture("Images/cross.png");
	new (&state->fontTexture) Texture("Images/OpenSans.bmp");
    new (&state->skybox) Skybox("Images/cubemap");

    // Cube VAO (Skybox, inventory blocks)
    glGenVertexArrays(1, &state->cubeVAO);
    glGenBuffers(1, &state->cubeVBO);
    glBindVertexArray(state->cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state->cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
    glBindVertexArray(0);

	// Square VAO (Sun, inventory bar)
    glGenVertexArrays(1, &state->squareVAO);
    glGenBuffers(1, &state->squareVBO);
    glBindVertexArray(state->squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state->squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), &squareVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void renderWorld(Game_state *state, ShaderProgram &sp) {
	for (Chunk *c : state->world.visible_chunks)
	{
		if (c->nblocks && c->render)
		{
			Vec3f chunk_offset(
				(float)(c->x * CHUNK_DIM),
				(float)(c->y * CHUNK_DIM),
				(float)(c->z * CHUNK_DIM));

			Mat4x4f model = mat4x4f_identity();
			model = mat4x4f_translate(model, chunk_offset);
			sp.setMatrix4fv("u_model", &model.m[0][0]);

			for (int m_idx = 0; m_idx < BLOCK_TYPE_COUNT; m_idx++)
			{
				Mesh *mesh = &c->meshes[m_idx];
				if (mesh->num_of_vs)
				{
					Vec3f mesh_color = Block_colors[m_idx];
					glUniform3f(glGetUniformLocation(sp.get(), "u_color"), mesh_color.r, mesh_color.g, mesh_color.b);
					glBindVertexArray(mesh->vao);
					glDrawArrays(GL_TRIANGLES, 0, mesh->num_of_vs);
					glBindVertexArray(0);
				}
			}
		}
	}
}

void drawText(Game_state *state, Game_input *input, std::string text, float x, float y, float scale) {
	const float spacing = 0.6;

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_OR);

	glDisable(GL_CULL_FACE);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(state->squareVAO);
	state->fontTexture.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	state->fontCharacterSP.use();

	for (int i = 0; i < text.size(); ++i) {
		int pos = toupper(text[i]) - 32;

		glm::mat4 model(1);
		model = glm::translate(model, glm::vec3(x + spacing * i * scale * 2 / input->aspect_ratio, y, 0.0f));
		model = glm::scale(model, glm::vec3(scale / input->aspect_ratio, scale, 1.0f));
		state->fontCharacterSP.setMatrix4fv("model", model);
		state->fontCharacterSP.set3fv("texturePos", glm::vec3(pos % 8, 7 - pos / 8, 0));
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
}

void rebuild_chunk(Game_memory *memory, Chunk *chunk) {
	if (chunk->nblocks) {
		Range3d *ranges = memory->ranges;
		uint8_t *visited = memory->visited;

		if (ranges && visited)
		{
			for (int i = 0; i < (BLOCKS_IN_CHUNK); i++) visited[i] = 0;
             
			int nranges = 0;
			gen_ranges_3d(chunk->blocks, ranges, visited, CHUNK_DIM, chunk->nblocks, &nranges);

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
			while (ranges_left > 0)
			{
				int ranges_count = 0;
				uint8_t range_type = ranges[ranges_idx_start].type;
				while ((ranges_idx_end < nranges) && ranges[ranges_idx_end].type == range_type)
				{
					ranges_count++;
					ranges_idx_end++;
				}
				ranges_left -= ranges_count;

				assert(range_type < BLOCK_TYPE_COUNT);
				Mesh *mesh_to_rebuild = &chunk->meshes[range_type];

				int vs_arr_size = 3 * 12 * ranges_count * sizeof(Vec3f);
				int ns_arr_size = 3 * 12 * ranges_count * sizeof(Vec3f);
				Vec3f *vs = (Vec3f*) memory->transient_mem;
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
					Mesh *m = &chunk->meshes[i];
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
	else
	{
		for (int i = 0; i < BLOCK_TYPE_COUNT; i++)
		{
			Mesh *m = &chunk->meshes[i];
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

bool chunk_exists(Game_state *state, int x, int y, int z) {
	for (Chunk *c : state->world.visible_chunks) {
		if (c->x == x && c->y == y && c->z == z) 
			return true;
	}

	return false;
}

void frustum_culling_perspective(Game_state *state, glm::vec3 cameraPos, glm::vec3 cameraViewDir, float nearZ, float farZ) {
	for (Chunk *c : state->world.visible_chunks) {
		glm::vec3 chunkCenter(c->x * CHUNK_DIM + CHUNK_DIM / 2, c->y * CHUNK_DIM + CHUNK_DIM / 2, c->z * CHUNK_DIM + CHUNK_DIM / 2);
		float radius = sqrt(3) * CHUNK_DIM / 2;

		glm::vec3 chunkDir = chunkCenter - cameraPos;
		float chunkDirProj = glm::dot(cameraViewDir, chunkDir);

		if (chunkDirProj + radius < nearZ || chunkDirProj - radius > farZ) {
			c->render = false;
			continue;
		}
		
		c->render = true;
	}
}

void game_update_and_render(Game_input *input, Game_memory *memory)
{
    assert(memory->is_initialized);
    Game_state *state = memory->game_state;

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

        if (input->n1.is_pressed)
        {
            state->block_to_place = BLOCK_GRASS;
        }
        if (input->n2.is_pressed)
        {
            state->block_to_place = BLOCK_DIRT;
        }
        if (input->n3.is_pressed)
        {
            state->block_to_place = BLOCK_STONE;
        }
        if (input->n4.is_pressed)
        {
            state->block_to_place = BLOCK_SNOW;
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
					rc.chunk->changed = true;
                    rc.chunk->blocks[block_idx] = BLOCK_AIR;
                    rc.chunk->nblocks--;
                    state->world.push_chunk_for_rebuild(rc.chunk);
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

                for (Chunk *c : state->world.visible_chunks)
                {
                    if ((c->x == last_chunk_x) &&
                        (c->y == last_chunk_y) &&
                        (c->z == last_chunk_z))
                    {
                        prev_chunk = c;
                        break;
                    }
                }

                if (!prev_chunk)
                {
                    prev_chunk = state->world.add_chunk(last_chunk_x, last_chunk_y, last_chunk_z);
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
						prev_chunk->changed = true;
                        prev_chunk->blocks[block_idx] = state->block_to_place;
                        prev_chunk->nblocks++;
                        state->world.push_chunk_for_rebuild(prev_chunk);
                    }
                }
            }
        }

		//Generate new chunks
		int cam_chunk_x = (int) state->cam_pos.x >> CHUNK_DIM_LOG2;
		int cam_chunk_y = (int) state->cam_pos.y >> CHUNK_DIM_LOG2;
		int cam_chunk_z = (int) state->cam_pos.z >> CHUNK_DIM_LOG2;

		for (int x = cam_chunk_x - WORLD_RADIUS; x <= cam_chunk_x + WORLD_RADIUS; ++x) {
			for (int z = cam_chunk_z - WORLD_RADIUS; z <= cam_chunk_z + WORLD_RADIUS; ++z) {
				for (int i = cam_chunk_y - GENERATION_Y_RADIUS; i <= cam_chunk_y + GENERATION_Y_RADIUS; ++i) {
					if (!chunk_exists(state, x, i, z)) {
						state->world.load_chunk(x, i, z);
					}
				}
			}
		}

		//Remove far chunks
		auto &chunks = state->world.visible_chunks;
		for (int i = chunks.size() - 1; i >= 0; --i) {
			Chunk *c = chunks[i];

			int dx = abs(cam_chunk_x - c->x);
			int dy = abs(cam_chunk_y - c->y);
			int dz = abs(cam_chunk_z - c->z);

			if (dx > WORLD_RADIUS || dz > WORLD_RADIUS || dy > GENERATION_Y_RADIUS) {
				state->world.unload_chunk(i);
			}
		}

		//Rebuild chunks
        while (!state->world.rebuild_stack.empty())
        {
            rebuild_chunk(memory, state->world.pop_chunk_for_rebuild());
        }
    }
    
    /* rendering */
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);

		glEnable(GL_DEPTH_TEST);
        glClearColor(0.75f, 0.96f, 0.9f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		glm::vec3 sunPosition(20 * sin(-20 + glfwGetTime() * TIME_SPEED), 20 * cos(0.002 - 20 + glfwGetTime() * TIME_SPEED), 20 * sin(glfwGetTime() * TIME_SPEED));
		float sunHeight = glm::dot(glm::normalize(sunPosition), glm::vec3(0.0f, 1.0f, 0.0f));
		float ambient = std::max(sunHeight / 2, 0.3f);
		
		//Shadow maps
		glm::vec3 cameraPos(state->cam_pos.x, state->cam_pos.y, state->cam_pos.z);
		glm::mat4 lightView = glm::lookAt(sunPosition + cameraPos, cameraPos, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightProjection1 = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 200.0f);
		glm::mat4 lightProjection2 = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 200.0f);
		glm::mat4 lightProjection3 = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 1.0f, 200.0f);
		glm::mat4 lightProjection4 = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, 1.0f, 200.0f);
		glm::mat4 lightProjectionViewMatrix1 = lightProjection1 * lightView;
		glm::mat4 lightProjectionViewMatrix2 = lightProjection2 * lightView;
		glm::mat4 lightProjectionViewMatrix3 = lightProjection3 * lightView;
		glm::mat4 lightProjectionViewMatrix4 = lightProjection4 * lightView;
		state->meshShadowMapSP.use();

		state->shadowMap1.bind();
		state->meshShadowMapSP.setMatrix4fv("u_projection_view", lightProjectionViewMatrix1);
		renderWorld(state, state->meshShadowMapSP);
		state->shadowMap1.unbind();

		state->shadowMap2.bind();
		state->meshShadowMapSP.setMatrix4fv("u_projection_view", lightProjectionViewMatrix2);
		renderWorld(state, state->meshShadowMapSP);
		state->shadowMap2.unbind();

		state->shadowMap3.bind();
		state->meshShadowMapSP.setMatrix4fv("u_projection_view", lightProjectionViewMatrix3);
		renderWorld(state, state->meshShadowMapSP);
		state->shadowMap3.unbind();

		state->shadowMap4.bind();
		state->meshShadowMapSP.setMatrix4fv("u_projection_view", lightProjectionViewMatrix4);
		renderWorld(state, state->meshShadowMapSP);
		state->shadowMap4.unbind();

		//World
        state->mesh_sp.use();

		Mat4x4f projection = mat4x4f_perspective(90.0f, input->aspect_ratio, 0.1f, 200.0f);
        state->mesh_sp.setMatrix4fv("u_projection", &projection.m[0][0]);

		glm::vec3 camViewDir(state->cam_view_dir.x, state->cam_view_dir.y, state->cam_view_dir.z);
		frustum_culling_perspective(state, cameraPos, camViewDir, 0.1f, 200.0f);
		
		Mat4x4f view = mat4x4f_lookat(state->cam_pos, state->cam_pos + state->cam_view_dir, state->cam_up);
		state->mesh_sp.setMatrix4fv("u_view", &view.m[0][0]);
		
		state->mesh_sp.set3fv("light_pos", sunPosition);
		state->mesh_sp.set1f("ambient_factor", ambient);
		state->mesh_sp.set1f("diffuse_strength", (sunPosition.y > 0.0f) ? 1.0f : 0.2f);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, state->shadowMap1.get());
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, state->shadowMap2.get());
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, state->shadowMap3.get());
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, state->shadowMap4.get());
		glUniform1i(glGetUniformLocation(state->mesh_sp.get(), "depthMap1"), 2);
		glUniform1i(glGetUniformLocation(state->mesh_sp.get(), "depthMap2"), 3);
		glUniform1i(glGetUniformLocation(state->mesh_sp.get(), "depthMap3"), 4);
		glUniform1i(glGetUniformLocation(state->mesh_sp.get(), "depthMap4"), 5);
		state->mesh_sp.setMatrix4fv("lightSpaceMatrix1", lightProjectionViewMatrix1);
		state->mesh_sp.setMatrix4fv("lightSpaceMatrix2", lightProjectionViewMatrix2);
		state->mesh_sp.setMatrix4fv("lightSpaceMatrix3", lightProjectionViewMatrix3);
		state->mesh_sp.setMatrix4fv("lightSpaceMatrix4", lightProjectionViewMatrix4);
		glUniform1f(glGetUniformLocation(state->mesh_sp.get(), "shadowStrength"), (sunHeight > 0.5f ? 1.0f : std::max(sunHeight * 2.0f, 0.0f)));

		if (sunHeight > 0.2f)
			state->mesh_sp.set1f("diffuse_strength", 1.0f);
		else if (sunHeight > 0.0f)
			state->mesh_sp.set1f("diffuse_strength", sunHeight * 5);
		else if (sunHeight > -0.2f) {
			state->mesh_sp.set1f("diffuse_strength", abs(sunHeight / 2));
			state->mesh_sp.set3fv("light_pos", -sunPosition);
		}
		else {
			state->mesh_sp.set1f("diffuse_strength", 0.1f);
			state->mesh_sp.set3fv("light_pos", -sunPosition);
		}

		renderWorld(state, state->mesh_sp);

		Raycast_result rc = raycast(&state->world, state->cam_pos, state->cam_view_dir);
		if (rc.collision) {
			glm::mat4 model(1);
			model = glm::translate(model, glm::vec3(rc.i + 0.5f, rc.j + 0.5f, rc.k + 0.5f));
			model = glm::scale(model, glm::vec3(0.51f, 0.51f, 0.51f));

			glDisable(GL_CULL_FACE);
			glBindVertexArray(state->cubeVAO);

			state->mesh_sp.use();
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			state->mesh_sp.setMatrix4fv("u_model", model);
			glUniform3f(glGetUniformLocation(state->mesh_sp.get(), "u_color"), 0.0f, 0.0f, 0.0f);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glClear(GL_DEPTH_BUFFER_BIT);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_EQUAL, 0, 0xFF);

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
		state->skyboxSP.set1f("ambient_factor", ambient * 2);
		glBindVertexArray(state->cubeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, state->skybox.texture());
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);
		glDisable(GL_DEPTH_CLAMP);

		//Sun
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_CLAMP);
		glDisable(GL_CULL_FACE);
		glBindVertexArray(state->squareVAO);
		glActiveTexture(GL_TEXTURE0);
		state->sunTexture.bind();
		state->sunSP.use();

		glm::mat4 model(1);
		model = glm::translate(model, sunPosition + glm::vec3(state->cam_pos.x, state->cam_pos.y, state->cam_pos.z));
		glm::vec3 sunPositionProjection(sunPosition.x, sunPosition.y, 0);
		float angle = 3.14f / 2.0f - acos(glm::dot(glm::normalize(sunPosition), glm::normalize(sunPositionProjection)));
		glm::vec3 sunRotationAxis = glm::cross(sunPosition, sunPositionProjection);
		model = glm::rotate(model, angle, sunRotationAxis);
		model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));

		state->sunSP.setMatrix4fv("model", model);
		state->sunSP.setMatrix4fv("view", &view.m[0][0]);
		state->sunSP.setMatrix4fv("projection", &projection.m[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);
		glDisable(GL_DEPTH_CLAMP);

		//Inventory
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glDisable(GL_DEPTH_TEST);
		glCullFace(GL_FRONT);

		Mat4x4f invBlockProjection = mat4x4f_perspective(45.0f, input->aspect_ratio, 0.1f, 10.0f);
        Mat4x4f invBlockView = mat4x4f_lookat(Vec3f(5.0f, 5.0f, 5.0f), Vec3f(5.0f, 5.0f, 5.0f) + normalize(Vec3f(-1.0f, -1.0f, -1.0f)), Vec3f(0.0f, 1.0f, 0.0f));

		for (int i = 0; i < BLOCK_TYPE_COUNT; ++i) {
			float slotSize = (state->block_to_place == i) ? 0.07f : 0.05f;
			float xPosition = (-static_cast<float>(BLOCK_TYPE_COUNT) / 2 + ((BLOCK_TYPE_COUNT % 2) ? 0 : 0.5f) + i) * 0.08f;
			float yPosition = -1.0f + ((state->block_to_place == i) ? 0.01f + slotSize : 0.03f + slotSize);

			//Bar slot
			glDisable(GL_CULL_FACE);
			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(state->squareVAO);
			state->inventoryBarTexture.bind();
			state->imageSP.use();

			glm::mat4 model(1);
			model = glm::translate(model, glm::vec3(xPosition, yPosition, 0.0f));
			model = glm::scale(model, glm::vec3(slotSize / input->aspect_ratio, slotSize, 1.0f));
			state->imageSP.setMatrix4fv("model", model);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			//Block
			glEnable(GL_CULL_FACE);
			glBindVertexArray(state->cubeVAO);
			state->inventoryBlockSP.use();

			model = glm::translate(glm::mat4(1), glm::vec3(xPosition, yPosition, 0.0f));
			model = glm::scale(model, glm::vec3(slotSize * 1.2f, slotSize * 1.2f, 1.0f));

			Vec3f color = Block_colors[i];
			state->inventoryBlockSP.setMatrix4fv("u_model", model);
			state->inventoryBlockSP.setMatrix4fv("u_view", &invBlockView.m[0][0]);
			state->inventoryBlockSP.setMatrix4fv("u_projection", &invBlockProjection.m[0][0]);
			glUniform3f(glGetUniformLocation(state->inventoryBlockSP.get(), "u_color"), color.r, color.g, color.b);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
		}

		glBindVertexArray(0);

		//FPS
		drawText(state, input, "FPS: " + std::to_string((int) state->fps), -0.96, 0.96, 0.04);

		glEnable(GL_DEPTH_TEST);

		//Cross
		glDisable(GL_CULL_FACE);
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(state->squareVAO);
		state->crossTexture.bind();
		state->imageSP.use();

		model = glm::scale(glm::mat4(1), glm::vec3(0.01f, 0.01f * input->aspect_ratio, 1.0f));
		state->imageSP.setMatrix4fv("model", model);
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(GL_XOR);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisable(GL_COLOR_LOGIC_OP);

		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(0);
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

	static uint8_t game_state_mem_blob[sizeof(Game_state)];
    static uint8_t transient_mem_blob[TRANSIENT_MEM_SIZE];
	static Chunk chunk_mem_blob[MAX_CHUNKS];
	
    Game_memory game_memory = {};
    game_memory.transient_mem_size = TRANSIENT_MEM_SIZE;
    game_memory.transient_mem = transient_mem_blob;
	game_memory.game_state = (Game_state*) game_state_mem_blob;
	game_memory.chunkAllocator = new PoolAllocator<Chunk>(chunk_mem_blob, MAX_CHUNKS);

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

		game_input->window_width = window_width;
		game_input->window_height = window_height;
        game_input->aspect_ratio = float(window_width) / float(window_height);
        game_input->dt = float(curr_time - prev_time);

		Game_state *state = game_memory.game_state;
		if (state->frameCount % 10 == 0) {
			state->fps = 1 / ((glfwGetTime() - state->fpsCounterPrevTime) / 10);
			state->fpsCounterPrevTime = glfwGetTime();
		}
		state->frameCount++;

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
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            game_input->n1.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            game_input->n2.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            game_input->n3.is_pressed = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            game_input->n4.is_pressed = 1;
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
