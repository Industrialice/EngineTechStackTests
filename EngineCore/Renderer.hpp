#pragma once

#include "System.hpp"
#include "RendererDataResource.hpp"

namespace EngineCore
{
	enum class PrimitiveTopology
	{
		Point,
		TriangleEnumeration,
		TriangleStrip,
		TriangleFan
	};

    class RenderTarget;
    class RendererPipelineState;
    class Material;
    class Shader;
    class RendererArray;
    class RendererVertexArray;
    class RendererIndexArray;
    class RendererCommandBuffer;
    class Camera;

    struct Matrix4x4;
    struct Matrix4x3;
    struct Vector3;

	class Renderer
	{
    private:
		Renderer(Renderer &&) = delete;
		Renderer &operator = (Renderer &&) = delete;

	protected:
        friend class RendererArray;
        friend class RendererVertexArray;
        friend class RendererIndexArray;
        friend class RendererComputeBuffer;
        friend class Texture;

		Renderer() = default;

        template <typename T> static T *RendererBackendData(const RendererFrontendData &frontendData)
        {
            return (T *)RendererBackendData(frontendData);
        }

		static void *RendererBackendData(const RendererFrontendData &frontendData);
		static void RendererBackendData(const RendererFrontendData &frontendData, void *data);
        static void **RendererBackendDataPointer(const RendererFrontendData &frontendData);
		static bool RendererFrontendDataDirtyState(const RendererFrontendData &frontendData);
		static void RendererFrontendDataDirtyState(const RendererFrontendData &frontendData, bool isChanged);

        virtual bool CreateArrayRegion(const RendererArray &array, BufferOwnedData data) = 0;
        virtual void UpdateArrayRegion(const RendererArray &array, BufferOwnedData data, ui32 sizeInBytes, ui32 offsetInBytes) = 0;
        virtual byte *LockArrayRegionForWrite(const RendererArray &array, ui32 sizeInBytes, ui32 offsetInBytes) = 0;
        virtual void UnlockArrayRegion(const RendererArray &array) = 0;

        virtual bool CreateTextureRegion(const Texture &texture, BufferOwnedData data, TextureDataFormat dataFormat) = 0;

	public:
		virtual ~Renderer() = default;

        virtual void NotifyFrontendDataIsBeingDeleted(const RendererFrontendData &frontendData) = 0;

		virtual void ClearCameraTargets(const Camera *camera) = 0;

        // pass nullptr to unbind the currently binded array
        virtual bool BindVertexArray(const shared_ptr<const RendererVertexArray> &array, ui32 number) = 0;
        virtual bool BindIndexArray(const shared_ptr<const RendererIndexArray> &array) = 0;

        virtual void DrawIntoRenderTarget(const RenderTarget *rt, const Vector3 *cameraPos, const Matrix4x3 *viewMatrix, const Matrix4x4 *projMatrix, const Matrix4x3 *modelMatrix, const RendererPipelineState *pipelineState, const Material *material, PrimitiveTopology topology, ui32 numVertices, ui32 instanceCount = 1) = 0;
        virtual void DrawIndexedIntoRenderTarget(const RenderTarget *rt, const Vector3 *cameraPos, const Matrix4x3 *viewMatrix, const Matrix4x4 *projMatrix, const Matrix4x3 *modelMatrix, const RendererPipelineState *pipelineState, const Material *material, PrimitiveTopology topology, ui32 numIndexes, ui32 instanceCount = 1) = 0;

        virtual void DrawWithCamera(const Camera *camera, const Matrix4x3 *modelMatrix, const RendererPipelineState *pipelineState, const Material *material, PrimitiveTopology topology, ui32 numVertices, ui32 instanceCount = 1) = 0;
        virtual void DrawIndexedWithCamera(const Camera *camera, const Matrix4x3 *modelMatrix, const RendererPipelineState *pipelineState, const Material *material, PrimitiveTopology topology, ui32 numIndexes, ui32 instanceCount = 1) = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void SwapBuffers() = 0;
	};
}