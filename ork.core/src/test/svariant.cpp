#include <unittest++/UnitTest++.h>
#include <ork/svariant.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

struct my_yo
{
	std::string mOne;
	std::string mTwo;
	std::string mThree;
	std::string mFour;
	std::string mFive;
	std::string mSix;
};

TEST(sv64_non_pod)
{
	my_yo the_yo;
	ork::svar64_t v64(the_yo);
	ork::svar64_t v64b;

	printf( "sizeof<my_yo:%d>\n", int(sizeof(my_yo)));

	v64b = v64;
}


