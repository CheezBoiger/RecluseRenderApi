#include <Recluse/RenderApi/Context.hpp>
#include <Recluse/RenderApi/Adapter.hpp>
#include <Recluse/RenderApi/Resource.hpp>

#include <Recluse/System/Window.hpp>
#include <Recluse/System/Input.hpp>
#include <Recluse/Logger.hpp>

#include <gtest/gtest.h>

using namespace Recluse;
using namespace Recluse::RenderApi;

TEST(VulkanTest, SimpleTest)
{
    Context::Description description = { };
    description.applicationName = "Test";
    description.engineName = "TestEngine";
    description.enableValidation = true;
    Context* context = Context::create(Api::Vulkan, description);

    ASSERT_NE(context, nullptr);
    EXPECT_NE(context->isValid(), false);

    ResultCode result = Context::free(context);

    EXPECT_EQ(result, RecluseResult_Ok);
}

TEST(VulkanTest, EnumerateDevices)
{
    Context::Description description = { };
    description.applicationName = "Test";
    description.engineName = "TestEngine";
    Context* context = Context::create(Api::Vulkan, description);

    ASSERT_NE(context, nullptr);
    EXPECT_NE(context->isValid(), false);

    std::vector<Adapter::Information> info;
    u32 count = context->enumerateAdapterInformation(nullptr, 0);
    EXPECT_NE(count, 0);
    info.resize(count);
    context->enumerateAdapterInformation(info.data(), count);

    for (uint i = 0; i < count; ++i)
    {
        EXPECT_NE(info[i].vendorId, 0);
        EXPECT_GE(info[i].index, 0);
        EXPECT_LE(info[i].index, count-1);
        EXPECT_NE(info[i].type, Adapter::Type_Unknown);
    }

    ResultCode result = Context::free(context);

    EXPECT_EQ(result, RecluseResult_Ok);
}

TEST(VulkanTest, CreateAdapter)
{
    Context* context = nullptr;
    Adapter* adapter = nullptr;
    {
        Context::Description description = { };
        description.applicationName = "Test";
        description.engineName = "TestEngine";
        description.enableValidation = true;
        context = Context::create(Api::Vulkan, description);
        ASSERT_NE(context, nullptr);
    }

    {
        std::vector<Adapter::Information> info;
        u32 count = context->enumerateAdapterInformation(nullptr, 0);
        EXPECT_NE(count, 0);
        info.resize(count);
        context->enumerateAdapterInformation(info.data(), count);
        uint index = -1;
        for (uint i = 0; i < count; ++i)
        {
            if (info[i].type == Adapter::Type_Discrete || info[i].type == Adapter::Type_Integrated)
            {
                index = info[i].index;
                break;
            }
        }

        ASSERT_NE(index, (uint)-1);
        
        adapter = context->createAdapter(index);
    }

    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->isValid(), true);
    ASSERT_EQ(context->freeAdapter(adapter), RecluseResult_Ok);
    Context::free(context);
}

TEST(VulkanTest, CreateDevice)
{
    LogSystem::initializeLoggingSystem();
    Window* window = Window::create("Test", 0, 0, 960, 620);
    window->setToCenter();
    window->show();
    Context* context = nullptr;
    Adapter* adapter = nullptr;
    {
        Context::Description description = { };
        description.applicationName = "Test";
        description.engineName = "TestEngine";
        description.enableValidation = true;
        context = Context::create(Api::Vulkan, description);
        ASSERT_NE(context, nullptr);
    }

    {
        std::vector<Adapter::Information> info;
        u32 count = context->enumerateAdapterInformation(nullptr, 0);
        EXPECT_NE(count, 0);
        info.resize(count);
        context->enumerateAdapterInformation(info.data(), count);
        uint index = -1;
        for (uint i = 0; i < count; ++i)
        {
            if (info[i].type == Adapter::Type_Discrete || info[i].type == Adapter::Type_Integrated)
            {
                index = info[i].index;
                break;
            }
        }

        ASSERT_NE(index, (uint)-1);
        
        adapter = context->createAdapter(index);
    }

    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->isValid(), true);

    Device::Description deviceDesc = { };
    CommandQueueType queues[] = { CommandQueueType_Compute, CommandQueueType_Graphics, CommandQueueType_Copy };
    deviceDesc.queueTypes = queues;
    deviceDesc.queueTypeCount = 3;
    deviceDesc.enableMeshShaders = true;
    Device* device = adapter->createDevice(deviceDesc);
    EXPECT_NE(device, nullptr); 

    Swapchain::Description swapchainDesc = { };
    swapchainDesc.renderWidth = window->getWidth();
    swapchainDesc.renderHeight = window->getHeight();
    swapchainDesc.format = ResourceFormat_R8G8B8A8_Unorm;
    swapchainDesc.numFrames = 5;
    swapchainDesc.usage = 0;
    swapchainDesc.presentMode = Swapchain::PresentMode_VSync;
    swapchainDesc.windowHandle = window->getNativeHandle();
    
    Swapchain* swapchain = device->createSwapchain(swapchainDesc);
    EXPECT_NE(swapchain, nullptr);

    FrameProcess::Description desc = { };
    desc.maxFramesInFlight = 2;
    desc.numCommandListJobThreads = 2;
    FrameProcess* frameProcessor = device->createFrameProcess(desc);
    ASSERT_NE(frameProcessor, nullptr);

    int i = 0;
    while (!window->shouldClose())
    {
        pollEvents();
        i++;

        //// Swapchain    
        FrameProcess::FrameDescription frameDescription;
        frameDescription.swapchain = swapchain;

        frameProcessor->beginFrame(frameDescription);
        CommandList commandlist;

        commandlist.begin({ Primary, Dynamic });
        commandlist.transition(swapchain->currentBackbuffer(), ResourceState_Present);
        commandlist.end();

        frameProcessor->submitCommandLists(CommandQueueType_Graphics, &commandlist, 1);

        device->processFrame(frameProcessor->endFrame());

        if (i == 1000)
            window->close();
    }

    frameProcessor->waitIdle();

    EXPECT_EQ(device->freeFrameProcess(frameProcessor), RecluseResult_Ok);
    EXPECT_EQ(device->freeSwapchain(swapchain), RecluseResult_Ok);
    EXPECT_EQ(adapter->freeDevice(device), RecluseResult_Ok);
    ASSERT_EQ(context->freeAdapter(adapter), RecluseResult_Ok);
    Context::free(context);
    Window::destroy(window);
    LogSystem::destroyLoggingSystem();
}