#include <iostream>

#include <Recluse/RenderApi/Context.hpp>
#include <Recluse/RenderApi/CommandList.hpp>
#include <Recluse/Logger.hpp>

using namespace std;

using namespace Recluse::RenderApi;

int main(int argc, char** argv)
{
    Recluse::LogSystem::initializeLoggingSystem();

    
    CommandList commandlist;
    commandlist.begin();
    commandlist.bindRenderTargets(nullptr, 0);
    commandlist.drawIndexedInstanced(36, 1, 0, 0, 0);
    commandlist.end();

    CommandStreamChunk chunk = commandlist.getChunk();

    Recluse::RenderApi::ContextHandle context = Recluse::RenderApi::Context::create(Recluse::RenderApi::Vulkan);

    if (!context) {
        cout << "Failed to create Vulkan context." << endl;
        return -1;
    }

    cout << "Hello World!" << endl;

    Recluse::ResultCode result = Recluse::RenderApi::Context::free(context);
    if (result != Recluse::RecluseResult_Ok) {
        cout << "Failed to free Vulkan context." << endl;
        return -1;
    }
    Recluse::LogSystem::destroyLoggingSystem();
    return 0;
}