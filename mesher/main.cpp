
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

struct Range1d
{
    int type;
    int start;
    int end;
};

struct Range2d
{
    int start_x;
    int start_y;
    int end_x;
    int end_y;
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

void mesh_1d(int *blocks, int width, int count, Range1d *ranges, int *num_of_ranges)
{

    int *visited = (int *)calloc(width, sizeof(int));
    int ranges_count = 0;
    while (count > 0)
    {
        int start = 0;
        int end = 0;
        for (start = 0; start < width; start++)
        {
            if (visited[start] == 0)
            {
                break;
            }
        }

        while (start < width && blocks[start] == 0)
        {
            visited[start] = 1;
            start++;
        }

        int block_type = 0;
        if (start < width)
        {
            visited[start] = 1;
            count--;

            block_type = blocks[start];
        }

        end = start;
        while ((end + 1 < width) && (blocks[end + 1] == block_type))
        {
            visited[end + 1] = 1;
            end++;
            count--;
        }

        ranges[ranges_count++] = { block_type, start, end };
    }

    *num_of_ranges = ranges_count;
    free(visited);
}

void mesh_2d(int *blocks, int width, int height, int count, Range2d *ranges, int *num_of_ranges)
{
    int *visited = (int *)calloc(width * height, sizeof(int));
    int ranges_count = 0;

    while (count > 0)
    {
        int start_y = 0;
        int end_y   = 0;

        int start_x = 0;
        int end_x   = 0;

        // skip all visited and empty blocks
        for (start_y = 0; start_y < height; start_y++)
        {
            for (start_x = 0; start_x < width; start_x++)
            {
                if (visited[start_y * width + start_x] == 0 &&
                    blocks[start_y * width + start_x] != 0)
                {
                    goto break1;
                }
            }
        }
break1:

        // if a block at start_x, start_y is in the grid (the grid is not empty),
        // mark it as visited
        if (start_x < width && start_y < height)
        {
            visited[start_y * width + start_x] = 1;
            count--;
        }

        // try expand in x direction
        end_x = start_x;
        while ( (end_x + 1 < width) &&
                (blocks[start_y * width + (end_x + 1)] != 0) &&
                (visited[start_y * width + (end_x + 1)] == 0))
        {
            visited[start_y * width + (end_x + 1)] = 1;
            end_x++;
            count--;
        }

        // try expand in y direction
        end_y = start_y;

        while (end_y + 1 < height)
        {
            bool can_expand = true;
            for (int x = start_x; x <= end_x; x++)
            {
                if (blocks[(end_y + 1) * width + x] == 0 ||
                    visited[(end_y + 1) * width + x] == 1)
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
                    visited[(end_y + 1) * width + x] = 1;
                }
                end_y++;
                count -= end_x - start_x + 1;
            }
            else
            {
                break;
            }
        }

        ranges[ranges_count++] = {start_x, start_y, end_x, end_y};
    }

    *num_of_ranges = ranges_count;
    free(visited);
}

void mesh_3d(uint8_t *blocks, int dim, int count, Range3d *ranges, int *num_of_ranges)
{
    int *visited = (int *)calloc(dim * dim * dim, sizeof(int));
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
                    // TODO(max): think about it
                    if (visited[start_y * dim * dim + start_z * dim + start_x] == 0 &&
                        blocks[start_y * dim * dim + start_z * dim + start_x] != 0)
                    {
                        goto break1;
                    }
                }
            }
        }
    break1:

        // If a block at (start_x, start_y, start_z) is in the grid (the grid is not empty), mark it as visited.
        // Also record block type.
        uint8_t block_type = 0;
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

        ranges[ranges_count++] = { block_type, start_x, start_y, start_z, end_x, end_y, end_z};
    }

    assert(count == 0);
    free(visited);
    *num_of_ranges = ranges_count;
}

void test_1d_meshing(void)
{
    const int width1 = 16;
    int blocks1[width1] = {
        1, 1, 1, 1,  0, 0,  2, 2, 2,  0,  1,  2,  1, 0, 3, 3
    };

    const int width2 = 16;
    int blocks2[width2] = {
        1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
    };

    const int width3 = 16;
    int blocks3[width3] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
    };

    const int width4 = 4;
    int blocks4[width4] = {
        0, 1, 1, 0,
    };

    int width   = width1;
    int *blocks = blocks1;

    int count = 0;
    for (int i = 0; i < width; i++)
    {
        if (blocks[i]) count++;
    }

    int ranges_count = 0;
    Range1d *ranges = (Range1d *)calloc(width / 2 + 1, sizeof(Range1d));

    mesh_1d(blocks, width, count, ranges, &ranges_count);

    if (ranges_count > 0)
    {
        for (int i = 0; i < ranges_count; i++)
        {
            printf("type %d: (%d, %d)\n", ranges[i].type, ranges[i].start, ranges[i].end);
        }
    }
    else
    {
        printf("empty chunk!\n");
    }

    free(ranges);
}

void test_2d_meshing(void)
{
    const int width1  = 4;
    const int height1 = 4;
    int blocks1[width1 * height1] = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 1, 1, 0
    };

    int width   = width1;
    int height  = height1;
    int *blocks = blocks1;

    int count = 0;
    for (int i = 0; i < width * height; i++)
    {
        if (blocks[i]) count++;
    }

    int ranges_count = 0;
    Range2d *ranges = (Range2d *)calloc((width * height) / 2 + 1, sizeof(Range2d));

    mesh_2d(blocks, width, height, count, ranges, &ranges_count);

    if (ranges_count > 0)
    {
        for (int i = 0; i < ranges_count; i++)
        {
            printf("(%d, %d) (%d, %d)\n", ranges[i].start_x, ranges[i].start_y, ranges[i].end_x, ranges[i].end_y);
        }
    }
    else
    {
        printf("empty chunk!\n");
    }

    free(ranges);
}

#if 1
void test_3d_meshing(void)
{
#define dim1 2
    uint8_t blocks1[dim1 * dim1 * dim1] = {
        3, 3,
        7, 6,
        
        3, 3,
        7, 6
    };

    int dim = dim1;
    uint8_t *blocks = blocks1;

    int count = 0;
    for (int i = 0; i < (dim * dim * dim); i++)
    {
        if (blocks[i]) count++;
    }

    int ranges_count = 0;
    Range3d *ranges = (Range3d *)calloc(dim * dim * dim, sizeof(Range3d));

    mesh_3d(blocks, dim, count, ranges, &ranges_count);

    if (ranges_count > 0)
    {
        for (int i = 0; i < ranges_count; i++)
        {
            printf("type %d:\n\t(%d, %d, %d)\n\t(%d, %d, %d)\n",
                ranges[i].type,
                ranges[i].start_x, ranges[i].start_y, ranges[i].start_z,
                ranges[i].end_x, ranges[i].end_y, ranges[i].end_z);
        }
    }
    else
    {
        printf("empty chunk!\n");
    }

    free(ranges);
}
#endif

int main(void)
{
    //test_1d_meshing();
    //test_2d_meshing();
    test_3d_meshing();
    
    /*uint8_t blocks[] = {
        1, 2, 3, 4, 5, 6, 7, 8
    };

    int n = 0;
    Range3d ranges[8 / 2 + 1];
    mesh_3d(blocks, 2, 8, ranges, &n);

    for (int i = 0; i < n; i++)
        printf("type %d:\n\t(%d, %d, %d)\n\t(%d, %d, %d)\n",
            ranges[i].type, 
            ranges[i].start_x, ranges[i].start_y, ranges[i].start_z, 
            ranges[i].end_x, ranges[i].end_y, ranges[i].end_z);*/

    return (0);
}
