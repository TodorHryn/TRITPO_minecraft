#include "Worldgen.h"

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
