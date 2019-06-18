#include "BasicHeader.hpp"
#include "ShadersManager.hpp"
#include "Shader.hpp"
#include <unordered_set>

namespace
{
    struct Hasher
    {
        size_t operator () (const shared_ptr<EngineCore::Shader> &key) const
        {
            return std::hash<string_view>()(key->Name());
        }
    };

    struct Comparer
    {
        bool operator () (const shared_ptr<EngineCore::Shader> &left, const shared_ptr<EngineCore::Shader> &right) const
        {
            return left->Name() < right->Name();
        }
    };

    std::unordered_set<shared_ptr<EngineCore::Shader>, Hasher, Comparer> LoadedShaders;
}

#define SHADER_VERSION "#version 400 \n"

auto EngineCore::ShadersManager::FindShaderByName(string_view name) -> shared_ptr<Shader>
{
    // TODO: fuck, that'd be slow
    auto searchResult = std::find_if(LoadedShaders.begin(), LoadedShaders.end(),
        [name](const shared_ptr<EngineCore::Shader> &value)
    {
        return value->Name() == name;
    });

    if (searchResult != LoadedShaders.end())
    {
        return *searchResult;
    }

    if (name == "Color") // TODO: do something about it
    {
        string_view vsCode = SHADER_VERSION TOSTR(
            in int gl_VertexID;

            void main()
            {
                vec2 texCoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
                gl_Position = vec4(texCoord * vec2(2, -2) + vec2(-1, 1), 1, 1);
            }
        );

        string_view psCode = SHADER_VERSION TOSTR(
            layout(location = 0) out vec4 OutputColor;

            uniform vec4 Color;

            void main()
            {
                OutputColor = Color;
            }
        );

        array<Shader::Uniform, 1> uniforms = {
            {"Color", 4, 1, 1, Shader::Uniform::Type::F32}};

        auto shader = Shader::New(name, vsCode, psCode, uniforms.data(), (ui32)uniforms.size());

        LoadedShaders.emplace(shader);

        return shader;
    }
    else if (name == "ColoredVertices")
    {
        string_view vsCode = SHADER_VERSION TOSTR(
            in int gl_VertexID;
            in vec4 position;
            in vec4 color;

            out vec4 procColor;

            void main()
            {
                gl_Position = position;
                procColor = color;
            }
        );

        string_view psCode = SHADER_VERSION TOSTR(
            in vec4 procColor;

            layout(location = 0) out vec4 OutputColor;

            uniform vec4 ColorMul;

            void main()
            {
                OutputColor = procColor * ColorMul;
            }
        );

        array<Shader::Uniform, 1> uniforms{
            {"ColorMul", 4, 1, 1, Shader::Uniform::Type::F32}};

        array<string_view, 2> inputAttributes{"position", "color"};

        auto shader = Shader::New(name, vsCode, psCode, uniforms.data(), (ui32)uniforms.size(), inputAttributes.data(), (ui32)inputAttributes.size());

        LoadedShaders.emplace(shader);

        return shader;
    }
    else if (name == "Colored3DVertices")
    {
        string_view vsCode = SHADER_VERSION TOSTR(
            in vec4 position;
            in vec4 color;

            out vec4 procColor;

            uniform mat4x3 _ModelMatrix;
            uniform mat4x4 _ViewProjectionMatrix;

            void main()
            {
                vec3 worldPos = _ModelMatrix * position;
                vec4 screenPos = _ViewProjectionMatrix * vec4(worldPos, 1.0);

                gl_Position = screenPos;

                procColor = color;
            }
        );

        string_view psCode = SHADER_VERSION TOSTR(
            in vec4 procColor;

            layout(location = 0) out vec4 OutputColor;

            uniform vec4 ColorMul;

            void main()
            {
                OutputColor = procColor * ColorMul;
            }
        );

        array<Shader::Uniform, 1> uniforms{
            Shader::Uniform{"ColorMul", 4, 1, 1, Shader::Uniform::Type::F32}};

        array<Shader::Uniform, 2> systemUniforms{
            Shader::Uniform{"_ModelMatrix", 3, 4, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"_ViewProjectionMatrix", 4, 4, 1, Shader::Uniform::Type::F32}};

        array<string_view, 2> inputAttributes{"position", "color"};

        auto shader = Shader::New(name, vsCode, psCode, uniforms.data(), (ui32)uniforms.size(), inputAttributes.data(), (ui32)inputAttributes.size(), systemUniforms.data(), (ui32)systemUniforms.size());

        LoadedShaders.emplace(shader);

        return shader;
    }
    else if (name == "Colored3DVerticesInstanced")
    {
        string_view vsCode = SHADER_VERSION TOSTR(
            in vec4 position;
            in vec4 color;
            in vec4 rotation;
            in vec4 wpos_scale;

            out vec4 procColor;

            uniform mat4x4 _ViewProjectionMatrix;

            void main()
            {
                vec4 outColor = color;

                float scale = wpos_scale.w;
                uint scaleInt = floatBitsToUint(scale);
                if ((scaleInt & 0x80000000) != 0)
                {
                    float gray = color.r * 0.299 + color.g * 0.587 + color.b * 0.114;
                    outColor = vec4(gray, gray, gray, color.a);
                }
                if ((scaleInt & 0x1) != 0)
                {
                    outColor.rgb -= vec3(0.1, 0.3, 0.5);
                }

                scale = abs(scale);

                vec3 u = vec3(rotation.x, rotation.y, rotation.z);
                vec3 v = position.xyz;
                float w = rotation.w;
                vec3 uv = cross(u, v);
                vec3 uuv = cross(u, uv);
                vec3 rotatedPos = v + ((uv * w) + uuv) * 2.0;

                vec3 scaledPos = rotatedPos + scale;

                vec3 worldPos = scaledPos + wpos_scale.xyz;

                vec4 screenPos = _ViewProjectionMatrix * vec4(worldPos, 1.0);

                gl_Position = screenPos;
                procColor = outColor;
            }
        );

        string_view psCode = SHADER_VERSION TOSTR(
            in vec4 procColor;

            layout(location = 0) out vec4 OutputColor;

            void main()
            {
                OutputColor = procColor;
            }
        );
        
        array<Shader::Uniform, 1> systemUniforms{
            Shader::Uniform{"_ViewProjectionMatrix", 4, 4, 1, Shader::Uniform::Type::F32}};

        array<string_view, 4> inputAttributes{"position", "color", "rotation", "wpos_scale"};

        auto shader = Shader::New(name, vsCode, psCode, nullptr, 0, inputAttributes.data(), (ui32)inputAttributes.size(), systemUniforms.data(), (ui32)systemUniforms.size());

        LoadedShaders.emplace(shader);

        return shader;
    }
    else if (name == "Background")
    {
        string_view vsCode = SHADER_VERSION TOSTR(
            in int gl_VertexID;

            out vec3 vertexCoord;

            uniform vec4 PlaneSize;
            uniform vec4 StripSizes;

            uniform mat4x3 _ModelMatrix;
            uniform mat4x4 _ViewProjectionMatrix;

            void main()
            {
                vec4 position;
                if (gl_VertexID == 0 || gl_VertexID == 5) /* bottom left */
                {
                    position = vec4(-PlaneSize.x, -PlaneSize.y, 0, 1);
                }
                else if (gl_VertexID == 1) /* top left */
                {
                    position = vec4(-PlaneSize.x, PlaneSize.y, 0, 1);
                }
                else if (gl_VertexID == 2 || gl_VertexID == 4) /* top right */
                {
                    position = vec4(PlaneSize.x, PlaneSize.y, 0, 1);
                }
                else /* if(gl_VertexID == 3) */ /* bottom right */
                {
                    position = vec4(PlaneSize.x, -PlaneSize.y, 0, 1);
                }

                vec3 worldPos = _ModelMatrix * position;

                vertexCoord = worldPos;

                vec4 screenPos = _ViewProjectionMatrix * vec4(worldPos, 1.0);

                gl_Position = screenPos;
            }
        );

        string_view psCode = SHADER_VERSION TOSTR(
            in vec3 vertexCoord;

            layout(location = 0) out vec4 OutputColor;

            uniform vec4 PlaneSize;
            uniform vec4 StripSizes;
            uniform vec4 BackColor;
            uniform vec4 ThickStripColor;
            uniform vec4 ThinStripColor;

			vec4 LineColor(vec4 backColor, float freq, vec4 color, float thickness, float falloff)
			{
				vec2 cell = mod(vertexCoord.xy, freq.xx);
				float halfStripFreq = freq * 0.5;
				vec2 distToCenter = vec2(distance(cell.x, halfStripFreq), distance(cell.y, halfStripFreq));
				float maxDistance = max(distToCenter.x, distToCenter.y);
				maxDistance -= halfStripFreq - (thickness + falloff);
				if (maxDistance <= 0)
				{
					return backColor;
				}
                else
				{
                    maxDistance += thickness;
					maxDistance /= falloff;
                    maxDistance = clamp(maxDistance, 0, 1);
                    maxDistance = pow(maxDistance, 4);
					return mix(backColor, color, maxDistance);
				}
			}

            void main()
            {
				vec4 thin = LineColor(BackColor, PlaneSize.w, ThinStripColor, StripSizes.y, StripSizes.w);
				OutputColor = LineColor(thin, PlaneSize.z, ThickStripColor, StripSizes.x, StripSizes.z);
            }
        );

        array<Shader::Uniform, 5> uniforms{
            Shader::Uniform{"PlaneSize", 4, 1, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"StripSizes", 4, 1, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"BackColor", 4, 1, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"ThickStripColor", 4, 1, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"ThinStripColor", 4, 1, 1, Shader::Uniform::Type::F32}};

        array<Shader::Uniform, 2> systemUniforms{
            Shader::Uniform{"_ModelMatrix", 3, 4, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"_ViewProjectionMatrix", 4, 4, 1, Shader::Uniform::Type::F32}};

        auto shader = Shader::New(name, vsCode, psCode, uniforms.data(), (ui32)uniforms.size(), nullptr, 0, systemUniforms.data(), (ui32)systemUniforms.size());

        LoadedShaders.emplace(shader);

        return shader;
    }
    else if (name == "BackgroundTextured")
    {
        string_view vsCode = SHADER_VERSION TOSTR(
            in int gl_VertexID;

            out vec2 TexCoord;
            out vec3 VertexWorldPos;

            uniform vec2 PlaneSize;

            uniform mat4x3 _ModelMatrix;
            uniform mat4x4 _ViewProjectionMatrix;

            void main()
            {
                vec4 position;
                if (gl_VertexID == 0 || gl_VertexID == 5) /* bottom left */
                {
                    position = vec4(-PlaneSize.x, -PlaneSize.y, 0, 1);
                    TexCoord = vec2(0, 0);
                }
                else if (gl_VertexID == 1) /* top left */
                {
                    position = vec4(-PlaneSize.x, PlaneSize.y, 0, 1);
                    TexCoord = vec2(0, 1) * PlaneSize;
                }
                else if (gl_VertexID == 2 || gl_VertexID == 4) /* top right */
                {
                    position = vec4(PlaneSize.x, PlaneSize.y, 0, 1);
                    TexCoord = vec2(1, 1) * PlaneSize;
                }
                else /* if(gl_VertexID == 3) */ /* bottom right */
                {
                    position = vec4(PlaneSize.x, -PlaneSize.y, 0, 1);
                    TexCoord = vec2(1, 0) * PlaneSize;
                }

                vec3 worldPos = _ModelMatrix * position;

                VertexWorldPos = worldPos;

                vec4 screenPos = _ViewProjectionMatrix * vec4(worldPos, 1.0);

                gl_Position = screenPos;
            }
        );

        string_view psCode = SHADER_VERSION TOSTR(
            layout(location = 0) out vec4 OutputColor;

            uniform vec3 _CameraPosition;
            uniform sampler2D BackTextureThickThin;
            uniform sampler2D BackTextureThick;
            uniform float ThinVisibility100Distance;
            uniform float ThinVisibility0Distance;

            in vec2 TexCoord;
            in vec3 VertexWorldPos;

            void main()
            {
                float dist = distance(VertexWorldPos, _CameraPosition);

                vec4 finalColor;

                if (dist < ThinVisibility100Distance)
                {
                    finalColor = texture2D(BackTextureThickThin, TexCoord);
                }
                else if (dist < ThinVisibility0Distance)
                {
                    float delta = ThinVisibility0Distance - ThinVisibility100Distance;
                    dist -= ThinVisibility100Distance;
                    dist /= delta;
                    finalColor = mix(texture2D(BackTextureThickThin, TexCoord), texture2D(BackTextureThick, TexCoord), dist);
                }
                else
                {
                    finalColor = texture2D(BackTextureThick, TexCoord);
                }

                OutputColor = finalColor;
            }
        );

        array<Shader::Uniform, 5> uniforms{
            Shader::Uniform{"BackTextureThickThin", 1, 1, 1, Shader::Uniform::Type::Texture},
            Shader::Uniform{"BackTextureThick", 1, 1, 1, Shader::Uniform::Type::Texture},
            Shader::Uniform{"PlaneSize", 2, 1, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"ThinVisibility100Distance", 1, 1, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"ThinVisibility0Distance", 1, 1, 1, Shader::Uniform::Type::F32}};

        array<Shader::Uniform, 3> systemUniforms{
            Shader::Uniform{"_ModelMatrix", 3, 4, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"_CameraPosition", 3, 1, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"_ViewProjectionMatrix", 4, 4, 1, Shader::Uniform::Type::F32}};

        auto shader = Shader::New(name, vsCode, psCode, uniforms.data(), (ui32)uniforms.size(), nullptr, 0, systemUniforms.data(), (ui32)systemUniforms.size());

        LoadedShaders.emplace(shader);

        return shader;
    }
    else if (name == "Line3D")
    {
        string_view vsCode = SHADER_VERSION TOSTR(
            in int gl_VertexID;

            in vec4 position;
            in vec2 texcoord;
            in vec4 scale;

            out vec2 procTexcoord;

            uniform mat4x3 _ModelMatrix;
            uniform mat4x4 _ViewProjectionMatrix;

            void main()
            {
                vec4 scaledPosition = position * scale;
                vec3 worldPos = _ModelMatrix * scaledPosition;
                vec4 screenPos = _ViewProjectionMatrix * vec4(worldPos, 1.0);

                gl_Position = screenPos;

                procTexcoord = texcoord;
            }
        );

        string_view psCode = SHADER_VERSION TOSTR(
            in vec2 procTexcoord;

            layout(location = 0) out vec4 OutputColor;

            void main() // TODO: bad code ahead
            {
                float rateOfChange = fwidth(procTexcoord.y);

                rateOfChange = rateOfChange * 5;
                rateOfChange = pow(rateOfChange, 1.5);
                rateOfChange = max(1, rateOfChange);

                float alphaMul = 1 / rateOfChange;

                vec4 fetchedColor;

                if (procTexcoord.y < 0.4 || procTexcoord.y > 0.6)
                {
                    float alpha = procTexcoord.y < 0.4 ? 
                        procTexcoord.y : 
                        1 - procTexcoord.y;
                    alpha = max(alpha, 0);
                    alpha *= 2.5;
                    float power = max(1, 4 * alphaMul);
                    alpha = pow(alpha, power);

                    fetchedColor = vec4(1, 1, 1, alpha * alphaMul);
                }
                else
                {
                    fetchedColor = vec4(1, 1, 1, alphaMul);
                }

                OutputColor = fetchedColor;
            }
        );

        array<Shader::Uniform, 0> uniforms{};

        array<Shader::Uniform, 2> systemUniforms{
            Shader::Uniform{"_ModelMatrix", 3, 4, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"_ViewProjectionMatrix", 4, 4, 1, Shader::Uniform::Type::F32}};

        array<string_view, 3> inputAttributes{"position", "texcoord", "scale"};

        auto shader = Shader::New(name, vsCode, psCode, uniforms.data(), (ui32)uniforms.size(), inputAttributes.data(), (ui32)inputAttributes.size(), systemUniforms.data(), (ui32)systemUniforms.size());

        LoadedShaders.emplace(shader);

        return shader;
    }
    else if (name == "OnlyDepthWrite")
    {
        string_view vsCode = SHADER_VERSION TOSTR(
            in vec4 position;

            uniform vec3 SidesScale;
            uniform mat4x3 _ModelMatrix;
            uniform mat4x4 _ViewProjectionMatrix;

            void main()
            {
                vec4 scaledPosition = vec4(position.xyz * SidesScale, position.w);
                vec3 worldPos = _ModelMatrix * scaledPosition;
                vec4 screenPos = _ViewProjectionMatrix * vec4(worldPos, 1.0);

                gl_Position = screenPos;
            }
        );

        string_view psCode = SHADER_VERSION TOSTR(
            void main()
            {
            }
        );

        array<Shader::Uniform, 1> uniforms{
            Shader::Uniform{"SidesScale", 3, 1, 1, Shader::Uniform::Type::F32}};

        array<Shader::Uniform, 2> systemUniforms{
            Shader::Uniform{"_ModelMatrix", 3, 4, 1, Shader::Uniform::Type::F32},
            Shader::Uniform{"_ViewProjectionMatrix", 4, 4, 1, Shader::Uniform::Type::F32}};

        array<string_view, 1> inputAttributes{"position"};

        auto shader = Shader::New(name, vsCode, psCode, uniforms.data(), (ui32)uniforms.size(), inputAttributes.data(), (ui32)inputAttributes.size(), systemUniforms.data(), (ui32)systemUniforms.size());

        LoadedShaders.emplace(shader);

        return shader;
    }

    return nullptr;
}