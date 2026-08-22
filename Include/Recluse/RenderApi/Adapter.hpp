#ifndef RECLUSE_RENDER_API_RENDER_ADAPTER_HPP
#define RECLUSE_RENDER_API_RENDER_ADAPTER_HPP
#pragma once

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Device.hpp>

namespace Recluse {
namespace RenderApi {

class Adapter : public IApiObject
{
public:
    Adapter(uint id) : m_id(id) { }

    enum Type
    {
        Type_Unknown = 0,
        Type_Discrete,
        Type_Integrated,
        Type_Virtual,
        Type_CPU,
    };

    struct Information
    {
        char        name[128];
        char        description[256];
        const char* vendorName;
        U32         vendorId;
        U32         deviceId;
        U32         driverVersion;
        U32         apiVersion;
        Type        type;
        uint        index;
    };
    
    virtual ResultCode  queryInformation(Information* info) = 0;
    virtual Bool        supportsFeature(FeatureFlag feature) = 0;
    virtual Device*     createDevice(const Device::Description& desc) = 0;

    virtual ResultCode  freeDevice(Device* device) = 0;

    uint getId() const { return m_id; }

private:
    uint m_id;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_ADAPTER_HPP