///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <unittest++/UnitTest++.h>
#include <ork/http.h>
#include <ork/thread.h>
#if defined(ORK_OSX)

#include <Wt/WApplication>
#include <Wt/WServer>
#include <Wt/WResource>
#include <Wt/Http/Response>

using namespace ork;
using namespace Wt;

///////////////////////////////////////////////////////////////////////////////

TEST(HttpOne)
{
    //////////////////////////////////////////////
    // testapp API endpoint 
    //////////////////////////////////////////////

    struct TestApi : public Wt::WResource
    {
      TestApi() {}
      ~TestApi() final {}

      void handleRequest(const Wt::Http::Request &request,
                         Wt::Http::Response &response) final
      {
        response.setMimeType("text/plain; charset=us-ascii");
        response.addHeader("Content-Length:", FormatString("%d",7) );
        response.out()<<"what up";
      }
    };

    //////////////////////////////////////////////
    // start app in its own thread
    //////////////////////////////////////////////

    auto thr = std::make_shared<thread>("wtserver_thread");

    std::shared_ptr<WServer> wtserver;

    std::atomic<bool> ok_to_exit(false);

    thr->start([&wtserver,&ok_to_exit](){

        static char* argv[] = 
        {
            "yo",
            "--docroot=/",
            "--http-port=1234",
            "--http-address=localhost",
            "--verbose",
            0
        };

        int argc = 5;

        try {

            wtserver = std::make_shared<WServer>(argv[0]);
            wtserver->setServerConfiguration(argc, argv, WTHTTP_CONFIGURATION);
            auto tapi = std::make_shared<TestApi>();
            wtserver->addResource(tapi.get(), "/yo");

            if (wtserver->start()) {
                while(false==ok_to_exit)
                    usleep(1<<18);
                wtserver->stop();
            }

        } catch (WServer::Exception& e) {

            std::cerr << e.what() << "\n";
            assert(false);

        } catch (std::exception& e) {

            std::cerr << "exception: " << e.what() << "\n";
            assert(false);

        }

        printf( "Leaving Server Thread\n");
    });
    
    //////////////////////////////////////////////
    // todo better sync method
    //////////////////////////////////////////////

    usleep(1<<20);

    HttpRequest req("http://localhost:1234/yo");
    req.get();
    std::string reponse = (const char*) req._payload.data();
    printf( "got<%s>\n", reponse.c_str()  );

    //////////////////////////////////////////////
    // todo better server stop method
    //////////////////////////////////////////////

    ok_to_exit=true;
    thr->join();

    //////////////////////////////////////////////

    CHECK_EQUAL(reponse,"what up");
}

#endif
