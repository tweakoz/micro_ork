#include <unittest++/UnitTest++.h>
#include <cmath>
#include <limits>
#include <ork/cvector2.h>
#include <ork/math_misc.h>
#include <string.h>

#include <ork/ringbuffer.hpp>
#include <ork/cringbuffer.inl>
#include <ork/svariant.h>
#include <ork/timer.h>
#include <ork/fixedstring.h>
#include <ork/thread.h>

using namespace ork;

typedef ork::thread thread_t;

struct EOTEST
{

};

static const int knummsgs = 1<<16;

template <typename queue_type>
	struct yo 
	{
		typedef typename queue_type::value_type value_type;

		queue_type mQueue;

		void RunTest()
		{
			auto l_producer = [&]( yo* pyo )
			{
				queue_type& the_queue = pyo->mQueue;
				printf( "STARTED PRODUCER knummsgs<%d>\n", knummsgs);
				for( int i=0; i<knummsgs; i++ )
				{
					//printf( " prod pushing<%d>\n", i );
					the_queue.push(i);
					extstring_t str;
					str.format("to<%d>", i);
					the_queue.push(str);
				}			
				the_queue.push(EOTEST());
			};

			auto l_consumer = [&]( yo* pyo )
			{
				printf( "STARTED CONSUMER\n");
				queue_type& the_queue = pyo->mQueue;
				value_type popped;

				bool bdone=false;
				int ictr1 = 0;
				int ictr2 = 0;
				while(false==bdone)
				{
					while( the_queue.try_pop(popped) )
					{
						//printf( " cons pulling ictr1<%d> ictr2<%d>\n", ictr1, ictr2 );

						if( popped.template IsA<int>() )
						{
							int iget = popped.template Get<int>();
							assert(iget==ictr1);
							ictr1++;
						}
						else if( popped.template IsA<extstring_t>() )
						{
							extstring_t& es = popped.template Get<extstring_t>();
							if( (ictr2%(1<<18))==0 )
								printf( "popped: %s\n", es.c_str() );
							ictr2++;
							assert(ictr2==ictr1);
						}
						else if( popped.template IsA<EOTEST>() )
							bdone=true;
						else
							assert(false);
					}
					usleep(2000);
				}
			};

			double fsynctime = ork::get_sync_time();
			
			ork::thread thr_p, thr_c;


		    thr_p.start([=](){ l_producer( this ); });
		    thr_c.start([=](){ l_consumer( this ); });
			thr_p.join();
			thr_c.join();

			double fsynctime2 = ork::get_sync_time();

			double elapsed = (fsynctime2-fsynctime);
			double mps = 2.0*double(knummsgs)/elapsed;
			double msg_size = double(sizeof(value_type));
			double bps = msg_size*mps;
			double gbps = bps/double(1<<30);
			printf( "nummsgs<%g> elapsed<%g> msgpersec<%g> GBps(%g)\n", double(knummsgs), elapsed, mps, gbps );			
		}




	};

///////////////////////////////////////////////////////////////////////////////

TEST(OrkMpMcRingBuf)
{
	printf("//////////////////////////////////////\n" );
	printf( "ORK MPMC CONQ TEST\n");
	printf("//////////////////////////////////////\n" );

	//auto the_yo = new yo<ork::mpmc_bounded_queue<svar1024_t,16<<10>>;
	auto the_yo = new yo<ork::SpScRingBuf<svar16k_t,16<<10>>;
	the_yo->RunTest();
	delete the_yo;

	printf("//////////////////////////////////////\n" );

}

///////////////////////////////////////////////////////////////////////////////

TEST(cringbuf1)
{
	using namespace cringbuf;

	printf("//////////////////////////////////////\n" );
	printf( "cringbuffer TEST\n");
	printf("//////////////////////////////////////\n" );
	static const int knumworkers = 2;
	static const int krbsize = 128<<20;
	static const char MAGIC_BYTE = 0x5a;
	static const int kwid0 = 0;
	static const int kwid1 = 1;

	typedef ringbuf<2,krbsize> ringbuf_t;

	auto rb = new ringbuf_t;
	rb->init();

	auto w0 = rb->getWorker(kwid0);
	auto w1 = rb->getWorker(kwid1);

	ork::thread thr_p1, thr_p2, thr_c;

	std::atomic<int> pvalue;

	////////////////////////////////////////////////////

	static const size_t knummsg = 128<<20;

	auto l_producer = [&](ringbuf_worker_t* wkr){

		for( size_t i=0; i<knummsg; i++ )
		{
			//printf("producer<%p> count<%d>\n", wkr, i );
			int value = pvalue.fetch_add(1);
			const size_t len = sizeof(value);			
			bool did = false;

			while( false == did )
			{
				if(auto write_ptr = rb->tryEnqueue(wkr, len))
				{
					//memcpy(&rb_storage[off], &value, len);
					rb->enqueued(wkr);
					did = true;
				}
				else
				{
					usleep(100);
				}

			}
		}
		printf("producer<%p> done...\n", wkr );

	};

	////////////////////////////////////////////////////

	auto l_consumer = [&](){

		size_t byte_count = 0;
		size_t msg_count = 0;
		size_t expected = knummsg*knumworkers*sizeof(int);
		ork::Timer t;
		t.Start();

		while(byte_count<expected)
		{
			if( auto try_pkt = rb->tryDequeue() ) {
				rb->dequeued(try_pkt._length);
				byte_count += try_pkt._length;

				if( t.SecsSinceStart()>1.0f )
				{
					printf("consume msg_count<%zu> byte_count<%zu>\n", msg_count, byte_count );
					t.Start();
					msg_count = 0;
				}

				msg_count+=(try_pkt._length/sizeof(int));
			}
			else
				usleep(100);
		}
		assert(byte_count==expected);
		printf("consumer done msg_count<%zu> byte_count<%zu>\n", msg_count, byte_count );

	};

	////////////////////////////////////////////////////

    thr_p1.start([=](){ l_producer(w0); });
    thr_p2.start([=](){ l_producer(w1); });
    thr_c.start([=](){ l_consumer(); });

    thr_p1.join();
    thr_p2.join();
    thr_c.join();

	printf("//////////////////////////////////////\n" );

	delete rb;
}

///////////////////////////////////////////////////////////////////////////////
