#pragma once

#include "MatrixMathTypes.hpp"

namespace EngineCore
{
	class Camera
	{
    public:
        enum class ProjectionTypet { Perspective, Orthogonal };

    private:
        Vector3 _position{};
        f32 _yaw{}, _pitch{}, _roll{};
        shared_ptr<class RenderTarget> _renderTarget{};
		f32 _horFOVDeg = 75.0f;
		f32 _nearPlane = 0.1f;
		f32 _farPlane = 1000.0f;
        optional<array<f32, 4>> _clearColorRGBA{};
        optional<f32> _clearDepth{};
        optional<ui8> _clearStencil{};
        string _name{};
        ProjectionTypet _projType = ProjectionTypet::Perspective;
        // these are just cached values, so mutable makes sense
        mutable optional<Matrix4x3> _viewMatrix{};
        mutable optional<Matrix4x4> _projMatrix{};
        mutable optional<Matrix4x4> _viewProjMatrix{};

		Camera(Camera &&) = delete;
		Camera &operator = (Camera &&) = delete;

    protected:
        Camera() = default;

	public:
        static shared_ptr<Camera> New();

        const Vector3 &Position() const;
        void Position(const Vector3 &newPosition);
        Vector3 Rotation() const;
        void Rotation(const Vector3 &pitchYawRollAngles);
        f32 Pitch() const;
        void Pitch(f32 angle);
        f32 Yaw() const;
        void Yaw(f32 angle);
        f32 Roll() const;
        void Roll(f32 angle);
        Vector3 RightAxis() const;
        Vector3 UpAxis() const;
        Vector3 ForwardAxis() const;
        void RotateAroundRightAxis(f32 pitchAngle);
        void RotateAroundUpAxis(f32 yawAngle);
        void RotateAroundForwardAxis(f32 rollAngle);
        void Rotate(const Vector3 &pitchYawRollAngles);
        void MoveAlongRightAxis(f32 shift);
        void MoveAlongUpAxis(f32 shift);
        void MoveAlongForwardAxis(f32 shift);
        void Move(const Vector3 &shift);
		void RenderTarget(const shared_ptr<class RenderTarget> &renderTarget);
		const shared_ptr<class RenderTarget> &RenderTarget() const;
		void ClearColorRGBA(optional<array<f32, 4>> color);
		optional<array<f32, 4>> ClearColorRGBA() const;
		void ClearDepthValue(optional<f32> value);
		optional<f32> ClearDepthValue() const;
		void ClearStencilValue(optional<ui8> value);
		optional<ui8> ClearStencilValue() const;
		void Name(string_view name);
		string_view Name() const;
        ProjectionTypet ProjectionType() const;
        void ProjectionType(const ProjectionTypet type);
        const Matrix4x3 &ViewMatrix() const;
        const Matrix4x4 &ProjectionMatrix() const;
        const Matrix4x4 &ViewProjectionMatrix() const;
	};
}