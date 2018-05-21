#include "PreHeader.hpp"
#include "SceneBackground.hpp"
#include <Logger.hpp>
#include <Application.hpp>
#include <Camera.hpp>
#include <Logger.hpp>
#include <RendererPipelineState.hpp>
#include <Material.hpp>
#include <Renderer.hpp>
#include <Texture.hpp>
#include <TextureSampler.hpp>
#include <MathFunctions.hpp>

using namespace EngineCore;
using namespace TradingApp;

struct PlaneResources
{
    shared_ptr<RendererPipelineState> pipeline;
    shared_ptr<Material> material;
};

namespace
{
    PlaneResources BackgroundPlane;
}

static shared_ptr<Texture> CreateCellTexture(bool isUseThinStrips);
static void FillCellTextureLevel(ui32 levelSize, ui8 *memory, bool isUseThinStrips);

bool SceneBackground::Create(bool isDepthWriteEnabled)
{
    auto cellTextureThickThin = CreateCellTexture(true);
    auto cellTextureThick = CreateCellTexture(false);

    auto shader = Application::LoadResource<Shader>("BackgroundTextured");
    if (shader == nullptr)
    {
        SENDLOG(Error, "SceneBackground failed to locate shader for background plane\n");
        return false;
    }

    auto material = Material::New(shader);

    if (false == material->UniformTexture("BackTextureThickThin", cellTextureThickThin))
    {
        SENDLOG(Error, "SceneBackground failed to set BackTextureThickThin texture\n");
        return false;
    }
    if (false == material->UniformTexture("BackTextureThick", cellTextureThick))
    {
        SENDLOG(Error, "SceneBackground failed to set BackTextureThick texture\n");
        return false;
    }
    if (false == material->UniformF32("PlaneSize", {1.0e+3f, 1.0e+3f}))
    {
        SENDLOG(Error, "SceneBackground failed to set uniform PlaneSize\n");
        return false;
    }
    if (false == material->UniformF32("ThinVisibility100Distance", 10.0f))
    {
        SENDLOG(Error, "SceneBackground failed to set uniform ThinVisibility100Distance\n");
        return false;
    }
    if (false == material->UniformF32("ThinVisibility0Distance", 15.0f))
    {
        SENDLOG(Error, "SceneBackground failed to set uniform ThinVisibility0Distance\n");
        return false;
    }

    auto pipelineState = RendererPipelineState::New(shader);
    pipelineState->EnableDepthWrite(isDepthWriteEnabled);
    pipelineState->PolygonCullMode(RendererPipelineState::PolygonCullModet::None);
    pipelineState->DepthComparisonFunc(RendererPipelineState::DepthComparisonFunct::Always);
    
    BackgroundPlane.material = move(material);
    BackgroundPlane.pipeline = move(pipelineState);

	SENDLOG(Info, "SceneBackground's created\n");
	return true;
}

void SceneBackground::Destroy()
{
    BackgroundPlane = PlaneResources();

    SENDLOG(Info, "SceneBackground's been destroyed\n");
}

void SceneBackground::Update()
{
}

void SceneBackground::Draw(const Vector3 &rotation, const Camera &camera)
{
    if (BackgroundPlane.material == nullptr || BackgroundPlane.pipeline == nullptr)
    {
        return;
    }

    auto modelMatrix = Matrix4x3::CreateRTS(rotation, nullopt, nullopt);

    Application::GetRenderer().DrawWithCamera(&camera, &modelMatrix, BackgroundPlane.pipeline.get(), BackgroundPlane.material.get(), PrimitiveTopology::TriangleEnumeration, 6);
}

shared_ptr<Texture> CreateCellTexture(bool isUseThinStrips)
{
    auto funcStart = TimeMoment::Now();

    ui32 textureSize = 1024;
    ui32 textureMipLevelsCount = 1;//Texture::FullChainMipLevelsCount(textureSize, textureSize);
    ui32 textureSizeInBytes = Texture::TextureSizeInBytes(textureSize, textureSize, 1, textureMipLevelsCount, TextureDataFormat::R8G8B8);

    BufferOwnedData textureData
    {
        (ui8 *)new ui8[textureSizeInBytes],
        [](void *p) {delete[] p; }
    };

    ui32 levelSize = textureSize;
    ui8 *memory = textureData.get();
    for (ui32 level = 0; level < textureMipLevelsCount; ++level)
    {
        FillCellTextureLevel(levelSize, memory, isUseThinStrips);
        memory += Texture::MipLevelSizeInBytes(levelSize, levelSize, 1, TextureDataFormat::R8G8B8);
        levelSize /= 2;
    }

    assert(memory == textureData.get() + textureSizeInBytes);

    auto sampler = TextureSampler::New();
    sampler->MagFilterMode(TextureSampler::FilterMode::Anisotropic);
    sampler->MinFilterMode(TextureSampler::FilterMode::Anisotropic);
    sampler->MaxAnisotropy(16);
    sampler->MinLinearInterpolateMips(true);
    sampler->XSampleMode(TextureSampler::SampleMode::Tile);
    sampler->YSampleMode(TextureSampler::SampleMode::Tile);

    auto texture = Texture::New(move(textureData), TextureDataFormat::R8G8B8, textureSize, textureSize, textureMipLevelsCount, TextureDataFormat::R8G8B8);
    if (texture == nullptr)
    {
        SENDLOG(Error, "SceneBackground failed to create texture\n");
        return false;
    }
    texture->Sampler(sampler);

    texture->IsRequestFullMipChain(true);

    TimeDifference delta = TimeMoment::Now() - funcStart;

    SENDLOG(Info, "Texture generation with isUseThinStrips %s took %fs\n", isUseThinStrips ? "true" : "false", delta.ToSeconds());

    return texture;
}

void FillCellTextureLevel(ui32 levelSize, ui8 *memory, bool isUseThinStrips)
{
    f32 textureSizeRev = 1.0f / levelSize;

    auto setColor = [memory, levelSize](ui32 x, ui32 y, array<f32, 3> color)
    {
        auto *target = memory + (y * levelSize + x) * 3;
        target[0] = (ui8)std::min<f32>(std::round(color[0] * 255.0f), 255.0f);
        target[1] = (ui8)std::min<f32>(std::round(color[1] * 255.0f), 255.0f);
        target[2] = (ui8)std::min<f32>(std::round(color[2] * 255.0f), 255.0f);
    };

    array<f32, 3> backgroundColor{0.2f, 0, 0.4f};
    array<f32, 3> thickStripColor{0.6f, 0.4f, 0.8f};
    array<f32, 3> thinStripColor{0.3f, 0.2f, 0.5f};

    constexpr f32 thickStripSize = 0.001f;
    constexpr f32 thickStripFalloff = 0.0075f;
    constexpr f32 thinStripSize = 0.0005f * 10;
    constexpr f32 thinStripFalloff = 0.005f * 10;

    auto getColor = [](f32 xPos, f32 yPos, array<f32, 3> backColor, array<f32, 3> foreColor, f32 stripSize, f32 stripFalloff)->array<f32, 3>
    {
        auto lerp = [](f32 left, f32 right, f32 factor) -> f32
        {
            assert(factor >= 0 && factor <= 1);
            return left + (right - left) * factor;
        };

        f32 dist = std::max<f32>(Distance(xPos, 0.5f), Distance(yPos, 0.5f));

        f32 stripHalfSize = stripSize * 0.5f;

        dist -= 0.5f - (stripHalfSize + stripFalloff);

        if (dist < 0)
        {
            return backColor;
        }
        else
        {
            dist += stripHalfSize;
            dist /= stripFalloff;
            dist = std::max<f32>(0, std::min<f32>(dist, 1));
            dist = pow(dist, 4);

            return {lerp(backColor[0], foreColor[0], dist), lerp(backColor[1], foreColor[1], dist), lerp(backColor[2], foreColor[2], dist)};
        }
    };

    for (ui32 y = 0; y < levelSize; ++y)
    {
        for (ui32 x = 0; x < levelSize; ++x)
        {
            if (levelSize <= 16)
            {
                setColor(x, y, backgroundColor);
                continue;
            }
            else if (levelSize <= 64)
            {
                isUseThinStrips = false;
            }

            f32 xPos = x * textureSizeRev;
            f32 yPos = y * textureSizeRev;

            auto backColor = backgroundColor;
            if (isUseThinStrips)
            {
                backColor = getColor(fmod(x * 10 * textureSizeRev, 1), fmod(y * 10 * textureSizeRev, 1), backgroundColor, thinStripColor, thinStripSize, thinStripFalloff);
            }

            auto finalColor = getColor(x * textureSizeRev, y * textureSizeRev, backColor, thickStripColor, thickStripSize, thickStripFalloff);

            setColor(x, y, finalColor);
        }
    }
}