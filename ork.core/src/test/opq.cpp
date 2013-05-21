#include <ork/timer.h>
#include <ork/opq.h>
#include <unittest++/UnitTest++.h>
#include <string.h>

typedef uint32_t u32;

using namespace ork;

#if 1
TEST(opq_serialized_ops)
{
	float fsynctime = ork::get_sync_time();

	srand( u32(fsynctime*100.0f) ); // so we dont get the same thread count seqeuence every run

	for( int i=0; i<16; i++ )
	{
		const int knumthreads = 1 + rand()%64;
		const int kopqconcurr = knumthreads;
		const int kops = 1<<14;

	    auto gopq1 = new OpMultiQ(knumthreads);
	    auto l0grp = gopq1->CreateOpGroup("l0");
	    auto l1grp = gopq1->CreateOpGroup("l1");
	    auto l2grp = gopq1->CreateOpGroup("l2");
	    auto l3grp = gopq1->CreateOpGroup("l3");
	    l0grp->mLimitMaxOpsInFlight = 1;
	    l1grp->mLimitMaxOpsInFlight = 1;
	    l2grp->mLimitMaxOpsInFlight = 1;
	    l3grp->mLimitMaxOpsInFlight = 1;

	    ork::atomic<int> ops_in_flight;
	    ork::atomic<int> level0_counter;
	    ork::atomic<int> level1_counter;
	    ork::atomic<int> level2_counter;
	    ork::atomic<int> level3_counter;

	    ops_in_flight = 0;
	    level0_counter = 0;
		level1_counter = 0;
		level2_counter = 0;
		level3_counter = 0;

	    ork::Timer measure;
	    measure.Start();

	    for( int i=0; i<kops; i++ )
	    {
	    	auto l0_op = [&]()
			{
		    	ops_in_flight++;

		    	assert(int(ops_in_flight)<=kopqconcurr);
		    	assert(int(ops_in_flight)>=0);

				int this_l0cnt = level0_counter++; // record l0cnt at l1 queue time

				auto l1_op = [=,&level1_counter,&level2_counter,&level3_counter]()
				{
					int this_l1cnt = level1_counter++; // record l1cnt at l2 queue time
					assert(this_l1cnt==this_l0cnt);

					auto l2_op = [=,&level2_counter,&level3_counter]()
					{
						int this_l2cnt = level2_counter++; // record l2cnt at l3 queue time
						assert(this_l2cnt==this_l1cnt);

						auto l3_op = [=,&level3_counter]()
						{
							int this_l3cnt = level3_counter++;
							assert(this_l3cnt==this_l2cnt);
						};
						l3grp->push(Op(l3_op));

					};
					l2grp->push(Op(l2_op));

				};
				l1grp->push(Op(l1_op));

				ops_in_flight--;
		    };

		    l0grp->push(Op(l0_op,"yo"));
		}

	    gopq1->drain();

		CHECK(level0_counter==kops);
		CHECK(level1_counter==kops);

		float elapsed = measure.SecsSinceStart();

		float fopspsec = float(kops*2)/elapsed;

		printf( "opq_serops test nthr<%d> elapsed<%.02f sec> nops<%d> ops/sec[agg]<%d> ops/sec[/thr]<%d> counter<%d>\n", knumthreads, elapsed, kops, int(fopspsec), int(fopspsec/float(knumthreads)), int(level0_counter) );

	    delete gopq1;
	}
}
#endif
#if 0
TEST(opq_maxinflight)
{
	float fsynctime = ork::get_sync_time();

	srand( u32(fsynctime*100.0f) ); // so we dont get the same thread count seqeuence every run

	for( int i=0; i<16; i++ )
	{
		const int knumthreads = 1 + rand()%64;
		const int kopqconcurr = knumthreads;
		const int kops = knumthreads*1000;

	    auto gopq1 = new OpMultiQ(knumthreads);
	    auto gopg1 = gopq1->CreateOpGroup("g1");
	    gopg1->mLimitMaxOpsInFlight = kopqconcurr;

	    //printf( "OpLimit::1 <%d>\n", kopqconcurr );

	    std::atomic<int> ops_in_flight;
	    std::atomic<int> counter;

	    ops_in_flight = 0;
	    counter = 0;

	    ork::Timer measure;
	    measure.Start();

	    for( int i=0; i<kops; i++ )
	    {
	    	auto the_op = [&]()
		    {
		    	ops_in_flight++;

		    	assert(int(ops_in_flight)<=kopqconcurr);
		    	assert(int(ops_in_flight)>=0);

		    	int ir = 16+(rand()%16);
		    	float fr = float(ir)*0.00001f;

			    ork::Timer throttle;
	            throttle.Start();

	            //printf( "BEG Op<%p> wait<%dms> OIF<%d>\n", this, ir, int(ops_in_flight) );

	            int i =0;

	            while(throttle.SecsSinceStart()<fr)
	            {
	            	i++;
	            }

				ops_in_flight--;

				counter++;

		    };

		    gopg1->push( Op(the_op,"yo") );

		    int j = rand()%1000;
		    if( j<10 )
			    gopq1->drain();

		}

	    //gopg1->drain();
	    gopq1->drain();

		CHECK(counter==kops);

		float elapsed = measure.SecsSinceStart();

		float fopspsec = float(kops)/elapsed;

		printf( "opqmaxinflight test nthr<%d> elapsed<%.02f sec> nops<%d> ops/sec[agg]<%d> ops/sec[/thr]<%d> counter<%d>\n", knumthreads, elapsed, kops, int(fopspsec), int(fopspsec/float(knumthreads)), int(counter) );

	    delete gopq1;
	}
}
#endif
#if 1

TEST(opq_maxinflight2)
{
	float fsynctime = ork::get_sync_time();

	srand( u32(fsynctime*100.0f) ); // so we dont get the same thread count seqeuence every run

	for( int i=0; i<16; i++ )
	{
		const int knumthreads = 1 + rand()%64;
		const int kopqconcurr = knumthreads;
		const int kops = knumthreads*100;

	    auto gopq1 = new OpMultiQ(knumthreads);
	    auto gopg1 = gopq1->CreateOpGroup("g1");
	    gopg1->mLimitMaxOpsInFlight = kopqconcurr;

	    //printf( "OpLimit::2 <%d>\n", kopqconcurr );

	    ork::atomic<int> ops_in_flight;
	    ork::atomic<int> counter;

	    ops_in_flight = 0;
	    counter = 0;

	    ork::Timer measure;
	    measure.Start();

	    for( int i=0; i<kops; i++ )
	    {
		    gopg1->push(Op(
		    [&]()
		    {
		    	ops_in_flight++;

		    	assert(int(ops_in_flight)<=kopqconcurr);
		    	assert(int(ops_in_flight)>=0);

		    	int ir = 16+(rand()%16);
		    	float fr = float(ir)*0.00001f;

			    ork::Timer throttle;
	            throttle.Start();

	            //printf( "BEG Op<%p> wait<%dms> OIF<%d>\n", this, ir, int(ops_in_flight) );

	            int i =0;

	            while(throttle.SecsSinceStart()<fr)
	            {
	            	i++;
	            }

				ops_in_flight--;

				counter++;

		    },"yo"));
		}

	    gopg1->drain();
	    gopq1->drain();

		CHECK(counter==kops);

		float elapsed = measure.SecsSinceStart();

		float fopspsec = float(kops)/elapsed;

		printf( "opq_mif2 test nthr<%d> elapsed<%.02f sec> nops<%d> ops/sec[agg]<%d> ops/sec[/thr]<%d> counter<%d>\n", knumthreads, elapsed, kops, int(fopspsec), int(fopspsec/float(knumthreads)), int(counter) );

	    delete gopq1;
	}
}
#endif