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
    , _numbytessofar(0)
    , _knownlength(0)
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

    ////////////////////////////////////

    struct HttpRequestImpl
    {
        ////////////////////////////////////

        static size_t onData(
            void *inc_data_ptr,
            size_t inc_item_size,
            size_t inc_num_items,
            void *user_data )
        {
            auto req = (HttpRequest*) user_data;
            size_t numbytes = (inc_item_size*inc_num_items);
            size_t prevbase = req->_numbytessofar;
            req->_numbytessofar += numbytes;

            //////////////
            // resize payload
            //   and copy bytes into it
            //////////////

            req->_payload.resize(req->_numbytessofar);
            char* pdestbase = (char*) req->_payload.data();
            memcpy((void*)(pdestbase+prevbase),inc_data_ptr,numbytes);

            //////////////

            //printf( "httpreq<%p> recieveddata numbytes<%zu> sofar<%zu>\n", req, numbytes, req->_numbytessofar );
            return numbytes;
        }

        ////////////////////////////////////

        static size_t onHdrData(
            void *inc_data_ptr,
            size_t inc_item_size,
            size_t inc_num_items,
            void *user_data )
        {
            // The header callback will be called once for each header
            //  and only complete header lines are passed on to the callback
            // Parsing headers is very easy using this. 
            // The size of the data pointed to by buffer is size multiplied
            // with nmemb. 
            // Do not assume that the header line is zero terminated

            auto req = (HttpRequest*) user_data;
            size_t numbytes = (inc_item_size*inc_num_items);
            auto copy = (char*) malloc(numbytes+1);
            memcpy(copy,inc_data_ptr,numbytes);
            copy[numbytes]=0;
            printf( "httpreq<%p> recievedhdr %s", req, copy );
            if( strstr(copy,"Content-Length:")==copy )
            {
                req->_knownlength = atol(copy+16);
                req->_payload.reserve(req->_knownlength);

                printf( "_knownlength<%zu>\n", req->_knownlength );
            }

            free(copy);
            return numbytes;
        }
    };

    ////////////////////////////////////

    if( auto try_curl = _impl.TryAs<CURL*>() )
    {
        auto curl = try_curl.value();

        ///////////////////////////
        // set up curl options
        ///////////////////////////

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpRequestImpl::onData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) this );
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HttpRequestImpl::onHdrData);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) this );

        ///////////////////////////
        // do the get
        ///////////////////////////

        auto res = curl_easy_perform(curl);
        if(res == CURLE_OK)
        {
            printf( "payload size<%zu>\n", _payload.size());
            printf( "_knownlength<%zu>\n", _knownlength );

            assert(_knownlength==_payload.size());
            rval = true;
        }
        else
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }


    return rval;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork