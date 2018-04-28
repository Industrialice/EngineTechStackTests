#pragma once

namespace EngineCore
{
    class Material;
    class RendererPipelineState;
    class RendererVertexArray;
    class RendererIndexArray;
    class Camera;
    struct Vector3;
    struct Quaternion;
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
        void Draw(const EngineCore::Camera *camera, const EngineCore::Vector3 &position, const EngineCore::Vector3 &rotation, f32 size);
        void Draw(const EngineCore::Camera *camera, const EngineCore::Vector3 &position, const EngineCore::Quaternion &rotation, f32 size);
    };
}