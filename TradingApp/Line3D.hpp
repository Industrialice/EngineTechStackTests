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
    class Line3D
    {
        shared_ptr<EngineCore::Material> _material{};
        shared_ptr<EngineCore::RendererVertexArray> _vertexArray{};
        shared_ptr<EngineCore::RendererIndexArray> _indexArray{};
        shared_ptr<EngineCore::RendererPipelineState> _pipelineState{};
        shared_ptr<EngineCore::Material> _materialForDepthWrite{};
        shared_ptr<EngineCore::RendererPipelineState> _pipelineStateForDepthWrite{};

    public:
        Line3D();
        void Draw(const EngineCore::Camera *camera, const EngineCore::Vector3 &point0, const EngineCore::Vector3 &point1, f32 thickness);
    };
}