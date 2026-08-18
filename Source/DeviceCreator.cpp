//
#include <Recluse/RenderApi/Context.hpp>

namespace Recluse {
namespace RenderApi {


Context* Context::create(Api api, const Description& description)
{
    return nullptr;
}


ResultCode Context::free(Context* context)
{
    return RecluseResult_NoImpl;
}
} // RenderApi
} // Recluse