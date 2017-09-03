///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <unittest++/UnitTest++.h>
#include <ork/http.h>

using namespace ork;

TEST(HttpOne)
{
    HttpRequest req("http://4k.com/wp-content/uploads/2014/06/4k-image-santiago.jpg");
    req.get();
    printf( "\n" );
    exit(0);
}
