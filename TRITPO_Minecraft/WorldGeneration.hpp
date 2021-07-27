#pragma once
#include "3DMath.h"
#include "main.h"
#include <algorithm>

unsigned int hash(unsigned int x) { //https://stackoverflow.com/a/12996028
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

Vec3f randomGradient(int x, int z) {
	int h = hash(hash(hash(x) + z) + WORLD_SEED);
	float gx = h;
	float gz = hash(h);

	return normalize(Vec3f(gx, 0, gz));
}

float interpolate(float begin, float end, float pos) {
	return (end - begin) * pos + begin;
}

float perlin_noise(float x, float z) {
	int x0 = floor(x);
	int z0 = floor(z);

	Vec3f g00 = randomGradient(x0, z0);
	Vec3f g01 = randomGradient(x0, z0 + 1);
	Vec3f g10 = randomGradient(x0 + 1, z0);
	Vec3f g11 = randomGradient(x0 + 1, z0 + 1);

	Vec3f d00(x - x0, 0, z - z0);
	Vec3f d01(x - x0, 0, z - z0 - 1);
	Vec3f d10(x - x0 - 1, 0, z - z0);
	Vec3f d11(x - x0 - 1, 0, z - z0 - 1);

	float dot00 = dot(g00, d00);
	float dot01 = dot(g01, d01);
	float dot10 = dot(g10, d10);
	float dot11 = dot(g11, d11);

	float dx = x - x0;
	float dz = z - z0;

	float int0 = interpolate(dot00, dot01, dz);
	float int1 = interpolate(dot10, dot11, dz);

	return interpolate(int0, int1, dx);
}

int get_height(int x, int z) {
	float noise0 = (perlin_noise(x / 128.0f, z / 128.0f) + 1) / 2;
	float noise1 = (perlin_noise(x / 64.0f, z / 64.0f) + 1) / 2;
	float noise2 = (perlin_noise(x / 32.0f, z / 32.0f) + 1) / 2;
	float noise3 = (perlin_noise(x / 16.0f, z / 16.0f) + 1) / 2;

	return CHUNK_DIM * (6 * noise0 + 3 * noise1 + 1.5 * noise2 + 0.75 * noise3);
}

void generate_chunk(World &world, int chunk_x, int chunk_y, int chunk_z) {
	Chunk *c = world.add_chunk(chunk_x, chunk_y, chunk_z);
    assert(c);

    int i = 0;
    for (int z = 0; z < CHUNK_DIM; z++)
    {
        for (int x = 0; x < CHUNK_DIM; x++)
        {
			int noise_x = (chunk_x * CHUNK_DIM + x);
			int noise_z = (chunk_z * CHUNK_DIM + z);					
			int h = get_height(noise_x, noise_z) - CHUNK_DIM * chunk_y;

			for (int y = 0; y < std::min(h, CHUNK_DIM); y++)
			{
				uint8_t block_type;

				block_type = BLOCK_STONE;
				c->blocks[CHUNK_DIM * CHUNK_DIM * y + CHUNK_DIM * z + x] = block_type;
				c->nblocks++;
				i++;
			}
        }
    }
    
	world.push_chunk_for_rebuild(c);
}