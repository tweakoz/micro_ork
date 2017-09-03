///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <ork/http.h>
#include <mutex>
#include <curl/curl.h>
#include <cstdlib>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

HttpRequest::HttpRequest(const std::string& url)
    : _url(url)
{
    /////////////////////////////////
    // one time init - start up CURL
    /////////////////////////////////

    static std::once_flag ginithttp;
    std::call_once(ginithttp, [](){
        curl_global_init(CURL_GLOBAL_DEFAULT);

        ////////////
        // register CURL global
        //  cleanup @ exit
        ////////////

        struct onexit
        {   static void doit()
            {   curl_global_cleanup();
            }
        };

        std::atexit(onexit::doit);

    });

    /////////////////////////////////
    // per HttpRequest init..
    /////////////////////////////////

    _impl = curl_easy_init();
    if( auto curl = _impl.TryAs<CURL*>() )
        curl_easy_setopt(curl.value(), CURLOPT_URL, _url.c_str() );
    else
        _impl = nullptr;
 
    /////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////

HttpRequest::~HttpRequest()
{
    if( auto curl = _impl.TryAs<CURL*>() )
        curl_easy_cleanup(curl.value());        

}

///////////////////////////////////////////////////////////////////////////////

bool HttpRequest::get(on_datacb_t ondata)
{
    bool rval = false;
    if( auto curl = _impl.TryAs<CURL*>() )
    {
        auto res = curl_easy_perform(curl.value());
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));
    }


    return rval;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork