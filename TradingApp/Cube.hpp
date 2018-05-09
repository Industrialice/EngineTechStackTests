#pragma once

namespace EngineCore
{
    class Material;
    class RendererPipelineState;
    class RendererVertexArray;
    class RendererIndexArray;
    class Camera;
}

namespace TradingApp
{
    class Cube
    {
        shared_ptr<EngineCore::Material> _material{};
        shared_ptr<EngineCore::RendererVertexArray> _vertexArray{};
        shared_ptr<EngineCore::RendererIndexArray> _indexArray{};
        shared_ptr<EngineCore::RendererPipelineState> _pipelineState{};

    public:
        Cube();
        void Draw(const EngineCore::Camera *camera, const Vector3 &position, const Vector3 &rotation, f32 size);
        void Draw(const EngineCore::Camera *camera, const Vector3 &position, const Quaternion &rotation, f32 size);
    };
}