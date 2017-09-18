//

#include <unittest++/UnitTest++.h>
#include <string.h>
#include <ork/atomic.h>

int main( int argc, char** argv, char** argp )
{	int rval = 0;
	ork::atomic_counter::init();
	/////////////////////////////////////////////
	// default List All Tests
	/////////////////////////////////////////////
	if( argc < 2 ) 
	{
        bool blist_tests = true;

        const char *testname = argv[1];
        const UnitTest::TestList & List = UnitTest::Test::GetTestList();
        const UnitTest::Test* ptest = List.GetHead();
        int itest = 0;

        printf( "//////////////////////////////////\n" );
        printf( "Listing Tests (use all for all tests\n" );
        printf( "//////////////////////////////////\n" );


        while( ptest )
        {   const UnitTest::TestDetails & Details = ptest->m_details;
            if( blist_tests )
                printf( "Test<%d:%s>\n", itest, Details.testName );
            ptest = ptest->next;
            itest++;
        }
    }
	/////////////////////////////////////////////
	// run a single test (higher signal/noise for debugging)
	/////////////////////////////////////////////
	else if( argc == 2 )
	{	
		bool all_tests = (0 == strcmp( argv[1], "all" ));

		const char *testname = argv[1];
		const UnitTest::TestList & List = UnitTest::Test::GetTestList();
		const UnitTest::Test* ptest = List.GetHead();
		int itest = 0;

		while( ptest )
		{	const UnitTest::TestDetails & Details = ptest->m_details;
			
			if( all_tests or (0 == strcmp( testname, Details.testName )) )
			{	printf( "Running Test<%s>\n", Details.testName );

				UnitTest::TestResults res;
				ptest->Run(res);
			}
			ptest = ptest->next;
			itest++;
		}
	}
	return rval;
}

