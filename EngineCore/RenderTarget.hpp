#pragma once

#include "System.hpp"

namespace EngineCore
{
	class RenderTarget : public RendererFrontendData
	{
        shared_ptr<class Texture> _colorTarget{}; // nullptr means the main window
        shared_ptr<class Texture> _depthStencilTarget{}; // nullptr means the main window
        string _name{};

		RenderTarget(RenderTarget &&) = delete;
		RenderTarget &operator = (RenderTarget &&) = delete;

    protected:
        RenderTarget(const shared_ptr<class Texture> &colorTarget, const shared_ptr<class Texture> &depthStencilTarget);

	public:
        static shared_ptr<RenderTarget> New(const shared_ptr<class Texture> &colorTarget = nullptr, const shared_ptr<class Texture> &depthStencilTarget = nullptr);

		~RenderTarget() = default;
        void ColorTarget(const shared_ptr<class Texture> &colorTarget);
        void DepthStencilTarget(const shared_ptr<class Texture> &depthStencilTarget);
		const shared_ptr<class Texture> &ColorTarget() const;
		const shared_ptr<class Texture> &DepthStencilTarget() const;
		void Name(string_view name);
		string_view Name() const;
	};
}