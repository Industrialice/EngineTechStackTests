#pragma once

#include "System.hpp"

namespace EngineCore
{
	class TextureSampler : public RendererFrontendData
	{
    public:
        enum class SampleMode : ui8
        {
            Tile /* aka Wrap or Repeat */,
            Mirror,
            Mirror_once, /* mirror once, the repeat the closes color to the border */
            Border, /* fetch border color is out of range */
            ClampToBorder /* aka ClampToEdge, repeat the closest color to the border */
        };

        enum class FilterMode : ui8
        {
            Nearest,
            Linear,
            Anisotropic
        };

    private:
		// TODO: these fields can be tightly packed
		SampleMode _xSampleMode = SampleMode::Tile;
		SampleMode _ySampleMode = SampleMode::Tile;
        SampleMode _zSampleMode = SampleMode::Tile;
		FilterMode _minFilterMode = FilterMode::Linear;
		FilterMode _magFilterMode = FilterMode::Linear;
		bool _minLinearInterpolateMips = true;
		ui8 _maxAnisotropy = 16; // 1 means no anisotropy filtering, 0 is clamped to 1

    protected:
        TextureSampler() = default;
		TextureSampler(TextureSampler &&) = delete;
		TextureSampler &operator = (TextureSampler &&) = delete;

	public:
        static shared_ptr<TextureSampler> New();

		~TextureSampler() = default;
		SampleMode XSampleMode() const;
		void XSampleMode(SampleMode mode);
		SampleMode YSampleMode() const;
		void YSampleMode(SampleMode mode);
        SampleMode ZSampleMode() const;
        void ZSampleMode(SampleMode mode);
		FilterMode MinFilterMode() const;
		void MinFilterMode(FilterMode mode);
		FilterMode MagFilterMode() const;
		void MagFilterMode(FilterMode mode);
		bool MinLinearInterpolateMips() const;
		void MinLinearInterpolateMips(bool doInterpolate);
		ui8 MaxAnisotropy() const;
		void MaxAnisotropy(ui8 level);
	};
}