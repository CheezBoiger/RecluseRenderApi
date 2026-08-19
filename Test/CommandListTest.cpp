#include <iostream>

#include <Recluse/RenderApi/Context.hpp>
#include <Recluse/RenderApi/CommandList.hpp>
#include <Recluse/Logger.hpp>

#include <gtest/gtest.h>

#include <Shared/CommandOps.hpp>

using namespace std;

using namespace Recluse;
using namespace Recluse::RenderApi;

TEST(VulkanTest, SimpleCommand)
{
    CommandList list;

    list.begin();
    list.end();

    CommandStreamChunk chunk = list.getChunk();

    uint offsetBytes = 0;
    UPtr cursor = chunk.baseAddress;

    CommandHeader* header = reinterpret_cast<CommandHeader*>(cursor);
    EXPECT_EQ(header->opcode, CommandOpcode_Begin);
    EXPECT_EQ(header->size, sizeof(CommandHeader));

    cursor += header->size;

    header = reinterpret_cast<CommandHeader*>(cursor);
    EXPECT_EQ(header->opcode, CommandOpcode_End);
    EXPECT_EQ(header->size, sizeof(CommandHeader));

    EXPECT_EQ(chunk.sizeBytes, 8);
    list.reset();

    chunk = list.getChunk();
    EXPECT_EQ(chunk.baseAddress, 0);
    EXPECT_EQ(chunk.sizeBytes, 0);
}