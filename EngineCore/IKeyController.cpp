#include "BasicHeader.hpp"
#include "IKeyController.hpp"

using namespace EngineCore;

void IKeyController::RemoveListener(IKeyController *instance, void *handle)
{
    return instance->RemoveListener(*(ListenerHandle *)handle);
}
