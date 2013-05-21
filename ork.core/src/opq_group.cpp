///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <ork/types.h>
#include <ork/opq.h>
#include <ork/timer.h>
#include <assert.h>
#include <ork/future.hpp>
//#include <sys/prctl.h>

///////////////////////////////////////////////////////////////////////////

OpGroup::OpGroup(OpMultiQ*popq, const char* pname)
	: mpOpQ(popq)
	, mLimitMaxOpsInFlight(0)
	, mLimitMaxOpsQueued(0)
	, mGroupName(pname)
	, mEnabled(true)
{
	mOpsPendingCounter = 0;
	mOpsInFlightCounter = 0;
	mOpSerialIndex = 0;
}

///////////////////////////////////////////////////////////////////////////

void OpGroup::push(const Op& the_op)
{
	if( mEnabled )
	{
		////////////////////////////////
		// throttle it (limit number of ops in queue)
		////////////////////////////////
		if( mLimitMaxOpsQueued )
		{
			while( int(mOpsPendingCounter) > int(mLimitMaxOpsQueued) )
			{
				usleep(100);
			}
		}
		////////////////////////////////

		mOps.push(the_op);

		mOpsPendingCounter.fetch_and_increment();

		mOpSerialIndex++;

		mpOpQ->notify_one();
	}
}

///////////////////////////////////////////////////////////////////////////

void OpGroup::drain()
{
	while(int(mOpsPendingCounter))
	{
		usleep(100);
	}
	//printf( "OpGroup::drain count<%d>\n", int(mSynchro.mOpCounter));
	//OpqDrained pred_is_drained;
	//mSynchro.WaitOnCondition(pred_is_drained);
}

///////////////////////////////////////////////////////////////////////////

bool OpGroup::try_pop( Op& out_op )
{
	bool rval = mOps.try_pop(out_op);

	return rval;
}

///////////////////////////////////////////////////////////////////////////
