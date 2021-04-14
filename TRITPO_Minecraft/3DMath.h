#pragma once
#include <cmath>

#define PI 3.14159265359f
#define TWO_PI (2.0f * PI)

struct Vec3f
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };

        struct
        {
            float r;
            float g;
            float b;
        };

        struct
        {
            float pitch;
            float roll;
            float yaw;
        };

        float v[3];
    };

    Vec3f(float xx = 0.0f, float yy = 0.0f, float zz = 0.0f) : x(xx), y(yy), z(zz) {}
};

struct Vec4f
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
            float w;
        };

        float v[4];
    };

    Vec4f(float xx = 0.0f, float yy = 0.0f, float zz = 0.0f, float ww = 0.0f) : x(xx), y(yy), z(zz), w(ww) {}
};

struct Mat4x4f
{
    float m[4][4];

    Mat4x4f()
    {
        float *p = &m[0][0];
        for (int i = 0; i < 4 * 4; i++)
        {
            *p++ = 0.0f;
        }
    }
};

struct Mat3x3f
{
    float m[3][3];
    Mat3x3f()
    {
        float *p = &m[0][0];
        for (int i = 0; i < 3 * 3; i++)
        {
            *p++ = 0.0f;
        }
    }
};


Vec3f operator-(const Vec3f &a);
Vec3f operator+(const Vec3f &a, const Vec3f &b);
Vec3f operator-(const Vec3f &a, const Vec3f &b);
Vec3f operator*(const Vec3f &a, float f);
Vec3f operator*(float f, const Vec3f &a);
Vec3f operator/(const Vec3f &a, float f);

inline float length(const Vec3f &v);
inline float length2(const Vec3f &v);
inline float dot(const Vec3f &a, const Vec3f &b);
inline Vec3f cross(const Vec3f &a, const Vec3f &b);
inline Vec3f normalize(const Vec3f &v);
Mat3x3f mat3x3f_identity(void);
Vec4f vec4f_mul(const Vec4f &v, const Mat4x4f &m);
Vec3f vec3f_mul(const Vec3f &v, const Mat3x3f &m);
Mat4x4f mat4x4f_mul(const Mat4x4f &m0, const Mat4x4f &m1);
Mat4x4f mat4x4f_transpose(const Mat4x4f &m);
Mat4x4f mat4x4f_identity(void);
Mat4x4f mat4x4f_rotate_x(const Mat4x4f &m, float angle);
Mat4x4f mat4x4f_rotate_y(const Mat4x4f &m, float angle);
Mat4x4f mat4x4f_translate(const Mat4x4f &m, const Vec3f &v);
Mat4x4f mat4x4f_perspective(float fov, float aspect_ration, float n, float f);
void mat3x3f_set_col(Mat3x3f &m, const Vec3f &v, int col);
void mat3x3f_set_row(Mat3x3f &m, const Vec3f &v, int row);
Mat3x3f mat3x3f_transpose(const Mat3x3f &m);
void mat4x4f_set_row(Mat4x4f &m, const Vec4f &v, int row);
Mat4x4f mat4x4f_lookat(const Vec3f &from, const Vec3f &to, const Vec3f &up);
