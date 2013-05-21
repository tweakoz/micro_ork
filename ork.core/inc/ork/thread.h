#pragma once

#include <pthread.h>
#include <ork/svariant.h>
#include <ork/opq.h>
#include <functional>

namespace ork {


struct thread
{

	thread()
	{
		mState = 0;
	}
	~thread()
	{
		join();
	}

	inline static void* thread_impl(void* pdat)
	{
		thread* pthr = (thread*) pdat;
		pthr->mLambda();
		return nullptr;
	}
	inline void start( const ork::void_lambda_t& l )
	{
		if( mState.fetch_and_increment()==0 )
		{
			mLambda = l;
			pthread_create(&mThreadHandle, NULL, thread_impl, (void*) this );

		}
	}

	inline void join()
	{
		if( mState.fetch_and_decrement()==1 )
		{
			pthread_join(mThreadHandle,NULL);
		}
	}

	tbb::atomic<int> mState;
	pthread_t mThreadHandle;
	ork::void_lambda_t mLambda;

};

}

