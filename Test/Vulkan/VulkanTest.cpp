#include <Recluse/RenderApi/Context.hpp>
#include <Recluse/RenderApi/Adapter.hpp>

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