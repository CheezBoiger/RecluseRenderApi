#include <iostream>

#include <Recluse/RenderApi/Context.hpp>
#include <Recluse/RenderApi/CommandList.hpp>
#include <Recluse/Logger.hpp>

#include <gtest/gtest.h>

#include <Shared/CommandOps.hpp>

using namespace std;

using namespace Recluse;
using namespace Recluse::RenderApi;

TEST(CommandListTest, SimpleCommand)
{
    CommandList list;

    list.begin();
    list.end();

    CommandStreamChunk chunk = list.getChunk();

    uint offsetBytes = 0;
    UPtr cursor = chunk.baseAddress;

    CommandHeader* header = reinterpret_cast<CommandHeader*>(cursor);
    EXPECT_EQ(header->opcode, CommandOpcode_Begin);
    EXPECT_EQ(header->size, sizeof(CommandListDescription));

    cursor += sizeof(CommandHeader);

    CommandListDescription* desc = reinterpret_cast<CommandListDescription*>(cursor);
    EXPECT_EQ(desc->instance, CommandList::Dynamic);
    EXPECT_EQ(desc->type, CommandList::Primary);

    cursor += header->size;

    header = reinterpret_cast<CommandHeader*>(cursor);
    EXPECT_EQ(header->opcode, CommandOpcode_End);
    EXPECT_EQ(header->size, 0);

    EXPECT_EQ(chunk.sizeBytes, 16);
    list.reset();

    chunk = list.getChunk();
    EXPECT_EQ(chunk.baseAddress, 0);
    EXPECT_EQ(chunk.sizeBytes, 0);
}