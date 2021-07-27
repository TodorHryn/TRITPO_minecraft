#pragma once

#include "3DMath.h"

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

extern Vec3f Block_colors[BLOCK_TYPE_COUNT];