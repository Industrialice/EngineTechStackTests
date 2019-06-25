#include "BasicHeader.hpp"
#include "OpenGLRendererProxy.h"
#include "BackendData.hpp"
#include <Application.hpp>
#include <Logger.hpp>
#include <Camera.hpp>
#include <RenderTarget.hpp>
#include <Texture.hpp>
#include <Material.hpp>
#include <TextureSampler.hpp>
#include <RendererPipelineState.hpp>
#include <RendererArray.hpp>
#include <MatrixMathTypes.hpp>
#include <unordered_set>
#include <map>

#ifdef WINPLATFORM
#include "OpenGLContextWindows.hpp"
#endif

using namespace EngineCore;
using namespace OGLRenderer;

static void APIENTRY OGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

static inline GLenum PrimitiveTopologyToOGL(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::Point:
        return GL_POINTS;
    case PrimitiveTopology::TriangleEnumeration:
        return GL_TRIANGLES;
    case PrimitiveTopology::TriangleFan:
        return GL_TRIANGLE_FAN;
    case PrimitiveTopology::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    }

    UNREACHABLE;
    return GL_INVALID_ENUM;
}

static inline std::pair<GLint, GLenum> VertexAttributeTypeToOGL(VertexLayout::Attribute::Formatt format)
{
    constexpr auto &topair = make_pair<GLint, GLenum>;

    switch (format)
    {
    case VertexLayout::Attribute::Formatt::R32F:
        return topair(1, GL_FLOAT);
    case VertexLayout::Attribute::Formatt::R32G32F:
        return topair(2, GL_FLOAT);
    case VertexLayout::Attribute::Formatt::R32G32B32F:
        return topair(3, GL_FLOAT);
    case VertexLayout::Attribute::Formatt::R32G32B32A32F:
        return topair(4, GL_FLOAT);
    }

    UNREACHABLE;
    return topair(-1, GL_INVALID_ENUM);
}

static inline GLenum IndexTypeToOGL(RendererIndexArray::IndexTypet type)
{
    switch (type)
    {
    case RendererIndexArray::IndexTypet::UI8:
        return GL_UNSIGNED_BYTE;
    case RendererIndexArray::IndexTypet::UI16:
        return GL_UNSIGNED_SHORT;
    case RendererIndexArray::IndexTypet::UI32:
        return GL_UNSIGNED_INT;
    case RendererIndexArray::IndexTypet::Undefined:
        return GL_INVALID_ENUM;
    }
    
    UNREACHABLE;
    return GL_INVALID_ENUM;
}

class OpenGLRendererImpl final : public OpenGLRendererProxy
{
    std::unordered_set<RendererBackendDataBase *> _allocatedBackendDatas{}; // unique_ptr would've been preferable, but using it as a key may be... tricky
    GLuint _emptyVAO = 0;
    GLuint _intermediateVAO = 0;
    unique_ptr<class OpenGLContext> _context{};
    array<shared_ptr<const RendererVertexArray>, 8> _boundVertexArrays{};
    ui8 _vertexBufferBindingChanged = 0xFF;
    ui8 _boundVertexBuffersThatNotNullptr = 0;
    shared_ptr<const RendererIndexArray> _boundIndexArray{};
    bool _indexBufferBindingChanged = true;

public:
    virtual ~OpenGLRendererImpl()
    {
        for (auto &data : _allocatedBackendDatas)
        {
            *data->backendDataPointer = nullptr;
            delete data;
        }
    }

    virtual bool CreateArrayRegion(const RendererArray &array, BufferOwnedData data) override
    {
        HasGLErrors();

        if (RendererBackendData(array) == nullptr)
        {
            OpenGLRendererProxy::AllocateBackendData<ArrayBackendData>(array);
        }
        RendererFrontendDataDirtyState(array, false);

        auto &arrayData = *RendererBackendData<ArrayBackendData>(array);

        if (arrayData.oglBuffer == 0)
        {
            glGenBuffers(1, &arrayData.oglBuffer);
        }

        GLenum type = array.Type() == RendererArray::Typet::VertexArray ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
        GLenum usage = array.Access().cpuMode.writeMode == RendererArray::CPUAccessMode::Mode::FrequentFull || array.Access().cpuMode.writeMode == RendererArray::CPUAccessMode::Mode::FrequentPartial ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
        ui32 size = array.NumberOfElements() * array.Stride();

        glBindBuffer(type, arrayData.oglBuffer);
        glBufferData(type, size, data.get(), usage);

        arrayData.data = move(data);

        return HasGLErrors() == false;
    }

    virtual void UpdateArrayRegion(const RendererArray &array, BufferOwnedData data, ui32 sizeInBytes, ui32 offsetInBytes) override
    {
        assert(RendererBackendData(array) != nullptr);
        auto &arrayData = *RendererBackendData<ArrayBackendData>(array);
        assert(arrayData.oglBuffer != 0);

        if (data == nullptr)
        {
            SOFTBREAK;
            return;
        }

        GLenum type = array.Type() == RendererArray::Typet::VertexArray ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;

        glBindBuffer(type, arrayData.oglBuffer);
        glBufferSubData(type, offsetInBytes, sizeInBytes, data.get());

        ui32 arrayTotalSize = array.NumberOfElements() * array.Stride();
        if (sizeInBytes == arrayTotalSize)
        {
            arrayData.data = move(data);
        }
        else
        {
            MemOps::Copy(arrayData.data.get() + offsetInBytes, data.get(), sizeInBytes);
        }

        HasGLErrors();
    }

    virtual ui8 *LockArrayRegionForWrite(const RendererArray &array, ui32 sizeInBytes, ui32 offsetInBytes) override
    {
        assert(RendererBackendData(array) != nullptr);
        auto &arrayData = *RendererBackendData<ArrayBackendData>(array);
        assert(arrayData.oglBuffer != 0);

        ui32 arrayTotalSize = array.NumberOfElements() * array.Stride();

        if (arrayData.data == nullptr)
        {
            arrayData.data = EngineCore::BufferOwnedData{new ui8[arrayTotalSize], [](void *p) {delete[] p; }};
        }

        arrayData.lockStart = offsetInBytes;
        arrayData.lockEnd = arrayData.lockStart + sizeInBytes;

        return arrayData.data.get() + offsetInBytes;
    }

    virtual void UnlockArrayRegion(const RendererArray &array) override
    {
        if (RendererBackendData(array) == nullptr) // the array has never been locked with the current backend, it may or may not be an error
        {
            SOFTBREAK;
            return;
        }

        auto &arrayData = *RendererBackendData<ArrayBackendData>(array);
        assert(arrayData.oglBuffer != 0);

        if (arrayData.lockStart == arrayData.lockEnd)
        {
            HARDBREAK;
            return;
        }

        GLenum type = array.Type() == RendererArray::Typet::VertexArray ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;

        glBindBuffer(type, arrayData.oglBuffer);
        glBufferSubData(type, arrayData.lockStart, arrayData.lockEnd - arrayData.lockStart, arrayData.data.get() + arrayData.lockStart);

        arrayData.lockStart = arrayData.lockEnd = 0;

        HasGLErrors();
    }

    virtual bool CreateTextureRegion(const Texture &texture, BufferOwnedData data, TextureDataFormat dataFormat) override
    {
        return OpenGLRendererProxy::CreateTextureRegion(texture, move(data), dataFormat);
    }

    virtual void NotifyFrontendDataIsBeingDeleted(const RendererFrontendData &frontendData) override
    {
        auto castedData = RendererBackendData<RendererBackendDataBase>(frontendData);
        assert(castedData != nullptr); // there should be no notification for nullptr datas
        size_t removed = _allocatedBackendDatas.erase(castedData);
        assert(removed == 1);
    }

    OpenGLRendererImpl(unique_ptr<class OpenGLContext> context)
    {
        _context = move(context);

        _context->MakeCurrent();

        glGenVertexArrays(1, &_emptyVAO);

    #ifdef DEBUG
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OGLDebugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_TRUE);
    #endif

        SENDLOG(Info, "OpenGLRenderer's created\n");
    }

    void SetRenderTarget(const RenderTarget &rt)
    {
        ui32 width, height;

        if (rt.ColorTarget() != nullptr)
        {
            width = rt.ColorTarget()->Width();
            height = rt.ColorTarget()->Height();
        }
        else if (rt.DepthStencilTarget() != nullptr)
        {
            width = rt.DepthStencilTarget()->Width();
            height = rt.DepthStencilTarget()->Height();
        }
        else
        {
            width = Application::GetMainWindow().width;
            height = Application::GetMainWindow().height;
        }

        glViewport(0, 0, width, height);

        HasGLErrors();
    }

    virtual void ClearCameraTargets(const Camera *camera) override
    {
        if (camera == nullptr)
        {
            SENDLOG(Info, "OpenGLRenderer::ClearCameraTargets camera is null\n");
            SOFTBREAK;
            return;
        }

        const auto &rt = camera->RenderTarget();
        if (rt == nullptr)
        {
            SENDLOG(Info, "OpenGLRenderer::ClearCameraTargets camera has no render target\n");
            SOFTBREAK;
            return;
        }

        SetRenderTarget(*rt);

        CheckRenderTargetBackendData(*rt);

        RenderTargetBackendData *rtData = RendererBackendData<RenderTargetBackendData>(*rt);
        if (rtData == nullptr)
        {
            SENDLOG(Info, "OpenGLRenderer::ClearCameraTargets aborted, render target is invalid\n");
            SOFTBREAK;
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, rtData->frameBufferName);

        GLbitfield clearFlags = 0;

        auto clearColorRGBA = camera->ClearColorRGBA();
        if (clearColorRGBA != nullopt)
        {
            glClearColor(clearColorRGBA->operator[](0), clearColorRGBA->operator[](1), clearColorRGBA->operator[](2), clearColorRGBA->operator[](3));
            clearFlags |= GL_COLOR_BUFFER_BIT;
        }

        auto clearDepthValue = camera->ClearDepthValue();
        if (clearDepthValue != nullopt)
        {
			glDepthMask(GL_TRUE);
            glClearDepth(*clearDepthValue);
            clearFlags |= GL_DEPTH_BUFFER_BIT;
        }

        auto clearStencilValue = camera->ClearStencilValue();
        if (clearStencilValue != nullopt)
        {
            glClearStencil(*clearStencilValue);
            clearFlags |= GL_STENCIL_BUFFER_BIT;
        }

        glClear(clearFlags);

        HasGLErrors();
    }

    virtual bool BindVertexArray(const shared_ptr<const class RendererVertexArray> &array, ui32 number) override
    {
        if (number >= _boundVertexArrays.size())
        {
            SENDLOG(Error, "BindVertexArray called with number %u, but current renderer supports maximum %u arrays\n", number, (ui32)_boundVertexArrays.size());
            return false;
        }

        if (_boundVertexArrays[number] != array)
        {
            _boundVertexBuffersThatNotNullptr = Funcs::SetBit(_boundVertexBuffersThatNotNullptr, number, array != nullptr);
            _boundVertexArrays[number] = array;
            _vertexBufferBindingChanged = Funcs::SetBit(_vertexBufferBindingChanged, number, true);
        }

        return true;
    }

    virtual bool BindIndexArray(const shared_ptr<const class RendererIndexArray> &array) override
    {
        if (array != _boundIndexArray)
        {
            _boundIndexArray = array;
            _indexBufferBindingChanged = true;
        }

        return true;
    }

    virtual void DrawIntoRenderTarget(const RenderTarget *rt, const Vector3 *cameraPos, const Matrix4x3 *viewMatrix, const Matrix4x4 *projMatrix, const Matrix4x3 *modelMatrix, const RendererPipelineState *pipelineState, const Material *material, PrimitiveTopology topology, ui32 numVertices, ui32 instanceCount = 1) override
    {
        if (pipelineState == nullptr)
        {
            SENDLOG(Error, "Draw called with nullptr pipeline state\n");
            return;
        }

        if (material == nullptr)
        {
            SENDLOG(Error, "Draw called with nullptr material\n");
            return;
        }

        if (instanceCount == 0)
        {
            SOFTBREAK;
            SENDLOG(Warning, "Draw called with 0 instance count\n");
            return;
        }

        assert(_intermediateVAO == 0);

        if (false == DrawGeneric(rt, cameraPos, viewMatrix, projMatrix, modelMatrix, *pipelineState, *material, topology, numVertices))
        {
            glDeleteVertexArrays(1, &_intermediateVAO);
            _intermediateVAO = 0;
            return;
        }

        bool isProcedural = pipelineState->Shader()->InputAttributes().empty();

        if (isProcedural)
        {
            assert(_intermediateVAO == 0);
            glBindVertexArray(_emptyVAO);
        }

        if (instanceCount > 1)
        {
            glDrawArraysInstanced(PrimitiveTopologyToOGL(topology), 0, numVertices, instanceCount);
        }
        else
        {
            glDrawArrays(PrimitiveTopologyToOGL(topology), 0, numVertices);
        }

        glUseProgram(0);

        if (isProcedural == false)
        {
            glDeleteVertexArrays(1, &_intermediateVAO);
            _intermediateVAO = 0;
        }

        HasGLErrors();
    }

    virtual void DrawIndexedIntoRenderTarget(const RenderTarget *rt, const Vector3 *cameraPos, const Matrix4x3 *viewMatrix, const Matrix4x4 *projMatrix, const Matrix4x3 *modelMatrix, const RendererPipelineState *pipelineState, const Material *material, PrimitiveTopology topology, ui32 numIndexes, ui32 instanceCount = 1) override
    {
        if (pipelineState == nullptr)
        {
            SENDLOG(Error, "Draw called with nullptr pipeline state\n");
            return;
        }

        if (material == nullptr)
        {
            SENDLOG(Error, "Draw called with nullptr material\n");
            return;
        }

        if (instanceCount == 0)
        {
            SENDLOG(Warning, "Draw called with 0 instance count\n");
            return;
        }

        assert(_intermediateVAO == 0);

        if (_boundIndexArray == nullptr)
        {
            SENDLOG(Error, "DrawIndexed called with no index buffer bound\n");
            return;
        }

        if (_boundIndexArray->IsLocked())
        {
            SENDLOG(Error, "DrawIndexed was called, but the index buffer %*s is still locked\n", SVIEWARG(_boundIndexArray->Name()));
            return;
        }

        if (false == DrawGeneric(rt, cameraPos, viewMatrix, projMatrix, modelMatrix, *pipelineState, *material, topology, numIndexes))
        {
            glDeleteVertexArrays(1, &_intermediateVAO);
            _intermediateVAO = 0;
            return;
        }

        assert(_boundIndexArray->Type() == RendererArray::Typet::IndexArray);
        auto *indexArrayBackendData = RendererBackendData<ArrayBackendData>(*_boundIndexArray);
        if (indexArrayBackendData == nullptr || indexArrayBackendData->oglBuffer == 0)
        {
            SENDLOG(Error, "Invalid index array\n");
            return;
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexArrayBackendData->oglBuffer);

        if (instanceCount > 1)
        {
            glDrawElementsInstanced(PrimitiveTopologyToOGL(topology), numIndexes, IndexTypeToOGL(_boundIndexArray->IndexType()), nullptr, instanceCount);
        }
        else
        {
            glDrawElements(PrimitiveTopologyToOGL(topology), numIndexes, IndexTypeToOGL(_boundIndexArray->IndexType()), nullptr);
        }

        glUseProgram(0);
        glDeleteVertexArrays(1, &_intermediateVAO);
        _intermediateVAO = 0;

        HasGLErrors();
    }

    virtual void DrawWithCamera(const Camera *camera, const struct Matrix4x3 *modelMatrix, const class RendererPipelineState *pipelineState, const class Material *material, PrimitiveTopology topology, ui32 numVertices, ui32 instanceCount) override
    {
        if (camera == nullptr)
        {
            SENDLOG(Error, "DrawWithCamera called with null camera\n");
            return;
        }
        return DrawIntoRenderTarget(camera->RenderTarget().get(), &camera->Position(), &camera->ViewMatrix(), &camera->ProjectionMatrix(), modelMatrix, pipelineState, material, topology, numVertices, instanceCount);
    }

    virtual void DrawIndexedWithCamera(const Camera *camera, const struct Matrix4x3 *modelMatrix, const class RendererPipelineState *pipelineState, const class Material *material, PrimitiveTopology topology, ui32 numIndexes, ui32 instanceCount) override
    {
        if (camera == nullptr)
        {
            SENDLOG(Error, "DrawWithCamera called with null camera\n");
            return;
        }
        return DrawIndexedIntoRenderTarget(camera->RenderTarget().get(), &camera->Position(), &camera->ViewMatrix(), &camera->ProjectionMatrix(), modelMatrix, pipelineState, material, topology, numIndexes, instanceCount);
    }

    inline bool DrawGeneric(const class RenderTarget *rt, const Vector3 *cameraPos, const Matrix4x3 *viewMatrix, const Matrix4x4 *projMatrix, const Matrix4x3 *modelMatrix, const RendererPipelineState &pipelineState, const Material &material, PrimitiveTopology topology, ui32 numPoints) // there's currently no caching, no checking of the dirty state
    {
        if (rt == nullptr)
        {
            SENDLOG(Error, "Draw called with null RenderTarget\n");
            return false;
        }

        SetRenderTarget(*rt);

        for (const auto &attribute : pipelineState.VertexDataLayout().Attributes())
        {
            if (attribute.VertexArrayNumber() >= _boundVertexArrays.size())
            {
                SENDLOG(Error, "RendererPipelineState's vertex layout requests vertex array with number %u, but the current renderer allows only %u\n", (ui32)attribute.VertexArrayNumber(), (ui32)_boundVertexArrays.size());
                return false;
            }

            if (Funcs::IsBitSet(_boundVertexBuffersThatNotNullptr, attribute.VertexArrayNumber()) == false)
            {
                SENDLOG(Error, "RendererPipelineState's vertex layout requests vertex array with number %u, but it isn't currently bound\n", (ui32)attribute.VertexArrayNumber());
                return false;
            }
        }

        bool isTriangles = topology == PrimitiveTopology::TriangleEnumeration || topology == PrimitiveTopology::TriangleFan || topology == PrimitiveTopology::TriangleStrip;

        if (isTriangles && numPoints % 3)
        {
            SENDLOG(Error, "Draw function called with PrimitiveTopology::Triangle, but number of points isn't divisible by 3\n");
            return false;
        }

        if (isTriangles && pipelineState.PolygonType() != RendererPipelineState::PolygonTypet::Triangle)
        {
            SENDLOG(Error, "Draw function requested triangle rendering, but pipeline polygon type isn't set to Triangle\n");
            return false;
        }

        const auto &shader = material.Shader();
        if (CheckShaderBackendData(*shader))
        {
            RendererFrontendDataDirtyState(material, true);
        }
        if (RendererBackendData(*shader) == nullptr)
        {
            SENDLOG(Error, "Draw function failed to update material's shader, material %*s shader %*s\n", SVIEWARG(material.Name()), SVIEWARG(shader->Name()));
            return false;
        }

        ApplyPipelineState(pipelineState);

        CheckMaterialBackendData(material);
        if (RendererBackendData(material) == nullptr)
        {
            SENDLOG(Error, "Draw function failed to update material %*s, shader %*s\n", SVIEWARG(material.Name()), SVIEWARG(shader->Name()));
            return false;
        }

        const auto &materialBackendData = *RendererBackendData<MaterialBackendData>(material);
        const auto &shaderBackendData = *RendererBackendData<ShaderBackendData>(*shader);

        glUseProgram(shaderBackendData.program);

        assert(_intermediateVAO == 0);

        for (ui32 attributeIndex = 0; attributeIndex < material.Shader()->InputAttributes().size(); ++attributeIndex)
        {
            if (_intermediateVAO == 0)
            {
                glGenVertexArrays(1, &_intermediateVAO);
                glBindVertexArray(_intermediateVAO);
            }

            auto glAttribLocation = shaderBackendData.attributeLocations[attributeIndex];
            const auto &shaderInputAttribute = material.Shader()->InputAttributes()[attributeIndex];
            const auto &pipelineLayoutAttributes = pipelineState.VertexDataLayout().Attributes();

            const auto &pipelineAttribute = std::find_if(pipelineLayoutAttributes.begin(), pipelineLayoutAttributes.end(), [&shaderInputAttribute](const VertexLayout::Attribute &pipelineLayoutAttribute) {return pipelineLayoutAttribute.Name() == shaderInputAttribute; });
            if (pipelineAttribute == pipelineLayoutAttributes.end())
            {
                SENDLOG(Error, "Shader %*s has input attribute %*s, but RendererPipelineState's vertex layout doesn't provide it\n", SVIEWARG(material.Shader()->Name()), SVIEWARG(shaderInputAttribute));
                return false;
            }

            const auto &vertexBuffer = _boundVertexArrays[pipelineAttribute->VertexArrayNumber()];
            assert(vertexBuffer->Type() == RendererArray::Typet::VertexArray);

            if (vertexBuffer->IsLocked())
            {
                SENDLOG(Error, "Draw called, but vertex buffer %*s it uses is still locked\n", SVIEWARG(vertexBuffer->Name()));
                return false;
            }

            auto *vertexBufferBackendData = RendererBackendData<ArrayBackendData>(*vertexBuffer);
            if (vertexBufferBackendData == nullptr || vertexBufferBackendData->oglBuffer == 0)
            {
                SENDLOG(Error, "Invalid vertex buffer\n");
                return false;
            }

            //glBindVertexArray(pipelineAttribute->VertexBufferNumber(), vertexBufferBackendData->oglBuffer, 0, vertexBuffer->Stride());

            auto sizeAndType = VertexAttributeTypeToOGL(pipelineAttribute->Format());
            //glVertexAttribFormat(glAttribLocation, sizeAndType.first, sizeAndType.second, GL_FALSE, *pipelineAttribute->VertexBufferOffset());

            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferBackendData->oglBuffer);
            glEnableVertexAttribArray(glAttribLocation);
			uiw vertexArrayOffset = *pipelineAttribute->VertexArrayOffset();
            glVertexAttribPointer(glAttribLocation, sizeAndType.first, sizeAndType.second, GL_FALSE, vertexBuffer->Stride(), reinterpret_cast<void *>(vertexArrayOffset));

            //glVertexAttribBinding()

            glVertexBindingDivisor(glAttribLocation, pipelineAttribute->InstanceStep().value_or(0));
        }

        GLenum curTexUnit = 0;
        const ui8 *uniformsMemory = materialBackendData.uniforms.get();

        for (ui32 uniformIndex = 0; uniformIndex < shader->Uniforms().size(); ++uniformIndex)
        {
            const auto &shaderUniform = shader->Uniforms()[uniformIndex];
            const auto &oglUniform = shaderBackendData.oglUniforms[uniformIndex];

            switch (shaderUniform.type)
            {
            case Shader::Uniform::Type::Bool:
            case Shader::Uniform::Type::F32:
            case Shader::Uniform::Type::I32:
            case Shader::Uniform::Type::UI32:
                if (shaderUniform.elementHeight > 1)
                {
                    auto func = (ShaderBackendData::SetMatrixUniformFunction)oglUniform.setFuncAddress;
                    func(oglUniform.location, shaderUniform.elementsCount, GL_FALSE, (GLfloat *)uniformsMemory);
                }
                else
                {
                    auto func = (ShaderBackendData::SetUniformFunction)oglUniform.setFuncAddress;
                    func(oglUniform.location, shaderUniform.elementsCount, uniformsMemory);
                }
                uniformsMemory += 4 * shaderUniform.elementWidth * shaderUniform.elementHeight * shaderUniform.elementsCount;
                break;
            case Shader::Uniform::Type::Texture:
            {
                MaterialBackendData::TextureUniform textureUniform;
                MemOps::Copy(&textureUniform, (MaterialBackendData::TextureUniform *)uniformsMemory, 1);
                if (textureUniform.textureBackendData == nullptr || textureUniform.textureSamplerBackendData == nullptr)
                {
                    if (textureUniform.textureBackendData == nullptr)
                    {
                        SENDLOG(Error, "Draw called with an incomplete texture\n");
                    }
                    else if (textureUniform.textureSamplerBackendData == nullptr)
                    {
                        SENDLOG(Error, "Draw called with a texture without a sampler\n");
                    }
                    return false;
                }

                glActiveTexture(GL_TEXTURE0 + curTexUnit);

                auto texData = textureUniform.textureBackendData;
                auto texSamplerData = textureUniform.textureSamplerBackendData;

                glBindTexture(texData->oglTextureDimension, Texture::IsFormatDepthStencil(texData->format) ? texData->oglRenderBuffer : texData->oglTexture);
                glUniform1i(oglUniform.location, curTexUnit);

                glBindSampler(curTexUnit, texSamplerData->oglSampler);

                ++curTexUnit;
                uniformsMemory += sizeof(MaterialBackendData::TextureUniform);
            } break;
            }
        }

		auto setSystemUniform = [&shader, &systemOglUniforms = shaderBackendData.systemOglUniforms](string_view uniformName, auto uniformValue, auto uniformSetFunction)
		{
			auto uniformSearch = std::find_if(shader->SystemUniforms().begin(), shader->SystemUniforms().end(), [uniformName](const Shader::Uniform &uniform) { return uniform.name == uniformName; });
			if (uniformSearch != shader->SystemUniforms().end())
			{
				auto index = uniformSearch - shader->SystemUniforms().begin();
				const auto &oglUniform = systemOglUniforms[index];

				std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<std::remove_cv_t<decltype(uniformValue)>>>> uniform;
				if (uniformValue)
				{
					uniform = *uniformValue;
				}

				uniformSetFunction(oglUniform, uniform);
			}
		};

		auto setMatrix = [](const ShaderBackendData::OGLUniform &oglUniform, const auto &matrix)
		{
			reinterpret_cast<ShaderBackendData::SetMatrixUniformFunction>(oglUniform.setFuncAddress)(oglUniform.location, 1, GL_FALSE, matrix.Data().data());
		};

		auto setFloats = [](const ShaderBackendData::OGLUniform &oglUniform, const auto &values)
		{
			reinterpret_cast<ShaderBackendData::SetUniformFunction>(oglUniform.setFuncAddress)(oglUniform.location, 1, values.Data().data());
		};

		Matrix4x4 viewProjMatrix;
		if (viewMatrix && projMatrix)
		{
			viewProjMatrix = *viewMatrix * *projMatrix;
		}

		Vector3 cameraForwardVector, cameraRightVector, cameraUpVector;
		if (viewMatrix)
		{
			cameraForwardVector = viewMatrix->GetColumn(2).ToVector3();
			cameraRightVector = viewMatrix->GetColumn(0).ToVector3();
			cameraUpVector = viewMatrix->GetColumn(1).ToVector3();
		}

		setSystemUniform("_ModelMatrix", modelMatrix, setMatrix);
		setSystemUniform("_ViewMatrix", viewMatrix, setMatrix);
		setSystemUniform("_ProjectionMatrix", projMatrix, setMatrix);
		setSystemUniform("_ViewProjectionMatrix", viewMatrix &&projMatrix ? &viewProjMatrix : nullptr, setMatrix);
		setSystemUniform("_CameraPosition", cameraPos, setFloats);
		setSystemUniform("_CameraForwardVector", viewMatrix ? &cameraForwardVector : nullptr, setFloats);
		setSystemUniform("_CameraRightVector", viewMatrix ? &cameraRightVector : nullptr, setFloats);
		setSystemUniform("_CameraUpVector", viewMatrix ? &cameraUpVector : nullptr, setFloats);

        return true;
    }

    void ApplyPipelineState(const RendererPipelineState &state)
    {
        switch (state.PolygonFillMode())
        {
        case RendererPipelineState::PolygonFillModet::Solid:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        case RendererPipelineState::PolygonFillModet::Wireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        }

        switch (state.PolygonCullMode())
        {
        case RendererPipelineState::PolygonCullModet::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case RendererPipelineState::PolygonCullModet::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case RendererPipelineState::PolygonCullModet::None:
            glDisable(GL_CULL_FACE);
            break;
        }

        glEnable(GL_DEPTH_TEST);
        switch (state.DepthComparisonFunc())
        {
        case RendererPipelineState::DepthComparisonFunct::Always:
            glDepthFunc(GL_ALWAYS);
            break;
        case RendererPipelineState::DepthComparisonFunct::Equal:
            glDepthFunc(GL_EQUAL);
            break;
        case RendererPipelineState::DepthComparisonFunct::Greater:
            glDepthFunc(GL_GREATER);
            break;
        case RendererPipelineState::DepthComparisonFunct::GreaterEqual:
            glDepthFunc(GL_GEQUAL);
            break;
        case RendererPipelineState::DepthComparisonFunct::Less:
            glDepthFunc(GL_LESS);
            break;
        case RendererPipelineState::DepthComparisonFunct::LessEqual:
            glDepthFunc(GL_LEQUAL);
            break;
        case RendererPipelineState::DepthComparisonFunct::Never:
            glDepthFunc(GL_NEVER);
            break;
        case RendererPipelineState::DepthComparisonFunct::NotEqual:
            glDepthFunc(GL_NOTEQUAL);
            break;
        }

        glDepthMask(state.EnableDepthWrite() ? GL_TRUE : GL_FALSE);

        glFrontFace(state.PolygonFrontIsCounterClockwise() ? GL_CCW : GL_CW);

        auto blend = state.BlendSettings();

        auto blendFactorToOGL = [](RendererPipelineState::BlendFactort factor) -> GLenum
        {
            switch (factor)
            {
            case RendererPipelineState::BlendFactort::One:
                return GL_ONE;
            case RendererPipelineState::BlendFactort::Zero:
                return GL_ZERO;
            case RendererPipelineState::BlendFactort::SourceColor:
                return GL_SRC_COLOR;
            case RendererPipelineState::BlendFactort::InvertSourceColor:
                return GL_ONE_MINUS_SRC_COLOR;
            case RendererPipelineState::BlendFactort::SourceAlpha:
                return GL_SRC_ALPHA;
            case RendererPipelineState::BlendFactort::InvertSourceAlpha:
                return GL_ONE_MINUS_SRC_ALPHA;
            case RendererPipelineState::BlendFactort::TargetColor:
                return GL_DST_COLOR;
            case RendererPipelineState::BlendFactort::InvertTargetColor:
                return GL_ONE_MINUS_DST_COLOR;
            case RendererPipelineState::BlendFactort::TargetAlpha:
                return GL_DST_ALPHA;
            case RendererPipelineState::BlendFactort::InvertTargetAlpha:
                return GL_ONE_MINUS_DST_ALPHA;
            }

            UNREACHABLE;
            return GL_INVALID_ENUM;
        };

        auto blendCombineModeToOGL = [](RendererPipelineState::BlendCombineModet mode) -> GLenum
        {
            switch (mode)
            {
            case RendererPipelineState::BlendCombineModet::Add:
                return GL_FUNC_ADD;
            case RendererPipelineState::BlendCombineModet::Subtract:
                return GL_FUNC_SUBTRACT;
            case RendererPipelineState::BlendCombineModet::ReverseSubtract:
                return GL_FUNC_REVERSE_SUBTRACT;
            case RendererPipelineState::BlendCombineModet::Max:
                return GL_MAX;
            case RendererPipelineState::BlendCombineModet::Min:
                return GL_MIN;
            }

            UNREACHABLE;
            return GL_INVALID_ENUM;
        };

        for (ui32 rtIndex = 0; rtIndex < RenderTargetsLimit; ++rtIndex)
        {
            blend.isEnabled ? glEnablei(GL_BLEND, rtIndex) : glDisablei(GL_BLEND, rtIndex);
            glBlendFuncSeparatei(rtIndex, blendFactorToOGL(blend.sourceColorFactor), blendFactorToOGL(blend.targetColorFactor), blendFactorToOGL(blend.sourceAlphaFactor), blendFactorToOGL(blend.targetAlphaFactor));
            glBlendEquationSeparatei(rtIndex, blendCombineModeToOGL(blend.colorCombineMode), blendCombineModeToOGL(blend.alphaCombineMode));
        }
    }

    virtual void BeginFrame() override
    {
        _context->MakeCurrent();
    }

    virtual void EndFrame() override
    {}

    virtual void SwapBuffers() override
    {
        _context->SwapBuffers();
    }

    virtual void *RendererContext() override
    {
        return _context->ContextPointer();
    }

    void *AddBackendData(void **backendDataPointer, RendererBackendDataBase::BackendDataType type)
    {
        using DataType = RendererBackendDataBase::BackendDataType;

        RendererBackendDataBase *data = nullptr;

        switch (type)
        {
        case DataType::Array:
            data = new ArrayBackendData;
            break;
        case DataType::Material:
            data = new MaterialBackendData;
            break;
        case DataType::PipelineState:
            data = new PipelineStateBackendData;
            break;
        case DataType::RenderTarget:
            data = new RenderTargetBackendData;
            break;
        case DataType::Shader:
            data = new ShaderBackendData;
            break;
        case DataType::Texture:
            data = new TextureBackendData;
            break;
        case DataType::TextureSampler:
            data = new TextureSamplerBackendData;
            break;
        }

        data->backendDataPointer = backendDataPointer;
        *backendDataPointer = data;
        auto it = _allocatedBackendDatas.insert(data);
        if (it.second == false)
        {
            HARDBREAK;
        }

        return data;
    }

    virtual void *AllocateBackendData(const RendererFrontendData &frontendData, RendererBackendDataBase::BackendDataType type) override
    {
        return AddBackendData(RendererBackendDataPointer(frontendData), type);
    }

    virtual void DeleteBackendData(const RendererFrontendData &frontendData) override
    {
        auto castedData = RendererBackendData<RendererBackendDataBase>(frontendData);
        if (castedData != nullptr)
        {
            *castedData->backendDataPointer = nullptr;
            delete castedData;
            size_t removed = _allocatedBackendDatas.erase(castedData);
            assert(removed == 1);
        }
    }
};

shared_ptr<OpenGLRenderer> OpenGLRenderer::New(bool isFullscreen, EngineCore::TextureDataFormat colorFormat, ui32 depth, ui32 stencil, OpenGLRenderer::Window window)
{
#ifdef WINPLATFORM
    auto context = OpenGLContextWindows::New(isFullscreen, colorFormat, depth, stencil, window);
#endif

    return make_shared<OpenGLRendererImpl>(move(context));
}

bool OpenGLRendererProxy::HasGLErrors()
{
    auto codeToText = [](GLint errorCode)
    {
        switch (errorCode)
        {
        case GL_INVALID_VALUE:
            return "invalid_value";
        case GL_INVALID_ENUM:
            return "invalid_enum";
        case GL_INVALID_OPERATION:
            return "invalid_operation";
        case GL_OUT_OF_MEMORY:
            return "out_of_memory";
        default:
            return "unknown_error";
        }
    };

    bool spottedErrors = false;

    for (GLint error = glGetError(); error != GL_NO_ERROR; error = glGetError())
    {
        spottedErrors = true;

        SENDLOG(Error, "OpenGL error has happened: %s\n", codeToText(error));
    }

    return spottedErrors;
}

void APIENTRY OGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    if (id == 131185) // Buffer detailed info: Buffer object # (bound to #, usage hint is #) will use VIDEO memory as the source for buffer object operations.
    {
        return;
    }
    if (id == 131154) // Pixel-path performance warning: Pixel transfer is synchronized with 3D rendering. (caused by FRAPS)
    {
        return;
    }
    SENDLOG(Error, "OGL debug callback called with message %s\n", message);
}