#pragma once

namespace EngineCore
{
    class Material;
    class RendererPipelineState;
    class RendererVertexArray;
    class RendererIndexArray;
    class Camera;
}

#include <MatrixMathTypes.hpp>

namespace TradingApp
{
    class SpheresInstanced
    {
        shared_ptr<EngineCore::Material> _material{};
        shared_ptr<EngineCore::RendererVertexArray> _vertexArray{};
        shared_ptr<EngineCore::RendererVertexArray> _vertexInstanceArray{};
        shared_ptr<EngineCore::RendererIndexArray> _indexArray{};
        shared_ptr<EngineCore::RendererPipelineState> _pipelineState{};

    public:
        struct InstanceData
        {
            Quaternion rotation;
            Vector3 position;
            f32 size;
        };

        SpheresInstanced(ui32 slices, ui32 layerCuts, ui32 maxInstances);
        InstanceData *Lock(ui32 instancesCount);
        void Unlock();
        void Draw(const EngineCore::Camera *camera, ui32 instancesCount);
    };
}