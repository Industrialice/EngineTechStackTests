#include "BasicHeader.hpp"
#include "MatrixMathTypes.hpp"
#include "_MatrixMathFunctions.hpp"

using namespace EngineCore;

namespace EngineCore
{
    template struct _Vector<Vector2Base<f32>>;
    template struct _Vector<Vector3Base<f32>>;
    template struct _Vector<Vector4Base<f32>>;

    template struct _Matrix<2, 2>;
    template struct _Matrix<3, 2>;
    template struct _Matrix<2, 3>;
    template struct _Matrix<3, 3>;
    template struct _Matrix<4, 3>;
    template struct _Matrix<3, 4>;
    template struct _Matrix<4, 4>;

    template struct Rectangle<f32>;
    template struct Rectangle<bool>;
    template struct Rectangle<i32>;
}

/////////////
// Vector2 //
/////////////

Vector2 Vector2::GetLeftNormal() const
{
    return {-y, x};
}

Vector2 Vector2::GetRightNormal() const
{
    return {y, x};
}

/////////////
// Vector3 //
/////////////

void Vector3::Cross(const Vector3 &other)
{
    f32 x = this->y * other.z - this->z * other.y;
    f32 y = this->z * other.x - this->x * other.z;
    f32 z = this->x * other.y - this->y * other.x;

    x = x;
    y = y;
    z = z;
}

Vector3 Vector3::GetCrossed(const Vector3 &other) const
{
    f32 x = this->y * other.z - this->z * other.y;
    f32 y = this->z * other.x - this->x * other.z;
    f32 z = this->x * other.y - this->y * other.x;

    return Vector3(x, y, z);
}

///////////////
// Matrix3x2 //
///////////////

Matrix3x3 Matrix3x2::GetInversed() const
{
    Matrix3x3 r;

    r[0][0] = elements[1][1];
    r[1][0] = -elements[1][0];
    r[2][0] = elements[1][0] * elements[2][1] - elements[1][1] * elements[2][0];

    r[0][1] = -elements[0][1];
    r[1][1] = elements[0][0];
    r[2][1] = -elements[0][0] * elements[2][1] - elements[0][1] * elements[2][0];

    r[0][2] = 0;
    r[1][2] = 0;
    r[2][2] = +(elements[0][0] * elements[1][1] - elements[0][1] * elements[1][0]);

    f32 det = elements[0][0] * r[0][0] + elements[0][1] * r[1][0];
    f32 revDet = 1.f / det;

    return r * revDet;
}

Matrix3x2 Matrix3x2::GetInversedClipped() const
{
    Matrix3x2 r;

    r[0][0] = elements[1][1];
    r[1][0] = -elements[1][0];
    r[2][0] = elements[1][0] * elements[2][1] - elements[1][1] * elements[2][0];

    r[0][1] = -elements[0][1];
    r[1][1] = elements[0][0];
    r[2][1] = -elements[0][0] * elements[2][1] - elements[0][1] * elements[2][0];

    f32 det = elements[0][0] * r[0][0] + elements[0][1] * r[1][0];
    f32 revDet = 1.f / det;

    return r * revDet;
}

Matrix3x2 Matrix3x2::CreateRTS(const optional<f32> &rotation, const optional<Vector2> &translation, const optional<Vector2> &scale)
{
    Matrix3x2 result;

    if (rotation)
    {
        f32 cr = cos(*rotation);
        f32 sr = sin(*rotation);

        result[0][0] = cr; result[0][1] = -sr;
        result[1][0] = sr; result[1][1] = cr;
    }

    if (translation)
    {
        result[2][0] = translation->x; result[2][1] = translation->y;
    }

    if (scale)
    {
        result[0][0] *= scale->x;
        result[0][1] *= scale->x;
        result[1][0] *= scale->y;
        result[1][1] *= scale->y;
    }

    return result;
}

///////////////
// Matrix4x3 //
///////////////

Matrix4x4 Matrix4x3::operator*(const Matrix4x4 &other) const
{
    Matrix4x4 result;
    for (uiw rowIndex = 0; rowIndex < 4; ++rowIndex)
    {
        for (uiw columnIndex = 0; columnIndex < 4; ++columnIndex)
        {
            result[rowIndex][columnIndex] =
                elements[rowIndex][0] * other[0][columnIndex] +
                elements[rowIndex][1] * other[1][columnIndex] +
                elements[rowIndex][2] * other[2][columnIndex];
            if (rowIndex == 3)
            {
                result[rowIndex][columnIndex] += other[3][columnIndex];
            }
        }
    }
    return result;
}

Matrix4x3 Matrix4x3::CreateRotationAroundAxis(const Vector3 &axis, f32 angle)
{
    return EngineCore::CreateRotationAroundAxis<Matrix4x3>(axis, angle);
}

Matrix4x3 Matrix4x3::CreateRTS(const optional<Vector3> &rotation, const optional<Vector3> &translation, const optional<Vector3> &scale)
{
    return EngineCore::CreateRTS<Matrix4x3, true>(rotation, translation, scale);
}

///////////////
// Matrix3x4 //
///////////////

Matrix3x4 Matrix3x4::CreateRotationAroundAxis(const Vector3 &axis, f32 angle)
{
    return EngineCore::CreateRotationAroundAxis<Matrix3x4>(axis, angle);
}

Matrix3x4 Matrix3x4::CreateRS(const Vector3 &rotation, const optional<Vector3> &scale)
{
    return EngineCore::CreateRTS<Matrix3x4, false>(rotation, nullopt, scale);
}

///////////////
// Matrix4x4 //
///////////////

Matrix4x4 Matrix4x4::CreateRotationAroundAxis(const Vector3 &axis, f32 angle)
{
    return EngineCore::CreateRotationAroundAxis<Matrix4x4>(axis, angle);
}

Matrix4x4 Matrix4x4::CreateRTS(const optional<Vector3> &rotation, const optional<Vector3> &translation, const optional<Vector3> &scale)
{
    return EngineCore::CreateRTS<Matrix4x4, true>(rotation, translation, scale);
}

Matrix4x4 Matrix4x4::CreatePerspectiveProjection(f32 horizontalFOV, f32 aspectRatio, f32 nearPlane, f32 farPlane)
{
    Matrix4x4 result;

    f32 fovH = horizontalFOV;
    f32 tanOfHalfFovH = tan(fovH * 0.5f);
    f32 fovV = 2.0f * atan(tanOfHalfFovH / aspectRatio);
    f32 q = farPlane / (farPlane - nearPlane);
    f32 w = 1.0f / tanOfHalfFovH;
    f32 h = 1.0f / tan(fovV * 0.5f);
    f32 l = -q * nearPlane;

    result[0][0] = w; result[0][1] = 0; result[0][2] = 0; result[0][3] = 0;
    result[1][0] = 0; result[1][1] = h; result[1][2] = 0; result[1][3] = 0;
    result[2][0] = 0; result[2][1] = 0; result[2][2] = q; result[2][3] = 1;
    result[3][0] = 0; result[3][1] = 0; result[3][2] = l; result[3][3] = 0;

    return result;
}

///////////////
// Matrix3x3 //
///////////////

Matrix3x3 Matrix3x3::CreateRotationAroundAxis(const Vector3 &axis, f32 angle)
{
    return EngineCore::CreateRotationAroundAxis<Matrix3x3>(axis, angle);
}

Matrix3x3 Matrix3x3::CreateRS(const Vector3 &rotation, const optional<Vector3> &scale)
{
    return EngineCore::CreateRTS<Matrix3x3, false>(rotation, nullopt, scale);
}