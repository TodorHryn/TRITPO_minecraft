#pragma once

#include "3DMath.h"
#include "main.h"

unsigned int hash(unsigned int x);
Vec3f randomGradient(int x, int z);
float interpolate(float begin, float end, float pos);
float perlin_noise(float x, float z);
int get_height(int x, int z);
