///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ork/types.h>
#include <ork/svariant.h>
#include <functional>
#include <ork/atomic.h>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

struct HttpRequest
{
    typedef std::function<void(HttpRequest*,const void*,size_t)> on_datacb_t;

    HttpRequest(const std::string& url);
    ~HttpRequest();

    bool get(on_datacb_t=nullptr);

    std::string _url;
    svarp_t _impl;
    size_t _numbytessofar;
    size_t _knownlength;
    std::vector<uint8_t> _payload;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace ork