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
    HttpRequest req("https://raw.githubusercontent.com/tweakoz/micro_ork/master/README.md");
    req.get();
    printf( "\n" );
    //exit(0);
}
