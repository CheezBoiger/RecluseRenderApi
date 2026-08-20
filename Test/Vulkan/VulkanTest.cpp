#include <Recluse/RenderApi/Context.hpp>
#include <Recluse/RenderApi/Adapter.hpp>

#include <gtest/gtest.h>

using namespace Recluse;
using namespace Recluse::RenderApi;

TEST(VulkanTest, SimpleTest)
{
    Context::Description description;
    description.flags = 0;
    description.applicationName = "Test";
    description.engineName = "TestEngine";
    Context* context = Context::create(Api::Vulkan, description);

    ASSERT_NE(context, nullptr);
    EXPECT_NE(context->isValid(), false);

    ResultCode result = Context::free(context);

    EXPECT_EQ(result, RecluseResult_Ok);
}

TEST(VulkanTest, EnumerateDevices)
{
    Context::Description description;
    description.flags = 0;
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