#include "BasicHeader.hpp"
#include "TextureSampler.hpp"

using namespace EngineCore;

shared_ptr<TextureSampler> TextureSampler::New()
{
    struct Proxy : public TextureSampler
    {
        Proxy() : TextureSampler() {}
    };
    return make_shared<Proxy>();
}

auto TextureSampler::XSampleMode() const -> SampleMode
{
	return _xSampleMode;
}

void TextureSampler::XSampleMode(SampleMode mode)
{
	_xSampleMode = mode;
	BackendDataMayBeDirty();
}

auto TextureSampler::YSampleMode() const -> SampleMode
{
	return _ySampleMode;
}

void TextureSampler::YSampleMode(SampleMode mode)
{
	_ySampleMode = mode;
	BackendDataMayBeDirty();
}

auto TextureSampler::ZSampleMode() const -> SampleMode
{
    return _zSampleMode;
}

void TextureSampler::ZSampleMode(SampleMode mode)
{
    _zSampleMode = mode;
    BackendDataMayBeDirty();
}

auto TextureSampler::MinFilterMode() const -> FilterMode
{
	return _minFilterMode;
}

void TextureSampler::MinFilterMode(FilterMode mode)
{
	_minFilterMode = mode;
	BackendDataMayBeDirty();
}

auto TextureSampler::MagFilterMode() const -> FilterMode
{
	return _magFilterMode;
}

void TextureSampler::MagFilterMode(FilterMode mode)
{
	_magFilterMode = mode;
	BackendDataMayBeDirty();
}

bool TextureSampler::MinLinearInterpolateMips() const
{
	return _minLinearInterpolateMips;
}

void TextureSampler::MinLinearInterpolateMips(bool doInterpolate)
{
	_minLinearInterpolateMips = doInterpolate;
	BackendDataMayBeDirty();
}

ui8 TextureSampler::MaxAnisotropy() const
{
	return _maxAnisotropy;
}

void TextureSampler::MaxAnisotropy(ui8 level)
{
	_maxAnisotropy = std::max<ui8>(level, 1);
	BackendDataMayBeDirty();
}
