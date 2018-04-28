#include "BasicHeader.hpp"
#include "Camera.hpp"
#include "Application.hpp"
#include "RenderTarget.hpp"
#include "Texture.hpp"
#include "MathFunctions.hpp"

using namespace EngineCore;

shared_ptr<Camera> Camera::New()
{
    struct Proxy : public Camera
    {
        Proxy() : Camera() {}
    };
    return make_shared<Proxy>();
}

const Vector3 &Camera::Position() const
{
    return _position;
}

void Camera::Position(const Vector3 &newPosition)
{
    _position = newPosition;
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

Vector3 Camera::Rotation() const
{
    return {_pitch, _yaw, _roll};
}

void Camera::Rotation(const Vector3 &pitchYawRollAngles)
{
    _pitch = pitchYawRollAngles.x;
    _yaw = pitchYawRollAngles.y;
    _roll = pitchYawRollAngles.z;
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

f32 Camera::Pitch() const
{
    return _pitch;
}

void Camera::Pitch(f32 angle)
{
    _pitch = angle;
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

f32 Camera::Yaw() const
{
    return _yaw;
}

void Camera::Yaw(f32 angle)
{
    _yaw = angle;
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

f32 Camera::Roll() const
{
    return _roll;
}

void Camera::Roll(f32 angle)
{
    _roll = angle;
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

Vector3 Camera::RightAxis() const
{
    auto rotMatrix = Matrix3x3::CreateRS(Vector3{_pitch, _yaw, _roll});
    return Vector3(1, 0, 0) * rotMatrix;
}

Vector3 Camera::UpAxis() const
{
    auto rotMatrix = Matrix3x3::CreateRS(Vector3{_pitch, _yaw, _roll});
    return Vector3(0, 1, 0) * rotMatrix;
}

Vector3 Camera::ForwardAxis() const
{
    auto rotMatrix = Matrix3x3::CreateRS(Vector3{_pitch, _yaw, _roll});
    return Vector3(0, 0, 1) * rotMatrix;
}

void Camera::RotateAroundRightAxis(f32 pitchAngle)
{
    _pitch = RadNormalize(_pitch + pitchAngle);
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

void Camera::RotateAroundUpAxis(f32 yawAngle)
{
    _yaw = RadNormalize(_yaw + yawAngle);
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

void Camera::RotateAroundForwardAxis(f32 rollAngle)
{
    _roll = RadNormalize(_roll + rollAngle);
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

void Camera::Rotate(const Vector3 &pitchYawRollAngles)
{
    if (pitchYawRollAngles.x)
    {
        RotateAroundRightAxis(pitchYawRollAngles.x);
    }
    if (pitchYawRollAngles.y)
    {
        RotateAroundUpAxis(pitchYawRollAngles.y);
    }
    if (pitchYawRollAngles.z)
    {
        RotateAroundForwardAxis(pitchYawRollAngles.z);
    }
}

void Camera::MoveAlongRightAxis(f32 shift)
{
    _position += RightAxis() * shift;
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

void Camera::MoveAlongUpAxis(f32 shift)
{
    _position += UpAxis() * shift;
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

void Camera::MoveAlongForwardAxis(f32 shift)
{
    _position += ForwardAxis() * shift;
    _viewMatrix = nullopt;
    _viewProjMatrix = nullopt;
}

void Camera::Move(const Vector3 &shift)
{
    if (shift.x)
    {
        MoveAlongRightAxis(shift.x);
    }
    if (shift.y)
    {
        MoveAlongUpAxis(shift.y);
    }
    if (shift.z)
    {
        MoveAlongForwardAxis(shift.z);
    }
}

void Camera::RenderTarget(const shared_ptr<class RenderTarget> &renderTarget)
{
	_renderTarget = renderTarget;
}

auto Camera::RenderTarget() const -> const shared_ptr<class RenderTarget> &
{
	return _renderTarget;
}

void Camera::ClearColorRGBA(optional<array<f32, 4>> color)
{
	_clearColorRGBA = color;
}

optional<array<f32, 4>> Camera::ClearColorRGBA() const
{
	return _clearColorRGBA;
}

void Camera::ClearDepthValue(optional<f32> value)
{
	_clearDepth = value;
}

optional<f32> Camera::ClearDepthValue() const
{
	return _clearDepth;
}

void Camera::ClearStencilValue(optional<ui8> value)
{
	_clearStencil = value;
}

optional<ui8> Camera::ClearStencilValue() const
{
	return _clearStencil;
}

void Camera::Name(string_view name)
{
	_name = name;
}

string_view Camera::Name() const
{
	if (_name.empty())
	{
		return "{empty}";
	}
	return _name;
}

auto EngineCore::Camera::ProjectionType() const -> ProjectionTypet
{
    return _projType;
}

void Camera::ProjectionType(const ProjectionTypet type)
{
    if (_projType != type)
    {
        _projType = type;
        _projMatrix = nullopt;
        _viewProjMatrix = nullopt;
    }
}

const Matrix4x3 &Camera::ViewMatrix() const
{
    if (!_viewMatrix)
    {
        auto xAxis = RightAxis();
        auto yAxis = UpAxis();
        auto zAxis = ForwardAxis();
        Vector3 row0{xAxis.x, yAxis.x, zAxis.x};
        Vector3 row1{xAxis.y, yAxis.y, zAxis.y};
        Vector3 row2{xAxis.z, yAxis.z, zAxis.z};
        Vector3 pos{-xAxis.Dot(_position), -yAxis.Dot(_position), -zAxis.Dot(_position)};

        *_viewMatrix = Matrix4x3(row0, row1, row2, pos);
    }
    return *_viewMatrix;
}

const Matrix4x4 &Camera::ProjectionMatrix() const
{
    _viewProjMatrix = nullopt;

    ui32 width, height;

    if (_renderTarget->ColorTarget() != nullptr)
    {
        width = _renderTarget->ColorTarget()->Width();
        height = _renderTarget->ColorTarget()->Height();
    }
    else if (_renderTarget->DepthStencilTarget() != nullptr)
    {
        width = _renderTarget->DepthStencilTarget()->Width();
        height = _renderTarget->DepthStencilTarget()->Height();
    }
    else
    {
        width = Application::GetMainWindow().width;
        height = Application::GetMainWindow().height;
    }

    switch (_projType)
    {
    case ProjectionTypet::Perspective:
        _projMatrix = Matrix4x4::CreatePerspectiveProjection(DegToRad(_horFOVDeg), (f32)width / (f32)height, _nearPlane, _farPlane);
        break;
    case ProjectionTypet::Orthogonal:
        NOIMPL;
        break;
    }

    return *_projMatrix;
}

const Matrix4x4 &Camera::ViewProjectionMatrix() const
{
    if (!_viewProjMatrix)
    {
        _viewProjMatrix = ViewMatrix() * ProjectionMatrix();
    }
    return *_viewProjMatrix;
}
