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

    const CommandStreamChunk* chunks = list.getChunks();
    const CommandStreamChunk& chunk = chunks[0]; 

    uint offsetBytes = 0;
    UPtr cursor = chunk.baseAddress;

    CommandHeader* header = reinterpret_cast<CommandHeader*>(cursor);
    EXPECT_EQ(header->opcode, CommandOpcode_Begin);
    EXPECT_EQ(header->size, 0);

    cursor += sizeof(CommandHeader);

    header = reinterpret_cast<CommandHeader*>(cursor);
    EXPECT_EQ(header->opcode, CommandOpcode_End);
    EXPECT_EQ(header->size, 0);

    EXPECT_EQ(chunk.sizeBytes, 8);
    list.reset();

    EXPECT_EQ(list.getNumChunks(), 0);
}