#include "BasicHeader.hpp"
#include "Renderer.hpp"
#include "Application.hpp"

using namespace EngineCore;

void *Renderer::RendererBackendData(const RendererFrontendData &frontendData)
{
    return frontendData._backendData;
}

void Renderer::RendererBackendData(const RendererFrontendData &frontendData, void *data)
{
    frontendData._backendData = data;
}

void **Renderer::RendererBackendDataPointer(const RendererFrontendData &frontendData)
{
    return &frontendData._backendData;
}

bool Renderer::RendererFrontendDataDirtyState(const RendererFrontendData &frontendData)
{
    return frontendData._is_updatedByFrontEnd;
}

void Renderer::RendererFrontendDataDirtyState(const RendererFrontendData &frontendData, bool isChanged)
{
    frontendData._is_updatedByFrontEnd = isChanged;
}

RendererFrontendData::~SystemFrontendData()
{
    if (_backendData != nullptr)
    {
        Application::GetRenderer().NotifyFrontendDataIsBeingDeleted(*this);
    }
}

void RendererFrontendData::BackendDataMayBeDirty() const
{
    _is_updatedByFrontEnd = true;
}