#pragma once

namespace EngineCore
{
    class Material;
    class RendererPipelineState;
    class RendererVertexArray;
    class RendererIndexArray;
    class Camera;
    struct Vector3;
}

namespace TradingApp
{
    class Plane
    {
        shared_ptr<EngineCore::Material> _material{};
        shared_ptr<EngineCore::RendererVertexArray> _vertexArray{};
        shared_ptr<EngineCore::RendererPipelineState> _pipelineState{};

    public:
        Plane();
        void Draw(const EngineCore::Camera *camera, const EngineCore::Vector3 &position, f32 size);
    };
}