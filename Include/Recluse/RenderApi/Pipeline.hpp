#ifndef RECLUSE_RENDER_API_PIPELINE_HPP
#define RECLUSE_RENDER_API_PIPELINE_HPP

#pragma once 

#include <Recluse/Types.hpp>
#include <Recluse/RenderApi/Common.hpp>

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
    Bool allowCaching;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_PIPELINE_HPP