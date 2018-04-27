#include "BasicHeader.hpp"
#include "RenderTarget.hpp"

using namespace EngineCore;

RenderTarget::RenderTarget(const shared_ptr<class Texture> &colorTarget, const shared_ptr<class Texture> &depthStencilTarget) : _colorTarget(colorTarget), _depthStencilTarget(depthStencilTarget)
{}

shared_ptr<RenderTarget> RenderTarget::New(const shared_ptr<class Texture> &colorTarget, const shared_ptr<class Texture> &depthStencilTarget)
{
    struct Proxy : public RenderTarget
    {
        Proxy(const shared_ptr<class Texture> &colorTarget, const shared_ptr<class Texture> &depthStencilTarget) : RenderTarget(colorTarget, depthStencilTarget) {}
    };
    return make_shared<Proxy>(colorTarget, depthStencilTarget);
}

void RenderTarget::ColorTarget(const shared_ptr<class Texture> &colorTarget)
{
    _colorTarget = colorTarget;
    BackendDataMayBeDirty();
}

void RenderTarget::DepthStencilTarget(const shared_ptr<class Texture> &depthStencilTarget)
{
    _depthStencilTarget = depthStencilTarget;
    BackendDataMayBeDirty();
}

auto RenderTarget::ColorTarget() const -> const shared_ptr<class Texture> &
{
	return _colorTarget;
}

auto RenderTarget::DepthStencilTarget() const -> const shared_ptr<class Texture> &
{
	return _depthStencilTarget;
}

void RenderTarget::Name(string_view name)
{
	_name = name;
}

string_view RenderTarget::Name() const
{
	if (_name.empty())
	{
		return "{empty}";
	}
	return _name;
}
