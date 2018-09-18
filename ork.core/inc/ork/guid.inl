#pragma once

#if defined(ORK_OSX)
#include <CoreFoundation/CFUUID.h>
#else
#include <uuid/uuid.h>
#endif

namespace ork {

// This is the linux friendly implementation, but it could work on other
// systems that have libuuid available
#ifdef GUID_LIBUUID
Guid newGuid()
{
    uuid_t id;
    uuid_generate(id);
    return id;
}
#endif

// this is the mac and ios version
#ifdef GUID_CFUUID
Guid newGuid()
{
    auto newId = CFUUIDCreate(NULL);
    auto bytes = CFUUIDGetUUIDBytes(newId);
    CFRelease(newId);

    std::array<unsigned char, 16> byteArray =
    {
        bytes.byte0,
        bytes.byte1,
        bytes.byte2,
        bytes.byte3,
        bytes.byte4,
        bytes.byte5,
        bytes.byte6,
        bytes.byte7,
        bytes.byte8,
        bytes.byte9,
        bytes.byte10,
        bytes.byte11,
        bytes.byte12,
        bytes.byte13,
        bytes.byte14,
        bytes.byte15
    };
    return byteArray;
}
#endif

} // namespace ork
