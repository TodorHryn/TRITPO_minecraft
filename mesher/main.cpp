
#include <stdio.h>
#include <stdlib.h>

struct Range1d
{
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

void mesh_1d(int *blocks, int width, int count, Range1d *ranges, int *num_of_ranges)
{

    int *visited = (int *)calloc(width, sizeof(int));
    int ranges_count = 0;
    while (count > 0)
    {
        int start = 0;
        int end   = 0;
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

        if (start < width)
        {
            visited[start] = 1;
            count--;
        }

        end = start;
        while (end + 1 < width && blocks[end + 1] != 0)
        {
            visited[end + 1] = 1;
            end++;
            count--;
        }

        ranges[ranges_count++] = {start, end};

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
}

void test_1d_meshing(void)
{
    const int width1 = 16;
    int blocks1[width1] = {
        1, 1, 1, 1,  0, 0,  1, 1, 1,  0,  1,  0,  1, 0, 1, 1
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

    int width   = width4;
    int *blocks = blocks4;

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
            printf("(%d, %d)\n", ranges[i].start, ranges[i].end);
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

int main(void)
{
    //test_1d_meshing();
    test_2d_meshing();
    return (0);
}
