#ifndef RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP
#define RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP

#pragma once

#include <RenderCommon.hpp>

#include <Recluse/Types.hpp>
#include <Recluse/Memory/LinearScratchMemory.hpp>

namespace Recluse {
namespace RenderApi {
class CommandList
{
public:
    typedef U32 Id;
    static const Id kBadId = ~0;

    CommandList();
    CommandList(const CommandList&);
    CommandList(const CommandList&&);

    void begin();
    void end();

    void drawIndexedInstanced();
    void drawInstanced();
    void dispatch(U32 x, U32 y, U32 z);

    void bindPipeline();
    void bindResourceTable();

    void reset();

    Id getId() const { return m_id; }

private:
    // Render command list id.
    Id m_id;
    // 8 Megabytes is a good preinitial size, and should be cautiously used for mainly
    // drawcalls. If we exceed so much, it is better to optimize the game itself, in order 
    // to reduce these calls. (batching would be highly beneficial.)
    LinearScratchMemory<R_MB(8), 4096,  true> m_commandAllocator;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP