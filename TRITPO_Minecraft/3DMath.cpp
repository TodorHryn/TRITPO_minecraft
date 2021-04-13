#include "3DMath.h"

Vec3f operator-(const Vec3f &a)
{
    return Vec3f(-a.x, -a.y, -a.z);
}

Vec3f operator+(const Vec3f &a, const Vec3f &b)
{
    return Vec3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3f operator-(const Vec3f &a, const Vec3f &b)
{
    return Vec3f(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3f operator*(const Vec3f &a, float f)
{
    return Vec3f(a.x * f, a.y * f, a.z * f);
}

Vec3f operator*(float f, const Vec3f &a)
{
    return Vec3f(a.x * f, a.y * f, a.z * f);
}

Vec3f operator/(const Vec3f &a, float f)
{
    return Vec3f(a.x / f, a.y / f, a.z / f);
}

inline float length(const Vec3f &v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float length2(const Vec3f &v)
{
    return (v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float dot(const Vec3f &a, const Vec3f &b)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}

inline Vec3f cross(const Vec3f &a, const Vec3f &b)
{
    Vec3f result;

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return (result);
}

inline Vec3f normalize(const Vec3f &v)
{
    Vec3f result;

    float len = length(v);
    if (len > 0.0001f)
    {
        result.x = v.x / len;
        result.y = v.y / len;
        result.z = v.z / len;
    }

    return (result);
}

Mat3x3f mat3x3f_identity(void)
{
    Mat3x3f result;

    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;

    return (result);
}

Vec4f vec4f_mul(const Vec4f &v, const Mat4x4f &m)
{
    Vec4f result;

    for (int j = 0; j < 4; j++)
    {
        for (int k = 0; k < 4; k++)
        {
            result.v[j] += m.m[k][j] * v.v[k];
        }
    }

    return (result);
}

Vec3f vec3f_mul(const Vec3f &v, const Mat3x3f &m)
{
    Vec3f result;

    for (int j = 0; j < 3; j++)
    {
        for (int k = 0; k < 3; k++)
        {
            result.v[j] += v.v[k] * m.m[k][j];
        }
    }

    return (result);
}

Mat4x4f mat4x4f_mul(const Mat4x4f &m0, const Mat4x4f &m1)
{
    Mat4x4f result;

    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            for (int k = 0; k < 4; k++)
            {
                result.m[j][i] += m0.m[j][k] * m1.m[k][i];
            }
        }
    }

    return (result);
}

Mat4x4f mat4x4f_transpose(const Mat4x4f &m)
{
    Mat4x4f result;

    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            result.m[i][j] = m.m[j][i];
        }
    }

    return (result);
}

Mat4x4f mat4x4f_identity(void)
{
    Mat4x4f result;

    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;

    return (result);
}

Mat4x4f mat4x4f_rotate_x(const Mat4x4f &m, float angle)
{
    Mat4x4f result;

    result.m[0][0] = 1.0f;
    result.m[1][1] = cosf(angle);
    result.m[2][2] = cosf(angle);
    result.m[3][3] = 1.0f;

    result.m[1][2] = -sinf(angle);
    result.m[2][1] = sinf(angle);

    return mat4x4f_mul(m, mat4x4f_transpose(result));
}

Mat4x4f mat4x4f_rotate_y(const Mat4x4f &m, float angle)
{
    Mat4x4f result;

    result.m[0][0] = cosf(angle);
    result.m[1][1] = 1.0f;
    result.m[2][2] = cosf(angle);
    result.m[3][3] = 1.0f;

    result.m[0][2] = sinf(angle);
    result.m[2][0] = -sinf(angle);

    return mat4x4f_mul(m, mat4x4f_transpose(result));
}

Mat4x4f mat4x4f_translate(const Mat4x4f &m, const Vec3f &v)
{
    Mat4x4f result = mat4x4f_identity();

    result.m[3][0] = v.x;
    result.m[3][1] = v.y;
    result.m[3][2] = v.z;

    return mat4x4f_mul(m, result);
}

Mat4x4f mat4x4f_perspective(float fov, float aspect_ration, float n, float f)
{
    Mat4x4f result;

    float ctg = 1.0f / tanf(fov * (PI / 360.0f));

    result.m[0][0] = ctg / aspect_ration;
    result.m[1][1] = ctg;
    result.m[2][3] = -1.0f;
    result.m[2][2] = (n + f) / (n - f);
    result.m[3][2] = (2.0f * n * f) / (n - f);

    return (result);
}

void mat3x3f_set_col(Mat3x3f &m, const Vec3f &v, int col)
{
    m.m[0][col] = v.x;
    m.m[1][col] = v.y;
    m.m[2][col] = v.z;
}

void mat3x3f_set_row(Mat3x3f &m, const Vec3f &v, int row)
{
    m.m[row][0] = v.x;
    m.m[row][1] = v.y;
    m.m[row][2] = v.z;
}

Mat3x3f mat3x3f_transpose(const Mat3x3f &m)
{
    Mat3x3f result;

    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            result.m[i][j] = m.m[j][i];
        }
    }

    return (result);
}

void mat4x4f_set_row(Mat4x4f &m, const Vec4f &v, int row)
{
    m.m[row][0] = v.x;
    m.m[row][1] = v.y;
    m.m[row][2] = v.z;
    m.m[row][3] = v.w;
}

Mat4x4f mat4x4f_lookat(const Vec3f &from, const Vec3f &to, const Vec3f &up)
{
    Vec3f z_dir = normalize(from - to);
    Vec3f x_dir = normalize(cross(up, z_dir));
    Vec3f y_dir = normalize(cross(z_dir, x_dir));

    Mat4x4f result;

    result.m[0][0] = x_dir.x;
    result.m[1][0] = x_dir.y;
    result.m[2][0] = x_dir.z;
    result.m[3][0] = dot(-from, x_dir);

    result.m[0][1] = y_dir.x;
    result.m[1][1] = y_dir.y;
    result.m[2][1] = y_dir.z;
    result.m[3][1] = dot(-from, y_dir);

    result.m[0][2] = z_dir.x;
    result.m[1][2] = z_dir.y;
    result.m[2][2] = z_dir.z;
    result.m[3][2] = dot(-from, z_dir);

    result.m[3][3] = 1.0f;
    
    return (result);
}
