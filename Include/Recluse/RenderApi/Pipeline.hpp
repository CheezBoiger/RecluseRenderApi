#ifndef RECLUSE_RENDER_API_PIPELINE_HPP
#define RECLUSE_RENDER_API_PIPELINE_HPP

#pragma once 

#include <Recluse/Types.hpp>
#include <Recluse/RenderApi/Common.hpp>

#include <vector>

namespace Recluse {
namespace RenderApi {

struct ShaderDescription
{
    ShaderDescription()
        : byteCode(nullptr)
        , entryPoint(nullptr)
    {}
    const char* byteCode;
    const char* entryPoint;
};

struct PipelineDescription
{
    PipelineDescription()
        : vertexShader()
        , pixelShader()
        , computeShader()
        , meshShader()
        , raytraceShader()
    {}
    ShaderDescription vertexShader;
    ShaderDescription pixelShader;
    ShaderDescription computeShader;
    ShaderDescription meshShader;
    ShaderDescription raytraceShader;
};


class Pipeline
{
public:
    struct StreamChunk
    {
        std::vector<u8> bytecode;
    };

    static const PipelineId kBadId = ~0;

    Pipeline(PipelineId id = kBadId) : m_id(id) { }
    virtual ~Pipeline() { }

    PipelineId getId() const { return m_id; }

    virtual ResultCode cache(StreamChunk* chunk) = 0;

private:
    PipelineId m_id;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_PIPELINE_HPP