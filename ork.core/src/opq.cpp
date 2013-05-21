///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <ork/types.h>
#include <ork/opq.h>
#include <ork/nf_assert.h>
#include <ork/timer.h>
#include <assert.h>
#include <ork/future.hpp>
#include <ork/atomic.h>

#if ! defined(OSX)
#include <sys/prctl.h>
#endif

///////////////////////////////////////////////////////////////////////////

void SetCurrentThreadName(const char* threadName)
{
#if ! defined(OSX)
	static const int  kMAX_NAME_LEN = 15;
	char name[kMAX_NAME_LEN+1];
	for( int i=0; i<kMAX_NAME_LEN; i++ ) name[i]=0;
	strncpy(name,threadName,kMAX_NAME_LEN);
	name[kMAX_NAME_LEN]=0;
	prctl(PR_SET_NAME,(unsigned long)&name);
#endif
}

////////////////////////////////////////////////////////////////////////////////
Op::Op(const void_lambda_t& op,const std::string& name)
	: mName(name)
{
	SetOp(op);
}
////////////////////////////////////////////////////////////////////////////////
//Op::Op(const void_block_t& op,const std::string& name)
//	: mName(name)
//{
//	if( op )
//		SetOp(Block_copy(op));
//	else
//		SetOp(NOP());
//}
////////////////////////////////////////////////////////////////////////////////
Op::Op(const BarrierSyncReq& op,const std::string& name)
	: mName(name)
{
	SetOp(op);
}
////////////////////////////////////////////////////////////////////////////////
Op::Op(const Op& oth)
	: mName(oth.mName)
{
	if( oth.mWrapped.IsA<void_lambda_t>() )
	{
		SetOp(oth.mWrapped.Get<void_lambda_t>());
	}
	//else if( oth.mWrapped.IsA<void_block_t>() )
	//{
	//	SetOp(oth.mWrapped.Get<void_block_t>());
	//}
	else if( oth.mWrapped.IsA<BarrierSyncReq>() )
	{
		SetOp(oth.mWrapped.Get<BarrierSyncReq>());
	}
	else if( oth.mWrapped.IsA<NOP>() )
	{
		SetOp(oth.mWrapped.Get<NOP>());
	}
	else // unhandled op type
	{
		assert(false);
	}
}
////////////////////////////////////////////////////////////////////////////////
Op::Op()
{
}
////////////////////////////////////////////////////////////////////////////////
Op::~Op()
{
}
////////////////////////////////////////////////////////////////////////////////
void Op::SetOp(const op_wrap_t& op)
{
	if( op.IsA<void_lambda_t>() )
	{
		mWrapped = op;
	}
	//else if( op.IsA<void_block_t>() )
	//{
	//	mWrapped = op;
	//}
	else if( op.IsA<BarrierSyncReq>() )
	{
		mWrapped = op;
	}
	else if( op.IsA<NOP>() )
	{
		mWrapped = op;
	}
	else // unhandled op type
	{
		assert(false);
	}
}
///////////////////////////////////////////////////////////////////////////
void Op::QueueASync(OpMultiQ&q) const
{
	q.push(*this);
}
void Op::QueueSync(OpMultiQ&q) const
{
	//AssertNotOnOpQ(q); // we dont have orkid's contextTLS template.. should we import it?
	q.push(*this);
	ork::Future the_fut;
	BarrierSyncReq R(the_fut);
	q.push(R);
	the_fut.GetResult();
}

///////////////////////////////////////////////////////////////////

struct OpqThreadData
{
	OpMultiQ* mpOpQ;
	int miThreadID;
	OpqThreadData() 
		: mpOpQ(nullptr)
		, miThreadID(0)
	{}
};

///////////////////////////////////////////////////////////////////

//struct OpqDrained : public IOpqSynchrComparison
//{
//	bool IsConditionMet(const OpqSynchro& synchro) const
//	{
//		return (int(synchro.mOpCounter)<=0);
//	}
//};

///////////////////////////////////////////////////////////////////////////

void* OpqThreadImpl( void* arg_opaq )
{
	OpqThreadData* opqthreaddata = (OpqThreadData*) arg_opaq;
	OpMultiQ* popq = opqthreaddata->mpOpQ;
	std::string opqn = popq->mName;
	SetCurrentThreadName( opqn.c_str() );


	static int icounter = 0;
	int thid = opqthreaddata->miThreadID+4;
	std::string channam = ork::FormatString("opqth%d",int(thid));

	////////////////////////////////////////////////
	// main opq loop
	////////////////////////////////////////////////

	popq->mThreadsRunning.fetch_and_increment();
	while(false==popq->mbOkToExit)
	{
		popq->BlockingIterate(thid);
	}
	popq->mThreadsRunning.fetch_and_decrement();

	////////////////////////////////////////////////

	//printf( "popq<%p> thread exiting...\n", popq );

	return (void*) 0;

}

///////////////////////////////////////////////////////////////////////////

void OpMultiQ::notify_all()
{
	//mOpsPendingCounter.fetch_and_increment();
}
void OpMultiQ::notify_one()
{
	//lock_t lock(mOpWaitMtx);
	//mOpWaitCV.notify_one();
	//mOpsPendingCounter.fetch_and_increment();
	mOpsPendingCounter2.fetch_and_increment();
}

///////////////////////////////////////////////////////////////////////////

void OpMultiQ::BlockingIterate(int thid)
{
	/////////////////////////////////////////////////
	// check "is exiting" status
	/////////////////////////////////////////////////

	if( mbOkToExit )
		return;

	static ork::atomic<int> gopctr;
	int iopctr = gopctr.fetch_and_increment();
	
	/////////////////////////////////////////////////
	// find a group with an op on it
	/////////////////////////////////////////////////

	OpGroup* exec_grp = nullptr;

	bool have_exclusive_ownership_of_group = false;

	Op the_op;

	ork::Timer tmr_grp_outer;
	tmr_grp_outer.Start();

	//const int ksleepar[9] = {10,21,43,87,175,351,701,1403,3001};
	//const int ksleepar[9] = {10,17,23,27,35,151,301,603,1201};
	const int ksleepar[9] = {10,17,23,27,35,151,301,603,1201};
	static const int ksh = 0;

	int iouterattempt = 0;
	while( (exec_grp == nullptr) && (false==mbOkToExit) )
	{
		/////////////////////////////////////////
		// get a group to test
		/////////////////////////////////////////

		OpGroup* test_grp = nullptr;
		int iinnerattempt = 0;
		ork::Timer tmr_grp_inner;
		tmr_grp_inner.Start();
		while( (false == mOpGroupRing.try_pop(test_grp)) && (false==mbOkToExit) )
		{
			/////////////////////////////////////////
			// check for stall
			/////////////////////////////////////////

			if( tmr_grp_inner.SecsSinceStart() > 3.0f )
			{
				nf_assert(false,"OPQ stall detected (getting group:inner) thid<%d> gopctr<%d> iouterattempt<%d>\n", thid, iopctr, iouterattempt);
				tmr_grp_inner.Start();
			}
			/////////////////////////////////////////
			// sleep semirandom amt of time for thread dispersion
			/////////////////////////////////////////
			usleep(ksleepar[(iinnerattempt>>ksh)%9]);
			iinnerattempt++;
		}

		/////////////////////////////////////////

		if( mbOkToExit ) return; // exiting ?

		/////////////////////////////////////////
		// we have a group to test
		/////////////////////////////////////////

		assert(test_grp!=nullptr);

		int imax = test_grp->mLimitMaxOpsInFlight;
		const char* grp_name = test_grp->mGroupName.c_str();

		/////////////////////////////////////////

		int inumopsalreadyinflight = test_grp->mOpsInFlightCounter.fetch_and_increment();
		int igrpnumopspending = test_grp->mOpsPendingCounter.fetch_and_decrement();

		bool exceeded_max_concurrent = ((imax!=0) && (inumopsalreadyinflight >= imax));
		bool reached_max_concurrent = ((imax!=0) && ((inumopsalreadyinflight+1) >= imax));

		if( (false==exceeded_max_concurrent) && (igrpnumopspending!=0) )
		{
			exec_grp = test_grp;

			if( reached_max_concurrent )
				/////////////////////////////////////////
				// defer group return until operation complete
				//   (limit concurrent ops on this group)
				/////////////////////////////////////////
				have_exclusive_ownership_of_group = true; 
			else
				/////////////////////////////////////////
				// return group now 
				//   (allow more concurrent ops on this group)
				/////////////////////////////////////////
				mOpGroupRing.push(exec_grp);

			ork::Timer tmr_op;
			tmr_op.Start();

			int iiiattempt = 0;

			while( (false==exec_grp->try_pop(the_op)) && (false==mbOkToExit) )
			{	if( tmr_op.SecsSinceStart() > 3.0f )
				{	nf_assert(false,"OPQ stall detected (getting op) thid<%d> grp<%s> goif<%d> gopspend<%d> gopctr<%d> iouterattempt<%d> iiiattempt<%d>\n", thid, grp_name, inumopsalreadyinflight, igrpnumopspending, iopctr, iouterattempt, iiiattempt);
					tmr_op.Start();
				}
				/////////////////////////////////////////
				// sleep semirandom amt of time for thread dispersion
				/////////////////////////////////////////
				usleep(ksleepar[(iiiattempt>>ksh)%9]);
				iiiattempt++;
			}
			////////////////////////////////
			if( mbOkToExit ) return; // early exit ?
			////////////////////////////////

			goto ok_to_go;
		}

		/////////////////////////////////////////
		// not processing an op this time,
		//  undo and atomic modifications
		//  and move on to next group 
		/////////////////////////////////////////

		test_grp->mOpsInFlightCounter.fetch_and_decrement();
		test_grp->mOpsPendingCounter.fetch_and_increment();


		mOpGroupRing.push(test_grp);

		/////////////////////////////////////////
		// sleep semirandom amt of time for thread dispersion
		/////////////////////////////////////////

		usleep(ksleepar[(iouterattempt>>ksh)%9]);
		iouterattempt++;

		/////////////////////////////////////////
		// check for idle
		/////////////////////////////////////////

		if( tmr_grp_outer.SecsSinceStart() > 10.0f )
		{
			nf_assert(false,"OPQTHR::IDLE thid<%d> gopctr<%d> iouterattempt<%d>\n", thid, iopctr, iouterattempt);
			tmr_grp_outer.Start();
		}
		/////////////////////////////////////////
	}
ok_to_go:
	if(mbOkToExit) return;
	assert(exec_grp!=nullptr);
	
	const int kmaxperquanta = 64;

	ProcessOne(exec_grp,the_op);

	int inumthisquanta = 1;

	if( have_exclusive_ownership_of_group ) // we exclusively own this group, milk it
	{
		ork::Timer quanta_timer;
		quanta_timer.Start();

		bool bkeepgoing = true;

		while( bkeepgoing )
		{
			int igrpnumopspending = exec_grp->mOpsPendingCounter.fetch_and_decrement();
			if( igrpnumopspending>0 )
			{
				bool bgotone = exec_grp->try_pop(the_op);
				assert(bgotone); 
				exec_grp->mOpsInFlightCounter.fetch_and_increment();
				ProcessOne(exec_grp,the_op);
				inumthisquanta++;
				bkeepgoing = (inumthisquanta<kmaxperquanta) && (quanta_timer.SecsSinceStart()<0.001);
			}
			else
			{	bkeepgoing = false;
				exec_grp->mOpsPendingCounter.fetch_and_increment();
			}
		}

		mOpGroupRing.push(exec_grp); // release the group
	}
}


///////////////////////////////////////////////////////////////////////////

void OpMultiQ::ProcessOne(OpGroup*pexecgrp,const Op& the_op)
{	/////////////////
	ork::Timer tmr;
	tmr.Start();
	/////////////////
	//int inumops = pexecgrp->NumOps();
	//printf( "  runop group<%s> OIF<%d> gctr<%d> npend<%d>\n", grp_name, int(pexecgrp->mOpsInFlightCounter), int(gctr), inumops );
	/////////////////////////////////////////////////
	// get debug names
	/////////////////////////////////////////////////
	const char* grp_name = pexecgrp->mGroupName.c_str();
	const char* op_nam = the_op.mName.length() ? the_op.mName.c_str() : "opx";
	/////////////////////////////////////////////////
	// we only support 3 op types currently
	//   lambdas / blocks / and barrier-sync ops
	/////////////////////////////////////////////////
	if( the_op.mWrapped.IsA<void_lambda_t>() )
	{	the_op.mWrapped.Get<void_lambda_t>()();
	}
	else if( the_op.mWrapped.IsA<BarrierSyncReq>() )
	{	auto& R = the_op.mWrapped.Get<BarrierSyncReq>();
		R.mFuture.Signal<bool>(true);
	}
	else if( the_op.mWrapped.IsA<NOP>() ) {}
	else
	{	printf( "unknown opq invokable type\n" );
		assert(false);
	}
	/////////////////////////////////////////////////
	pexecgrp->mOpsInFlightCounter.fetch_and_decrement();
	this->mOpsPendingCounter2.fetch_and_decrement();
}

///////////////////////////////////////////////////////////////////////////

void OpMultiQ::push(const Op& the_op)
{
	mDefaultGroup->push(the_op);
}

///////////////////////////////////////////////////////////////////////////

void OpMultiQ::sync()
{
	//AssertNotOnOpQ(*this);
	ork::Future the_fut;
	BarrierSyncReq R(the_fut);
	push(R);
	the_fut.GetResult();
}

///////////////////////////////////////////////////////////////////////////

void OpMultiQ::drain()
{
	while(int(mOpsPendingCounter2))
	{
		usleep(100);
	}
	//printf( "Opq::drain count<%d>\n", int(mSynchro.mOpCounter));
	//OpqDrained pred_is_drained;
	//mSynchro.WaitOnCondition(pred_is_drained);
}

///////////////////////////////////////////////////////////////////////////

OpGroup* OpMultiQ::CreateOpGroup(const char* pname)
{
	OpGroup* pgrp = new OpGroup(this,pname);
	mOpGroups.insert(pgrp);
	mGroupCounter++;

	mOpGroupRing.push(pgrp);
	return pgrp;
}

///////////////////////////////////////////////////////////////////////////

OpMultiQ::OpMultiQ(int inumthreads,const char* name)
	: mbOkToExit(false)
	, mName(name)
{
	mOpsPendingCounter2 = 0;
	//mOpsPendingCounter = 0;
	mGroupCounter = 0;
	mThreadsRunning = 0;

	mDefaultGroup = CreateOpGroup("defconq");

	//sem_init(&mSemaphore,0,0);

	for( int i=0; i<inumthreads; i++ )
	{
		OpqThreadData* thd = new OpqThreadData;
		thd->miThreadID = i;

		thd->mpOpQ = this;
	    pthread_t thread_handle;  
	    int rc = pthread_create(&thread_handle, NULL, OpqThreadImpl, (void*)thd);
	}
}

///////////////////////////////////////////////////////////////////////////

OpMultiQ::~OpMultiQ()
{
	/////////////////////////////////
	// dont accept any more ops
	/////////////////////////////////

	for( auto& grp : mOpGroups )
	{
		grp->Disable();
	}

	/////////////////////////////////
	// drain what ops we have
	/////////////////////////////////

	drain();

	/////////////////////////////////
	// Signal to worker threads that we are ready to go down
	// Spam the worker thread semaphore until it does go down
	// Wait until all worker threads have exited
	/////////////////////////////////

	mbOkToExit = true;

	while(int(mThreadsRunning)!=0)
	{
		notify_all();
		usleep(10);
	}


	/////////////////////////////////
	// trash the groups
	/////////////////////////////////

	OpGroup* pgrp = nullptr;

	for( auto& it : mOpGroups )
	{
		delete it;
	}

	/////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////
