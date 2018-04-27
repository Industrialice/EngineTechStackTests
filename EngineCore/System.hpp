#pragma once

namespace EngineCore
{
	template <typename System> class SystemFrontendData
	{
		friend System;

		mutable void *_backendData = nullptr;
		mutable bool _is_updatedByFrontEnd = true;

        // that class cannot be movable or copyable because a backend can store the address of _backendData
        SystemFrontendData(SystemFrontendData &&) = delete;
        SystemFrontendData &operator = (SystemFrontendData &&) = delete;

	protected:
        ~SystemFrontendData();
        SystemFrontendData() = default;
        void BackendDataMayBeDirty() const;
	};

	using RendererFrontendData = SystemFrontendData<class Renderer>;
}